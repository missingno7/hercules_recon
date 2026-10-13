"""Bounded original AC60 FT4 raster plus windowed surface-tail trial."""
from __future__ import annotations

import hashlib
import argparse
import json
import os
import struct
import sys
from pathlib import Path

BOOT = argparse.ArgumentParser(add_help=False)
BOOT.add_argument("--repo-root", required=True, type=Path)
BOOT.add_argument("--exe", required=True, type=Path)
BOOT.add_argument("--deps", required=True, type=Path, help="Unicorn dependency directory")
BOOT.add_argument("--analysis-deps", required=True, type=Path, help="pefile/Capstone dependency directory")
BOOT_ARGS, _BOOT_REST = BOOT.parse_known_args()
REPO = BOOT_ARGS.repo_root.resolve()
sys.path.insert(0, str(REPO / "scripts"))
sys.path.insert(0, str(BOOT_ARGS.deps.resolve()))
sys.path.insert(0, str(BOOT_ARGS.analysis_deps.resolve()))
from pe import PE
import pefile
import unicorn
from unicorn import Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_INVALID, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MODE_32, UC_PROT_EXEC, UC_PROT_READ, UC_PROT_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EFLAGS, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP

EXE_PATH = BOOT_ARGS.exe.resolve()
EXPECTED_EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
BASE = 0x00400000
PAGE = 0x1000
INPUT = 0x10000000
STACK = 0x20000000
STACK_SIZE = 0x10000
SENTINEL = 0x30000000
PIXELS = 0x34000000
PIXEL_SIZE = 0x14000
DIB_INFO = 0x35000000
HDC_TOKEN = 0x36000000
STRETCH_STUB = 0x10004000
FLUSH_STUB = 0x10004010
TEXTURE = 0x0096E6E0
MAX_INSNS = 800000

ALLOWED = (
    (0xAC60, 0xB2F0, "AC60"),
    (0xD420, 0xDD70, "D420"),
    (0xCAB0, 0xCC45, "CAB0"),
    (0x6FF0, 0x70B7, "LOCK"),
    (0x70C0, 0x7136, "REMAP"),
    (0x7140, 0x716E, "UNLOCK"),
    (0x6620, 0x66F5, "PRESENT"),
    (0xBD80, 0xBD96, "BD80"),
)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def align_up(value: int) -> int:
    return (value + PAGE - 1) & ~(PAGE - 1)


def put32(uc: Uc, address: int, value: int) -> None:
    uc.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))


def u32(uc: Uc, address: int) -> int:
    return struct.unpack("<I", bytes(uc.mem_read(address, 4)))[0]


def packet(tag: int, length: int, next_address: int, fields: dict[int, bytes]) -> bytes:
    raw = bytearray([0xA5] * length)
    raw[0:4] = struct.pack("<I", next_address)
    raw[7] = tag
    for offset, value in fields.items():
        raw[offset:offset + len(value)] = value
    return bytes(raw)


def s16(value: int) -> bytes:
    return struct.pack("<h", value)


def u16(value: int) -> bytes:
    return struct.pack("<H", value)


