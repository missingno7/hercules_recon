"""Private, bounded native-x86 traversal over a read-only archived FOREST map."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import argparse
from pathlib import Path
import struct
import sys
sys.dont_write_bytecode = True
from pathguard import create_fresh_output, discover_repo_root, input_file, repo_path

LANE = Path(__file__).resolve().parent
SCRIPT_REPO = discover_repo_root(LANE)
REPO = SCRIPT_REPO
ORIGINALS = None
ARCHIVE = None
OUTPUT_DIR = None
ARCHIVE_SHA = "affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4"
MAP_SHA = "78bc5a60ecce1c99e752fb30a16bc5ace4ee80f686365e1da8136a0decbd69cb"
MAP_OFFSET = 18845296
MAP_LENGTH = 1304576
PY = Path(r"C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe")
FIXTURE = SCRIPT_REPO / "renderer_contract/fixtures/eng1_association/run_fixture.py"

spec = importlib.util.spec_from_file_location("eng1_assoc_fixture", FIXTURE)
fx = importlib.util.module_from_spec(spec)
assert spec.loader
spec.loader.exec_module(fx)

# Keep all machine-code ranges exact and separate by owner. 2DA90 is a distinct
# caller from 2DF20; the resource consumer and the emitter are not merged.
fx.ENG_SPANS.update({
    "resource_rebuild_blob_20670": (0x20670, 0x1C0),
    "resource_select_group_20830": (0x20830, 0xC0),
    "resource_rebuild_group_rows_208F0": (0x208F0, 0xA0),
    "2da90_native_traversal": (0x2DA90, 0x48B),
    # 2E600's measured branch at +0x22 enters the separate legacy body at
    # 2E669; its RET at 2E6B4 is pinned independently of 2E600's RET.
    "2e600_explicit_branch_tail_2e669": (0x2E669, 0x4C),
    # 2ED10's per-point decision at +0x97 branches to the shared tail at
    # 2EE75; that tail returns at 2EF51 and remains a separate proof span.
    "2ed10_explicit_branch_tail_2ee75": (0x2EE75, 0xDE),
})
# Exact original host table slot 164 reached by ENG1's 2C920 angle/matrix
# wrapper. Disassembly is a pure three-angle transform reading the already
# copied trig table; first RET is host RVA 0xBC2B.
fx.HOST_SPANS[0xBB00] = 0x12C
# Slot165 follows 164 in the 2C940 wrapper and applies the measured three-axis
# scale vector to the 9 WORD matrix; first RET is host RVA 0xBCCE.
fx.HOST_SPANS[0xBC30] = 0x9F

BASE = fx.SYNTH
MAP_BASE = BASE + 0x1800000
CONTEXT = BASE + 0x32000
MATRIX_SOURCE = BASE + 0x80000
TRANSLATION_SOURCE = BASE + 0x81000
OT_BASE = BASE + 0x800000
PACKET_BASE = BASE + 0x900000
PACKET_BYTES = 0x600000
OT_BYTES = 0x100000
STACK = fx.STACK
SENTINEL = fx.SENTINEL


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def s16(b: bytes, off: int = 0) -> int:
    return struct.unpack_from("<h", b, off)[0]


def read_map() -> bytes:
    if sha(ARCHIVE.read_bytes()) != ARCHIVE_SHA:
        raise RuntimeError("read-only HERCULES.FS identity mismatch")
    with ARCHIVE.open("rb") as stream:
        stream.seek(MAP_OFFSET)
        data = stream.read(MAP_LENGTH)
    if len(data) != MAP_LENGTH or sha(data) != MAP_SHA:
        raise RuntimeError("archived FOREST MAP_PSX.BIN identity mismatch")
    return data


def map_asset(uc, data: bytes) -> None:
    rounded = (len(data) + 0xFFF) & ~0xFFF
    uc.mem_map(MAP_BASE, rounded, fx.UC_PROT_READ | fx.UC_PROT_WRITE)
    uc.mem_write(MAP_BASE, data)


class Trace(fx.RunTrace):
    def __init__(self, *args, **kwargs):
        self.mesh_calls = []
        self.dynamic_source_reads = []
        self.real_point_outputs = []
        self.real_packet_calls = []
        self.packet_events = []
        self.current_mesh = None
        self.current_source_read = None
        self.current_point_input = None
        self.pending_point_output = None
        self.map_grid_reads = []
        self.mesh_color_reads = []
        self.association_events = []
        self.association_sequence = 0
        self.winding_field_writes = []
        self.prep_results = []
        self.pending_prep = None
        super().__init__(*args, **kwargs)

    def association_event(self, event):
        self.association_sequence += 1
        event["sequence"] = self.association_sequence
        self.association_events.append(event)

    def on_code(self, uc, address, size, user):
        super().on_code(uc, address, size, user)
        if self.pending_prep is not None:
            return_pc, visit_address, event = self.pending_prep
            if address == return_pc:
                eax = uc.reg_read(fx.UC_X86_REG_EAX) & 0xFFFFFFFF
                event["return_pc"] = hex(address)
                event["return_eax_raw"] = hex(eax)
                event["return_value_signed"] = eax if eax < 0x80000000 else eax - 0x100000000
                event["visit_depth_after"] = fx.s16(uc, visit_address + 0x10)
                self.prep_results.append(event)
                self.pending_prep = None
        if self.pending_point_output is not None:
            ret_pc, output, event = self.pending_point_output
            if address == ret_pc:
                event["scratch_xy_after_return"] = [fx.u16(uc, output), fx.u16(uc, output + 2)]
                event["scratch_payload_after_return"] = hex(fx.u32(uc, output + 4))
                self.pending_point_output = None
        if address == fx.ENG_BASE + 0x2DF20:
            sp = uc.reg_read(fx.UC_X86_REG_ESP)
            visit = fx.u32(uc, sp + 4)
            packed = fx.u32(uc, visit + 0x20)
            index = (packed >> 1) & 0x3FF
            lookup = fx.u32(uc, fx.HEADER_PTR_GLOBAL)
            segment = s16(bytes(uc.mem_read(lookup + (index - 1) * 4, 2))) + index
            stream = lookup + (segment - 1) * 4
            selector = s16(bytes(uc.mem_read(stream, 2)))
            count = uc.mem_read(stream + 2, 1)[0]
            face_count = uc.mem_read(stream + 3, 1)[0]
            point_start = stream + 4
            face_start = point_start + 4 * count + 4
            cell_word = packed
            lookup_id = (packed >> 11) & 0x1FFF
            cell_lookup = fx.u32(uc, fx.MATERIAL_PTR_GLOBAL) + lookup_id * 4
            cell_id = fx.u16(uc, cell_lookup)
            material_flags = fx.u16(uc, cell_lookup + 2)
            child = fx.u32(uc, fx.ENG_BASE + 0x70D58) + (cell_id << 4)
            layer = fx.u32(uc, sp + 8)
            row = fx.s32(uc, visit + 0x18)
            column = fx.s32(uc, visit + 0x1C)
            side = uc.mem_read(visit + 0x29, 1)[0]
            depth = fx.s16(uc, visit + 0x10)
            winding = uc.mem_read(visit + 0x24, 1)[0]
            shift = uc.mem_read(visit + 0x26, 1)[0]
            limit = uc.mem_read(visit + 0x27, 1)[0]
            actual_cell_read = next((event for event in reversed(self.map_grid_reads)
                                     if event["value"] == packed and event["pc_rva"] >= 0x2DA90 and
                                     event["pc_rva"] < 0x2DF1B), None)
            if actual_cell_read is None:
                raise RuntimeError(f"2DA90 visit has no preceding actual grid read for {packed:#x}")
            cell_address = int(actual_cell_read["address"], 16)
            self.current_emit = "ft4" if (material_flags & 8) else "gt4"
            self.point_stream = point_start
            self.current_mesh = {
                "visit": visit, "packed_cell": hex(cell_word), "mesh_index": index,
                "stream": stream, "selector": selector, "point_start": point_start,
                "point_count": count, "face_count": face_count,
                "face_start": face_start, "cell_lookup_index": lookup_id,
                "cell_id": cell_id, "material_flags": material_flags,
                "variant": (fx.u16(uc, child + 0xE) >> 14) & 3,
                "depth_010": depth, "winding_024": winding,
                "shift_026": shift, "limit_027": limit,
                "emit": self.current_emit, "layer": layer, "row": row,
                "column": column, "side": side, "cell_address": cell_address,
                "cell_file_offset": cell_address - MAP_BASE,
                "native_cell_read": actual_cell_read,
            }
            self.current_mesh["source_cell_word"] = hex(fx.u32(uc, cell_address))
            points = []
            for point_index in range(count):
                raw = fx.u32(uc, point_start + point_index * 4)
                packed_bytes = raw.to_bytes(4, "little")
                points.append({"index": point_index, "address": hex(point_start + point_index * 4),
                               "file_offset": point_start + point_index * 4 - MAP_BASE,
                               "packed_dword": hex(raw),
                               "source_xyz": [packed_bytes[0] - (256 if packed_bytes[0] & 128 else 0),
                                              packed_bytes[1] - (256 if packed_bytes[1] & 128 else 0),
                                              packed_bytes[3] - (256 if packed_bytes[3] & 128 else 0)]})
            faces = []
            for face_index in range(face_count):
                face_addr = face_start + face_index * 8
                word = fx.u32(uc, face_addr)
                faces.append({"index": face_index, "address": hex(face_addr),
                              "file_offset": face_addr - MAP_BASE, "packed_index_word": hex(word),
                              "point_indices": [(word >> (6 * i)) & 0x3F for i in range(4)],
                              "flags_high_byte": (word >> 24) & 0xFF,
                              "second_dword": hex(fx.u32(uc, face_addr + 4))})
            self.current_mesh["source_points"] = points
            self.current_mesh["source_faces"] = faces
            self.mesh_calls.append(self.current_mesh)
            self.current_source_read = None
            self.current_point_input = None

        if address == fx.ENG_BASE + 0x2E3B0:
            sp = uc.reg_read(fx.UC_X86_REG_ESP)
            visit_address = fx.u32(uc, sp + 4)
            return_pc = fx.u32(uc, sp)
            call_index = len(self.mesh_calls) - 1
            if self.current_mesh is None or self.current_mesh["visit"] != visit_address:
                raise RuntimeError("2E3B0 visit pointer does not match current resource mesh")
            event = {
                "mesh_call_index": call_index,
                "mesh_index": self.current_mesh["mesh_index"],
                "entry_rva": "0x2E3B0",
                "return_pc": None,
                "visit_address": hex(visit_address),
                "call_return_address": hex(return_pc),
                "visit_depth_before": fx.s16(uc, visit_address + 0x10),
                "cell_id": fx.u16(uc, visit_address + 0x2A),
                "variant": uc.mem_read(visit_address + 0x25, 1)[0],
            }
            self.pending_prep = (return_pc, visit_address, event)

        # Slot101/B6F0 receives each real producer XYZ scratch input. The
        # source identity is the immediately preceding original load from the
        # current mesh's point stream, not a coordinate-value match.
        if address == fx.EXE_BASE + 0xB6F0:
            sp = uc.reg_read(fx.UC_X86_REG_ESP)
            input_address = fx.u32(uc, sp + 4)
            return_pc = fx.u32(uc, sp)
            source = self.current_source_read
            if source is not None and self.current_mesh is not None and source["mesh_index"] == self.current_mesh["mesh_index"]:
                source_index = source["source_point_index"]
                self.association_event({
                    "kind": "b6f0_input",
                    "mesh_call_index": len(self.mesh_calls) - 1,
                    "mesh_index": self.current_mesh["mesh_index"],
                    "source_point_index": source_index,
                    "source_read_pc": source["pc"],
                    "source_read_address": source["address"],
                    "source_read_size": source["size"],
                    "host_callback_pc": hex(address),
                    "host_callback_return_pc": hex(return_pc),
                    "host_input_address": hex(input_address),
                })
                self.current_point_input = {
                    "mesh_call_index": len(self.mesh_calls) - 1,
                    "mesh_index": self.current_mesh["mesh_index"],
                    "source_point_index": source["source_point_index"],
                    "source_address": source["address"],
                    "source_packed_dword": source["value"],
                    "source_read_pc": source["pc"],
                    "source_read_size": source["size"],
                    "host_callback_rva": "0xB6F0",
                    "host_callback_return_pc": hex(return_pc),
                    "host_input_address": hex(input_address),
                    "host_input_signed_words": [fx.s16(uc, input_address + 2 * i) for i in range(3)],
                    "producer_registers": {name: hex(uc.reg_read(register)) for name, register in (
                        ("EAX", fx.UC_X86_REG_EAX), ("EBX", fx.UC_X86_REG_EBX),
                        ("ECX", fx.UC_X86_REG_ECX), ("EDX", fx.UC_X86_REG_EDX),
                        ("ESI", fx.UC_X86_REG_ESI), ("EDI", fx.UC_X86_REG_EDI),
                        ("EBP", fx.UC_X86_REG_EBP))},
                }

        # B740 is the native camera transform callback and B920 performs the
        # native projection. Snapshot their exact per-point input state instead
        # of assuming the initializer's first camera configuration stayed live.
        if address == fx.EXE_BASE + 0xB740 and self.current_point_input is not None:
            # B740 is a no-argument host callback here; the point is in the
            # three global WORD lanes written by the preceding slot-101 path.
            input_addresses = (0x00A8E708, 0x00A8E70E, 0x00A8E714)
            self.current_point_input["camera_transform"] = {
                "input_global_addresses": [hex(a) for a in input_addresses],
                "input_signed_words": [fx.s16(uc, a) for a in input_addresses],
                "matrix_words_row_major": [fx.s16(uc, 0x00A8E75C + 2 * i) for i in range(9)],
                "translation_dwords": [fx.s32(uc, a) for a in (0x00A8E770, 0x00A8E774, 0x00A8E778)],
            }
        if address == fx.EXE_BASE + 0xB920 and self.current_point_input is not None:
            self.current_point_input["projection"] = {
                "camera_input_signed_words": [fx.s16(uc, a) for a in (0x00A8E708, 0x00A8E70E, 0x00A8E714)],
            }

        # Slot106/B720 writes native screen XY to the scratch vertex pointer.
        # The result is read only after the actual host callback returns.
        if address == fx.EXE_BASE + 0xB720:
            sp = uc.reg_read(fx.UC_X86_REG_ESP)
            output = fx.u32(uc, sp + 4)
            return_pc = fx.u32(uc, sp)
            source = self.current_point_input
            if source is not None and self.current_mesh is not None and source["mesh_call_index"] == len(self.mesh_calls) - 1:
                self.association_event({
                    "kind": "b720_output",
                    "mesh_call_index": len(self.mesh_calls) - 1,
                    "mesh_index": self.current_mesh["mesh_index"],
                    "source_point_index_from_b6f0": source["source_point_index"],
                    "source_read_pc": source["source_read_pc"],
                    "source_read_address": source["source_address"],
                    "host_input_address_from_b6f0": source["host_input_address"],
                    "host_callback_pc": hex(address),
                    "host_callback_return_pc": hex(return_pc),
                    "scratch_output_address": hex(output),
                })
                event = {
                    "mesh_call_index": len(self.mesh_calls) - 1,
                    "mesh_index": self.current_mesh["mesh_index"],
                    "source_point_index": source["source_point_index"],
                    "source_address": source["source_address"],
                    "source_packed_dword": source["source_packed_dword"],
                    "source_read_pc": source["source_read_pc"],
                    "source_read_size": source["source_read_size"],
                    "host_input_address": source["host_input_address"],
                    "host_input_signed_words": source["host_input_signed_words"],
                    "camera_transform": source.get("camera_transform"),
                    "projection": source.get("projection"),
                    "scratch_output_address": hex(output),
                    "host_callback_pc": hex(address),
                    "host_callback_return_pc": hex(return_pc),
                    "camera_xyz_signed": [fx.s32(uc, a) for a in (0x00A8E71C, 0x00A8E728, 0x00A8E734)],
                    "projected_xy_words": [fx.u16(uc, 0x00A8E740), fx.u16(uc, 0x00A8E746)],
                    "scratch_xy_before_return": [fx.u16(uc, output), fx.u16(uc, output + 2)],
                    "scratch_xy_after_return": None,
                }
                self.real_point_outputs.append(event)
                self.pending_point_output = (return_pc, output, event)

        if address in (fx.ENG_BASE + 0x2C770, fx.ENG_BASE + 0x2C790):
            sp = uc.reg_read(fx.UC_X86_REG_ESP)
            packet = fx.u32(uc, sp + 4)
            face = self.face_index_reads[-1] if self.face_index_reads else None
            face_address = int(face["address"], 16) if face else None
            face_record = next((row for row in (self.current_mesh or {}).get("source_faces", [])
                                if face_address == int(row["address"], 16)), None)
            if face is not None:
                self.association_event({
                    "kind": "packet_emit",
                    "mesh_call_index": len(self.mesh_calls) - 1,
                    "mesh_index": self.current_mesh["mesh_index"] if self.current_mesh else None,
                    "face_record_index": face.get("face_record_index"),
                    "face_record_address": face.get("address"),
                    "face_point_indices": face.get("point_indices"),
                    "packet_address": hex(packet),
                    "packet_wrapper_pc": hex(address),
                })
            self.real_packet_calls.append({
                "mesh_call_index": len(self.mesh_calls) - 1 if self.mesh_calls else None,
                "mesh_index": self.current_mesh["mesh_index"] if self.current_mesh else None,
                "mesh_cell": {k: self.current_mesh[k] for k in ("layer", "row", "column", "cell_file_offset", "source_cell_word")} if self.current_mesh else None,
                "producer_wrapper_rva": hex(address - fx.ENG_BASE),
                "packet_address": hex(packet),
                "face_record_address": face.get("address") if face else None,
                "face_record_index": face.get("face_record_index") if face else None,
                "face_word": face.get("word") if face else None,
                "point_indices": face.get("point_indices") if face else None,
                "face_source_record": face_record,
                "route": self.current_emit,
            })

    def on_read(self, uc, access, address, size, value, user):
        pc = uc.reg_read(fx.UC_X86_REG_EIP)
        if fx.ENG_BASE + 0x6F4E8 <= address < fx.ENG_BASE + 0x6F4E8 + 512 * 4:
            self.mesh_color_reads.append({"pc": hex(pc), "address": hex(address), "size": size,
                                          "value": hex(fx.u32(uc, address)),
                                          "mesh_call_index": len(self.mesh_calls) - 1 if self.mesh_calls else None})
        if MAP_BASE + 0x106818 <= address < MAP_BASE + 0x11E800 and size == 4:
            self.map_grid_reads.append({"pc": hex(pc), "pc_rva": pc - fx.ENG_BASE,
                                        "address": hex(address), "value": fx.u32(uc, address)})
        if self.current_mesh is not None:
            lo = self.current_mesh["point_start"]
            hi = lo + self.current_mesh["point_count"] * 4
            if lo <= address < hi:
                event = {
                    "pc": hex(uc.reg_read(fx.UC_X86_REG_EIP)),
                    "address": hex(address), "size": size,
                    "source_point_index": (address - lo) // 4,
                    "value": hex(int.from_bytes(bytes(uc.mem_read(address, size)), "little")),
                    "mesh_index": self.current_mesh["mesh_index"],
                    "face_start": hex(self.current_mesh["face_start"]),
                }
                self.dynamic_source_reads.append(event)
                if self.current_mesh.get("mesh_index") == 156:
                    self.association_event({
                        "kind": "source_point_read",
                        "mesh_call_index": len(self.mesh_calls) - 1,
                        "mesh_index": self.current_mesh["mesh_index"],
                        "source_point_index": event["source_point_index"],
                        "source_read_pc": event["pc"],
                        "source_read_address": event["address"],
                        "source_read_size": event["size"],
                    })
                self.current_source_read = event
        super().on_read(uc, access, address, size, value, user)

    def on_write(self, uc, access, address, size, value, user):
        pc = uc.reg_read(fx.UC_X86_REG_EIP)
        self.writes.append((pc, address, size, value))
        if self.current_mesh is not None and address == self.current_mesh["visit"] + 0x24:
            self.winding_field_writes.append({
                "mesh_call_index": len(self.mesh_calls) - 1,
                "mesh_index": self.current_mesh["mesh_index"],
                "pc": hex(pc),
                "pc_rva": hex(pc - fx.ENG_BASE) if pc >= fx.ENG_BASE else hex(pc - fx.EXE_BASE),
                "size": size,
                "value": hex(value & 0xff),
            })
        if OT_BASE <= address < OT_BASE + OT_BYTES:
            self.ot_writes.append({"mode": self.mode, "pc": hex(pc), "address": hex(address),
                                   "size": size, "value": hex(value & 0xFFFFFFFF)})
        if PACKET_BASE <= address < PACKET_BASE + PACKET_BYTES:
            self.packet_writes.append({"mode": self.mode, "pc": hex(pc), "address": hex(address),
                                       "size": size, "value": hex(value & 0xFFFFFFFF)})
        super().on_write(uc, access, address, size, value, user)


def main(argv=None) -> int:
    global REPO, ORIGINALS, ARCHIVE, OUTPUT_DIR, FIXTURE
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--eng1", required=True, type=Path)
    parser.add_argument("--archive", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args(argv)
    REPO = repo_path(args.repo_root)
    if REPO != SCRIPT_REPO.resolve():
        raise ValueError(f"fixture location and --repo-root disagree: {SCRIPT_REPO} vs {REPO}")
    exe_path = input_file(args.exe, "HERCULES.EXE")
    eng_path = input_file(args.eng1, "ENG1.DLL")
    ARCHIVE = input_file(args.archive, "HERCULES.FS")
    if exe_path.name.upper() != "HERCULES.EXE" or eng_path.name.upper() != "ENG1.DLL":
        raise ValueError("the supplied oracle files must retain HERCULES.EXE and ENG1.DLL names")
    if exe_path.parent != eng_path.parent:
        raise ValueError("the bounded original fixture requires EXE and ENG1.DLL in one explicit originals directory")
    ORIGINALS = exe_path.parent
    for path, expected, label in ((exe_path, fx.EXPECTED["HERCULES.EXE"], "HERCULES.EXE"),
                                  (eng_path, fx.EXPECTED["ENG1.DLL"], "ENG1.DLL"),
                                  (ARCHIVE, ARCHIVE_SHA, "HERCULES.FS")):
        if sha(path.read_bytes()) != expected:
            raise RuntimeError(f"{label} identity mismatch")
    OUTPUT_DIR = create_fresh_output(REPO, args.output)
    fx.configure(REPO, ORIGINALS, OUTPUT_DIR, [str(Path(sys.argv[0])), *list(argv or sys.argv[1:])])
    data = read_map()
    uc = fx.Uc(fx.UC_ARCH_X86, fx.UC_MODE_32)
    exe, eng = fx.PE(str(exe_path)), fx.PE(str(eng_path))
    exec_ranges = fx.map_pe(uc, exe, fx.EXE_BASE) + fx.map_pe(uc, eng, fx.ENG_BASE)
    # SYNTH ends immediately before the fixture stack; the archived map is a
    # separate read-only-backed emulated range after the stack.
    uc.mem_map(BASE, 0x1000000, fx.UC_PROT_READ | fx.UC_PROT_WRITE)
    uc.mem_map(STACK, fx.STACK_SIZE, fx.UC_PROT_READ | fx.UC_PROT_WRITE)
    uc.mem_map(SENTINEL, 0x1000, fx.UC_PROT_READ | fx.UC_PROT_WRITE | fx.UC_PROT_EXEC)
    map_asset(uc, data)

    # Publish the original full host transport before assigning any fixture data.
    callback_table = exe.read_rva(fx.TABLE_RVA, fx.TABLE_BYTES)
    if sha(callback_table) != "52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8":
        raise RuntimeError("original 250-DWORD host transport changed")
    uc.mem_write(fx.CALLBACKS_DEST, callback_table)
    context = bytearray(0x80)
    # Forest's archive producer uses config_row 2; unknown_040 is the native
    # producer's existing early-out gate and is zero for the finite input case.
    struct.pack_into("<b", context, 0, 2)
    struct.pack_into("<I", context, 0x38, 1)
    struct.pack_into("<I", context, 0x40, 0)
    uc.mem_write(CONTEXT, bytes(context))
    fx.put32(uc, fx.INTERFACE_PREFIX + 4, CONTEXT)
    fx.put32(uc, fx.ENG_BASE + 0x70ED4, MAP_BASE)
    fx.put16(uc, fx.ENG_BASE + 0x70CFE, 0)
    fx.put16(uc, fx.ENG_BASE + 0x70D01, 0)

    trace = Trace(uc, exec_ranges, "forest_real_row_native_traversal")
    trace.mode = "resource_rebind_20670"
    fx.invoke(uc, trace, fx.ENG_BASE + 0x20670)
    header = {hex(rva): fx.u32(uc, fx.ENG_BASE + rva)
              for rva in (0x70D28, 0x70D30, 0x70D34, 0x70D44, 0x70D50, 0x70D54, 0x70D58)}
    expected = {
        "0x70d28": MAP_BASE + 0x2892C,
        "0x70d30": MAP_BASE + 0xB1B4,
        "0x70d34": MAP_BASE + 0x44,
        "0x70d44": MAP_BASE + 0x285B8,
        "0x70d50": MAP_BASE + 0x27B74,
        "0x70d54": MAP_BASE + 0x26D80,
        "0x70d58": MAP_BASE + 0x26D30,
    }
    if header != expected:
        raise RuntimeError(f"native 20670 resource pointers differ: {header!r}")

    # Bind the actual archived row0 bytes into its observed table slot, then let
    # native 208F0 decode that 24-byte row header into the layer row pointers.
    row_block_offset = 0x106800
    row_pointer = MAP_BASE + row_block_offset
    if data[row_block_offset:row_block_offset + 24] != struct.pack("<H11H", 0, *([95] * 11)):
        raise RuntimeError("unexpected real row header; not executing a fixture row")
    fx.put32(uc, fx.ENG_BASE + 0x71E40, row_pointer)
    trace.mode = "resource_row_consumer_208f0"
    fx.invoke(uc, trace, fx.ENG_BASE + 0x208F0)
    row_grid = fx.u32(uc, fx.ENG_BASE + 0x70DCC)
    if row_grid != row_pointer + 24:
        raise RuntimeError(f"native 208F0 row grid mismatch: {row_grid:#x}")

    # Host perspective is a transparent synthetic camera/context. The map and
    # all geometry/material/UV/lookup bytes remain exact archived bytes.
    matrix = [4096, 0, 0, 0, 4096, 0, 0, 0, 4096]
    uc.mem_write(MATRIX_SOURCE, struct.pack("<9h", *matrix))
    for i, value in enumerate((0, 0, 0)):
        fx.put32(uc, TRANSLATION_SOURCE + 0x14 + 4 * i, value)
    trace.mode = "setup_projection_callbacks"
    fx.invoke(uc, trace, fx.EXE_BASE + 0xB880, (MATRIX_SOURCE,))
    fx.invoke(uc, trace, fx.EXE_BASE + 0xB8F0, (TRANSLATION_SOURCE,))
    fx.invoke(uc, trace, fx.EXE_BASE + 0xB970, (160, 120))
    fx.invoke(uc, trace, fx.EXE_BASE + 0xB990, (256,))

    # Explicit finite camera cell: scan around archived layer0 row14/column10.
    # 2DA90 retains its real six-layer mode0 traversal and row/column stepping.
    camera = [0, 0, 0, 0, 0, 0, 14, 10]
    uc.mem_write(fx.ENG_BASE + 0x70C60, struct.pack("<8i", *camera))
    fx.put32(uc, fx.ENG_BASE + 0x70CA0, 0)
    fx.put32(uc, fx.ENG_BASE + 0x70CA8, -256)
    fx.put32(uc, fx.ENG_BASE + 0x70D1C, 0)
    fx.put32(uc, fx.ENG_BASE + 0x71EC4, OT_BASE)
    fx.put32(uc, fx.PACKET_CURSOR_GLOBAL, PACKET_BASE)
    # Parent instruction: no frame-capacity assumption. This is only an
    # emulator backing range; an overrun faults and aborts the run visibly.
    trace.mode = "native_2da90_traversal"
    try:
        call = fx.invoke(uc, trace, fx.ENG_BASE + 0x2DA90)
    except Exception as error:
        context = {
            "error": repr(error),
            "tail_pcs": [[hex(address), size] for _mode, address, size, _name in trace.original_pcs[-24:]],
            "tail_all_pcs": [hex(address) for address in trace.all_pcs[-32:]],
            "last_emitter": trace.current_mesh,
            "last_face": trace.face_index_reads[-1] if trace.face_index_reads else None,
            "last_point_input": trace.vertex_input_events[-1] if trace.vertex_input_events else None,
            "last_callback": trace.vertex_output_events[-1] if trace.vertex_output_events else None,
        }
        (OUTPUT_DIR / "native_real_map_failure.json").write_text(json.dumps(context, indent=2) + "\n", encoding="utf-8")
        raise

    # Preserve bounded packet bytes and the exact OT word(s) that link each
    # sampled packet. FT4's XY words are interleaved at byte offsets 12/20/28/36.
    packet_samples = []
    for packet_call in trace.real_packet_calls[:12]:
        packet_address = int(packet_call["packet_address"], 16)
        raw = bytes(uc.mem_read(packet_address, 40))
        packet_samples.append({
            "packet_call": packet_call,
            "raw_40_bytes": raw.hex(),
            "xy_words": [[s16(raw, off), s16(raw, off + 2)] for off in (8, 16, 24, 32)],
            "tag_dword": hex(struct.unpack_from("<I", raw, 0)[0]),
            "ot_writes_linking_packet": [w for w in trace.ot_writes
                                         if int(w["value"], 16) == packet_address],
        })

    # Select a single archived 8-point / 3-face resource that repeats point
    # indices across faces. Keep the selection predicate tied to observed bytes
    # and assert the expected discriminator rather than selecting by coords.
    selected_mesh_index = next((i for i, mesh in enumerate(trace.mesh_calls)
                                if mesh["mesh_index"] == 156 and mesh["point_count"] == 8 and
                                mesh["face_count"] == 3 and mesh["cell_file_offset"] == 0x10D33C), None)
    if selected_mesh_index is None:
        raise RuntimeError("archived repeated/reordered three-face discriminator not found")
    selected_mesh = trace.mesh_calls[selected_mesh_index]
    selected_point_outputs = [event for event in trace.real_point_outputs
                              if event["mesh_call_index"] == selected_mesh_index]
    if len(selected_point_outputs) != selected_mesh["point_count"]:
        raise RuntimeError("selected source points did not each reach B720 exactly once")
    selected_packets = []
    for packet_call in trace.real_packet_calls:
        if packet_call["mesh_call_index"] != selected_mesh_index:
            continue
        packet_address = int(packet_call["packet_address"], 16)
        raw = bytes(uc.mem_read(packet_address, 40))
        xy = [[s16(raw, off), s16(raw, off + 2)] for off in (8, 16, 24, 32)]
        by_point = {event["source_point_index"]: event for event in selected_point_outputs}
        face = packet_call["face_source_record"]
        corner_rows = []
        for slot, point_index in enumerate(face["point_indices"]):
            point = by_point[point_index]
            if xy[slot] != point["projected_xy_words"]:
                raise RuntimeError(f"packet corner {slot} does not match the source index's projected XY")
            corner_rows.append({"corner": slot, "source_point_index": point_index,
                                "source_address_file_offset": point["source_address"],
                                "projected_xy_words": point["projected_xy_words"],
                                "packet_xy_words": xy[slot]})
        packet_events = [w for w in trace.packet_writes
                         if packet_address <= int(w["address"], 16) < packet_address + 40]
        ot_link = next((w for w in trace.ot_writes if int(w["value"], 16) == packet_address), None)
        if ot_link is None:
            raise RuntimeError(f"packet {packet_address:#x} has no original OT-head insertion")
        bucket_address = int(ot_link["address"], 16)
        selected_packets.append({
            "mesh_call_index": selected_mesh_index,
            "mesh_index_table_index": selected_mesh["mesh_index"],
            "face_record_index": face["index"],
            "face_file_offset": face["file_offset"],
            "face_packed_index_word": face["packed_index_word"],
            "face_point_indices": face["point_indices"],
            "face_second_dword": face["second_dword"],
            "route": packet_call["route"],
            "wrapper_rva": packet_call["producer_wrapper_rva"],
            "packet_address": hex(packet_address),
            "packet_raw_40_bytes": raw.hex(),
            "packet_tag_previous_link": hex(struct.unpack_from("<I", raw, 0)[0]),
            "packet_xy_words_by_corner": xy,
            "corner_associations": corner_rows,
            "packet_write_instructions": packet_events,
            "ot_insert": {"writer_pc": ot_link["pc"], "bucket_address": hex(bucket_address),
                          "bucket_index_from_fixture_base": (bucket_address - OT_BASE) // 4,
                          "new_head_packet": hex(packet_address),
                          "packet_tag_previous_link": hex(struct.unpack_from("<I", raw, 0)[0])},
        })
    if [p["face_point_indices"] for p in selected_packets] != [[5, 4, 7, 6], [3, 2, 5, 4], [0, 1, 3, 2]]:
        raise RuntimeError("selected face sequence no longer discriminates reuse/reorder")

    # Two additional already-executed native resource calls discriminate the
    # source mesh emitter's face culling: one emits only its first face at
    # winding 0; one has winding 1 and emits none. These are slices of the same
    # bounded traversal, not separately invented producer inputs.
    auxiliary_cases = []
    for selector in (
            {"mesh_index": 284, "point_count": 10, "face_count": 3, "winding_024": 0, "expected_packets": 1},
            {"mesh_index": 17, "point_count": 10, "face_count": 4, "winding_024": 1, "expected_packets": 0},):
        auxiliary_index = next((i for i, row in enumerate(trace.mesh_calls)
                                if all(row.get(key) == value for key, value in selector.items()
                                       if key != "expected_packets")), None)
        if auxiliary_index is None:
            raise RuntimeError(f"native culling/winding discriminator not found: {selector}")
        auxiliary_mesh = trace.mesh_calls[auxiliary_index]
        auxiliary_outputs = [event for event in trace.real_point_outputs
                             if event["mesh_call_index"] == auxiliary_index]
        if len(auxiliary_outputs) != auxiliary_mesh["point_count"]:
            raise RuntimeError("auxiliary resource did not produce one B720 result per point")
        auxiliary_packets = []
        for packet_call in trace.real_packet_calls:
            if packet_call["mesh_call_index"] != auxiliary_index:
                continue
            packet_address = int(packet_call["packet_address"], 16)
            raw = bytes(uc.mem_read(packet_address, 40))
            xy = [[s16(raw, off), s16(raw, off + 2)] for off in (8, 16, 24, 32)]
            face = packet_call["face_source_record"]
            by_point = {event["source_point_index"]: event for event in auxiliary_outputs}
            associations = []
            for corner, point_index in enumerate(face["point_indices"]):
                point = by_point[point_index]
                if point["scratch_xy_after_return"] != xy[corner]:
                    raise RuntimeError("auxiliary packet corner does not match the indexed original point")
                associations.append({"corner": corner, "source_point_index": point_index,
                                     "source_address_file_offset": point["source_address"],
                                     "projected_xy_words": point["scratch_xy_after_return"],
                                     "packet_xy_words": xy[corner]})
            ot_link = next((w for w in trace.ot_writes if int(w["value"], 16) == packet_address), None)
            if ot_link is None:
                raise RuntimeError("auxiliary packet has no original OT-head insertion")
            auxiliary_packets.append({
                "face_record_index": face["index"],
                "face_file_offset": face["file_offset"],
                "face_packed_index_word": face["packed_index_word"],
                "face_point_indices": face["point_indices"],
                "packet_address": hex(packet_address),
                "packet_raw_40_bytes": raw.hex(),
                "packet_xy_words_by_corner": xy,
                "corner_associations": associations,
                "ot_insert": {"writer_pc": ot_link["pc"], "bucket_address": ot_link["address"],
                              "bucket_index_from_fixture_base": (int(ot_link["address"], 16) - OT_BASE) // 4,
                              "packet_tag_previous_link": hex(struct.unpack_from("<I", raw, 0)[0])},
            })
        if len(auxiliary_packets) != selector["expected_packets"]:
            raise RuntimeError(f"native culling count differs: {selector}, got {len(auxiliary_packets)}")
        auxiliary_cases.append({"selector": selector, "mesh_call_index": auxiliary_index,
                                "mesh": auxiliary_mesh, "point_outputs": auxiliary_outputs,
                                "packets": auxiliary_packets})

    selected_aux_indices = [selected_mesh_index] + [case["mesh_call_index"] for case in auxiliary_cases]
    prep_results = [event for event in trace.prep_results
                    if event["mesh_call_index"] in selected_aux_indices]
    prep_by_call = {event["mesh_call_index"]: event for event in prep_results}
    if set(prep_by_call) != set(selected_aux_indices):
        raise RuntimeError("selected resource cases lack a completed 2E3B0 entry/return observation")

    report = {
        "scope": "Hash-pinned original HERCULES.EXE/ENG1.DLL x86 resource consumer and ENG1 2DA90 traversal over archived FOREST MAP_PSX.BIN bytes; the camera/context is synthetic and declared.",
        "result": "EXECUTED",
        "hashes": {"HERCULES.EXE": fx.EXPECTED["HERCULES.EXE"], "ENG1.DLL": fx.EXPECTED["ENG1.DLL"],
                   "HERCULES.FS": ARCHIVE_SHA, "MAP_PSX.BIN": MAP_SHA},
        "map_location": {"archive_offset": MAP_OFFSET, "length": MAP_LENGTH, "map_base": hex(MAP_BASE)},
        "native_resource_setup": {"entry": "ENG1+0x20670", "header_pointers": {k: hex(v) for k, v in header.items()},
                                   "row_entry": "ENG1+0x208F0", "actual_row_block_file_offset": hex(row_block_offset),
                                   "row_grid_file_offset": hex(row_block_offset + 24), "grid_address": hex(row_grid)},
        "camera_context": {"label": "synthetic finite input, not shipped live state", "config_row": 2,
                           "transform_words": camera, "camera_z": -256, "center": [160, 120], "focal": 256},
        "traversal_call": call,
        "mesh_call_count": len(trace.mesh_calls),
        "mesh_color_reads": trace.mesh_color_reads,
        "mesh_calls": trace.mesh_calls,
        "dynamic_source_reads": trace.dynamic_source_reads,
        "dynamic_source_reads_first_250": trace.dynamic_source_reads[:250],
        "face_reads_first_250": trace.face_index_reads[:250],
        "point_outputs_first_250": trace.vertex_output_events[:250],
        "packet_calls_first_250": trace.packet_calls[:250],
        "original_packet_calls": trace.real_packet_calls,
        "packet_samples_first_12": packet_samples,
        "ot_writes_first_24": trace.ot_writes[:24],
        "projection_setup_calls": trace.projection_state_updates,
        "selected_real_resource_case": {
            "mesh_call_index": selected_mesh_index,
            "mesh": selected_mesh,
            "point_outputs": selected_point_outputs,
            "packets": selected_packets,
        },
        "auxiliary_real_resource_cases": auxiliary_cases,
        "selected_and_auxiliary_winding_writes": [event for event in trace.winding_field_writes
                                                   if event["mesh_call_index"] in
                                                   [selected_mesh_index] + [case["mesh_call_index"] for case in auxiliary_cases]],
        "selected_and_auxiliary_2e3b0_results": prep_results,
        "association_events_selected_call": [event for event in trace.association_events
                                                if event.get("mesh_call_index") == selected_mesh_index],
        "captured_real_point_outputs": trace.real_point_outputs,
        "packet_write_count": len(trace.packet_writes),
        "ot_write_count": len(trace.ot_writes),
        "original_executed_rvas": sorted(set(hex(rva) for mode, rva, _size, _span in trace.original_pcs if mode == "native_2da90_traversal")),
        "executed_spans": sorted(set(span for mode, _rva, _size, span in trace.original_pcs)),
        "faults": trace.faults,
        "limitations": ["This first report establishes resource setup/traversal only; source-vs-native candidate differential remains open.",
                        "Emulator packet/OT backing sizes are bounded test memory, not an original frame-capacity claim.",
                        "No actor identity, live camera pose, general level initialization, recovery state, or renderer implementation claim."],
    }
    output = OUTPUT_DIR / "native_real_map_report.json"
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": str(output), "mesh_calls": len(trace.mesh_calls),
                      "packet_writes": len(trace.packet_writes), "ot_writes": len(trace.ot_writes),
                      "spans": report["executed_spans"]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
