#!/usr/bin/env python3
"""Run a finite, gated mixed packet chain in unchanged original AC60 code."""
from __future__ import annotations

import hashlib
import struct
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))
from oracle_common import parse_args, verify_runtime, write_receipt  # noqa: E402

ARGS = parse_args(
    "Run original AC60 with the native byte suppression flag set",
    "ac60_gate_report.json",
)
from pe import PE  # noqa: E402
import unicorn  # noqa: E402
from unicorn import (  # noqa: E402
    Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_INVALID,
    UC_HOOK_MEM_WRITE, UC_MODE_32, UC_PROT_EXEC, UC_PROT_READ, UC_PROT_WRITE,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EFLAGS, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP,
)

PAGE = 0x1000
EXE_BASE = 0x00400000
AC60_RVA = 0xAC60
AC60_END_RVA = 0xB2F0
BD80_RVA = 0xBD80
BD80_END_RVA = 0xBD96
MOVE_TARGET = EXE_BASE + 0xA8F0
TEXTURE_BASE = 0x0096E6E0
WORKSPACE = 0x00A90900
STATE_CLUT = 0x00A8E6F4
STATE_PAGE = 0x00A8E6F8
STATE_SEMI = 0x00A8E6FC
CALLBACK_CELL = 0x00A8E780
NOTIFICATION_GATE = 0x00A8E788
SUPPRESSION_BYTE = 0x00BA01FC
INPUT_BASE = 0x10000000
STACK_BASE = 0x20000000
STACK_SIZE = 0x10000
RETURN_SENTINEL = 0x30000000
UNKNOWN_CALLBACK = 0x31000000
MAX_INSTRUCTIONS = 250_000

ALLOWED_PC_RANGES = [
    (EXE_BASE + AC60_RVA, EXE_BASE + AC60_END_RVA, "AC60"),
    (EXE_BASE + 0x6FF0, EXE_BASE + 0x70B7, "original_6FF0_safe_path"),
    (EXE_BASE + 0x70C0, EXE_BASE + 0x7136, "original_70C0_safe_path"),
    (EXE_BASE + 0x7140, EXE_BASE + 0x716E, "original_7140_safe_path"),
    (EXE_BASE + 0x6620, EXE_BASE + 0x66F5, "original_6620_no_lock_path"),
    (EXE_BASE + BD80_RVA, EXE_BASE + BD80_END_RVA, "original_BD80_gate"),
]
RASTER_TARGETS = {EXE_BASE + 0xD420, EXE_BASE + 0xE680,
                  EXE_BASE + 0xCAB0, EXE_BASE + 0xC8F0}
STATE_CELLS = [
    WORKSPACE + offset for offset in range(0xB0, 0x108, 4)
] + [WORKSPACE + 0x10C, WORKSPACE + 0x110,
     STATE_CLUT, STATE_PAGE, STATE_SEMI]
GATE_BRANCHES = {
    EXE_BASE + 0xACD7: "polygon_0x20",
    EXE_BASE + 0xB05F: "group_0x60",
}


def fail(message: str) -> None:
    raise AssertionError(message)


def align_up(value: int) -> int:
    return (value + PAGE - 1) & ~(PAGE - 1)


def pe_permissions(characteristics: int) -> int:
    result = 0
    if characteristics & 0x40000000:
        result |= UC_PROT_READ
    if characteristics & 0x80000000:
        result |= UC_PROT_WRITE
    if characteristics & 0x20000000:
        result |= UC_PROT_EXEC
    return result


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
        mapped.append({"name": section["name"], "va": va, "size": mapped_size})
    return mapped


def pack_packet(tag: int, size: int, link: int, fields: dict[int, bytes]) -> bytes:
    packet = bytearray([0xA5] * size)
    packet[0:4] = struct.pack("<I", link)
    packet[7] = tag
    for offset, value in fields.items():
        packet[offset:offset + len(value)] = value
    return bytes(packet)


def s16(value: int) -> bytes:
    return struct.pack("<h", value)


def u16(value: int) -> bytes:
    return struct.pack("<H", value)


