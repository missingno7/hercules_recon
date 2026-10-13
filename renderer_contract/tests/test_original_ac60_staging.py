"""Bounded Unicorn execution of original AC60 up to real raster call entries."""
from __future__ import annotations

import json
import struct
from pathlib import Path

from oracle_common import parse_args, verify_runtime, write_receipt

ARGS = parse_args(
    "Run finite original HERCULES.EXE AC60 packet-staging fixtures",
    "ac60_report.json",
)

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
    UC_X86_REG_ESI,
    UC_X86_REG_ESP,
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
INPUT_BASE = 0x10000000
STACK_BASE = 0x20000000
STACK_SIZE = 0x10000
RETURN_SENTINEL = 0x30000000
UNKNOWN_CALLBACK = 0x31000000
MAX_INSTRUCTIONS = 250_000

# AC60 plus only its no-device/no-draw original helper paths and final BD80 gate.
ALLOWED_PC_RANGES = [
    (EXE_BASE + AC60_RVA, EXE_BASE + AC60_END_RVA, "AC60"),
    (EXE_BASE + 0x6FF0, EXE_BASE + 0x70B7, "original_6FF0_safe_path"),
    (EXE_BASE + 0x70C0, EXE_BASE + 0x7136, "original_70C0_safe_path"),
    (EXE_BASE + 0x7140, EXE_BASE + 0x716E, "original_7140_safe_path"),
    (EXE_BASE + 0x6620, EXE_BASE + 0x66F5, "original_6620_no_lock_path"),
    (EXE_BASE + BD80_RVA, EXE_BASE + BD80_END_RVA, "original_BD80_gate"),
]
RASTER_TARGETS = {
    EXE_BASE + 0xD420: {"name": "FT4_D420", "return_rvas": {0xB049}},
    EXE_BASE + 0xE680: {"name": "polygon_E680", "return_rvas": {0xB03F}},
    EXE_BASE + 0xCAB0: {"name": "sprite_CAB0", "return_rvas": {0xB171}},
    EXE_BASE + 0xC8F0: {"name": "sprite_C8F0", "return_rvas": {0xB167}},
}
WORKSPACE_OFFSETS = [
    0xB0, 0xB4, 0xB8, 0xBC, 0xC0, 0xC4, 0xC8, 0xCC, 0xD0, 0xD4,
    0xD8, 0xDC, 0xE0, 0xE4, 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xFC,
    0x100, 0x104, 0x10C, 0x110,
]


def fail(message: str) -> None:
    raise AssertionError(message)


def align_up(value: int, alignment: int = PAGE) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


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
        mapped.append({"name": section["name"], "va": va, "size": mapped_size})
    return mapped


def pack_packet(tag: int, size: int, link: int, fields: dict[int, bytes]) -> bytes:
    data = bytearray([0xA5] * size)
    data[0:4] = struct.pack("<I", link)
    data[7] = tag
    for offset, value in fields.items():
        data[offset:offset + len(value)] = value
    return bytes(data)


def put_s16(value: int) -> bytes:
    return struct.pack("<h", value)


def put_u16(value: int) -> bytes:
    return struct.pack("<H", value)


