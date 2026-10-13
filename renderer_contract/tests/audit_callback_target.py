#!/usr/bin/env python3
"""Static, hash-pinned audit of the primary BD80 callback producer boundary.

Reads the pinned HERCULES.EXE and its three shipped DLLs. It does not load or
execute any inspected image. Output is restricted to this checkout's work/.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
REPO = next((p for p in HERE.parents if (p / "scripts" / "pe.py").is_file()), None)
if REPO is None:
    raise SystemExit("cannot find checkout root containing scripts/pe.py")
sys.path.insert(0, str(REPO / "scripts"))
import env  # noqa: F401,E402
from pe import PE  # noqa: E402
import capstone  # noqa: E402

PINNED = {
    "HERCULES.EXE": ("587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d", 0x400000),
    "ENG1.DLL": ("01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757", 0x10000000),
    "ENG3.DLL": ("aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7", 0x10000000),
    "TITLE.DLL": ("b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b", 0x10000000),
}
TABLE_ROOTS = {"ENG1.DLL": 0x70F20, "ENG3.DLL": 0x407A0, "TITLE.DLL": 0x2BB40}
SLOT92_ADAPTERS = {"ENG1.DLL": 0x2C640, "ENG3.DLL": 0x1FC50,
                   "TITLE.DLL": 0xC6E0}
PRIMARY = 0xA8E780
GATE = 0xA8E788
SECONDARY = 0xA8E784
SETTER_RVA = 0xBDA0
RESET_RVA = 0xBDF0
DISPATCH_RVA = 0xBD80
TITLE_ADAPTER_RVA = 0xC6E0
EXPECTED_CAPSTONE_VERSION = "5.0.7"
EXPECTED_CALLS = {
    "ENG1.DLL": [
        {"rva": 0x632A, "adapter_rva": 0x2C640, "span_rva": 0x6328, "span_size": 0x0A,
         "instructions": [("push", "0"), ("call", "0x1002c640"), ("add", "esp, 4")]},
        {"rva": 0x1F22C, "adapter_rva": 0x2C640, "span_rva": 0x1F220, "span_size": 0x14,
         "instructions": [("push", "0"), ("mov", "dword ptr [0x10071ec0], 0"),
                          ("call", "0x1002c640"), ("add", "esp, 4")]},
    ],
    "ENG3.DLL": [
        {"rva": 0xB1D1, "adapter_rva": 0x1FC50, "span_rva": 0xB1CF, "span_size": 0x0A,
         "instructions": [("push", "0"), ("call", "0x1001fc50"), ("add", "esp, 4")]},
        {"rva": 0x1A46C, "adapter_rva": 0x1FC50, "span_rva": 0x1A460, "span_size": 0x14,
         "instructions": [("push", "0"), ("mov", "dword ptr [0x10041444], 0"),
                          ("call", "0x1001fc50"), ("add", "esp, 4")]},
    ],
    "TITLE.DLL": [
        {"rva": 0x1D4E2, "adapter_rva": 0xC6E0, "span_rva": 0x1D4E0, "span_size": 0x0A,
         "instructions": [("push", "0"), ("call", "0x1000c6e0"), ("add", "esp, 4")]},
    ],
}
EXPECTED_ADAPTERS = {
    "ENG1.DLL": {"rva": 0x2C640, "slot_va": 0x10071090},
    "ENG3.DLL": {"rva": 0x1FC50, "slot_va": 0x10040910},
    "TITLE.DLL": {"rva": 0xC6E0, "slot_va": 0x1002BCB0},
}
PREDICTION_HISTORY = [
    {
        "prediction": "No DLL callsite would directly reach TITLE's slot-92 adapter, leaving the primary setter target statically unresolved.",
        "falsifier": "Any direct adapter call, slot-92 table access, or adapter export/reference.",
        "outcome": "FALSIFIED: the full skipdata sweep found direct wrapper users in ENG1, ENG3, and TITLE.",
    },
    {
        "prediction": "Every direct call to the three local slot-92 adapters passes NULL; no non-null target is exposed by those direct code paths.",
        "falsifier": "A nonzero/unknown pushed argument, direct HERC BDA0 call, or additional direct slot-92 reference in the complete raw .text scans.",
        "outcome": "SUPPORTED for the five observed calls; register-computed aliases and other runtime callers remain outside this static claim.",
    },
]


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_args():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--original-root", required=True, type=Path,
                    help="read-only original checkout containing work/discs and assets/pc")
    ap.add_argument("--exe", type=Path,
                    help="pinned original HERCULES.EXE; defaults below --original-root/work/discs/pc_install")
    ap.add_argument("--output", type=Path,
                    help="JSON output, required to remain under this checkout's work/")
    a = ap.parse_args()
    a.original_root = a.original_root.resolve()
    a.exe = (a.exe or (a.original_root / "work/discs/pc_install/HERCULES.EXE")).resolve()
    a.output = (a.output or (HERE / "evidence.json")).resolve()
    try:
        a.output.relative_to((REPO / "work").resolve())
    except ValueError:
        ap.error("--output must be inside this checkout's work/ directory")
    if a.output == a.exe:
        ap.error("output must not name the original executable")
    return a


def image_paths(args):
    return {
        "HERCULES.EXE": args.exe,
        **{name: args.original_root / "assets/pc/HERCULES" / name
           for name in ("ENG1.DLL", "ENG3.DLL", "TITLE.DLL")},
    }


def code_sweep(im):
    text = next(s for s in im.sections if s["name"] == ".text")
    raw = im.read_rva(text["virtual_address"], text["raw_size"])
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    md.skipdata = True
    insns = list(md.disasm(raw, im.image_base + text["virtual_address"]))
    real = [i for i in insns if i.mnemonic != ".byte"]
    skipped = sum(i.size for i in insns if i.mnemonic == ".byte")
    covered = sum(i.size for i in insns)
    if covered != len(raw):
        raise RuntimeError(f"incomplete raw .text coverage: {covered}/{len(raw)}")
    # A linear sweep gives exhaustive byte coverage, not proof that every decoded
    # instruction boundary is a reachable instruction. Keep that limitation explicit.
    return text, real, {"raw_bytes": len(raw), "decoded_instructions": len(real),
                        "skipdata_bytes": skipped, "covered_bytes": covered,
                        "first_rva": hex(text["virtual_address"]),
                        "last_rva_exclusive": hex(text["virtual_address"] + text["raw_size"]),
                        "decode_note": "full linear sweep with Capstone skipdata; embedded data may decode as instructions"}


def decode_bounded(im, rva, size):
    """Decode a byte-checked bounded code span without skipdata or resync."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    raw = im.read_rva(rva, size)
    insns = list(md.disasm(raw, im.image_base + rva))
    if sum(i.size for i in insns) != size:
        raise RuntimeError(f"bounded decode incomplete at {Path(im.path).name}+{rva:#x}")
    return [{"rva": hex(i.address - im.image_base), "size": i.size,
             "mnemonic": i.mnemonic, "operands": i.op_str} for i in insns]


