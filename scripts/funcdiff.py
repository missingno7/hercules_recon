"""Planning-only instruction difference classifier for reconstructed functions.

This tool freshly compiles each recorded source/profile group once, compares
complete /Gy COFF contributions with their native spans, and reports
instruction-level differences. It never updates recovery state or proves
function identity.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import difflib
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86_const import X86_OP_IMM, X86_OP_MEM, X86_OP_REG

from env import ROOT
from coff import COFF
from match import DEFAULT_TOOLCHAIN, compile_source
from pe import PE


RELOC_WIDTH = {
    0x0001: 2,   # DIR16
    0x0002: 2,   # REL16
    0x0006: 4,   # DIR32
    0x0007: 4,   # DIR32NB
    0x000A: 2,   # SECTION
    0x000B: 4,   # SECREL
    0x000C: 4,   # TOKEN
    0x000D: 1,   # SECREL7
    0x0014: 4,   # REL32
}
STACK_BASES = {"esp", "ebp"}
ALLOCATOR_CLASSES = {"register only", "stack-offset only"}
REAL_CLASSES = {"immediate only", "branch-target only", "different mnemonic/opcode", "unaligned"}


def _digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _fmt_offset(value: int | None) -> str | None:
    return None if value is None else f"0x{value:x}"


def _insn_text(insn) -> str:
    return insn.mnemonic + (" " + insn.op_str if insn.op_str else "")


def _relocation_ranges(relocations):
    return [(int(r["offset"]), int(r["offset"]) + RELOC_WIDTH.get(int(r["type"]), 4), r)
            for r in relocations]


def _decode(data: bytes, base: int, relocations=()):
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    reloc_ranges = _relocation_ranges(relocations)
    result = []
    consumed = 0
    for insn in md.disasm(data, base):
        offset = insn.address - base
        item = {
            "insn": insn,
            "offset": offset,
            "size": insn.size,
            "bytes": bytes(insn.bytes),
            "text": _insn_text(insn),
            "relocation_ranges": [
                {"offset": lo, "size": hi - lo, "symbol": r["symbol"], "type": r["type"]}
                for lo, hi, r in reloc_ranges
                if lo < offset + insn.size and offset < hi
            ],
        }
        item["key"] = _alignment_key(item)
        result.append(item)
        consumed = offset + insn.size
    return result, data[consumed:]


def _alignment_key(item):
    """Instruction mnemonic and operand shape, with allocation/value fields masked."""
    insn = item["insn"]
    operands = []
    is_direct_branch = insn.mnemonic in {
        "call", "jmp", "ja", "jae", "jb", "jbe", "jc", "jcxz", "je", "jecxz",
        "jg", "jge", "jl", "jle", "jna", "jnae", "jnb", "jnbe", "jnc", "jne",
        "jng", "jnge", "jnl", "jnle", "jno", "jnp", "jns", "jnz", "jo", "jp",
        "jpe", "jpo", "js", "jz", "loop", "loope", "loopne", "loopnz", "loopz",
    }
    for op in insn.operands:
        if op.type == X86_OP_REG:
            operands.append(("reg", op.size))
        elif op.type == X86_OP_IMM:
            operands.append(("branch-target" if is_direct_branch else "imm", op.size))
        elif op.type == X86_OP_MEM:
            base = insn.reg_name(op.mem.base) if op.mem.base else ""
            index = insn.reg_name(op.mem.index) if op.mem.index else ""
            if base in STACK_BASES:
                disp_shape = "stack-offset"
            else:
                # Displacements are value fields, not operand structure. Keeping
                # their value out of alignment lets us classify them after pairing.
                disp_shape = "disp"
            operands.append(("mem", op.size, bool(base), bool(index), op.mem.scale,
                             bool(op.mem.segment), disp_shape))
        else:
            operands.append(("other", op.size, op.type))
    return insn.mnemonic, tuple(operands)


def _operand_registers(insn):
    """Yield register locations and names in stable operand order."""
    result = []
    for index, op in enumerate(insn.operands):
        if op.type == X86_OP_REG:
            result.append(((index, "reg"), insn.reg_name(op.reg)))
        elif op.type == X86_OP_MEM:
            for field, value in (("base", op.mem.base), ("index", op.mem.index),
                                 ("segment", op.mem.segment)):
                if value:
                    result.append(((index, field), insn.reg_name(value)))
    return result


def _target_offset(item, base):
    insn = item["insn"]
    if not insn.operands or insn.operands[0].type != X86_OP_IMM:
        return None
    return int(insn.operands[0].imm) - base


def _target_index(target, instructions):
    if target is None:
        return None
    for index, item in enumerate(instructions):
        if item["offset"] == target:
            return index
    return None


def _relocation_covers_byte(item, compiled_offset, byte_index):
    absolute = compiled_offset + byte_index
    return any(row["offset"] <= absolute < row["offset"] + row["size"]
               for row in item["relocation_ranges"])


def _pair_class(native, ours, native_base, ours_base, native_instructions,
                ours_instructions, native_to_ours):
    ni, oi = native["insn"], ours["insn"]

    # COFF relocation fields can encode an immediate, moffs address, memory
    # displacement, or relative branch field. Decide from byte coverage before
    # Capstone operand form checks so equivalent encodings are treated alike.
    limit = max(len(native["bytes"]), len(ours["bytes"]))
    changed_bytes = [i for i in range(limit)
                     if (native["bytes"][i] if i < len(native["bytes"]) else None) !=
                     (ours["bytes"][i] if i < len(ours["bytes"]) else None)]
    if (changed_bytes and all(i < len(ours["bytes"])
                              and _relocation_covers_byte(ours, ours["offset"], i)
                              for i in changed_bytes)):
        return "relocation-operand only", []

    if ni.mnemonic != oi.mnemonic:
        return "different mnemonic/opcode", []

    # First identify field differences, retaining enough detail to build maps.
    differences = set()
    register_pairs = []
    slot_pairs = []
    equivalent_branch_ranges = []
    if len(ni.operands) != len(oi.operands):
        return "different mnemonic/opcode", []

    branch_changed = False
    for index, (nop, oop) in enumerate(zip(ni.operands, oi.operands)):
        if nop.type != oop.type or nop.size != oop.size:
            return "different mnemonic/opcode", []
        if nop.type == X86_OP_REG:
            nr, or_ = ni.reg_name(nop.reg), oi.reg_name(oop.reg)
            if nr != or_:
                differences.add("register")
                register_pairs.append((nr, or_))
        elif nop.type == X86_OP_MEM:
            nfields = (nop.mem.scale, bool(nop.mem.segment))
            ofields = (oop.mem.scale, bool(oop.mem.segment))
            if nfields != ofields:
                return "different mnemonic/opcode", []
            nb = ni.reg_name(nop.mem.base) if nop.mem.base else ""
            ob = oi.reg_name(oop.mem.base) if oop.mem.base else ""
            nx = ni.reg_name(nop.mem.index) if nop.mem.index else ""
            ox = oi.reg_name(oop.mem.index) if oop.mem.index else ""
            ns = ni.reg_name(nop.mem.segment) if nop.mem.segment else ""
            os_ = oi.reg_name(oop.mem.segment) if oop.mem.segment else ""
            for nreg, oreg in ((nb, ob), (nx, ox), (ns, os_)):
                if nreg != oreg:
                    differences.add("register")
                    if nreg and oreg:
                        register_pairs.append((nreg, oreg))
            if nop.mem.disp != oop.mem.disp:
                if nb in STACK_BASES or ob in STACK_BASES:
                    differences.add("stack")
                    if nb == ob == "esp":
                        slot_pairs.append((int(nop.mem.disp), int(oop.mem.disp)))
                else:
                    # The listed classes have no separate non-stack displacement
                    # bucket; this is an encoded value-only operand difference.
                    differences.add("immediate")
        elif nop.type == X86_OP_IMM:
            if int(nop.imm) != int(oop.imm):
                direct_branch = ni.mnemonic in {
                    "call", "jmp", "ja", "jae", "jb", "jbe", "jc", "jcxz", "je", "jecxz",
                    "jg", "jge", "jl", "jle", "jna", "jnae", "jnb", "jnbe", "jnc", "jne",
                    "jng", "jnge", "jnl", "jnle", "jno", "jnp", "jns", "jnz", "jo", "jp",
                    "jpe", "jpo", "js", "jz", "loop", "loope", "loopne", "loopnz", "loopz",
                }
                if direct_branch and index == 0:
                    nt = _target_offset(native, native_base)
                    ot = _target_offset(ours, ours_base)
                    nti = _target_index(nt, native_instructions)
                    oti = _target_index(ot, ours_instructions)
                    if nti is not None and oti is not None and native_to_ours.get(nti) == oti:
                        encoding = oi.encoding
                        if encoding.imm_size:
                            equivalent_branch_ranges.append(
                                (encoding.imm_offset, encoding.imm_offset + encoding.imm_size))
                        continue
                    branch_changed = True
                    differences.add("branch")
                else:
                    differences.add("immediate")

    if not differences:
        # Equivalent decoded instructions can use distinct encodings. Preserve
        # unexplained residue as an opcode difference. A relative branch whose
        # target is the same aligned instruction is semantically identical.
        if native["bytes"] != ours["bytes"]:
            changed = {i for i, (nb, ob) in enumerate(zip(native["bytes"], ours["bytes"])) if nb != ob}
            if not (len(native["bytes"]) == len(ours["bytes"]) and equivalent_branch_ranges and
                    all(any(lo <= i < hi for lo, hi in equivalent_branch_ranges) for i in changed)):
                return "different mnemonic/opcode", []
        return "identical", []
    if differences == {"register"}:
        return "register only", register_pairs
    if differences == {"stack"}:
        return "stack-offset only", slot_pairs
    if differences == {"immediate"}:
        return "immediate only", []
    if differences == {"branch"}:
        return "branch-target only", []
    return "different mnemonic/opcode", []


def _entry(native, ours, cls, moved=False, map_pairs=()):
    return {
        "class": cls,
        "native_offset": None if native is None else _fmt_offset(native["offset"]),
        "compiled_offset": None if ours is None else _fmt_offset(ours["offset"]),
        "native_instruction": None if native is None else native["text"],
        "compiled_instruction": None if ours is None else ours["text"],
        "native_bytes": None if native is None else native["bytes"].hex(),
        "compiled_bytes": None if ours is None else ours["bytes"].hex(),
        "moved": bool(moved),
        "map_pairs": list(map_pairs),
    }


def _first_real_difference(listing, reordered):
    return next((row for row in listing
                 if row["class"] in REAL_CLASSES or
                 (reordered and row["moved"] and row["class"] != "relocation-operand only")), None)


def align_and_classify(native_bytes: bytes, compiled_bytes: bytes, *, native_base=0,
                       compiled_base=0, relocations=()):
    """Return an instruction alignment and summary for synthetic or real bytes."""
    native, native_tail = _decode(native_bytes, native_base)
    ours, ours_tail = _decode(compiled_bytes, compiled_base, relocations)
    nkeys = [item["key"] for item in native]
    okeys = [item["key"] for item in ours]
    matcher = difflib.SequenceMatcher(None, nkeys, okeys, autojunk=False)
    pairs = []
    unmatched_native, unmatched_ours = [], []
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            pairs.extend((i, j, False) for i, j in zip(range(i1, i2), range(j1, j2)))
        elif tag == "replace":
            count = min(i2 - i1, j2 - j1)
            pairs.extend((i1 + k, j1 + k, False) for k in range(count))
            unmatched_native.extend(range(i1 + count, i2))
            unmatched_ours.extend(range(j1 + count, j2))
        elif tag == "delete":
            unmatched_native.extend(range(i1, i2))
        elif tag == "insert":
            unmatched_ours.extend(range(j1, j2))

    # Pair instructions displaced by code motion using their masked signature.
    # Sorting within each signature preserves duplicate occurrence order.
    n_by_key, o_by_key = defaultdict(list), defaultdict(list)
    for i in unmatched_native:
        n_by_key[nkeys[i]].append(i)
    for j in unmatched_ours:
        o_by_key[okeys[j]].append(j)
    moved_native, moved_ours = set(), set()
    for key in n_by_key.keys() & o_by_key.keys():
        left, right = n_by_key[key], o_by_key[key]
        for i, j in zip(left, right):
            pairs.append((i, j, True))
            moved_native.add(i)
            moved_ours.add(j)

    unmatched_native = [i for i in unmatched_native if i not in moved_native]
    unmatched_ours = [j for j in unmatched_ours if j not in moved_ours]
    # Remaining slots in a SequenceMatcher replace region have already been
    # paired positionally. True residuals are insertions/deletions.
    pairs.sort(key=lambda p: (p[0], p[1]))
    native_to_ours = {i: j for i, j, _ in pairs}

    listing = []
    register_pairs, slot_pairs = [], []
    for i, j, moved in pairs:
        cls, maps = _pair_class(native[i], ours[j], native_base, compiled_base,
                                native, ours, native_to_ours)
        listing.append(_entry(native[i], ours[j], cls, moved, maps))
        if cls == "register only":
            register_pairs.extend(maps)
        elif cls == "stack-offset only":
            slot_pairs.extend(maps)
    for i in unmatched_native:
        listing.append(_entry(native[i], None, "unaligned"))
    for j in unmatched_ours:
        listing.append(_entry(None, ours[j], "unaligned"))
    if native_tail:
        listing.append({"class": "unaligned", "native_offset": _fmt_offset(len(native_bytes)-len(native_tail)),
                        "compiled_offset": None, "native_instruction": "<undecoded: " + native_tail.hex() + ">",
                        "compiled_instruction": None, "native_bytes": native_tail.hex(),
                        "compiled_bytes": None, "moved": False, "map_pairs": []})
    if ours_tail:
        listing.append({"class": "unaligned", "native_offset": None,
                        "compiled_offset": _fmt_offset(len(compiled_bytes)-len(ours_tail)),
                        "native_instruction": None, "compiled_instruction": "<undecoded: " + ours_tail.hex() + ">",
                        "native_bytes": None, "compiled_bytes": ours_tail.hex(),
                        "moved": False, "map_pairs": []})
    listing.sort(key=lambda row: (float("inf") if row["native_offset"] is None else int(row["native_offset"], 16),
                                  float("inf") if row["compiled_offset"] is None else int(row["compiled_offset"], 16)))

    classes = Counter(row["class"] for row in listing)
    mappings = defaultdict(set)
    reverse = defaultdict(set)
    for native_reg, compiled_reg in register_pairs:
        mappings[native_reg].add(compiled_reg)
        reverse[compiled_reg].add(native_reg)
    register_map = {key: sorted(values) for key, values in sorted(mappings.items())}
    register_consistent = all(len(v) == 1 for v in mappings.values()) and all(len(v) == 1 for v in reverse.values())
    slot_mapping = defaultdict(set)
    for native_slot, compiled_slot in slot_pairs:
        slot_mapping[str(native_slot)].add(compiled_slot)
    stack_slot_map = {key: sorted(values) for key, values in sorted(slot_mapping.items(), key=lambda x: int(x[0]))}

    moved_count = sum(1 for row in listing if row["moved"])
    same_multiset = Counter(nkeys) == Counter(okeys)
    reordered = same_multiset and nkeys != okeys
    # This is the requested allocator-only predicate over non-identical pairs
    # and equal byte extents; order changes are separately exposed as scheduling.
    permutation_only = (
        len(native_bytes) == len(compiled_bytes) and
        all(row["class"] == "identical" or row["class"] in ALLOCATOR_CLASSES for row in listing)
    )
    first_real = _first_real_difference(listing, reordered)
    dominant = _dominant_class(classes)
    meaningful_count = sum(classes[c] for c in REAL_CLASSES)
    return {
        "classes": dict(sorted(classes.items())),
        "dominant_class": dominant,
        "first_real_difference": None if first_real is None else {
            "native_offset": first_real["native_offset"],
            "compiled_offset": first_real["compiled_offset"],
            "native_instruction": first_real["native_instruction"],
            "compiled_instruction": first_real["compiled_instruction"],
        },
        "permutation_only": permutation_only,
        "register_map": register_map,
        "register_map_consistent": register_consistent,
        "stack_slot_map": stack_slot_map,
        "native_instruction_count": len(native),
        "compiled_instruction_count": len(ours),
        "native_size": len(native_bytes),
        "compiled_size": len(compiled_bytes),
        "size_equal": len(native_bytes) == len(compiled_bytes),
        "same_instruction_multiset": same_multiset,
        "reordered": reordered,
        "moved_pair_count": moved_count,
        "real_difference_count": meaningful_count,
        "listing": listing,
    }


def _dominant_class(counts):
    if not counts:
        return "identical"
    order = ["different mnemonic/opcode", "branch-target only", "immediate only",
             "stack-offset only", "register only", "relocation-operand only", "unaligned", "identical"]
    return sorted(counts, key=lambda key: (-counts[key], order.index(key) if key in order else 99))[0]


def _family(result):
    if not result["size_equal"]:
        return "extent"
    classes = result["classes"]
    changed = {key for key, value in classes.items() if value and key != "identical"}
    if changed and changed <= ALLOCATOR_CLASSES:
        return "allocator-only"
    if result["same_instruction_multiset"] and result["reordered"]:
        return "scheduling"
    return "real logic"


def _load_rows(ids):
    state = json.loads((ROOT / "recovery.json").read_text(encoding="utf-8"))
    by_id = {row["id"]: row for row in state["functions"]}
    missing = [ident for ident in ids if ident not in by_id]
    if missing:
        raise ValueError("Unknown recovery IDs: " + ", ".join(missing))
    return state, [by_id[ident] for ident in ids]


def _compile_groups(rows):
    groups = {}
    for row in rows:
        key = (str((ROOT / row["source"]).resolve()), tuple(row["flags"]))
        groups.setdefault(key, []).append(row)
    objects, errors = {}, {}
    calls = 0
    # The user-specified metrics directory is the default for both CLI modes.
    os.environ["HERCULES_COMPILE_METRICS_DIR"] = "work/velocity_e/compiler_events"
    for key, members in groups.items():
        source, flags = key
        try:
            obj, command = compile_source(source, list(flags), DEFAULT_TOOLCHAIN)
            calls += 1
            objects[key] = {"path": obj, "command": command}
        except Exception as exc:
            # compile_source records failed compiler attempts in the metrics
            # directory too; count the attempted call once and retain the error.
            calls += 1
            errors[key] = f"{type(exc).__name__}: {exc}"
    return groups, objects, errors, calls


def _classify_row(row, object_record):
    target = (ROOT / row["target"]).resolve()
    target_bytes = target.read_bytes()
    if _digest(target_bytes) != row["target_sha256"]:
        raise ValueError(f"Target hash mismatch for {row['id']}: {target}")
    section = COFF(object_record["path"]).function(row["symbol"])
    native_bytes = PE(target).read_rva(int(row["rva"], 0), int(row["size"]))
    result = align_and_classify(native_bytes, section["data"],
                                native_base=int(row["rva"], 0), compiled_base=0,
                                relocations=section["relocations"])
    result.update({"id": row["id"], "source": row["source"], "symbol": row["symbol"],
                   "target": row["target"], "rva": row["rva"], "status": row["status"],
                   "family": _family(result),
                   "source_sha256": _digest((ROOT / row["source"]).read_bytes()),
                   "target_sha256": row["target_sha256"],
                   "relocation_count": len(section["relocations"])})
    result.pop("listing")
    return result


def _summary(rows, compiler_calls, groups):
    families = Counter(row["family"] for row in rows)
    dominant = Counter(row["dominant_class"] for row in rows)
    return {"row_count": len(rows), "group_count": groups, "compiler_calls": compiler_calls,
            "families": {name: families.get(name, 0) for name in
                         ("allocator-only", "scheduling", "real logic", "extent")},
            "dominant_classes": dict(sorted(dominant.items())),
            "permutation_only_count": sum(bool(row["permutation_only"]) for row in rows),
            "register_map_inconsistent_count": sum(not row["register_map_consistent"] for row in rows)}


def _write_markdown(path, rows, summary):
    lines = ["# Function difference classifier", "",
             "Planning diagnostic only; these comparisons grant no matching acceptance.", "",
             f"Rows: {summary['row_count']}  |  source/flag groups: {summary['group_count']}  |  compiler calls: {summary['compiler_calls']}", "",
             "## Counts by family", "",
             "| Family | Rows |", "|---|---:|"]
    for family in ("allocator-only", "scheduling", "real logic", "extent"):
        lines.append(f"| {family} | {summary['families'].get(family, 0)} |")
    candidates = [row for row in rows if row["family"] in {"allocator-only", "scheduling"}]
    sort_key = lambda row: (row["real_difference_count"],
                            row.get("classes", {}).get("different mnemonic/opcode", 0) +
                            row.get("classes", {}).get("immediate only", 0) +
                            row.get("classes", {}).get("branch-target only", 0),
                            row["id"])
    candidates.sort(key=sort_key)
    selected = candidates[:30]
    if len(selected) < 30:
        remaining = [row for row in rows if row not in candidates and row["family"] != "extent"]
        remaining.sort(key=sort_key)
        selected.extend(remaining[:30 - len(selected)])
    if len(selected) < 30:
        remaining = [row for row in rows if row not in candidates and row["family"] == "extent"]
        remaining.sort(key=sort_key)
        selected.extend(remaining[:30 - len(selected)])
    lines += ["", "## Most promising rows", "",
              (f"Ordered by fewest real instruction differences. {len(candidates)} rows qualify as allocator-only or scheduling; "
               "the table is completed with the closest remaining rows when fewer than 30 qualify."), "",
              "| ID | Family | Real differences | Source |", "|---|---|---:|---|"]
    for row in selected:
        source = row["source"].replace("|", "\\|")
        lines.append(f"| `{row['id']}` | {row['family']} | {row['real_difference_count']} | `{source}` |")
    if not candidates:
        lines.append("| (none) | — | — | — |")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def _revision():
    try:
        return subprocess.run(["git", "rev-parse", "HEAD"], cwd=ROOT,
                              check=True, capture_output=True, text=True).stdout.strip()
    except Exception:
        return None


def run_batch(ids, output):
    state, rows = _load_rows(ids)
    state_bytes = (ROOT / "recovery.json").read_bytes()
    recovery_sha = _digest(state_bytes)
    groups, objects, errors, calls = _compile_groups(rows)
    results = []
    for key, members in groups.items():
        if key in errors:
            for row in members:
                results.append({"id": row["id"], "source": row["source"], "family": "real logic",
                                "error": errors[key], "classes": {}, "first_real_difference": None,
                                "permutation_only": False, "register_map": {},
                                "register_map_consistent": False, "real_difference_count": 0})
            continue
        for row in members:
            try:
                results.append(_classify_row(row, objects[key]))
            except Exception as exc:
                results.append({"id": row["id"], "source": row["source"], "family": "real logic",
                                "error": f"{type(exc).__name__}: {exc}", "classes": {},
                                "first_real_difference": None, "permutation_only": False,
                                "register_map": {}, "register_map_consistent": False,
                                "real_difference_count": 0})
    results.sort(key=lambda result: next(i for i, row in enumerate(rows) if row["id"] == result["id"]))
    if (ROOT / "recovery.json").read_bytes() != state_bytes:
        raise RuntimeError("recovery.json changed during classification")
    summary = _summary(results, calls, len(groups))
    report = {"schema_version": 1, "diagnostic_only": True, "base_revision": _revision(),
              "recovery_sha256": recovery_sha, "compiler_calls": calls,
              "summary": summary, "rows": results,
              "compile_errors": {"/".join([source, ",".join(flags)]): error
                                 for (source, flags), error in errors.items()}}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    md_path = ROOT / "work/velocity_e/classify/summary.md"
    _write_markdown(md_path, results, summary)
    handoff = {
        "task_id": "velocity_e_funcdiff_classifier",
        "function_ids": ids,
        "base_revision": report["base_revision"],
        "recovery_sha256": recovery_sha,
        "owned_files": ["scripts/funcdiff.py", "tests/test_funcdiff.py"],
        "outputs": [str(output.relative_to(ROOT)), "work/velocity_e/classify/summary.md"],
        "candidate_paths": [],
        "binding_paths": [],
        "native_review_paths": [],
        "compiler_calls": calls,
        "source_flag_groups": len(groups),
        "verified_result": {
            "python scripts/funcdiff.py --ids-file work/velocity_e/near_ids.json --output work/velocity_e/classify/near.json":
                f"completed: {summary['row_count']} rows; {len(errors)} group compile errors; {sum(bool(r.get('error')) for r in results)} row errors",
            "python -m unittest tests.test_funcdiff -q": "pending",
        },
        "family_counts": summary["families"],
        "input_ids_sha256": _digest((ROOT / "work/velocity_e/near_ids.json").read_bytes()),
        "source_hashes": {row["source"]: row["source_sha256"] for row in results if row.get("source_sha256")},
        "target_hashes": {row["target"]: row["target_sha256"] for row in results if row.get("target_sha256")},
        "implementation_sha256": _digest(Path(__file__).read_bytes()),
        "focused_test_sha256": _digest((ROOT / "tests/test_funcdiff.py").read_bytes()),
        "first_blocker": next((row["error"] for row in results if row.get("error")), None),
        "next_evidence": "Planning diagnostic only; review native/source semantics independently before any matching claim.",
    }
    (ROOT / "work/velocity_e/classify/handoff.json").write_text(
        json.dumps(handoff, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2, sort_keys=True))
    return report


def _load_ids(path):
    data = json.loads(path.read_text(encoding="utf-8"))
    if isinstance(data, dict):
        data = data.get("ids", data.get("function_ids"))
    if not isinstance(data, list) or not all(isinstance(item, str) for item in data):
        raise ValueError("ID file must contain a JSON list of function ID strings")
    return data


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("id", nargs="?", help="one recovery function ID")
    parser.add_argument("--ids-file", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--full", action="store_true", help="print each aligned instruction pair")
    args = parser.parse_args(argv)
    if args.ids_file:
        if args.id or not args.output:
            parser.error("batch mode requires --ids-file and --output, with no positional ID")
        run_batch(_load_ids(args.ids_file), args.output.resolve() if args.output.is_absolute() else ROOT / args.output)
        return 0
    if not args.id or args.output:
        parser.error("single-row mode requires one ID; --output is for batch mode")
    _, rows = _load_rows([args.id])
    groups, objects, errors, calls = _compile_groups(rows)
    key = next(iter(groups))
    if key in errors:
        raise RuntimeError(errors[key])
    row = rows[0]
    target = (ROOT / row["target"]).resolve()
    if _digest(target.read_bytes()) != row["target_sha256"]:
        raise ValueError("Target hash mismatch")
    section = COFF(objects[key]["path"]).function(row["symbol"])
    native = PE(target).read_rva(int(row["rva"], 0), int(row["size"]))
    result = align_and_classify(native, section["data"], native_base=int(row["rva"], 0),
                                compiled_base=0, relocations=section["relocations"])
    if args.full:
        print(json.dumps(result, indent=2))
    else:
        display = {key: value for key, value in result.items() if key != "listing"}
        display.update(id=row["id"], source=row["source"], compiler_calls=calls)
        print(json.dumps(display, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