def fixture_packets():
    addresses = {
        "ft3": INPUT_BASE + 0x1000,
        "ft4": INPUT_BASE + 0x1040,
        "gt4": INPUT_BASE + 0x1080,
        "e1": INPUT_BASE + 0x10C0,
        "ignored68": INPUT_BASE + 0x10D0,
        "f3": INPUT_BASE + 0x10E0,
        "move": INPUT_BASE + 0x1110,
        "sprite64": INPUT_BASE + 0x1130,
        "sprite74": INPUT_BASE + 0x1160,
        "sprite7c": INPUT_BASE + 0x1180,
        "ot_gate": INPUT_BASE + 0x11A0,
    }
    records = {
        "ft3": pack_packet(0x26, 0x20, addresses["ft4"], {
            0x04: bytes((0x31, 0x42, 0x53)), 0x08: s16(-101), 0x0A: s16(202),
            0x0C: bytes((17, 29)), 0x0E: u16(0x1234), 0x10: s16(303),
            0x12: s16(-404), 0x14: bytes((43, 71)), 0x16: u16(0x004B),
            0x18: s16(505), 0x1A: s16(606), 0x1C: bytes((101, 203)),
        }),
        "ft4": pack_packet(0x2C, 0x28, addresses["gt4"], {
            0x04: bytes((0x64, 0x75, 0x86)), 0x08: s16(-111), 0x0A: s16(212),
            0x0C: bytes((19, 31)), 0x0E: u16(0x5678), 0x10: s16(515),
            0x12: s16(-616), 0x14: bytes((47, 73)), 0x16: u16(0x006D),
            0x18: s16(717), 0x1A: s16(-818), 0x1C: bytes((103, 205)),
            0x20: s16(-919), 0x22: s16(1018), 0x24: bytes((109, 211)),
        }),
        "gt4": pack_packet(0x3E, 0x34, addresses["e1"], {
            0x04: bytes((0x97, 0xA8, 0xB9)), 0x08: s16(-121), 0x0A: s16(222),
            0x0C: bytes((23, 37)), 0x0E: u16(0x9ABC), 0x10: bytes((0xC1, 0xC2, 0xC3)),
            0x14: s16(525), 0x16: s16(-626), 0x18: bytes((53, 79)), 0x1A: u16(0x01BB),
            0x1C: bytes((0xC4, 0xC5, 0xC6)), 0x20: s16(737), 0x22: s16(-838),
            0x24: bytes((107, 213)), 0x28: bytes((0xC7, 0xC8, 0xC9)),
            0x2C: s16(-949), 0x2E: s16(1050), 0x30: bytes((113, 217)),
        }),
        "e1": pack_packet(0xE1, 8, addresses["ignored68"],
                          {0x04: struct.pack("<I", 0xE100017B)}),
        "ignored68": pack_packet(0x68, 8, addresses["f3"], {}),
        "f3": pack_packet(0xF3, 0x0C, addresses["move"], {
            0x04: s16(-7), 0x06: bytes((0xF5,)), 0x08: s16(20), 0x0A: s16(300),
        }),
        "move": pack_packet(0x01, 0x18, addresses["sprite64"], {
            0x0C: s16(-15), 0x0E: s16(240), 0x10: s16(300), 0x12: s16(244),
            0x14: s16(17), 0x16: s16(9),
        }),
        "sprite64": pack_packet(0x64, 0x18, addresses["sprite74"], {
            0x04: bytes((0xC1, 0xD2, 0xE3)), 0x08: s16(-131), 0x0A: s16(232),
            0x0C: bytes((127, 229)), 0x0E: u16(13), 0x10: u16(13), 0x12: u16(27),
        }),
        "sprite74": pack_packet(0x74, 0x10, addresses["sprite7c"], {
            0x08: s16(-231), 0x0A: s16(332), 0x0C: bytes((137, 239)),
        }),
        "sprite7c": pack_packet(0x7C, 0x10, addresses["ot_gate"], {
            0x08: s16(331), 0x0A: s16(-432), 0x0C: bytes((147, 249)),
        }),
        "ot_gate": pack_packet(0x40, 8, 0, {}),
    }
    return addresses, records


