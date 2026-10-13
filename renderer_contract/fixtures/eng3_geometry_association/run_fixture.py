"""Execute the pinned ENG3 FT4 producer over one finite synthetic primitive."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import struct
import sys
from pathlib import Path

DEFAULT_DEPS = Path(r"C:\tools\hercules\renderer-oracle-python")
DEFAULT_ANALYSIS_DEPS = Path(r"C:\tools\hercules\python")

def _configure():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path,
                        help="checkout containing renderer_contract/, scripts/pe.py, and work/")
    parser.add_argument("--exe", required=True, type=Path, help="pinned original HERCULES.EXE")
    parser.add_argument("--eng3", required=True, type=Path, help="pinned original ENG3.DLL")
    parser.add_argument("--output", required=True, type=Path,
                        help="new output directory below this checkout's ignored work/")
    parser.add_argument("--deps", type=Path, default=DEFAULT_DEPS)
    parser.add_argument("--analysis-deps", type=Path, default=DEFAULT_ANALYSIS_DEPS)
    args = parser.parse_args()
    args.repo_root = args.repo_root.resolve()
    args.exe = args.exe.resolve()
    args.eng3 = args.eng3.resolve()
    args.output = args.output.resolve()
    args.deps = args.deps.resolve()
    args.analysis_deps = args.analysis_deps.resolve()
    if not (args.repo_root / "renderer_contract").is_dir() or not (args.repo_root / "scripts" / "pe.py").is_file():
        parser.error("--repo-root must contain renderer_contract/ and scripts/pe.py")
    try:
        args.output.relative_to((args.repo_root / "work").resolve())
    except ValueError:
        parser.error("--output must stay below --repo-root/work")
    if args.output.exists():
        parser.error("--output must be a fresh, nonexistent directory")
    if not args.output.parent.is_dir():
        parser.error("--output parent must already exist")
    for label, source in (("--exe", args.exe), ("--eng3", args.eng3)):
        if not source.is_file():
            parser.error(f"{label} must name an existing regular file")
    if not args.deps.is_dir() or not args.analysis_deps.is_dir():
        parser.error("the pinned Unicorn and analysis dependency directories must exist")
    os.environ["HERCULES_PYTHON_DEPS"] = str(args.analysis_deps)
    sys.path[:0] = [str(args.deps), str(args.analysis_deps), str(args.repo_root / "scripts")]
    return args

ARGS = _configure()
REPO = ARGS.repo_root
EXE_PATH = ARGS.exe
ENG3_PATH = ARGS.eng3
OUT = ARGS.output
INTERFACE_COPY_EVIDENCE = REPO / "renderer_contract" / "evidence" / "native_interface_copy.json"
PREDICTION_PATH = Path(__file__).with_name("prediction.md")
TEST_HELPER_PATH = Path(__file__).resolve().parents[2] / "tests" / "test_original_eng3_geometry_association.py"
from pe import PE  # noqa: E402
import unicorn  # noqa: E402
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI,
)

EXE_BASE = 0x00400000
ENG_BASE = 0x10000000
SYNTH = 0x0C000000
OBJECT = SYNTH + 0x1000
STREAM = SYNTH + 0x2000
CONTEXT = SYNTH + 0x3000
MATRIX = SYNTH + 0x4000
OT = SYNTH + 0x8000
PACKET = SYNTH + 0x10000
STACK = 0x0D000000
STACK_SIZE = 0x10000
STACK_TOP = STACK + STACK_SIZE - 0x100
SENTINEL = 0x0E000000
CALLBACK_RVA = 0
CALLBACK_BYTES = 0
MATRIX_PTR_RVA = 0x394B8
CONTEXT_PTR_RVA = 0x431C0
PACKET_CURSOR_RVA = 0x43164
VIEW_PTR_RVA = 0x41448

ORIGINALS_SHA = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG3.DLL": "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
}
PUBLIC_INPUTS = {
    "renderer_contract/cross_module_geometry.md": "",
    "renderer_contract/evidence/upstream_geometry_modules.json": "",
    "renderer_contract/evidence/module_packets.json": "",
    "renderer_contract/evidence/eng3_handlers.json": "",
    "renderer_contract/evidence/native_interface_copy.json": "",
}

# Whole producer plus its exact copied wrappers, and only the host callback
# bodies used by this producer. The wrappers jump/call through the copied table.
ENG_RANGES = [
    (0x231A0, 0x293, "ENG3 FT4 producer"),
    (0x1FCA0, 0x0F, "ENG3 slot101 wrapper"),
    (0x1FCB0, 0x19, "ENG3 slot102 wrapper"),
    (0x1FD00, 0x06, "ENG3 slot106 wrapper"),
    (0x1FD10, 0x06, "ENG3 slot107 wrapper"),
    (0x1FD20, 0x0F, "ENG3 slot108 wrapper"),
    (0x1FD40, 0x0F, "ENG3 slot110 wrapper"),
    (0x1FD50, 0x0F, "ENG3 slot111 wrapper"),
    (0x1FD80, 0x0F, "ENG3 slot114 wrapper"),
    (0x1FD90, 0x0F, "ENG3 slot117 wrapper"),
]
HOST_RANGES = [
    (0xB2F0, 0x1F, "HERC slot111 B2F0"), (0xB320, 0x10F, "HERC slot107 B320"),
    (0xB430, 0x6A, "HERC slot102 B430"), (0xB5C0, 0x47, "HERC slot117 B5C0"),
    (0xB6F0, 0x24, "HERC slot101 B6F0"), (0xB720, 0x1A, "HERC slot114 B720"),
    (0xB740, 0xCA, "HERC slot106 B740"), (0xB840, 0x23, "HERC slot103 B840"),
    (0xB880, 0x32, "HERC slot108 B880"), (0xB8C0, 0x28, "HERC slot110 B8C0"),
    (0xB920, 0x4E, "HERC slot106 B920"),
    (0xB970, 0x14, "HERC slot174 B970 synthetic projection setup"),
    (0xB990, 0x0A, "HERC slot175 B990 synthetic projection setup"),
]
EXECUTABLES = [(EXE_BASE + r, EXE_BASE + r + n, label) for r, n, label in HOST_RANGES]
EXECUTABLES += [(ENG_BASE + r, ENG_BASE + r + n, label) for r, n, label in ENG_RANGES]
WRAPPER_SLOTS = {
    0x1FCA0: 101, 0x1FCB0: 102, 0x1FD00: 106, 0x1FD10: 107,
    0x1FD20: 108, 0x1FD40: 110, 0x1FD50: 111, 0x1FD80: 114, 0x1FD90: 117,
}
HOST_NAMES = {rva: label for rva, _size, label in HOST_RANGES}
HOST_ARG_COUNTS = {0xB2F0: 1, 0xB320: 0, 0xB430: 3, 0xB5C0: 1, 0xB6F0: 1,
                   0xB720: 1, 0xB740: 0, 0xB880: 1, 0xB8C0: 1, 0xB920: 0,
                   0xB970: 2, 0xB990: 1}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def get_u16(uc: Uc, address: int) -> int:
    return struct.unpack("<H", bytes(uc.mem_read(address, 2)))[0]


def get_s16(uc: Uc, address: int) -> int:
    return struct.unpack("<h", bytes(uc.mem_read(address, 2)))[0]


def get_u32(uc: Uc, address: int) -> int:
    return struct.unpack("<I", bytes(uc.mem_read(address, 4)))[0]


def put_u16(uc: Uc, address: int, value: int) -> None:
    uc.mem_write(address, struct.pack("<H", value & 0xFFFF))


def put_u32(uc: Uc, address: int, value: int) -> None:
    uc.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))


def map_pe(uc: Uc, image: PE, base: int) -> None:
    size = image.pe.OPTIONAL_HEADER.SizeOfImage
    lo = base & ~0xFFF
    hi = (base + size + 0xFFF) & ~0xFFF
    uc.mem_map(lo, hi - lo)
    payload = image.pe.get_memory_mapped_image()
    uc.mem_write(base, payload[:size])


class Trace:
    def __init__(self, uc: Uc):
        self.uc = uc
        self.instruction_count = 0
        self.pcs: list[dict] = []
        self.reads: list[dict] = []
        self.writes: list[dict] = []
        self.calls: list[dict] = []
        self.wrapper_events: list[dict] = []
        self.host_call_order: list[dict] = []
        self.point_events: list[dict] = []
        self.projection_events: list[dict] = []
        self.batch_projection: dict | None = None
        self.single_projection: dict | None = None
        self.batch_packet_xy: list | None = None
        self.single_packet_xy: list | None = None
        self.context_translation: dict | None = None
        self.packet_writes: list[dict] = []
        self.ot_writes: list[dict] = []
        self.exception: str | None = None
        self.mode = "setup"
        self.active_lane: int | None = None
        self.resource_range = (STREAM, STREAM + 0x100)
        self.packet_range = (PACKET, PACKET + 0x100)
        self.ot_range = (OT, OT + 0x4000)
        uc.hook_add(UC_HOOK_CODE, self.on_code)
        uc.hook_add(UC_HOOK_MEM_READ, self.on_read)
        uc.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def on_code(self, uc, address, size, _user):
        self.instruction_count += 1
        if address == SENTINEL:
            uc.emu_stop()
            return
        span = next((label for lo, hi, label in EXECUTABLES if lo <= address < hi), None)
        if span is None:
            base = ENG_BASE if ENG_BASE <= address < ENG_BASE + 0x100000 else EXE_BASE
            rva = address - base
            self.exception = f"unsupported code entry {address:#x} RVA {rva:#x} ({self.mode})"
            raise RuntimeError(self.exception)
        self.pcs.append({"pc": hex(address), "size": size, "span": span, "mode": self.mode})
        rva_eng = address - ENG_BASE
        if rva_eng in WRAPPER_SLOTS:
            esp = uc.reg_read(UC_X86_REG_ESP)
            n_args = {101: 1, 102: 3, 106: 0, 107: 0, 108: 1, 110: 1,
                      111: 1, 114: 1, 117: 1}[WRAPPER_SLOTS[rva_eng]]
            self.wrapper_events.append({"module_rva": hex(rva_eng), "slot": WRAPPER_SLOTS[rva_eng],
                                        "return_address": hex(get_u32(uc, esp)),
                                        "args": [hex(get_u32(uc, esp + 4 + 4 * i)) for i in range(n_args)],
                                        "mode": self.mode})
        if EXE_BASE <= address < EXE_BASE + 0x1000000 and address - EXE_BASE in HOST_NAMES:
            host_rva = address - EXE_BASE
            esp = uc.reg_read(UC_X86_REG_ESP)
            self.host_call_order.append({"callback": HOST_NAMES[address - EXE_BASE],
                                         "rva": hex(host_rva), "mode": self.mode,
                                         "return_address": hex(get_u32(uc, esp)),
                                         "args": [hex(get_u32(uc, esp + 4 + 4 * i))
                                                  for i in range(HOST_ARG_COUNTS[host_rva])]})
        if address in (EXE_BASE + 0xB430, EXE_BASE + 0xB320, EXE_BASE + 0xB5C0,
                       EXE_BASE + 0xB6F0, EXE_BASE + 0xB920, EXE_BASE + 0xB720):
            esp = uc.reg_read(UC_X86_REG_ESP)
            callback = address - EXE_BASE
            if callback == 0xB430:
                args = [get_u32(uc, esp + 4 + 4 * i) for i in range(3)]
                self.calls.append({"callback": "B430", "pc": hex(address), "args": [hex(a) for a in args],
                                   "triplets_s16": [[get_s16(uc, a + 2 * j) for j in range(3)] for a in args]})
                self.point_events.append({"kind": "batch_input", "addresses": [hex(a) for a in args],
                                          "triplets_s16": [[get_s16(uc, a + 2 * j) for j in range(3)] for a in args]})
            elif callback == 0xB6F0:
                arg = get_u32(uc, esp + 4)
                self.calls.append({"callback": "B6F0", "pc": hex(address), "arg": hex(arg),
                                   "triplet_s16": [get_s16(uc, arg + 2 * j) for j in range(3)]})
                self.point_events.append({"kind": "single_input", "address": hex(arg),
                                          "triplet_s16": [get_s16(uc, arg + 2 * j) for j in range(3)]})
            elif callback == 0xB5C0:
                arg = get_u32(uc, esp + 4)
                self.calls.append({"callback": "B5C0", "pc": hex(address), "packet_arg": hex(arg)})
            elif callback == 0xB920:
                self.calls.append({"callback": "B920", "pc": hex(address)})
                self.projection_events.append({"stage": "B920_before", "camera_input_words": [
                    get_s16(uc, 0x00A8E708), get_s16(uc, 0x00A8E70E), get_s16(uc, 0x00A8E714)]})
            elif callback == 0xB720:
                arg = get_u32(uc, esp + 4)
                self.calls.append({"callback": "B720", "pc": hex(address), "output_arg": hex(arg)})

        if address == ENG_BASE + 0x2338D:
            self.batch_projection = {
                "capture_pc": hex(address),
                "camera_xyz_dwords_by_vertex": [
                    [get_u32(uc, 0x00A8E71C + 4 * i), get_u32(uc, 0x00A8E728 + 4 * i),
                     get_u32(uc, 0x00A8E734 + 4 * i)] for i in range(3)],
                "projected_xy_words_by_vertex": [
                    [get_u16(uc, 0x00A8E740 + 2 * i), get_u16(uc, 0x00A8E746 + 2 * i)]
                    for i in range(3)],
                "B430_arguments": self.calls[-1].get("args") if self.calls and self.calls[-1].get("callback") == "B430" else None,
            }
            self.projection_events.append({"stage": "B320_return", **self.batch_projection})
        if address == ENG_BASE + 0x231DC:
            ctx = get_u32(uc, ENG_BASE + CONTEXT_PTR_RVA)
            self.context_translation = {"capture_pc": hex(address), "context": hex(ctx),
                                        "translation_dwords_after_object_adjustment": [
                                            get_u32(uc, ctx + 0x78), get_u32(uc, ctx + 0x7C),
                                            get_u32(uc, ctx + 0x80), get_u32(uc, ctx + 0x84)]}
        if address == ENG_BASE + 0x233AF:
            packet = get_u32(uc, ENG_BASE + PACKET_CURSOR_RVA)
            self.batch_packet_xy = [[get_u16(uc, packet + off), get_u16(uc, packet + off + 2)]
                                    for off in (0x08, 0x10, 0x18)]
            self.projection_events.append({"stage": "B5C0_return_packet_xy", "pc": hex(address),
                                           "packet": hex(packet), "packet_xy_by_vertex": self.batch_packet_xy})
        if address == ENG_BASE + 0x233CE:
            self.single_projection = {
                "capture_pc": hex(address),
                "camera_xyz_dwords": [get_u32(uc, 0x00A8E71C), get_u32(uc, 0x00A8E728), get_u32(uc, 0x00A8E734)],
                "projected_xy_words": [get_u16(uc, 0x00A8E740), get_u16(uc, 0x00A8E746)],
                "B6F0_argument": next((row.get("arg") for row in reversed(self.calls)
                                        if row.get("callback") == "B6F0"), None),
            }
            self.projection_events.append({"stage": "B920_return", **self.single_projection})
        if address == ENG_BASE + 0x2340F:
            packet = get_u32(uc, ENG_BASE + PACKET_CURSOR_RVA)
            self.single_packet_xy = [get_u16(uc, packet + 0x20), get_u16(uc, packet + 0x22)]
            self.projection_events.append({"stage": "B720_return_packet_xy", "pc": hex(address),
                                           "packet": hex(packet), "packet_xy": self.single_packet_xy})

    def on_read(self, uc, _access, address, size, value, _user):
        pc = uc.reg_read(UC_X86_REG_EIP)
        raw = bytes(uc.mem_read(address, size))
        val = int.from_bytes(raw, "little")
        self.reads.append({"pc": hex(pc), "address": hex(address), "size": size, "value": hex(val)})
        if STREAM <= address < STREAM + 0x100:
            self.reads[-1]["owner"] = "synthetic FT4 resource stream"
        elif CONTEXT <= address < CONTEXT + 0x100:
            self.reads[-1]["owner"] = "synthetic producer context"
        elif OBJECT <= address < OBJECT + 0x100:
            self.reads[-1]["owner"] = "synthetic object/context"
        elif PACKET <= address < PACKET + 0x100:
            self.reads[-1]["owner"] = "synthetic packet arena"
        elif OT <= address < OT + 0x4000:
            self.reads[-1]["owner"] = "synthetic ordering table"
        elif STACK <= address < STACK + STACK_SIZE:
            self.reads[-1]["owner"] = "synthetic x86 call stack/local scratch"
        elif ENG_BASE <= address < ENG_BASE + 0x100000:
            self.reads[-1]["owner"] = "original ENG3 image"
        elif EXE_BASE <= address < EXE_BASE + 0x1000000:
            self.reads[-1]["owner"] = "original HERCULES image"
        else:
            self.reads[-1]["owner"] = "unexpected/unmapped context"

    def on_write(self, uc, _access, address, size, value, _user):
        pc = uc.reg_read(UC_X86_REG_EIP)
        item = {"pc": hex(pc), "address": hex(address), "size": size,
                "value": hex(value & ((1 << (8 * size)) - 1))}
        self.writes.append(item)
        if PACKET <= address < PACKET + 0x100:
            self.packet_writes.append(item.copy())
        if OT <= address < OT + 0x4000:
            self.ot_writes.append(item.copy())


def invoke(uc: Uc, trace: Trace, entry: int, args: tuple[int, ...], mode: str):
    sp = STACK_TOP
    uc.mem_write(sp, struct.pack("<I", SENTINEL))
    for i, arg in enumerate(args):
        uc.mem_write(sp + 4 + 4 * i, struct.pack("<I", arg & 0xFFFFFFFF))
    uc.reg_write(UC_X86_REG_ESP, sp)
    uc.reg_write(UC_X86_REG_EBP, 0)
    uc.reg_write(UC_X86_REG_EBX, 0)
    uc.reg_write(UC_X86_REG_ESI, 0)
    uc.reg_write(UC_X86_REG_EDI, 0)
    trace.mode = mode
    before_count = len(trace.pcs)
    uc.emu_start(entry, SENTINEL, timeout=0, count=50000)
    if uc.reg_read(UC_X86_REG_EIP) != SENTINEL:
        raise RuntimeError(f"{mode} did not return; EIP={uc.reg_read(UC_X86_REG_EIP):#x}")
    return {"entry": hex(entry), "args": [hex(x) for x in args],
            "executed_instructions": len(trace.pcs) - before_count}


def main() -> None:
    if sha(EXE_PATH.read_bytes()) != ORIGINALS_SHA["HERCULES.EXE"]:
        raise RuntimeError("HERCULES.EXE hash mismatch")
    if sha(ENG3_PATH.read_bytes()) != ORIGINALS_SHA["ENG3.DLL"]:
        raise RuntimeError("ENG3.DLL hash mismatch")
    if unicorn.__version__ != "2.1.4":
        raise RuntimeError(f"unexpected Unicorn {unicorn.__version__}")
    exe, eng3 = PE(str(EXE_PATH)), PE(str(ENG3_PATH))
    copy_evidence = json.loads(INTERFACE_COPY_EVIDENCE.read_text(encoding="utf-8-sig"))
    eng3_layout = next(row for row in copy_evidence["module_layouts"] if row["module"] == "ENG3.DLL")
    table_rva = int(eng3_layout["module_local_table"]["rva"], 16)
    cold_copy = next(row for row in copy_evidence["scenarios"]
                     if row.get("module") == "ENG3.DLL" and row.get("scenario") == "cold_valid_copy")
    CALLBACK_RVA = int(copy_evidence["source_table_fixture"]["original_table_rva"], 16)
    CALLBACK_BYTES = int(copy_evidence["source_table_fixture"]["original_table_bytes"])
    source_table_hash = copy_evidence["source_table_fixture"]["original_table_sha256_expected_and_computed"]
    if (eng3_layout["sha256_expected_and_computed"] != ORIGINALS_SHA["ENG3.DLL"] or
            int(eng3_layout["module_local_table"]["bytes"]) != CALLBACK_BYTES or
            int(eng3_layout["module_local_table"]["words"]) != CALLBACK_BYTES // 4 or
            cold_copy["table_rva"] != hex(table_rva) or
            cold_copy["table_matches_source_exactly"] is not True or
            int(cold_copy["copied_bytes_seen_at_native_rep_instruction"]) != CALLBACK_BYTES):
        raise RuntimeError("canonical evidence does not pin ENG3's cold 250-DWORD callback-table copy")
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    map_pe(uc, exe, EXE_BASE)
    map_pe(uc, eng3, ENG_BASE)
    uc.mem_map(STACK, STACK_SIZE)
    uc.mem_map(SYNTH, 0x20000)
    uc.mem_map(0x0E000000, 0x1000)
    callback_prefix = exe.read_rva(CALLBACK_RVA, CALLBACK_BYTES)
    if len(callback_prefix) != CALLBACK_BYTES or sha(callback_prefix) != source_table_hash:
        raise RuntimeError("original host callback prefix does not match the canonical 250-DWORD source-table pin")
    uc.mem_write(ENG_BASE + table_rva, callback_prefix)
    matrix_words = [4096, 0, 0, 0, 4096, 0, 0, 0, 4096]
    original_matrix_bytes = eng3.read_rva(MATRIX_PTR_RVA, 18)
    uc.mem_write(ENG_BASE + MATRIX_PTR_RVA, struct.pack("<9h", *matrix_words))
    put_u32(uc, ENG_BASE + CONTEXT_PTR_RVA, CONTEXT)
    put_u32(uc, ENG_BASE + PACKET_CURSOR_RVA, PACKET)
    put_u32(uc, ENG_BASE + VIEW_PTR_RVA, OT)
    uc.mem_write(CONTEXT, bytes(0x100))
    uc.mem_write(OBJECT, bytes(0x100))
    uc.mem_write(STREAM, bytes([0x11, 0x22, 0x34, 0x46, 0x58, 0x6A]) + bytes(0xFA))
    uc.mem_write(OT, bytes(0x4000))
    uc.mem_write(PACKET, bytes(0x100))

    # Synthetic object fields read directly by the original producer. These are
    # context inputs, not a reconstructed runtime record or asset.
    put_u16(uc, OBJECT + 0x10, 1024)  # OT depth bucket input
    put_u16(uc, OBJECT + 0x1E, 0)     # texture/material selector byte and adjacent mode
    put_u16(uc, OBJECT + 0x28, 1)     # producer's observed context+0x7c increment source
    put_u16(uc, OBJECT + 0x46, 0)     # packed material-table selector
    put_u32(uc, OBJECT + 0x54, 0)     # observed producer mode flags
    # These first three DWORDs are written by the producer's B2F0 call.
    # Only the fourth DWORD is outside that callback's observed writes.
    put_u32(uc, CONTEXT + 0x84, 0)
    # Seed the camera snapshot that the original producer copies to its object
    # context before loading that context as the B8C0 translation vector.
    put_u32(uc, 0x00A8E71C, 0)
    put_u32(uc, 0x00A8E728, 0)
    put_u32(uc, 0x00A8E734, 512)
    trace = Trace(uc)

    # B970/B990 are the observed public callbacks for screen center/focal.
    setup = []
    setup.append(invoke(uc, trace, EXE_BASE + 0xB970, (320, 240), "projection_setup_B970"))
    setup.append(invoke(uc, trace, EXE_BASE + 0xB990, (512,), "projection_setup_B990"))
    producer_call = invoke(uc, trace, ENG_BASE + 0x231A0, (STREAM, OBJECT, 1), "ENG3_231A0")

    old_bucket_index = 0x820 - (1024 & 0xFFFC)
    bucket_address = OT + old_bucket_index * 4
    packet_bytes = bytes(uc.mem_read(PACKET, 0x28))
    packet_words = list(struct.unpack("<20H", packet_bytes))
    packet_dwords = list(struct.unpack("<10I", packet_bytes))
    source_points = trace.point_events[0]["triplets_s16"] + [trace.point_events[1]["triplet_s16"]]
    camera_points = (trace.batch_projection["camera_xyz_dwords_by_vertex"] + [trace.single_projection["camera_xyz_dwords"]]
                     if trace.batch_projection and trace.single_projection else [])
    projected_points = (trace.batch_projection["projected_xy_words_by_vertex"] + [trace.single_projection["projected_xy_words"]]
                        if trace.batch_projection and trace.single_projection else [])
    packet_points = (trace.batch_packet_xy + [trace.single_packet_xy]
                     if trace.batch_packet_xy and trace.single_packet_xy else [])
    if len(source_points) != 4 or len({tuple(row) for row in source_points}) != 4:
        raise AssertionError(f"synthetic stream did not produce four distinct source triplets: {source_points}")
    expected_camera_points = [[point[0], point[1] + 2, 512] for point in source_points]
    if camera_points != expected_camera_points:
        raise AssertionError(f"camera point provenance mismatch: {camera_points}")
    if projected_points != packet_points:
        raise AssertionError(f"projected lanes do not match packet corner copies: {projected_points} != {packet_points}")
    if (get_u32(uc, ENG_BASE + PACKET_CURSOR_RVA) - PACKET) != 0x28:
        raise AssertionError("original FT4 producer did not advance by 0x28")
    if get_u32(uc, PACKET + 4) != 0x2E000000:
        raise AssertionError("original FT4 producer command word differs from 0x2e000000")
    if get_u32(uc, bucket_address) != PACKET:
        raise AssertionError("original FT4 producer did not link the packet at the selected OT bucket")
    report = {
        "status": "ORIGINAL_ENG3_FT4_PRODUCER_EXECUTED_SYNTHETIC_CONTEXT",
        "proof_level": "BOUNDED_ORIGINAL_X86_EXECUTION",
        "prediction_and_falsifier": PREDICTION_PATH.read_text(encoding="utf-8-sig"),
        "originals": {
            "HERCULES.EXE": {"path": str(EXE_PATH), "sha256": sha(EXE_PATH.read_bytes()), "preferred_base": hex(EXE_BASE)},
            "ENG3.DLL": {"path": str(ENG3_PATH), "sha256": sha(ENG3_PATH.read_bytes()), "preferred_base": hex(ENG_BASE)},
        },
        "input_evidence_sha256": {
            name: sha((REPO / name).read_bytes()) for name in PUBLIC_INPUTS
        },
        "interface_copy": {
            "source_rva": hex(CALLBACK_RVA), "bytes": CALLBACK_BYTES, "dwords": 250,
            "sha256": sha(callback_prefix), "destination_rva_derived_from_canonical_evidence": hex(table_rva),
            "destination_va": hex(ENG_BASE + table_rva),
            "cold_copy_evidence": {"path": str(INTERFACE_COPY_EVIDENCE),
                                    "sha256": sha(INTERFACE_COPY_EVIDENCE.read_bytes()),
                                    "scenario": cold_copy["scenario"],
                                    "scenario_table_matches_source_exactly": cold_copy["table_matches_source_exactly"]},
            "method": "canonical native_interface_copy.json pins the original cold-valid 250-DWORD copy; fixture copies those same source bytes before seeding producer globals",
            "slot_words": {str(i): hex(struct.unpack_from("<I", callback_prefix, i * 4)[0]) for i in [101, 102, 106, 107, 108, 110, 111, 114, 117, 121, 174, 175]},
        },
        "runtime_setup": {
            "center_callback": setup[0], "focal_callback": setup[1],
            "matrix_source": {"address": hex(ENG_BASE + MATRIX_PTR_RVA), "signed_words_row_major": matrix_words,
                              "seeded_original_module_data": True},
            "seeded_globals": {"current_context_pointer": hex(ENG_BASE + CONTEXT_PTR_RVA),
                               "packet_cursor_pointer": hex(ENG_BASE + PACKET_CURSOR_RVA), "view_pointer": hex(ENG_BASE + VIEW_PTR_RVA)},
            "camera_snapshot_xyz_before_producer": [0, 0, 512],
            "synthetic_context_0x84_fourth_dword": 0,
            "context_translation_after_producer_copy_and_adjustment": trace.context_translation,
            "explicitly_uninitialized_context_word": "context+0x84 is synthetic zero; B2F0 only writes the first three DWORDs",
        },
        "synthetic_inputs": {
            "object_address": hex(OBJECT), "resource_stream_address": hex(STREAM),
            "producer_arguments_cdecl": [hex(STREAM), hex(OBJECT), "0x1"],
            "resource_stream_hex_consumed_prefix": bytes(uc.mem_read(STREAM, 7)).hex(),
            "object_fields_seeded": {"+0x10_depth_word": 1024, "+0x1e_mode_byte": 0, "+0x28_counter_word": 1,
                                      "+0x46_selector_word": 0, "+0x54_flags_dword": 0},
            "packet_cursor_initial": hex(PACKET), "packet_arena_mapping_bytes": "0x100 (one output record in this fixture; not an engine capacity claim)",
            "order_table_base": hex(OT), "order_table_mapping_bytes": "0x4000 (synthetic bucket storage; not an engine capacity claim)",
        },
        "producer_entry": producer_call,
        "trace": {
            "instruction_count": trace.instruction_count,
            "executed_spans": sorted({x["span"] for x in trace.pcs}),
            "pcs": trace.pcs,
            "reads": trace.reads,
            "writes": trace.writes,
            "callback_calls": trace.calls,
            "point_events": trace.point_events,
            "projection_events": trace.projection_events,
            "packet_writes": trace.packet_writes,
            "ot_writes": trace.ot_writes,
        },
        "observed_packet": {
            "address": hex(PACKET), "stride_advanced": hex(get_u32(uc, ENG_BASE + PACKET_CURSOR_RVA) - PACKET),
            "raw_40_bytes_hex": packet_bytes.hex(), "dwords": [hex(x) for x in packet_dwords],
            "words": [hex(x) for x in packet_words],
            "command_word": hex(get_u32(uc, PACKET + 4)),
            "ot_bucket_index": old_bucket_index, "ot_bucket_address": hex(bucket_address),
            "ot_bucket_head_after": hex(get_u32(uc, bucket_address)),
            "packet_link_previous_head": hex(get_u32(uc, PACKET)),
        },
        "point_association": {
            "association_basis": "producer-built stack triplet addresses passed to B430 in pointer order; the third point address is then overwritten by ENG3+0x233BB before B6F0 reuses it for point four; camera lanes are captured on original producer return PCs; projected lane XY is compared with B5C0/B720 packet memory writes.",
            "source_triplets_signed_word_xyz": source_points,
            "batch_input_addresses_ordered_at_B430": trace.point_events[0]["addresses"],
            "fourth_input_address_at_B6F0": trace.point_events[1]["address"],
            "third_point_scratch_reuse": trace.point_events[0]["addresses"][2] == trace.point_events[1]["address"],
            "camera_xyz_dwords_by_point": camera_points,
            "projected_xy_words_by_point": projected_points,
            "packet_xy_words_by_corner": packet_points,
            "projection_to_packet_equal": projected_points == packet_points,
        },
        "material_and_texture_evidence": {
            "synthetic_stream_byte_reads": [row for row in trace.reads if row.get("owner") == "synthetic FT4 resource stream"],
            "synthetic_object_field_reads": [row for row in trace.reads if row.get("owner") == "synthetic object/context"],
            "original_material_lookup_reads": [row for row in trace.reads if int(row["address"], 16) in
                (ENG_BASE + 0x39AC0, ENG_BASE + 0x42562, ENG_BASE + 0x42564)],
            "original_matrix_bytes_before_seed": original_matrix_bytes.hex(),
            "packet_fields_written_by_original_producer": [row for row in trace.packet_writes if int(row["pc"], 16) >= ENG_BASE and
                int(row["pc"], 16) < ENG_BASE + 0x100000],
            "interpretation_limit": "Offsets and raw values are proven by executed reads/stores; the fixture does not assign engine asset semantics beyond the measured source table/stream/object fields.",
        },
        "limits": [
            "Only the original 0x231A0 producer and its directly reached copied wrappers/HERC callbacks execute; the 0x22E40→0x24770→0x8DA0 chain is static call-site context and its visibility gate remains unresolved in this fixture.",
            "Synthetic stream/object/context/packet/OT inputs are not authentic assets or live object identity.",
            "Camera center/focal and snapshot globals are synthetic frame state. No engine initialization, scene/resource loading, visibility, clipping, whole frame, capacity, rasterization, or GPU behavior is claimed.",
            "Executed span extents do not alter recovery.json or prove source identity/module layout/whole-binary equality.",
        ],
    }
    OUT.mkdir(parents=False, exist_ok=False)
    raw_path = OUT / "eng3_native_fixture_raw.json"
    raw_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
    evidence_hashes = {name: sha((REPO / name).read_bytes()) for name in PUBLIC_INPUTS}
    runner_path = Path(__file__).resolve()
    pe_adapter = REPO / "scripts" / "pe.py"
    unicorn_dll = Path(unicorn.__file__).parent / "lib" / "unicorn.dll"
    source_reads = [row for row in trace.reads if row.get("owner") == "synthetic FT4 resource stream"]
    object_reads = [row for row in trace.reads if row.get("owner") == "synthetic object/context"]
    context_reads = [row for row in trace.reads if row.get("owner") == "synthetic producer context"]
    texture_reads = [row for row in trace.reads if int(row["address"], 16) in
                     (ENG_BASE + 0x39AC0, ENG_BASE + 0x42562, ENG_BASE + 0x42564)]
    scratch_points = [row for row in trace.writes if row["pc"] in
                      [hex(ENG_BASE + pc) for pc in (0x2334C, 0x23351, 0x2335E, 0x23363, 0x2336F, 0x2337B, 0x233BB)]]
    producer_material_stores = [row for row in trace.packet_writes if int(row["pc"], 16) >= ENG_BASE and
                                int(row["pc"], 16) < ENG_BASE + 0x100000]
    def executable_ranges(image: PE, base: int):
        return [(base + section.VirtualAddress,
                 base + section.VirtualAddress + max(section.Misc_VirtualSize, section.SizeOfRawData))
                for section in image.pe.sections if section.Characteristics & 0x20000000]
    executable_sections = executable_ranges(exe, EXE_BASE) + executable_ranges(eng3, ENG_BASE)
    code_write_events = [row for row in trace.writes if any(lo <= int(row["address"], 16) < hi
                                                             for lo, hi in executable_sections)]
    if code_write_events:
        raise AssertionError(f"original code write observed: {code_write_events[:3]}")
    unexpected_reads = [row for row in trace.reads if row.get("owner") == "unexpected/unmapped context"]
    if unexpected_reads:
        raise AssertionError(f"read outside the pinned original images and explicit synthetic mappings: {unexpected_reads[:8]}")
    compact = {
        "status": report["status"],
        "proof_level": report["proof_level"],
        "prediction_and_falsifier": report["prediction_and_falsifier"],
        "originals": report["originals"],
        "runtime": {"python": {"version": sys.version.splitlines()[0], "path": sys.executable, "sha256": sha(Path(sys.executable).read_bytes())},
                    "unicorn": {"version": unicorn.__version__, "module": str(unicorn.__file__),
                                "module_sha256": sha(Path(unicorn.__file__).read_bytes()),
                                "native_dll": str(unicorn_dll), "native_dll_sha256": sha(unicorn_dll.read_bytes())},
                    "pe_adapter": {"path": str(pe_adapter), "sha256": sha(pe_adapter.read_bytes())},
                    "runner": {"path": str(runner_path), "sha256": sha(runner_path.read_bytes())}},
        "input_evidence_sha256": {**evidence_hashes,
                                   "renderer_contract/fixtures/eng3_geometry_association/run_fixture.py": sha(runner_path.read_bytes()),
                                   "renderer_contract/fixtures/eng3_geometry_association/prediction.md": sha(PREDICTION_PATH.read_bytes()),
                                   "renderer_contract/tests/test_original_eng3_geometry_association.py": sha(TEST_HELPER_PATH.read_bytes())},
        "interface_copy": report["interface_copy"],
        "runtime_setup": {**report["runtime_setup"], "matrix_original_18_bytes": original_matrix_bytes.hex()},
        "synthetic_inputs": report["synthetic_inputs"],
        "producer_call_boundary": {
            "static_route": "ENG3+0x22E40 handler index 3 -> +0x24770 -> +0x8DA0 -> +0x231A0 callsite +0x901D",
            "static_callsite_arguments": ["arg0 = resource stream pointer saved at 0x8DA0+0x14", "arg1 = child/object pointer", "arg2 = resource first-byte loop count"],
            "fixture_direct_entry": report["producer_entry"],
            "fixture_call_arguments_match_static_order": True,
            "handler_gate": "UNRESOLVED in dynamic fixture; 22E40/24770/8DA0 were not executed, and no passing predicate or live visibility is claimed.",
            "producer_span": {"rva": "0x231a0", "size": "0x293", "sha256": sha(eng3.read_rva(0x231A0, 0x293)),
                              "executed_through_final_ret": trace.pcs[-1]["pc"] if trace.pcs else None},
            "executed_spans": sorted({x["span"] for x in trace.pcs}),
            "instruction_count": trace.instruction_count,
            "wrapper_call_order": trace.wrapper_events,
            "host_callback_entry_order": trace.host_call_order,
            "captured_callback_events": trace.calls,
        },
        "association": report["point_association"],
        "scratch_write_provenance": scratch_points,
        "texture_material": {"resource_stream_reads": source_reads, "object_field_reads": object_reads,
                             "producer_context_reads": context_reads,
                             "original_lookup_table_reads": texture_reads,
                             "producer_packet_field_writes": producer_material_stores},
        "packet_and_ot": report["observed_packet"],
        "ot_insert_writes": trace.ot_writes,
        "packet_xy_callback_writes": [row for row in trace.packet_writes if int(row["pc"], 16) in
                                        [0x40B5CB, 0x40B5D6, 0x40B5E1, 0x40B5EC, 0x40B5F7, 0x40B602, 0x40B72B, 0x40B735]],
        "no_original_code_writes": True,
        "raw_trace_receipt": {"path": str(raw_path), "sha256": sha(raw_path.read_bytes()), "bytes": raw_path.stat().st_size,
                              "contains_instruction_pc_memory_read_write_trace": True},
        "limits": report["limits"],
    }
    path = OUT / "eng3_geometry_association.json"
    path.write_text(json.dumps(compact, indent=2) + "\n", encoding="utf-8", newline="\n")
    if sha(EXE_PATH.read_bytes()) != ORIGINALS_SHA["HERCULES.EXE"] or sha(ENG3_PATH.read_bytes()) != ORIGINALS_SHA["ENG3.DLL"]:
        raise AssertionError("read-only original input hash changed during fixture execution")
    print(json.dumps({"report": str(path), "sha256": sha(path.read_bytes()), "bytes": path.stat().st_size,
                      "raw_trace": str(raw_path), "raw_sha256": sha(raw_path.read_bytes()),
                      "packet_words": report["observed_packet"]["words"],
                      "calls": trace.calls, "point_events": trace.point_events,
                      "association": compact["association"],
                      "executed_spans": compact["producer_call_boundary"]["executed_spans"]}, indent=2))


if __name__ == "__main__":
    main()
