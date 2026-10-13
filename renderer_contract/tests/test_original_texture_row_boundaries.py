#!/usr/bin/env python3
"""Finite unchanged-instruction row-boundary probes for HERCULES A710/A8F0."""
from __future__ import annotations

import hashlib
import json
import struct
import sys
from pathlib import Path


def find_repo_root(start: Path) -> Path:
    for candidate in (start, *start.parents):
        if (candidate / "renderer_contract").is_dir() and (candidate / "scripts" / "pe.py").is_file():
            return candidate
    raise SystemExit(f"cannot find repository root from {start}")


REPO_ROOT = find_repo_root(Path(__file__).resolve().parent)
sys.path.insert(0, str(REPO_ROOT / "renderer_contract" / "tests"))
from oracle_common import parse_args, verify_runtime, write_receipt, sha256  # noqa: E402

if not any(a == "--receipt" or a.startswith("--receipt=") for a in sys.argv[1:]):
    raise SystemExit("--receipt is required; choose a path under this repository's work/")
ARGS = parse_args("Run finite original A710/A8F0 row-boundary probes", "texture_row_boundaries.json")
if ARGS.original_root.resolve() != REPO_ROOT.resolve():
    raise SystemExit(f"--original-root must match detected repository root {REPO_ROOT}")
if not (REPO_ROOT / "renderer_contract" / "hercules_textures.h").is_file():
    raise SystemExit("renderer_contract/hercules_textures.h is missing")
if not any(a == "--original-root" or a.startswith("--original-root=") for a in sys.argv[1:]):
    raise SystemExit("--original-root is required for reproducibility")
try:
    ARGS.receipt.resolve().relative_to((REPO_ROOT / "work").resolve())
except ValueError as error:
    raise SystemExit(f"--receipt must remain under {REPO_ROOT / 'work'}") from error

sys.path.insert(0, str(REPO_ROOT / "scripts"))
from pe import PE  # noqa: E402
import unicorn  # noqa: E402
from unicorn import (  # noqa: E402
    Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_INVALID,
    UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MODE_32,
    UC_PROT_EXEC, UC_PROT_READ, UC_PROT_WRITE,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_ESP,
)

EXE_BASE = 0x00400000
A710_RVA, A710_END = 0xA710, 0xA8E5
A8F0_RVA, A8F0_END = 0xA8F0, 0xABF3
BD80_RVA, BD80_END = 0xBD80, 0xBD96
EXPECTED_HASHES = {
    "oracle_common.py": "a024048d9acc6e18197e10a82427cd84351a90666e1626e610bf60c6105f1357",
    "pe.py": "8da71eb87b5a88dd1d80a3fe180e2b94557e2c3017fb4e5c37c4a8c630af62ca",
    "hercules_textures.h": "3fdd37291af16f6328a42b545d5655058f1d7d510d796ffdcbe562596e8369ad",
    "hercules_textures.c": "07c951a693b8aeab12cff7cf6b09dc802de7c203e0c8387bd4d2bdf4ce23a5c5",
}
EXPECTED_SPAN_HASHES = {
    (A710_RVA, A710_END): "30bd84ea8724152e3bfb0e2c521650399913bcec115f1aef489768085981d0fe",
    (A8F0_RVA, A8F0_END): "bc04260d2227d221b124bb397b3d7f1138eed53081654722e3f1acd52599196e",
    (BD80_RVA, BD80_END): "6a9578c5be0c980bc35b29642d3ac389e829a0c5666022353cb88c04f19db64f",
}
STATIC_SPANS = (
    ("A710", 0xA71A, 0xA726, "entry y<512 signed branch"),
    ("A710", 0xA740, 0xA759, "high-band row expression, recomputed at loop head"),
    ("A710", 0xA816, 0xA83A, "low-band row expression, recomputed at loop head"),
    ("A710", 0xA7DC, 0xA7F3, "high-band y increment and row-loop back edge"),
    ("A710", 0xA8C0, 0xA8D7, "low-band y increment and row-loop back edge"),
    ("A8F0", 0xA8F7, 0xA906, "entry source-y<512 signed branch"),
    ("A8F0", 0xA8F0, 0xA921, "cdecl frame, full DWORD destination-y argument slot, entry band and row-loop setup"),
    ("A8F0", 0xA921, 0xA956, "high-band source/destination row expressions"),
    ("A8F0", 0xA92C, 0xA930, "high-entry full DWORD destination-y load"),
    ("A8F0", 0xA962, 0xA966, "high-entry full DWORD destination-x load"),
    ("A8F0", 0xAA3B, 0xAA5B, "high-band source/destination increments and loop back edge"),
    ("A8F0", 0xAA85, 0xAAD7, "low-band source/destination row expressions"),
    ("A8F0", 0xAAA5, 0xAAA9, "low-entry full DWORD destination-y load"),
    ("A8F0", 0xAAE8, 0xAAEC, "low-entry full DWORD destination-x load"),
    ("A8F0", 0xABC0, 0xABE0, "low-band source/destination increments and loop back edge"),
    ("A8F0", 0xABCC, 0xABE4, "DWORD destination-y counter load, 32-bit INC, store, and signed row-loop branch"),
)
TEXTURE_BASE = 0x0096E6E0
CALLBACK_CELL, GATE_CELL = 0x00A8E780, 0x00A8E788
INPUT_BASE, INPUT_SIZE = 0x10000000, 0x10000
STACK_BASE, STACK_SIZE = 0x20000000, 0x10000
RETURN_SENTINEL, UNKNOWN_CALLBACK = 0x30000000, 0x31000000
MAX_INSTRUCTIONS, TIMEOUT_US = 100_000, 2_000_000
EMBEDDED_PREDICTION = {
    "prediction": "Each callback latches its low/high row mapping from signed entry y and recomputes that expression for each row. A8F0's source y selects the mapping for both source and destination; its destination x/y arguments are loaded as DWORDs, and the full destination-y value participates in the low-entry row comparison.",
    "falsifier": "A native read/write address disagrees with the entry-selected expression, destination y=65536 behaves as low-WORD zero, or static instructions show WORD loads/cached row pointers/narrower destination counters.",
    "scope": "Finite original A710/A8F0 cases with small positive heights, including A8F0 destination-y 65536, x=65536 masked-address, and 32767-to-32768 counter discriminators. Original BD80 gate only; no game, OS/window, DirectDraw, resource data, overlap, negative dimensions, or canonical edits.",
    "oracle_sha256": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
}