def gate_exact_instructions(label, decoded, expected):
    actual = [(row["mnemonic"], row["operands"]) for row in decoded]
    if actual != expected:
        raise RuntimeError(f"acceptance gate failed for {label}: expected {expected!r}, got {actual!r}")


def run_acceptance_gates(images, modules, module_insns, source_slot92_target):
    if capstone.__version__ != EXPECTED_CAPSTONE_VERSION:
        raise RuntimeError(f"expected Capstone {EXPECTED_CAPSTONE_VERSION}, found {capstone.__version__}")
    receipts = []
    bounded = []

    # Every DLL adapter is the fixed one-argument forwarding shim to its sole
    # slot-92 cell. Direct callsites below are checked separately.
    for name, spec in EXPECTED_ADAPTERS.items():
        im = images[name]
        cell = spec["slot_va"]
        refs = modules[name]["local_host_table_slot_92"]["accesses"]
        expected_ref_rva = spec["rva"] + 5
        if len(refs) != 1 or int(refs[0]["rva"], 16) != expected_ref_rva:
            raise RuntimeError(f"acceptance gate failed: {name} slot-92 must have one fixed adapter access")
        if refs[0]["mnemonic"] != "call" or refs[0]["operands"] != f"dword ptr [{cell:#x}]":
            raise RuntimeError(f"acceptance gate failed: {name} slot-92 adapter call operand changed")
        decoded = decode_bounded(im, spec["rva"], 0x10)
        expected = [
            ("mov", "eax, dword ptr [esp + 4]"),
            ("push", "eax"),
            ("call", f"dword ptr [{cell:#x}]"),
            ("add", "esp, 4"),
            ("ret", ""),
            ("nop", ""),
        ]
        gate_exact_instructions(f"{name} adapter", decoded, expected)
        bounded.append({"module": name, "label": "slot92_adapter", "rva": hex(spec["rva"]),
                        "size": 0x10, "instructions": decoded})
        receipts.append({"gate": f"{name} has one fixed one-argument slot-92 adapter", "passed": True})

    # There are exactly five direct wrapper callers in the pinned images. Each
    # bounded caller span must decode cleanly and exactly as push NULL, optional
    # unrelated MOV, CALL adapter, caller cleanup.
    observed = {}
    for name, expectation_list in EXPECTED_CALLS.items():
        actual_callers = modules[name]["slot92_adapter"]["direct_callers"]
        got_rvas = sorted(int(x["rva"], 16) for x in actual_callers)
        want_rvas = sorted(item["rva"] for item in expectation_list)
        if got_rvas != want_rvas:
            raise RuntimeError(f"acceptance gate failed: {name} slot-92 caller RVAs {got_rvas!r} != {want_rvas!r}")
        for spec in expectation_list:
            rows = decode_bounded(images[name], spec["span_rva"], spec["span_size"])
            expected = spec["instructions"]
            gate_exact_instructions(f"{name} caller {spec['rva']:#x}", rows, expected)
            caller = next(x for x in actual_callers if int(x["rva"], 16) == spec["rva"])
            if (caller["argument_value_if_direct_push"] != "0x0"
                    or int(caller["last_unbalanced_push"]["rva"], 16) != spec["span_rva"]
                    or caller["cleanup_after_call"] is None
                    or caller["cleanup_after_call"]["mnemonic"] != "add"
                    or caller["cleanup_after_call"]["operands"] != "esp, 4"):
                raise RuntimeError(f"acceptance gate failed: {name} caller {spec['rva']:#x} is not NULL + add esp,4")
            observed.setdefault(name, []).append({
                "call_rva": hex(spec["rva"]), "argument": "NULL",
                "cleanup": "add esp, 4", "instructions": rows,
                "sha256": sha(images[name].read_rva(spec["span_rva"], spec["span_size"])),
            })
            bounded.append({"module": name, "label": "slot92_adapter_callsite",
                            "rva": hex(spec["span_rva"]), "size": spec["span_size"],
                            "instructions": rows})
        receipts.append({"gate": f"{name} exact direct slot-92 call set and NULL arguments", "passed": True})

    setter = decode_bounded(images["HERCULES.EXE"], SETTER_RVA, 0x10)
    gate_exact_instructions("HERC BDA0 setter", setter, [
        ("mov", "ecx, dword ptr [esp + 4]"),
        ("mov", "eax, dword ptr [0xa8e780]"),
        ("mov", "dword ptr [0xa8e780], ecx"),
        ("ret", ""),
    ])
    bounded.append({"module": "HERCULES.EXE", "label": "bda0_primary_setter",
                    "rva": hex(SETTER_RVA), "size": 0x10, "instructions": setter})
    receipts.append({"gate": "BDA0 reads one cdecl callback argument, returns old pointer, and writes A8E780", "passed": True})

    bd80 = decode_bounded(images["HERCULES.EXE"], DISPATCH_RVA, 0x16)
    gate_exact_instructions("HERC BD80 dispatcher", bd80, [
        ("mov", "eax, dword ptr [0xa8e780]"), ("test", "eax, eax"),
        ("je", "0x40bd95"), ("mov", "ecx, dword ptr [0xa8e788]"),
        ("test", "ecx, ecx"), ("je", "0x40bd95"), ("jmp", "eax"), ("ret", ""),
    ])
    bounded.append({"module": "HERCULES.EXE", "label": "bd80_dispatch",
                    "rva": hex(DISPATCH_RVA), "size": 0x16, "instructions": bd80})
    receipts.append({"gate": "BD80 tests target/gate and tail-jumps without argument setup", "passed": True})

    reset = decode_bounded(images["HERCULES.EXE"], RESET_RVA, 0x17)
    gate_exact_instructions("HERC BDF0 reset", reset, [
        ("xor", "eax, eax"), ("mov", "dword ptr [0xa8e788], 1"),
        ("mov", "dword ptr [0xa8e780], eax"),
        ("mov", "dword ptr [0xa8e784], eax"), ("ret", ""),
    ])
    bounded.append({"module": "HERCULES.EXE", "label": "bdf0_gate_enable_and_clear",
                    "rva": hex(RESET_RVA), "size": 0x17, "instructions": reset})
    receipts.append({"gate": "BDF0 enables shared gate and clears primary/secondary targets", "passed": True})

    disable = decode_bounded(images["HERCULES.EXE"], 0xBDE0, 0x0D)
    gate_exact_instructions("HERC BDE0 gate disable", disable, [
        ("mov", "dword ptr [0xa8e788], 0"), ("xor", "eax, eax"), ("ret", ""),
    ])
    bounded.append({"module": "HERCULES.EXE", "label": "bde0_gate_disable",
                    "rva": "0xbde0", "size": 0x0D, "instructions": disable})
    receipts.append({"gate": "BDE0 disables the shared callback gate", "passed": True})

    if source_slot92_target != images["HERCULES.EXE"].image_base + SETTER_RVA:
        raise RuntimeError("acceptance gate failed: HERC source-table slot 92 is not BDA0")
    if modules["HERCULES.EXE"]["direct_calls_to_host_primary_setter"]:
        raise RuntimeError("acceptance gate failed: expected no direct HERC call to BDA0")
    if any(m["direct_calls_to_host_primary_setter"] for n, m in modules.items() if n != "HERCULES.EXE"):
        raise RuntimeError("acceptance gate failed: unexpected absolute direct call to BDA0 from DLL")
    herc_insns = module_insns["HERCULES.EXE"]
    bdf0_callers = direct_transfers(herc_insns, images["HERCULES.EXE"].image_base,
                                     images["HERCULES.EXE"].image_base + RESET_RVA)
    init_callers = direct_transfers(herc_insns, images["HERCULES.EXE"].image_base,
                                    images["HERCULES.EXE"].image_base + 0x10690)
    if [int(x["rva"], 16) for x in bdf0_callers] != [0x106E2]:
        raise RuntimeError(f"acceptance gate failed: BDF0 callsites changed: {bdf0_callers!r}")
    if [int(x["rva"], 16) for x in init_callers] != [0x2043]:
        raise RuntimeError(f"acceptance gate failed: initializer callers changed: {init_callers!r}")
    receipts.append({"gate": "source slot 92 maps to BDA0 and no direct BDA0 calls exist", "passed": True})
    receipts.append({"gate": "BDF0 is called at 0x106E2 and initializer 0x10690 at 0x2043", "passed": True})
    receipts.append({"gate": "original EXE/DLL SHA-256 and image-base pins", "passed": True})
    receipts.append({"gate": f"Capstone version is pinned at {EXPECTED_CAPSTONE_VERSION}", "passed": True})
    return receipts, bounded, observed


