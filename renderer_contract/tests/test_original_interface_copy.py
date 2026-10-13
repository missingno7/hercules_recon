"""Bounded execution of original PC DLL cold table-copy prefixes."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from collections import Counter
from pathlib import Path
import platform
import struct
import sys

DEFAULT_DEPS = Path(r"C:\tools\hercules\renderer-oracle-python")
DEFAULT_ANALYSIS_DEPS = Path(r"C:\tools\hercules\python")
DEFAULT_RECEIPT_RELATIVE = Path("work/renderer_next/interface_copy/interface_copy_public_profile_report.json")
PRIOR_RECEIPT_RELATIVE = Path("work/renderer_next/interface_copy/interface_copy_report.json")
TABLE_SOURCE_RVA = 0x361B8
TABLE_WORDS = 250
TABLE_BYTES = TABLE_WORDS * 4
SIGNATURE = 0x5AC00CAC
SOURCE_ADDR = 0x20000000
BAD_SOURCE_ADDR = SOURCE_ADDR + 0x400
STACK_BASE = 0x30000000
STACK_SIZE = 0x3000
RETURN_SENTINEL = STACK_BASE + 0x1000
STACK_TOP = STACK_BASE + STACK_SIZE - 0x10
MAX_INSTRUCTIONS = 5000

REPLACEMENTS = {
    89: 0x61610000 + 89 * 4,
    129: 0x61610000 + 129 * 4,
    130: 0x61610000 + 130 * 4,
    131: 0x61610000 + 131 * 4,
    143: 0x61610000 + 143 * 4,
    148: 0x61610000 + 148 * 4,
    81: 0x61610000 + 81 * 4,
    174: 0x61610000 + 174 * 4,
    175: 0x61610000 + 175 * 4,
}
MODULE_PATHS = {
    "ENG1.DLL": "ENG1.DLL",
    "ENG3.DLL": "ENG3.DLL",
    "TITLE.DLL": "TITLE.DLL",
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def as_int(value) -> int:
    return int(value, 16) if isinstance(value, str) else int(value)


def discover_repo_root(start: Path) -> Path:
    for candidate in (start.resolve(), *start.resolve().parents):
        if (candidate / "renderer_contract").is_dir() and (candidate / "scripts" / "pe.py").is_file():
            return candidate
    raise ValueError("cannot discover repo root; pass --repo-root containing renderer_contract/ and scripts/pe.py")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path,
                        help="repository root; otherwise discovered from this script's ancestors")
    parser.add_argument("--exe", type=Path, required=True, help="explicit original HERCULES.EXE path")
    parser.add_argument("--eng1", type=Path, required=True, help="explicit original ENG1.DLL path")
    parser.add_argument("--eng3", type=Path, required=True, help="explicit original ENG3.DLL path")
    parser.add_argument("--title", type=Path, required=True, help="explicit original TITLE.DLL path")
    parser.add_argument("--deps", type=Path, default=DEFAULT_DEPS)
    parser.add_argument("--analysis-deps", type=Path, default=DEFAULT_ANALYSIS_DEPS)
    parser.add_argument("--profile", type=Path,
                        help="compatibility evidence profile (default: <repo-root>/renderer_contract/evidence/compatibility.json)")
    parser.add_argument("--receipt", type=Path,
                        help="ignored-work receipt path (default: work/renderer_next/interface_copy/interface_copy_public_profile_report.json)")
    args = parser.parse_args()
    args.repo_root_was_explicit = args.repo_root is not None
    args.repo_root = (args.repo_root.resolve() if args.repo_root else
                      discover_repo_root(Path(__file__).resolve().parent))
    if not (args.repo_root / "renderer_contract").is_dir() or not (args.repo_root / "scripts" / "pe.py").is_file():
        parser.error("--repo-root must contain renderer_contract/ and scripts/pe.py")
    for name in ("exe", "eng1", "eng3", "title", "deps", "analysis_deps"):
        setattr(args, name, getattr(args, name).resolve())
    args.profile = (args.profile.resolve() if args.profile else
                    args.repo_root / "renderer_contract" / "evidence" / "compatibility.json")
    args.receipt = (args.receipt.resolve() if args.receipt else
                    args.repo_root / DEFAULT_RECEIPT_RELATIVE)
    if not args.profile.is_file():
        parser.error(f"compatibility evidence profile not found: {args.profile}")
    for name in ("exe", "eng1", "eng3", "title"):
        if not getattr(args, name).is_file():
            parser.error(f"--{name} file not found: {getattr(args, name)}")
    try:
        args.receipt.relative_to((args.repo_root / "work").resolve())
    except ValueError as exc:
        parser.error("--receipt must stay under --repo-root/work")
    return args


def load_support(args):
    sys.path[:0] = [str(args.deps), str(args.analysis_deps), str(args.repo_root / "scripts"),
                    str(args.repo_root / "renderer_contract" / "tests")]
    os.environ["HERCULES_PYTHON_DEPS"] = str(args.analysis_deps)
    import env  # noqa: F401  # Adds the existing analysis dependency directory.
    from pe import PE
    import capstone
    import unicorn
    from oracle_common import (EXPECTED_EXE_SHA256, verify_runtime, write_receipt)
    runtime = verify_runtime(unicorn, args.deps)
    return PE, capstone, unicorn, EXPECTED_EXE_SHA256, runtime, write_receipt


def make_decoder(capstone):
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    return decoder


def insn_registers(unicorn):
    return unicorn.x86_const.UC_X86_REG_EAX, unicorn.x86_const.UC_X86_REG_ECX, \
        unicorn.x86_const.UC_X86_REG_EDX, unicorn.x86_const.UC_X86_REG_ESI, \
        unicorn.x86_const.UC_X86_REG_EDI, unicorn.x86_const.UC_X86_REG_ESP, \
        unicorn.x86_const.UC_X86_REG_EIP, unicorn.x86_const.UC_X86_REG_EFLAGS


def word_at(uc, address):
    return struct.unpack("<I", bytes(uc.mem_read(address, 4)))[0]


def pack_words(words):
    return struct.pack("<" + "I" * len(words), *words)


def overlay_words(original_words):
    words = list(original_words)
    for slot, value in REPLACEMENTS.items():
        words[slot] = value
    return words


def expected_disassembly(module, image, module_evidence, decoder, capstone):
    entry = module_evidence["entry_span"]
    entry_rva, size = as_int(entry["rva"]), int(entry["size"])
    raw = image.read_rva(entry_rva, size)
    if sha256(raw) != entry["sha256"].lower():
        raise ValueError(f"{module} entry span differs from compatibility profile")
    instructions = list(decoder.disasm(raw, image.image_base + entry_rva))
    call_sites = {}
    for instruction in instructions:
        if instruction.id == capstone.x86_const.X86_INS_CALL:
            target_va = instruction.operands[0].imm if instruction.operands else None
            call_sites[instruction.address - image.image_base] = {
                "rva": hex(instruction.address - image.image_base),
                "operand": instruction.op_str,
                "target_va": hex(target_va) if target_va is not None else None,
                "target_rva": hex(target_va - image.image_base) if target_va is not None else None,
            }
    copy = module_evidence["copy_and_validation"]
    copy_rva = as_int(copy["copy_instruction_rva"])
    sig_rva = as_int(copy["signature_check_rva"])
    if not any(insn.address - image.image_base == copy_rva for insn in instructions):
        raise ValueError(f"{module} recorded copy instruction is outside entry span")
    if not any(insn.address - image.image_base == sig_rva for insn in instructions):
        raise ValueError(f"{module} recorded signature check is outside entry span")
    post_copy_calls = [rva for rva in call_sites if rva > copy_rva]
    if not post_copy_calls:
        raise ValueError(f"{module} entry has no bounded initializer call after table copy")
    text = next(s for s in image.sections if s["name"] == ".text")
    text_lo, text_hi = text["virtual_address"], text["virtual_address"] + text["virtual_size"]
    for site in call_sites.values():
        if site["target_rva"] is None or not (text_lo <= as_int(site["target_rva"]) < text_hi):
            raise ValueError(f"{module} entry call target is not an internal .text target: {site}")
    return {
        "entry_rva": entry_rva,
        "entry_size": size,
        "entry_span_sha256": entry["sha256"],
        "copy_rva": copy_rva,
        "signature_check_rva": sig_rva,
        "call_sites": call_sites,
        "first_call_after_copy_rva": post_copy_calls[0],
        "text_rva": text_lo,
        "text_virtual_size": text["virtual_size"],
        "text_raw_size": text["raw_size"],
        "instructions": instructions,
    }


def run_case(unicorn, capstone, module_name, image, module_evidence, source_words,
             source_pointer, scenario, gate_override=None):
    UC_ARCH_X86 = unicorn.UC_ARCH_X86
    UC_MODE_32 = unicorn.UC_MODE_32
    UC_PROT_ALL = unicorn.UC_PROT_ALL
    UC_HOOK_CODE = unicorn.UC_HOOK_CODE
    UC_HOOK_MEM_WRITE = unicorn.UC_HOOK_MEM_WRITE
    Uc = unicorn.Uc
    base = image.image_base
    mapped = bytes(image.pe.get_memory_mapped_image())
    image_size = image.pe.OPTIONAL_HEADER.SizeOfImage
    if len(mapped) > image_size or image_size & 0xfff:
        raise ValueError(f"{module_name} SizeOfImage is invalid for preferred-base mapping")
    mapped += b"\0" * (image_size - len(mapped))
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(base, len(mapped), UC_PROT_ALL)
    uc.mem_write(base, mapped)
    uc.mem_map(SOURCE_ADDR, 0x1000, UC_PROT_ALL)
    uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_ALL)
    uc.mem_write(RETURN_SENTINEL, b"\xf4")

    table_rva = as_int(module_evidence["copy_and_validation"]["copy_destination_rva"])
    table_va = base + table_rva
    gate_rva = as_int(module_evidence["copy_and_validation"]["local_initialization_flag_rva"])
    gate_va = base + gate_rva
    initial_table = bytes(uc.mem_read(table_va, TABLE_BYTES))
    initial_gate = word_at(uc, gate_va)
    source_blob = pack_words(source_words) if source_words is not None else b""
    if source_pointer in (SOURCE_ADDR, BAD_SOURCE_ADDR):
        if source_pointer + len(source_blob) > SOURCE_ADDR + 0x1000:
            raise ValueError("synthetic host table exceeds its mapped private page")
        uc.mem_write(source_pointer, source_blob)
    if gate_override is not None:
        uc.mem_write(gate_va, struct.pack("<I", gate_override))

    # Only the bounded entry span and a one-byte sentinel are legal code PCs.
    static = expected_disassembly(module_name, image, module_evidence,
                                  make_decoder(capstone), capstone)
    entry_va = base + static["entry_rva"]
    entry_end_va = entry_va + static["entry_size"]
    call_sites = {rva: value for rva, value in static["call_sites"].items()}
    current_pc_rva = {"value": None}
    sequence = {"value": 0}
    stop = {"reason": None, "call_site": None, "error": None,
            "sentinel_reached": False, "instruction_count": 0}
    trace = []
    timeline = []
    table_write_events = []
    text_write_count = {"value": 0}
    source_write_count = {"value": 0}
    regs = insn_registers(unicorn)
    eax, ecx, edx, esi, edi, esp, eip, eflags = regs
    incoming_esi, incoming_edi = 0x51515151, 0x61616161
    initial_esp = STACK_TOP - 12
    flags = 0x13572468 if scenario == "warm_gate_zero" else 1
    pointer = source_pointer
    if scenario == "null_source":
        pointer = 0
    if scenario == "bad_signature":
        pointer = BAD_SOURCE_ADDR
    stack_blob = struct.pack("<III", RETURN_SENTINEL, pointer, flags)
    uc.mem_write(initial_esp, stack_blob)
    uc.reg_write(esi, incoming_esi)
    uc.reg_write(edi, incoming_edi)
    uc.reg_write(esp, initial_esp)
    uc.reg_write(eflags, 0x202)
    uc.reg_write(eip, entry_va)

    def code_hook(engine, address, size, _user):
        sequence["value"] += 1
        stop["instruction_count"] += 1
        if address == RETURN_SENTINEL:
            stop["reason"] = "return_sentinel"
            stop["sentinel_reached"] = True
            engine.emu_stop()
            return
        if not (entry_va <= address < entry_end_va):
            stop["reason"] = "escaped_bounded_entry_span"
            stop["error"] = f"unexpected code PC {address:#x}"
            engine.emu_stop()
            return
        rva = address - base
        current_pc_rva["value"] = rva
        if len(trace) < MAX_INSTRUCTIONS:
            trace.append(rva)
        timeline.append({"sequence": sequence["value"], "event": "instruction", "rva": hex(rva)})
        if rva in call_sites:
            stop["reason"] = "stopped_before_first_call"
            stop["call_site"] = call_sites[rva]
            engine.emu_stop()

    def write_hook(engine, access, address, size, value, _user):
        sequence["value"] += 1
        pc = current_pc_rva["value"]
        if base + static["text_rva"] <= address < base + static["text_rva"] + static["text_virtual_size"]:
            text_write_count["value"] += 1
        if SOURCE_ADDR <= address < SOURCE_ADDR + 0x1000:
            source_write_count["value"] += 1
        if table_va <= address < table_va + TABLE_BYTES:
            row = {"sequence": sequence["value"], "pc_rva": hex(pc) if pc is not None else None,
                   "address": hex(address), "size": int(size), "value": int(value)}
            table_write_events.append(row)
            timeline.append({"sequence": sequence["value"], "event": "table_write", "rva": row["pc_rva"],
                             "address": row["address"], "size": row["size"]})

    uc.hook_add(UC_HOOK_CODE, code_hook)
    uc.hook_add(UC_HOOK_MEM_WRITE, write_hook)
    try:
        uc.emu_start(entry_va, 0, count=MAX_INSTRUCTIONS)
    except Exception as exc:
        stop["reason"] = "emulator_error"
        stop["error"] = f"{type(exc).__name__}: {exc}"

    final_table = bytes(uc.mem_read(table_va, TABLE_BYTES))
    final_gate = word_at(uc, gate_va)
    final_source = (bytes(uc.mem_read(source_pointer, len(source_blob)))
                    if source_pointer in (SOURCE_ADDR, BAD_SOURCE_ADDR) and source_blob else None)
    final_text = bytes(uc.mem_read(base + static["text_rva"], static["text_virtual_size"]))
    before_text = mapped[static["text_rva"]:static["text_rva"] + static["text_virtual_size"]]
    clone_matches_source = source_blob != b"" and final_table == source_blob
    write_intervals = []
    for event in table_write_events:
        start = as_int(event["address"])
        lo, hi = max(start, table_va), min(start + event["size"], table_va + TABLE_BYTES)
        if lo < hi:
            write_intervals.append((lo, hi))
    covered = set()
    for lo, hi in write_intervals:
        covered.update(range(lo, hi))
    copy_bytes_covered = len(covered)
    copy_rva = static["copy_rva"]
    signature_rva = static["signature_check_rva"]
    rep_visits = [i for i, rva in enumerate(trace) if rva == copy_rva]
    signature_visits = [i for i, rva in enumerate(trace) if rva == signature_rva]
    copy_timeline = [e for e in timeline if e["event"] == "table_write" and e["rva"] == hex(copy_rva)]
    compare_events = [e for e in timeline if e["event"] == "instruction" and e["rva"] == hex(signature_rva)]
    last_copy_write_seq = max((e["sequence"] for e in copy_timeline), default=None)
    compare_seq = compare_events[0]["sequence"] if compare_events else None
    visit_counts = Counter(trace)
    prepared_init_args = None
    if stop["reason"] == "stopped_before_first_call" and stop["call_site"]["rva"] == hex(static["first_call_after_copy_rva"]):
        final_esp = uc.reg_read(esp)
        prepared_init_args = [word_at(uc, final_esp), word_at(uc, final_esp + 4)]

    state = {
        "module": module_name,
        "scenario": scenario,
        "initial_gate": initial_gate,
        "gate_override": gate_override,
        "gate_after": final_gate,
        "table_rva": hex(table_rva),
        "table_va": hex(table_va),
        "initial_table_sha256": sha256(initial_table),
        "final_table_sha256": sha256(final_table),
        "table_matches_source_exactly": clone_matches_source,
        "table_unchanged": final_table == initial_table,
        "copied_bytes_seen_at_native_rep_instruction": copy_bytes_covered,
        "table_write_event_count": len(table_write_events),
        "last_copy_write_sequence": last_copy_write_seq,
        "signature_compare_sequence": compare_seq,
        "copy_instruction_precedes_signature_compare": bool(rep_visits and signature_visits and rep_visits[0] < signature_visits[0]),
        "trace_contains_copy_instruction": bool(rep_visits),
        "trace_contains_signature_compare": bool(signature_visits),
        "stop_reason": stop["reason"],
        "stopped_call_site": stop["call_site"],
        "initializer_stack_words_before_call": prepared_init_args,
        "return_sentinel_reached": stop["sentinel_reached"],
        "instruction_count": stop["instruction_count"],
        "entry_arguments": {"source_pointer": hex(pointer), "flags_word": hex(flags), "entry_esp": hex(initial_esp)},
        "return_state": None,
        "callee_saved_esi_edi_restored_on_return": None,
        "source_table_unchanged": final_source == source_blob if source_blob else None,
        "code_text_unchanged": final_text == before_text,
        "code_text_write_events": text_write_count["value"],
        "synthetic_source_write_events_during_execution": source_write_count["value"],
        "copy_rva": hex(copy_rva),
        "signature_check_rva": hex(signature_rva),
        "key_instruction_visit_counts": {
            hex(rva): visit_counts[rva]
            for rva in sorted({copy_rva, signature_rva, *call_sites.keys()})
        },
        "failure": stop["error"],
    }
    if stop["sentinel_reached"]:
        final_esp = uc.reg_read(esp)
        state["return_state"] = {
            "eax": hex(uc.reg_read(eax)),
            "esp": hex(final_esp),
            "entry_esp_after_ret": final_esp == initial_esp + 4,
        }
        state["callee_saved_esi_edi_restored_on_return"] = (
            uc.reg_read(esi) == incoming_esi and uc.reg_read(edi) == incoming_edi
        )
    return state, final_table, source_blob, initial_table


def assert_case(result, condition, message):
    if not condition:
        raise AssertionError(f"{result['module']} {result['scenario']}: {message}")


def main() -> int:
    args = parse_args()
    PE, capstone, unicorn, expected_exe_hash, runtime, write_receipt = load_support(args)
    receipt = {
        "schema_version": 1,
        "lane": "renderer_next_interface_copy_prefix",
        "scope": "Bounded execution of original PC_DLL EngineMain entry prefixes in isolated Unicorn machines at preferred DLL base. Each run stops before the first internal call or returns through the native null/signature failure path. No initializer, OS endpoint, gameplay, or module startup is executed.",
        "prediction_and_falsifier": {
            "prediction": "Each cold entry copies exactly 250 DWORDs to a module-local table before checking the shared signature; the copy preserves selected source-table substitutions and every other source word. A bad signature still leaves the copied table. A synthetic warm gate value of zero skips this copy before the next internal call.",
            "falsifier": "Any cold DLL failing the exact 250-word comparison, checking signature before copy, changing an unselected source word, failing to retain selected values, copying on the warm-gate branch, or executing a call before the stop boundary falsifies the prediction.",
        },
        "proof_level": "BOUNDED_EQUIVALENT for named original-DLL entry prefixes only; no full-export or source-equivalence claim.",
        "package_context": {
            "repo_root": str(args.repo_root),
            "repo_root_selection": "explicit --repo-root" if args.repo_root_was_explicit else "nearest ancestor containing renderer_contract/ and scripts/pe.py",
            "public_profile": str(args.profile),
            "receipt_path": str(args.receipt),
            "receipt_guard": "output path must be inside repo_root/work",
        },
        "tool": {},
        "inputs": {},
        "source_table_fixture": {},
        "module_layouts": [],
        "scenarios": [],
        "summary": {},
        "proof_limits": [
            "No OS imports or internal initializer callees were executed; the emulator stops on the first CALL instruction in the exported entry span.",
            "The cold valid case proves the copy prefix and pre-initializer call setup only; it does not establish full export behavior or universal calling convention.",
            "The bad-signature and null-source return observations apply only to these tested entry paths; they do not establish universal DLL behavior or a complete API contract.",
            "The validation loop repeatedly tests source word zero; the exact byte comparison independently verifies copied words, but this prefix does not prove every callback target valid or safe to dispatch.",
            "The warm-gate-zero case injects only the observed gate value into isolated DLL data to exercise the copy-skipping branch; it is not an observed live game session.",
            "Synthetic overlay callback words are never dispatched; they test data-copy preservation only.",
            "No file bytes, original code pages, canonical package files, or recovery state were modified.",
        ],
    }
    prior_receipt = args.repo_root / PRIOR_RECEIPT_RELATIVE
    if prior_receipt.is_file():
        receipt["prior_receipt_provenance"] = {
            "path": str(prior_receipt),
            "sha256": sha256(prior_receipt.read_bytes()),
            "note": "Earlier private-evidence run retained separately; this audit writes a distinct public-profile receipt.",
        }
    failures = []
    try:
        profile_raw = args.profile.read_bytes()
        profile = json.loads(profile_raw)
        exe_raw = args.exe.read_bytes()
        if sha256(exe_raw) != expected_exe_hash:
            raise ValueError("--exe differs from the pinned original HERCULES.EXE SHA-256")
        exe = PE(args.exe)
        table_bytes = exe.read_rva(TABLE_SOURCE_RVA, TABLE_BYTES)
        table_hash = sha256(table_bytes)
        expected_table_hash = profile["interface"]["source_table_sha256"].lower()
        if table_hash != expected_table_hash:
            raise ValueError("original HERCULES table span differs from public compatibility profile")
        original_words = struct.unpack("<" + "I" * TABLE_WORDS, table_bytes)
        if original_words[0] != SIGNATURE:
            raise ValueError("original host interface table does not contain the shared signature")
        if as_int(profile["interface"]["dword_count"]) != TABLE_WORDS or as_int(profile["interface"]["byte_count"]) != TABLE_BYTES:
            raise ValueError("public compatibility profile does not describe the expected 250-DWORD prefix")
        source_words = overlay_words(original_words)
        bad_words = list(source_words)
        bad_words[0] = SIGNATURE ^ 1
        source_blob = pack_words(source_words)
        if len(source_blob) != TABLE_BYTES:
            raise AssertionError("bad private source table size")

        receipt["tool"] = {
            **runtime,
            "python": platform.python_version(),
            "capstone_version": capstone.__version__,
            "instruction_budget_per_scenario": MAX_INSTRUCTIONS,
            "decoder_mode": "Capstone x86-32 disassembly only for stop-site selection; Unicorn executes original DLL bytes",
        }
        receipt["inputs"] = {
            "exe": {"path": str(args.exe), "sha256_expected": expected_exe_hash, "sha256_computed": sha256(exe_raw)},
            "public_compatibility_profile": {"path": str(args.profile), "sha256": sha256(profile_raw)},
            "analysis_tools": {"pe_adapter": str(args.repo_root / "scripts" / "pe.py"), "unicorn_deps": str(args.deps), "analysis_deps": str(args.analysis_deps)},
        }
        receipt["source_table_fixture"] = {
            "origin": "Read-only copy of the original HERCULES.EXE 250-DWORD source table, placed in private Unicorn memory.",
            "original_table_rva": hex(TABLE_SOURCE_RVA),
            "original_table_bytes": TABLE_BYTES,
            "original_table_sha256_expected_and_computed": table_hash,
            "signature": hex(SIGNATURE),
            "overlay_slots": {str(slot): {"value": hex(value), "origin": "synthetic selected-slot substitution; never dispatched"}
                              for slot, value in REPLACEMENTS.items()},
            "unselected_words_preserved_from_original_count": TABLE_WORDS - len(REPLACEMENTS),
            "bad_signature_word0": hex(bad_words[0]),
            "private_scratch_address": hex(SOURCE_ADDR),
        }

        dll_args = {"ENG1.DLL": args.eng1, "ENG3.DLL": args.eng3, "TITLE.DLL": args.title}
        for module_name, path in dll_args.items():
            module_ev = profile["modules"][module_name]
            expected_oracle = profile["oracles"][module_name]
            raw = path.read_bytes()
            computed_hash = sha256(raw)
            if computed_hash != expected_oracle["sha256"].lower():
                raise ValueError(f"{module_name} differs from the public compatibility-profile original hash")
            image = PE(path)
            metadata = image.metadata()
            entry_exports = [item for item in metadata["exports"] if item["name"] == "PC_DLLEngineMain"]
            if len(entry_exports) != 1 or entry_exports[0]["ordinal"] != 1:
                raise ValueError(f"{module_name} does not have the expected single ordinal-1 PC_DLLEngineMain export")
            static = expected_disassembly(module_name, image, module_ev, make_decoder(capstone), capstone)
            if entry_exports[0]["rva"] != static["entry_rva"]:
                raise ValueError(f"{module_name} export RVA differs from its copied-table entry span")
            base = image.image_base
            if base != 0x10000000 or as_int(module_ev["image_base"]) != base:
                raise ValueError(f"{module_name} is not at its evidenced preferred base 0x10000000")
            copy_info = module_ev["copy_and_validation"]
            gate_rva = as_int(copy_info["local_initialization_flag_rva"])
            table_rva = as_int(copy_info["copy_destination_rva"])
            flag_raw = image.read_rva(gate_rva, 4)
            flag_initial = struct.unpack("<I", flag_raw)[0]
            if flag_initial != 1:
                raise ValueError(f"{module_name} on-disk local initialization flag is not initially one")
            layout = {
                "module": module_name,
                "path": str(path),
                "sha256_expected_and_computed": computed_hash,
                "preferred_image_base": hex(base),
                "export_rva": hex(entry_exports[0]["rva"]),
                "entry_span": {"rva": hex(static["entry_rva"]), "size": static["entry_size"],
                               "sha256": static["entry_span_sha256"]},
                "copy_instruction_rva": hex(static["copy_rva"]),
                "signature_check_rva": hex(static["signature_check_rva"]),
                "module_local_gate": {"rva": hex(gate_rva), "va": hex(base + gate_rva),
                                      "initial_dword": flag_initial},
                "module_local_table": {"rva": hex(table_rva), "va": hex(base + table_rva),
                                       "words": TABLE_WORDS, "bytes": TABLE_BYTES},
                "entry_call_sites_and_targets": list(static["call_sites"].values()),
            }
            receipt["module_layouts"].append(layout)

            scenarios = [
                ("cold_valid_copy", source_words, SOURCE_ADDR, None),
                ("bad_signature_copy_then_fail", bad_words, BAD_SOURCE_ADDR, None),
                ("null_source_prefix", None, 0, None),
                ("warm_gate_zero", source_words, SOURCE_ADDR, 0),
            ]
            for scenario, words, pointer, gate_override in scenarios:
                result, final_table, case_source_blob, initial_table = run_case(
                    unicorn, capstone, module_name, image, module_ev,
                    words, pointer, scenario, gate_override,
                )
                if result["failure"]:
                    raise RuntimeError(f"{module_name} {scenario}: {result['failure']}")
                if result["code_text_write_events"] != 0 or not result["code_text_unchanged"]:
                    raise AssertionError(f"{module_name} {scenario} wrote original .text")
                if scenario == "cold_valid_copy":
                    assert_case(result, result["stop_reason"] == "stopped_before_first_call", "did not stop before first initializer call")
                    assert_case(result, result["stopped_call_site"]["rva"] == hex(static["first_call_after_copy_rva"]), "stopped at an unexpected call")
                    assert_case(result, result["table_matches_source_exactly"], "copied 250 words differ from private host source")
                    assert_case(result, result["copied_bytes_seen_at_native_rep_instruction"] == TABLE_BYTES, "not all 1000 table bytes were written by the REP copy")
                    assert_case(result, result["copy_instruction_precedes_signature_compare"], "signature compare ran before the REP copy")
                    assert_case(result, result["last_copy_write_sequence"] < result["signature_compare_sequence"], "table write occurred after signature check")
                    assert_case(result, result["initializer_stack_words_before_call"] == [source_words[2], source_words[3]], "prepared pre-call table words did not survive the copy")
                    result["selected_overlay_words_retained"] = all(
                        struct.unpack_from("<I", final_table, slot * 4)[0] == value
                        for slot, value in REPLACEMENTS.items()
                    )
                    result["unselected_original_words_unchanged"] = all(
                        struct.unpack_from("<I", final_table, slot * 4)[0] == original_words[slot]
                        for slot in range(TABLE_WORDS) if slot not in REPLACEMENTS
                    )
                    assert_case(result, result["selected_overlay_words_retained"], "selected substitutions were not retained")
                    assert_case(result, result["unselected_original_words_unchanged"], "an unselected original word changed")
                    assert_case(result, result["gate_after"] == 1, "cold gate changed before initializer call")
                    assert_case(result, not result["return_sentinel_reached"], "valid cold case executed past initializer stop")
                elif scenario == "bad_signature_copy_then_fail":
                    assert_case(result, result["return_sentinel_reached"], "bad-signature branch did not return through its bounded failure path")
                    assert_case(result, result["table_matches_source_exactly"], "bad-signature path did not preserve the full copied table")
                    assert_case(result, result["copied_bytes_seen_at_native_rep_instruction"] == TABLE_BYTES, "bad signature did not follow a full table copy")
                    assert_case(result, result["copy_instruction_precedes_signature_compare"], "bad signature was checked before copy")
                    assert_case(result, result["last_copy_write_sequence"] < result["signature_compare_sequence"], "bad-signature table writes occurred after signature check")
                    assert_case(result, result["return_state"]["entry_esp_after_ret"], "observed failure return did not restore caller ESP to its first argument")
                    assert_case(result, result["callee_saved_esi_edi_restored_on_return"], "ESI/EDI not restored on observed failure return")
                    assert_case(result, result["gate_after"] == 1, "bad signature changed cold gate")
                elif scenario == "null_source_prefix":
                    assert_case(result, result["return_sentinel_reached"], "null-source prefix did not take the tested return path")
                    assert_case(result, not result["trace_contains_copy_instruction"], "null-source path executed REP copy")
                    assert_case(result, not result["trace_contains_signature_compare"], "null-source path reached signature comparison")
                    assert_case(result, result["table_unchanged"], "null-source path changed the local table")
                    assert_case(result, result["return_state"]["entry_esp_after_ret"], "observed null-source return did not restore caller ESP to its first argument")
                    assert_case(result, result["callee_saved_esi_edi_restored_on_return"], "ESI/EDI not restored on observed null-source return")
                else:
                    assert_case(result, gate_override == 0 and result["gate_after"] == 0, "warm-gate test did not inject the zero gate")
                    assert_case(result, result["stop_reason"] == "stopped_before_first_call", "warm-gate branch did not stop before its next internal call")
                    assert_case(result, result["stopped_call_site"]["rva"] != hex(static["first_call_after_copy_rva"]), "warm gate entered the cold table-init call")
                    assert_case(result, result["table_unchanged"], "warm gate unexpectedly copied into the module-local table")
                    assert_case(result, not result["trace_contains_copy_instruction"], "warm gate executed REP copy")
                    assert_case(result, not result["trace_contains_signature_compare"], "warm gate executed signature validation")
                    assert_case(result, not result["return_sentinel_reached"], "warm case ran beyond the first-call stop")
                result["passed"] = True
                receipt["scenarios"].append(result)

        gate_rvas = [as_int(item["module_local_gate"]["rva"]) for item in receipt["module_layouts"]]
        table_rvas = [as_int(item["module_local_table"]["rva"]) for item in receipt["module_layouts"]]
        gate_vas = [as_int(item["module_local_gate"]["va"]) for item in receipt["module_layouts"]]
        table_vas = [as_int(item["module_local_table"]["va"]) for item in receipt["module_layouts"]]
        if len(set(gate_rvas)) != 3 or len(set(table_rvas)) != 3 or len(set(gate_vas)) != 3 or len(set(table_vas)) != 3:
            raise AssertionError("module-local gate/table cells are not independent addresses")
        receipt["summary"] = {
            "modules_tested": len(receipt["module_layouts"]),
            "scenarios_tested": len(receipt["scenarios"]),
            "passed_scenarios": sum(1 for row in receipt["scenarios"] if row["passed"]),
            "all_250_words_copied_on_cold_valid": True,
            "bad_signature_copies_before_failure": True,
            "null_source_prefix_returned_safely": True,
            "warm_gate_zero_skips_copy_before_internal_call": True,
            "selected_overlays_retained": True,
            "all_unselected_original_words_retained": True,
            "independent_module_local_gate_rvas": [hex(x) for x in gate_rvas],
            "independent_module_local_table_rvas": [hex(x) for x in table_rvas],
            "no_original_code_bytes_written": all(row["code_text_write_events"] == 0 and row["code_text_unchanged"] for row in receipt["scenarios"]),
            "no_internal_calls_executed": all(row["stop_reason"] == "stopped_before_first_call" or row["return_sentinel_reached"] for row in receipt["scenarios"]),
        }
        receipt["status"] = "passed"
    except Exception as exc:
        failures.append(f"{type(exc).__name__}: {exc}")
        receipt["status"] = "failed"
        receipt["failures"] = failures
    write_receipt(args.receipt, receipt)
    print(json.dumps({"receipt": str(args.receipt), "status": receipt.get("status"),
                      "summary": receipt.get("summary"), "failures": receipt.get("failures", [])}, indent=2))
    return 0 if receipt.get("status") == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
