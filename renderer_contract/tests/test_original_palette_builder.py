"""Bounded CPU oracle for original HERCULES.EXE palette/LUT builder 71A0.

The runner maps the pinned PE bytes at the preferred base, runs 71A0 and its
original 6780 helper, and stops before the 69A0 call. No Windows API is called.
All table and palette buffers are synthetic emulator state; the original file
is only read and hash-checked.
"""
from __future__ import annotations

import collections
import argparse
import hashlib
import json
import platform
import struct
import sys
from pathlib import Path

SCRIPT_PATH = Path(__file__).resolve()
USER_ARGV = list(sys.argv)


def load_public_oracle_common():
    bootstrap = argparse.ArgumentParser(add_help=False)
    bootstrap.add_argument("--repo-root", required=True, type=Path)
    bootstrap_args, _unknown = bootstrap.parse_known_args()
    repo_root = bootstrap_args.repo_root.resolve()

    # When copied into renderer_contract/tests, use the sibling helper. In this
    # private staging location, use that same public helper under --repo-root.
    directories = [SCRIPT_PATH.parent, *SCRIPT_PATH.parents]
    directories.extend(directory / "renderer_contract" / "tests" for directory in SCRIPT_PATH.parents)
    directories.extend((repo_root / "renderer_contract" / "tests", repo_root / "tests"))
    helper = next((directory / "oracle_common.py" for directory in directories
                   if (directory / "oracle_common.py").is_file()), None)
    if helper is None:
        raise RuntimeError("cannot find public oracle_common.py beside this script or under --repo-root")
    sys.path.insert(0, str(helper.parent))
    from oracle_common import parse_args, verify_runtime, write_receipt  # noqa: E402

    normalized = [USER_ARGV[0]]
    index = 1
    while index < len(USER_ARGV):
        token = USER_ARGV[index]
        if token == "--repo-root":
            normalized.extend(("--original-root", USER_ARGV[index + 1]))
            index += 2
        elif token.startswith("--repo-root="):
            normalized.append("--original-root=" + token.split("=", 1)[1])
            index += 1
        elif token == "--original-root" or token.startswith("--original-root="):
            raise SystemExit("use --repo-root; this test supplies the original root explicitly")
        else:
            normalized.append(token)
            index += 1
    sys.argv[:] = normalized
    args = parse_args(
        "Run original 71A0 palette/LUT builder up to (but not into) its 69A0 call",
        "palette_builder_report.json",
    )
    if args.original_root != repo_root:
        raise SystemExit("--repo-root resolution differs from oracle_common --original-root")
    return args, parse_args, verify_runtime, write_receipt


ARGS, parse_args, verify_runtime, write_receipt = load_public_oracle_common()

from pe import PE  # noqa: E402
import unicorn  # noqa: E402
from unicorn import (  # noqa: E402
    Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_INVALID,
    UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MODE_32, UC_PROT_EXEC,
    UC_PROT_READ, UC_PROT_WRITE,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDI, UC_X86_REG_EDX, UC_X86_REG_EFLAGS, UC_X86_REG_EIP,
    UC_X86_REG_ESI, UC_X86_REG_ESP,
)

BASE = 0x400000
START_RVA = 0x71A0
STOP_RVA = 0x728D  # first byte of push 1; the following call at 0x728F is not run
BUILDER_SHA_SPAN = (START_RVA, STOP_RVA - START_RVA)
ENTRY_WRITER_RVA = 0x6780
ENTRY_WRITER_SIZE = 0x87
MAX_INSNS = 20_000_000

STACK = 0x20000000
STACK_SIZE = 0x10000
SCRATCH = 0x10000000
SCRATCH_SIZE = 0x4000
LOGPALETTE = SCRATCH + 0x1000
BITMAPINFO = SCRATCH + 0x2000

SHADE = 0x91C488
SHADE_SIZE = 0x10000
CLUT_RAMP = 0x92C490
CLUT_RAMP_SIZE = 0x2000
BLEND = 0x92E3D0
BLEND_SIZE = 0x40000
MODE_FLAG = 0x92C488
DD_MODE_BYTE = 0xBA020C
LOGPALETTE_GLOBAL = 0x96E3F8
BITMAPINFO_GLOBAL = 0x96E3F4

