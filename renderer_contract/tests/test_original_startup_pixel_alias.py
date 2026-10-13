"""Execute original 71A0 then C700/C882 on one private Unicorn state."""
from __future__ import annotations

import collections
import hashlib
import json
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT_CANDIDATES = (HERE, *HERE.parents)
ROOT = next((path for path in ROOT_CANDIDATES
             if (path / "renderer_contract" / "tests" / "oracle_common.py").is_file()
             and (path / "scripts" / "pe.py").is_file()), None)
if ROOT is None:
    raise SystemExit("cannot locate ancestor repository with renderer_contract/tests and scripts/pe.py")
sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))
from oracle_common import parse_args, verify_runtime, write_receipt  # noqa: E402

ARGS = parse_args(
    "Run the unchanged original 71A0 builder followed by C700/C882 pixel slices",
    "startup_pixel_alias.json",
)
if not any(arg == "--receipt" or arg.startswith("--receipt=") for arg in sys.argv[1:]):
    raise SystemExit("--receipt is required and must remain inside <original-root>/work")

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
EXE_SHA = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
BUILDER_START, BUILDER_STOP = 0x71A0, 0x728D
PALETTE_HELPER_START, PALETTE_HELPER_SIZE = 0x6780, 0x87
PIXEL_START, PIXEL_STOP = 0xC7F6, 0xC89F
MAX_BUILDER_INSNS, MAX_PIXEL_INSNS = 20_000_000, 10_000

STACK, STACK_SIZE = 0x20000000, 0x10000
FRAME = STACK + 0x8000
SCRATCH, SCRATCH_SIZE = 0x10000000, 0x4000
TEXTURE, DEST = SCRATCH + 0x3000, SCRATCH + 0x3800
LOGPALETTE, BITMAPINFO = SCRATCH + 0x1000, SCRATCH + 0x2000

SHADE, SHADE_SIZE = 0x91C488, 0x10000
CLUT_RAMP, CLUT_RAMP_SIZE = 0x92C490, 0x2000
BLEND, BLEND_SIZE = 0x92E3D0, 0x40000
MODE_FLAG = 0x92C488
LOGPALETTE_GLOBAL, BITMAPINFO_GLOBAL = 0x96E3F8, 0x96E3F4

SURFACE, STRIDE = 0xBA0210, 0xBA0214
TPAGE, COLOR, SOURCE_ROW, SOURCE_BASE = 0xA8E6F8, 0xA90A00, 0xA909D8, 0xA909FC
DEST_CELL, BLEND_CELL, COLOR_LUT_CELL, WIDTH = 0xA90944, 0xA90948, 0xA909AC, 0xA90A0C
COLOR_ROW, SOURCE_BYTE = 0x23, 0x01
PIXEL_CASES = ((0x3C, 0x7C), (0xBF, 0xFF), (0xC0, 0x60))

SHADE_STORE_PC = 0x71DE
BLEND_STORE_PCS = {0x720F, 0x7227, 0x723D, 0x724D}
CLUT_STORE_PC = 0x7274
PALETTE_STORE_PCS = {0x67AD, 0x67BE, 0x67CC, 0x67D6, 0x67E0, 0x67EC, 0x67F6, 0x6801}
PIXEL_DATA_WRITES = {
    DEST_CELL: 0xC80D,
    BLEND_CELL: 0xC831,
    COLOR_LUT_CELL: 0xC83B,
    SOURCE_BASE: 0xC84C,
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    raise AssertionError(message)


def align(value: int) -> int:
    return (value + 0xFFF) & ~0xFFF


def pe_perms(characteristics: int) -> int:
    result = 0
    if characteristics & 0x40000000:
        result |= UC_PROT_READ
    if characteristics & 0x80000000:
        result |= UC_PROT_WRITE
    if characteristics & 0x20000000:
        result |= UC_PROT_EXEC
    return result


def map_pe(uc, pe):
    header_size = pe.pe.OPTIONAL_HEADER.SizeOfHeaders
    uc.mem_map(BASE, align(header_size), UC_PROT_READ)
    uc.mem_write(BASE, pe.data[:header_size])
    for section in pe.sections:
        size = align(max(section["virtual_size"], section["raw_size"]))
        address = BASE + section["virtual_address"]
        uc.mem_map(address, size, pe_perms(section["characteristics"]))
        if section["raw_size"]:
            uc.mem_write(address, pe.read_rva(section["virtual_address"], section["raw_size"]))
        if size > section["raw_size"]:
            uc.mem_write(address + section["raw_size"], b"\0" * (size - section["raw_size"]))


def data_range(pe):
    section = next(section for section in pe.sections if section["name"] == ".data")
    start = BASE + section["virtual_address"]
    size = align(max(section["virtual_size"], section["raw_size"]))
    return start, size


def contains(address, size, start, end):
    return start <= address and address + size <= end


def outside_equal(before, after, base, allowed):
    cursor = base
    for start, end in sorted(allowed):
        if cursor < start and before[cursor - base:start - base] != after[cursor - base:start - base]:
            return False
        cursor = end
    return before[cursor - base:] == after[cursor - base:]


def put32(uc, address, value):
    uc.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))


