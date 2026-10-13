"""Static receipts and finite pre-pixel geometry traces for C8F0/CAB0."""
from __future__ import annotations

import hashlib
import json
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))
from oracle_common import parse_args, verify_runtime  # noqa: E402

ARGS = parse_args(
    "Audit original sprite geometry setup before its pixel loops",
    "native_sprite_geometry_replay.json",
)
if ARGS.original_root.resolve() != ROOT.resolve():
    raise SystemExit("--original-root must be the repository containing this public audit script")
from pe import PE  # noqa: E402
import unicorn  # noqa: E402
from unicorn import (  # noqa: E402
    Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_INVALID,
    UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MODE_32,
    UC_PROT_EXEC, UC_PROT_READ, UC_PROT_WRITE,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_EFLAGS, UC_X86_REG_EIP, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_ESP,
)

BASE = 0x400000
EXE_SHA = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
EXPECTED_SPANS = {
    "AC60 closure": (0xAC60, 0x690, "f7724c04bef582e378b183a12550ba841df1a6fa3660be51da71154aa5c525af"),
    "AC60 sprite page selection": (0xB06B, 0x25, "09462af986f96a95f47dc1fbd9f98efe28ee720c622539d5afb66bf79aaf5bc6"),
    "AC60 sprite packet staging and dispatch": (0xB097, 0xDA, "6975b7a6366d4527511b57e5b2462c41cae5c5d1b0e1169baf2b8bbaaab4c54a"),
    "C8F0 function": (0xC8F0, 0x1C0, "95ac42d7f4447015eb0760dfccbf2c40fddf600baa76f8f885cf3be7ad9926bf"),
    "CAB0 function": (0xCAB0, 0x1A0, "fe6c00a7e9109250e2e5aa4c3c3419c7381cf9758422d42d319b3e4648d12609"),
    "C8F0 pre-loop": (0xC8F0, 0x161, "e027d326c9c0e30f8c653c4352eb10cc2615b6132407f2bbf9df7b76740a9d35"),
    "CAB0 pre-loop": (0xCAB0, 0x144, "ba3a45c5845dcd7dfa7b3dcc00ce1c89d7f02279bb4f40eed078593f182eecda"),
}

LOOP_STOP = {0xC8F0: 0xCA51, 0xCAB0: 0xCBF4}
FUNC_LIMIT = {0xC8F0: 0xCAA8, 0xCAB0: 0xCC45}
STACK = 0x20000000
STACK_SIZE = 0x10000
CALL_ESP = STACK + 0x8000
RETURN_SENTINEL = 0x30000000
SURFACE = 0xBA0210
STRIDE = 0xBA0214
DRAW_GATE = 0xBA01F0
SPRITE_X = 0xA909B8
SPRITE_Y = 0xA909C8
SOURCE_U = 0xA909D8
SOURCE_V = 0xA909E8
WIDTH = 0xA90A0C
HEIGHT = 0xA90A10
PAGE_BASE = 0xA909FC
DEST_PTR = 0xA90944
BLEND_PTR = 0xA90948
BLEND_SELECTOR = 0xA8E6F8
BOTTOM = 0x43A48C
RIGHT = 320
BLEND_FAMILY = 0x92E3D0
SYNTH_SURFACE = 0x31000000
SYNTH_PAGE = 0x32000000
MAX_INSNS = 2000


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def u16(value: int) -> int:
    return value & 0xFFFF


def s16(value: int) -> int:
    value &= 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


def u32(value: int) -> int:
    return value & 0xFFFFFFFF


def align(value: int) -> int:
    return (value + 0xFFF) & ~0xFFF


def pe_permissions(characteristics: int) -> int:
    result = 0
    if characteristics & 0x40000000:
        result |= UC_PROT_READ
    if characteristics & 0x80000000:
        result |= UC_PROT_WRITE
    if characteristics & 0x20000000:
        result |= UC_PROT_EXEC
    return result