SHADE_STORE_RVA = 0x71DE
BLEND_STORE_RVAS = {0x720F, 0x7227, 0x723D, 0x724D}
CLUT_RAMP_STORE_RVA = 0x7274
PALETTE_STORE_RVAS = {0x67AD, 0x67BE, 0x67CC, 0x67D6, 0x67E0, 0x67EC, 0x67F6, 0x6801}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def align_page(value: int) -> int:
    return (value + 0xFFF) & ~0xFFF


def perms(characteristics: int) -> int:
    result = 0
    if characteristics & 0x40000000:
        result |= UC_PROT_READ
    if characteristics & 0x80000000:
        result |= UC_PROT_WRITE
    if characteristics & 0x20000000:
        result |= UC_PROT_EXEC
    return result


def map_pe(uc, pe: PE) -> None:
    header_size = pe.pe.OPTIONAL_HEADER.SizeOfHeaders
    uc.mem_map(BASE, align_page(header_size), UC_PROT_READ)
    uc.mem_write(BASE, pe.data[:header_size])
    for section in pe.sections:
        size = align_page(max(section["virtual_size"], section["raw_size"]))
        address = BASE + section["virtual_address"]
        uc.mem_map(address, size, perms(section["characteristics"]))
        if section["raw_size"]:
            uc.mem_write(address, pe.read_rva(section["virtual_address"], section["raw_size"]))
        if size > section["raw_size"]:
            uc.mem_write(address + section["raw_size"], b"\0" * (size - section["raw_size"]))


def contains(address: int, size: int, start: int, end: int) -> bool:
    return start <= address and address + size <= end


def expected_shade() -> bytes:
    return bytes(min(0xFF, (row * column) >> 7)
                 for row in range(256) for column in range(256))


def expected_blend() -> bytes:
    tables = bytearray()
    for selector in range(4):
        for row in range(256):
            for column in range(256):
                if selector == 0:
                    value = (row + column) >> 1
                elif selector == 1:
                    value = min(row + column, 0xFF)
                elif selector == 2:
                    value = max(column - row, 0)
                else:
                    value = min((row >> 1) + column, 0xFF)
                tables.append(value)
    # 71A0 generates tables, then writes its 32 procedural CLUT ramps. The
    # final 192 bytes of row 31 overlap selector-0 row 0, columns 0..191.
    tables[:0xC0] = bytes(range(0x40, 0x100))
    return bytes(tables)


def expected_palette() -> tuple[bytes, bytes]:
    log_entries = bytearray()
    dib_entries = bytearray()
    for index in range(256):
        log_entries.extend((index, index, index, 4))
        dib_entries.extend((index, index, index, 0))
    return bytes(log_entries), bytes(dib_entries)


def merge_ranges(ranges: list[tuple[int, int]]) -> list[tuple[int, int]]:
    merged: list[list[int]] = []
    for start, end in sorted(ranges):
        if not merged or start > merged[-1][1]:
            merged.append([start, end])
        else:
            merged[-1][1] = max(merged[-1][1], end)
    return [(start, end) for start, end in merged]


def outside_equal(before: bytes, after: bytes, base: int, allowed: list[tuple[int, int]]) -> bool:
    cursor = base
    for start, end in merge_ranges(allowed):
        if cursor < start and before[cursor - base:start - base] != after[cursor - base:start - base]:
            return False
        cursor = end
    return before[cursor - base:] == after[cursor - base:]


