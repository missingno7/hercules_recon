"""Finite CPU-only execution oracle for original HERCULES.EXE A710.

The target instructions remain the original PE bytes. Synthetic descriptor,
source, stack, return sentinel, and notification state live in Unicorn memory.
The address expression in expected_addresses is an independent assertion from
the reviewed static instruction analysis; actual bytes and PC write addresses
come from executing the original x86 code.
"""
from __future__ import annotations

import hashlib
import json
import struct
from collections import Counter

from oracle_common import parse_args, verify_runtime, write_receipt  # noqa: E402

ARGS = parse_args("Run finite original HERCULES.EXE A710 upload fixtures", "report.json")
ROOT = ARGS.original_root

from pe import PE  # noqa: E402
import unicorn  # noqa: E402
from unicorn import (  # noqa: E402
    Uc,
    UcError,
    UC_ARCH_X86,
    UC_HOOK_CODE,
    UC_HOOK_MEM_INVALID,
    UC_HOOK_MEM_WRITE,
    UC_MODE_32,
    UC_PROT_EXEC,
    UC_PROT_READ,
    UC_PROT_WRITE,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EFLAGS,
    UC_X86_REG_EIP,
    UC_X86_REG_ESP,
)

PAGE = 0x1000
EXE_BASE = 0x00400000
ENTRY_RVA = 0x0000A710
ENTRY_END_RVA = 0x0000A8E5
NOTIFY_RVA = 0x0000BD80
NOTIFY_END_RVA = 0x0000BD96
TEXTURE_BASE = 0x0096E6E0
CALLBACK_CELL = 0x00A8E780
NOTIFICATION_GATE = 0x00A8E788

INPUT_BASE = 0x10000000
STACK_BASE = 0x20000000
STACK_SIZE = 0x10000
RETURN_SENTINEL = 0x30000000
UNKNOWN_CALLBACK = 0x31000000

MAX_INSTRUCTIONS = 100_000
EXPECTED_EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
COPY_PCS = {
    EXE_BASE + 0xA78D,
    EXE_BASE + 0xA794,
    EXE_BASE + 0xA7C5,
    EXE_BASE + 0xA873,
    EXE_BASE + 0xA87A,
    EXE_BASE + 0xA8A9,
}


def fail(message: str) -> None:
    raise AssertionError(message)


def align_up(value: int, alignment: int = PAGE) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def pe_permissions(characteristics: int) -> int:
    permissions = 0
    if characteristics & 0x40000000:
        permissions |= UC_PROT_READ
    if characteristics & 0x80000000:
        permissions |= UC_PROT_WRITE
    if characteristics & 0x20000000:
        permissions |= UC_PROT_EXEC
    return permissions


def expected_addresses(x: int, y: int, width: int, height: int):
    """Return address/source pairs from the reviewed native address equation."""
    for row_index in range(height):
        row = y + row_index
        if row < 512:
            row_index_native = (0 if row < 256 else 0x800) + (row & 0xFF)
        else:
            raise ValueError("fixture is outside the bounded y < 512 scope")
        for column in range(width):
            byte_x = 2 * (x + column)
            pixel = TEXTURE_BASE + row_index_native * 0x100
            pixel += ((byte_x & 0x700) << 8) + (byte_x & 0xFF)
            source_offset = row_index * width * 2 + column * 2
            yield pixel, source_offset
            yield pixel + 1, source_offset + 1


def cases():
    return [
        {"name": "sub256_fast_rows", "rect": (5, 224, 7, 3)},
        {"name": "x128_boundary_byte_rows", "rect": (127, 478, 3, 2)},
        {"name": "empty_height", "rect": (9, 240, 4, 0)},
    ]


def original_pc_address(address: int) -> bool:
    return (
        EXE_BASE + ENTRY_RVA <= address < EXE_BASE + ENTRY_END_RVA
        or EXE_BASE + NOTIFY_RVA <= address < EXE_BASE + NOTIFY_END_RVA
    )