def map_pe(uc: Uc, pe: PE) -> tuple[int, int]:
    header_size = pe.pe.OPTIONAL_HEADER.SizeOfHeaders
    uc.mem_map(BASE, align(header_size), UC_PROT_READ)
    uc.mem_write(BASE, pe.data[:header_size])
    data = None
    for section in pe.sections:
        virtual_size = max(section["virtual_size"], section["raw_size"])
        size = align(virtual_size)
        va = BASE + section["virtual_address"]
        uc.mem_map(va, size, pe_permissions(section["characteristics"]))
        if section["raw_size"]:
            uc.mem_write(va, pe.read_rva(section["virtual_address"], section["raw_size"]))
        if size > section["raw_size"]:
            uc.mem_write(va + section["raw_size"], b"\0" * (size - section["raw_size"]))
        if section["name"] == ".data":
            data = (va, virtual_size)
    if data is None:
        raise AssertionError("original PE has no .data section")
    return data


def expected_prepare(case: dict, textured: bool) -> dict:
    """Independent word-width model of pre-loop C8F0/CAB0 behavior."""
    x = int(case["x"])
    y = int(case["y"])
    width = s16(case["raw_width"])
    height = s16(case["raw_height"])
    u = int(case["u"])
    v = int(case["v"])
    state = {
        "x": x, "y": y, "width": u16(width), "height": u16(height),
        "u": u, "v": v, "page_base": SYNTH_PAGE,
        "dest_ptr": None, "blend_ptr": None, "row_v_offset": None,
        "reason": None,
    }
    if not case["gate"]:
        state["reason"] = "gate"
        return state
    if not case["surface_nonnull"]:
        state["reason"] = "null_surface"
        return state
    if x >= RIGHT or y >= case["bottom_limit"]:
        state["reason"] = "origin_outside"
        return state
    right = x + width
    bottom = y + height
    if right < 0 or bottom < 0:
        state["reason"] = "far_edge_before_origin"
        return state
    if x < 0:
        width = s16(width + x)
        u = u32(u - x)
        x = 0
    if y < 0:
        height = s16(height + y)
        v = u32(v - y)
        y = 0
    if right >= RIGHT:
        width = s16(width + RIGHT - right)
    if bottom >= case["bottom_limit"]:
        height = s16(height + case["bottom_limit"] - bottom)
    state.update(x=x, y=y, width=u16(width), height=u16(height), u=u, v=v)
    if width <= 0 or height <= 0:
        state["reason"] = "empty_after_clip"
        return state
    state["dest_ptr"] = u32(SYNTH_SURFACE + y * case["stride"] + x)
    state["page_base"] = u32(SYNTH_PAGE + u)
    state["row_v_offset"] = u32(v << 8)
    if textured:
        state["blend_ptr"] = u32(BLEND_FAMILY + ((case["blend_selector"] & 3) << 16))
    state["reason"] = "pre_loop"
    return state


def word_at(uc: Uc, address: int) -> int:
    return struct.unpack("<H", bytes(uc.mem_read(address, 2)))[0]


def dword_at(uc: Uc, address: int) -> int:
    return struct.unpack("<I", bytes(uc.mem_read(address, 4)))[0]


def store_word(uc: Uc, address: int, value: int) -> None:
    uc.mem_write(address, struct.pack("<H", value & 0xFFFF))


def store_dword(uc: Uc, address: int, value: int) -> None:
    uc.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))


