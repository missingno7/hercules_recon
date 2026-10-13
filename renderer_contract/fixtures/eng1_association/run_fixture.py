"""Bounded original-x86 ENG1 mesh association fixture (synthetic inputs only)."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import struct
import sys
from collections import defaultdict

LANE = Path(__file__).resolve().parent
ROOT = LANE.parents[2]
ORIGINALS: Path | None = None
RUN_OUTPUT: Path | None = None
RUN_COMMAND: list[str] | None = None
PYTHON_DEPS = Path(r"C:\tools\hercules\renderer-oracle-python")
ANALYSIS_DEPS = Path(r"C:\tools\hercules\python")
PYTHON_RUNTIME = Path(r"C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe")
EXPECTED = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "ENG3.DLL": "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
    "TITLE.DLL": "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
}
EXE_BASE = 0x00400000
ENG_BASE = 0x10000000
BRIDGE_BASE = 0x11000000
STACK = 0x0D000000
STACK_SIZE = 0x10000
STACK_TOP = STACK + STACK_SIZE - 0x100
SENTINEL = 0x0E000000
SYNTH = 0x0C000000
STAGE = SYNTH + 0x10000
HEADER_TABLE = SYNTH + 0x20000
MATERIALS = SYNTH + 0x30000
CHILD_TABLE = SYNTH + 0x31000
VISIT = SYNTH + 0x40000
VIEW_STATE = SYNTH + 0x88000
OT = SYNTH + 0x50000
PACKETS = SYNTH + 0x60000
TEXTURES = SYNTH + 0x70000
MATRIX_SOURCE = SYNTH + 0x80000
TRANSLATION_SOURCE = SYNTH + 0x81000
CONFIG_ARGS = SYNTH + 0x82000
STAGE_PTR_GLOBAL = ENG_BASE + 0x70F30
TRIG_PTR_GLOBAL = ENG_BASE + 0x70F34
HEADER_PTR_GLOBAL = ENG_BASE + 0x70D34
MATERIAL_PTR_GLOBAL = ENG_BASE + 0x70D54
FACE_TABLE_GLOBAL = ENG_BASE + 0x70D50
OT_BASE_GLOBAL = ENG_BASE + 0x70D30
OT_PTR_GLOBAL = ENG_BASE + 0x71EC4
PACKET_CURSOR_GLOBAL = ENG_BASE + 0x73484
INTERFACE_PREFIX = ENG_BASE + 0x70F20
CALLBACKS_DEST = INTERFACE_PREFIX
TABLE_RVA = 0x361B8
TABLE_BYTES = 250 * 4
INSTRUCTION_LIMIT = 250000

# Exact original host callback spans from the static audit plus callbacks that
# the selected original prep/packet path is expected to use.
HOST_SPANS = {
    0xB4A0: 0x0D, 0xB4B0: 0x62, 0xB5C0: 0x47, 0xB660: 0x47,
    0xB6B0: 0x3A, 0xB6F0: 0x24, 0xB720: 0x1A, 0xB740: 0xCA,
    0xB810: 0x25, 0xB840: 0x23, 0xB880: 0x32, 0xB8F0: 0x22,
    0xB920: 0x4E, 0xB970: 0x14, 0xB990: 0x0A,
}
# The producer entry and FT4 tail have separate proof identities. Other spans
# are bounded to the exact leaf wrappers/prep/emitters reached by this fixture.
ENG_SPANS = {
    "2df20_GT4_entry": (0x2DF20, 0x334),
    "2df20_FT4_tail": (0x2E254, 0x156),
    # The old static audit recorded the first-RET prefix (0x1AD). This fixture
    # executes the positive-depth cache-hit continuation through its own RET.
    "2e3b0_probe_prep_selected_path": (0x2E3B0, 0x1BF),
    "2e600_FT4_points": (0x2E600, 0x69),
    "2ed10_GT4_points": (0x2ED10, 0x165),
    "2c670_area_inputs": (0x2C670, 0x19),
    "2c690_vertex_input": (0x2C690, 0x0F),
    "2c6c0_camera_snapshot": (0x2C6C0, 0x06),
    "2c6d0_area_result": (0x2C6D0, 0x06),
    "2c6e0_transform_project": (0x2C6E0, 0x06),
    "2c700_matrix_set": (0x2C700, 0x0F),
    "2c710_translation_set": (0x2C710, 0x0F),
    "2c740_area_result": (0x2C740, 0x0F),
    "2c750_camera_output": (0x2C750, 0x0F),
    "2c760_xy_output": (0x2C760, 0x0F),
    "2c770_ft4_packet": (0x2C770, 0x0F),
    "2c790_gt4_packet": (0x2C790, 0x0F),
    "2c920_viewport_matrix": (0x2C920, 0x14),
    "2c940_viewport_translation": (0x2C940, 0x14),
}
# Executed original address calculations feed B6B0's three cdecl arguments.
# These read-PC orders are a data-flow map, not coordinate-based matching.
AREA_ARGUMENT_READ_RVAS = {
    "gt4": (0x2E129, 0x2E126, 0x2E124),  # arg0, arg1, arg2
    "ft4": (0x2E28E, 0x2E2A5, 0x2E2A2),  # arg0, arg1, arg2
}

os.environ["HERCULES_PYTHON_DEPS"] = str(ANALYSIS_DEPS)
sys.path[:0] = [str(PYTHON_DEPS), str(ROOT / "scripts")]
from pe import PE  # noqa: E402
import unicorn  # noqa: E402
import pefile  # noqa: E402
from unicorn import (  # noqa: E402
    Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ,
    UC_HOOK_MEM_WRITE, UC_PROT_READ, UC_PROT_WRITE, UC_PROT_EXEC,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EFLAGS,
)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def u16(uc, address: int) -> int:
    return struct.unpack("<H", bytes(uc.mem_read(address, 2)))[0]


def s16(uc, address: int) -> int:
    return struct.unpack("<h", bytes(uc.mem_read(address, 2)))[0]


def s32(uc, address: int) -> int:
    return struct.unpack("<i", bytes(uc.mem_read(address, 4)))[0]


def signed32(value: int) -> int:
    value &= 0xFFFFFFFF
    return value - 0x100000000 if value & 0x80000000 else value


def u32(uc, address: int) -> int:
    return struct.unpack("<I", bytes(uc.mem_read(address, 4)))[0]


def put16(uc, address: int, value: int) -> None:
    uc.mem_write(address, struct.pack("<H", value & 0xFFFF))


def put32(uc, address: int, value: int) -> None:
    uc.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))


def map_pe(uc, image: PE, base: int) -> list[tuple[int, int]]:
    mapped: list[tuple[int, int]] = []
    executable: list[tuple[int, int]] = []
    for section in image.pe.sections:
        size = max(section.Misc_VirtualSize, section.SizeOfRawData)
        if not size:
            continue
        address = base + section.VirtualAddress
        lo = address & ~0xFFF
        hi = (address + size + 0xFFF) & ~0xFFF
        if any(lo < old_hi and old_lo < hi for old_lo, old_hi in mapped):
            raise RuntimeError(f"overlapping section mapping for {image.path}: {address:#x}")
        uc.mem_map(lo, hi - lo, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC)
        raw = image.pe.get_data(section.VirtualAddress, section.SizeOfRawData)
        if raw:
            uc.mem_write(address, raw)
        permissions = 0
        chars = section.Characteristics
        if chars & 0x40000000: permissions |= UC_PROT_READ
        if chars & 0x80000000: permissions |= UC_PROT_WRITE
        if chars & 0x20000000: permissions |= UC_PROT_EXEC
        uc.mem_protect(lo, hi - lo, permissions)
        mapped.append((lo, hi))
        if chars & 0x20000000:
            executable.append((address, address + size))
    return executable


def build_bridge(_output: Path) -> dict:
    raise RuntimeError("the public integration test must supply its hash-validated projection bridge builder")


def configure(repo_root: Path, originals: Path, output: Path, command: list[str]) -> None:
    global ROOT, ORIGINALS, RUN_OUTPUT, RUN_COMMAND
    ROOT = repo_root.resolve()
    ORIGINALS = originals.resolve()
    RUN_OUTPUT = output.resolve()
    RUN_COMMAND = command


def exports(image: PE, base: int) -> dict[str, int]:
    found = {}
    for symbol in image.pe.DIRECTORY_ENTRY_EXPORT.symbols:
        name = symbol.name.decode("ascii", "replace") if symbol.name else f"ordinal_{symbol.ordinal}"
        found[name] = base + symbol.address
    return found


class RunTrace:
    def __init__(self, uc, exec_ranges: list[tuple[int, int]], case_name: str,
                 bridge_ranges: list[tuple[int, int]] = ()):
        self.uc = uc
        self.exec_ranges = exec_ranges
        self.bridge_ranges = bridge_ranges
        self.case_name = case_name
        self.allowed = [(EXE_BASE + r, EXE_BASE + r + n, f"HERC+{r:#x}")
                        for r, n in HOST_SPANS.items()]
        self.allowed += [(ENG_BASE + r, ENG_BASE + r + n, f"ENG1+{r:#x}")
                         for _name, (r, n) in ENG_SPANS.items()]
        self.original_pcs = []
        self.bridge_pcs = []
        self.all_pcs = []
        self.reads = []
        self.writes = []
        self.faults = []
        self.mode = "setup"
        self.instruction_count = 0
        self.projection_events = []
        self.projection_state_updates = []
        self.bridge_capture_writes = []
        self.bridge_capture_addr = None
        self.current_point_source = None
        self.current_point_id = None
        self.current_emit = None
        self.vertex_index = 0
        self.packet_calls = []
        self.producer_context_events = []
        self.point_source_reads = []
        self.vertex_input_events = []
        self.vertex_output_events = []
        self.area_input_events = []
        self.area_output_events = []
        self.face_index_reads = []
        self.packet_xy_copy_calls = []
        self.active_area_event = None
        self.active_xy_copy_event = None
        self.stage_ptr = None
        self.area_results = []
        self.ot_writes = []
        self.packet_writes = []
        self.boundaries = defaultdict(list)
        uc.hook_add(UC_HOOK_CODE, self.on_code)
        uc.hook_add(UC_HOOK_MEM_READ, self.on_read)
        uc.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def in_code(self, address: int, ranges: list[tuple[int, int]]) -> bool:
        return any(lo <= address < hi for lo, hi in ranges)

    def on_code(self, uc, address, size, _user):
        self.instruction_count += 1
        if self.instruction_count > INSTRUCTION_LIMIT:
            raise RuntimeError(f"instruction budget exceeded in {self.case_name}")
        self.all_pcs.append(address)
        if self.in_code(address, self.exec_ranges):
            allowed = next((name for lo, hi, name in self.allowed if lo <= address < hi), None)
            if allowed is None:
                rva = address - (ENG_BASE if address >= ENG_BASE else EXE_BASE)
                raise RuntimeError(f"unexpected original instruction {address:#x} (RVA {rva:#x}) in {self.case_name}")
            module_base = ENG_BASE if address >= ENG_BASE else EXE_BASE
            self.original_pcs.append((self.mode, address - module_base, size, allowed))
        elif self.in_code(address, self.bridge_ranges):
            self.bridge_pcs.append((self.mode, address - BRIDGE_BASE))
        else:
            raise RuntimeError(f"execution outside pinned original/bridge code: {address:#x} in {self.case_name}")
        # Pin route branch and packet calls.
        rva = address - ENG_BASE if ENG_BASE <= address < ENG_BASE + 0x100000 else None
        if rva in (0x2E0F1, 0x2E27B):
            ebp = uc.reg_read(UC_X86_REG_EBP)
            self.producer_context_events.append({"mode": self.mode, "event": "face_index_read_entry",
                                                 "pc": hex(address), "face_record_pointer": hex(ebp + 8),
                                                 "face_record_word": hex(u32(uc, ebp + 8)),
                                                 "EBP": hex(ebp),
                                                 "ESI_packet_cursor_candidate": hex(uc.reg_read(UC_X86_REG_ESI)),
                                                 "EDI_point_base_candidate": hex(uc.reg_read(UC_X86_REG_EDI)),
                                                 "packet_cursor_global": hex(u32(uc, PACKET_CURSOR_GLOBAL)),
                                                 "stage_table_base": hex(u32(uc, STAGE_PTR_GLOBAL) + 0x40)})
        if rva in (0x2E0DA, 0x2E0E7, 0x2E254, 0x2E3A9, 0x2E253):
            self.boundaries[self.mode].append(hex(rva))
        if rva in (0x2C770, 0x2C790):
            packet_arg = u32(uc, uc.reg_read(UC_X86_REG_ESP) + 4)
            self.producer_context_events.append({"mode": self.mode, "event": "packet_callback_entry",
                                                 "pc": hex(address), "packet_pointer_arg": hex(packet_arg),
                                                 "ESI": hex(uc.reg_read(UC_X86_REG_ESI)),
                                                 "EDI": hex(uc.reg_read(UC_X86_REG_EDI)),
                                                 "latest_face_record": self.face_index_reads[-1]
                                                 if self.face_index_reads else None})
            self.packet_calls.append({"mode": self.mode, "wrapper_rva": hex(rva),
                                      "packet_arg": hex(packet_arg),
                                      "packet_destination": hex(packet_arg),
                                      "face_input": self.face_index_reads[-1] if self.face_index_reads else None})
        if address == EXE_BASE + 0xB920:
            self.projection_events.append({"mode": self.mode, "phase": "enter", "pc": hex(address),
                                           "camera_input_words": [s16(uc, 0x00A8E708), s16(uc, 0x00A8E70E), s16(uc, 0x00A8E714)],
                                           "source_point": self.point_source_reads[-1] if self.point_source_reads else None})
        if address in (EXE_BASE + 0xB880, EXE_BASE + 0xB8F0, EXE_BASE + 0xB970, EXE_BASE + 0xB990):
            sp = uc.reg_read(UC_X86_REG_ESP)
            arg = u32(uc, sp + 4)
            if address == EXE_BASE + 0xB880:
                update = {"mode": self.mode, "callback": "B880_matrix", "input_address": hex(arg),
                          "matrix_i16_row_major": [s16(uc, arg + 2 * i) for i in range(9)]}
            elif address == EXE_BASE + 0xB8F0:
                update = {"mode": self.mode, "callback": "B8F0_translation", "input_address": hex(arg),
                          "translation_source_address": hex(arg + 0x14),
                          "translation_xyz": [u32(uc, arg + 0x14 + 4 * i) for i in range(3)]}
            elif address == EXE_BASE + 0xB970:
                update = {"mode": self.mode, "callback": "B970_center", "center_xy":
                          [u32(uc, sp + 4), u32(uc, sp + 8)]}
            else:
                update = {"mode": self.mode, "callback": "B990_focal", "focal": u32(uc, sp + 4)}
            self.projection_state_updates.append(update)
        if address == EXE_BASE + 0xB6F0:
            input_ptr = u32(uc, uc.reg_read(UC_X86_REG_ESP) + 4)
            self.vertex_input_events.append({"mode": self.mode, "pc": hex(address),
                                             "input_address": hex(input_ptr),
                                             "input_words": [s16(uc, input_ptr + n * 2) for n in range(3)],
                                             "source_point": self.point_source_reads[-1] if self.point_source_reads else None})
        if address == EXE_BASE + 0xB720:
            output_ptr = u32(uc, uc.reg_read(UC_X86_REG_ESP) + 4)
            self.vertex_output_events.append({"mode": self.mode, "pc": hex(address),
                                              "output_address": hex(output_ptr),
                                              "source_point": self.point_source_reads[-1] if self.point_source_reads else None})
        if address == EXE_BASE + 0xB6B0:
            sp = uc.reg_read(UC_X86_REG_ESP)
            stage_reads = [row for row in self.reads[-16:]
                           if self.stage_ptr is not None and
                           self.stage_ptr + 0x40 <= row[1] < self.stage_ptr + 0x40 + 0x40]
            area_event = {"mode": self.mode, "pc": hex(address),
                          "packed_xy_args": [hex(u32(uc, sp + 4 + 4 * n)) for n in range(3)],
                          "stage_reads_before_call": stage_reads,
                          "face_input": self.face_index_reads[-1] if self.face_index_reads else None,
                          "global_output_writes": []}
            scratch_base = self.stage_ptr + 0x40 if self.stage_ptr is not None else 0
            argument_read_rvas = AREA_ARGUMENT_READ_RVAS.get(self.current_emit, ())
            argument_rows = []
            for read_rva in argument_read_rvas:
                found = next((row for row in stage_reads if row[0] == ENG_BASE + read_rva), None)
                if found is not None:
                    argument_rows.append(found)
            stage_indices = [(row[1] - scratch_base) // 8 for row in stage_reads
                             if scratch_base <= row[1] < scratch_base + 0x40 and
                             (row[1] - scratch_base) % 8 == 0]
            area_event["stage_read_point_indices"] = stage_indices
            area_event["argument_source_read_rvas"] = [hex(row[0] - ENG_BASE) for row in argument_rows]
            area_event["argument_source_read_addresses"] = [hex(row[1]) for row in argument_rows]
            area_event["argument_point_indices"] = [(row[1] - scratch_base) // 8 for row in argument_rows]
            self.area_input_events.append(area_event)
            self.active_area_event = area_event
        if address == EXE_BASE + 0xB4A0:
            sp = uc.reg_read(UC_X86_REG_ESP)
            self.area_output_events.append({"mode": self.mode, "pc": hex(address),
                                            "destination_address": hex(u32(uc, sp + 4)),
                                            "signed_area_global": s32(uc, 0x00A8E758),
                                            "face_input": self.face_index_reads[-1] if self.face_index_reads else None})
        if address in (EXE_BASE + 0xB660, EXE_BASE + 0xB5C0):
            sp = uc.reg_read(UC_X86_REG_ESP)
            output_pointer = u32(uc, sp + 4)
            copy_event = {"mode": self.mode, "pc": hex(address),
                          "callback": "B660_GT4" if address == EXE_BASE + 0xB660 else "B5C0_FT4",
                          "packet_destination": hex(output_pointer),
                          "face_input": self.face_index_reads[-1] if self.face_index_reads else None,
                          "global_xy_reads": [], "packet_xy_writes": []}
            self.packet_xy_copy_calls.append(copy_event)
            self.active_xy_copy_event = copy_event
        # Original callback returns through these instructions after a wrapper's
        # jmp; capture the post-B920 globals before the next point overwrites them.
        if address in (ENG_BASE + 0x2EE09, ENG_BASE + 0x2E649, ENG_BASE + 0x2E516):
            pending = next((item for item in reversed(self.projection_events)
                            if item["phase"] == "enter" and "return" not in item), None)
            if pending is not None:
                pending["return_pc"] = hex(address)
                pending["camera_xyz"] = [u32(uc, 0x00A8E71C), u32(uc, 0x00A8E728), u32(uc, 0x00A8E734)]
                pending["camera_xyz_signed"] = [s32(uc, 0x00A8E71C), s32(uc, 0x00A8E728),
                                                 s32(uc, 0x00A8E734)]
                pending["projected_xy"] = [u16(uc, 0x00A8E740), u16(uc, 0x00A8E746)]
                pending["return"] = True
        if self.bridge_capture_addr is not None and address >= BRIDGE_BASE and address < BRIDGE_BASE + 0x100000:
            # A getter is called once before production to locate the already-built
            # wrapper's existing 16-byte output record. This is a passive write spy.
            pass

    def on_read(self, uc, _access, address, size, value, _user):
        pc = uc.reg_read(UC_X86_REG_EIP)
        observed = int.from_bytes(bytes(uc.mem_read(address, size)), "little")
        self.reads.append((pc, address, size, observed))
        rva = pc - ENG_BASE if ENG_BASE <= pc < ENG_BASE + 0x100000 else None
        if self.current_emit == "gt4" and rva in (0x2ED97, 0x2EE53) and self.point_stream <= address < self.point_stream + 16:
            source_index = (address - self.point_stream) // 4
            self.point_source_reads.append({"mode": self.mode, "pc": hex(pc), "source_address": hex(address),
                                           "source_index": source_index, "packed": observed})
        if self.current_emit == "ft4" and rva in (0x2E61B, 0x2E649):
            self.point_source_reads.append({"mode": self.mode, "pc": hex(pc), "source_address": hex(address),
                                           "source_index": (address - self.point_stream) // 4,
                                           "packed": observed})
        if rva in (0x2E0F1, 0x2E27B) and self.point_stream + 20 <= address < self.point_stream + 20 + 0x80:
            word = observed & 0xFFFFFFFF
            self.face_index_reads.append({"mode": self.mode, "pc": hex(pc), "address": hex(address),
                                          "size": size, "word": hex(word),
                                          "face_record_index": (address - (self.point_stream + 20)) // 8,
                                          "point_indices": [(word >> (slot * 6)) & 0x3F for slot in range(4)]})
        if (self.active_xy_copy_event is not None and
                EXE_BASE + 0xB5C0 <= pc < EXE_BASE + 0xB6A7):
            if address in (0x00A8E740, 0x00A8E746, 0x00A8E742, 0x00A8E748, 0x00A8E744, 0x00A8E74A):
                self.active_xy_copy_event["global_xy_reads"].append({"pc": hex(pc), "address": hex(address),
                                                                      "size": size, "value": hex(observed)})
        if ((self.current_emit == "gt4" and rva == 0x2E183) or
                (self.current_emit == "ft4" and rva == 0x2E2C7)):
            self.fourth_vertex_reads = getattr(self, "fourth_vertex_reads", [])
            self.fourth_vertex_reads.append({"mode": self.mode, "pc": hex(pc), "source_address": hex(address),
                                             "size": size, "value": hex(observed),
                                             "face_input": self.face_index_reads[-1] if self.face_index_reads else None,
                                             "producer_route": self.current_emit})

    def on_write(self, uc, _access, address, size, value, _user):
        pc = uc.reg_read(UC_X86_REG_EIP)
        self.writes.append((pc, address, size, value))
        if OT <= address < OT + 0x10000:
            self.ot_writes.append({"mode": self.mode, "pc": hex(pc), "address": hex(address),
                                   "size": size, "value": hex(value & 0xffffffff)})
        if PACKETS <= address < PACKETS + 0x4000:
            self.packet_writes.append({"mode": self.mode, "pc": hex(pc), "address": hex(address),
                                       "size": size, "value": hex(value & 0xffffffff)})
            if (self.active_xy_copy_event is not None and
                    EXE_BASE + 0xB5C0 <= pc < EXE_BASE + 0xB6A7):
                self.active_xy_copy_event["packet_xy_writes"].append({"pc": hex(pc), "address": hex(address),
                                                                       "size": size, "value": hex(value & 0xffffffff)})
        if (self.active_area_event is not None and
                EXE_BASE + 0xB6B0 <= pc < EXE_BASE + 0xB6F0 and
                0x00A8E740 <= address <= 0x00A8E74A):
            write = {"pc": hex(pc), "address": hex(address), "size": size,
                     "value": hex(value & 0xFFFF)}
            if write not in self.active_area_event["global_output_writes"]:
                self.active_area_event["global_output_writes"].append(write)
        if self.bridge_capture_addr is not None and self.bridge_capture_addr <= address < self.bridge_capture_addr + 16:
            self.bridge_capture_writes.append({"mode": self.mode, "pc": hex(pc),
                                               "address": hex(address), "size": size,
                                               "value": hex(value & 0xffffffff)})
        if pc == EXE_BASE + 0xB920 and address in (0x00A8E71C, 0x00A8E728, 0x00A8E734, 0x00A8E740, 0x00A8E746):
            self.projection_events.append({"mode": self.mode, "phase": "write", "pc": hex(pc),
                                           "address": hex(address), "size": size,
                                           "value": hex(value & ((1 << (size * 8)) - 1))})

    def set_point_stream(self, address: int):
        self.point_stream = address

    def set_stage_ptr(self, address: int):
        self.stage_ptr = address


def invoke(uc, trace: RunTrace, entry: int, args: tuple[int, ...] = ()) -> dict:
    sp = STACK_TOP
    uc.mem_write(sp, struct.pack("<I", SENTINEL))
    for index, arg in enumerate(args):
        put32(uc, sp + 4 + index * 4, arg)
    for register, value in ((UC_X86_REG_EBX, 0x13579BDF), (UC_X86_REG_ESI, 0x2468ACE0),
                            (UC_X86_REG_EDI, 0x31415926), (UC_X86_REG_EBP, 0x27182818)):
        uc.reg_write(register, value)
    uc.reg_write(UC_X86_REG_EAX, 0x10203040)
    uc.reg_write(UC_X86_REG_ECX, 0x20304050)
    uc.reg_write(UC_X86_REG_EDX, 0x30405060)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
    uc.reg_write(UC_X86_REG_ESP, sp)
    uc.reg_write(UC_X86_REG_EIP, entry)
    start_count = trace.instruction_count
    uc.emu_start(entry, SENTINEL, count=INSTRUCTION_LIMIT)
    if uc.reg_read(UC_X86_REG_EIP) != SENTINEL:
        raise RuntimeError(f"call at {entry:#x} failed to return; EIP={uc.reg_read(UC_X86_REG_EIP):#x}")
    return {"eax": hex(uc.reg_read(UC_X86_REG_EAX)), "esp": hex(uc.reg_read(UC_X86_REG_ESP)),
            "instructions": trace.instruction_count - start_count}


def pack_xyz(x: int, y: int, z: int) -> int:
    return (x & 0xFF) | ((y & 0xFF) << 8) | ((z & 0xFF) << 24)


def face_word(indices: tuple[int, int, int, int]) -> int:
    return sum((v & 0x3F) << (i * 6) for i, v in enumerate(indices))


def setup_image(bridge_on: bool, output: Path, mode: str, faces: list[tuple[int, int, int, int]],
                flags: int, face_offset: int = 0):
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    exe = PE(str(ORIGINALS / "HERCULES.EXE"))
    eng = PE(str(ORIGINALS / "ENG1.DLL"))
    exec_ranges = map_pe(uc, exe, EXE_BASE) + map_pe(uc, eng, ENG_BASE)
    bridge_meta = None
    bridge_ranges = []
    bridge_exports = {}
    if bridge_on:
        bridge_meta = build_bridge(output / "bridge")
        bridge_image = PE(bridge_meta["dll"])
        bridge_exports = exports(bridge_image, BRIDGE_BASE)
        bridge_ranges = map_pe(uc, bridge_image, BRIDGE_BASE)
    uc.mem_map(STACK, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(SYNTH, 0x90000, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(SENTINEL, 0x1000, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC)

    # Copy the exact native 250-DWORD transport into the ENG1 interface prefix,
    # then explicitly seed only the producer-owned globals and synthetic input.
    callback_table = exe.read_rva(TABLE_RVA, TABLE_BYTES)
    table_hash = sha(callback_table)
    if table_hash != "52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8":
        raise RuntimeError("host callback transport hash mismatch")
    uc.mem_write(CALLBACKS_DEST, callback_table)
    original_slot106 = struct.unpack_from("<I", callback_table, 106 * 4)[0]
    if original_slot106 != EXE_BASE + 0xB920:
        raise RuntimeError(f"slot106 target unexpected: {original_slot106:#x}")

    # Bridge mode substitutes only transport word 106 with the already-reviewed
    # VC5 passive wrapper and gives it the exact original slot 106 target.
    if bridge_on:
        set_original = bridge_exports["hercules_projection_diagnostic_set_original_slot106"]
        capture = bridge_exports["hercules_projection_diagnostic_capture_slot106"]
        getter = bridge_exports["hercules_projection_diagnostic_capture_data"]
        trace = RunTrace(uc, exec_ranges, mode, bridge_ranges)
        trace.mode = "bridge_setup"
        invoke(uc, trace, set_original, (original_slot106,))
        result = invoke(uc, trace, getter)
        capture_addr = int(result["eax"], 16)
        trace.bridge_capture_addr = capture_addr
        put32(uc, CALLBACKS_DEST + 106 * 4, capture)
    else:
        trace = RunTrace(uc, exec_ranges, mode)
        capture_addr = None

    # Producer globals in ENG1's copied module. Its unchanged host prefix retains
    # the exact copied transport values (scratch/trig pointers included).
    point_stream = HEADER_TABLE + 0x104
    trace.set_point_stream(point_stream)
    put32(uc, HEADER_PTR_GLOBAL, HEADER_TABLE)
    put32(uc, MATERIAL_PTR_GLOBAL, CHILD_TABLE)
    put32(uc, FACE_TABLE_GLOBAL, TEXTURES)
    put32(uc, OT_BASE_GLOBAL, OT)
    put32(uc, OT_PTR_GLOBAL, OT)
    put32(uc, PACKET_CURSOR_GLOBAL, PACKETS)
    put32(uc, INTERFACE_PREFIX + 4, VIEW_STATE)
    # The DLL host table transport has a host pointer here. Keep the original
    # shared scratch pointer from the table; only the actual visit record is synthetic.

    # Host-owned perspective state through original setters; the 2E3B0 cache-hit
    # path later uses the original translation setter with synthetic stage bytes.
    matrix = [4096, 0, 0, 0, 4096, 0, 0, 0, 4096]
    uc.mem_write(MATRIX_SOURCE, struct.pack("<9h", *matrix))
    for i, value in enumerate((0, 0, 0)):
        put32(uc, TRANSLATION_SOURCE + 0x14 + 4 * i, value)
    trace.mode = "setup_host_callbacks"
    invoke(uc, trace, EXE_BASE + 0xB880, (MATRIX_SOURCE,))
    invoke(uc, trace, EXE_BASE + 0xB8F0, (TRANSLATION_SOURCE,))
    invoke(uc, trace, EXE_BASE + 0xB970, (160, 120))
    invoke(uc, trace, EXE_BASE + 0xB990, (32,))

    # Four synthetic point records. Points 0 and 3 differ in camera Z but have
    # equal projected XY under the configured projection; face indices also
    # reorder and repeat point IDs.
    points = [(31, 20, 64), (-30, 0, 64), (0, 80, 64), (34, 12, 96)]
    for i, point in enumerate(points):
        put32(uc, point_stream + 4 * i, pack_xyz(*point))
        put32(uc, TEXTURES + 4 * i, 0x11220000 | i)
    # Indexed native segment: selector 1 maps to segment data at HEADER_TABLE+0x100.
    put16(uc, HEADER_TABLE, 0x40)
    segment = HEADER_TABLE + 0x100
    put16(uc, segment, 1)  # prep selector==1 avoids optional material helper call
    uc.mem_write(segment + 2, bytes((4, len(faces))))
    # For four points the first face index word follows the point DWORDs plus 8.
    face_start = point_stream + 20
    for i, indices in enumerate(faces):
        record = face_start + 8 * i
        put32(uc, record, face_word(indices))
        put32(uc, record + 4, face_offset)
    # Material selector zero maps to MATERIALS; mode code at +0x0e is zero.
    put16(uc, MATERIALS + 0x0E, 0)
    put32(uc, MATERIALS + 8, 0)
    put16(uc, CHILD_TABLE, 0)
    put16(uc, CHILD_TABLE + 2, flags)
    put16(uc, VISIT + 0x20, 3)  # header index 1, winding flag 1
    put32(uc, VISIT + 4, MATERIALS)
    put32(uc, VISIT + 8, 0)
    put16(uc, VISIT + 0x10, 300)
    put16(uc, VISIT + 0x26, 0)
    put16(uc, VISIT + 0x27, 0)
    put16(uc, VISIT + 0x2A, 0)
    put16(uc, VISIT + 0x2C, flags)
    put16(uc, VISIT + 0x38, 256)
    put32(uc, VISIT + 0x40, 0)
    put32(uc, VISIT + 0x44, 0)
    put32(uc, VISIT + 0x48, 0)
    put32(uc, VISIT + 0x4C, 0)
    put32(uc, VISIT + 0x54, 0)
    put32(uc, VISIT + 0x58, 0)
    put32(uc, VISIT + 0x5C, 0)
    put16(uc, VISIT + 0x60, 0)
    put32(uc, VISIT + 0x64, 0)
    # Probe-cache hit condition in 2E3B0 uses visit+2A/+40; the top-level active
    # view guard uses a separate context pointer at interface-prefix word one.
    put16(uc, VISIT + 0x2A, 0)
    put32(uc, VISIT + 0x40, 0)
    put32(uc, VISIT + 0x44, 0)
    put32(uc, VIEW_STATE + 0x40, 0)
    # This global makes the selected prep path take its documented cache branch.
    put32(uc, ENG_BASE + 0x70CD0, 0)
    # The host B8F0 call reads the stage pointer's +0x14 source fields.
    stage_ptr = u32(uc, STAGE_PTR_GLOBAL)
    trace.set_stage_ptr(stage_ptr)
    for i in range(3):
        put32(uc, stage_ptr + 0x14 + 4 * i, 0)
    # Preseed one distinct address in the OT bucket; the producer must link it.
    # Actual bucket index is derived from camera Z and material word in execution.
    for i in range(0x800):
        put32(uc, OT + i * 4, 0x0BAD0000 + i)
    trace.mode = mode
    trace.current_emit = "ft4" if flags & 8 else "gt4"
    return uc, trace, bridge_exports, capture_addr, point_stream, points, segment, bridge_meta


def summarize_case(uc, trace: RunTrace, bridge_on: bool, bridge_exports: dict,
                   capture_addr: int | None, point_stream: int, points: list,
                   segment: int, bridge_meta: dict | None) -> dict:
    trace.mode = "mesh_producer"
    args = (VISIT,)
    call = invoke(uc, trace, ENG_BASE + 0x2DF20, args)
    cursor = u32(uc, PACKET_CURSOR_GLOBAL)
    packet_stride = 0x28 if trace.current_emit == "ft4" else 0x34
    packet_count = (cursor - PACKETS) // packet_stride
    if cursor < PACKETS or (cursor - PACKETS) % packet_stride:
        raise RuntimeError(f"producer cursor has invalid stride: {cursor:#x}")
    packets = []
    for pi in range(packet_count):
        address = PACKETS + pi * packet_stride
        vertex_offsets = (0x08, 0x10, 0x18, 0x20) if packet_stride == 0x28 else (0x08, 0x14, 0x20, 0x2C)
        vertices = []
        for vi, offset in enumerate(vertex_offsets):
            vertices.append({"slot": vi, "packet_xy_address": hex(address + offset),
                             "xy": [u16(uc, address + offset), u16(uc, address + offset + 2)]})
        link = u32(uc, address)
        tag = u32(uc, address + 4)
        packets.append({"packet_address": hex(address), "stride": packet_stride,
                        "link_previous_head": hex(link), "tag_word": hex(tag), "vertices": vertices})
    # Read point scratch (native stage base is copied table slot 4, plus 0x40).
    stage_ptr = u32(uc, STAGE_PTR_GLOBAL)
    point_records = []
    for i, point in enumerate(points):
        addr = stage_ptr + 0x40 + 8 * i
        point_records.append({"source_index": i, "source_address": hex(point_stream + 4 * i),
                              "source_xyz": list(point), "scratch_address": hex(addr),
                              "scratch_xy": [u16(uc, addr), u16(uc, addr + 2)],
                              "scratch_payload": hex(u32(uc, addr + 4))})
    # Preserve every face read, including faces culled before packet allocation.
    # Packet association comes from the producer's last face-record read at the
    # actual 2C790/2C770 call, then follows its copy callbacks and writes.
    face_reads = trace.face_index_reads
    face_by_record = {int(row["address"], 16): row for row in face_reads}
    call_by_packet = {int(row["packet_destination"], 16): row for row in trace.packet_calls}
    xy_copy_by_packet = {int(row["packet_destination"], 16): row for row in trace.packet_xy_copy_calls}
    area_by_face_record = {}
    for row in trace.area_input_events:
        face = row.get("face_input")
        if face:
            area_by_face_record[int(face["address"], 16)] = row
    fourth_by_face_record = {int(row["face_input"]["address"], 16): row
                             for row in getattr(trace, "fourth_vertex_reads", [])
                             if row.get("face_input")}
    for packet in packets:
        packet_address = int(packet["packet_address"], 16)
        call_event = call_by_packet.get(packet_address)
        if call_event is None or call_event.get("face_input") is None:
            raise RuntimeError(f"packet has no executed face-record association: {packet_address:#x}")
        face = call_event["face_input"]
        face_record = int(face["address"], 16)
        area_event = area_by_face_record.get(face_record)
        copy_event = xy_copy_by_packet.get(packet_address)
        if area_event is None or copy_event is None:
            raise RuntimeError(f"packet lacks original area/copy callback provenance: {packet_address:#x}")
        face_indices = face["point_indices"]
        if area_event["argument_point_indices"] != face_indices[:3]:
            raise RuntimeError(f"face slots do not match observed area callback source addresses: {packet_address:#x}")
        packet["face_record_address"] = hex(face_record)
        packet["face_record_index"] = face["face_record_index"]
        packet["face_point_indices"] = face_indices
        packet["vertices"][0]["copy_callback"] = copy_event["callback"]
        packet["vertices"][1]["copy_callback"] = copy_event["callback"]
        packet["vertices"][2]["copy_callback"] = copy_event["callback"]
        for vi, vertex in enumerate(packet["vertices"]):
            src_index = face_indices[vi]
            source_addr = stage_ptr + 0x40 + 8 * src_index
            vertex["point_index"] = src_index
            vertex["source_stage_record_address"] = hex(source_addr)
            vertex["source_point_stream_address"] = hex(point_stream + 4 * src_index)
            vertex["source_scratch_xy"] = [u16(uc, source_addr), u16(uc, source_addr + 2)]
            if vi < 3:
                read_pair = copy_event["global_xy_reads"][2 * vi:2 * vi + 2]
                write_pair = copy_event["packet_xy_writes"][2 * vi:2 * vi + 2]
                vertex["copy_provenance"] = {
                    "face_record_read_pc": face["pc"],
                    "face_record_address": face["address"],
                    "face_record_word": face["word"],
                    "face_slot": vi,
                    "area_callback_argument_point_index": area_event["argument_point_indices"][vi],
                    "area_callback_source_stage_read": next((row for row in area_event["stage_reads_before_call"]
                                                             if int(row[1]) == source_addr), None),
                    "packet_copy_callback": copy_event["callback"],
                    "packet_copy_global_reads": read_pair,
                    "packet_copy_destination_writes": write_pair,
                }
            else:
                fourth = fourth_by_face_record.get(face_record)
                if fourth is None:
                    raise RuntimeError(f"fourth packet vertex has no original source read: {packet_address:#x}")
                else:
                    fourth_offset = 0x20 if packet_stride == 0x28 else 0x2C
                    fourth_store_pc = ENG_BASE + (0x2E307 if packet_stride == 0x28 else 0x2E18B)
                    fourth_store = next((row for row in trace.packet_writes
                                         if int(row["address"], 16) == packet_address + fourth_offset and
                                         int(row["pc"], 16) == fourth_store_pc), None)
                    if fourth_store is None:
                        raise RuntimeError(f"fourth packet vertex has no original store: {packet_address:#x}")
                    source_index = (int(fourth["source_address"], 16) - (stage_ptr + 0x40)) // 8
                    if source_index != src_index:
                        raise RuntimeError(f"fourth source read does not match face slot 3: {packet_address:#x}")
                    vertex["copy_source_stage_address"] = fourth["source_address"]
                    vertex["copy_provenance"] = {"source_read_pc": fourth["pc"],
                                                  "source_stage_address": fourth["source_address"],
                                                  "source_face_record": fourth["face_input"],
                                                  "source_point_index_from_address": source_index,
                                                  "source_read_value": fourth["value"],
                                                  "original_store_pc": fourth_store["pc"],
                                                  "packet_destination_address": fourth_store["address"],
                                                  "packet_store_value": fourth_store["value"]}
    face_to_packet = [{"packet_address": packet["packet_address"],
                       "face_record_address": packet["face_record_address"],
                       "face_record_index": packet["face_record_index"],
                       "point_indices": packet["face_point_indices"]}
                      for packet in packets]
    # Join source memory reads, the B6F0 input pointer, B720 scratch output, and
    # post-B920 values by source record address/index, never coordinate equality.
    projection_by_source = {row["source_point"]["source_index"]: row
                            for row in trace.projection_events
                            if row.get("phase") == "enter" and row.get("return") and row.get("source_point")}
    input_by_source = {row["source_point"]["source_index"]: row
                       for row in trace.vertex_input_events if row.get("source_point")}
    output_by_source = {row["source_point"]["source_index"]: row
                        for row in trace.vertex_output_events if row.get("source_point")}
    for row in point_records:
        index = row["source_index"]
        row["source_read"] = next((read for read in trace.point_source_reads
                                   if read["source_index"] == index), None)
        row["B6F0_input"] = input_by_source.get(index)
        row["B720_output"] = output_by_source.get(index)
        projection = projection_by_source.get(index)
        row["B920_projection_return"] = (None if projection is None else {
            "pc": projection.get("return_pc"), "camera_xyz": projection.get("camera_xyz_signed"),
            "camera_xyz_raw": projection.get("camera_xyz"),
            "projected_xy": projection.get("projected_xy")})
    # Bucket pointers written by the producer are the actual destination addresses
    # from observed original memory writes. Keep all writes for auditability.
    non_sentinel_heads = {}
    for row in trace.ot_writes:
        if int(row["value"], 16) >= PACKETS:
            non_sentinel_heads[row["address"]] = row["value"]
    bridge_record = None
    bridge_capture_samples = []
    bridge_capture_matches_native = None
    if bridge_on and capture_addr is not None:
        captured_writes = trace.bridge_capture_writes
        if len(captured_writes) % 5:
            raise RuntimeError(f"slot-106 bridge produced a partial capture write group: {len(captured_writes)}")
        for start in range(0, len(captured_writes), 5):
            group = captured_writes[start:start + 5]
            expected_fields = [(capture_addr + offset, size) for offset, size in
                               ((0, 4), (4, 4), (8, 4), (12, 2), (14, 2))]
            observed_fields = [(int(item["address"], 16), item["size"]) for item in group]
            if observed_fields != expected_fields:
                raise RuntimeError("slot-106 bridge capture field layout differs from its 16-byte source ABI")
            camera_raw = [int(item["value"], 16) for item in group[:3]]
            bridge_capture_samples.append({
                "camera_xyz_raw": camera_raw,
                "camera_xyz_signed": [signed32(value) for value in camera_raw],
                "projected_xy": [int(item["value"], 16) for item in group[3:]],
                "writes": group,
            })
        native_projection_samples = [{"camera_xyz": row["camera_xyz"], "projected_xy": row["projected_xy"]}
                                     for row in trace.projection_events
                                     if row.get("phase") == "enter" and row.get("return")]
        bridge_values = [{"camera_xyz": row["camera_xyz_raw"], "projected_xy": row["projected_xy"]}
                         for row in bridge_capture_samples]
        bridge_capture_matches_native = bridge_values == native_projection_samples
        if not bridge_capture_matches_native:
            raise RuntimeError("passive slot-106 bridge captures differ from original B920 producer observations")
        bridge_record = {"capture_struct_address": hex(capture_addr),
                         "write_sequence": trace.bridge_capture_writes,
                         "samples": bridge_capture_samples,
                         "final_capture_bytes": bytes(uc.mem_read(capture_addr, 16)).hex(),
                         "matches_native_B920_events": bridge_capture_matches_native}
    executed_ranges = defaultdict(list)
    for _mode, rva, insn_size, name in trace.original_pcs:
        executed_ranges[name].append((rva, rva + insn_size))
    compressed_ranges = {}
    for name, spans in executed_ranges.items():
        merged = []
        for lo, hi in sorted(spans):
            if merged and lo <= merged[-1][1]:
                merged[-1] = (merged[-1][0], max(merged[-1][1], hi))
            else:
                merged.append((lo, hi))
        compressed_ranges[name] = [{"start_rva": hex(lo), "end_rva_exclusive": hex(hi)}
                                   for lo, hi in merged]
    bucket_observations = []
    for row in trace.ot_writes:
        address = int(row["address"], 16)
        bucket_observations.append({**row, "bucket_index": (address - OT) // 4,
                                    "bucket_index_hex": hex((address - OT) // 4)})
    return {
        "entry_call": call,
        "case": trace.case_name,
        "route": "2DF20 -> 2E3B0 -> 2ED10 -> 2E0F1..2E253 -> 2C790" if trace.current_emit == "gt4" else
                 "2DF20 -> 2E3B0 -> 2E600 -> 2E0DA -> 2E254..2E3A9 -> 2C770",
        "point_input_records": point_records,
        "source_point_reads": trace.point_source_reads,
        "projection_events": trace.projection_events,
        "projection_state_updates": trace.projection_state_updates,
        "vertex_input_events": trace.vertex_input_events,
        "vertex_output_events": trace.vertex_output_events,
        "area_input_events": trace.area_input_events,
        "area_output_events": trace.area_output_events,
        "packet_calls": trace.packet_calls,
        "producer_context_events": trace.producer_context_events,
        "packet_xy_copy_calls": trace.packet_xy_copy_calls,
        "fourth_vertex_source_reads": getattr(trace, "fourth_vertex_reads", []),
        "packet_count": packet_count,
        "packet_cursor": hex(cursor),
        "packets": packets,
        "face_index_reads": face_reads,
        "face_to_packet_associations": face_to_packet,
        "all_face_input_indices": [row["point_indices"] for row in face_reads],
        "OT_bucket_writes": trace.ot_writes,
        "OT_bucket_observations": bucket_observations,
        "OT_packet_head_writes": non_sentinel_heads,
        "packet_memory_writes": trace.packet_writes,
        "route_boundaries": dict(trace.boundaries),
        "bridge_capture": bridge_record,
        "bridge_capture_samples": bridge_capture_samples,
        "bridge_build": bridge_meta,
        "original_instruction_count": trace.instruction_count,
        "original_executed_rvas": sorted({hex(rva) for _, rva, _, _ in trace.original_pcs}),
        "executed_byte_ranges_by_span": compressed_ranges,
        "executed_span_names": sorted({name for _, _, _, name in trace.original_pcs}),
        "bridge_executed_rvas": sorted({hex(rva) for _, rva in trace.bridge_pcs}),
    }


def run_case(name: str, flags: int, face_indices: list[tuple[int,int,int,int]],
             bridge_on: bool, output: Path):
    uc, trace, bridge_exports, capture_addr, point_stream, points, segment, bridge_meta = setup_image(
        bridge_on, output, name, face_indices, flags)
    result = summarize_case(uc, trace, bridge_on, bridge_exports, capture_addr,
                            point_stream, points, segment, bridge_meta)
    result["bridge_on"] = bridge_on
    result["original_eng1_sha256"] = EXPECTED["ENG1.DLL"]
    result["original_exe_sha256"] = EXPECTED["HERCULES.EXE"]
    return result


def main() -> int:
    if ORIGINALS is None or RUN_OUTPUT is None:
        raise RuntimeError("configure explicit hash-checked originals and redirected work output before running")
    for name, expected in EXPECTED.items():
        path = ORIGINALS / name
        if not path.is_file() or sha(path.read_bytes()) != expected:
            raise RuntimeError(f"original hash mismatch: {path}")
    if unicorn.__version__ != "2.1.4":
        raise RuntimeError(f"unexpected Unicorn version {unicorn.__version__}")
    output = RUN_OUTPUT
    output.mkdir(parents=True, exist_ok=True)
    cases = []
    faces = [(0, 3, 2, 1), (2, 3, 1, 2), (1, 2, 3, 3)]
    cases.append(run_case("gt4_direct", 0, faces, False, output))
    cases.append(run_case("gt4_bridge", 0, faces, True, output))
    cases.append(run_case("ft4_tail_direct", 8, [faces[0]], False, output))
    cull_faces = [(0, 2, 1, 3), faces[0], faces[2]]
    cases.append(run_case("gt4_culled_prefix_direct", 0, cull_faces, False, output))
    # Strict discriminators: two point IDs project to same XY at different Z,
    # and their face indices are visibly swapped/repeated in packet provenance.
    for row in cases:
        pts = row["point_input_records"]
        if pts and pts[0]["scratch_xy"] != pts[3]["scratch_xy"]:
            raise AssertionError(f"equal-XY/different-depth discriminator failed: {row['case']}")
    direct, bridged, ft4, culled = cases
    if direct["packets"] != bridged["packets"] or direct["OT_bucket_writes"] != bridged["OT_bucket_writes"]:
        raise AssertionError("slot-106 direct vs passive bridge producer outputs differ")
    if not any(row["case"] == "ft4_tail_direct" and "0x2e254" in row["original_executed_rvas"]
               and "0x2e3a9" in row["original_executed_rvas"] for row in cases):
        raise AssertionError("FT4 original tail did not execute through its own RET")
    culled = next(row for row in cases if row["case"] == "gt4_culled_prefix_direct")
    if (culled["area_output_events"][0]["signed_area_global"] <= 0 or
            culled["packet_count"] != 2 or
            [row["face_record_index"] for row in culled["face_to_packet_associations"]] != [1, 2]):
        raise AssertionError("culled face did not leave the next emitted packet associated with its own face record")
    callback_bytes = PE(str(ORIGINALS / "HERCULES.EXE")).read_rva(TABLE_RVA, TABLE_BYTES)
    callback_values = {slot: struct.unpack_from("<I", callback_bytes, slot * 4)[0]
                       for slot in (4, 101, 106)}
    bridge_build = next(row["bridge_build"] for row in cases if row["case"] == "gt4_bridge")
    for row in cases:
        row.pop("bridge_build", None)
    python_version = sys.version.splitlines()[0]
    python_sha = sha(PYTHON_RUNTIME.read_bytes())
    pe_helper = ROOT / "scripts" / "pe.py"
    unicorn_library = Path(unicorn.__file__).parent / "lib" / "unicorn.dll"
    prediction_path = LANE / "prediction.md"
    runner_path = Path(__file__).resolve()
    expected_callback_dest = {
        4: CALLBACKS_DEST + 4 * 4,
        101: CALLBACKS_DEST + 101 * 4,
        106: CALLBACKS_DEST + 106 * 4,
    }
    packet_route_summary = []
    for row in cases:
        packet_route_summary.append({
            "case": row["case"], "packet_count": row["packet_count"],
            "faces_emitted": row["face_to_packet_associations"],
            "packet_tags": [{"packet": packet["packet_address"], "tag": packet["tag_word"],
                             "link_previous_head": packet["link_previous_head"]}
                            for packet in row["packets"]],
            "OT_bucket_observations": row["OT_bucket_observations"],
        })
    report = {
        "status": "ORIGINAL_X86_SYNTHETIC_MESH_ASSOCIATION_AND_SLOT106_BRIDGE_OBSERVED",
        "scope": "Hash-pinned original HERCULES.EXE and ENG1.DLL x86 execution over synthetic mesh, point, material, and OT inputs in Unicorn; no game process/assets.",
        "oracle_hashes": EXPECTED,
        "mapped_image_bases": {"HERCULES.EXE": hex(EXE_BASE), "ENG1.DLL": hex(ENG_BASE)},
        "prediction": {"path": str(prediction_path), "sha256": sha(prediction_path.read_bytes())},
        "runner": {"path": str(runner_path), "sha256": sha(runner_path.read_bytes()),
                   "command": RUN_COMMAND or [str(PYTHON_RUNTIME), str(runner_path)]},
        "dependencies": {
            "python": {"path": str(PYTHON_RUNTIME), "version": python_version, "sha256": python_sha},
            "unicorn": {"version": unicorn.__version__, "package_path": unicorn.__file__,
                        "package_init_sha256": sha(Path(unicorn.__file__).read_bytes()),
                        "native_library": str(unicorn_library), "native_library_sha256": sha(unicorn_library.read_bytes())},
            "pefile": {"version": getattr(pefile, "__version__", "unknown"), "path": pefile.__file__,
                       "sha256": sha(Path(pefile.__file__).read_bytes())},
            "repository_pe_adapter": {"path": str(pe_helper), "sha256": sha(pe_helper.read_bytes())},
            "vc5_bridge_toolchain": bridge_build["toolchain"],
        },
        "callback_transport": {"source_image": "HERCULES.EXE", "source_rva": hex(TABLE_RVA),
                               "bytes": TABLE_BYTES, "dword_count": 250,
                               "sha256": sha(PE(str(ORIGINALS / "HERCULES.EXE")).read_rva(TABLE_RVA, TABLE_BYTES)),
                               "destination_image": "ENG1.DLL", "destination": hex(CALLBACKS_DEST),
                               "copy_order": "copy all 250 DWORDs first; then seed only required producer globals and synthetic input",
                               "selected_slots": {
                                   str(slot): {"source_table_entry_rva": hex(TABLE_RVA + slot * 4),
                                               "destination_address": hex(expected_callback_dest[slot]),
                                               "value": hex(callback_values[slot])}
                                   for slot in (4, 101, 106)},
                               "slot106_original": hex(EXE_BASE + 0xB920),
                               "bridge_substitution": {"slot": 106,
                                                        "destination_address": hex(expected_callback_dest[106]),
                                                        "only_word_changed": True}},
        "synthetic_fixture_layout": {
            "point_stream": hex(HEADER_TABLE + 0x104),
            "face_record_start": hex(HEADER_TABLE + 0x104 + 20), "face_record_stride": 8,
            "synthetic_mesh_header": hex(HEADER_TABLE), "synthetic_segment": hex(HEADER_TABLE + 0x100),
            "synthetic_material": hex(MATERIALS), "material_mode_byte_offset": "0x0e",
            "material_mode_byte_value": 0, "visit_record": hex(VISIT),
            "visit_winding_word": "0x0003", "visit_depth_input_word": 256,
            "synthetic_ot_base": hex(OT), "synthetic_packet_arena": hex(PACKETS),
            "synthetic_packet_arena_mapping_bytes": "0x4000",
            "packet_arena_capacity_claim": False,
            "point_stage_pointer_from_native_slot4": hex(callback_values[4]),
            "point_stage_record_base": hex(callback_values[4] + 0x40),
            "point_stage_record_stride": 8,
            "points_xyz": [(31, 20, 64), (-30, 0, 64), (0, 80, 64), (34, 12, 96)],
            "faces": {"gt4_primary": faces, "ft4_mode0_tail": [faces[0]], "gt4_cull_discriminator": cull_faces},
            "material_mode0_discriminator": "material[+0x0e]=0; FT4 dispatch flag word is 8",
        },
        "producer_observation_contract": {
            "point_transform_path": "original source read -> B6F0 input scratch pointer -> B740 camera transform -> B920 cameraXYZ/projectedXY -> B720 same scratch output pointer",
            "GT4_face_word_read": {"rva": "0x2e0f1", "word_address_register": "[EBP+8]",
                                     "point_source_read_pc_to_area_argument_order": ["0x2e129", "0x2e126", "0x2e124"]},
            "FT4_face_word_read": {"rva": "0x2e27b", "word_address_register": "[EBP+8]",
                                     "point_source_read_pc_to_area_argument_order": ["0x2e28e", "0x2e2a5", "0x2e2a2"]},
            "packet_commit_callbacks": {"GT4_rva": "0x2c790", "FT4_rva": "0x2c770",
                                         "packet_pointer_argument": "[ESP+4]", "producer_register": "ESI"},
            "GT4_fourth_vertex_copy": {"source_read_rva": "0x2e183", "packet_store_rva": "0x2e18b"},
            "FT4_fourth_vertex_copy": {"source_read_rva": "0x2e2c7", "packet_store_rva": "0x2e307"},
            "association_basis": "face-word indices, exact scratch addresses, callback argument order/read PCs, callback global reads, packet destination writes, and direct fourth-slot load/store PCs; no coordinate-equality join",
        },
        "packet_and_ot_summary": packet_route_summary,
        "passive_slot106_bridge_build": bridge_build,
        "proof_spans": {name: {"rva": hex(rva), "size": hex(size)} for name, (rva, size) in ENG_SPANS.items()},
        "host_callback_spans": {hex(rva): {"size": hex(size)} for rva, size in HOST_SPANS.items()},
        "historical_geometry_extent_preserved": {"entry_rva": "0x2df20", "size": "0x334", "ret_rva": "0x2e253"},
        "separate_ft4_tail_extent": {"entry_rva": "0x2e254", "size": "0x156", "ret_rva": "0x2e3a9", "branch_rva": "0x2e0da"},
        "cases": cases,
        "limits": ["Synthetic object/mesh/material inputs only; no live scene object ID or actor-pool association.",
                   "Executed selected original x86 spans and explicit host callbacks only; no whole-module recovery claim.",
                   "A callback-only slot-106 observer cannot recover point IDs: B6B0 receives packed XY; B660/B5C0 copy global XY. Equal-XY point IDs 0 and 3 have camera Z 320 and 352. Preserve face identity with a producer-side observation at 2E0F1/2E27B and confirm emission at 2C790/2C770.",
                   "The passive slot-106 bridge result is a bounded call-path comparison, not callback publication or lifetime proof.",
                   "The original 2DF20 first-RET extent remains 0x334; the observed FT4 tail is separately recorded and does not promote recovery state.",
                   "Synthetic OT bucket index is reported from observed writes; no wider depth formula, real frame capacity, module-layout, or whole-binary claim is made."],
    }
    report_path = output / "association_report.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": str(report_path), "sha256": sha(report_path.read_bytes()),
                      "cases": [(c["case"], c["packet_count"], c["executed_span_names"]) for c in cases]}, indent=2))
    return 0


def u32_from_hex(value: str) -> int:
    return int(value, 16)


if __name__ == "__main__":
    raise SystemExit(main())