def put16(uc, address, value):
    uc.mem_write(address, struct.pack("<H", value & 0xFFFF))


class NativeMachine:
    def __init__(self, pe):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        map_pe(self.uc, pe)
        self.uc.mem_map(SCRATCH, SCRATCH_SIZE, UC_PROT_READ | UC_PROT_WRITE)
        self.uc.mem_map(STACK, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
        self.data_base, self.data_size = data_range(pe)
        self.data_end = self.data_base + self.data_size
        self.stage = None
        self.active_stop = None
        self.error = {"kind": None, "detail": None}
        self.trace_counts = collections.Counter()
        self.builder_helper_entries = 0
        self.builder_global_reads = []
        self.builder_write_counts = collections.Counter()
        self.builder_writer_counts = collections.Counter()
        self.palette_writer_counts = collections.Counter()
        self.selected_builder_writes = {
            BLEND + 0x3C: [], BLEND + 0xBF: [], BLEND + 0xC0: [],
        }
        self.pixel_events = []
        self.pixel_global_reads = []
        self.pixel_global_writes = []
        self.pixel_allowed_reads = set()
        self.pixel_stop = None
        self.stage_stop = None
        self.uc.hook_add(UC_HOOK_CODE, self.on_code)
        self.uc.hook_add(UC_HOOK_MEM_READ, self.on_read)
        self.uc.hook_add(UC_HOOK_MEM_WRITE, self.on_write)
        self.uc.hook_add(UC_HOOK_MEM_INVALID, self.on_invalid)

    def stop_error(self, kind, detail):
        if self.error["kind"] is None:
            self.error.update(kind=kind, detail=detail)
        self.uc.emu_stop()

    def on_code(self, machine, address, size, _user):
        rva = address - BASE
        if self.active_stop is not None and address == BASE + self.active_stop:
            self.stage_stop = rva
            machine.emu_stop()
            return
        if self.stage == "builder":
            if BUILDER_START <= rva < BUILDER_STOP:
                self.trace_counts["builder"] += 1
            elif PALETTE_HELPER_START <= rva < PALETTE_HELPER_START + PALETTE_HELPER_SIZE:
                self.trace_counts["palette_helper_6780"] += 1
                if rva == PALETTE_HELPER_START:
                    self.builder_helper_entries += 1
            else:
                self.stop_error("unexpected_builder_code", hex(rva))
        elif self.stage == "pixel":
            if not PIXEL_START <= rva < PIXEL_STOP:
                self.stop_error("unexpected_pixel_code", hex(rva))
            else:
                self.trace_counts["pixel"] += 1
        else:
            self.stop_error("unknown_execution_stage", self.stage)

    def on_read(self, machine, access, address, size, value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP) - BASE
        raw = bytes(machine.mem_read(address, size))
        event = ("read", pc, address, size, int.from_bytes(raw, "little"))
        if self.stage == "builder":
            if contains(address, size, STACK, STACK + STACK_SIZE) or contains(address, size, SCRATCH, SCRATCH + SCRATCH_SIZE):
                return
            if contains(address, size, self.data_base, self.data_end):
                if (address, size) not in ((LOGPALETTE_GLOBAL, 4), (BITMAPINFO_GLOBAL, 4)):
                    self.stop_error("unexpected_builder_data_read", (hex(pc), hex(address), size))
                else:
                    self.builder_global_reads.append(event)
                return
            self.stop_error("unexpected_builder_memory_read", (hex(pc), hex(address), size))
        elif self.stage == "pixel":
            self.pixel_events.append(event)
            if contains(address, size, STACK, STACK + STACK_SIZE) or contains(address, size, SCRATCH, SCRATCH + SCRATCH_SIZE):
                return
            if contains(address, size, self.data_base, self.data_end):
                if (pc, address, size) not in self.pixel_allowed_reads:
                    self.stop_error("unexpected_pixel_data_read", (hex(pc), hex(address), size))
                else:
                    self.pixel_global_reads.append(event)
                return
            self.stop_error("unexpected_pixel_memory_read", (hex(pc), hex(address), size))
        else:
            self.stop_error("read_outside_phase", (hex(pc), hex(address), size))

    def on_write(self, machine, access, address, size, value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP) - BASE
        masked = value & ((1 << (size * 8)) - 1)
        event = ("write", pc, address, size, masked)
        if self.stage == "builder":
            if contains(address, size, STACK, STACK + STACK_SIZE):
                return
            if pc == SHADE_STORE_PC:
                if size != 1 or not contains(address, size, SHADE, SHADE + SHADE_SIZE):
                    self.stop_error("bad_shade_write", (hex(pc), hex(address), size))
                else:
                    self.builder_write_counts[address] += 1
                    self.builder_writer_counts[pc] += 1
                return
            if pc in BLEND_STORE_PCS:
                if size != 1 or not contains(address, size, BLEND, BLEND + BLEND_SIZE):
                    self.stop_error("bad_blend_write", (hex(pc), hex(address), size))
                else:
                    self.builder_write_counts[address] += 1
                    self.builder_writer_counts[pc] += 1
                    if address in self.selected_builder_writes:
                        self.selected_builder_writes[address].append({"pc_rva": hex(pc), "value": hex(masked)})
                return
            if pc == CLUT_STORE_PC:
                if size != 1 or not contains(address, size, CLUT_RAMP, CLUT_RAMP + CLUT_RAMP_SIZE):
                    self.stop_error("bad_clut_write", (hex(pc), hex(address), size))
                else:
                    self.builder_write_counts[address] += 1
                    self.builder_writer_counts[pc] += 1
                    if address in self.selected_builder_writes:
                        self.selected_builder_writes[address].append({"pc_rva": hex(pc), "value": hex(masked)})
                return
            if contains(address, size, SCRATCH, SCRATCH + SCRATCH_SIZE):
                log_range = (LOGPALETTE + 4, LOGPALETTE + 4 + 0x400)
                dib_range = (BITMAPINFO + 0x28, BITMAPINFO + 0x28 + 0x400)
                if (pc not in PALETTE_STORE_PCS or size != 1 or
                        not (contains(address, size, *log_range) or contains(address, size, *dib_range))):
                    self.stop_error("unexpected_builder_scratch_write", (hex(pc), hex(address), size))
                else:
                    self.palette_writer_counts[pc] += 1
                return
            self.stop_error("unexpected_builder_memory_write", (hex(pc), hex(address), size))
        elif self.stage == "pixel":
            self.pixel_events.append(event)
            if contains(address, size, STACK, STACK + STACK_SIZE):
                return
            if contains(address, size, self.data_base, self.data_end):
                if PIXEL_DATA_WRITES.get(address) != pc or size != 4:
                    self.stop_error("unexpected_pixel_data_write", (hex(pc), hex(address), size))
                else:
                    self.pixel_global_writes.append(event)
                return
            if address == DEST and size == 1 and pc == 0xC898:
                return
            self.stop_error("unexpected_pixel_memory_write", (hex(pc), hex(address), size))
        else:
            self.stop_error("write_outside_phase", (hex(pc), hex(address), size))

    def on_invalid(self, machine, access, address, size, value, _user):
        self.stop_error("invalid_memory", {
            "pc_rva": hex(machine.reg_read(UC_X86_REG_EIP) - BASE),
            "access": access, "address": hex(address), "size": size,
        })
        return False

    def run_builder(self):
        self.stage = "builder"
        self.active_stop = BUILDER_STOP
        esp = STACK + 0x8000
        for register, value in (
            (UC_X86_REG_EAX, 0), (UC_X86_REG_EBX, 0), (UC_X86_REG_ECX, 0),
            (UC_X86_REG_EDX, 0), (UC_X86_REG_ESI, 0), (UC_X86_REG_EDI, 0),
            (UC_X86_REG_EBP, 0), (UC_X86_REG_ESP, esp),
            (UC_X86_REG_EFLAGS, 0x202), (UC_X86_REG_EIP, BASE + BUILDER_START),
        ):
            self.uc.reg_write(register, value)
        error = None
        try:
            self.uc.emu_start(BASE + BUILDER_START, 0, timeout=0, count=MAX_BUILDER_INSNS)
        except UcError as exc:
            error = str(exc)
        if self.error["kind"]:
            fail(f"builder emulator audit failed: {self.error}")
        if error:
            fail(f"builder Unicorn error: {error}")
        if self.stage_stop != BUILDER_STOP:
            fail(f"builder did not stop before 0x{BUILDER_STOP:x}: {self.stage_stop}")
        if self.trace_counts["builder"] + self.trace_counts["palette_helper_6780"] >= MAX_BUILDER_INSNS:
            fail("builder instruction budget exhausted")
        self.active_stop = None

    def run_pixel(self, destination_byte):
        self.stage = "pixel"
        self.active_stop = PIXEL_STOP
        self.stage_stop = None
        self.pixel_events = []
        self.pixel_global_reads = []
        self.pixel_global_writes = []
        blend_address = BLEND + destination_byte
        shade_address = SHADE + COLOR_ROW * 256 + SOURCE_BYTE
        self.pixel_allowed_reads = {
            (0xC7F6, STRIDE, 4), (0xC7FD, SURFACE, 4), (0xC805, COLOR, 4),
            (0xC812, TPAGE, 4), (0xC836, SOURCE_ROW, 4), (0xC841, SOURCE_BASE, 4),
            (0xC858, BLEND_CELL, 4), (0xC860, SOURCE_BASE, 4),
            (0xC869, WIDTH, 2), (0xC870, DEST_CELL, 4),
            (0xC87A, COLOR_LUT_CELL, 4), (0xC88E, shade_address, 1),
            (0xC895, blend_address, 1),
        }

        put32(self.uc, SURFACE, DEST)
        put32(self.uc, STRIDE, 8)
        put32(self.uc, TPAGE, 0)
        put32(self.uc, COLOR, COLOR_ROW)
        put32(self.uc, SOURCE_ROW, 0)
        put32(self.uc, SOURCE_BASE, TEXTURE)
        put16(self.uc, WIDTH, 1)
        self.uc.mem_write(TEXTURE, bytes((SOURCE_BYTE,)))
        self.uc.mem_write(DEST, bytes((destination_byte,)))
        put32(self.uc, FRAME - 4, 0)
        before_data = bytes(self.uc.mem_read(self.data_base, self.data_size))
        before_tables = (bytes(self.uc.mem_read(SHADE, SHADE_SIZE)),
                         bytes(self.uc.mem_read(BLEND, BLEND_SIZE)))
        start_trace = self.trace_counts["pixel"]

        for register, value in (
            (UC_X86_REG_EAX, 0), (UC_X86_REG_EBX, 0), (UC_X86_REG_ECX, 0),
            (UC_X86_REG_EDX, 0), (UC_X86_REG_EBP, FRAME),
            (UC_X86_REG_ESP, FRAME - 4), (UC_X86_REG_EFLAGS, 0x202),
            (UC_X86_REG_EIP, BASE + PIXEL_START),
        ):
            self.uc.reg_write(register, value)
        error = None
        try:
            self.uc.emu_start(BASE + PIXEL_START, 0, timeout=1_000_000, count=MAX_PIXEL_INSNS)
        except UcError as exc:
            error = str(exc)
        if self.error["kind"]:
            fail(f"pixel emulator audit failed: {self.error}")
        if error:
            fail(f"pixel Unicorn error: {error}")
        if self.stage_stop != PIXEL_STOP:
            fail(f"pixel did not stop before 0x{PIXEL_STOP:x}: {self.stage_stop}")
        instruction_count = self.trace_counts["pixel"] - start_trace
        if instruction_count >= MAX_PIXEL_INSNS:
            fail("pixel instruction budget exhausted")

        expected_blend_index = destination_byte
        expected_blend_value = dict(PIXEL_CASES)[destination_byte]
        actual_shade = int.from_bytes(self.uc.mem_read(shade_address, 1), "little")
        actual_blend = int.from_bytes(self.uc.mem_read(blend_address, 1), "little")
        if actual_shade != 0:
            fail(f"builder did not produce zero shade result at {shade_address:#x}: {actual_shade:#x}")
        if actual_blend != expected_blend_value:
            fail(f"post-builder blend cell {blend_address:#x}={actual_blend:#x}, expected {expected_blend_value:#x}")
        output = int.from_bytes(self.uc.mem_read(DEST, 1), "little")
        if output != expected_blend_value:
            fail(f"native pixel output for destination {destination_byte:#x} is {output:#x}, expected {expected_blend_value:#x}")

        expected_global_reads = [
            (0xC7F6, STRIDE, 4), (0xC7FD, SURFACE, 4), (0xC805, COLOR, 4),
            (0xC812, TPAGE, 4), (0xC836, SOURCE_ROW, 4), (0xC841, SOURCE_BASE, 4),
            (0xC858, BLEND_CELL, 4), (0xC860, SOURCE_BASE, 4),
            (0xC869, WIDTH, 2), (0xC870, DEST_CELL, 4), (0xC87A, COLOR_LUT_CELL, 4),
            (0xC88E, shade_address, 1), (0xC895, blend_address, 1),
        ]
        actual_global_reads = [(e[1], e[2], e[3]) for e in self.pixel_global_reads]
        if actual_global_reads != expected_global_reads:
            fail(f"pixel global-read order differs: {actual_global_reads!r} != {expected_global_reads!r}")
        expected_global_values = [
            8, DEST, COLOR_ROW, 0, 0, TEXTURE, BLEND, TEXTURE, 1, DEST,
            SHADE + COLOR_ROW * 256, 0, expected_blend_value,
        ]
        actual_global_values = [event[4] for event in self.pixel_global_reads]
        if actual_global_values != expected_global_values:
            fail(f"pixel global-read values differ: {actual_global_values!r} != {expected_global_values!r}")
        pixel_access_expected = [
            ("read", 0xC882, TEXTURE, 1, SOURCE_BYTE),
            ("read", 0xC88E, shade_address, 1, 0),
            ("read", 0xC892, DEST, 1, destination_byte),
            ("read", 0xC895, blend_address, 1, expected_blend_value),
            ("write", 0xC898, DEST, 1, expected_blend_value),
        ]
        actual_pixel_access = [event for event in self.pixel_events
                               if event[2] in (TEXTURE, shade_address, DEST, blend_address)]
        if actual_pixel_access != pixel_access_expected:
            fail(f"pixel access/write ordering differs: {actual_pixel_access!r} != {pixel_access_expected!r}")
        expected_data_writes = [
            (0xC80D, DEST_CELL, 4, DEST),
            (0xC831, BLEND_CELL, 4, BLEND),
            (0xC83B, COLOR_LUT_CELL, 4, SHADE + COLOR_ROW * 256),
            (0xC84C, SOURCE_BASE, 4, TEXTURE),
        ]
        actual_data_writes = [(event[1], event[2], event[3], event[4])
                              for event in self.pixel_global_writes]
        if actual_data_writes != expected_data_writes:
            fail(f"pixel global-write order differs: {actual_data_writes!r} != {expected_data_writes!r}")
        expected_data = bytearray(before_data)
        for _pc, address, size, value in expected_data_writes:
            offset = address - self.data_base
            expected_data[offset:offset + size] = struct.pack("<I", value)
        after_data = bytes(self.uc.mem_read(self.data_base, self.data_size))
        if after_data != bytes(expected_data):
            fail("pixel phase changed .data outside its four documented pointer cells")
        after_tables = (bytes(self.uc.mem_read(SHADE, SHADE_SIZE)),
                        bytes(self.uc.mem_read(BLEND, BLEND_SIZE)))
        if after_tables != before_tables:
            fail("pixel phase modified builder-generated shade/blend tables")
        code = bytes(self.uc.mem_read(BASE + PIXEL_START, PIXEL_STOP - PIXEL_START))
        if code != self.pixel_code:
            fail("pixel phase modified original C700/C882 code bytes")

        result = {
            "destination_before": hex(destination_byte),
            "source_byte_nonzero": hex(SOURCE_BYTE),
            "shade_lut_address": hex(shade_address),
            "shade_lut_actual_value": hex(actual_shade),
            "blend_address": hex(blend_address),
            "blend_actual_value_after_builder": hex(actual_blend),
            "naive_average_before_clut_overwrite": hex((destination_byte + 0) >> 1),
            "native_destination_after": hex(output),
            "global_read_order": [self.event_json(event) for event in self.pixel_global_reads],
            "pixel_access_order": [self.event_json(event) for event in actual_pixel_access],
            "global_write_order": [self.event_json(event) for event in self.pixel_global_writes],
            "instruction_count": instruction_count,
            "stopped_before_rva": hex(PIXEL_STOP),
            "table_unchanged_by_pixel_phase": True,
            "original_code_unchanged": True,
        }
        self.uc.mem_write(self.data_base, before_data)
        self.active_stop = None
        return result

    @staticmethod
    def event_json(event):
        return {"kind": event[0], "pc_rva": hex(event[1]), "address": hex(event[2]),
                "size": event[3], "value": hex(event[4])}


def seed_builder_inputs(uc):
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
    uc.mem_write(LOGPALETTE, struct.pack("<HH", 0x0300, 0x0100) + b"\xD1" * 0x404)
    bmi_header = bytes((index * 7 + 3) & 0xFF for index in range(0x28))
    uc.mem_write(BITMAPINFO, bmi_header + b"\xD2" * 0x404)
    put32(uc, LOGPALETTE_GLOBAL, LOGPALETTE)
    put32(uc, BITMAPINFO_GLOBAL, BITMAPINFO)
    return guards


def main():
    pe = PE(ARGS.exe)
    if sha(pe.data) != EXE_SHA or pe.image_base != BASE:
        fail("pinned original EXE identity/image-base mismatch")
    if getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        fail("unexpected relocation table for fixed-base fixture")
    runtime = verify_runtime(unicorn, ARGS.deps)

    builder_script = ROOT / "renderer_contract" / "tests" / "test_original_palette_builder.py"
    pixel_script = ROOT / "renderer_contract" / "tests" / "test_original_c882_pixel_loop.py"
    for public_script in (builder_script, pixel_script):
        if not public_script.is_file():
            fail(f"missing public reference test script: {public_script}")

    machine = NativeMachine(pe)
    uc = machine.uc
    # Scratch and stack are mapped in NativeMachine; builder inputs reuse the
    # public palette fixture addresses before the same state enters C700.
    guards = seed_builder_inputs(uc)
    data_before_builder = bytes(uc.mem_read(machine.data_base, machine.data_size))
    text_section = next(section for section in pe.sections if section["name"] == ".text")
    text_base = BASE + text_section["virtual_address"]
    text_before = bytes(uc.mem_read(text_base, text_section["raw_size"]))
    machine.pixel_code = bytes(uc.mem_read(BASE + PIXEL_START, PIXEL_STOP - PIXEL_START))
    builder_code = bytes(uc.mem_read(BASE + BUILDER_START, BUILDER_STOP - BUILDER_START))
    helper_code = bytes(uc.mem_read(BASE + PALETTE_HELPER_START, PALETTE_HELPER_SIZE))
    machine.run_builder()

    expected_writer_counts = {
        SHADE_STORE_PC: SHADE_SIZE,
        0x720F: 0x10000,
        0x7227: 0x10000,
        0x723D: 0x10000,
        0x724D: 0x10000,
        CLUT_STORE_PC: CLUT_RAMP_SIZE,
    }
    actual_writer_counts = {pc: machine.builder_writer_counts[pc] for pc in expected_writer_counts}
    if actual_writer_counts != expected_writer_counts:
        fail(f"builder write-event counts differ: {actual_writer_counts!r}")
    if machine.builder_helper_entries != 256:
        fail(f"builder entered 6780 {machine.builder_helper_entries} times, expected 256")
    expected_palette_writer_counts = {pc: 256 for pc in PALETTE_STORE_PCS}
    if dict(machine.palette_writer_counts) != expected_palette_writer_counts:
        fail(f"builder palette helper write counts differ: {dict(machine.palette_writer_counts)!r}")
    unique_expected = SHADE_SIZE + BLEND_SIZE + CLUT_RAMP_SIZE - 0xC0
    if len(machine.builder_write_counts) != unique_expected:
        fail(f"builder unique table cells {len(machine.builder_write_counts)} != {unique_expected}")
    if sum(machine.builder_write_counts.values()) != SHADE_SIZE + BLEND_SIZE + CLUT_RAMP_SIZE:
        fail("builder total table write count differs")
    for address, count in machine.builder_write_counts.items():
        wanted_count = 2 if BLEND <= address < BLEND + 0xC0 else 1
        if count != wanted_count:
            fail(f"builder write multiplicity mismatch at {address:#x}: {count} != {wanted_count}")

    expected_selected_writes = {
        BLEND + 0x3C: [{"pc_rva": "0x720f", "value": "0x1e"},
                       {"pc_rva": "0x7274", "value": "0x7c"}],
        BLEND + 0xBF: [{"pc_rva": "0x720f", "value": "0x5f"},
                       {"pc_rva": "0x7274", "value": "0xff"}],
        BLEND + 0xC0: [{"pc_rva": "0x720f", "value": "0x60"}],
    }
    if machine.selected_builder_writes != expected_selected_writes:
        fail(f"builder selected write order differs: {machine.selected_builder_writes!r}")
    global_read_pattern = machine.builder_global_reads[:8]
    if len(machine.builder_global_reads) != 8 * 256:
        fail(f"builder global pointer read count is {len(machine.builder_global_reads)}, expected 2048")
    if any(machine.builder_global_reads[index:index + 8] != global_read_pattern
           for index in range(0, len(machine.builder_global_reads), 8)):
        fail("builder helper global-read pattern/order did not repeat 256 times")
    allowed_builder_data = [
        (SHADE, SHADE + SHADE_SIZE),
        (CLUT_RAMP, CLUT_RAMP + CLUT_RAMP_SIZE),
        (BLEND, BLEND + BLEND_SIZE),
    ]
    data_after_builder = bytes(uc.mem_read(machine.data_base, machine.data_size))
    if not outside_equal(data_before_builder, data_after_builder, machine.data_base, allowed_builder_data):
        fail("builder changed original .data outside the shade/blend/CLUT union")
    if bytes(uc.mem_read(text_base, text_section["raw_size"])) != text_before:
        fail("builder modified original .text")
    if any(bytes(uc.mem_read(address, len(value))) != value for address, value in guards.items()):
        fail("builder changed a protected table/mode guard")

    machine.pixel_code = bytes(uc.mem_read(BASE + PIXEL_START, PIXEL_STOP - PIXEL_START))
    pixel_results = [machine.run_pixel(destination) for destination, _expected in PIXEL_CASES]
    final_disk_sha = sha(ARGS.exe.read_bytes())
    if final_disk_sha != EXE_SHA:
        fail("original EXE changed on disk")

    report = {
        "schema_version": 1,
        "status": "BOUNDED_EQUIVALENT",
        "scope": "Same-state original-byte execution: native 71A0 plus original 6780 builder helper through 0x728C, then three finite original C700/C882 pixel slices stopped before 0xC89F. This proves only the specified LUT-alias pixel cases.",
        "oracle": {
            "path": str(pe.path), "sha256": sha(pe.data), "image_base": hex(pe.image_base),
            "base_relocations": 0,
            "builder_span": {"start_rva": hex(BUILDER_START), "stop_before_rva": hex(BUILDER_STOP),
                             "size": len(builder_code), "sha256": sha(builder_code)},
            "palette_helper_span": {"start_rva": hex(PALETTE_HELPER_START), "size": len(helper_code),
                                    "sha256": sha(helper_code)},
            "pixel_span": {"start_rva": hex(PIXEL_START), "stop_before_rva": hex(PIXEL_STOP),
                           "size": len(machine.pixel_code), "sha256": sha(machine.pixel_code)},
            "public_reference_scripts": {
                str(builder_script): sha(builder_script.read_bytes()),
                str(pixel_script): sha(pixel_script.read_bytes()),
            },
        },
        "runtime": {
            "engine": "Unicorn CPU emulator", "unicorn_version": unicorn.__version__,
            "architecture": "x86 32-bit", "original_sections_mapped_at_preferred_base": True,
            "python_executable": sys.executable, "python_version": sys.version.split()[0],
            "private_script_sha256": sha(Path(__file__).read_bytes()),
            "single_emulator_instance_for_builder_and_pixels": True,
            "builder_instruction_budget": MAX_BUILDER_INSNS, "pixel_instruction_budget_per_case": MAX_PIXEL_INSNS,
            "builder_instruction_count": machine.trace_counts["builder"] + machine.trace_counts["palette_helper_6780"],
            "builder_71a0_instruction_count": machine.trace_counts["builder"],
            "builder_6780_instruction_count": machine.trace_counts["palette_helper_6780"],
            "builder_6780_entries": machine.builder_helper_entries,
            "builder_stopped_before_rva": hex(BUILDER_STOP),
            "69a0_or_os_call_executed": False,
            "pixel_instruction_counts": [item["instruction_count"] for item in pixel_results],
            "all_pixel_cases_stopped_before_rva": hex(PIXEL_STOP),
            "unknown_pc_memory_or_os_endpoint_fails": True,
            "original_code_writes_forbidden": True,
            "builder_data_writes_limited_to_original_table_spans": True,
            "pixel_phase_did_not_modify_generated_tables": True,
            "fixture": {"shade_seed": "0xa5", "blend_seed": "0xb6", "clut_seed": "0xc7",
                        "source_nonzero": hex(SOURCE_BYTE), "color_row": hex(COLOR_ROW),
                        "selector": 0, "width": 1, "v": 0},
            "builder_global_read_order": {
                "pattern_repetitions": 256,
                "total_reads": len(machine.builder_global_reads),
                "pattern": [machine.event_json(event) for event in global_read_pattern],
            },
            "builder_table_writer_counts_by_pc": {hex(pc): count for pc, count in sorted(machine.builder_writer_counts.items())},
            "builder_selected_writes": {hex(address): events for address, events in machine.selected_builder_writes.items()},
            "pixel_cases": pixel_results,
        },
        "assertions": {
            "source_one_is_nonzero": True,
            "native_builder_shade_result_is_zero": all(item["shade_lut_actual_value"] == "0x0" for item in pixel_results),
            "final_blend_results": [item["native_destination_after"] for item in pixel_results],
            "expected_final_blend_results": [hex(value) for _, value in PIXEL_CASES],
            "first_cell_naive_average": "0x1e",
            "first_cell_native_final_after_row32_clut_overwrite": "0x7c",
            "row0_column191_final": "0xff",
            "row0_column192_final": "0x60",
            "builder_global_reads_before_pixel_calls": True,
            "exe_sha_after_run": final_disk_sha,
        },
        "limitations": [
            "Only one source texel and three destination-byte discriminators are tested; this is not full rasterization or image coverage.",
            "Only selector 0, color row 0x23, U=V=0, and a one-pixel C700/C882 slice are exercised.",
            "The native builder is stopped before its 0x69A0 call; no Windows API, DirectDraw, GPU, gameplay, FUNCTION_MATCH, or module equality is claimed.",
            "Table inputs are canary-seeded before native execution; no LUT values are computed or overlaid for the pixel phase.",
        ],
        "invocation": {"exe": str(ARGS.exe), "original_root": str(ARGS.original_root),
                       "deps": str(ARGS.deps), "analysis_deps": str(ARGS.analysis_deps),
                       "receipt": str(ARGS.receipt)},
    }
    report["runtime"].update(runtime)
    write_receipt(ARGS.receipt, report)
    print(json.dumps({"status": report["status"], "exe_sha256": report["oracle"]["sha256"],
                      "builder_instructions": report["runtime"]["builder_instruction_count"],
                      "pixel_instructions": report["runtime"]["pixel_instruction_counts"],
                      "outputs": report["assertions"]["final_blend_results"],
                      "receipt": str(ARGS.receipt)}, indent=2))


if __name__ == "__main__":
    main()