def run_case(pe: PE, data_region: tuple[int, int], case: dict, entry: int) -> dict:
    stop_rva = LOOP_STOP[entry]
    limit_rva = FUNC_LIMIT[entry]
    textured = entry == 0xC8F0
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    mapped_data = map_pe(uc, pe)
    if mapped_data != data_region:
        raise AssertionError("inconsistent PE .data mapping")
    uc.mem_map(STACK, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_write(CALL_ESP, struct.pack("<I", RETURN_SENTINEL))
    # The surface/page values are deliberately unmapped. Reaching a pixel loop
    # would therefore stop as an invalid access if the boundary hook failed.
    for address, value in (
        (DRAW_GATE, int(case["gate"])),
        (SURFACE, SYNTH_SURFACE if case["surface_nonnull"] else 0),
        (STRIDE, case["stride"]),
        (SPRITE_X, case["x"]),
        (SPRITE_Y, case["y"]),
        (SOURCE_U, case["u"]),
        (SOURCE_V, case["v"]),
        (PAGE_BASE, SYNTH_PAGE),
        (BLEND_SELECTOR, case["blend_selector"]),
        (BOTTOM, case["bottom_limit"]),
    ):
        store_dword(uc, address, value)
    store_word(uc, WIDTH, case["raw_width"])
    store_word(uc, HEIGHT, case["raw_height"])

    code_start = BASE + entry
    code_stop = BASE + stop_rva
    function_limit = BASE + limit_rva
    data_va, data_size = data_region
    events = []
    instruction_trace = []
    stop = {"reason": None, "pc": None}
    error = {"kind": None, "detail": None}

    def on_code(machine, address, size, _):
        if address == code_stop:
            stop.update(reason="pre_loop", pc=address)
            machine.emu_stop()
            return
        if not code_start <= address < function_limit:
            error.update(kind="unexpected_code", detail=hex(address))
            machine.emu_stop()
            return
        instruction_trace.append(hex(address - BASE))

    def on_read(machine, access, address, size, value, _):
        pc = machine.reg_read(UC_X86_REG_EIP) - BASE
        data_access = data_va <= address and address + size <= data_va + data_size
        stack_access = STACK <= address and address + size <= STACK + STACK_SIZE
        if not data_access and not stack_access:
            error.update(kind="unexpected_read", detail=(hex(pc), hex(address), size))
            machine.emu_stop()
            return
        raw = bytes(machine.mem_read(address, size))
        events.append({"kind": "read", "rva": hex(pc), "address": hex(address),
                       "size": size, "value": int.from_bytes(raw, "little")})

    def on_write(machine, access, address, size, value, _):
        pc = machine.reg_read(UC_X86_REG_EIP) - BASE
        data_access = data_va <= address and address + size <= data_va + data_size
        stack_access = STACK <= address and address + size <= STACK + STACK_SIZE
        if not data_access and not stack_access:
            error.update(kind="unexpected_write", detail=(hex(pc), hex(address), size))
            machine.emu_stop()
            return
        events.append({"kind": "write", "rva": hex(pc), "address": hex(address),
                       "size": size, "value": value & ((1 << (size * 8)) - 1)})

    def on_invalid(machine, access, address, size, value, _):
        error.update(kind="invalid_memory", detail=(
            hex(machine.reg_read(UC_X86_REG_EIP) - BASE), access, hex(address), size
        ))
        machine.emu_stop()
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_READ, on_read)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
    uc.hook_add(UC_HOOK_MEM_INVALID, on_invalid)
    for register, value in (
        (UC_X86_REG_EAX, 0x11111111), (UC_X86_REG_EBX, 0x22222222),
        (UC_X86_REG_ECX, 0x33333333), (UC_X86_REG_EDX, 0x44444444),
        (UC_X86_REG_ESI, 0x55555555), (UC_X86_REG_EDI, 0x66666666),
        (UC_X86_REG_EBP, STACK + 0x4000), (UC_X86_REG_ESP, CALL_ESP),
        (UC_X86_REG_EFLAGS, 0x202), (UC_X86_REG_EIP, code_start),
    ):
        uc.reg_write(register, value)
    try:
        uc.emu_start(code_start, RETURN_SENTINEL, timeout=1_000_000, count=MAX_INSNS)
    except UcError as exception:
        if error["kind"] is None:
            error.update(kind="unicorn_error", detail=str(exception))
    if error["kind"] is not None:
        raise AssertionError(f"{case['name']} {hex(entry)}: {error}")
    if stop["reason"] is None and uc.reg_read(UC_X86_REG_EIP) == RETURN_SENTINEL:
        stop.update(reason="returned", pc=RETURN_SENTINEL)

    expected = expected_prepare(case, textured)
    wanted_stop = "pre_loop" if expected["reason"] == "pre_loop" else "returned"
    if stop["reason"] != wanted_stop:
        raise AssertionError(f"{case['name']} {hex(entry)}: stop {stop} != {wanted_stop}")
    observed = {
        "x": s16(dword_at(uc, SPRITE_X)),
        "y": s16(dword_at(uc, SPRITE_Y)),
        "width": word_at(uc, WIDTH),
        "height": word_at(uc, HEIGHT),
        "u": dword_at(uc, SOURCE_U),
        "v": dword_at(uc, SOURCE_V),
        "page_base": dword_at(uc, PAGE_BASE),
        "dest_ptr": dword_at(uc, DEST_PTR) if expected["reason"] == "pre_loop" else None,
        "blend_ptr": dword_at(uc, BLEND_PTR) if textured and expected["reason"] == "pre_loop" else None,
        "row_v_offset": None,
        "reason": expected["reason"],
    }
    # The local's address is EBP-after-prologue minus four: initial call ESP - 8.
    if expected["reason"] == "pre_loop":
        observed["row_v_offset"] = dword_at(uc, CALL_ESP - 8)
    compare_keys = ("x", "y", "width", "height", "u", "v", "page_base", "dest_ptr", "blend_ptr", "row_v_offset")
    for key in compare_keys:
        if observed[key] != expected[key]:
            raise AssertionError(f"{case['name']} {hex(entry)}: {key} {observed[key]!r} != {expected[key]!r}")

    data_writes = [event for event in events if event["kind"] == "write" and
                   data_va <= int(event["address"], 16) < data_va + data_size]
    row_local_writes = [event for event in events if event["kind"] == "write" and
                        event["rva"] in ("0xca28", "0xcbd3")]
    result = {
        "case": case["name"], "entry_rva": hex(entry), "boundary_rva": hex(stop_rva),
        "stop": stop["reason"], "expected": expected, "observed": observed,
        "global_writes": data_writes,
        "row_local_writes": row_local_writes,
        "global_read_count": sum(1 for event in events if event["kind"] == "read" and
                                  data_va <= int(event["address"], 16) < data_va + data_size),
        "executed_instruction_count": len(instruction_trace),
    }
    return result