def fixture_packets():
    # Distinct signed positions and UV bytes make sign extension and the FT3
    # F4 overlap visible. Packet extents stay above AC60's direct access minima.
    addresses = {
        "ft3": INPUT_BASE + 0x1000,
        "ft4": INPUT_BASE + 0x1040,
        "gt4": INPUT_BASE + 0x1080,
        "e1": INPUT_BASE + 0x10C0,
        "f3": INPUT_BASE + 0x10D0,
        "move": INPUT_BASE + 0x1100,
        "sprite": INPUT_BASE + 0x1120,
        "ot_gate": INPUT_BASE + 0x1160,
    }
    records = {
        "ft3": pack_packet(0x26, 0x20, addresses["ft4"], {
            0x04: bytes((0x31, 0x42, 0x53)),
            0x08: put_s16(-101), 0x0A: put_s16(202),
            0x0C: bytes((17, 29)), 0x0E: put_u16(0x1234),
            0x10: put_s16(303), 0x12: put_s16(-404),
            0x14: bytes((43, 71)), 0x16: put_u16(0x004B),
            0x18: put_s16(505), 0x1A: put_s16(606), 0x1C: bytes((101, 203)),
        }),
        "ft4": pack_packet(0x2C, 0x28, addresses["gt4"], {
            0x04: bytes((0x64, 0x75, 0x86)),
            0x08: put_s16(-111), 0x0A: put_s16(212),
            0x0C: bytes((19, 31)), 0x0E: put_u16(0x5678),
            0x10: put_s16(515), 0x12: put_s16(-616),
            0x14: bytes((47, 73)), 0x16: put_u16(0x006D),
            0x18: put_s16(717), 0x1A: put_s16(-818),
            0x1C: bytes((103, 205)), 0x20: put_s16(-919), 0x22: put_s16(1018),
            0x24: bytes((109, 211)),
        }),
        "gt4": pack_packet(0x3E, 0x34, addresses["e1"], {
            0x04: bytes((0x97, 0xA8, 0xB9)),
            0x10: bytes((0xC1, 0xC2, 0xC3)),
            0x08: put_s16(-121), 0x0A: put_s16(222),
            0x0C: bytes((23, 37)), 0x0E: put_u16(0x9ABC),
            0x14: put_s16(525), 0x16: put_s16(-626),
            0x18: bytes((53, 79)), 0x1A: put_u16(0x01BB),
            0x1C: bytes((0xC4, 0xC5, 0xC6)),
            0x20: put_s16(737), 0x22: put_s16(-838),
            0x24: bytes((107, 213)), 0x2C: put_s16(-949), 0x2E: put_s16(1050),
            0x28: bytes((0xC7, 0xC8, 0xC9)), 0x30: bytes((113, 217)),
        }),
        "e1": pack_packet(0xE1, 0x08, addresses["f3"], {
            # +7 is both the tag and the high byte of AC60's DWORD read.
            0x04: struct.pack("<I", 0xE10001FB),
        }),
        "f3": pack_packet(0xF3, 0x0C, addresses["move"], {
            0x04: put_s16(-7), 0x06: bytes((0xF5,)),
            0x08: put_s16(20), 0x0A: put_s16(300),
        }),
        "move": pack_packet(0x01, 0x18, addresses["sprite"], {
            0x0C: put_s16(-15), 0x0E: put_s16(240),
            0x10: put_s16(300), 0x12: put_s16(244),
            0x14: put_s16(17), 0x16: put_s16(9),
        }),
        "sprite": pack_packet(0x64, 0x18, addresses["ot_gate"], {
            0x04: bytes((0xC1, 0xD2, 0xE3)),
            0x08: put_s16(-131), 0x0A: put_s16(232), 0x0C: bytes((127, 229)),
            0x10: put_u16(13), 0x12: put_u16(27),
        }),
        "ot_gate": pack_packet(0x40, 0x08, 0, {}),
    }
    return addresses, records


