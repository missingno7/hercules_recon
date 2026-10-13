"""Finite Unicorn execution oracle for original A8F0 moves and A710 aliasing."""
from __future__ import annotations

import json
import struct
import sys
from collections import Counter
from pathlib import Path

def find_repo_root(start: Path) -> Path:
    for candidate in (start, *start.parents):
        if (candidate / "renderer_contract").is_dir():
            return candidate
    raise SystemExit(f"cannot find an ancestor containing renderer_contract from {start}")


REPO_ROOT = find_repo_root(Path(__file__).resolve().parent)
EXPECTED_PUBLIC_ORACLE_COMMON_SHA256 = (
    "a024048d9acc6e18197e10a82427cd84351a90666e1626e610bf60c6105f1357"
)
PUBLIC_ORACLE_CANDIDATES = (
    REPO_ROOT / "renderer_contract" / "tests" / "oracle_common.py",
    Path(__file__).resolve().with_name("oracle_common.py"),
)
PUBLIC_ORACLE_COMMON = next(
    (candidate for candidate in PUBLIC_ORACLE_CANDIDATES if candidate.is_file()), None
)
if PUBLIC_ORACLE_COMMON is None:
    raise SystemExit(
        "public oracle helper is missing from renderer_contract/tests and runner sibling"
    )
PUBLIC_ORACLE_COMMON = PUBLIC_ORACLE_COMMON.resolve()
sys.path.insert(0, str(PUBLIC_ORACLE_COMMON.parent))
from oracle_common import parse_args, verify_runtime, write_receipt

if not any(arg == "--receipt" or arg.startswith("--receipt=") for arg in sys.argv[1:]):
    raise SystemExit("--receipt is required; choose an output path under this repository's work/")
if not any(arg == "--original-root" or arg.startswith("--original-root=") for arg in sys.argv[1:]):
    sys.argv[1:1] = ["--original-root", str(REPO_ROOT)]

ARGS = parse_args(
    "Run finite original HERCULES.EXE A8F0 VRAM-move fixtures",
    "vram_move_report.json",
)
if ARGS.original_root.resolve() != REPO_ROOT.resolve():
    raise SystemExit(f"--original-root must match detected repository root {REPO_ROOT}")
try:
    ARGS.receipt.resolve().relative_to((REPO_ROOT / "work").resolve())
except ValueError as error:
    raise SystemExit(f"--receipt must remain under {REPO_ROOT / 'work'}") from error
if ARGS.receipt.resolve() == ARGS.exe.resolve():
    raise SystemExit("--receipt must not overwrite the original executable")

PUBLIC_ORACLE_COMMON_SHA256 = __import__("hashlib").sha256(
    PUBLIC_ORACLE_COMMON.read_bytes()
).hexdigest()
if PUBLIC_ORACLE_COMMON_SHA256 != EXPECTED_PUBLIC_ORACLE_COMMON_SHA256:
    raise SystemExit(
        "unexpected public oracle helper SHA-256: " + PUBLIC_ORACLE_COMMON_SHA256
    )
RUNNER_SHA256 = __import__("hashlib").sha256(
    Path(__file__).resolve().read_bytes()
).hexdigest()

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
    UC_X86_REG_EAX,
    UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,
    UC_X86_REG_ESP,
)

PAGE = 0x1000
EXE_BASE = 0x00400000
A8F0_RVA = 0xA8F0
A8F0_END_RVA = 0xABF3
A710_RVA = 0xA710
A710_END_RVA = 0xA8E5
BD80_RVA = 0xBD80
BD80_END_RVA = 0xBD96
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

# Low-y original A8F0 stores: fast row copy and byte fallback.
A8F0_FAST_DWORD_PC = EXE_BASE + 0xAB42
A8F0_FAST_TAIL_PC = EXE_BASE + 0xAB49
A8F0_BYTE_STORE_PC = EXE_BASE + 0xABA1
# A710's corresponding low-y string-copy stores, used only by alias case.
A710_FAST_DWORD_PC = EXE_BASE + 0xA873
A710_FAST_TAIL_PC = EXE_BASE + 0xA87A
A710_BYTE_STORE_PC = EXE_BASE + 0xA8A9