def main() -> None:
    runtime = verify_runtime(unicorn, ARGS.deps)
    pe = PE(ARGS.exe)
    exe_bytes = ARGS.exe.read_bytes()
    if sha(exe_bytes) != EXE_SHA:
        raise AssertionError("pinned original EXE identity changed")
    spans = {}
    for label, (start, size, expected_hash) in EXPECTED_SPANS.items():
        digest = sha(pe.read_rva(start, size))
        if digest != expected_hash:
            raise AssertionError(f"{label} hash {digest} != {expected_hash}")
        spans[label] = {"rva": hex(start), "size": size, "sha256": digest}

    # Guard the exact pre-pixel addresses and the lack of direct calls in both
    # original function spans. The report preserves the fixed disassembly cuts.
    import capstone
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    calls = []
    boundaries = {}
    references = set()
    for start, stop in ((0xC8F0, 0xCA51), (0xCAB0, 0xCBF4)):
        code = pe.read_rva(start, stop - start)
        insns = list(decoder.disasm(code, BASE + start))
        expected_end = BASE + stop
        if not insns or insns[-1].address + insns[-1].size != expected_end:
            raise AssertionError(f"disassembly did not end on stated boundary {hex(stop)}")
        boundaries[hex(start)] = {
            "stop_before": hex(stop), "instruction_count": len(insns),
            "last_instruction": {
                "rva": hex(insns[-1].address - BASE), "bytes": insns[-1].bytes.hex(),
                "mnemonic": insns[-1].mnemonic, "operands": insns[-1].op_str,
            },
        }
        for insn in insns:
            if insn.group(capstone.CS_GRP_CALL):
                calls.append({"rva": hex(insn.address - BASE), "mnemonic": insn.mnemonic,
                              "operands": insn.op_str})
            for operand in insn.operands:
                if operand.type == capstone.x86.X86_OP_MEM and operand.mem.base == 0 and operand.mem.index == 0:
                    references.add(operand.mem.disp & 0xFFFFFFFF)
    if calls:
        raise AssertionError(f"unexpected calls before pixel loops: {calls}")

    data_va = BASE + next(section["virtual_address"] for section in pe.sections if section["name"] == ".data")
    data_size = max(next(section["virtual_size"] for section in pe.sections if section["name"] == ".data"),
                    next(section["raw_size"] for section in pe.sections if section["name"] == ".data"))
    cases = [
        {"name": "plain", "x": 10, "y": 20, "raw_width": 4, "raw_height": 5,
         "u": 250, "v": 254, "gate": True, "surface_nonnull": True, "stride": 640,
         "bottom_limit": 240, "blend_selector": 2},
        {"name": "left_top_advance_without_byte_wrap", "x": -3, "y": -5,
         "raw_width": 7, "raw_height": 8, "u": 254, "v": 253, "gate": True,
         "surface_nonnull": True, "stride": 640, "bottom_limit": 240, "blend_selector": 3},
        {"name": "right_bottom_clamp", "x": 318, "y": 238, "raw_width": 8,
         "raw_height": 8, "u": 255, "v": 255, "gate": True, "surface_nonnull": True,
         "stride": 640, "bottom_limit": 240, "blend_selector": 1},
        {"name": "fully_left_rejected_before_clip", "x": -8, "y": 2,
         "raw_width": 7, "raw_height": 1, "u": 5, "v": 9, "gate": True,
         "surface_nonnull": True, "stride": 640, "bottom_limit": 240, "blend_selector": 0},
        {"name": "negative_width_signed", "x": 10, "y": 10, "raw_width": 0xFFFF,
         "raw_height": 2, "u": 5, "v": 9, "gate": True, "surface_nonnull": True,
         "stride": 640, "bottom_limit": 240, "blend_selector": 0},
        {"name": "most_negative_width_signed", "x": 10, "y": 10, "raw_width": 0x8000,
         "raw_height": 2, "u": 5, "v": 9, "gate": True, "surface_nonnull": True,
         "stride": 640, "bottom_limit": 240, "blend_selector": 0},
        {"name": "left_clip_becomes_empty_but_advances_u", "x": -5, "y": 1,
         "raw_width": 5, "raw_height": 2, "u": 251, "v": 9, "gate": True,
         "surface_nonnull": True, "stride": 640, "bottom_limit": 240, "blend_selector": 0},
        {"name": "zero_width", "x": 7, "y": 9, "raw_width": 0,
         "raw_height": 2, "u": 1, "v": 2, "gate": True, "surface_nonnull": True,
         "stride": 640, "bottom_limit": 240, "blend_selector": 0},
        {"name": "zero_height", "x": 7, "y": 9, "raw_width": 2,
         "raw_height": 0, "u": 1, "v": 2, "gate": True, "surface_nonnull": True,
         "stride": 640, "bottom_limit": 240, "blend_selector": 0},
        {"name": "negative_height_signed", "x": 7, "y": 9, "raw_width": 2,
         "raw_height": 0xFFFE, "u": 1, "v": 2, "gate": True, "surface_nonnull": True,
         "stride": 640, "bottom_limit": 240, "blend_selector": 0},
        {"name": "fully_above_rejected_before_clip", "x": 4, "y": -3,
         "raw_width": 2, "raw_height": 2, "u": 1, "v": 253, "gate": True,
         "surface_nonnull": True, "stride": 640, "bottom_limit": 240, "blend_selector": 0},
        {"name": "gate_stops_before_mutation", "x": -1, "y": -1,
         "raw_width": 3, "raw_height": 3, "u": 4, "v": 5, "gate": False,
         "surface_nonnull": True, "stride": 640, "bottom_limit": 240, "blend_selector": 0},
        {"name": "null_surface_stops_before_mutation", "x": -1, "y": -1,
         "raw_width": 3, "raw_height": 3, "u": 4, "v": 5, "gate": True,
         "surface_nonnull": False, "stride": 640, "bottom_limit": 240, "blend_selector": 0},
    ]
    results = [run_case(pe, (data_va, data_size), case, entry)
               for case in cases for entry in (0xC8F0, 0xCAB0)]
    report = {
        "schema_version": 1,
        "scope": "C8F0/CAB0 pre-pixel sprite geometry only; execution stops before CA51/CBF4",
        "oracle": {"path": str(ARGS.exe), "sha256": EXE_SHA, "image_base": hex(BASE)},
        "runtime": runtime,
        "analysis": {"capstone_version": capstone.__version__},
        "spans": spans,
        "boundaries": boundaries,
        "direct_calls_before_loops": calls,
        "execution_policy": {
            "selected_code_only": True,
            "loop_entry_stops": ["0xca51", "0xcbf4"],
            "unknown_pc_or_memory_access_fails": True,
            "original_code_mapped_read_execute": True,
            "full_game_or_os_endpoints": False,
            "separate_emulator_per_case": True,
        },
        "absolute_data_references": [hex(value) for value in sorted(references)],
        "facts": {
            "entry_abi": "both are no-argument internal routines reading/writing fixed EXE globals; no calls occur in either bounded body",
            "ac60_sprite_staging": "tag 0x64 sign-extends packet i16 x/y at +8/+0xA, zero-extends U/V bytes +0xC/+0xD, and copies dimension words +0x10/+0x12; tags 0x74/0x7C stage 8x8/16x16; tag bit 1 selects C8F0 when set, CAB0 when clear",
            "ac60_page_selection": "before sprite entry, A909FC is 0x96E6E0+(((A8E6F4&0x1F)>>1)<<16), overridden to 0xA6E6E0 when A8E6FC is nonzero",
            "coordinate_inputs": "A909B8/A909C8 are signed 32-bit staged x/y; A90A0C/A90A10 are sign-extended i16 extents at use (variable packet words are copied raw; fixed commands stage 8 or 16)",
            "uv_inputs": "A909D8/A909E8 are zero-extended packet UV bytes from AC60; top/left clipping increases offsets by the clipped amount without byte masking",
            "clip_order": "reject x>=320 or y>=bottom, then reject x+signed_width<0 or y+signed_height<0; clip left/top and advance U/V, clip right/bottom by shrinking dimensions, then return if signed width/height <=0",
            "preloop_setup": "for positive clipped extents, A90944=surface+y*stride+x and A909FC=selected_page+U; C8F0 additionally writes A90948=0x92E3D0+((A8E6F8&3)<<16); both stage V<<8 at [EBP-4] before their respective pixel loop",
            "empty_side_effects": "left/top clip mutations to x/y, width/height, U/V remain visible even when the clipped extent becomes zero; destination/page/blend/local-row setup does not occur on these empty exits",
            "clip_bounds": {
                "left": "constant zero; negative x is detected at C958/CB18 and x is set to zero at C96B/C973 or CB2B/CB33",
                "top": "constant zero; negative y is detected at C979/CB39 and y is set to zero at C98C/C994 or CB4C/CB54",
                "right": "constant 320 (0x140), compared at C917/C9A1 and CAD7/CB61; A909B0, A909B4, and 43A488 are not referenced in either pre-pixel code slice",
                "bottom": "dword read from 43A48C at C928/C9B7 and CAE8/CB77; used for the origin reject and bottom shrink",
            },
            "after_boundary": "C8F0 starts its blended pixel loop at CA51; CAB0 starts its direct-copy loop at CBF4. Static row update adds 0x100 and masks the 16-bit row offset at CA7C/CA86 and CC13/CC1D; pixel writes are covered separately.",
        },
        "native_cases": results,
        "limits": [
            "Finite test dimensions, clipping bounds 320x240, synthetic pointer values and fixture states only.",
            "No pixel-loop instruction executes; no complete frame/game is run.",
            "No complete native VRAM backing interval or upstream upload ownership is claimed.",
            "The separately compiled C fixture checks layout and selected scenarios; it does not compare every C result to all 26 native trace vectors.",
        ],
    }
    ARGS.receipt.parent.mkdir(parents=True, exist_ok=True)
    ARGS.receipt.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": str(ARGS.receipt), "cases": len(results),
                      "spans": spans, "calls_before_loops": calls}, indent=2))


if __name__ == "__main__":
    main()