def abs_memory_refs(insns, base, addresses):
    found = {hex(a): [] for a in addresses}
    for i in insns:
        for op in i.operands:
            if (op.type == capstone.x86.X86_OP_MEM and op.mem.base == 0
                    and op.mem.index == 0 and op.mem.disp in addresses):
                found[hex(op.mem.disp)].append({
                    "rva": hex(i.address - base), "mnemonic": i.mnemonic,
                    "operands": i.op_str,
                    "access": ("write" if op.access & capstone.CS_AC_WRITE else
                               "read" if op.access & capstone.CS_AC_READ else "unknown"),
                })
    return found


def direct_transfers(insns, base, va):
    rows = []
    for i in insns:
        if i.mnemonic not in ("call", "jmp"):
            continue
        if any(op.type == capstone.x86.X86_OP_IMM and op.imm == va for op in i.operands):
            rows.append({"rva": hex(i.address - base), "mnemonic": i.mnemonic, "operands": i.op_str})
    return rows


def adapter_callers(insns, base, rva):
    rows = []
    ordered = sorted(insns, key=lambda i: i.address)
    by_end = {i.address + i.size: i for i in ordered}
    by_address = {i.address: i for i in ordered}
    for i in ordered:
        if i.mnemonic not in ("call", "jmp"):
            continue
        if not any(op.type == capstone.x86.X86_OP_IMM and op.imm == base + rva
                   for op in i.operands):
            continue
        # Instruction lengths vary, so locate the immediately preceding decoded
        # instruction by its end address rather than assuming a byte distance.
        previous = by_end.get(i.address)
        following = by_address.get(i.address + i.size)
        # A harmless MOV can separate the argument push and call (two observed
        # reset paths do this), so find the last stack push without crossing a
        # call, pop, return, or explicit ESP adjustment.
        push = None
        cursor = previous
        while cursor is not None:
            if cursor.mnemonic == "push":
                push = cursor
                break
            if (cursor.mnemonic in ("call", "jmp", "ret", "retn", "pop")
                    or (cursor.mnemonic in ("add", "sub") and cursor.operands
                        and cursor.operands[0].type == capstone.x86.X86_OP_REG
                        and cursor.operands[0].reg == capstone.x86.X86_REG_ESP)):
                break
            cursor = by_end.get(cursor.address)
        arg = None
        if push is not None and push.operands:
            op = push.operands[0]
            if op.type == capstone.x86.X86_OP_IMM:
                arg = op.imm & 0xFFFFFFFF
        rows.append({
            "rva": hex(i.address - base), "mnemonic": i.mnemonic,
            "immediate_predecessor": None if previous is None else {
                "rva": hex(previous.address - base), "mnemonic": previous.mnemonic,
                "operands": previous.op_str},
            "last_unbalanced_push": None if push is None else {
                "rva": hex(push.address - base), "mnemonic": push.mnemonic,
                "immediate": None if not push.operands or push.operands[0].type != capstone.x86.X86_OP_IMM
                    else hex(push.operands[0].imm & 0xFFFFFFFF)},
            "argument_value_if_direct_push": None if arg is None else hex(arg),
            "cleanup_after_call": None if following is None else {
                "rva": hex(following.address - base), "mnemonic": following.mnemonic,
                "operands": following.op_str},
        })
    return rows