def run() -> dict:
    pe = PE(ARGS.exe)
    oracle_sha_before = sha(pe.data)
    if oracle_sha_before != "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d":
        raise AssertionError("pinned original EXE hash mismatch")
    runtime = verify_runtime(unicorn, ARGS.deps)
    text_section = next(section for section in pe.sections if section["name"] == ".text")
    data_section = next(section for section in pe.sections if section["name"] == ".data")
    text_base = BASE + text_section["virtual_address"]
    text_size = text_section["raw_size"]
    data_base = BASE + data_section["virtual_address"]
    data_size = align_page(max(data_section["virtual_size"], data_section["raw_size"]))

    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    map_pe(uc, pe)
    uc.mem_map(SCRATCH, SCRATCH_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(STACK, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)

    # Seed distinct table canaries in emulator memory, then place one explicit
    # nonzero mode-byte canary in the eight-byte gap before the CLUT ramp.
    uc.mem_write(SHADE, b"\xA5" * SHADE_SIZE)
    uc.mem_write(BLEND, b"\xB6" * BLEND_SIZE)
    uc.mem_write(CLUT_RAMP, b"\xC7" * CLUT_RAMP_SIZE)
    guards = {
        SHADE - 1: b"\x11",
        MODE_FLAG: b"\xD8",
        MODE_FLAG + 1: bytes(range(0xE1, 0xE8)),
        BLEND + BLEND_SIZE: b"\xF9",
    }
    for address, value in guards.items():
        uc.mem_write(address, value)

    # 6780 expects these two globals to point at caller-owned output structures.
    uc.mem_write(LOGPALETTE, struct.pack("<HH", 0x0300, 0x0100) + b"\xD1" * 0x404)
    bmi_header = bytes((i * 7 + 3) & 0xFF for i in range(0x28))
    uc.mem_write(BITMAPINFO, bmi_header + b"\xD2" * 0x404)
    uc.mem_write(LOGPALETTE_GLOBAL, struct.pack("<I", LOGPALETTE))
    uc.mem_write(BITMAPINFO_GLOBAL, struct.pack("<I", BITMAPINFO))

    data_before = bytes(uc.mem_read(data_base, data_size))
    text_before = bytes(uc.mem_read(text_base, text_size))
    scratch_before = bytes(uc.mem_read(SCRATCH, SCRATCH_SIZE))
    original_code_hash = sha(pe.read_rva(START_RVA, STOP_RVA - START_RVA))
    entry_writer_hash = sha(pe.read_rva(ENTRY_WRITER_RVA, ENTRY_WRITER_SIZE))
    selected_addresses = {
        BLEND + 0x3C: "selector0_row0_col60",
        BLEND + 0xBF: "selector0_row0_col191",
        BLEND + 0xC0: "selector0_row0_col192",
    }
    selected_events: dict[int, list[dict[str, str]]] = {address: [] for address in selected_addresses}
    write_counts: collections.Counter[int] = collections.Counter()
    table_writer_counts: collections.Counter[int] = collections.Counter()
    palette_writer_counts: collections.Counter[int] = collections.Counter()
    error: dict[str, object] = {"kind": None, "detail": None}
    stop: dict[str, object] = {"pc": None, "instruction_count": 0, "helper_entries": 0}
    last_pc = {"rva": None}

    def fail(kind: str, detail) -> None:
        if error["kind"] is None:
            error["kind"] = kind
            error["detail"] = detail
        uc.emu_stop()

    def in_stack(address: int, size: int) -> bool:
        return contains(address, size, STACK, STACK + STACK_SIZE)

    def in_scratch(address: int, size: int) -> bool:
        return contains(address, size, SCRATCH, SCRATCH + SCRATCH_SIZE)

    def on_code(machine, address, size, _user) -> None:
        rva = address - BASE
        if rva == STOP_RVA:
            stop["pc"] = hex(rva)
            machine.emu_stop()
            return
        if START_RVA <= rva < STOP_RVA:
            pass
        elif ENTRY_WRITER_RVA <= rva < ENTRY_WRITER_RVA + ENTRY_WRITER_SIZE:
            if rva == ENTRY_WRITER_RVA:
                stop["helper_entries"] = int(stop["helper_entries"]) + 1
        else:
            fail("unexpected_code", hex(rva))
            return
        stop["instruction_count"] = int(stop["instruction_count"]) + 1
        last_pc["rva"] = rva

    def on_read(machine, access, address, size, value, _user) -> None:
        pc = machine.reg_read(UC_X86_REG_EIP) - BASE
        if in_stack(address, size) or in_scratch(address, size):
            return
        if contains(address, size, data_base, data_base + data_size):
            if (address, size) in ((LOGPALETTE_GLOBAL, 4), (BITMAPINFO_GLOBAL, 4)):
                return
            fail("unexpected_original_data_read", {"pc": hex(pc), "address": hex(address), "size": size})
            return
        fail("unexpected_memory_read", {"pc": hex(pc), "address": hex(address), "size": size})

    def on_write(machine, access, address, size, value, _user) -> None:
        pc = machine.reg_read(UC_X86_REG_EIP) - BASE
        if in_stack(address, size):
            return
        if pc == SHADE_STORE_RVA:
            if not contains(address, size, SHADE, SHADE + SHADE_SIZE) or size != 1:
                fail("bad_shade_write", {"pc": hex(pc), "address": hex(address), "size": size})
                return
            write_counts[address] += 1
            table_writer_counts[pc] += 1
            return
        if pc in BLEND_STORE_RVAS:
            if not contains(address, size, BLEND, BLEND + BLEND_SIZE) or size != 1:
                fail("bad_blend_write", {"pc": hex(pc), "address": hex(address), "size": size})
                return
            write_counts[address] += 1
            table_writer_counts[pc] += 1
            if address in selected_events:
                selected_events[address].append({"pc_rva": hex(pc), "value": hex(value & 0xFF)})
            return
        if pc == CLUT_RAMP_STORE_RVA:
            if (not contains(address, size, CLUT_RAMP, CLUT_RAMP + CLUT_RAMP_SIZE) or size != 1 or
                    (value & 0xFF) != ((address - CLUT_RAMP) & 0xFF)):
                fail("bad_clut_ramp_write", {"pc": hex(pc), "address": hex(address), "size": size, "value": hex(value & 0xFF)})
                return
            write_counts[address] += 1
            table_writer_counts[pc] += 1
            if address in selected_events:
                selected_events[address].append({"pc_rva": hex(pc), "value": hex(value & 0xFF)})
            return
        log_start, log_end = LOGPALETTE + 4, LOGPALETTE + 4 + 0x400
        dib_start, dib_end = BITMAPINFO + 0x28, BITMAPINFO + 0x28 + 0x400
        if in_scratch(address, size):
            if not (pc in PALETTE_STORE_RVAS and size == 1 and
                    (contains(address, size, log_start, log_end) or contains(address, size, dib_start, dib_end))):
                fail("unexpected_scratch_write", {"pc": hex(pc), "address": hex(address), "size": size})
                return
            palette_writer_counts[pc] += 1
            return
        fail("unexpected_memory_write", {"pc": hex(pc), "address": hex(address), "size": size})

    def on_invalid(machine, access, address, size, value, _user):
        fail("invalid_memory", {
            "pc": hex(machine.reg_read(UC_X86_REG_EIP) - BASE),
            "access": access,
            "address": hex(address),
            "size": size,
        })
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_READ, on_read)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
    uc.hook_add(UC_HOOK_MEM_INVALID, on_invalid)

    esp = STACK + 0x8000
    for register, value in (
        (UC_X86_REG_EAX, 0), (UC_X86_REG_EBX, 0), (UC_X86_REG_ECX, 0),
        (UC_X86_REG_EDX, 0), (UC_X86_REG_ESI, 0), (UC_X86_REG_EDI, 0),
        (UC_X86_REG_EBP, 0), (UC_X86_REG_ESP, esp),
        (UC_X86_REG_EFLAGS, 0x202), (UC_X86_REG_EIP, BASE + START_RVA),
    ):
        uc.reg_write(register, value)

    emu_error = None
    try:
        uc.emu_start(BASE + START_RVA, 0, timeout=0, count=MAX_INSNS)
    except UcError as exc:
        emu_error = str(exc)

    if error["kind"]:
        raise AssertionError(f"emulator audit failed: {error}")
    if emu_error:
        raise AssertionError(f"Unicorn error before stop: {emu_error}")
    if stop["pc"] != hex(STOP_RVA):
        raise AssertionError(f"did not stop at pre-69A0 boundary; stop state={stop}")
    if int(stop["instruction_count"]) >= MAX_INSNS:
        raise AssertionError(f"instruction budget exhausted: {stop['instruction_count']}")

    expected_table_writes = {
        SHADE_STORE_RVA: SHADE_SIZE,
        0x720F: 0x10000,
        0x7227: 0x10000,
        0x723D: 0x10000,
        0x724D: 0x10000,
        CLUT_RAMP_STORE_RVA: CLUT_RAMP_SIZE,
    }
    observed_table_writes = {key: table_writer_counts[key] for key in expected_table_writes}
    if observed_table_writes != expected_table_writes:
        raise AssertionError(f"table writer event counts differ: {observed_table_writes}")
    expected_unique_table_addresses = SHADE_SIZE + BLEND_SIZE + CLUT_RAMP_SIZE - 0xC0
    if len(write_counts) != expected_unique_table_addresses:
        raise AssertionError(f"unexpected unique table-byte writes: {len(write_counts)}")
    if sum(write_counts.values()) != SHADE_SIZE + BLEND_SIZE + CLUT_RAMP_SIZE:
        raise AssertionError(f"unexpected total table-byte writes: {sum(write_counts.values())}")
    overlap_start, overlap_end = BLEND, BLEND + 0xC0
    for address, count in write_counts.items():
        expected_count = 2 if overlap_start <= address < overlap_end else 1
        if count != expected_count:
            raise AssertionError(f"table byte write multiplicity differs at {hex(address)}: {count} != {expected_count}")

    shade_actual = bytes(uc.mem_read(SHADE, SHADE_SIZE))
    blend_actual = bytes(uc.mem_read(BLEND, BLEND_SIZE))
    clut_actual = bytes(uc.mem_read(CLUT_RAMP, CLUT_RAMP_SIZE))
    shade_want = expected_shade()
    blend_want = expected_blend()
    if shade_actual != shade_want:
        raise AssertionError("full shade table differs from independent formula")
    if blend_actual != blend_want:
        mismatch = next(index for index, (actual, wanted) in enumerate(zip(blend_actual, blend_want)) if actual != wanted)
        raise AssertionError({"message": "full blend tables differ from formula plus post-generation CLUT ramp overlap",
                              "first_mismatch": hex(mismatch), "actual": hex(blend_actual[mismatch]),
                              "expected": hex(blend_want[mismatch]),
                              "actual_context": blend_actual[max(0, mismatch - 4):mismatch + 5].hex(),
                              "expected_context": blend_want[max(0, mismatch - 4):mismatch + 5].hex()})
    expected_clut = bytes(range(256)) * 32
    if clut_actual != expected_clut:
        raise AssertionError("full CLUT range differs from 32 generated 0..255 ramps")

    log_expected, dib_expected = expected_palette()
    log_actual = bytes(uc.mem_read(LOGPALETTE + 4, 0x400))
    dib_actual = bytes(uc.mem_read(BITMAPINFO + 0x28, 0x400))
    if log_actual != log_expected or dib_actual != dib_expected:
        raise AssertionError("full synthetic palette outputs differ from 6780 grayscale entries")
    if bytes(uc.mem_read(LOGPALETTE, 4)) != struct.pack("<HH", 0x0300, 0x0100):
        raise AssertionError("LOGPALETTE header changed")
    if bytes(uc.mem_read(BITMAPINFO, 0x28)) != bmi_header:
        raise AssertionError("BITMAPINFO header changed")
    expected_palette_stores = {pc: 256 for pc in PALETTE_STORE_RVAS}
    if dict(palette_writer_counts) != expected_palette_stores:
        raise AssertionError(f"palette writer store counts differ: {dict(palette_writer_counts)}")

    expected_selected_events = {
        BLEND + 0x3C: [{"pc_rva": "0x720f", "value": "0x1e"}, {"pc_rva": "0x7274", "value": "0x7c"}],
        BLEND + 0xBF: [{"pc_rva": "0x720f", "value": "0x5f"}, {"pc_rva": "0x7274", "value": "0xff"}],
        BLEND + 0xC0: [{"pc_rva": "0x720f", "value": "0x60"}],
    }
    if selected_events != expected_selected_events:
        raise AssertionError(f"writer ordering at overlap boundary differs: {selected_events}")
    selector_distinct = [blend_actual[s * 0x10000 + 7 * 256 + 3] for s in range(4)]
    if selector_distinct != [5, 10, 0, 6] or len(set(selector_distinct)) != 4:
        raise AssertionError(f"selector discriminator differs: {selector_distinct}")
    selector0_source0_dest3c = blend_actual[0x3C]
    if selector0_source0_dest3c != 0x7C:
        raise AssertionError("selector 0 source=0,destination=0x3c did not observe the CLUT-overlap ramp value")
    formula_before_overlap = (0 + 0x3C) >> 1
    if formula_before_overlap != 0x1E:
        raise AssertionError("independent selector-0 average fixture is malformed")

    for address, expected in guards.items():
        actual = bytes(uc.mem_read(address, len(expected)))
        if actual != expected:
            raise AssertionError(f"guard canary changed at {hex(address)}: {actual.hex()} != {expected.hex()}")
    # The table generator's permitted host-data writes are precisely these
    # ranges (with the blend/CLUT 0xC0-byte overlap counted twice).
    table_allow = [
        (SHADE, SHADE + SHADE_SIZE),
        (CLUT_RAMP, CLUT_RAMP + CLUT_RAMP_SIZE),
        (BLEND, BLEND + BLEND_SIZE),
    ]
    data_after = bytes(uc.mem_read(data_base, data_size))
    text_after = bytes(uc.mem_read(text_base, text_size))
    scratch_after = bytes(uc.mem_read(SCRATCH, SCRATCH_SIZE))
    scratch_allow = [
        (LOGPALETTE + 4, LOGPALETTE + 4 + 0x400),
        (BITMAPINFO + 0x28, BITMAPINFO + 0x28 + 0x400),
    ]
    if not outside_equal(data_before, data_after, data_base, table_allow):
        raise AssertionError("original .data changed outside the explicit shade/blend/CLUT write union")
    if not outside_equal(scratch_before, scratch_after, SCRATCH, scratch_allow):
        raise AssertionError("synthetic palette buffers changed outside their entry tables")
    if text_after != text_before:
        raise AssertionError("original mapped .text bytes changed")
    oracle_sha_after = sha(ARGS.exe.read_bytes())
    if oracle_sha_after != oracle_sha_before:
        raise AssertionError("original EXE on disk changed during emulation")

    section_info = {
        "text": {"rva": hex(text_section["virtual_address"]), "raw_size": text_section["raw_size"]},
        "data": {
            "rva": hex(data_section["virtual_address"]),
            "virtual_size": hex(data_section["virtual_size"]),
            "raw_size": hex(data_section["raw_size"]),
            "mode_byte_rva": hex(DD_MODE_BYTE - BASE),
        },
    }
    return {
        "schema_version": 1,
        "status": "PASS",
        "scope": "BOUNDED_EQUIVALENT: unchanged original 71A0 builder and 6780 palette-entry helper, stopped before the 69A0 call; CPU emulation only, no OS/window/DirectDraw APIs.",
        "oracle": {
            "path": str(ARGS.exe),
            "sha256_before": oracle_sha_before,
            "sha256_after": oracle_sha_after,
            "image_base": hex(pe.image_base),
            "builder_start_rva": hex(START_RVA),
            "stop_before_rva": hex(STOP_RVA),
            "executed_builder_span_size": BUILDER_SHA_SPAN[1],
            "executed_builder_span_sha256": original_code_hash,
            "palette_helper_rva": hex(ENTRY_WRITER_RVA),
            "palette_helper_size": ENTRY_WRITER_SIZE,
            "palette_helper_sha256": entry_writer_hash,
            "sections": section_info,
        },
        "runtime": {
            "python": sys.executable,
            "python_version": platform.python_version(),
            **runtime,
        },
        "command": " ".join(sys.argv),
        "execution": {
            "fixture_scenario_count": 1,
            "preferred_base_original_pe_mapped": True,
            "unknown_code_or_memory_fails": True,
            "instruction_budget": MAX_INSNS,
            "instruction_count": stop["instruction_count"],
            "helper_6780_entries": stop["helper_entries"],
            "stopped_at_rva": stop["pc"],
            "69A0_call_rva_728F_executed": False,
            "only_allowed_original_data_writes": True,
            "allowed_original_write_ranges": [
                {"start": hex(SHADE), "end_exclusive": hex(SHADE + SHADE_SIZE), "writer_rva": hex(SHADE_STORE_RVA)},
                {"start": hex(BLEND), "end_exclusive": hex(BLEND + BLEND_SIZE), "writer_rvas": [hex(v) for v in sorted(BLEND_STORE_RVAS)]},
                {"start": hex(CLUT_RAMP), "end_exclusive": hex(CLUT_RAMP + CLUT_RAMP_SIZE), "writer_rva": hex(CLUT_RAMP_STORE_RVA)},
            ],
            "table_byte_write_events": sum(write_counts.values()),
            "unique_table_bytes_written": len(write_counts),
            "table_writer_counts_by_rva": {hex(k): v for k, v in sorted(table_writer_counts.items())},
            "palette_entry_store_events": sum(palette_writer_counts.values()),
            "original_text_unchanged": text_after == text_before,
            "original_data_outside_table_union_unchanged": True,
            "synthetic_palette_buffers_outside_entries_unchanged": True,
            "original_file_unchanged": oracle_sha_after == oracle_sha_before,
        },
        "fixtures": {
            "seed_canaries": {"shade": "0xa5", "blend": "0xb6", "clut_ramp_range": "0xc7", "mode_flag_0x92c488": "0xd8"},
            "guard_canaries_unchanged": {hex(k): value.hex() for k, value in guards.items()},
            "shade_final_sha256": sha(shade_actual),
            "shade_matches_formula_all_65536_bytes": True,
            "blend_final_sha256": sha(blend_actual),
            "blend_matches_formula_plus_clut_overlap_all_262144_bytes": True,
            "clut_ramp_final_sha256": sha(clut_actual),
            "clut_ramp_matches_all_8192_ramp_bytes": True,
            "logical_palette_entries_sha256": sha(log_actual),
            "dib_rgbquad_entries_sha256": sha(dib_actual),
            "palette_entries_match_grayscale_256": True,
            "selector0_source_zero_destination_0x3c": {
                "average_formula_before_overlap": formula_before_overlap,
                "final_table_value": selector0_source0_dest3c,
                "cell_address": hex(BLEND + 0x3C),
            },
            "selector0_row0_clut_overlap_boundary": {
                "column_191": {"formula": 0x5F, "final": blend_actual[0xBF]},
                "column_192": {"formula": 0x60, "final": blend_actual[0xC0]},
                "selected_original_writer_events": {hex(k): v for k, v in selected_events.items()},
            },
            "distinct_selector_sample_row7_col3": selector_distinct,
            "mode_flag_preserved": bytes(uc.mem_read(MODE_FLAG, 1)).hex() == "d8",
        },
        "limitations": [
            "This executes the builder only; it does not execute 69A0, initialize DirectDraw/Windows, or establish runtime deployment behavior.",
            "The test verifies fixed table bytes and selected discriminators, not complete game rendering or visual output.",
            "Original PE bytes are mapped unchanged; mutable tables and palette arrays are fixture state inside Unicorn only.",
        ],
    }


if __name__ == "__main__":
    receipt = run()
    receipt["command"] = " ".join(USER_ARGV)
    write_receipt(ARGS.receipt, receipt)
    print(json.dumps({
        "status": receipt["status"],
        "receipt": str(ARGS.receipt),
        "instruction_count": receipt["execution"]["instruction_count"],
        "shade_sha256": receipt["fixtures"]["shade_final_sha256"],
        "blend_sha256": receipt["fixtures"]["blend_final_sha256"],
        "clut_sha256": receipt["fixtures"]["clut_ramp_final_sha256"],
    }, indent=2))