def expected_for_packet(name: str):
    # Each tuple is (workspace/global VA, write size, final value). Values are
    # the independent staging contract checked at the actual original call PC.
    if name == "ft3":
        return {
            "raster": "polygon_E680", "return_rva": 0xB03F,
            "fields": {
                0xB8: -101, 0xBC: 303, 0xC0: 505, 0xC4: 505,
                0xC8: 202, 0xCC: -404, 0xD0: 606, 0xD4: 606,
                0xD8: 17 << 16, 0xDC: 43 << 16, 0xE0: 101 << 16,
                0xE8: 29 << 16, 0xEC: 71 << 16, 0xF0: 203 << 16,
                0xF4: 101 << 16, 0xF8: 3, 0x100: 0x31, 0x104: 0x1234,
                0xFC: TEXTURE_BASE + ((0x4B & 0x1F) >> 1) * 0x10000,
            },
            "globals": {STATE_CLUT: 0x004B & 0x1F, STATE_PAGE: 2, STATE_SEMI: 0},
        }
    if name == "ft4":
        return {
            "raster": "FT4_D420", "return_rva": 0xB049,
            "fields": {
                0xB8: -111, 0xBC: 515, 0xC0: -919, 0xC4: 717,
                0xC8: 212, 0xCC: -616, 0xD0: 1018, 0xD4: -818,
                0xD8: 19 << 16, 0xDC: 47 << 16, 0xE0: 109 << 16,
                0xE4: 103 << 16, 0xE8: 31 << 16, 0xEC: 73 << 16,
                0xF0: 211 << 16, 0xF4: 205 << 16,
                0xF8: 4, 0x100: 0x64, 0x104: 0x5678,
                0xFC: TEXTURE_BASE + ((0x6D & 0x1F) >> 1) * 0x10000,
            },
            "globals": {STATE_CLUT: 0x6D & 0x1F, STATE_PAGE: 3, STATE_SEMI: 0},
        }
    if name == "gt4":
        return {
            "raster": "polygon_E680", "return_rva": 0xB03F,
            "fields": {
                0xB8: -121, 0xBC: 525, 0xC0: -949, 0xC4: 737,
                0xC8: 222, 0xCC: -626, 0xD0: 1050, 0xD4: -838,
                0xD8: 23 << 16, 0xDC: 53 << 16, 0xE0: 113 << 16,
                0xE4: 107 << 16, 0xE8: 37 << 16, 0xEC: 79 << 16,
                0xF0: 217 << 16, 0xF4: 213 << 16,
                0xF8: 4, 0x100: 0x97, 0x104: 0x9ABC,
                0xFC: 0x00A6E6E0,
            },
            "globals": {STATE_CLUT: 0xBB & 0x1F, STATE_PAGE: 1, STATE_SEMI: 1},
        }
    if name == "sprite":
        return {
            "raster": "sprite_CAB0", "return_rva": 0xB171,
            "fields": {
                0xB8: -131, 0xC8: 232, 0xD8: 127, 0xE8: 229,
                0xFC: 0x00A6E6E0,
                0x10C: 13, 0x110: 27,
            },
            "globals": {STATE_CLUT: 0x1B, STATE_PAGE: 3, STATE_SEMI: 1},
        }
    raise ValueError(name)


def dword(uc: Uc, address: int) -> int:
    return struct.unpack("<I", bytes(uc.mem_read(address, 4)))[0]


def signed32(uc: Uc, address: int) -> int:
    value = dword(uc, address)
    return value - (1 << 32) if value & 0x80000000 else value


def snapshot_workspace(uc: Uc):
    return {
        f"{offset:#x}": dword(uc, WORKSPACE + offset)
        for offset in WORKSPACE_OFFSETS
        if offset not in (0x10C, 0x110)
    } | {
        "0x10c_u16": struct.unpack("<H", bytes(uc.mem_read(WORKSPACE + 0x10C, 2)))[0],
        "0x110_u16": struct.unpack("<H", bytes(uc.mem_read(WORKSPACE + 0x110, 2)))[0],
    }