def map_original(uc: Uc, pe: PE) -> list[dict]:
    mapped = []
    header_size = align_up(pe.pe.OPTIONAL_HEADER.SizeOfHeaders)
    uc.mem_map(BASE, header_size, UC_PROT_READ)
    uc.mem_write(BASE, pe.data[:pe.pe.OPTIONAL_HEADER.SizeOfHeaders])
    for section in pe.sections:
        virtual_size = max(section["virtual_size"], section["raw_size"])
        size = align_up(virtual_size)
        va = BASE + section["virtual_address"]
        flags = section["characteristics"]
        permissions = UC_PROT_READ
        if flags & 0x80000000:
            permissions |= UC_PROT_WRITE
        if flags & 0x20000000:
            permissions |= UC_PROT_EXEC
        uc.mem_map(va, size, permissions)
        if section["raw_size"]:
            uc.mem_write(va, pe.read_rva(section["virtual_address"], section["raw_size"]))
        if size > section["raw_size"]:
            uc.mem_write(va + section["raw_size"], b"\0" * (size - section["raw_size"]))
        mapped.append({"name": section["name"], "va": hex(va), "size": hex(size)})
    return mapped


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--deps", required=True, type=Path)
    parser.add_argument("--analysis-deps", required=True, type=Path)
    parser.add_argument("--case", choices=("base", "offset_rectangle", "asymmetric_uv"), default="base")
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    if args.repo_root.resolve() != REPO or args.exe.resolve() != EXE_PATH or \
       args.deps.resolve() != BOOT_ARGS.deps.resolve() or \
       args.analysis_deps.resolve() != BOOT_ARGS.analysis_deps.resolve():
        raise SystemExit("parsed paths differ from the explicit bootstrap paths")
    if not args.output_dir.is_absolute():
        raise SystemExit("output directory must be an explicit absolute path")
    output = args.output_dir.resolve()
    work_root = (REPO / "work").resolve()
    if not output.is_relative_to(work_root) or output == work_root:
        raise SystemExit("native receipts must be written below repository work/")
    protected = [(REPO / "assets").resolve(), (REPO / "work" / "discs").resolve()]
    if any(output.is_relative_to(item) or item.is_relative_to(output) for item in protected):
        raise SystemExit("native receipt path overlaps protected assets/original inputs")
    if Path(os.path.abspath(str(args.output_dir))).resolve() != output:
        raise SystemExit("native output path has a symlink/junction redirect")
    output.mkdir(parents=True, exist_ok=True)
    args.output_dir = output
    if args.case == "base":
        x, y, width, height, u, v = 1, 1, 2, 2, 0, 0
        texture_samples = {(0, 0): 0, (1, 0): 0x5A,
                           (0, 1): 0x77, (1, 1): 0x6B}
    elif args.case == "offset_rectangle":
        x, y, width, height, u, v = 4, 5, 3, 2, 2, 1
        texture_samples = {(2, 1): 0x11, (3, 1): 0x22, (4, 1): 0x33,
                           (2, 2): 0x44, (3, 2): 0x55, (4, 2): 0x66}
    else:
        x, y, width, height, u, v = 1, 1, 2, 2, 0, 0
        texture_samples = {(0, 0): 0, (1, 0): 0x5A,
                           (0, 1): 0x77, (1, 1): 0x6B}
    pe = PE(EXE_PATH)
    if sha(pe.data) != EXPECTED_EXE_SHA256 or pe.image_base != BASE:
        raise RuntimeError("pinned original image hash/base mismatch")
    if getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        raise RuntimeError("unexpected base relocations")

    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    mapped = map_original(uc, pe)
    uc.mem_map(INPUT, 0x4000, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(STACK, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(PIXELS, PIXEL_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(DIB_INFO, PAGE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(STRETCH_STUB, PAGE, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC)
    uc.mem_map(SENTINEL, PAGE, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC)

    ft4 = INPUT + 0x1000
    end_packet = INPUT + 0x1040
    packet_bytes = packet(0x2C, 0x28, end_packet, {
        0x04: bytes((0x64, 0x75, 0x86)),
        0x08: s16(x), 0x0A: s16(y), 0x0C: bytes((u, v)), 0x0E: u16(0),
        0x10: s16(x + width), 0x12: s16(y),
        0x14: bytes((u + width, v)), 0x16: u16(0),
        0x18: s16(x), 0x1A: s16(y + height),
        0x1C: bytes((u + (1 if args.case == "asymmetric_uv" else 0), v + height)), 0x1E: u16(0),
        0x20: s16(x + width), 0x22: s16(y + height),
        0x24: bytes((u + width, v + height)), 0x26: u16(0),
    })
    terminator = packet(0x40, 8, 0, {})
    ot = INPUT + 0x100
    uc.mem_write(ot, struct.pack("<I", ft4))
    uc.mem_write(ft4, packet_bytes)
    uc.mem_write(end_packet, terminator)

    # Both cases are eligible D420 rectangles. The second shifts/resizes the
    # rectangle and starts UV at (2,1), so native source/destination addresses
    # must change. All texture bytes are synthetic; no asset is read.
    texture = bytearray(0x10000)
    for (sample_u, sample_v), sample in texture_samples.items():
        texture[sample_v * 0x100 + sample_u] = sample
    uc.mem_write(TEXTURE, bytes(texture))
    uc.mem_write(PIXELS, b"\xa5" * PIXEL_SIZE)

    data_section = next(section for section in pe.sections if section["name"] == ".data")
    data_va = BASE + data_section["virtual_address"]
    data_size = align_up(max(data_section["virtual_size"], data_section["raw_size"]))
    before_data = bytes(uc.mem_read(data_va, data_size))
    before_code = {
        start: bytes(uc.mem_read(BASE + start, end - start))
        for start, end in ((0xAC60, 0xB2F0), (0xD420, 0xDD70), (0xCAB0, 0xCC45),
                           (0x6FF0, 0x70B7), (0x70C0, 0x7136), (0x7140, 0x716E),
                           (0x6620, 0x66F5), (0xBD80, 0xBD96))
    }

    # DIB, windowed surface and D420 state are controlled fixture providers.
    fixture_cells = {
        0x00BA01FC: b"\0", 0x00BA020C: b"\0",
        0x0096E3D8: struct.pack("<I", PIXELS),
        0x0096E3F4: struct.pack("<I", DIB_INFO),
        0x0096E400: struct.pack("<I", HDC_TOKEN),
        0x00BA0220: struct.pack("<I", 320), 0x00BA0224: struct.pack("<I", 240),
        0x00BA5664: struct.pack("<I", STRETCH_STUB),
        0x00BA56F4: struct.pack("<I", FLUSH_STUB),
        0x0043A590: b"\1",  # explicitly seeded present-dirty request
        0x00A90A04: struct.pack("<I", 0x7800), # normalized row-zero direct route
        0x0096E3E0: struct.pack("<I", 0), # pixel-color gate off for direct copy
        0x00436A0C: b"\x80", # original 70C0 guard takes the skip branch
        0x00A8E780: struct.pack("<I", 0x31000000),
        0x00A8E788: b"\0" * 4, # do not dispatch the unknown host callback
    }
    old_fixture_cells = {address: bytes(uc.mem_read(address, len(value))) for address, value in fixture_cells.items()}
    for address, value in fixture_cells.items():
        uc.mem_write(address, value)
    before_pixels = bytes(uc.mem_read(PIXELS, PIXEL_SIZE))
    before_texture = bytes(uc.mem_read(TEXTURE, len(texture)))

    trace: list[int] = []
    reads: list[dict] = []
    writes: list[dict] = []
    pixel_events: list[dict] = []
    data_events: list[dict] = []
    gdi_events: list[dict] = []
    code_markers: list[dict] = []
    native_events: list[dict] = []
    state = {"stop": None, "detail": None, "instructions": 0}
    data_end = data_va + data_size
    packet_range = (INPUT, INPUT + 0x4000)
    stack_range = (STACK, STACK + STACK_SIZE)
    scratch_ranges = (packet_range, stack_range, (DIB_INFO, DIB_INFO + PAGE),
                      (STRETCH_STUB, STRETCH_STUB + PAGE), (SENTINEL, SENTINEL + PAGE))
    external = {
        STRETCH_STUB: ("StretchDIBits", 13, 52, 1),
        FLUSH_STUB: ("GdiFlush", 0, 0, 1),
    }

    def stop(kind: str, detail=None):
        state["stop"] = kind
        state["detail"] = detail
        uc.emu_stop()

    def add_event(kind: str, source_rva: int, raw_token: int, arguments=None):
        native_events.append({
            "instruction_order": state["instructions"],
            "kind": kind,
            "source_rva": hex(source_rva),
            "raw_token": hex(raw_token),
            "arguments": list(arguments or [0, 0, 0, 0]),
        })

    def on_code(machine, address, _size, _user):
        state["instructions"] += 1
        if state["instructions"] > MAX_INSNS:
            stop("instruction_budget", MAX_INSNS)
            return
        if address == SENTINEL:
            stop("return_sentinel")
            return
        if address in external:
            name, argc, pop, result = external[address]
            esp = machine.reg_read(UC_X86_REG_ESP)
            ret = u32(machine, esp)
            args = [u32(machine, esp + 4 + 4 * index) for index in range(argc)]
            gdi_events.append({"service": name, "provider": "controlled synthetic endpoint; no OS call",
                               "return_rva": hex(ret - BASE), "arguments": [hex(value) for value in args],
                               "return_value": result, "instruction_count": state["instructions"]})
            if name == "StretchDIBits":
                add_event("final_output", ret - BASE, args[0], [ret - BASE, 0, 0, 0])
            else:
                add_event("output_flush", ret - BASE, 0, [ret - BASE, 0, 0, 0])
            machine.reg_write(UC_X86_REG_ESP, esp + 4 + pop)
            machine.reg_write(UC_X86_REG_EIP, ret)
            machine.reg_write(UC_X86_REG_EAX, result)
            return
        rva = address - BASE
        if rva == 0xAC60:
            add_event("frame_entry", rva, ot)
        elif rva == 0x6FF0:
            add_event("surface_lock", rva, PIXELS,
                      [u32(machine, 0x00BA0220), u32(machine, 0x00BA0224), PIXELS, 0])
        elif rva == 0xACB9:
            packet_address = machine.reg_read(UC_X86_REG_ESI)
            tag = bytes(machine.mem_read(packet_address + 7, 1))[0]
            add_event("packet_consumed", rva, packet_address,
                      [tag, u32(machine, packet_address), 0, 0])
        elif rva == 0x70C0:
            esp = machine.reg_read(UC_X86_REG_ESP)
            remap_arg = u32(machine, esp + 4)
            guard = bytes(machine.mem_read(0x00436A0C, 1))[0]
            code_markers.append({"name": "REMAP_REQUEST", "argument": hex(remap_arg),
                                 "guard": hex(guard), "instruction_count": state["instructions"]})
            add_event("remap_request", rva, PIXELS, [remap_arg, guard, 0, 0])
        elif rva == 0x7140:
            add_event("surface_unlock", rva, PIXELS)
        elif rva == 0x6620:
            add_event("present_request", rva, PIXELS,
                      [bytes(machine.mem_read(0x0043A590, 1))[0],
                       u32(machine, 0x00BA0220), u32(machine, 0x00BA0224), 0])
        if address == 0x0040D420:
            esp = machine.reg_read(UC_X86_REG_ESP)
            code_markers.append({"name": "FT4_D420_entry", "return_rva": hex(u32(machine, esp) - BASE),
                                 "packet": hex(machine.reg_read(0x1000001D)) if False else hex(ft4),
                                 "instruction_count": state["instructions"]})
            x0 = u32(machine, 0x00A909B8)
            y0 = u32(machine, 0x00A909C8)
            add_event("geometry_command", rva, ft4,
                      [x0, y0, u32(machine, 0x00A909BC) - x0,
                       u32(machine, 0x00A909D0) - y0])
        if not any(lo <= rva < hi for lo, hi, _name in ALLOWED):
            stop("unexpected_code", {"pc": hex(address), "rva": hex(rva), "recent": [hex(x) for x in trace[-16:]]})
            return
        trace.append(rva)
        if rva in (0xAC60, 0x6FF0, 0x70C0, 0x70D4, 0x70DB, 0x7132, 0x7140,
                   0x6620, 0xD420, 0xD549, 0xD60C, 0xDC8A, 0xDC92, 0xCAB0,
                   0xCBD6, 0xCBF4, 0xCBFB, 0xBD80):
            code_markers.append({"rva": hex(rva), "instruction_count": state["instructions"]})

    def on_read(machine, _access, address, size, _value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP) - BASE
        if TEXTURE <= address and address + size <= TEXTURE + 0x10000:
            raw = bytes(machine.mem_read(address, size))
            reads.append({"domain": "synthetic_texture", "pc_rva": hex(pc), "address": hex(address),
                          "size": size, "value": raw.hex()})
        if PIXELS <= address and address + size <= PIXELS + PIXEL_SIZE:
            raw = bytes(machine.mem_read(address, size))
            pixel_events.append({"kind": "read", "pc_rva": hex(pc), "offset": address - PIXELS,
                                 "size": size, "value": raw.hex()})

    def on_write(machine, _access, address, size, value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP) - BASE
        event = {"pc_rva": hex(pc), "address": hex(address), "size": size,
                 "value": hex(value & ((1 << (size * 8)) - 1))}
        if data_va <= address and address + size <= data_end:
            data_events.append(event)
            return
        if PIXELS <= address and address + size <= PIXELS + PIXEL_SIZE:
            pixel_value = value & ((1 << (size * 8)) - 1)
            pixel_events.append({"kind": "write", "pc_rva": hex(pc), "offset": address - PIXELS,
                                 "size": size, "value": event["value"],
                                 "instruction_count": state["instructions"]})
            add_event("cpu_pixel_write", pc, address,
                      [address - PIXELS, pixel_value, pixel_value, 0])
            writes.append(event)
            return
        if any(lo <= address and address + size <= hi for lo, hi in scratch_ranges):
            if packet_range[0] <= address < packet_range[1]:
                stop("write_to_packet_fixture", event)
            return
        stop("unexpected_write", event)

    def on_invalid(machine, access, address, size, _value, _user):
        stop("invalid_memory", {"pc": hex(machine.reg_read(UC_X86_REG_EIP)), "access": access,
                                "address": hex(address), "size": size})
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_READ, on_read)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
    uc.hook_add(UC_HOOK_MEM_INVALID, on_invalid)
    sp = STACK + STACK_SIZE - 0x100
    uc.mem_write(sp, struct.pack("<II", SENTINEL, ot))
    uc.reg_write(UC_X86_REG_ESP, sp)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
    uc.reg_write(UC_X86_REG_EIP, BASE + 0xAC60)

    try:
        uc.emu_start(BASE + 0xAC60, 0, timeout=6_000_000, count=MAX_INSNS)
    except UcError as error:
        if state["stop"] is None:
            state.update(stop="unicorn_error", detail=str(error))
    if state["stop"] is None and state["instructions"] >= MAX_INSNS:
        state.update(stop="instruction_budget", detail={"pc": hex(uc.reg_read(UC_X86_REG_EIP))})

    final_pixels = bytes(uc.mem_read(PIXELS, PIXEL_SIZE))
    changed = [{"offset": index, "before": before, "after": after}
               for index, (before, after) in enumerate(zip(before_pixels, final_pixels))
               if before != after]
    code_unchanged = all(bytes(uc.mem_read(BASE + start, len(raw))) == raw for start, raw in before_code.items())
    data_after = bytes(uc.mem_read(data_va, data_size))
    data_changed = [hex(data_va + index) for index, (before, after) in enumerate(zip(before_data, data_after)) if before != after]

    expected_changed = []
    for row in range(height):
        for column in range(width):
            sample = texture[(v + row) * 0x100 + u + column]
            if sample != 0:
                expected_changed.append({"offset": (y + row) * 320 + x + column,
                                         "before": 0xA5, "after": sample})
    expected_kinds = ["frame_entry", "surface_lock", "packet_consumed",
                      "geometry_command"] + ["cpu_pixel_write"] * len(expected_changed) + [
                      "packet_consumed", "remap_request", "surface_unlock",
                      "present_request", "final_output", "output_flush"]
    remap_branch = [item for item in trace if item in (0x70D4, 0x70DB, 0x7132)]
    remap_loop = [item for item in trace if 0x70DD <= item < 0x7132]
    selected_route = [hex(rva) for rva in trace
                      if rva in (0xD420, 0xD549, 0xCAB0, 0xD555, 0xD60C)]
    allowed_data_write_addresses = {
        0x00BA0214, 0x00BA0210, 0x00BA01F0,
        0x00A909B8, 0x00A909BC, 0x00A909C0, 0x00A909C4,
        0x00A909C8, 0x00A909CC, 0x00A909D0, 0x00A909D4,
        0x00A909D8, 0x00A909DC, 0x00A909E0, 0x00A909E4,
        0x00A909E8, 0x00A909EC, 0x00A909F0, 0x00A909F4,
        0x00A909F8, 0x00A909FC, 0x00A90A00, 0x00A90A04,
        0x00A90A0C, 0x00A90A10, 0x00A8E6FC, 0x00A8E6F4,
        0x00A8E6F8, 0x00A90944, 0x0043A590,
    }
    if state["stop"] != "return_sentinel":
        raise RuntimeError(f"original connected raster fixture did not return: {state}")
    if args.case != "asymmetric_uv" and changed != expected_changed:
        raise RuntimeError(f"native CPU-surface writes differ from the bounded prediction: {changed}")
    if args.case != "asymmetric_uv" and [item["kind"] for item in native_events] != expected_kinds:
        raise RuntimeError(f"native connected event order differs: {[item['kind'] for item in native_events]}")
    if args.case != "asymmetric_uv" and selected_route != ["0xd420", "0xd549", "0xcab0"]:
        raise RuntimeError(f"positive native case missed the D420→CAB0 path: {selected_route}")
    if args.case == "asymmetric_uv" and (
            0xD555 not in trace or 0xD60C not in trace or 0xCAB0 in trace):
        raise RuntimeError(f"asymmetric UV did not reject D420→CAB0 as predicted: {trace[:24]}")
    if remap_branch != [0x70D4, 0x70DB, 0x7132] or remap_loop:
        raise RuntimeError(f"70C0 missed its equality skip branch: {remap_branch}, loop={remap_loop[:8]}")
    if [item["service"] for item in gdi_events] != ["StretchDIBits", "GdiFlush"]:
        raise RuntimeError(f"controlled output endpoint order differs: {gdi_events}")
    if args.case == "asymmetric_uv":
        # The failed fast-path continues through D420's generic row converter.
        # Bound this discriminator to the documented raster workspace plus the
        # same service/dirty cells; it is not a model of that generic route.
        def in_asymmetric_workspace(address: int) -> bool:
            return (0x00A9091C <= address <= 0x00A90A04 or
                    address in (0x00A8E6F4, 0x00A8E6F8, 0x00A8E6FC,
                                0x00BA0214, 0x00BA0210, 0x00BA01F0, 0x0043A590))
        if any(not in_asymmetric_workspace(int(event["address"], 16))
               for event in data_events):
            raise RuntimeError(f"asymmetric fallback wrote outside bounded workspace: {data_events}")
    elif any(int(event["address"], 16) not in allowed_data_write_addresses for event in data_events):
        raise RuntimeError(f"native helper wrote an unapproved data cell: {data_events}")
    if not code_unchanged:
        raise RuntimeError("original code bytes changed in emulator")

    result = {
        "status": ("ORIGINAL_D420_ASYMMETRIC_UV_FALLBACK_OBSERVED"
                    if args.case == "asymmetric_uv" else "ORIGINAL_CONNECTED_RASTER_PASS"),
        "oracle": {
            "exe": str(EXE_PATH), "sha256": sha(pe.data), "image_base": hex(pe.image_base),
            "base_relocations": 0,
            "code_spans": [
                {"rva": hex(start), "size": len(raw), "sha256": sha(raw)}
                for start, raw in before_code.items()
            ],
        },
        "runtime": {
            "python_executable": sys.executable, "python_version": sys.version,
            "native_script": str(Path(__file__).resolve()),
            "native_script_sha256": sha(Path(__file__).read_bytes()),
            "pefile_version": pefile.__version__,
            "pefile_module": pefile.__file__,
            "unicorn": unicorn.__version__, "unicorn_module": unicorn.__file__,
            "unicorn_dll": str(Path(unicorn.__file__).resolve().parent / "lib" / "unicorn.dll"),
            "unicorn_dll_sha256": sha((Path(unicorn.__file__).resolve().parent / "lib" / "unicorn.dll").read_bytes()),
        },
        "fixture": {"packet": "FT4 tag 0x2c", "packet_address": hex(ft4), "packet_sha256": sha(packet_bytes),
                    "terminator_address": hex(end_packet), "terminator_sha256": sha(terminator),
                    "raw_input_bytes_hex": (uc.mem_read(ot, 4) + packet_bytes + terminator).hex(),
                    "texture_address": hex(TEXTURE), "texture_sha256": sha(before_texture),
                    "texture_length": len(texture),
                    "texture_samples": [hex(value) for value in texture_samples.values()],
                    "case": args.case,
                    "rectangle_xywh": [x, y, width, height],
                    "uv_origin": [u, v],
                    "surface_address": hex(PIXELS), "surface_pitch": 320, "surface_size": PIXEL_SIZE,
                    "initial_pixel": "0xa5", "dirty_seeded": True,
                    "remap_argument": "0xff", "remap_guard_seeded": "0x80"},
        "execution": {"stop": state["stop"], "detail": state["detail"],
                      "instruction_count": state["instructions"], "code_markers": code_markers,
                      "selected_route_trace": selected_route,
                      "texture_reads": reads, "surface_events": pixel_events,
                      "native_event_projection": native_events,
                      "surface_changed": changed, "surface_changed_count": len(changed),
                      "remap_branch_rvas": [hex(item) for item in remap_branch],
                      "remap_pixel_loop_instruction_count": len(remap_loop),
                      "services": gdi_events, "original_data_writes": data_events,
                      "data_changed_addresses": data_changed, "code_unchanged": code_unchanged,
                      "final_surface_sha256": sha(final_pixels)},
        "mapped_original_sections": mapped,
        "limits": ["Synthetic packet, texture, palette/LUT, CPU view, window, and GDI providers only.",
                   "No game, OS graphics, gameplay, semantic layer, or full-frame equivalence."],
    }
    default_receipts = {
        "base": "native_ft4_frame.json",
        "offset_rectangle": "native_ft4_offset_rectangle.json",
        "asymmetric_uv": "native_ft4_asymmetric_uv.json",
    }
    receipt = args.output_dir / default_receipts[args.case]
    receipt.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": result["status"], "stop": state["stop"], "detail": state["detail"],
                      "instruction_count": state["instructions"], "changed_count": len(changed),
                      "changed_first_32": changed[:32],
                      "markers": code_markers[-20:], "services": gdi_events}, indent=2))
    return 0 if state["stop"] == "return_sentinel" else 2


if __name__ == "__main__":
    raise SystemExit(main())