def align_up(value: int, alignment: int = 0x1000) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def section_permissions(characteristics: int) -> int:
    result = 0
    if characteristics & 0x40000000:
        result |= UC_PROT_READ
    if characteristics & 0x80000000:
        result |= UC_PROT_WRITE
    if characteristics & 0x20000000:
        result |= UC_PROT_EXEC
    return result


def map_sections(uc, pe):
    size = align_up(pe.pe.OPTIONAL_HEADER.SizeOfHeaders)
    uc.mem_map(EXE_BASE, size, UC_PROT_READ)
    uc.mem_write(EXE_BASE, pe.data[:pe.pe.OPTIONAL_HEADER.SizeOfHeaders])
    mapped = [{"name": "headers", "va": EXE_BASE, "size": size}]
    for section in pe.sections:
        raw = section["raw_size"]
        span = align_up(max(raw, section["virtual_size"]))
        va = EXE_BASE + section["virtual_address"]
        uc.mem_map(va, span, section_permissions(section["characteristics"]))
        if raw:
            uc.mem_write(va, pe.read_rva(section["virtual_address"], raw))
        if span > raw:
            uc.mem_write(va + raw, b"\0" * (span - raw))
        mapped.append({"name": section["name"], "va": va, "size": span})
    return mapped


def section_span(pe, name):
    item = next(s for s in pe.sections if s["name"] == name)
    return EXE_BASE + item["virtual_address"], align_up(max(item["raw_size"], item["virtual_size"]))


def normal_row(y: int) -> int:
    if not 0 <= y < 512:
        raise ValueError("normal helper only supports y<512")
    return y if y < 256 else 0x800 + (y & 0xff)


def high_row(y: int) -> int:
    if not 512 <= y <= 1023:
        raise ValueError("high-y helper only supports 512<=y<=1023")
    return 0x1000 + (y & 0x1ff)


def entry_band_row(start_y: int, current_y: int) -> int:
    """Observed candidate expression chosen by the entry y<512 branch."""
    if start_y < 512:
        return (0 if current_y < 256 else 0x800) + (current_y & 0xff)
    return 0x1000 + (current_y & 0x1ff)


def byte_address(x_word: int, row: int, byte_i: int) -> int:
    bx = 2 * x_word + byte_i
    return TEXTURE_BASE + (row << 8) + ((bx & 0x700) << 8) + (bx & 0xff)


def helper_addresses(x: int, y: int, width: int, height: int):
    result = []
    for dy in range(height):
        yy = y + dy
        if yy < 512:
            row = normal_row(yy)
        elif yy <= 1023:
            row = high_row(yy)
        else:
            return None
        for b in range(width * 2):
            result.append(byte_address(x, row, b))
    return result


def helper_row_details(x: int, y: int, width: int, height: int):
    """Keep standalone-helper support explicit for each logical row."""
    details = []
    for dy in range(height):
        yy = y + dy
        if yy < 512:
            row = normal_row(yy)
        elif yy <= 1023:
            row = high_row(yy)
        else:
            details.append({"logical_y": yy, "supported": False, "addresses": None})
            continue
        details.append({"logical_y": yy, "supported": True,
                        "addresses": [hex(byte_address(x, row, b))
                                      for b in range(width * 2)]})
    return details