def map_original_sections(uc: Uc, pe):
    mapped = []
    headers_size = pe.pe.OPTIONAL_HEADER.SizeOfHeaders
    uc.mem_map(EXE_BASE, align_up(headers_size), UC_PROT_READ)
    uc.mem_write(EXE_BASE, pe.data[:headers_size])
    mapped.append({"name": "headers", "va": EXE_BASE, "size": align_up(headers_size)})

    for section in pe.sections:
        virtual_size = max(section["virtual_size"], section["raw_size"])
        mapped_size = align_up(virtual_size)
        va = EXE_BASE + section["virtual_address"]
        uc.mem_map(va, mapped_size, pe_permissions(section["characteristics"]))
        if section["raw_size"]:
            raw = pe.read_rva(section["virtual_address"], section["raw_size"])
            uc.mem_write(va, raw)
        if mapped_size > section["raw_size"]:
            uc.mem_write(
                va + section["raw_size"],
                b"\0" * (mapped_size - section["raw_size"]),
            )
        mapped.append(
            {
                "name": section["name"],
                "va": va,
                "size": mapped_size,
                "virtual_size": section["virtual_size"],
                "raw_size": section["raw_size"],
                "permissions": section["characteristics"],
            }
        )
    return mapped


def data_section(pe):
    section = next(section for section in pe.sections if section["name"] == ".data")
    start = EXE_BASE + section["virtual_address"]
    length = align_up(max(section["virtual_size"], section["raw_size"]))
    return start, length