def fail(message: str) -> None:
    raise AssertionError(message)


def align_up(value: int, alignment: int = PAGE) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def sha256(data: bytes) -> str:
    import hashlib
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


def map_original_sections(uc: Uc, pe: PE):
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
            uc.mem_write(va, pe.read_rva(section["virtual_address"], section["raw_size"]))
        if mapped_size > section["raw_size"]:
            uc.mem_write(va + section["raw_size"], b"\0" * (mapped_size - section["raw_size"]))
        mapped.append({
            "name": section["name"], "va": va, "size": mapped_size,
            "virtual_size": section["virtual_size"], "raw_size": section["raw_size"],
            "permissions": section["characteristics"],
        })
    return mapped


def section_span(pe: PE, name: str):
    section = next(section for section in pe.sections if section["name"] == name)
    start = EXE_BASE + section["virtual_address"]
    size = align_up(max(section["virtual_size"], section["raw_size"]))
    return start, size


def row_index(y: int) -> int:
    if not 0 <= y < 512:
        raise ValueError("fixture is outside bounded y<512 domain")
    return (0 if y < 256 else 0x800) + (y & 0xFF)


def address_for_byte(x_word: int, y: int, byte_in_row: int) -> int:
    byte_x = 2 * x_word + byte_in_row
    return (
        TEXTURE_BASE + row_index(y) * 0x100
        + ((byte_x & 0x700) << 8) + (byte_x & 0xFF)
    )


def valid_rect(x: int, y: int, width: int, height: int) -> bool:
    return (
        0 <= x <= 1023 and 0 <= y < 512
        and 0 <= width and x + width <= 1024
        and 0 <= height and y + height <= 512
    )


def x_range_crosses_page(x: int, width: int) -> bool:
    if width <= 0:
        return False
    return ((x ^ (x + width - 1)) & ~0x7F) != 0


def copy_cases():
    return [
        {"name": "distinct_fast_rectangle", "src": (5, 40), "dst": (15, 60), "size": (7, 3)},
        {"name": "horizontal_overlap_right", "src": (20, 80), "dst": (21, 80), "size": (8, 1)},
        {"name": "horizontal_overlap_left", "src": (21, 90), "dst": (20, 90), "size": (8, 1)},
        {"name": "vertical_overlap_down", "src": (30, 100), "dst": (30, 101), "size": (6, 3)},
        {"name": "vertical_overlap_up", "src": (30, 101), "dst": (30, 100), "size": (6, 3)},
        {"name": "both_x_page_cross", "src": (127, 120), "dst": (255, 130), "size": (3, 2)},
        {"name": "zero_height", "src": (4, 140), "dst": (16, 160), "size": (5, 0)},
    ]


def byte_seed(address: int, salt: int) -> int:
    return (address * 37 ^ (address >> 8) ^ (address >> 16) ^ (salt * 73)) & 0xFF


def addresses_for_copy(case):
    sx, sy = case["src"]
    dx, dy = case["dst"]
    width, height = case["size"]
    sources, destinations = [], []
    for row in range(height):
        for byte_i in range(width * 2):
            sources.append(address_for_byte(sx, sy + row, byte_i))
            destinations.append(address_for_byte(dx, dy + row, byte_i))
    return sources, destinations