def compare_helper_rows(details, observed_addresses, width):
    bytes_per_row = width * 2
    comparisons = []
    for index, detail in enumerate(details):
        observed = observed_addresses[index * bytes_per_row:(index + 1) * bytes_per_row]
        expected = detail["addresses"]
        comparisons.append({
            "logical_y": detail["logical_y"],
            "helper_supported": detail["supported"],
            "expected_addresses": expected,
            "observed_addresses": [hex(a) for a in observed],
            "matches": observed == [int(a, 16) for a in expected] if expected else None,
        })
    return comparisons


def band_addresses(x: int, entry_y: int, first_y: int, width: int, height: int):
    return [byte_address(x, entry_band_row(entry_y, first_y + dy), b)
            for dy in range(height) for b in range(width * 2)]


def linear_band_addresses(x: int, entry_y: int, first_y: int, width: int, height: int):
    first = entry_band_row(entry_y, first_y)
    first_row = [byte_address(x, first, b) for b in range(width * 2)]
    return [address + (dy * 0x100) for dy in range(height) for address in first_row]


def seed_byte(address: int, salt: int) -> int:
    return (address * 29 ^ (address >> 8) ^ (address >> 17) ^ salt * 61) & 0xff


def row_case(name, entry, y, *, slow=False):
    if entry == "A710":
        x, width = (127, 2) if slow else (5, 1)
        return {"name": name, "entry": entry, "rect": (x, y, width, 2),
                "source_xy": None, "destination_xy": None, "slow": slow}
    sx, dx, width = (127, 255, 2) if slow else (5, 20, 1)
    return {"name": name, "entry": entry, "rect": (sx, y, width, 2),
            "source_xy": (sx, y), "destination_xy": (dx, y), "slow": slow}


def a8f0_cross_band_case(name, source_y, destination_y):
    return {"name": name, "entry": "A8F0", "rect": (5, source_y, 1, 2),
            "source_xy": (5, source_y), "destination_xy": (20, destination_y),
            "slow": False}


def a8f0_argument_width_case(name, destination_x, destination_y):
    return {"name": name, "entry": "A8F0", "rect": (5, 0, 1, 1),
            "source_xy": (5, 0), "destination_xy": (destination_x, destination_y),
            "slow": False}