def run_case(pe, case):
    x, y, width, height = case["rect"]
    if min(x, y, width, height) < 0:
        fail("fixture rectangle must be nonnegative")
    if y + height > 480:
        fail("fixture must remain inside the known producer y range")

    expected = list(expected_addresses(x, y, width, height))
    if len({address for address, _ in expected}) != len(expected):
        fail("static expected address set overlaps within a fixture")
    if any(not (TEXTURE_BASE <= address < 0x00A6E6E0) for address, _ in expected):
        fail("fixture address escaped the reviewed y<512 1 MiB address band")

    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    mapped = map_original_sections(uc, pe)
    uc.mem_map(INPUT_BASE, 0x2000, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(RETURN_SENTINEL, PAGE, UC_PROT_READ | UC_PROT_EXEC)

    data_va, data_size = data_section(pe)
    before_data = bytes(uc.mem_read(data_va, data_size))
    before_entry = bytes(uc.mem_read(EXE_BASE + ENTRY_RVA, ENTRY_END_RVA - ENTRY_RVA))
    before_notify = bytes(
        uc.mem_read(EXE_BASE + NOTIFY_RVA, NOTIFY_END_RVA - NOTIFY_RVA)
    )
    old_callback = bytes(uc.mem_read(CALLBACK_CELL, 4))
    old_gate = bytes(uc.mem_read(NOTIFICATION_GATE, 4))

    rect_va = INPUT_BASE + 0x100
    source_va = INPUT_BASE + 0x1000
    source_size = width * height * 2
    if source_size >= 256:
        fail("test data must remain unique within one byte for stride diagnosis")
    source = bytes(range(1, source_size + 1))
    uc.mem_write(rect_va, struct.pack("<4h", x, y, width, height))
    if source:
        uc.mem_write(source_va, source)

    # A nonzero trap pointer forces original BD80 to read its DWORD gate. With
    # the gate zero, the callback must not execute; an attempted jump is unmapped.
    uc.mem_write(CALLBACK_CELL, struct.pack("<I", UNKNOWN_CALLBACK))
    uc.mem_write(NOTIFICATION_GATE, struct.pack("<I", 0))
    sp = STACK_BASE + STACK_SIZE - 0x100
    uc.mem_write(sp, struct.pack("<III", RETURN_SENTINEL, rect_va, source_va))
    uc.reg_write(UC_X86_REG_ESP, sp)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x202)  # 32-bit Windows entry: interrupts on, DF clear.
    uc.reg_write(UC_X86_REG_EIP, EXE_BASE + ENTRY_RVA)

    trace = []
    write_events = []
    stop = {"kind": None, "detail": None}
    data_end = data_va + data_size
    image_ranges = [
        (row["va"], row["va"] + row["size"], row["name"])
        for row in mapped
    ]
    text_start, text_end, _ = next(
        (start, end, name) for start, end, name in image_ranges if name == ".text"
    )
    scratch_ranges = [
        (INPUT_BASE, INPUT_BASE + 0x2000),
        (STACK_BASE, STACK_BASE + STACK_SIZE),
    ]

    def on_code(machine, address, size, _user):
        if address == RETURN_SENTINEL:
            stop["kind"] = "return_sentinel"
            stop["detail"] = hex(address)
            machine.emu_stop()
            return
        if not original_pc_address(address):
            stop["kind"] = "unexpected_code"
            stop["detail"] = hex(address)
            machine.emu_stop()
            return
        trace.append(address)

    def on_write(machine, access, address, size, value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP)
        if data_va <= address and address + size <= data_end:
            if text_start <= address and address + size <= text_end:
                stop["kind"] = "write_to_original_code"
                stop["detail"] = {"pc": hex(pc), "address": hex(address)}
                machine.emu_stop()
                return
            write_events.append(
                {
                    "pc": pc,
                    "address": address,
                    "size": size,
                    "value": value,
                }
            )
            return
        if any(start <= address and address + size <= end for start, end in scratch_ranges):
            return
        stop["kind"] = "unexpected_write"
        stop["detail"] = {"pc": hex(pc), "address": hex(address), "size": size}
        machine.emu_stop()

    def on_invalid(machine, access, address, size, value, _user):
        stop["kind"] = "invalid_memory"
        stop["detail"] = {
            "pc": hex(machine.reg_read(UC_X86_REG_EIP)),
            "access": access,
            "address": hex(address),
            "size": size,
        }
        machine.emu_stop()
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
    uc.hook_add(UC_HOOK_MEM_INVALID, on_invalid)
    try:
        uc.emu_start(EXE_BASE + ENTRY_RVA, 0, timeout=2_000_000, count=MAX_INSTRUCTIONS)
    except UcError as error:
        if stop["kind"] is None:
            stop["kind"] = "unicorn_error"
            stop["detail"] = str(error)

    if stop["kind"] != "return_sentinel":
        fail(f"{case['name']} did not return through the sentinel: {stop}")
    if trace[0] != EXE_BASE + ENTRY_RVA:
        fail(f"{case['name']} did not begin at the original A710 entry")
    if trace.count(EXE_BASE + NOTIFY_RVA) != 1:
        fail(f"{case['name']} did not call original BD80 exactly once")
    required_gate_path = [0xBD80, 0xBD85, 0xBD87, 0xBD89, 0xBD8F, 0xBD91, 0xBD95]
    for rva in required_gate_path:
        if EXE_BASE + rva not in trace:
            fail(f"{case['name']} missed original BD80 gate instruction {rva:#x}")
    if EXE_BASE + 0xBD93 in trace:
        fail(f"{case['name']} executed the disabled callback jump at BD93")

    observed = []
    for event in write_events:
        if event["pc"] not in COPY_PCS:
            fail(
                f"{case['name']} wrote output from unknown original PC "
                f"{event['pc'] - EXE_BASE:#x}"
            )
        raw = bytes(
            (event["value"] >> (byte_index * 8)) & 0xFF
            for byte_index in range(event["size"])
        )
        observed.extend(
            (event["address"] + byte_index, raw[byte_index], event["pc"])
            for byte_index in range(event["size"])
        )

    expected_values = [(address, source[offset]) for address, offset in expected]
    observed_values = [(address, value) for address, value, _pc in observed]
    if observed_values != expected_values:
        fail(
            f"{case['name']} original write stream differs from predicted address/value "
            f"stream: observed={observed_values!r} expected={expected_values!r}"
        )

    after_data = bytes(uc.mem_read(data_va, data_size))
    expected_diff = {
        address
        for address, offset in expected
        if before_data[address - data_va] != source[offset]
    }
    for cell, value in (
        (CALLBACK_CELL, struct.pack("<I", UNKNOWN_CALLBACK)),
        (NOTIFICATION_GATE, struct.pack("<I", 0)),
    ):
        expected_diff.update(
            cell + index
            for index, byte in enumerate(value)
            if before_data[cell + index - data_va] != byte
        )
    actual_diff = {
        data_va + index
        for index, (before, after) in enumerate(zip(before_data, after_data))
        if before != after
    }
    if actual_diff != expected_diff:
        unexpected = sorted(actual_diff - expected_diff)
        missing = sorted(expected_diff - actual_diff)
        fail(
            f"{case['name']} data diff escaped exact pixel/fixture cells: "
            f"unexpected={[hex(a) for a in unexpected[:16]]} "
            f"missing={[hex(a) for a in missing[:16]]}"
        )

    if bytes(uc.mem_read(EXE_BASE + ENTRY_RVA, len(before_entry))) != before_entry:
        fail(f"{case['name']} modified original A710 code bytes")
    if bytes(uc.mem_read(EXE_BASE + NOTIFY_RVA, len(before_notify))) != before_notify:
        fail(f"{case['name']} modified original BD80 code bytes")
    if struct.unpack("<I", bytes(uc.mem_read(NOTIFICATION_GATE, 4)))[0] != 0:
        fail(f"{case['name']} changed the disabled notification gate")

    rows = []
    for row_index in range(height):
        row_begin = row_index * width * 2
        row_end = row_begin + width * 2
        row_pc_bytes = Counter(
            hex(pc - EXE_BASE)
            for _address, _value, pc in observed[row_begin:row_end]
        )
        rows.append(
            {
                "input_y": y + row_index,
                "source_offset": row_begin,
                "source_bytes": width * 2,
                "observed_addresses": [
                    hex(address)
                    for address, _value, _pc in observed[row_begin:row_end]
                ],
                "copy_pc_byte_counts": dict(row_pc_bytes),
            }
        )

    write_pc_bytes = Counter()
    write_pc_events = Counter()
    for event in write_events:
        rva = event["pc"] - EXE_BASE
        write_pc_bytes[hex(rva)] += event["size"]
        write_pc_events[hex(rva)] += 1

    report = {
        "name": case["name"],
        "rect_signed_words": [x, y, width, height],
        "source_bytes": source_size,
        "rows": rows,
        "original_instruction_trace_count": len(trace),
        "bd80_gate_trace_rvas": [
            hex(address - EXE_BASE)
            for address in trace
            if EXE_BASE + NOTIFY_RVA <= address < EXE_BASE + NOTIFY_END_RVA
        ],
        "bd80_call_count": trace.count(EXE_BASE + NOTIFY_RVA),
        "bd80_zero_gate_tested": True,
        "notification_callback_jumped": False,
        "write_events_by_original_pc_rva": dict(write_pc_events),
        "write_bytes_by_original_pc_rva": dict(write_pc_bytes),
        "observed_output_byte_count": len(observed),
        "untouched_data_outside_output_and_fixture_cells": actual_diff == expected_diff,
        "code_bytes_unchanged": True,
        "returned_to_sentinel": True,
        "mapped_sections": mapped,
    }

    # Restore every changed .data byte, including the temporary notification
    # fixture cells, then prove this emulator instance is back to its pre-case
    # state before it is discarded.
    for address, _offset in expected:
        original = before_data[address - data_va]
        uc.mem_write(address, bytes([original]))
    uc.mem_write(CALLBACK_CELL, old_callback)
    uc.mem_write(NOTIFICATION_GATE, old_gate)
    if bytes(uc.mem_read(data_va, data_size)) != before_data:
        fail(f"{case['name']} failed to restore its private emulator .data state")
    report["private_data_state_restored"] = True
    return report