def code_span(im, label, rva, size):
    return {"module": Path(im.path).name, "label": label, "rva": hex(rva),
            "size": size, "sha256": sha(im.read_rva(rva, size))}


def section_location(im, rva):
    for section in im.sections:
        start = section["virtual_address"]
        delta = rva - start
        if 0 <= delta < section["virtual_size"]:
            return {"section": section["name"], "section_rva": hex(start),
                    "section_offset": hex(delta), "raw_size": section["raw_size"],
                    "virtual_size": section["virtual_size"],
                    "file_backed": delta < section["raw_size"]}
    return None


def occurrences(im, value):
    needle = value.to_bytes(4, "little")
    rows = []
    start = 0
    while True:
        off = im.data.find(needle, start)
        if off < 0:
            break
        for sec in im.sections:
            if sec["raw_offset"] <= off < sec["raw_offset"] + sec["raw_size"]:
                rows.append({"section": sec["name"],
                             "rva": hex(sec["virtual_address"] + off - sec["raw_offset"])})
                break
        else:
            if off < im.pe.OPTIONAL_HEADER.SizeOfHeaders:
                rows.append({"section": "headers", "file_offset": hex(off)})
        start = off + 1
    return rows


def main():
    args = parse_args()
    paths = image_paths(args)
    images = {}
    modules = {}
    module_insns = {}
    for name, path in paths.items():
        im = PE(path)
        digest = sha(im.data)
        expected, base = PINNED[name]
        if digest != expected or im.image_base != base:
            raise SystemExit(f"pinned image mismatch: {name} sha={digest} base={im.image_base:#x}")
        images[name] = im
        text, insns, coverage = code_sweep(im)
        module_insns[name] = insns
        watched = [PRIMARY, SECONDARY, GATE]
        module_entry = {
            "path_role": "host exe" if name == "HERCULES.EXE" else "shipped DLL",
            "sha256": digest, "image_base": hex(base),
            "text": {"rva": hex(text["virtual_address"]), "raw_size": text["raw_size"],
                     "sha256": sha(im.read_rva(text["virtual_address"], text["raw_size"])),
                     **coverage},
            "direct_absolute_callback_cell_refs": abs_memory_refs(insns, base, watched),
            "direct_calls_to_host_primary_setter": direct_transfers(insns, base, 0x400000 + SETTER_RVA),
        }
        if name in TABLE_ROOTS:
            cell = base + TABLE_ROOTS[name] + 92 * 4
            module_entry["local_host_table_slot_92"] = {
                "cell_va": hex(cell), "accesses": abs_memory_refs(insns, base, [cell])[hex(cell)]}
            adapter_rva = SLOT92_ADAPTERS[name]
            module_entry["slot92_adapter"] = {
                "rva": hex(adapter_rva),
                "direct_callers": adapter_callers(insns, base, adapter_rva),
            }
        modules[name] = module_entry

    herc = images["HERCULES.EXE"]
    title = images["TITLE.DLL"]
    source_slot92_rva = 0x361B8 + 92 * 4
    source_slot92_target = int.from_bytes(herc.read_rva(source_slot92_rva, 4), "little")
    if source_slot92_target != herc.image_base + SETTER_RVA:
        raise SystemExit(f"unexpected HERC source-table slot 92 target: {source_slot92_target:#x}")
    acceptance_gates, bounded_decodes, accepted_call_sites = run_acceptance_gates(
        images, modules, module_insns, source_slot92_target)
    metadata = {name: images[name].metadata() for name in images}
    # Values are searched in raw file bytes as corroboration only; occurrences
    # can be non-code immediates or unrelated data, so they are not call proof.
    title_adapter_va = title.image_base + TITLE_ADAPTER_RVA
    adapter_occurrences = {name: occurrences(images[name], title_adapter_va) for name in images}
    setter_va = herc.image_base + SETTER_RVA
    setter_occurrences = {name: occurrences(images[name], setter_va) for name in images}

    # Fingerprinted small spans make this audit independently checkable without
    # copying original bytes into its receipt.
    span_defs = {
        "HERCULES.EXE": [("bd80_dispatch", DISPATCH_RVA, 0x16),
                         ("bda0_primary_setter", SETTER_RVA, 0x10),
                         ("bde0_gate_disable", 0xBDE0, 0x0D),
                         ("bdf0_gate_enable_and_clear", RESET_RVA, 0x17)],
        "TITLE.DLL": [("slot92_adapter", TITLE_ADAPTER_RVA, 0x10)],
    }
    spans = []
    for name, defs in span_defs.items():
        im = images[name]
        for label, rva, size in defs:
            data = im.read_rva(rva, size)
            spans.append({"module": name, "label": label, "rva": hex(rva),
                          "size": size, "sha256": sha(data)})

    title_exports = [{"name": e["name"], "ordinal": e["ordinal"], "rva": hex(e["rva"])}
                     for e in metadata["TITLE.DLL"]["exports"]]
    report = {
        "schema_version": 1,
        "scope": "Static primary BD80 callback producer and direct write audit; no original execution or writes.",
        "prediction_history": PREDICTION_HISTORY,
        "tooling": {"reader": "repository scripts/pe.py", "capstone": capstone.__version__,
                    "decode": "Capstone x86-32 detail + skipdata full raw .text linear sweep",
                    "audit_source_sha256": sha(Path(__file__).read_bytes())},
        "acceptance_gates": acceptance_gates,
        "bounded_semantic_decodes": bounded_decodes,
        "accepted_direct_call_sites": accepted_call_sites,
        "module_scans": modules,
        "cells": {
            "primary_target_va": hex(PRIMARY), "secondary_target_va": hex(SECONDARY),
            "shared_gate_va": hex(GATE),
            "primary_target_storage": {"rva": hex(PRIMARY - 0x400000),
                                       **section_location(herc, PRIMARY - herc.image_base)},
            "host_setter": {"rva": hex(SETTER_RVA), "signature": "cdecl: one 32-bit callback address argument; returns previous target in EAX",
                            "setter_callers_direct": modules["HERCULES.EXE"]["direct_calls_to_host_primary_setter"]},
            "reset": {"rva": hex(RESET_RVA), "effect": "gate=1; primary=0; secondary=0"},
            "bd80": {"rva": hex(DISPATCH_RVA),
                     "effect": "if primary != 0 and shared gate != 0, tail-jump primary; no args are explicitly pushed by this stub"},
        },
        "title_slot92_adapter": {
            "rva": hex(TITLE_ADAPTER_RVA), "signature": "one 32-bit argument forwarded through TITLE local host-table slot 92",
            "host_table_cell_va": hex(title.image_base + TABLE_ROOTS["TITLE.DLL"] + 92 * 4),
            "slot92_direct_accesses": modules["TITLE.DLL"]["local_host_table_slot_92"]["accesses"],
            "direct_code_callers": modules["TITLE.DLL"]["slot92_adapter"]["direct_callers"],
            "raw_pointer_occurrences_by_module": adapter_occurrences,
            "exports_matching_adapter": [e for e in title_exports if int(e["rva"], 16) == TITLE_ADAPTER_RVA],
        },
        "all_static_slot92_adapter_calls": {
            name: modules[name]["slot92_adapter"]["direct_callers"] for name in SLOT92_ADAPTERS
        },
        "host_source_table_slot_92": {
            "rva": hex(source_slot92_rva), "target_va": hex(source_slot92_target),
            "target_rva": hex(source_slot92_target - herc.image_base),
            "meaning": "host interface source-table entry for BDA0; DLL local cells above are copied-table call targets",
        },
        "startup_gate_path": {
            "bdf0_callers": direct_transfers(module_insns["HERCULES.EXE"], herc.image_base,
                                              herc.image_base + RESET_RVA),
            "bde0_callers": direct_transfers(module_insns["HERCULES.EXE"], herc.image_base,
                                              herc.image_base + 0xBDE0),
            "initializer_rva": "0x10690",
            "initializer_callers": direct_transfers(module_insns["HERCULES.EXE"], herc.image_base,
                                                     herc.image_base + 0x10690),
        },
        "primary_setter_pointer_occurrences_by_module": setter_occurrences,
        "spans": spans,
        "conclusion": {
            "fixed_primary_callback_target": None,
            "primary_installer_producer": "no non-null static producer was found: the five directly observed calls into ENG1/ENG3/TITLE slot-92 adapters all push NULL; the runtime value remains unresolved because the setter is externally callable through copied host table slot 92",
            "write_set": "unknown for the runtime-installed primary callback; BD80 imposes no visible no-alias or write-set rule. Preserve it as a synchronous callback boundary and reload live order-table links after it.",
            "renderer_state_mutation": "cannot prove absence of OT/page/LUT/renderer-global mutation; likewise no evidence that the shipped runtime callback does so",
            "coverage_limit": "full raw .text byte coverage and direct absolute references are established; linear sweep can decode embedded data, and computed register/table aliases, indirect callers, or runtime code generation are not ruled out",
        },
    }
    for name, adapter_rva in SLOT92_ADAPTERS.items():
        report["spans"].append(code_span(images[name], "slot92_adapter", adapter_rva, 0x10))
        for caller in modules[name]["slot92_adapter"]["direct_callers"]:
            caller_rva = int(caller["rva"], 16)
            push = caller.get("last_unbalanced_push")
            lo = int(push["rva"], 16) if push is not None else max(0, caller_rva - 2)
            clean = caller.get("cleanup_after_call")
            hi = caller_rva + 5
            if clean is not None and clean["mnemonic"] == "add":
                clean_rva = int(clean["rva"], 16)
                clean_insn = next((ins for ins in module_insns[name]
                                   if ins.address - images[name].image_base == clean_rva), None)
                if clean_insn is not None:
                    hi = clean_rva + clean_insn.size
            report["spans"].append(code_span(images[name], "slot92_adapter_callsite", lo, hi - lo))
    report["spans"].append(code_span(herc, "host_interface_source_slot_92",
                                     source_slot92_rva, 4))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(args.output), "sha256": sha(args.output.read_bytes()),
                      "modules": list(modules),
                      "text_coverage": {n: m["text"]["covered_bytes"] for n, m in modules.items()},
                      "title_slot92_accesses": len(modules["TITLE.DLL"]["local_host_table_slot_92"]["accesses"]),
                      "adapter_direct_callers": {n: len(modules[n]["slot92_adapter"]["direct_callers"]) for n in SLOT92_ADAPTERS},
                      "primary_exe_direct_refs": modules["HERCULES.EXE"]["direct_absolute_callback_cell_refs"][hex(PRIMARY)]},
                     indent=2))


if __name__ == "__main__":
    main()