def probe_case(pe, case, case_index):
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    mapped = map_sections(uc, pe)
    uc.mem_map(INPUT_BASE, INPUT_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(RETURN_SENTINEL, 0x1000, UC_PROT_READ | UC_PROT_EXEC)

    data_va, data_size = section_span(pe, ".data")
    text_va, text_size = section_span(pe, ".text")
    before_data = bytes(uc.mem_read(data_va, data_size))
    before_text = bytes(uc.mem_read(text_va, text_size))
    entry_rva = A710_RVA if case["entry"] == "A710" else A8F0_RVA
    entry_end = A710_END if case["entry"] == "A710" else A8F0_END
    before_entry = bytes(uc.mem_read(EXE_BASE + entry_rva, entry_end - entry_rva))
    before_bd80 = bytes(uc.mem_read(EXE_BASE + BD80_RVA, BD80_END - BD80_RVA))
    x, y, width, height = case["rect"]

    dest_x = x if case["entry"] == "A710" else case["destination_xy"][0]
    dest_y = y if case["entry"] == "A710" else case["destination_xy"][1]
    entry_y = y
    helper_dest = helper_addresses(dest_x, dest_y, width, height)
    helper_dest_rows = helper_row_details(dest_x, dest_y, width, height)
    linear_dest = linear_band_addresses(dest_x, entry_y, dest_y, width, height)
    band_dest = band_addresses(dest_x, entry_y, dest_y, width, height)
    seed_addresses = set(helper_dest or ()) | set(linear_dest or ()) | set(band_dest)
    if case["entry"] == "A8F0":
        sx, sy = case["source_xy"]
        source_helper = helper_addresses(sx, sy, width, height)
        source_helper_rows = helper_row_details(sx, sy, width, height)
        source_band = band_addresses(sx, entry_y, sy, width, height)
        source_linear = linear_band_addresses(sx, entry_y, sy, width, height)
        seed_addresses |= set(source_helper or ()) | set(source_linear or ()) | set(source_band)
        # Candidate expressions for the first row at y=1023 and the y=1024
        # bank-wrap result. They are witnesses only; the helper rejects y=1024.
        for xx in (sx, case["destination_xy"][0]):
            if y == 1023:
                for row in (0x1000, 0x1200, 0x2000):
                    for b in range(width * 2):
                        seed_addresses.add(byte_address(xx, row, b))
    else:
        source_va = INPUT_BASE + 0x2000
        source = bytes(((i * 41) + 0x37 + case_index) & 0xff for i in range(width * height * 2))
        uc.mem_write(source_va, source)

    salt = case_index + 11
    for address in sorted(seed_addresses):
        uc.mem_write(address, bytes([seed_byte(address, salt)]))
    uc.mem_write(CALLBACK_CELL, struct.pack("<I", UNKNOWN_CALLBACK))
    uc.mem_write(GATE_CELL, struct.pack("<I", 0))
    fixture_data = bytes(uc.mem_read(data_va, data_size))

    rect_va = INPUT_BASE + 0x100
    if case["entry"] == "A710":
        uc.mem_write(rect_va, struct.pack("<4h", x, y, width, height))
        args = (rect_va, source_va)
        expected_source = source
    else:
        sx, sy = case["source_xy"]
        dx, dy = case["destination_xy"]
        uc.mem_write(rect_va, struct.pack("<4h", sx, sy, width, height))
        args = (rect_va, dx, dy)
        expected_source = None

    sp = STACK_BASE + STACK_SIZE - 0x100
    uc.mem_write(sp, struct.pack("<I", RETURN_SENTINEL))
    for i, arg in enumerate(args):
        uc.mem_write(sp + 4 * (i + 1), struct.pack("<I", arg & 0xffffffff))
    uc.reg_write(UC_X86_REG_ESP, sp)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
    uc.reg_write(UC_X86_REG_EIP, EXE_BASE + entry_rva)

    code_ranges = ((EXE_BASE + entry_rva, EXE_BASE + entry_end),
                   (EXE_BASE + BD80_RVA, EXE_BASE + BD80_END))
    scratch_ranges = ((INPUT_BASE, INPUT_BASE + INPUT_SIZE),
                      (STACK_BASE, STACK_BASE + STACK_SIZE))
    trace, reads, writes = [], [], []
    stop = {"value": None, "steps": 0}
    source_limit = TEXTURE_BASE + 0x400000

    def on_code(machine, address, _size, _user):
        stop["steps"] += 1
        if address == RETURN_SENTINEL:
            stop["value"] = "return_sentinel"
            machine.emu_stop()
            return
        if not any(lo <= address < hi for lo, hi in code_ranges):
            stop["value"] = {"unexpected_code": hex(address)}
            machine.emu_stop()
            return
        trace.append(address - EXE_BASE)
        if stop["steps"] > MAX_INSTRUCTIONS:
            stop["value"] = "instruction_budget"
            machine.emu_stop()

    def on_read(machine, _access, address, size, _value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP) - EXE_BASE
        if case["entry"] == "A710":
            if source_va <= address and address + size <= source_va + len(expected_source):
                reads.append({"pc_rva": hex(pc), "address": hex(address), "size": size,
                              "bytes": bytes(machine.mem_read(address, size)).hex()})
        elif entry_rva <= pc < entry_end and TEXTURE_BASE <= address < source_limit:
            reads.append({"pc_rva": hex(pc), "address": hex(address), "size": size,
                          "bytes": bytes(machine.mem_read(address, size)).hex()})

    def on_write(machine, _access, address, size, value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP) - EXE_BASE
        if data_va <= address and address + size <= data_va + data_size:
            raw = (value & ((1 << (size * 8)) - 1)).to_bytes(size, "little")
            writes.append({"pc_rva": hex(pc), "address": hex(address), "size": size,
                           "bytes": raw.hex()})
            return
        if any(lo <= address and address + size <= hi for lo, hi in scratch_ranges):
            return
        stop["value"] = {"unexpected_write": {"pc_rva": hex(pc),
                            "address": hex(address), "size": size}}
        machine.emu_stop()

    def on_invalid(machine, access, address, size, _value, _user):
        stop["value"] = {"invalid_memory": {"pc_rva": hex(machine.reg_read(UC_X86_REG_EIP) - EXE_BASE),
                            "access": access, "address": hex(address), "size": size}}
        machine.emu_stop()
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_READ, on_read)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
    uc.hook_add(UC_HOOK_MEM_INVALID, on_invalid)
    try:
        uc.emu_start(EXE_BASE + entry_rva, 0, timeout=TIMEOUT_US, count=MAX_INSTRUCTIONS)
    except UcError as error:
        if stop["value"] is None:
            stop["value"] = {"unicorn_error": str(error)}
    if stop["value"] != "return_sentinel":
        raise AssertionError(f"{case['name']} stopped unexpectedly: {stop['value']}")
    if trace[0] != entry_rva or trace.count(BD80_RVA) != 1:
        raise AssertionError(f"{case['name']} did not execute its original entry and BD80 once")
    gate_path = {0xBD80, 0xBD85, 0xBD87, 0xBD89, 0xBD8F, 0xBD91, 0xBD95}
    if not gate_path.issubset(set(trace)) or 0xBD93 in trace:
        raise AssertionError(f"{case['name']} missed the disabled original BD80 return gate")

    # Flatten observed output writes and source reads into byte-address streams.
    output_bytes = []
    for event in writes:
        value = bytes.fromhex(event["bytes"])
        start = int(event["address"], 16)
        output_bytes.extend((start + i, value[i]) for i in range(len(value)))
    source_bytes = []
    for event in reads:
        value = bytes.fromhex(event["bytes"])
        start = int(event["address"], 16)
        source_bytes.extend((start + i, value[i]) for i in range(len(value)))

    if case["entry"] == "A710":
        if len(source_bytes) != len(expected_source) or bytes(v for _a, v in source_bytes) != expected_source:
            raise AssertionError(f"{case['name']} A710 borrowed-source read stream mismatch: {reads}")
    else:
        if len(source_bytes) != width * height * 2:
            raise AssertionError(f"{case['name']} A8F0 did not read exactly one source rectangle: {reads}")
        if bytes(v for _a, v in output_bytes) != bytes(v for _a, v in source_bytes):
            raise AssertionError(f"{case['name']} A8F0 output values differ from the observed source-read stream")
    if len(output_bytes) != width * height * 2:
        raise AssertionError(f"{case['name']} wrote unexpected byte count {len(output_bytes)}")

    actual_dest = [a for a, _v in output_bytes]
    helper_dest = helper_addresses(dest_x, dest_y, width, height)
    linear_dest = linear_band_addresses(dest_x, entry_y, dest_y, width, height)
    band_dest = band_addresses(dest_x, entry_y, dest_y, width, height)
    observed_source = [a for a, _v in source_bytes]
    if case["entry"] == "A8F0":
        sx, sy = case["source_xy"]
        helper_source = helper_addresses(sx, sy, width, height)
        linear_source = linear_band_addresses(sx, entry_y, sy, width, height)
        band_source = band_addresses(sx, entry_y, sy, width, height)
    else:
        helper_source = linear_source = band_source = None
        source_helper_rows = None

    if actual_dest != band_dest:
        raise AssertionError(f"{case['name']} destination byte addresses violate entry-band-per-row model")
    if case["entry"] == "A8F0" and observed_source != source_band:
        raise AssertionError(f"{case['name']} source byte addresses violate entry-band-per-row model")
    destination_row_comparisons = compare_helper_rows(helper_dest_rows, actual_dest, width)
    source_row_comparisons = (compare_helper_rows(source_helper_rows, observed_source, width)
                              if source_helper_rows is not None else None)

    after_data = bytes(uc.mem_read(data_va, data_size))
    expected_after = bytearray(fixture_data)
    for address, value in output_bytes:
        expected_after[address - data_va] = value
    if after_data != bytes(expected_after):
        raise AssertionError(f"{case['name']} changed .data beyond observed output writes")
    if bytes(uc.mem_read(text_va, text_size)) != before_text:
        raise AssertionError(f"{case['name']} changed original .text")
    if bytes(uc.mem_read(EXE_BASE + entry_rva, entry_end - entry_rva)) != before_entry:
        raise AssertionError(f"{case['name']} changed original entry code")
    if bytes(uc.mem_read(EXE_BASE + BD80_RVA, BD80_END - BD80_RVA)) != before_bd80:
        raise AssertionError(f"{case['name']} changed original BD80 code")
    if struct.unpack("<I", bytes(uc.mem_read(GATE_CELL, 4)))[0] != 0:
        raise AssertionError(f"{case['name']} changed the disabled BD80 gate")
    if struct.unpack("<I", bytes(uc.mem_read(CALLBACK_CELL, 4)))[0] != UNKNOWN_CALLBACK:
        raise AssertionError(f"{case['name']} changed the disabled BD80 callback endpoint")
    uc.mem_write(data_va, before_data)
    if bytes(uc.mem_read(data_va, data_size)) != before_data:
        raise AssertionError(f"{case['name']} did not restore the complete emulated .data image")

    return {
        "name": case["name"], "entry": case["entry"], "entry_rva": hex(entry_rva),
        "descriptor_words": list(case["rect"]),
        "source_xy": list(case["source_xy"]) if case["source_xy"] else None,
        "destination_xy": list(case["destination_xy"]) if case["destination_xy"] else None,
        "page_cross_slow_path_requested": case["slow"],
        "row_entry_band": "low y<512" if y < 512 else "high y>=512",
        "source_read_events": reads,
        "destination_write_events": writes,
        "observed_source_byte_addresses": [hex(a) for a in observed_source],
        "observed_destination_byte_addresses": [hex(a) for a in actual_dest],
        "helper_per_row_destination_addresses": [hex(a) for a in helper_dest] if helper_dest else None,
        "helper_per_row_destination_details": helper_dest_rows,
        "helper_destination_row_comparisons": destination_row_comparisons,
        "entry_band_per_row_destination_addresses": [hex(a) for a in band_dest],
        "start_row_plus_0x100_destination_addresses": [hex(a) for a in linear_dest] if linear_dest else None,
        "matches_helper_per_row_destination": actual_dest == helper_dest if helper_dest else None,
        "matches_entry_band_per_row_destination": actual_dest == band_dest,
        "matches_cached_linear_destination": actual_dest == linear_dest if linear_dest else None,
        "source_address_models": {
            "helper_per_row": [hex(a) for a in helper_source] if helper_source else None,
            "helper_per_row_details": source_helper_rows,
            "helper_row_comparisons": source_row_comparisons,
            "entry_band_per_row": [hex(a) for a in band_source] if band_source else None,
            "start_row_plus_0x100": [hex(a) for a in linear_source] if linear_source else None,
            "matches_helper": observed_source == helper_source if helper_source else None,
            "matches_entry_band": observed_source == band_source if band_source else None,
            "matches_cached_linear": observed_source == linear_source if linear_source else None,
        } if case["entry"] == "A8F0" else None,
        "source_to_destination_values_match": bytes(v for _a, v in source_bytes) == bytes(v for _a, v in output_bytes),
        "bd80_call_count": 1, "bd80_disabled_gate_path_returned": True,
        "bd80_endpoint_unchanged": True,
        "original_code_unchanged": True, "only_observed_output_changed_data": True,
        "complete_data_restored": True, "instruction_count": len(trace),
    }


