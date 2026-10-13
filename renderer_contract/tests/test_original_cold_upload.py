#!/usr/bin/env python3
"""Bounded original A710 execution for the literal ENG1/ENG3 cold-panel RECT."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import struct
import sys
from pathlib import Path

EXPECTED_EXE = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
EXPECTED_PUBLIC_HELPERS = {
    "scripts/env.py": "1b743fc8adbb5df32b167fd44c39b8026bb66115ad81d68a32979c38e45047e4",
    "scripts/pe.py": "8da71eb87b5a88dd1d80a3fe180e2b94557e2c3017fb4e5c37c4a8c630af62ca",
    "renderer_contract/tests/oracle_common.py": "a024048d9acc6e18197e10a82427cd84351a90666e1626e610bf60c6105f1357",
}
EXPECTED_UNICORN = Path(r"C:\tools\hercules\renderer-oracle-python")
EXPECTED_ANALYSIS = Path(r"C:\tools\hercules\python")
EXE_BASE = 0x00400000
ENTRY_RVA, ENTRY_END = 0xA710, 0xA8E5
NOTIFY_RVA, NOTIFY_END = 0xBD80, 0xBD96
DEST_BEGIN, DEST_END = 0x00A6E6E0, 0x00A8E6E0
CALLBACK_CELL, GATE_CELL = 0x00A8E780, 0x00A8E788
INPUT_BASE, INPUT_SIZE = 0x10000000, 0x22000
STACK_BASE, STACK_SIZE = 0x20000000, 0x10000
RETURN_SENTINEL, UNKNOWN_CALLBACK = 0x30000000, 0x31000000
SOURCE_BYTES = 0x20000
MAX_INSTRUCTIONS, TIMEOUT_US = 250000, 10000000
COPY_PCS = {EXE_BASE + 0xA78D, EXE_BASE + 0xA794, EXE_BASE + 0xA8A9}
BD80_PATH = (0xBD80, 0xBD85, 0xBD87, 0xBD89, 0xBD8F, 0xBD91, 0xBD95)
STATE_CELLS = (
    (0x00A8E6F4, 4), (0x00A8E6F8, 4), (0x00A8E6FC, 4),
    (0x00A909AC, 4), (0x00A909FC, 4),
    (0x00A90A00, 4), (0x00A90A04, 4), (0x00A90A08, 4),
    (0x00A8E918, 4),
)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-root", type=Path, required=True)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--deps", type=Path, default=EXPECTED_UNICORN)
    parser.add_argument("--analysis-deps", type=Path, default=EXPECTED_ANALYSIS)
    args = parser.parse_args()
    args.original_root = args.original_root.resolve()
    args.exe = args.exe.resolve()
    args.output = (args.output if args.output.is_absolute()
                   else args.original_root / args.output).resolve()
    args.deps = args.deps.resolve()
    args.analysis_deps = args.analysis_deps.resolve()
    work = (args.original_root / "work").resolve()
    try:
        args.output.relative_to(work)
    except ValueError:
        parser.error("--output must resolve below --original-root/work")
    if args.output == work:
        parser.error("--output must name a file below --original-root/work")
    if not args.exe.is_file() or sha256(args.exe.read_bytes()) != EXPECTED_EXE:
        parser.error("--exe must be the pinned, read-only original HERCULES.EXE")
    if not (args.original_root / "scripts" / "pe.py").is_file():
        parser.error("--original-root must contain scripts/pe.py")
    if not (args.original_root / "renderer_contract" / "tests" / "oracle_common.py").is_file():
        parser.error("--original-root must contain renderer_contract/tests/oracle_common.py")
    if not (args.original_root / "renderer_contract" / "hercules_textures.h").is_file():
        parser.error("--original-root must contain renderer_contract/hercules_textures.h")
    for relative, expected in EXPECTED_PUBLIC_HELPERS.items():
        if sha256((args.original_root / relative).read_bytes()) != expected:
            parser.error(f"public helper identity changed: {relative}")
    for path in (args.deps, args.analysis_deps):
        if not path.is_dir():
            parser.error(f"missing pinned dependency directory: {path}")
    os.environ["HERCULES_PYTHON_DEPS"] = str(args.analysis_deps)
    sys.path[:0] = [
        str(args.deps), str(args.original_root / "scripts"),
        str(args.original_root / "renderer_contract" / "tests"),
    ]
    return args


ARGS = parse_args()
from oracle_common import verify_runtime, write_receipt  # noqa: E402
from pe import PE  # noqa: E402
import unicorn  # noqa: E402
from unicorn import (  # noqa: E402
    Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_INVALID,
    UC_HOOK_MEM_WRITE, UC_MODE_32, UC_PROT_EXEC, UC_PROT_READ, UC_PROT_WRITE,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EFLAGS, UC_X86_REG_EIP, UC_X86_REG_ESP,
)


def align_up(value: int) -> int:
    return (value + 0xFFF) & ~0xFFF


def section_permissions(characteristics: int) -> int:
    result = 0
    if characteristics & 0x40000000:
        result |= UC_PROT_READ
    if characteristics & 0x80000000:
        result |= UC_PROT_WRITE
    if characteristics & 0x20000000:
        result |= UC_PROT_EXEC
    return result


def map_original_sections(uc, pe):
    headers = pe.pe.OPTIONAL_HEADER.SizeOfHeaders
    uc.mem_map(EXE_BASE, align_up(headers), UC_PROT_READ)
    uc.mem_write(EXE_BASE, pe.data[:headers])
    mapped = []
    for section in pe.sections:
        size = align_up(max(section["virtual_size"], section["raw_size"]))
        address = EXE_BASE + section["virtual_address"]
        permissions = section_permissions(section["characteristics"])
        uc.mem_map(address, size, permissions)
        if section["raw_size"]:
            uc.mem_write(address, pe.read_rva(section["virtual_address"], section["raw_size"]))
        raw_size = section["raw_size"]
        if size > raw_size:
            uc.mem_write(address + raw_size, b"\0" * (size - raw_size))
        mapped.append({"name": section["name"], "va": address, "size": size})
    return mapped


def high_row(y: int) -> int:
    if not 512 <= y <= 1023:
        raise ValueError("fixture uses only the observed 512..1023 high-y branch rows")
    return 0x1000 + (y & 0x1FF)


def main():
    pe = PE(ARGS.exe)
    if pe.image_base != EXE_BASE:
        raise AssertionError(f"unexpected image base {pe.image_base:#x}")
    if getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        raise AssertionError("fixed-base original unexpectedly has relocations")

    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    mapped = map_original_sections(uc, pe)
    uc.mem_map(INPUT_BASE, INPUT_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(RETURN_SENTINEL, 0x1000, UC_PROT_READ | UC_PROT_EXEC)

    data_section = next(item for item in pe.sections if item["name"] == ".data")
    data_va = EXE_BASE + data_section["virtual_address"]
    data_size = align_up(max(data_section["virtual_size"], data_section["raw_size"]))
    before_data = bytes(uc.mem_read(data_va, data_size))
    before_a710 = bytes(uc.mem_read(EXE_BASE + ENTRY_RVA, ENTRY_END - ENTRY_RVA))
    before_bd80 = bytes(uc.mem_read(EXE_BASE + NOTIFY_RVA, NOTIFY_END - NOTIFY_RVA))
    state_before = {
        f"0x{address:08x}": bytes(uc.mem_read(address, size)).hex()
        for address, size in STATE_CELLS
    }

    x, y, width, height = (0, 512, 128, 512)
    rect_va, source_va = INPUT_BASE + 0x100, INPUT_BASE + 0x1000
    source = bytes((index * 37 + 11) & 0xFF for index in range(SOURCE_BYTES))
    uc.mem_write(rect_va, struct.pack("<4h", x, y, width, height))
    uc.mem_write(source_va, source)
    uc.mem_write(CALLBACK_CELL, struct.pack("<I", UNKNOWN_CALLBACK))
    uc.mem_write(GATE_CELL, struct.pack("<I", 0))

    stack = STACK_BASE + STACK_SIZE - 0x100
    uc.mem_write(stack, struct.pack("<III", RETURN_SENTINEL, rect_va, source_va))
    uc.reg_write(UC_X86_REG_ESP, stack)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
    uc.reg_write(UC_X86_REG_EIP, EXE_BASE + ENTRY_RVA)

    trace = []
    write_events = []
    coverage = bytearray(SOURCE_BYTES)
    stop = {"kind": None, "detail": None}
    data_end = data_va + data_size
    scratch = ((INPUT_BASE, INPUT_BASE + INPUT_SIZE),
               (STACK_BASE, STACK_BASE + STACK_SIZE))

    def on_code(machine, address, _size, _user):
        if address == RETURN_SENTINEL:
            stop["kind"], stop["detail"] = "return_sentinel", hex(address)
            machine.emu_stop()
            return
        if not (EXE_BASE + ENTRY_RVA <= address < EXE_BASE + ENTRY_END or
                EXE_BASE + NOTIFY_RVA <= address < EXE_BASE + NOTIFY_END):
            stop["kind"], stop["detail"] = "unexpected_code", hex(address)
            machine.emu_stop()
            return
        trace.append(address)

    def on_write(machine, _access, address, size, value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP)
        if data_va <= address and address + size <= data_end:
            if not (DEST_BEGIN <= address and address + size <= DEST_END):
                stop["kind"] = "unexpected_data_write"
                stop["detail"] = {"pc": hex(pc), "address": hex(address), "size": size}
                machine.emu_stop()
                return
            if pc not in COPY_PCS:
                stop["kind"] = "unexpected_write_pc"
                stop["detail"] = {"pc": hex(pc), "address": hex(address), "size": size}
                machine.emu_stop()
                return
            offset = address - DEST_BEGIN
            raw = int(value).to_bytes(size, "little", signed=False)
            if raw != source[offset:offset + size]:
                stop["kind"] = "write_value_mismatch"
                stop["detail"] = {"pc": hex(pc), "address": hex(address), "size": size}
                machine.emu_stop()
                return
            coverage[offset:offset + size] = b"\1" * size
            write_events.append((pc, address, size))
            return
        if any(start <= address and address + size <= end for start, end in scratch):
            return
        stop["kind"] = "unexpected_write"
        stop["detail"] = {"pc": hex(pc), "address": hex(address), "size": size}
        machine.emu_stop()

    def on_invalid(machine, access, address, size, _value, _user):
        stop["kind"] = "invalid_memory"
        stop["detail"] = {
            "pc": hex(machine.reg_read(UC_X86_REG_EIP)), "access": access,
            "address": hex(address), "size": size,
        }
        machine.emu_stop()
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
    uc.hook_add(UC_HOOK_MEM_INVALID, on_invalid)
    try:
        uc.emu_start(EXE_BASE + ENTRY_RVA, 0, timeout=TIMEOUT_US, count=MAX_INSTRUCTIONS)
    except UcError as error:
        if stop["kind"] is None:
            stop["kind"], stop["detail"] = "unicorn_error", str(error)

    if stop["kind"] != "return_sentinel":
        raise AssertionError(f"A710 did not return cleanly: {stop}")
    if trace[0] != EXE_BASE + ENTRY_RVA:
        raise AssertionError("execution did not begin at original A710")
    if trace.count(EXE_BASE + NOTIFY_RVA) != 1:
        raise AssertionError("original BD80 notification was not called exactly once")
    bd80_trace = [address - EXE_BASE for address in trace
                  if EXE_BASE + NOTIFY_RVA <= address < EXE_BASE + NOTIFY_END]
    if any(address not in bd80_trace for address in BD80_PATH) or 0xBD93 in bd80_trace:
        raise AssertionError(f"disabled BD80 gate path differed: {bd80_trace}")
    if any(coverage) is False or coverage.count(1) != SOURCE_BYTES:
        raise AssertionError("original write hooks did not cover the whole output interval")
    if bytes(uc.mem_read(DEST_BEGIN, SOURCE_BYTES)) != source:
        raise AssertionError("original output bytes differ from the synthetic source")
    observed_output_sha256 = sha256(bytes(uc.mem_read(DEST_BEGIN, SOURCE_BYTES)))

    expected_data = bytearray(before_data)
    expected_data[DEST_BEGIN - data_va:DEST_END - data_va] = source
    expected_data[CALLBACK_CELL - data_va:CALLBACK_CELL - data_va + 4] = struct.pack(
        "<I", UNKNOWN_CALLBACK)
    expected_data[GATE_CELL - data_va:GATE_CELL - data_va + 4] = struct.pack("<I", 0)
    after_data = bytes(uc.mem_read(data_va, data_size))
    if after_data != bytes(expected_data):
        raise AssertionError(".data changed outside the exact upload and fixture cells")
    state_after = {
        f"0x{address:08x}": bytes(uc.mem_read(address, size)).hex()
        for address, size in STATE_CELLS
    }
    if state_after != state_before:
        raise AssertionError("page/state/reciprocal witness cells changed")
    if bytes(uc.mem_read(EXE_BASE + ENTRY_RVA, ENTRY_END - ENTRY_RVA)) != before_a710:
        raise AssertionError("original A710 bytes changed")
    if bytes(uc.mem_read(EXE_BASE + NOTIFY_RVA, NOTIFY_END - NOTIFY_RVA)) != before_bd80:
        raise AssertionError("original BD80 bytes changed")
    if uc.reg_read(UC_X86_REG_EIP) != RETURN_SENTINEL:
        raise AssertionError("return sentinel was not reached")

    # Restore the whole emulator-local .data snapshot and check byte-for-byte.
    uc.mem_write(data_va, before_data)
    if bytes(uc.mem_read(data_va, data_size)) != before_data:
        raise AssertionError("private .data image failed exact restoration")

    source_hash = sha256(source)
    row_first = DEST_BEGIN + (high_row(512) << 8) - 0x100000
    row_last = DEST_BEGIN + (high_row(1023) << 8) - 0x100000 + 0xFF
    if (row_first, row_last) != (DEST_BEGIN, DEST_END - 1):
        raise AssertionError("static high-band row endpoint expression changed")

    receipt = {
        "schema_version": 1,
        "status": "FINITE_ORIGINAL_EXECUTION",
        "scope": "One literal ENG1/ENG3 startup upload RECT with synthetic input bytes; no allocation-ownership, complete callback-domain, native-process, gameplay, or renderer-output claim.",
        "oracle": {
            "exe_sha256": sha256(pe.data), "image_base": hex(pe.image_base),
            "a710_rva": hex(ENTRY_RVA),
            "a710_span_sha256": sha256(pe.read_rva(ENTRY_RVA, ENTRY_END - ENTRY_RVA)),
            "bd80_rva": hex(NOTIFY_RVA),
            "bd80_span_sha256": sha256(pe.read_rva(NOTIFY_RVA, NOTIFY_END - NOTIFY_RVA)),
            "base_relocations": 0,
        },
        "public_helpers": EXPECTED_PUBLIC_HELPERS,
        "fixture": {
            "rectangle_signed_words": [x, y, width, height],
            "source_bytes": SOURCE_BYTES, "source_pattern": "(i*37+11)&0xff",
            "source_sha256": source_hash,
            "expected_output_interval": [hex(DEST_BEGIN), hex(DEST_END)],
            "observed_write_bytes": coverage.count(1),
            "observed_write_event_count": len(write_events),
            "write_pc_byte_counts": {
                hex(pc - EXE_BASE): sum(size for event_pc, _address, size in write_events
                                        if event_pc == pc)
                for pc in sorted({event_pc for event_pc, _address, _size in write_events})
            },
            "observed_output_sha256": observed_output_sha256,
            "output_matches_synthetic_source": True,
            "whole_data_diff_exactly_output_plus_fixture_cells": True,
            "state_page_color_and_reciprocal_cells_unchanged": list(state_before),
            "bd80_call_count": 1, "bd80_disabled_gate_trace_rvas": [hex(v) for v in bd80_trace],
            "callback_endpoint_executed": False,
            "original_code_spans_unchanged": True,
            "private_data_image_restored": True,
            "instruction_budget": MAX_INSTRUCTIONS,
            "timeout_us": TIMEOUT_US,
            "trace_instruction_count": len(trace),
        },
        "mapped_sections": [
            {"name": item["name"], "va": hex(item["va"]), "size": hex(item["size"])}
            for item in mapped
        ],
        "limitations": [
            "The exact literal RECT is y=512 through 1023; this does not validate other high-y values or allocation ownership.",
            "The R[0] overlap at x=284,y=514 is only a separately tested address-expression witness, not exercised by this producer fixture.",
            "BD80's original gated body ran; the callback endpoint stayed disabled and no host notification implementation was fabricated.",
        ],
        "runtime": verify_runtime(unicorn, ARGS.deps),
        "invocation": {
            "exe": str(ARGS.exe), "original_root": str(ARGS.original_root),
            "deps": str(ARGS.deps), "analysis_deps": str(ARGS.analysis_deps),
        },
    }
    write_receipt(ARGS.output, receipt)
    print(json.dumps({
        "status": receipt["status"], "rectangle": receipt["fixture"]["rectangle_signed_words"],
        "output": receipt["fixture"]["expected_output_interval"],
        "write_bytes": receipt["fixture"]["observed_write_bytes"],
        "write_events": receipt["fixture"]["observed_write_event_count"],
        "bd80_trace": receipt["fixture"]["bd80_disabled_gate_trace_rvas"],
        "receipt": str(ARGS.output),
    }, indent=2))


if __name__ == "__main__":
    main()
