"""Bounded static alias/dataflow audit for the host's callback-table prefix."""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

EXPECTED_IMAGES = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "ENG3.DLL": "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
    "TITLE.DLL": "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
}
TABLE_BYTES = 1000
POINTER_BYTES = 4
CONTEXT_WRITER_RVA = 0x1BD0
CONTEXT_WRITER_SIZE = 291
DISPATCH_PREFIX_RVA = 0x5EF3
DISPATCH_PREFIX_SIZE = 21
KNOWN_CONTEXT = 0x00615C98


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def hx(value: int) -> str:
    return f"0x{value & 0xFFFFFFFF:08x}"


def discover_root(start: Path) -> Path:
    for candidate in (start.resolve(), *start.resolve().parents):
        if (candidate / "renderer_contract" / "evidence" / "compatibility.json").is_file() and \
                (candidate / "scripts" / "pe.py").is_file():
            return candidate
    raise ValueError("cannot locate renderer checkout; pass --repo-root")


def find_image_path(root: Path, relative: Path) -> Path:
    for candidate in (root, *root.parents):
        path = candidate / relative
        if path.is_file():
            return path.resolve()
    return (root / relative).resolve()


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path,
                        help="checkout containing renderer_contract and scripts/pe.py")
    parser.add_argument("--exe", type=Path, help="original pinned HERCULES.EXE")
    parser.add_argument("--eng1", type=Path, help="original pinned ENG1.DLL")
    parser.add_argument("--eng3", type=Path, help="original pinned ENG3.DLL")
    parser.add_argument("--title", type=Path, help="original pinned TITLE.DLL")
    parser.add_argument("--output", type=Path,
                        help="receipt path under checkout/work; defaults to this audit directory")
    args = parser.parse_args()
    args.repo_root = args.repo_root.resolve() if args.repo_root else discover_root(Path(__file__).resolve())
    default_paths = {
        "exe": find_image_path(args.repo_root, Path("work/discs/pc_install/HERCULES.EXE")),
        "eng1": find_image_path(args.repo_root, Path("assets/pc/HERCULES/ENG1.DLL")),
        "eng3": find_image_path(args.repo_root, Path("assets/pc/HERCULES/ENG3.DLL")),
        "title": find_image_path(args.repo_root, Path("assets/pc/HERCULES/TITLE.DLL")),
    }
    for name, fallback in default_paths.items():
        value = getattr(args, name)
        setattr(args, name, value.resolve() if value else fallback.resolve())
        if not getattr(args, name).is_file():
            parser.error(f"original image missing for --{name}: {getattr(args, name)}")
    args.output = (args.output.resolve() if args.output else
                   args.repo_root / "work" / "replay" / "interface_publication.json")
    work_path = args.repo_root / "work"
    work = work_path.resolve()
    if work != work_path:
        parser.error("refusing a redirected work directory; output must stay in checkout/work")
    try:
        args.output.relative_to(work)
    except ValueError as exc:
        parser.error("--output must be under checkout/work")
    for image_path in (args.exe, args.eng1, args.eng3, args.title):
        if args.output == image_path or args.output.is_relative_to(image_path.parent.resolve()):
            parser.error("receipt output must not be inside any original-image directory")
    return args


def section_at_va(pe, va: int):
    rva = va - pe.image_base
    for section in pe.sections:
        start = section["virtual_address"]
        raw_end = start + section["raw_size"]
        virtual_end = start + max(section["virtual_size"], section["raw_size"])
        if start <= rva < virtual_end:
            offset = rva - start
            return {
                "name": section["name"], "rva": hx(rva),
                "file_backed": rva < raw_end,
                "file_offset": hx(section["raw_offset"] + offset) if rva < raw_end else None,
                "zero_fill": rva >= raw_end,
            }
    return None