def simulate_copy(initial: bytes, data_va: int, case, *, a710_source=None):
    """Model the observed low-y instruction order, including REP dword chunks."""
    sx, sy = case["src"]
    dx, dy = case["dst"]
    width, height = case["size"]
    if not all((valid_rect(sx, sy, width, height), valid_rect(dx, dy, width, height))):
        raise ValueError(f"fixture outside bounded address domain: {case}")
    state = {}
    sources, destinations = addresses_for_copy(case)
    for address in sources + destinations:
        state[address] = initial[address - data_va]
    if a710_source is not None:
        for offset in range(width * height * 2):
            state[a710_source + offset] = initial[a710_source + offset - data_va]

    def read(address):
        return state[address]

    events = []
    fast = not x_range_crosses_page(dx, width)
    if a710_source is None:
        fast = fast and not x_range_crosses_page(sx, width)
    for row in range(height):
        if fast:
            source_start = (
                a710_source + row * width * 2
                if a710_source is not None
                else address_for_byte(sx, sy + row, 0)
            )
            destination_start = address_for_byte(dx, dy + row, 0)
            byte_count = width * 2
            dword_count = byte_count // 4
            for dword_index in range(dword_count):
                source = source_start + 4 * dword_index
                destination = destination_start + 4 * dword_index
                chunk = bytes(read(source + i) for i in range(4))
                value = int.from_bytes(chunk, "little")
                events.append((destination, 4, value,
                               A710_FAST_DWORD_PC if a710_source is not None else A8F0_FAST_DWORD_PC))
                for index, byte in enumerate(chunk):
                    state[destination + index] = byte
            for tail_index in range(byte_count & 3):
                index = 4 * dword_count + tail_index
                source = source_start + index
                destination = destination_start + index
                value = read(source)
                events.append((destination, 1, value,
                               A710_FAST_TAIL_PC if a710_source is not None else A8F0_FAST_TAIL_PC))
                state[destination] = value
        else:
            if a710_source is not None:
                raise ValueError("A710 alias fixture intentionally stays in its fast row path")
            for byte_i in range(width * 2):
                source = address_for_byte(sx, sy + row, byte_i)
                destination = address_for_byte(dx, dy + row, byte_i)
                value = read(source)
                events.append((destination, 1, value, A8F0_BYTE_STORE_PC))
                state[destination] = value
    return events, state


def original_ranges(entry_rva: int, entry_end: int):
    return [
        (EXE_BASE + entry_rva, EXE_BASE + entry_end),
        (EXE_BASE + BD80_RVA, EXE_BASE + BD80_END_RVA),
    ]