def read_u32(uc: Uc, address: int) -> int:
    return struct.unpack("<I", bytes(uc.mem_read(address, 4)))[0]


def signed32(uc: Uc, address: int) -> int:
    value = read_u32(uc, address)
    return value - (1 << 32) if value & 0x80000000 else value


def snapshot_state(uc: Uc) -> dict[str, str]:
    result = {}
    for address in STATE_CELLS:
        size = 2 if address in (WORKSPACE + 0x10C, WORKSPACE + 0x110) else 4
        result[hex(address)] = bytes(uc.mem_read(address, size)).hex()
    return result


def run_fixture(pe: PE):
    if pe.image_base != EXE_BASE:
        fail(f"unexpected image base {pe.image_base:#x}")
    if getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        fail("fixed-base original unexpectedly has base relocations")
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    mapped = map_original_sections(uc, pe)
    uc.mem_map(INPUT_BASE, 0x4000, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(RETURN_SENTINEL, PAGE, UC_PROT_READ | UC_PROT_EXEC)

    data_section = next(row for row in pe.sections if row["name"] == ".data")
    data_va = EXE_BASE + data_section["virtual_address"]
    data_size = align_up(max(data_section["virtual_size"], data_section["raw_size"]))
    before_data = bytes(uc.mem_read(data_va, data_size))
    code_spans = [(EXE_BASE + AC60_RVA, AC60_END_RVA - AC60_RVA),
                  (EXE_BASE + BD80_RVA, BD80_END_RVA - BD80_RVA)]
    before_code = {va: bytes(uc.mem_read(va, size)) for va, size in code_spans}
    addresses, records = fixture_packets()
    ot_va = INPUT_BASE + 0x100
    uc.mem_write(ot_va, struct.pack("<I", addresses["ft3"]))
    for name, address in addresses.items():
        uc.mem_write(address, records[name])

    # Upper bytes are deliberate canaries. AC60 consumes only BA01FC as a byte.
    fixture_cells = {
        SUPPRESSION_BYTE: b"\x01\xA5\x5A\xC3",
        0x0096E698: b"\0", 0x00BA020C: b"\0", 0x0043A590: b"\0",
        0x0096E3D8: b"\0" * 4,
        CALLBACK_CELL: struct.pack("<I", UNKNOWN_CALLBACK),
        NOTIFICATION_GATE: b"\0" * 4,
    }
    old_fixture_cells = {
        va: bytes(uc.mem_read(va, len(value))) for va, value in fixture_cells.items()
    }
    for va, value in fixture_cells.items():
        uc.mem_write(va, value)

    sp = STACK_BASE + STACK_SIZE - 0x100
    uc.mem_write(sp, struct.pack("<II", RETURN_SENTINEL, ot_va))
    uc.reg_write(UC_X86_REG_ESP, sp)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
    uc.reg_write(UC_X86_REG_EIP, EXE_BASE + AC60_RVA)

    text_section = next(row for row in pe.sections if row["name"] == ".text")
    text_start = EXE_BASE + text_section["virtual_address"]
    text_end = text_start + align_up(max(text_section["virtual_size"], text_section["raw_size"]))
    scratch_ranges = [(INPUT_BASE, INPUT_BASE + 0x4000), (STACK_BASE, STACK_BASE + STACK_SIZE)]
    state = {"kind": None, "detail": None, "instructions": 0, "pending_gate": None}
    trace = []
    writes = []
    gate_boundaries = []
    ot_gate_packets = []
    move_boundary = None

    def on_code(machine, address, _size, _user):
        nonlocal move_boundary
        state["instructions"] += 1
        if state["instructions"] > MAX_INSTRUCTIONS:
            state.update(kind="instruction_budget", detail=state["instructions"])
            machine.emu_stop()
            return
        if address == RETURN_SENTINEL:
            state.update(kind="return_sentinel", detail=None)
            machine.emu_stop()
            return
        if address in GATE_BRANCHES:
            packet_va = machine.reg_read(UC_X86_REG_ESI)
            tag = bytes(machine.mem_read(packet_va + 7, 1))[0]
            eflags = machine.reg_read(UC_X86_REG_EFLAGS)
            gate_value = bytes(machine.mem_read(SUPPRESSION_BYTE, 1))[0]
            if gate_value != 1 or (eflags & 0x40):
                state.update(kind="gate_branch_not_taken", detail={
                    "pc_rva": hex(address - EXE_BASE), "tag": hex(tag),
                    "gate_byte": gate_value, "eflags": hex(eflags),
                })
                machine.emu_stop()
                return
            before = snapshot_state(machine)
            state["pending_gate"] = {
                "branch_rva": hex(address - EXE_BASE),
                "group": GATE_BRANCHES[address], "packet_va": packet_va,
                "tag": hex(tag), "gate_byte": gate_value,
                "jne_taken_from_test_flags": True, "before": before,
            }
        elif address == EXE_BASE + 0xACA2 and state["pending_gate"] is not None:
            item = state["pending_gate"]
            state["pending_gate"] = None
            item["after"] = snapshot_state(machine)
            item["state_unchanged_across_branch"] = item["before"] == item["after"]
            gate_boundaries.append(item)
        if address == EXE_BASE + 0xB176:
            packet_va = machine.reg_read(UC_X86_REG_ESI)
            ot_gate_packets.append({
                "packet_va": hex(packet_va),
                "tag": hex(bytes(machine.mem_read(packet_va + 7, 1))[0]),
            })
        if address in RASTER_TARGETS:
            state.update(kind="unexpected_raster_entry", detail=hex(address - EXE_BASE))
            machine.emu_stop()
            return
        if address == MOVE_TARGET:
            esp = machine.reg_read(UC_X86_REG_ESP)
            packet_va = machine.reg_read(UC_X86_REG_ESI)
            return_address = read_u32(machine, esp)
            rect_pointer = read_u32(machine, esp + 4)
            destination = [signed32(machine, esp + 8), signed32(machine, esp + 12)]
            rect = list(struct.unpack("<4h", bytes(machine.mem_read(rect_pointer, 8))))
            if (packet_va != addresses["move"] or return_address - EXE_BASE != 0xB1EC or
                    rect_pointer != addresses["move"] + 0x10 or destination != [-15, 240] or
                    rect != [300, 244, 17, 9]):
                state.update(kind="command01_call_mismatch", detail={
                    "packet": hex(packet_va), "return_rva": hex(return_address - EXE_BASE),
                    "rect_pointer": hex(rect_pointer), "destination": destination, "rect": rect,
                })
                machine.emu_stop()
                return
            move_boundary = {
                "entry_rva": hex(address - EXE_BASE),
                "return_rva": hex(return_address - EXE_BASE),
                "packet": hex(packet_va), "embedded_rect_pointer": hex(rect_pointer),
                "destination_signed": destination, "rectangle_signed_words": rect,
            }
            state.update(kind="move_boundary", detail={"return_address": return_address})
            machine.emu_stop()
            return
        if not any(start <= address < end for start, end, _name in ALLOWED_PC_RANGES):
            state.update(kind="unexpected_code", detail=hex(address))
            machine.emu_stop()
            return
        trace.append(address)

    def on_write(machine, _access, address, size, value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP)
        if text_start <= address and address + size <= text_end:
            state.update(kind="write_to_original_code", detail={
                "pc": hex(pc), "address": hex(address), "size": size,
            })
            machine.emu_stop()
            return
        if data_va <= address and address + size <= data_va + data_size:
            writes.append({
                "pc_rva": hex(pc - EXE_BASE), "address": hex(address), "size": size,
                "value": hex(value & ((1 << (size * 8)) - 1)),
                "packet_va": hex(machine.reg_read(UC_X86_REG_ESI)),
            })
            return
        if any(start <= address and address + size <= end for start, end in scratch_ranges):
            return
        state.update(kind="unexpected_write", detail={
            "pc": hex(pc), "address": hex(address), "size": size,
        })
        machine.emu_stop()

    def on_invalid(machine, access, address, size, _value, _user):
        state.update(kind="invalid_memory", detail={
            "pc": hex(machine.reg_read(UC_X86_REG_EIP)), "access": access,
            "address": hex(address), "size": size,
        })
        machine.emu_stop()
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
    uc.hook_add(UC_HOOK_MEM_INVALID, on_invalid)
    while True:
        state.update(kind=None, detail=None)
        try:
            uc.emu_start(uc.reg_read(UC_X86_REG_EIP), 0, timeout=3_000_000)
        except UcError as error:
            if state["kind"] is None:
                state.update(kind="unicorn_error", detail=str(error))
        if state["kind"] == "move_boundary":
            esp = uc.reg_read(UC_X86_REG_ESP)
            uc.reg_write(UC_X86_REG_ESP, esp + 4)
            uc.reg_write(UC_X86_REG_EIP, state["detail"]["return_address"])
            continue
        if state["kind"] == "return_sentinel":
            break
        fail(f"gated original AC60 stopped unexpectedly: {state}")

    expected_gates = [
        ("polygon_0x20", "0x26"), ("polygon_0x20", "0x2c"),
        ("polygon_0x20", "0x3e"), ("group_0x60", "0x68"),
        ("group_0x60", "0x64"), ("group_0x60", "0x74"),
        ("group_0x60", "0x7c"),
    ]
    observed_gates = [(row["group"], row["tag"]) for row in gate_boundaries]
    if observed_gates != expected_gates:
        fail(f"unexpected native gate sequence: {observed_gates}")
    if any(not row["state_unchanged_across_branch"] for row in gate_boundaries):
        fail("workspace/page state changed across a native gated branch")
    if move_boundary is None:
        fail("command 01 did not continue through the gate");
    if [(row["packet_va"], row["tag"]) for row in ot_gate_packets] != [
        (hex(addresses["e1"]), "0xe1"),
        (hex(addresses["f3"]), "0xf3"),
        (hex(addresses["move"]), "0x1"),
        (hex(addresses["ot_gate"]), "0x40"),
    ]:
        fail(f"E1/F3/command 01/OT did not continue through group route: {ot_gate_packets}")

    packet_names_by_va = {hex(value): name for name, value in addresses.items()}
    gated_names = {"ft3", "ft4", "gt4", "ignored68", "sprite64", "sprite74", "sprite7c"}
    gated_vas = {hex(addresses[name]) for name in gated_names}
    gated_writes = [event for event in writes if event["packet_va"] in gated_vas]
    if gated_writes:
        fail(f"gated packet emitted original data writes: {gated_writes}")
    if bytes(uc.mem_read(SUPPRESSION_BYTE, 4)) != b"\x01\xA5\x5A\xC3":
        fail("native AC60 changed the four-byte gate fixture/canary")
    if read_u32(uc, STATE_CLUT) != 0x1B or read_u32(uc, STATE_PAGE) != 3 or read_u32(uc, STATE_SEMI) != 0:
        fail("E1 state update did not continue with byte gate set")
    if bytes(uc.mem_read(NOTIFICATION_GATE, 4)) != b"\0" * 4:
        fail("BD80 notification gate was not disabled")
    if trace.count(EXE_BASE + BD80_RVA) != 1 or EXE_BASE + 0xBD93 in trace:
        fail("original BD80 gate did not execute exactly once without callback jump")
    for va, before in before_code.items():
        if bytes(uc.mem_read(va, len(before))) != before:
            fail(f"original code bytes changed at {va:#x}")

    after_data = bytes(uc.mem_read(data_va, data_size))
    final_state_before_restore = snapshot_state(uc)
    changed = {data_va + i for i, (a, b) in enumerate(zip(before_data, after_data)) if a != b}
    allowed = {address for address in STATE_CELLS}
    allowed.update(range(0x0043A488, 0x0043A490))
    allowed.update(range(0x00BA0210, 0x00BA0218))
    allowed.add(0x00BA01F0)
    for va, value in fixture_cells.items():
        allowed.update(range(va, va + len(value)))
    if not changed <= allowed:
        extras = sorted(changed - allowed)
        fail(f"original writes escaped exact state/fixture set: {[hex(a) for a in extras[:16]]}")

    # Restore every changed image byte and fixture setup cell exactly.
    for address in changed:
        uc.mem_write(address, before_data[address - data_va:address - data_va + 1])
    for va, before in old_fixture_cells.items():
        uc.mem_write(va, before)
    if bytes(uc.mem_read(data_va, data_size)) != before_data:
        fail("private emulator .data did not restore byte-for-byte")

    # Static byte slices and runtime branch outcomes jointly establish byte width.
    gate_slices = {
        "top_AC60_mov_al_byte": pe.read_rva(0xAC60, 5).hex(),
        "polygon_ACCF_mov_cl_byte": pe.read_rva(0xACCF, 6).hex(),
        "sprite_B057_mov_cl_byte": pe.read_rva(0xB057, 6).hex(),
    }
    return {
        "gate_byte": hex(fixture_cells[SUPPRESSION_BYTE][0]),
        "gate_upper_canary": fixture_cells[SUPPRESSION_BYTE][1:].hex(),
        "native_gate_load_slices": gate_slices,
        "native_gate_branch_boundaries": gate_boundaries,
        "native_gate_branch_vectors": observed_gates,
        "gated_packets_with_data_writes": gated_writes,
        "command01_boundary": move_boundary,
        "non_raster_state_writes": [event for event in writes
                                     if event["packet_va"] in {
                                         hex(addresses["e1"]), hex(addresses["f3"])}],
        "ot_gate_reached": ot_gate_packets,
        "instruction_count": state["instructions"],
        "original_code_bytes_unchanged": True,
        "unknown_or_os_endpoint_executed": False,
        "raster_callees_executed": False,
        "bd80_call_count": trace.count(EXE_BASE + BD80_RVA),
        "private_data_state_restored": True,
        "mapped_original_sections": [
            {"name": row["name"], "va": hex(row["va"]), "size": hex(row["size"])}
            for row in mapped
        ],
        "packet_addresses": {name: hex(value) for name, value in addresses.items()},
        "packet_commands": {name: hex(record[7]) for name, record in records.items()},
        "observed_data_write_count": len(writes),
        "data_write_events": writes,
        "final_state_snapshot_before_restore": final_state_before_restore,
        "fixture_cell_bytes": {hex(va): value.hex() for va, value in fixture_cells.items()},
        "packet_name_by_va": packet_names_by_va,
    }


def main() -> int:
    pe = PE(ARGS.exe)
    result = run_fixture(pe)
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_EQUIVALENT",
        "scope": (
            "Finite unchanged-original AC60 mixed-chain gate test for FT3/FT4/GT4, "
            "opaque 0x68, variable/fixed sprites, E1, F3, command 01, and OT 0x40. "
            "Raster and A8F0 callees are skipped at actual call boundaries; no pixels claimed."
        ),
        "oracle": {
            "path": str(ARGS.exe), "sha256": hashlib.sha256(ARGS.exe.read_bytes()).hexdigest(),
            "image_base": hex(pe.image_base), "ac60_span_sha256": hashlib.sha256(
                pe.read_rva(AC60_RVA, AC60_END_RVA - AC60_RVA)).hexdigest(),
            "ac60_rva": hex(AC60_RVA), "ac60_span_size": AC60_END_RVA - AC60_RVA,
            "base_relocations": 0,
        },
        "execution": {
            **verify_runtime(unicorn, ARGS.deps), "architecture": "x86 32-bit",
            "instructions_bounded": MAX_INSTRUCTIONS,
        },
        "result": result,
        "limitations": [
            "Only these finite packet records and the observed AC60 gate branches are covered.",
            "The byte gate is proved from original load instruction slices and branch outcomes; no full renderer or device path is run.",
            "No pixel, VRAM mutation, gameplay, source identity, or whole-module claim is made.",
        ],
    }
    write_receipt(ARGS.receipt, receipt)
    print(f"BOUNDED_EQUIVALENT;gate_branches={len(result['native_gate_branch_vectors'])};"
          f"gated_writes={len(result['gated_packets_with_data_writes'])};receipt={ARGS.receipt}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