def main():
    pe = PE(ARGS.exe)
    if sha256(pe.data) != EXPECTED_EXE_SHA256:
        fail("pinned EXE SHA-256 changed")
    if pe.image_base != EXE_BASE:
        fail(f"unexpected original image base {pe.image_base:#x}")
    if getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        fail("unexpected base relocation table for this fixed-base fixture")

    results = [run_case(pe, case) for case in cases()]
    mapped_sections = results[0].pop("mapped_sections")
    for result in results[1:]:
        result.pop("mapped_sections")
    mapped_sections = [
        {
            "name": section["name"],
            "va": hex(section["va"]),
            "mapped_size": hex(section["size"]),
            "raw_size": hex(section.get("raw_size", 0)),
            "permissions": hex(section.get("permissions", 0)),
        }
        for section in mapped_sections
    ]
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_EQUIVALENT",
        "scope": "Finite nonnegative original A710 upload rectangles with y+height <= 480, using unchanged HERCULES.EXE A710 and BD80 instructions in Unicorn. This is not FUNCTION_MATCH, full A710-domain, image-storage extent, renderer, gameplay, or module acceptance.",
        "oracle": {
            "path": str(pe.path),
            "sha256": sha256(pe.data),
            "image_base": hex(pe.image_base),
            "entry_rva": hex(ENTRY_RVA),
            "entry_span_size": ENTRY_END_RVA - ENTRY_RVA,
            "entry_span_sha256": sha256(pe.read_rva(ENTRY_RVA, ENTRY_END_RVA - ENTRY_RVA)),
            "notification_rva": hex(NOTIFY_RVA),
            "notification_span_size": NOTIFY_END_RVA - NOTIFY_RVA,
            "notification_span_sha256": sha256(pe.read_rva(NOTIFY_RVA, NOTIFY_END_RVA - NOTIFY_RVA)),
            "base_relocations": 0,
        },
        "oracle_execution": {
            "engine": "Unicorn CPU emulator",
            "unicorn_version": __import__("unicorn").__version__,
            "architecture": "x86 32-bit",
            "original_executable_sections_mapped_at_image_base": True,
            "checked_address_equation": "addr=0x96e6e0 + row*0x100 + ((2*x&0x700)<<8) + (2*x&0xff); y<512 row=(y<256?0:0x800)+(y&0xff)",
            "instruction_budget_per_case": MAX_INSTRUCTIONS,
            "sentinel_return": hex(RETURN_SENTINEL),
            "no_original_code_bytes_written": True,
            "unknown_original_or_os_endpoints_fail": True,
            "notification_fixture": {
                "callback_cell_va": hex(CALLBACK_CELL),
                "callback_cell_value": hex(UNKNOWN_CALLBACK),
                "gate_va": hex(NOTIFICATION_GATE),
                "gate_value": 0,
                "gate_path_exercised": True,
                "callback_endpoint_executed": False,
            },
            "crt_memcpy_call": False,
            "copy_mechanism": "original REP MOVSD/REP MOVSB and original byte stores",
        },
        "mapped_original_sections": mapped_sections,
        "cases": results,
        "limitations": [
            "Only finite synthetic inputs were executed; the complete valid callback domain and ownership extent are unproved.",
            "Only the known y<512 producer band was used; y>=512/index-derived writes were deliberately excluded.",
            "No CRT memcpy call exists in the measured A710 span; native string instructions and byte stores perform the upload.",
            "No GPU, DirectDraw, rasterization, packet dispatch, gameplay, source identity, FUNCTION_MATCH, or whole-module result is claimed.",
        ],
    }
    receipt["oracle_execution"].update(verify_runtime(unicorn, ARGS.deps))
    receipt["invocation"] = {
        "exe": str(ARGS.exe),
        "original_root": str(ARGS.original_root),
        "unicorn_deps": str(ARGS.deps),
        "analysis_deps": str(ARGS.analysis_deps),
    }
    write_receipt(ARGS.receipt, receipt)
    print(
        json.dumps(
            {
                "status": receipt["status"],
                "original_exe_sha256": receipt["oracle"]["sha256"],
                "unicorn_version": receipt["oracle_execution"]["unicorn_version"],
                "cases": [
                    {
                        "name": case["name"],
                        "rectangle": case["rect_signed_words"],
                        "output_bytes": case["observed_output_byte_count"],
                        "copy_pcs": case["write_bytes_by_original_pc_rva"],
                        "bd80_trace": case["bd80_gate_trace_rvas"],
                        "untouched_data": case[
                            "untouched_data_outside_output_and_fixture_cells"
                        ],
                    }
                    for case in results
                ],
                "receipt": str(ARGS.receipt),
            },
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