def run_move_case(pe: PE, case, *, entry_rva=A8F0_RVA, notify_gate=0,
                  notify_endpoint=False, alias_source=None):
    sx, sy = case["src"]
    dx, dy = case["dst"]
    width, height = case["size"]
    if not valid_rect(sx, sy, width, height) or not valid_rect(dx, dy, width, height):
        fail(f"{case['name']} is outside x<1024,y<512 fixtures: {case}")
    if alias_source is not None and (entry_rva != A710_RVA or height != 1):
        fail("A710 native-map source alias is restricted to its single-row fixture")

    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    mapped = map_original_sections(uc, pe)
    uc.mem_map(INPUT_BASE, 0x4000, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(RETURN_SENTINEL, PAGE, UC_PROT_READ | UC_PROT_EXEC)
    if notify_endpoint:
        # Empty executable page plus pre-fetch hook only: no callback bytes are written or run.
        uc.mem_map(UNKNOWN_CALLBACK, PAGE, UC_PROT_EXEC)

    data_va, data_size = section_span(pe, ".data")
    text_va, text_size = section_span(pe, ".text")
    before_data = bytes(uc.mem_read(data_va, data_size))
    before_text = bytes(uc.mem_read(text_va, text_size))
    entry_end = A710_END_RVA if entry_rva == A710_RVA else A8F0_END_RVA
    before_entry = bytes(uc.mem_read(EXE_BASE + entry_rva, entry_end - entry_rva))
    before_notify = bytes(uc.mem_read(EXE_BASE + BD80_RVA, BD80_END_RVA - BD80_RVA))

    sources, destinations = addresses_for_copy(case)
    seeded = set(sources + destinations)
    if alias_source is not None:
        seeded.update(alias_source + offset for offset in range(width * 2))
    for address in seeded:
        uc.mem_write(address, bytes([byte_seed(address, len(case["name"]))]))

    rect_va = INPUT_BASE + 0x100
    if entry_rva == A710_RVA:
        uc.mem_write(rect_va, struct.pack("<4h", dx, dy, width, height))
        source_pointer = alias_source
        if source_pointer is None:
            fail("only the native texture-map A710 source alias fixture is supported")
        args = (rect_va, source_pointer)
    else:
        uc.mem_write(rect_va, struct.pack("<4h", sx, sy, width, height))
        args = (rect_va, dx, dy)

    uc.mem_write(CALLBACK_CELL, struct.pack("<I", UNKNOWN_CALLBACK))
    uc.mem_write(NOTIFICATION_GATE, struct.pack("<I", notify_gate))
    fixture_data = bytes(uc.mem_read(data_va, data_size))
    expected_events, expected_state = simulate_copy(
        fixture_data, data_va, case, a710_source=alias_source
    )

    sp = STACK_BASE + STACK_SIZE - 0x100
    uc.mem_write(sp, struct.pack("<I", RETURN_SENTINEL))
    for index, arg in enumerate(args):
        uc.mem_write(sp + 4 * (index + 1), struct.pack("<I", arg & 0xFFFFFFFF))
    uc.reg_write(UC_X86_REG_ESP, sp)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x202)  # DF clear, as at an ordinary 32-bit call.
    uc.reg_write(UC_X86_REG_EIP, EXE_BASE + entry_rva)

    events = []
    trace = []
    chronology = {"step": 0, "stop": None, "endpoint": None}
    data_end = data_va + data_size
    scratch = [(INPUT_BASE, INPUT_BASE + 0x4000), (STACK_BASE, STACK_BASE + STACK_SIZE)]
    allowed = original_ranges(entry_rva, entry_end)

    def in_original(address):
        return any(start <= address < end for start, end in allowed)

    def on_code(machine, address, _size, _user):
        chronology["step"] += 1
        if address == RETURN_SENTINEL:
            chronology["stop"] = "return_sentinel"
            machine.emu_stop()
            return
        if notify_endpoint and address == UNKNOWN_CALLBACK:
            endpoint_sp = machine.reg_read(UC_X86_REG_ESP)
            return_address = struct.unpack("<I", bytes(machine.mem_read(endpoint_sp, 4)))[0]
            chronology["endpoint"] = {
                "eip": hex(machine.reg_read(UC_X86_REG_EIP)),
                "esp": hex(endpoint_sp),
                "eax": hex(machine.reg_read(UC_X86_REG_EAX)),
                "stack_return_address": hex(return_address),
                "callback_bytes_written_or_executed": False,
            }
            chronology["stop"] = "notification_endpoint"
            machine.emu_stop()
            return
        if not in_original(address):
            chronology["stop"] = {"kind": "unexpected_code", "eip": hex(address)}
            machine.emu_stop()
            return
        trace.append({"step": chronology["step"], "rva": address - EXE_BASE})
        if chronology["step"] > MAX_INSTRUCTIONS:
            chronology["stop"] = "instruction_budget"
            machine.emu_stop()

    def on_write(machine, _access, address, size, value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP)
        if data_va <= address and address + size <= data_end:
            events.append({
                "step": chronology["step"], "pc_rva": pc - EXE_BASE,
                "address": address, "size": size,
                "value": value & ((1 << (8 * size)) - 1),
            })
            return
        if any(start <= address and address + size <= end for start, end in scratch):
            return
        chronology["stop"] = {
            "kind": "unexpected_write", "pc": hex(pc),
            "address": hex(address), "size": size,
        }
        machine.emu_stop()

    def on_invalid(machine, access, address, size, _value, _user):
        chronology["stop"] = {
            "kind": "invalid_memory", "pc": hex(machine.reg_read(UC_X86_REG_EIP)),
            "access": access, "address": hex(address), "size": size,
        }
        machine.emu_stop()
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
    uc.hook_add(UC_HOOK_MEM_INVALID, on_invalid)
    try:
        uc.emu_start(EXE_BASE + entry_rva, 0, timeout=2_000_000, count=MAX_INSTRUCTIONS)
    except UcError as error:
        if chronology["stop"] is None:
            chronology["stop"] = {"kind": "unicorn_error", "detail": str(error)}

    wanted_stop = "notification_endpoint" if notify_endpoint else "return_sentinel"
    if chronology["stop"] != wanted_stop:
        fail(f"{case['name']} stopped unexpectedly: {chronology['stop']}")
    if not trace or trace[0]["rva"] != entry_rva:
        fail(f"{case['name']} did not begin at original entry {entry_rva:#x}")
    if sum(row["rva"] == BD80_RVA for row in trace) != 1:
        fail(f"{case['name']} did not enter original BD80 exactly once")
    if notify_gate == 0:
        gate_path = [0xBD80, 0xBD85, 0xBD87, 0xBD89, 0xBD8F, 0xBD91, 0xBD95]
        observed_rvas = {row["rva"] for row in trace}
        if not all(rva in observed_rvas for rva in gate_path) or 0xBD93 in observed_rvas:
            fail(f"{case['name']} missed BD80's disabled full-return gate path")
    else:
        if not notify_endpoint or chronology["endpoint"] is None:
            fail("nonzero BD80 gate must stop at the empty synthetic notify endpoint")
        if chronology["endpoint"]["stack_return_address"] != hex(EXE_BASE + (0xABE9 if entry_rva == A8F0_RVA else 0xA8E3)):
            fail(f"{case['name']} notification endpoint stack did not retain native return address")
        if EXE_BASE + 0xBD93 not in [EXE_BASE + row["rva"] for row in trace]:
            fail(f"{case['name']} did not execute BD80's original gated jump")

    actual_events = [
        (row["address"], row["size"], row["value"], EXE_BASE + row["pc_rva"])
        for row in events
    ]
    if actual_events != expected_events:
        fail(f"{case['name']} native store stream differs from instruction-derived model: "
             f"actual={actual_events!r} expected={expected_events!r}")

    bd80_step = min(row["step"] for row in trace if row["rva"] == BD80_RVA)
    if events and max(row["step"] for row in events) >= bd80_step:
        fail(f"{case['name']} notification started before the last output store")

    after_data = bytes(uc.mem_read(data_va, data_size))
    expected_diff = {
        address for address, value in expected_state.items()
        if fixture_data[address - data_va] != value
    }
    actual_diff = {
        data_va + index
        for index, (before, after) in enumerate(zip(fixture_data, after_data))
        if before != after
    }
    if actual_diff != expected_diff:
        fail(f"{case['name']} data changed outside modeled stores: "
             f"unexpected={[hex(a) for a in sorted(actual_diff - expected_diff)[:16]]} "
             f"missing={[hex(a) for a in sorted(expected_diff - actual_diff)[:16]]}")
    if bytes(uc.mem_read(text_va, text_size)) != before_text:
        fail(f"{case['name']} modified original .text bytes")
    if bytes(uc.mem_read(EXE_BASE + entry_rva, entry_end - entry_rva)) != before_entry:
        fail(f"{case['name']} modified original entry bytes")
    if bytes(uc.mem_read(EXE_BASE + BD80_RVA, BD80_END_RVA - BD80_RVA)) != before_notify:
        fail(f"{case['name']} modified original BD80 bytes")
    if struct.unpack("<I", bytes(uc.mem_read(CALLBACK_CELL, 4)))[0] != UNKNOWN_CALLBACK:
        fail(f"{case['name']} changed callback cell")
    if struct.unpack("<I", bytes(uc.mem_read(NOTIFICATION_GATE, 4)))[0] != notify_gate:
        fail(f"{case['name']} changed notification gate")

    write_pc_counts = Counter(hex(row["pc_rva"]) for row in events)
    write_byte_counts = Counter()
    for row in events:
        write_byte_counts[hex(row["pc_rva"])] += row["size"]
    source_read_addresses = (
        [alias_source + offset for offset in range(width * height * 2)]
        if alias_source is not None else sources
    )
    output_bytes = []
    for row in events:
        output_bytes.extend(
            {"address": row["address"] + offset,
             "value": (row["value"] >> (8 * offset)) & 0xFF,
             "pc_rva": hex(row["pc_rva"])}
            for offset in range(row["size"])
        )
    report = {
        "name": case["name"],
        "entry_rva": hex(entry_rva),
        "source_xy": list(case["src"]),
        "destination_xy": list(case["dst"]),
        "width_height_words": list(case["size"]),
        "notification_gate": notify_gate,
        "stop": chronology["stop"],
        "notification_endpoint": chronology["endpoint"],
        "trace_count": len(trace),
        "bd80_trace_rvas": [hex(row["rva"]) for row in trace if 0xBD80 <= row["rva"] < 0xBD96],
        "bd80_call_count": sum(row["rva"] == BD80_RVA for row in trace),
        "last_output_store_step": max((row["step"] for row in events), default=None),
        "bd80_entry_step": bd80_step,
        "output_write_event_count": len(events),
        "output_byte_count": len(output_bytes),
        "write_pc_event_counts": dict(write_pc_counts),
        "write_pc_byte_counts": dict(write_byte_counts),
        "output_write_bytes": output_bytes,
        "source_read_addresses": [hex(address) for address in source_read_addresses],
        "source_bytes_before": [
            hex(fixture_data[address - data_va]) for address in source_read_addresses
        ],
        "source_bytes_after": [
            hex(expected_state.get(address, fixture_data[address - data_va]))
            for address in source_read_addresses
        ],
        "destination_addresses": [hex(address) for address in destinations],
        "destination_bytes_before": [
            hex(fixture_data[address - data_va]) for address in destinations
        ],
        "final_destination_bytes": [
            {"address": hex(address), "value": hex(expected_state[address])}
            for address in destinations
        ],
        "source_seed_sha256": sha256(bytes(byte_seed(address, len(case["name"])) for address in sorted(seeded))),
        "native_writes_match_instruction_derived_model": actual_events == expected_events,
        "untouched_data_outside_modeled_stores": actual_diff == expected_diff,
        "code_bytes_unchanged": True,
        "returned_to_sentinel": chronology["stop"] == "return_sentinel",
    }

    # Restore the entire private emulator .data image, including fixture writes.
    uc.mem_write(data_va, before_data)
    if bytes(uc.mem_read(data_va, data_size)) != before_data:
        fail(f"{case['name']} failed full .data restoration")
    report["private_data_state_restored"] = True
    return report, mapped