def main():
    pe = PE(ARGS.exe)
    if sha256(pe.data) != "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d":
        raise SystemExit("pinned original HERCULES.EXE identity mismatch")
    if pe.image_base != EXE_BASE or getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        raise SystemExit("unexpected image base or relocation table")
    input_paths = {
        "oracle_common.py": REPO_ROOT / "renderer_contract" / "tests" / "oracle_common.py",
        "pe.py": REPO_ROOT / "scripts" / "pe.py",
        "hercules_textures.h": REPO_ROOT / "renderer_contract" / "hercules_textures.h",
        "hercules_textures.c": REPO_ROOT / "renderer_contract" / "hercules_textures.c",
    }
    for label, path in input_paths.items():
        actual = sha256(path.read_bytes())
        if actual != EXPECTED_HASHES[label]:
            raise SystemExit(f"pinned public input changed: {label}: {actual}")
    for bounds, expected in EXPECTED_SPAN_HASHES.items():
        actual = sha256(pe.read_rva(bounds[0], bounds[1] - bounds[0]))
        if actual != expected:
            raise SystemExit(f"pinned native function span changed at RVA {bounds[0]:#x}: {actual}")
    cases = []
    for y, label in ((255, "255_256"), (511, "511_512"), (1023, "1023_1024")):
        cases.append(row_case(f"a710_y{label}_fast", "A710", y))
        cases.append(row_case(f"a8f0_y{label}_fast", "A8F0", y))
    cases.append(row_case("a710_y511_512_horizontal_page_cross", "A710", 511, slow=True))
    cases.append(row_case("a8f0_y511_512_horizontal_page_cross", "A8F0", 511, slow=True))
    cases.append(a8f0_cross_band_case("a8f0_source511_destination512_low_entry_band", 511, 512))
    cases.append(a8f0_cross_band_case("a8f0_source512_destination511_high_entry_band", 512, 511))
    cases.append(a8f0_cross_band_case("a8f0_destination_i32_counter_32767_to_32768_low_entry", 0, 32767))
    cases.append(a8f0_argument_width_case("a8f0_destination_y_dword_65536_low_entry", 20, 65536))
    cases.append(a8f0_argument_width_case("a8f0_destination_x_dword_65536_masked_address", 65536, 0))
    results = [probe_case(pe, case, index + 1) for index, case in enumerate(cases)]
    original_spans = []
    for name, (lo, hi) in (("A710 upload callback", (A710_RVA, A710_END)),
                           ("A8F0 copy callback", (A8F0_RVA, A8F0_END)),
                           ("BD80 notification gate", (BD80_RVA, BD80_END))):
        original_spans.append({"module": "HERCULES.EXE", "rva": hex(lo), "size": hi - lo,
                               "sha256": sha256(pe.read_rva(lo, hi - lo)), "label": name})
    static_spans = []
    for entry, lo, hi, label in STATIC_SPANS:
        static_spans.append({"module": "HERCULES.EXE", "rva": hex(lo), "size": hi - lo,
                             "sha256": sha256(pe.read_rva(lo, hi - lo)),
                             "label": label, "function": entry})
    receipt = {
        "schema_version": 1,
        "status": "FINITE_ORIGINAL_EXECUTION",
        "scope": "Thirteen finite A710/A8F0 cases with source/destination separated, including row boundaries, page crosses, both A8F0 entry-band directions, the 32767-to-32768 counter, and single-row full-DWORD destination x/y arguments. Original code only; no assets or original OS/window execution.",
        "runner": {"path": str(Path(__file__).resolve()),
                   "sha256": sha256(Path(__file__).resolve().read_bytes())},
        "public_inputs": [{"path": str(path.relative_to(REPO_ROOT)).replace("\\", "/"),
                           "sha256": sha256(path.read_bytes())}
                          for path in input_paths.values()],
        "oracle": {"exe_sha256": sha256(pe.data), "image_base": hex(pe.image_base),
                   "a710": {"rva": hex(A710_RVA), "end_exclusive": hex(A710_END),
                            "sha256": sha256(pe.read_rva(A710_RVA, A710_END - A710_RVA))},
                   "a8f0": {"rva": hex(A8F0_RVA), "end_exclusive": hex(A8F0_END),
                            "sha256": sha256(pe.read_rva(A8F0_RVA, A8F0_END - A8F0_RVA))},
                   "bd80": {"rva": hex(BD80_RVA), "end_exclusive": hex(BD80_END),
                            "sha256": sha256(pe.read_rva(BD80_RVA, BD80_END - BD80_RVA))}},
        "original_spans": original_spans,
        "static_row_control_spans": static_spans,
        "prediction": EMBEDDED_PREDICTION,
        "native_execution": {"engine": "Unicorn CPU emulator", "unicorn_version": unicorn.__version__,
                              "architecture": "x86 32-bit", "original_sections_mapped_at_image_base": True,
                              "instruction_budget_per_case": MAX_INSTRUCTIONS,
                              "bd80": "original disabled gate pointer/gate path; callback endpoint not executed",
                              "memory_trace": "all A710 external source reads, A8F0 texture-range reads, and all .data writes recorded by address/size/PC",
                              "code_unchanged": True, "full_mapped_data_restored_after_each_case": True,
                              **verify_runtime(unicorn, ARGS.deps)},
        "row_models": {
            "public_normal_helper": "row=y for y<256; row=0x800+(y&0xff) for 256<=y<512; this is the standalone helper's per-row domain",
            "public_high_helper": "row=0x1000+(y&0x1ff) for 512<=y<=1023; helper rejects other y; this is the standalone helper's per-row domain",
            "observed_entry_band_contract": "Callback chooses low or high code once from the initial signed y WORD (A710 destination RECT y; A8F0 source RECT y). The chosen code then recomputes source/destination row expressions each iteration using the current y values. A8F0 applies the source-entry band branch to both source and destination row addressing.",
            "low_entry_row_expression": "(current_y<256 ? 0 : 0x800) + (current_y&0xff)",
            "high_entry_row_expression": "0x1000 + (current_y&0x1ff)",
            "cached_pointer_model": "Rejected by finite boundary witnesses: both callbacks recompute row expressions; +0x100 continuation is not a general contract.",
            "address": "0x96e6e0 + row*0x100 + (((2*x+byte_i)&0x700)<<8) + ((2*x+byte_i)&0xff)",
            "helper_mismatch": "Standalone per-logical-row helpers disagree when a rectangle crosses a callback's entry-band branch or the high helper's y<=1023 limit. The native fixtures do not broaden or modify either public helper.",
            "higher_domain": "No helper contract is broadened; y=1024 is reported only as the second row of the y=1023 native fixture."
        },
        "static_height_observation": {
            "A710": "At RVA 0xA71F, CMP CX,0x200 followed by signed JL at 0xA725 selects the entry band. Height is MOVSX at 0xA72E (high) or 0xA801 (low), width at 0xA740/0xA834; y+height is compared with signed JGE/JL loop control. Zero height still reaches BD80 in prior finite fixtures. No general invalid-input behavior claimed.",
            "A8F0": "At RVA 0xA8FE, CMP CX,0x200 followed by signed JL at 0xA904 selects the entry band from source y. Height is MOVSX at 0xA90D (high) or 0xAA6E (low), width at 0xA925/0xAAB4. Destination x/y are read as DWORD stack values at [esp+0x28]/[esp+0x2c]; the low-entry y=65536 fixture distinguishes the full value from low-WORD truncation. Destination y is incremented with 32-bit INC EBX at 0xABD3; the 32767-to-32768 fixture observes the continuation. No general invalid-input behavior claimed."
        },
        "static_destination_counter_observation": {
            "module": "HERCULES.EXE",
            "function_rva": "0xa8f0",
            "frame_span": next(s for s in static_spans if s["rva"] == "0xa8f0" and s["size"] == 49),
            "counter_span": next(s for s in static_spans if s["rva"] == "0xabcc"),
            "cdecl_stack_map": "At entry, return/[esp+0], RECT/[esp+4], destination_x/[esp+8], destination_y/[esp+0xc]. SUB ESP,0x10 and four 32-bit register pushes move ESP down 0x20, so the original destination_y argument is current [esp+0x2c].",
            "decoded_counter_sites": [
                {"rva": "0xabcc", "instruction": "mov ebx, dword ptr [esp+0x2c]"},
                {"rva": "0xabd3", "instruction": "inc ebx"},
                {"rva": "0xabda", "instruction": "mov dword ptr [esp+0x2c], ebx"},
                {"rva": "0xabde", "instruction": "jl 0x40aa85"}
            ],
            "bounded_discriminator": "source RECT y=0, destination y=32767, width=1, height=2; native rows write with low-entry rows 0x8ff then 0x800, not the i16-narrowed second-row result."
        },
        "static_destination_argument_width_observation": {
            "module": "HERCULES.EXE",
            "function_rva": "0xa8f0",
            "frame_span": next(s for s in static_spans if s["rva"] == "0xa8f0" and s["size"] == 49),
            "cdecl_stack_map": "At entry, destination_x is DWORD [esp+0x8] and destination_y is DWORD [esp+0xc]. SUB ESP,0x10 and four 32-bit pushes move them to current [esp+0x28] and [esp+0x2c]. Source RECT x/y remain signed WORD fields.",
            "destination_y_loads": [
                {"rva": "0xa92c", "instruction": "mov edi, dword ptr [esp+0x2c]", "span": next(s for s in static_spans if s["rva"] == "0xa92c")},
                {"rva": "0xaaa5", "instruction": "mov ecx, dword ptr [esp+0x2c]", "span": next(s for s in static_spans if s["rva"] == "0xaaa5")}
            ],
            "destination_x_loads": [
                {"rva": "0xa962", "instruction": "mov esi, dword ptr [esp+0x28]", "span": next(s for s in static_spans if s["rva"] == "0xa962")},
                {"rva": "0xaae8", "instruction": "mov edx, dword ptr [esp+0x28]", "span": next(s for s in static_spans if s["rva"] == "0xaae8")}
            ],
            "native_discriminators": {
                "destination_y_65536": "With source RECT y=0 and destination y=65536, width=1, height=1, native output is 0x9ee708/09 from low-entry row 0x800; WORD truncation to zero would write 0x96e708/09.",
                "destination_x_65536": "With source RECT x/y=5/0 and destination x=65536,y=0,width=1,height=1, native output aliases x=0 at 0x96e6e0/e1 because the byte-address expression masks higher x bits. This only establishes the single-word address expression, not arbitrary-width clipping or 32-bit wraparound."
            }
        },
        "static_row_counter_observation": {
            "A710": [
                {"rva": "0xa7ea", "instruction": "inc edx", "role": "high-entry current-y counter"},
                {"rva": "0xa8ce", "instruction": "inc ecx", "role": "low-entry current-y counter"}
            ],
            "A8F0": [
                {"rva": "0xaa4b", "instruction": "inc ecx", "role": "high-entry source-y counter"},
                {"rva": "0xaa4e", "instruction": "inc ebx", "role": "high-entry DWORD destination-y counter"},
                {"rva": "0xabd0", "instruction": "inc ecx", "role": "low-entry source-y counter"},
                {"rva": "0xabd3", "instruction": "inc ebx", "role": "low-entry DWORD destination-y counter"}
            ],
            "width": "All listed INC instructions are 32-bit register operations; source RECT y is sign-extended on entry. The DWORD A8F0 destination argument is loaded from [esp+0x2c].",
            "implication": "The row-address helper takes renderer_i32 current_y; only the callback entry-y selector takes renderer_i16."
        },
        "cases": results,
        "limitations": [
            "Finite synthetic rectangles only; negative coordinates, negative dimensions, clipping, huge heights, and all invalid-input semantics are unproved.",
            "The high-y helper remains domain-limited to y=512..1023; the y=1024 row is only observed as the next iteration of a rectangle beginning at 1023.",
            "No allocation ownership or texture backing extent is inferred from these addresses.",
            "No source/destination overlap case was added; existing native_vram_move.json covers finite overlap behavior separately.",
            "No DirectDraw, GPU, gameplay, FUNCTION_MATCH, source identity, or whole-module equality is claimed."
        ],
        "invocation": {"exe": str(ARGS.exe), "original_root": str(ARGS.original_root),
                       "receipt": str(ARGS.receipt), "deps": str(ARGS.deps),
                       "analysis_deps": str(ARGS.analysis_deps)}
    }
    write_receipt(ARGS.receipt, receipt)
    print(json.dumps({"status": receipt["status"], "cases": [
        {"name": c["name"], "entry": c["entry"],
         "observed_source": c["observed_source_byte_addresses"],
         "observed_destination": c["observed_destination_byte_addresses"],
         "helper_match": c["matches_helper_per_row_destination"],
         "entry_band_match": c["matches_entry_band_per_row_destination"],
         "linear_match": c["matches_cached_linear_destination"]}
        for c in results], "receipt": str(ARGS.receipt)}, indent=2))


if __name__ == "__main__":
    main()