def disassemble_host(pe, capstone):
    text = next(section for section in pe.sections if section["name"] == ".text")
    raw = pe.read_rva(text["virtual_address"], text["raw_size"])
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    decoder.skipdata = True
    insns = list(decoder.disasm(raw, pe.image_base + text["virtual_address"]))
    covered = sum(insn.size for insn in insns)
    return insns, {
        "section": ".text", "rva": hx(text["virtual_address"]),
        "raw_size": text["raw_size"], "record_count": len(insns),
        "decoded_count": sum(insn.id != 0 for insn in insns),
        "skipdata_count": sum(insn.id == 0 for insn in insns),
        "covered_bytes": covered, "coverage_complete": covered == text["raw_size"],
        "limit": "Linear sweep over file-backed .text; byte coverage is not instruction-boundary proof.",
    }


def instruction_row(insn, image_base: int):
    return {"rva": hx(insn.address - image_base),
            "instruction": f"{insn.mnemonic} {insn.op_str}".strip()}


def main() -> int:
    args = parse_args()
    root = args.repo_root
    evidence_dir = root / "renderer_contract" / "evidence"
    public_inputs = {}
    loaded = {}
    for name in ("compatibility.json", "loader.json", "bootstrap_order.json",
                 "texture_ownership.json", "upload_domain_bounds.json"):
        path = evidence_dir / name
        raw = path.read_bytes()
        public_inputs[name] = {"sha256": sha(raw), "size": len(raw)}
        loaded[name] = json.loads(raw)
    compatibility = loaded["compatibility.json"]
    loader = loaded["loader.json"]
    bootstrap = loaded["bootstrap_order.json"]
    texture = loaded["texture_ownership.json"]
    upload = loaded["upload_domain_bounds.json"]

    image_paths = {"HERCULES.EXE": args.exe, "ENG1.DLL": args.eng1,
                   "ENG3.DLL": args.eng3, "TITLE.DLL": args.title}
    image_hashes = {}
    for name, path in image_paths.items():
        digest = sha(path.read_bytes())
        public_expected = compatibility["oracles"][name]["sha256"].lower()
        if digest != EXPECTED_IMAGES[name] or digest != public_expected:
            raise ValueError(f"{name} differs from the pinned public original-image manifest")
        image_hashes[name] = {"path": str(path), "sha256": digest, "size": path.stat().st_size}

    sys.path.insert(0, str(root / "scripts"))
    import env  # noqa: F401  # configures the repo's existing analysis dependency directory
    import capstone
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM
    from pe import PE

    pe = PE(args.exe)
    module_paths = {"ENG1.DLL": args.eng1, "ENG3.DLL": args.eng3, "TITLE.DLL": args.title}
    module_copies = {}
    for module_name, module_path in module_paths.items():
        module_pe = PE(module_path)
        module_contract = compatibility["modules"][module_name]
        entry_spec = module_contract["entry_span"]
        entry_rva = int(entry_spec["rva"], 16)
        entry_raw = module_pe.read_rva(entry_rva, int(entry_spec["size"]))
        if sha(entry_raw) != entry_spec["sha256"].lower():
            raise ValueError(f"{module_name} cold-entry span differs from public compatibility evidence")
        entry_decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        entry_decoder.detail = True
        entry_instructions = list(entry_decoder.disasm(entry_raw, module_pe.image_base + entry_rva))
        copy_rva = int(module_contract["copy_and_validation"]["copy_instruction_rva"], 16)
        copy_positions = [index for index, insn in enumerate(entry_instructions)
                          if insn.address - module_pe.image_base == copy_rva]
        if len(copy_positions) != 1 or entry_instructions[copy_positions[0]].mnemonic != "rep movsd":
            raise ValueError(f"{module_name} cold prefix copy is not the pinned REP MOVSD")
        copy_pos = copy_positions[0]
        window = entry_instructions[max(0, copy_pos - 5):copy_pos + 2]
        copy_insn = entry_instructions[copy_pos]
        if "[edi]" not in copy_insn.op_str or "[esi]" not in copy_insn.op_str:
            raise ValueError(f"{module_name} REP MOVSD operand direction differs")
        source_register_stores = []
        for insn in entry_instructions:
            for operand in insn.operands:
                if operand.type == X86_OP_MEM and operand.mem.base == capstone.x86_const.X86_REG_ESI and operand.access & 2:
                    source_register_stores.append({"rva": hx(insn.address - module_pe.image_base),
                                                   "instruction": f"{insn.mnemonic} {insn.op_str}"})
        module_copies[module_name] = {
            "entry_span": {"rva": entry_spec["rva"], "size": entry_spec["size"],
                           "sha256": entry_spec["sha256"]},
            "local_copy_root_rva": module_contract["copy_and_validation"]["copy_destination_rva"],
            "copy_instruction_rva": hx(copy_rva),
            "copy_instruction": f"{copy_insn.mnemonic} {copy_insn.op_str}",
            "copy_window": [instruction_row(insn, module_pe.image_base) for insn in window],
            "entry_span_memory_writes_through_esi": source_register_stores,
            "signature_check_rva": module_contract["copy_and_validation"]["signature_check_rva"],
            "copy_before_signature_check": module_contract["copy_and_validation"]["copy_before_signature_check"],
            "copy_direction": "source ESI to module-local destination EDI",
        }
    interface = compatibility["interface"]
    table_rva = int(interface["source_table_rva"], 16)
    table_va = int(interface["source_table_va"], 16)
    table_count = int(interface["dword_count"])
    table_size = table_count * 4
    pointer_rva = int(interface["pointer_slot_rva"], 16)
    pointer_va = pe.image_base + pointer_rva
    table_raw = pe.read_rva(table_rva, table_size)
    pointer_raw = pe.read_rva(pointer_rva, POINTER_BYTES)
    pointer_value = struct.unpack("<I", pointer_raw)[0]
    words = list(struct.unpack("<250I", table_raw))
    table_end_va = table_va + table_size
    if (table_size != TABLE_BYTES or sha(table_raw) != interface["source_table_sha256"].lower() or
            pointer_value != table_va or pointer_rva != table_rva + table_size):
        raise ValueError("source table or adjacent pointer cell differs from public interface evidence")
    if int(loader["table"]["rva"], 16) != table_rva or int(loader["source_pointer"]["rva"], 16) != pointer_rva:
        raise ValueError("public loader evidence disagrees with compatibility interface addresses")
    if texture["host_callback_table"]["sha256"].lower() != sha(table_raw):
        raise ValueError("public texture evidence disagrees with source-table bytes")

    public_span_pins = {}
    for label, expected_rva, expected_size in (
            ("title_entry_call_sequence", 0x5EF3, DISPATCH_PREFIX_SIZE),
            ("context_writer", CONTEXT_WRITER_RVA, CONTEXT_WRITER_SIZE)):
        candidates = [row for row in upload["original_spans"]
                      if row["module"] == "HERCULES.EXE" and row["label"] == label]
        if len(candidates) != 1:
            raise ValueError(f"public upload receipt lacks one {label} span")
        record = candidates[0]
        if int(record["rva"], 16) != expected_rva or int(record["size"]) != expected_size:
            raise ValueError(f"public upload receipt span identity changed for {label}")
        observed_span_hash = sha(pe.read_rva(expected_rva, expected_size))
        expected_span_hash = record["sha256"].lower()
        if observed_span_hash != expected_span_hash:
            raise ValueError(f"original {label} bytes differ from the public span hash pin")
        public_span_pins[label] = {
            "rva": hx(expected_rva), "size": expected_size,
            "sha256": expected_span_hash,
        }

    insns, scan = disassemble_host(pe, capstone)
    immediate_aliases = []
    indirect_table_displacements = []
    source_word1_reads = []
    context_pointer = words[1]
    for insn in insns:
        if insn.id == 0:
            continue
        row = instruction_row(insn, pe.image_base)
        for op_index, operand in enumerate(insn.operands):
            if operand.type == X86_OP_IMM:
                value = operand.imm & 0xFFFFFFFF
                if table_va <= value <= pointer_va + 3:
                    immediate_aliases.append({**row, "operand_index": op_index, "value": hx(value)})
            elif operand.type == X86_OP_MEM:
                displacement = operand.mem.disp & 0xFFFFFFFF
                if (operand.mem.base or operand.mem.index) and table_va <= displacement <= pointer_va + 3:
                    indirect_table_displacements.append({
                        **row, "operand_index": op_index, "displacement": hx(displacement),
                        "base": insn.reg_name(operand.mem.base) if operand.mem.base else None,
                        "index": insn.reg_name(operand.mem.index) if operand.mem.index else None,
                        "access": operand.access,
                    })
                if not operand.mem.base and not operand.mem.index and displacement == table_va + 4:
                    source_word1_reads.append({**row, "access": operand.access})

    # Inspect file-backed non-.text dwords for numerical aliases to any source
    # prefix byte or the adjacent pointer cell; values are candidate aliases only.
    data_aliases = []
    for section in pe.sections:
        if section["name"] == ".text":
            continue
        raw = pe.read_rva(section["virtual_address"], section["raw_size"])
        for offset in range(max(0, len(raw) - 3)):
            value = struct.unpack_from("<I", raw, offset)[0]
            if table_va <= value <= pointer_va + 3:
                data_aliases.append({
                    "section": section["name"],
                    "rva": hx(section["virtual_address"] + offset),
                    "aligned_to_dword": offset % 4 == 0,
                    "value": hx(value),
                })

    # The pointer-cell load is the only published dynamic base source. Inspect
    # its exact bounded caller sequence and check that EDX is passed, not used
    # as a memory base or copied into a REP destination before the call.
    dispatch = [insn for insn in insns
                if DISPATCH_PREFIX_RVA <= insn.address - pe.image_base < DISPATCH_PREFIX_RVA + DISPATCH_PREFIX_SIZE]
    pointer_load = [insn for insn in dispatch if insn.address - pe.image_base == 0x5EF3]
    if len(pointer_load) != 1 or pointer_load[0].mnemonic != "mov":
        raise ValueError("cold dispatcher source-pointer load no longer matches the pinned anchor")
    edx_memory_uses = []
    edx_rep_destinations = []
    for insn in dispatch:
        if insn.address - pe.image_base <= 0x5EF3:
            continue
        if insn.id in (capstone.x86_const.X86_INS_MOVSD, capstone.x86_const.X86_INS_MOVSB):
            if any(prefix in (0xF2, 0xF3) for prefix in insn.prefix):
                edx_rep_destinations.append(instruction_row(insn, pe.image_base))
        for operand in insn.operands:
            if operand.type == X86_OP_MEM and operand.mem.base == capstone.x86_const.X86_REG_EDX:
                edx_memory_uses.append(instruction_row(insn, pe.image_base))
    if not any(insn.address - pe.image_base == 0x5F03 and insn.mnemonic == "call" for insn in dispatch):
        raise ValueError("bounded cold dispatcher callsite moved")

    # Resolve only the known word-1 host callback path: `word1` is loaded from
    # its static cell and the callback writes through that pointer at +0x138.
    writer = [insn for insn in insns
              if CONTEXT_WRITER_RVA <= insn.address - pe.image_base < CONTEXT_WRITER_RVA + CONTEXT_WRITER_SIZE]
    context_absolute_loads = []
    context_field_writes = []
    for insn in writer:
        row = instruction_row(insn, pe.image_base)
        for operand in insn.operands:
            if operand.type != X86_OP_MEM:
                continue
            mem = operand.mem
            disp = mem.disp & 0xFFFFFFFF
            if not mem.base and not mem.index and disp == table_va + 4:
                context_absolute_loads.append({**row, "access": operand.access})
            if mem.base == capstone.x86_const.X86_REG_ECX and disp == 0x138 and operand.access & 2:
                context_field_writes.append({**row, "access": operand.access})
    context_target = context_pointer + 0x138
    context_target_section = section_at_va(pe, context_target)
    context_target_bytes = None
    if context_target_section and context_target_section["file_backed"]:
        context_target_bytes = pe.read_rva(context_target - pe.image_base, 4)


    spans = {
        "source_prefix": {"rva": hx(table_rva), "size": table_size, "sha256": sha(table_raw),
                           "section": section_at_va(pe, table_va)["name"]},
        "source_pointer_cell": {"rva": hx(pointer_rva), "size": POINTER_BYTES,
                                "sha256": sha(pointer_raw), "initial_value": hx(pointer_value),
                                "section": section_at_va(pe, pointer_va)["name"]},
        "dispatcher_pointer_load_to_call": {"rva": hx(DISPATCH_PREFIX_RVA),
                                            "size": DISPATCH_PREFIX_SIZE,
                                            "sha256": sha(pe.read_rva(DISPATCH_PREFIX_RVA, DISPATCH_PREFIX_SIZE))},
        "word1_context_writer": {"rva": hx(CONTEXT_WRITER_RVA), "size": CONTEXT_WRITER_SIZE,
                                 "sha256": sha(pe.read_rva(CONTEXT_WRITER_RVA, CONTEXT_WRITER_SIZE))},
    }
    public_slot_reads = [row for row in compatibility["interface"]["source_table_direct_code_ref_summary"]
                         if int(row["slot"]) in (1, 2, 3, 5)]
    public_slot_read_summary = [{
        "slot": int(row["slot"]), "reads": row["read_count"], "writes": row["write_count"],
        "unknown": row["unknown_count"], "site_count": len(row.get("instruction_rvas", [])),
        "first_ref_rva": row.get("first_ref_rva"), "last_ref_rva": row.get("last_ref_rva"),
    } for row in public_slot_reads]

    def table_word(index: int) -> dict:
        value = words[index]
        target = section_at_va(pe, value)
        if index == 0:
            classification = "signature"
        elif index == 1:
            classification = "shared_context_pointer"
        elif index in (2, 3, 5):
            classification = "host_read_data_or_pointer"
        elif target and target["name"] == ".text":
            classification = "host_text_pointer"
        elif target:
            classification = "other_image_pointer"
        else:
            classification = "outside_image_value"
        return {"slot": index, "offset": hx(index * 4), "value": hx(value),
                "class": classification, "target_section": target}

    slot_values = [table_word(index) for index in (0, 1, 2, 3, 5, 19)]
    context_writer_window = [instruction_row(insn, pe.image_base) for insn in writer
                             if 0x1C03 <= insn.address - pe.image_base <= 0x1C0E]
    writer_rows_hash = sha(json.dumps({"context_absolute_loads": context_absolute_loads,
                                       "context_field_writes": context_field_writes},
                                      sort_keys=True).encode())
    result = {
        "schema_version": 1,
        "status": "STATIC_OBSERVED_BOUNDED_PREFIX_ALIAS_AUDIT",
        "scope": "Pinned original PE bytes; direct absolute write counts are taken from existing public evidence, with this audit adding address-immediate/data alias search and bounded pointer/context dataflow. No host process, proxy, game, or gameplay execution.",
        "prediction_and_falsifier": {
            "prediction": "No source-prefix or source-pointer write/alias is found in the bounded address-anchor paths; word1 is a stable pointer value to a mutable shared context object.",
            "falsifier": "A derived store or REP destination reaches any source-prefix byte or pointer cell, or a later host writer changes word1/source-pointer selection before a subsequent interface copy.",
            "outcome": "No new table or source-pointer mutation path was found by the bounded alias/dataflow checks. Word1's pointee is actively mutable at context+0x138; the copied pointer aliases that live host context.",
        },
        "inputs": {
            "audit_runner_sha256": sha(Path(__file__).read_bytes()),
            "public_evidence": public_inputs,
            "original_images": image_hashes,
            "pe_helper_sha256": sha((root / "scripts" / "pe.py").read_bytes()),
            "capstone_version": capstone.__version__,
        },
        "source_table": {
            "rva": hx(table_rva), "va": hx(table_va), "word_count": table_count,
            "byte_count": table_size, "sha256": sha(table_raw),
            "storage": "file-backed .data initializer",
            "source_pointer_cell": {"rva": hx(pointer_rva), "va": hx(pointer_va),
                                    "initial_value": hx(pointer_value),
                                    "initial_bytes_sha256": sha(pointer_raw),
                                    "direct_reads_and_writes": loader["source_pointer"]["direct_code_refs"],
                                    "published_direct_write_count": loader["source_pointer"]["direct_writes"]},
            "selected_initial_words": slot_values,
            "original_module_cold_copies": module_copies,
            "published_direct_absolute_table_writes": compatibility["interface"]["static_direct_code_writes_observed"],
            "published_fixed_address_read_slots": public_slot_read_summary,
        },
        "new_alias_checks": {
            "candidate_counts": {
                "text_address_immediates": len(immediate_aliases),
                "register_displacements": len(indirect_table_displacements),
                "file_backed_nontext_values": len(data_aliases),
            },
            "text_address_immediates_into_prefix_or_pointer_cell": immediate_aliases,
            "register_based_displacements_numerically_in_prefix_or_pointer_range": indirect_table_displacements,
            "file_backed_nontext_dword_values_into_prefix_or_pointer_cell": data_aliases,
            "public_span_pins": public_span_pins,
            "bounded_dispatch_pointer_flow": {
                "span": spans["dispatcher_pointer_load_to_call"],
                "instructions": [instruction_row(item, pe.image_base) for item in dispatch],
                "pointer_cell_load": instruction_row(pointer_load[0], pe.image_base),
                "edx_based_memory_uses_before_call": edx_memory_uses,
                "edx_rep_destinations_before_call": edx_rep_destinations,
                "interpretation": "The source-table pointer is loaded from 0x4365a0 into EDX and passed as an engine-entry argument; within this bounded sequence it is not used as a memory destination or REP destination.",
            },
        },
        "word1_shared_context": {
            "source_cell_rva": hx(table_rva + 4), "source_cell_va": hx(table_va + 4),
            "initial_value": hx(context_pointer), "target_section": section_at_va(pe, context_pointer),
            "target_write_address": hx(context_target),
            "target_write_bytes_sha256": sha(context_target_bytes) if context_target_bytes else None,
            "pinned_host_writer_span": spans["word1_context_writer"],
            "direct_word1_loads_in_writer": context_absolute_loads,
            "word1_to_field_store_instruction_window": context_writer_window,
            "derived_context_field_writes": context_field_writes,
            "context_writer_evidence": {
                "table_slot_index": upload["host_context_writer"]["table_slot_index"],
                "target_rva": upload["host_context_writer"]["target_rva"],
                "written_context_offset": upload["host_context_writer"]["written_context_offset"],
                "native_stack_mapping": upload["host_context_writer"]["native_stack_mapping"],
            },
            "classification": "Prefix word 1 is a static pointer value; host code writes through that pointer at +0x138. The pointed-to state can change, but this does not change the copied word unless an unobserved alias rewrites source word 1.",
            "bounded_trace_sha256": writer_rows_hash,
        },
        "temporal_scope": {
            "claim": "This report addresses table-content coherence after a raw prefix copy only; it does not establish DLL_PROCESS_ATTACH timing, module-load order, or source-table availability at attach.",
            "deferred_directdraw_note": "The unresolved 0x79A0 route in bootstrap_order.json bears on a publication strategy that waits for DirectDrawCreate to return. It does not contradict or decide a hypothetical earlier DLL_PROCESS_ATTACH publication; import/attach timing is outside this audit.",
            "host_pointer_read_rva": compatibility["interface"]["pointer_slot_direct_code_refs"][0]["rva"],
            "host_first_copy_order_note": bootstrap["conclusion"]["direct_cfg_order"],
        },
        "linear_scan": scan,
        "spans": spans,
        "limits": [
            "Existing direct absolute scans remain the authority for absolute source-cell writes; this audit does not repeat or broaden their function claims.",
            "Numerical data aliases and register displacement matches are candidates, not runtime aliases by themselves.",
            "The pointer-to-context write trace covers the pinned host callback body, not every consumer or every runtime state transition.",
            "Arbitrary register-computed aliases, callbacks, external modules, process scheduling, and user-mode writes remain unproved.",
            "This is a static coherence result; it does not validate a deployed proxy or guarantee that an early DllMain hook is called at a particular time.",
        ],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(args.output), "sha256": sha(args.output.read_bytes()),
                      "table_sha256": sha(table_raw), "source_words": table_count,
                      "text_address_immediate_aliases": len(immediate_aliases),
                      "register_displacement_candidates": len(indirect_table_displacements),
                      "nontext_alias_candidates": len(data_aliases),
                      "word1_host_context_writes": len(context_field_writes),
                      "module_cold_copy_contracts": len(module_copies)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