def run_a710_alias(pe: PE):
    case = {
        "name": "a710_borrowed_source_alias_native_vram",
        "src": (20, 180),
        "dst": (21, 180),
        "size": (8, 1),
    }
    alias_source = address_for_byte(*case["src"], 0)
    return run_move_case(pe, case, entry_rva=A710_RVA, alias_source=alias_source)


def main():
    pe = PE(ARGS.exe)
    if sha256(pe.data) != EXPECTED_EXE_SHA256:
        fail("pinned original HERCULES.EXE SHA-256 changed")
    if pe.image_base != EXE_BASE:
        fail(f"unexpected image base {pe.image_base:#x}")
    if getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        fail("unexpected relocation table in pinned original")

    results = []
    mapped_sections = None
    for case in copy_cases():
        result, mapped_sections = run_move_case(pe, case)
        results.append(result)

    notify_case = {
        "name": "enabled_notification_endpoint_after_distinct_copy",
        "src": (50, 160), "dst": (70, 170), "size": (5, 2),
    }
    result, mapped_sections = run_move_case(
        pe, notify_case, notify_gate=1, notify_endpoint=True
    )
    results.append(result)

    alias_case, mapped_sections = run_a710_alias(pe)
    results.append(alias_case)

    compact_mapped = [
        {
            "name": item["name"], "va": hex(item["va"]),
            "mapped_size": hex(item["size"]),
            "raw_size": hex(item.get("raw_size", 0)),
            "permissions": hex(item.get("permissions", 0)),
        }
        for item in mapped_sections
    ]
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_EQUIVALENT",
        "scope": "Finite original A8F0 low-y VRAM rectangle moves with signed-WORD descriptors, overlap/page-crossing/empty cases, and one original A710 same-map source alias. No full domain or storage ownership claim.",
        "runner": {
            "path": str(Path(__file__).resolve()),
            "sha256": RUNNER_SHA256,
            "public_oracle_common": {
                "path": str(PUBLIC_ORACLE_COMMON.resolve()),
                "sha256": PUBLIC_ORACLE_COMMON_SHA256,
            },
        },
        "prediction": "work/renderer_next/vram_move/prediction.json",
        "oracle": {
            "file": str(ARGS.exe), "sha256": sha256(pe.data),
            "image_base": hex(pe.image_base), "base_relocations": 0,
            "A8F0": {
                "rva": hex(A8F0_RVA), "end_exclusive": hex(A8F0_END_RVA),
                "size": A8F0_END_RVA - A8F0_RVA,
                "sha256": sha256(pe.read_rva(A8F0_RVA, A8F0_END_RVA - A8F0_RVA)),
            },
            "A710": {
                "rva": hex(A710_RVA), "end_exclusive": hex(A710_END_RVA),
                "size": A710_END_RVA - A710_RVA,
                "sha256": sha256(pe.read_rva(A710_RVA, A710_END_RVA - A710_RVA)),
            },
            "BD80": {
                "rva": hex(BD80_RVA), "end_exclusive": hex(BD80_END_RVA),
                "size": BD80_END_RVA - BD80_RVA,
                "sha256": sha256(pe.read_rva(BD80_RVA, BD80_END_RVA - BD80_RVA)),
            },
        },
        "execution": {
            "engine": "Unicorn CPU emulator",
            "unicorn_version": unicorn.__version__,
            "architecture": "x86 32-bit",
            "original_sections_mapped_at_image_base": True,
            "instruction_budget_per_case": MAX_INSTRUCTIONS,
            "sentinel_return": hex(RETURN_SENTINEL),
            "DF_clear": True,
            "unknown_original_code_or_memory_endpoint_fails": True,
            "notification": {
                "disabled_fixture": "callback pointer nonzero, gate zero; original BD80 gate returns and the call fully unwinds",
                "enabled_fixture": "callback pointer and gate nonzero; original BD80 jump reaches an empty synthetic executable endpoint hook, captures original entry state, then emulator stops before executing any callback byte",
                "callback_code_fabricated_or_executed": False,
            },
            "data_restore_per_case": "entire mapped .data image restored to pre-fixture bytes and compared byte-for-byte",
            "original_code_restore_per_case": "entire mapped .text section compared byte-for-byte",
        },
        "address_model": {
            "bounded_domain": "0<=x<1024 words; 0<=y<512 rows; nonnegative width/height; rectangles remain within bounds",
            "row": "(y<256 ? y : 0x800+(y&0xff))",
            "byte_address": "0x96e6e0 + row*0x100 + (((2*x+byte_in_word)&0x700)<<8) + ((2*x+byte_in_word)&0xff)",
            "width_unit": "16-bit words; 2*width bytes per row",
            "allocation_extent_proven": False,
        },
        "cases": results,
        "limitations": [
            "Finite synthetic fixtures only; negative coordinates, clipping/invalid extents, y>=512, and full callback domain are unproved.",
            "The tested address formula is not an allocation-ownership or 2MiB backing claim.",
            "A8F0's forward overlap result is specific to tested row and x-page cases; no universal overlap contract beyond those cases is claimed.",
            "The enabled notification case stops at the original callback endpoint without executing callback code; only gate-zero fixtures fully return through BD80.",
            "No DirectDraw, raster output, gameplay, FUNCTION_MATCH, source identity, or whole-module equality is claimed.",
        ],
    }
    receipt["execution"].update(verify_runtime(unicorn, ARGS.deps))
    receipt["invocation"] = {
        "exe": str(ARGS.exe), "original_root": str(ARGS.original_root),
        "unicorn_deps": str(ARGS.deps), "analysis_deps": str(ARGS.analysis_deps),
    }
    write_receipt(ARGS.receipt, receipt)
    print(json.dumps({
        "status": receipt["status"],
        "exe_sha256": receipt["oracle"]["sha256"],
        "a8f0_sha256": receipt["oracle"]["A8F0"]["sha256"],
        "cases": [
            {
                "name": case["name"], "entry": case["entry_rva"],
                "src": case["source_xy"], "dst": case["destination_xy"],
                "size": case["width_height_words"],
                "writes": case["output_byte_count"], "stop": case["stop"],
                "BD80": case["bd80_trace_rvas"],
            }
            for case in results
        ],
        "receipt": str(ARGS.receipt),
    }, indent=2))


if __name__ == "__main__":
    main()