def run_fixture(pe: PE):
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    mapped = map_original_sections(uc, pe)
    uc.mem_map(INPUT_BASE, 0x4000, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(RETURN_SENTINEL, PAGE, UC_PROT_READ | UC_PROT_EXEC)

    data_section = next(section for section in pe.sections if section["name"] == ".data")
    data_va = EXE_BASE + data_section["virtual_address"]
    data_size = align_up(max(data_section["virtual_size"], data_section["raw_size"]))
    before_data = bytes(uc.mem_read(data_va, data_size))
    code_spans = [
        (EXE_BASE + AC60_RVA, AC60_END_RVA - AC60_RVA),
        (EXE_BASE + BD80_RVA, BD80_END_RVA - BD80_RVA),
    ]
    before_code = {va: bytes(uc.mem_read(va, size)) for va, size in code_spans}

    addresses, records = fixture_packets()
    ot_va = INPUT_BASE + 0x100
    uc.mem_write(ot_va, struct.pack("<I", addresses["ft3"]))
    for name, address in addresses.items():
        uc.mem_write(address, records[name])

    # Force the valid original startup helper's no-device path. No COM method
    # or OS import is synthesized. The final BD80 callback pointer is a trap,
    # but its original DWORD gate is zero.
    fixture_cells = {
        0x00BA01FC: b"\0",
        0x0096E698: b"\0",
        0x00BA020C: b"\0",
        0x0043A590: b"\0",
        0x0096E3D8: b"\0" * 4,
        CALLBACK_CELL: struct.pack("<I", UNKNOWN_CALLBACK),
        NOTIFICATION_GATE: b"\0" * 4,
    }
    old_fixture_cells = {va: bytes(uc.mem_read(va, len(data))) for va, data in fixture_cells.items()}
    for va, value in fixture_cells.items():
        uc.mem_write(va, value)

    sp = STACK_BASE + STACK_SIZE - 0x100
    uc.mem_write(sp, struct.pack("<II", RETURN_SENTINEL, ot_va))
    uc.reg_write(UC_X86_REG_ESP, sp)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
    uc.reg_write(UC_X86_REG_EIP, EXE_BASE + AC60_RVA)

    trace = []
    writes = []
    boundaries = []
    state_command_boundaries = []
    state = {"kind": None, "detail": None, "instructions": 0, "write_index": 0}
    data_end = data_va + data_size
    text_section = next(section for section in pe.sections if section["name"] == ".text")
    text_start = EXE_BASE + text_section["virtual_address"]
    text_end = text_start + align_up(max(text_section["virtual_size"], text_section["raw_size"]))
    scratch_ranges = [(INPUT_BASE, INPUT_BASE + 0x4000), (STACK_BASE, STACK_BASE + STACK_SIZE)]

    def on_code(machine, address, _size, _user):
        state["instructions"] += 1
        if state["instructions"] > MAX_INSTRUCTIONS:
            state["kind"] = "instruction_budget"
            machine.emu_stop()
            return
        if address == RETURN_SENTINEL:
            state["kind"] = "return_sentinel"
            machine.emu_stop()
            return
        raster = RASTER_TARGETS.get(address)
        if raster:
            esp = machine.reg_read(UC_X86_REG_ESP)
            return_address = dword(machine, esp)
            return_rva = return_address - EXE_BASE
            if return_rva not in raster["return_rvas"]:
                state["kind"] = "unexpected_raster_callsite"
                state["detail"] = {
                    "raster": raster["name"], "return_rva": hex(return_rva),
                }
                machine.emu_stop()
                return
            # Packet identity is linked-list order; the producer-facing packet
            # tag remains available in ESI at the real original call boundary.
            esi = machine.reg_read(UC_X86_REG_ESI)
            packet_name = next((name for name, packet in addresses.items()
                                if packet == esi), "unknown")
            exp = expected_for_packet(packet_name) if packet_name in {"ft3", "ft4", "gt4", "sprite"} else None
            if exp is None or exp["raster"] != raster["name"]:
                state["kind"] = "unexpected_packet_raster_pair"
                state["detail"] = {"packet": packet_name, "raster": raster["name"]}
                machine.emu_stop()
                return
            workspace = snapshot_workspace(machine)
            for offset, value in exp["fields"].items():
                if offset in (0x10C, 0x110):
                    actual = workspace[f"{offset:#x}_u16"]
                else:
                    actual = workspace[f"{offset:#x}"]
                if actual != (value & 0xFFFFFFFF):
                    state["kind"] = "staging_mismatch"
                    state["detail"] = {
                        "packet": packet_name,
                        "field": hex(offset),
                        "actual": hex(actual),
                        "expected": hex(value & 0xFFFFFFFF),
                    }
                    machine.emu_stop()
                    return
            globals_snapshot = {
                "clut": dword(machine, STATE_CLUT),
                "page": dword(machine, STATE_PAGE),
                "semi": dword(machine, STATE_SEMI),
            }
            wanted_globals = {
                "clut": exp["globals"][STATE_CLUT],
                "page": exp["globals"][STATE_PAGE],
                "semi": exp["globals"][STATE_SEMI],
            }
            if globals_snapshot != wanted_globals:
                state["kind"] = "texture_state_mismatch"
                state["detail"] = {"packet": packet_name, "actual": globals_snapshot,
                                    "expected": wanted_globals}
                machine.emu_stop()
                return
            write_start = state["write_index"]
            write_end = len(writes)
            boundaries.append({
                "packet": packet_name,
                "tag": hex(bytes(machine.mem_read(esi + 7, 1))[0]),
                "raster_entry_rva": hex(address - EXE_BASE),
                "callsite_return_rva": hex(return_rva),
                "instruction_count": state["instructions"],
                "workspace": workspace,
                "texture_state": globals_snapshot,
                "write_event_range": [write_start, write_end],
                "write_event_count_since_previous_boundary": write_end - write_start,
            })
            if packet_name == "gt4":
                boundaries[-1]["auxiliary_rgb_bytes"] = {
                    hex(offset): bytes(machine.mem_read(esi + offset, 3)).hex()
                    for offset in (0x10, 0x1C, 0x28)
                }
                staged_bytes = set()
                for event in writes[write_start:write_end]:
                    size = event["size"]
                    raw = int(event["value"], 16).to_bytes(size, "little")
                    staged_bytes.update(raw)
                ignored_values = {0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9}
                boundaries[-1]["auxiliary_rgb_values_absent_from_native_stores"] = not bool(
                    staged_bytes & ignored_values
                )
                if not boundaries[-1]["auxiliary_rgb_values_absent_from_native_stores"]:
                    state["kind"] = "gt4_auxiliary_rgb_was_staged"
                    state["detail"] = boundaries[-1]
                    machine.emu_stop()
                    return
            state["write_index"] = len(writes)
            state["kind"] = "raster_boundary"
            state["detail"] = {"return_address": return_address}
            machine.emu_stop()
            return
        if address == MOVE_TARGET:
            esp = machine.reg_read(UC_X86_REG_ESP)
            return_address = dword(machine, esp)
            packet_va = machine.reg_read(UC_X86_REG_ESI)
            rect_pointer = dword(machine, esp + 4)
            destination_x = signed32(machine, esp + 8)
            destination_y = signed32(machine, esp + 12)
            rect = struct.unpack("<4h", bytes(machine.mem_read(rect_pointer, 8)))
            expected_rect = (300, 244, 17, 9)
            expected_clips = {
                "left": dword(machine, WORKSPACE + 0xB0),
                "top": dword(machine, WORKSPACE + 0xB4),
                "right": dword(machine, 0x0043A488),
                "bottom": dword(machine, 0x0043A48C),
            }
            if (
                packet_va != addresses["move"]
                or return_address - EXE_BASE != 0xB1EC
                or rect_pointer != addresses["move"] + 0x10
                or (destination_x, destination_y) != (-15, 240)
                or rect != expected_rect
            ):
                state["kind"] = "command01_call_mismatch"
                state["detail"] = {
                    "packet": hex(packet_va), "return_rva": hex(return_address - EXE_BASE),
                    "rect_pointer": hex(rect_pointer), "destination": [destination_x, destination_y],
                    "rect": rect,
                }
                machine.emu_stop()
                return
            if expected_clips != {"left": 0, "top": 5, "right": 13, "bottom": 240}:
                state["kind"] = "f3_state_mismatch"
                state["detail"] = expected_clips
                machine.emu_stop()
                return
            state_command_boundaries.append({
                "packet": "move",
                "tag": hex(bytes(machine.mem_read(packet_va + 7, 1))[0]),
                "call_entry_rva": hex(address - EXE_BASE),
                "callsite_return_rva": hex(return_address - EXE_BASE),
                "packet_payload_pointer": hex(rect_pointer),
                "destination_signed_words": [destination_x, destination_y],
                "embedded_rect_signed_words": list(rect),
                "preceding_f3_clipped_state": expected_clips,
                "instruction_count": state["instructions"],
            })
            state["kind"] = "move_boundary"
            state["detail"] = {"return_address": return_address}
            machine.emu_stop()
            return
        if not any(start <= address < end for start, end, _name in ALLOWED_PC_RANGES):
            state["kind"] = "unexpected_code"
            state["detail"] = hex(address)
            machine.emu_stop()
            return
        trace.append(address)

    def on_write(machine, _access, address, size, value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP)
        if text_start <= address and address + size <= text_end:
            state["kind"] = "write_to_original_code"
            state["detail"] = {"pc": hex(pc), "address": hex(address), "size": size}
            machine.emu_stop()
            return
        if data_va <= address and address + size <= data_end:
            writes.append({
                "pc_rva": hex(pc - EXE_BASE),
                "address": hex(address),
                "size": size,
                "value": hex(value & ((1 << (size * 8)) - 1)),
            })
            return
        if any(start <= address and address + size <= end for start, end in scratch_ranges):
            return
        state["kind"] = "unexpected_write"
        state["detail"] = {"pc": hex(pc), "address": hex(address), "size": size}
        machine.emu_stop()

    def on_invalid(machine, access, address, size, _value, _user):
        state["kind"] = "invalid_memory"
        state["detail"] = {
            "pc": hex(machine.reg_read(UC_X86_REG_EIP)),
            "access": access, "address": hex(address), "size": size,
        }
        machine.emu_stop()
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
    uc.hook_add(UC_HOOK_MEM_INVALID, on_invalid)

    while True:
        state["kind"] = None
        state["detail"] = None
        try:
            uc.emu_start(uc.reg_read(UC_X86_REG_EIP), 0, timeout=3_000_000)
        except UcError as error:
            if state["kind"] is None:
                state["kind"] = "unicorn_error"
                state["detail"] = str(error)
        if state["kind"] in {"raster_boundary", "move_boundary"}:
            # Host callback models the target's RET without executing or
            # synthesizing any target instructions; the original caller then
            # resumes at its recorded return address.
            return_address = state["detail"]["return_address"]
            esp = uc.reg_read(UC_X86_REG_ESP)
            uc.reg_write(UC_X86_REG_ESP, esp + 4)
            uc.reg_write(UC_X86_REG_EIP, return_address)
            continue
        if state["kind"] == "return_sentinel":
            break
        fail(f"AC60 original execution stopped unexpectedly: {state}")

    expected_order = ["ft3", "ft4", "gt4", "sprite"]
    if [entry["packet"] for entry in boundaries] != expected_order:
        fail(f"unexpected real raster boundary order: {[b['packet'] for b in boundaries]}")
    if [entry["packet"] for entry in state_command_boundaries] != ["move"]:
        fail(f"unexpected command-01 boundary sequence: {state_command_boundaries}")

    e1_stores = [
        (event["pc_rva"], event["address"], event["size"], event["value"])
        for event in writes
        if event["pc_rva"] in {"0xb1b0", "0xb1bc", "0xb1ca"}
    ]
    expected_e1_stores = [
        ("0xb1b0", "0xa8e6fc", 4, "0x1"),
        ("0xb1bc", "0xa8e6f4", 4, "0x1b"),
        ("0xb1ca", "0xa8e6f8", 4, "0x3"),
    ]
    if e1_stores != expected_e1_stores:
        fail(f"original E1 state stores differ: {e1_stores}")
    if any(
        event["address"] == "0xa909fc"
        for event in writes
        if 0xB19C <= int(event["pc_rva"], 16) <= 0xB1CF
    ):
        fail("original E1 command unexpectedly wrote A909FC")

    f3_stores = [
        (event["pc_rva"], event["address"], event["size"], event["value"])
        for event in writes
        if 0xB203 <= int(event["pc_rva"], 16) <= 0xB2B3
    ]
    expected_f3_stores = [
        ("0xb203", "0xa909b0", 4, "0xfffffff9"),
        ("0xb218", "0xa909b4", 4, "0xf5"),
        ("0xb226", "0xa909b4", 4, "0x5"),
        ("0xb232", "0x43a488", 4, "0xd"),
        ("0xb23f", "0x43a48c", 4, "0x131"),
        ("0xb249", "0xa909b0", 4, "0x0"),
        ("0xb2b3", "0x43a48c", 4, "0xf0"),
    ]
    if f3_stores != expected_f3_stores:
        fail(f"original F3 state-store trace differs: {f3_stores}")
    if trace[0] != EXE_BASE + AC60_RVA:
        fail("fixture did not start at original AC60")
    if trace.count(EXE_BASE + BD80_RVA) != 1:
        fail("original AC60 did not reach gated BD80 exactly once")
    if EXE_BASE + 0xBD93 in trace:
        fail("disabled BD80 callback jump unexpectedly executed")
    if sum(1 for entry in boundaries if entry["raster_entry_rva"] == "0xe680") != 2:
        fail("expected FT3 and GT4 to reach original E680 call boundary")
    if sum(1 for entry in boundaries if entry["raster_entry_rva"] == "0xd420") != 1:
        fail("expected FT4 to reach original D420 call boundary")
    if sum(1 for entry in boundaries if entry["raster_entry_rva"] == "0xcab0") != 1:
        fail("expected variable sprite to reach original CAB0 call boundary")

    # Every original data write must have come from AC60 or its three safe
    # setup/maintenance helpers. Expected writes are checked at each boundary;
    # this allowed set excludes OS/COM, unrelated data, and any pixel function.
    allowed_write_rvas = set(range(0xAC60, 0xB2F0)) | set(range(0x6FF0, 0x70B7)) | set(range(0x70C0, 0x7136)) | set(range(0x7140, 0x716E)) | set(range(0x6620, 0x66F5)) | set(range(0xBD80, 0xBD96))
    for event in writes:
        if int(event["pc_rva"], 16) not in allowed_write_rvas:
            fail(f"write came from unapproved original PC: {event}")

    expected_changed = set()
    for entry in boundaries:
        # These fixtures deliberately make each expected workspace field
        # nonzero so a missing original store cannot be hidden by initial BSS.
        for offset in expected_for_packet(entry["packet"])["fields"]:
            size = 2 if offset in (0x10C, 0x110) else 4
            expected_changed.update(WORKSPACE + offset + index for index in range(size))
        for va in (STATE_CLUT, STATE_PAGE, STATE_SEMI):
            expected_changed.update(va + index for index in range(4))
    for va, data in fixture_cells.items():
        expected_changed.update(va + index for index in range(len(data)))
    expected_changed.update(range(0x00BA0210, 0x00BA0218))
    expected_changed.add(0x00BA01F0)
    expected_changed.update(range(WORKSPACE + 0xB0, WORKSPACE + 0xB8))
    expected_changed.update(range(0x0043A488, 0x0043A490))
    after_data = bytes(uc.mem_read(data_va, data_size))
    actual_changed = {
        data_va + index
        for index, (before, after) in enumerate(zip(before_data, after_data))
        if before != after
    }
    if not actual_changed <= expected_changed:
        extras = sorted(actual_changed - expected_changed)
        fail(f"original AC60/helper writes escaped fixture/staging fields: {[hex(a) for a in extras[:16]]}")

    for va, before in before_code.items():
        if bytes(uc.mem_read(va, len(before))) != before:
            fail(f"original code bytes changed at {va:#x}")
    if dword(uc, NOTIFICATION_GATE) != 0:
        fail("original BD80 notification gate did not remain disabled")

    # Restore all observed persistent changes (including temporary gate/setup
    # fixture cells) in the private Unicorn image and verify exact restoration.
    for address in actual_changed:
        uc.mem_write(address, before_data[address - data_va:address - data_va + 1])
    for va, before in old_fixture_cells.items():
        uc.mem_write(va, before)
    if bytes(uc.mem_read(data_va, data_size)) != before_data:
        fail("private emulator .data did not restore byte-for-byte")

    return {
        "packet_addresses": {name: hex(value) for name, value in addresses.items()},
        "ordering_table_pointer": hex(ot_va),
        "packet_chain_order": ["ft3", "ft4", "gt4", "e1", "f3", "move", "sprite", "ot_gate", "null"],
        "packet_sizes": {name: len(data) for name, data in records.items()},
        "raster_boundaries": boundaries,
        "state_command_boundaries": state_command_boundaries,
        "staging_observations": {
            "ft3_uv2": {
                "u2": 101, "v2": 203,
                "a909e0": boundaries[0]["workspace"]["0xe0"],
                "a909f0": boundaries[0]["workspace"]["0xf0"],
                "a909f4_final": boundaries[0]["workspace"]["0xf4"],
                "a909f4_uses_u2_after_original_overwrite": (
                    boundaries[0]["workspace"]["0xf4"] == (101 << 16)
                ),
            },
            "gt4_auxiliary_rgb_not_staged": {
                "source_bytes": boundaries[2]["auxiliary_rgb_bytes"],
                "native_store_bytes_exclude_distinctive_values": boundaries[2]["auxiliary_rgb_values_absent_from_native_stores"],
                "color0_red_staged": boundaries[2]["workspace"]["0x100"],
            },
            "e1_state_stores": [
                {"pc_rva": pc, "address": address, "size": size, "value": value}
                for pc, address, size, value in e1_stores
            ],
            "e1_does_not_write_a909fc": True,
            "f3_store_trace": [
                {"pc_rva": pc, "address": address, "size": size, "value": value}
                for pc, address, size, value in f3_stores
            ],
        },
        "instruction_trace_count": len(trace),
        "instruction_count_including_boundary_hooks": state["instructions"],
        "bd80_call_count": trace.count(EXE_BASE + BD80_RVA),
        "bd80_gate_rvas": [hex(pc - EXE_BASE) for pc in trace if EXE_BASE + BD80_RVA <= pc < EXE_BASE + BD80_END_RVA],
        "unknown_or_os_endpoint_executed": False,
        "raster_callees_executed": False,
        "raster_callees_skipped_by_host_return_callback": True,
        "only_original_data_writes_from_allowed_ranges": True,
        "original_code_bytes_unchanged": True,
        "data_writes_escaped_exact_fixture_staging_set": False,
        "private_data_state_restored": True,
        "mapped_original_sections": [
            {"name": s["name"], "va": hex(s["va"]), "size": hex(s["size"])}
            for s in mapped
        ],
        "fixture_cells": {hex(va): data.hex() for va, data in fixture_cells.items()},
        "temporary_setup_write_events": writes[:state["write_index"]],
        "all_original_data_write_events": writes,
    }


def main():
    pe = PE(ARGS.exe)
    if pe.image_base != EXE_BASE:
        fail(f"unexpected image base {pe.image_base:#x}")
    if getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        fail("fixed-base original unexpectedly has base relocations")
    result = run_fixture(pe)
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_EQUIVALENT",
        "scope": (
            "One finite synthetic AC60 ordering-table chain executed in unchanged original HERCULES.EXE code through FT3, FT4, GT4, E1, F3, command-01, variable-sprite, and 0x40 gate processing. Original staging is checked at four rasterizer entries and the A8F0 call boundary; host return callbacks skip callees. No raster pixels or full DirectDraw/device path are claimed."
        ),
        "oracle": {
            "path": str(ARGS.exe),
            "sha256": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
            "image_base": hex(pe.image_base),
            "ac60_rva": hex(AC60_RVA),
            "ac60_span_size": AC60_END_RVA - AC60_RVA,
            "ac60_span_sha256": __import__("hashlib").sha256(pe.read_rva(AC60_RVA, AC60_END_RVA - AC60_RVA)).hexdigest(),
            "bd80_rva": hex(BD80_RVA),
            "bd80_body_size": BD80_END_RVA - BD80_RVA,
            "bd80_body_sha256": __import__("hashlib").sha256(pe.read_rva(BD80_RVA, BD80_END_RVA - BD80_RVA)).hexdigest(),
            "base_relocations": 0,
        },
        "execution": {
            **verify_runtime(unicorn, ARGS.deps),
            "architecture": "x86 32-bit",
            "original_pe_sections_mapped_at_true_image_base": True,
            "instruction_budget": MAX_INSTRUCTIONS,
            "sentinel_return": hex(RETURN_SENTINEL),
            "original_code_modified": False,
            "fixture_data_only": True,
            "unknown_pc_memory_os_or_iat_endpoint_fails": True,
            "notifications": "original BD80 gate executes with nonzero trap callback pointer and zero gate; callback jump is not taken",
            "host_call_hooks": "At each original raster entry and A8F0 call entry, record staged values/arguments and native return address; emulator stops, then host advances ESP and resumes at that recorded return address. No raster or VRAM-move body or synthetic instruction executes.",
        },
        "result": result,
        "limitations": [
            "Single synthetic well-formed packet chain only; no packet producer, allocator extent, malformed OT, or full OT traversal claim.",
            "FT3/FT4/GT4 and variable-sprite staging are checked at real raster call boundaries; F3 state and command-01 arguments are checked at A8F0 entry. Fixed sprites and other packet groups remain untested.",
            "Rasterizer callees and A8F0 are intentionally skipped; no pixel/rasterizer or VRAM-move output equivalence is claimed.",
            "The no-device AC60 helper path is exercised; DirectDraw COM calls, OS imports, GPU/device behavior, and gameplay are excluded.",
            "No source identity, FUNCTION_MATCH, whole-module equality, or texture storage ownership is established.",
        ],
        "invocation": {
            "exe": str(ARGS.exe), "original_root": str(ARGS.original_root),
            "unicorn_deps": str(ARGS.deps), "analysis_deps": str(ARGS.analysis_deps),
        },
    }
    write_receipt(ARGS.receipt, receipt)
    print(json.dumps({
        "status": receipt["status"],
        "original_exe_sha256": receipt["oracle"]["sha256"],
        "unicorn_version": receipt["execution"]["unicorn_version"],
        "raster_boundaries": [
            {"packet": b["packet"], "tag": b["tag"], "raster_entry_rva": b["raster_entry_rva"], "callsite_return_rva": b["callsite_return_rva"]}
            for b in result["raster_boundaries"]
        ],
        "state_command_boundaries": result["state_command_boundaries"],
        "receipt": str(ARGS.receipt),
    }, indent=2))


if __name__ == "__main__":
    main()
