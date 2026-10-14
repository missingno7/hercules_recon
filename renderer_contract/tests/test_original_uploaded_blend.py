"""Probe connected original A710 upload -> AC60 textured blend executions.

Prediction recorded before this case family: after A710 stores the supplied
packed upload bytes, AC60 with a finite 2x2 FT4 and packet red 0x80 should route
through E680/C8F0 with raw source indices; packet red 0x7f should route through
DD70/C700, read the selected 256-byte shade row, then index the same selected
blend bank with (shaded_source << 8) | destination. A zero texture source should
skip shade/blend/destination traffic; a nonzero source mapped to shade zero
should still blend. Falsifiers: any byte mismatch, route mismatch, missing or
extra table/destination access, wrong dirty-flag PC, unexpected code/memory,
changed original code, or instruction-limit stop. This is bounded synthetic
original-byte evidence, not function-match or full-frame acceptance.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))
from oracle_common import verify_runtime  # noqa: E402

BASE = 0x00400000
EXE_SHA = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
SHADE_BASE = 0x0091C488
BLEND_BASE = 0x0092E3D0
SHADE_MODE = 0xA90A00
ROW_CODE = 0xA90A04
TPAGE_CELL = 0xA8E6F8
DIRTY = 0x0043A590
MAX_INSNS = 300000
FRAME_BYTES = 320 * 240

# All function spans use the established native receipt boundaries and hashes.
PINNED_SPANS = {
    "A710_upload": (0xA710, 0xA8E5, "30bd84ea8724152e3bfb0e2c521650399913bcec115f1aef489768085981d0fe"),
    "AC60_packet_chain": (0xAC60, 0xB2F0, "f7724c04bef582e378b183a12550ba841df1a6fa3660be51da71154aa5c525af"),
    "D420_dispatch": (0xD420, 0xDD70, "4645c60cf0a357ed4aae074c3487a2755384852d0874e2f12b352b994544e07e"),
    "DD70_shaded_dispatch_and_body": (0xDD70, 0xE675, "2d13dbe76315a0e9e0b2fbec9fc1d84beffc377ee8df2988a6f1f354329cf9be"),
    "E680_raw_dispatch_and_body": (0xE680, 0xEF60, "f42bfcb06d042a8cd9ced6a8df5375b734c1e5641811a062059bbd02cb8dcb58"),
    "C700_shaded_rectangle_kernel": (0xC700, 0xC8E3, "399af24f03efd83d72c62c74a808f0f5e4bceb87f48205524d7b53d88063316f"),
    "C8F0_raw_rectangle_kernel": (0xC8F0, 0xCAA8, "74bfb760505e5695cf5f1668e390d07046a32f7014c506706a85f9488870d52c"),
    "CAB0_unshaded_rectangle_dispatch": (0xCAB0, 0xCC45, "5ca22ca070b69abaf98a0da5c794b0642cf816026b7ff87c794d6a82f6d9c71f"),
    "surface_lock": (0x6FF0, 0x70B7, "d6f022e5f7103bec40e16000f8a4e8e08ea186c337d828f29afd46f4f8d3e88e"),
    "surface_remap": (0x70C0, 0x7136, "f5225f837714b2dc9379a6e1358bef73dbfcf9bc32393c47581acf62f4ba5d40"),
    "surface_unlock": (0x7140, 0x716E, "7d70b14e30cf0c37b0215c471953a0472b799067aecbb0eb368e9cf994ff7f64"),
    "present_tail": (0x6620, 0x66F5, "52a5fe4fd4d5d08288467e6e8d5daef54f3bb176218847788e9d0fbcea7f0287"),
}

CASES = {
    **{f"raw_s{selector}": {"mode": 0x80, "selector": selector,
                              "upload": bytes((0x11, 0x23, 0x35, 0x47))}
       for selector in range(4)},
    **{f"shaded_s{selector}": {"mode": 0x7F, "selector": selector,
                                 "upload": bytes((0x11, 0x23, 0x35, 0x47))}
       for selector in range(4)},
    "raw_transparent": {"mode": 0x80, "selector": 2, "upload": bytes(4)},
    "shaded_transparent": {"mode": 0x7F, "selector": 2, "upload": bytes(4)},
    "shaded_zero": {"mode": 0x7F, "selector": 2,
                    "upload": bytes((0x31, 0x23, 0x35, 0x47))},
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def shade_value(index: int) -> int:
    """Deterministic non-identity row; 0x31 intentionally maps to zero."""
    if index == 0x31:
        return 0
    return ((index * 29 + 71) ^ (index >> 1) ^ ((index * index) >> 5)) & 0xFF


def blend_value(source: int, destination: int, selector: int) -> int:
    """Deterministic nonlinear and selector-distinct synthetic blend LUT."""
    return (source * 37 + destination * 13 + selector * 59 +
            ((source ^ destination) * 7) + ((source * destination) >> 4) + 17) & 0xFF


def load_mapper(exe: Path):
    helper = ROOT / "renderer_contract" / "fixtures" / "composition" / "raster" / "tests" / "run_original_composition_raster.py"
    old_argv = sys.argv[:]
    sys.argv = [str(helper), "--repo-root", str(ROOT), "--exe", str(exe),
                "--deps", "C:/tools/hercules/renderer-oracle-python",
                "--analysis-deps", "C:/tools/hercules/python"]
    try:
        spec = importlib.util.spec_from_file_location("uploaded_blend_composition_mapper", helper)
        if spec is None or spec.loader is None:
            raise RuntimeError("could not import existing original composition mapper")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return helper, module
    finally:
        sys.argv = old_argv


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--case", choices=tuple(CASES), required=True)
    args = parser.parse_args()

    sys.path.insert(0, str(ROOT / "renderer_contract" / "fixtures" / "eng1_real_mesh"))
    from pathguard import _under_work, input_file  # noqa: E402
    args.exe = input_file(args.exe, "original EXE")
    receipt_path = _under_work(ROOT, args.receipt)
    if receipt_path.exists():
        raise ValueError("receipt must be fresh")
    if sha(args.exe.read_bytes()) != EXE_SHA:
        raise AssertionError("pinned original EXE SHA-256 mismatch")

    case = CASES[args.case]
    helper, R = load_mapper(args.exe)
    runtime = verify_runtime(R.unicorn, Path("C:/tools/hercules/renderer-oracle-python"))
    pe = R.PE(args.exe)
    if sha(pe.data) != EXE_SHA or pe.image_base != BASE:
        raise AssertionError("pinned original image identity/base mismatch")

    uc = R.Uc(R.UC_ARCH_X86, R.UC_MODE_32)
    R.map_original(uc, pe)
    for address, length, permissions in (
        (R.INPUT, 0x4000, R.UC_PROT_READ | R.UC_PROT_WRITE),
        (R.STACK, R.STACK_SIZE, R.UC_PROT_READ | R.UC_PROT_WRITE),
        (R.PIXELS, R.PIXEL_SIZE, R.UC_PROT_READ | R.UC_PROT_WRITE),
        (R.DIB_INFO, R.PAGE, R.UC_PROT_READ | R.UC_PROT_WRITE),
        (R.STRETCH_STUB, R.PAGE, R.UC_PROT_READ | R.UC_PROT_EXEC),
        (R.SENTINEL, R.PAGE, R.UC_PROT_READ | R.UC_PROT_EXEC),
    ):
        uc.mem_map(address, length, permissions)

    # A710 receives one packed word per row, for two rows at byte U=0,V=0.
    x, y, width, height, u, v = 1, 1, 2, 2, 0, 0
    upload_rect = (u // 2, v, (width + 1) // 2, height)
    rect_address, source_address = R.INPUT + 0x2000, R.INPUT + 0x2100
    uc.mem_write(rect_address, struct.pack("<4h", *upload_rect))
    uc.mem_write(source_address, case["upload"])
    packet_address, end_address, ot = R.INPUT + 0x1000, R.INPUT + 0x1040, R.INPUT + 0x100
    packet_bytes = R.packet(0x2E, 40, end_address, {
        4: bytes((case["mode"], 0, 0)),
        8: R.s16(x), 10: R.s16(y), 12: bytes((u, v)), 14: R.u16(0),
        16: R.s16(x + width), 18: R.s16(y), 20: bytes((u + width, v)),
        22: R.u16(0x20 * case["selector"]),
        24: R.s16(x), 26: R.s16(y + height), 28: bytes((u, v + height)),
        30: R.u16(0),
        32: R.s16(x + width), 34: R.s16(y + height),
        36: bytes((u + width, v + height)), 38: R.u16(0),
    })
    uc.mem_write(packet_address, packet_bytes)
    uc.mem_write(end_address, R.packet(0x40, 8, 0, {}))
    R.put32(uc, ot, packet_address)
    uc.mem_write(R.TEXTURE, bytes(65536))
    uc.mem_write(R.PIXELS, bytes((0xA5,)) * R.PIXEL_SIZE)

    # Seed only the selected native row and selected native blend table. Their
    # address/hash/recipe are receipt inputs; no original or prior output table
    # is used as an oracle.
    selector = case["selector"]
    mode = case["mode"]
    shade_index = mode & 0xFF
    shade_row = bytes(shade_value(index) for index in range(256))
    blend_table = bytes(blend_value(source, destination, selector)
                        for source in range(256) for destination in range(256))
    shade_address = SHADE_BASE + shade_index * 256
    blend_address = BLEND_BASE + selector * 0x10000
    uc.mem_write(shade_address, shade_row)
    uc.mem_write(blend_address, blend_table)

    cells = {
        0x00BA01FC: b"\0", 0x00BA020C: b"\0", 0x0096E698: b"\0",
        0x0096E3D8: struct.pack("<I", R.PIXELS),
        0x0096E3F4: struct.pack("<I", R.DIB_INFO),
        0x0096E400: struct.pack("<I", R.HDC_TOKEN),
        0x00BA0220: struct.pack("<I", 320), 0x00BA0224: struct.pack("<I", 240),
        0x00BA5664: struct.pack("<I", R.STRETCH_STUB),
        0x00BA56F4: struct.pack("<I", R.FLUSH_STUB),
        0x00A90A04: struct.pack("<I", 0x7800),
        0x00A90A00: struct.pack("<I", 0),
        0x0096E3E0: struct.pack("<I", 0),
        0x00436A0C: b"\x80",
        0x00A8E780: struct.pack("<I", 0x31000000),
        0x00A8E788: b"\0" * 4,
        0x0043A590: b"\0",
    }
    for address, value in cells.items():
        uc.mem_write(address, value)

    spans = [(lo, hi - lo) for lo, hi, _ in R.ALLOWED]
    spans.extend((lo, hi - lo) for lo, hi, _digest in PINNED_SPANS.values())
    spans = sorted(set(spans))
    before_code = {lo: bytes(uc.mem_read(BASE + lo, size)) for lo, size in spans}
    for name, (lo, hi, expected) in PINNED_SPANS.items():
        actual = sha(before_code[lo])
        if actual != expected:
            raise AssertionError(f"{name} pinned span hash mismatch: {actual}")

    state = {"phase": "upload", "instructions": 0, "stop": None}
    trace: list[int] = []
    routes: list[str] = []
    upload_stores: list[dict] = []
    texture_reads: list[dict] = []
    shade_reads: list[dict] = []
    blend_reads: list[dict] = []
    surface_reads: list[dict] = []
    surface_stores: list[dict] = []
    dirty_events: list[dict] = []
    services: list[str] = []
    allowed = spans

    def code(machine, address, size, _context):
        state["instructions"] += 1
        if state["instructions"] > MAX_INSNS:
            raise AssertionError("instruction limit exceeded")
        if address == R.SENTINEL:
            state["stop"] = "returned"
            machine.emu_stop()
            return
        if address in (R.STRETCH_STUB, R.FLUSH_STUB):
            assert state["phase"] == "frame"
            services.append("StretchDIBits" if address == R.STRETCH_STUB else "GdiFlush")
            stack = machine.reg_read(R.UC_X86_REG_ESP)
            machine.reg_write(R.UC_X86_REG_EIP, R.u32(machine, stack))
            machine.reg_write(R.UC_X86_REG_ESP, stack + (56 if address == R.STRETCH_STUB else 4))
            machine.reg_write(R.UC_X86_REG_EAX, 1)
            return
        rva = address - BASE
        if not any(lo <= rva < lo + size for lo, size in allowed):
            raise AssertionError(f"unexpected original code RVA {rva:#x}; trace={trace[-24:]}")
        if state["phase"] == "frame" and rva in {
            0xD420, 0xDD70, 0xDE9F, 0xC700, 0xE680, 0xE7C7, 0xC8F0,
            0xCAB0, 0xCC37, 0x6637, 0x6FF0, 0x70C0, 0x7140, 0x6620,
        }:
            routes.append(hex(rva))
        trace.append(rva)

    def read(machine, _access, address, size, _value, _context):
        pc = machine.reg_read(R.UC_X86_REG_EIP) - BASE
        raw = bytes(machine.mem_read(address, size))
        event = {"address": hex(address), "size": size, "value_hex": raw.hex(),
                 "pc_rva": hex(pc)}
        if R.TEXTURE <= address and address + size <= R.TEXTURE + 65536:
            if state["phase"] == "frame":
                texture_reads.append(event)
        elif shade_address <= address and address + size <= shade_address + len(shade_row):
            if state["phase"] == "frame":
                shade_reads.append(event)
        elif blend_address <= address and address + size <= blend_address + len(blend_table):
            if state["phase"] == "frame":
                blend_reads.append(event)
        elif R.PIXELS <= address and address + size <= R.PIXELS + R.PIXEL_SIZE:
            if state["phase"] == "frame":
                surface_reads.append({**event, "offset": address - R.PIXELS})

    def write(machine, _access, address, size, value, _context):
        pc = machine.reg_read(R.UC_X86_REG_EIP) - BASE
        raw = int(value & ((1 << (8 * size)) - 1)).to_bytes(size, "little")
        if R.TEXTURE <= address and address + size <= R.TEXTURE + 65536:
            if state["phase"] == "upload":
                upload_stores.append({"address": hex(address), "bytes_hex": raw.hex(), "pc_rva": hex(pc)})
            else:
                raise AssertionError("draw wrote texture storage")
        elif R.PIXELS <= address and address + size <= R.PIXELS + R.PIXEL_SIZE:
            if state["phase"] != "frame":
                raise AssertionError("surface write occurred during upload")
            surface_stores.append({"offset": address - R.PIXELS, "size": size,
                                   "bytes_hex": raw.hex(), "pc_rva": hex(pc)})
        if address == DIRTY:
            dirty_events.append({"phase": state["phase"], "pc_rva": hex(pc),
                                 "value": value & ((1 << (8 * size)) - 1), "size": size})

    def invalid(machine, access, address, size, value, _context):
        raise AssertionError(f"unmapped endpoint access={access} address={address:#x} size={size}")

    uc.hook_add(R.UC_HOOK_CODE, code)
    uc.hook_add(R.UC_HOOK_MEM_READ, read)
    uc.hook_add(R.UC_HOOK_MEM_WRITE, write)
    uc.hook_add(R.UC_HOOK_MEM_INVALID, invalid)

    stack = R.STACK + R.STACK_SIZE - 0x100
    uc.mem_write(stack, struct.pack("<III", R.SENTINEL, rect_address, source_address))
    uc.reg_write(R.UC_X86_REG_ESP, stack)
    uc.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
    uc.emu_start(BASE + 0xA710, 0, count=MAX_INSNS + 1)
    if state["stop"] != "returned":
        raise AssertionError(f"A710 did not return cleanly: {state}")
    expected_texture = bytearray(65536)
    for row in range(height):
        start = (v + row) * 256 + u
        expected_texture[start:start + 2] = case["upload"][row * 2:row * 2 + 2]
    actual_texture = bytes(uc.mem_read(R.TEXTURE, 65536))
    if actual_texture != bytes(expected_texture):
        raise AssertionError("A710 upload bytes differ from declared packed input")
    upload_instruction_count = state["instructions"]
    dirty_before_draw = bytes(uc.mem_read(DIRTY, 1))[0]
    if dirty_before_draw != 0:
        raise AssertionError(f"dirty flag must be zero at connected draw entry; got {dirty_before_draw}")

    # Tables are installed before the connected original draw; the selected
    # destination page and surface are controlled synthetic buffers.
    state.update(phase="frame", stop=None)
    uc.mem_write(stack, struct.pack("<II", R.SENTINEL, ot))
    uc.reg_write(R.UC_X86_REG_ESP, stack)
    uc.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
    uc.emu_start(BASE + 0xAC60, 0, count=MAX_INSNS + 1)
    if state["stop"] != "returned":
        raise AssertionError(f"AC60 frame did not return cleanly: {state}")

    # Assert byte-for-byte output against independent fixture recipes, while
    # retaining the actual native table/destination access traces in the receipt.
    expected_frame = bytearray((0xA5,)) * R.PIXEL_SIZE
    samples = list(case["upload"])
    expected_values = []
    expected_blend_addresses = []
    expected_shade_addresses = []
    for index, source in enumerate(samples):
        if source == 0:
            continue
        shaded = shade_value(source) if mode != 0x80 else source
        offset = (y + index // width) * 320 + x + (index % width)
        destination = 0xA5
        out = blend_value(shaded, destination, selector)
        expected_frame[offset] = out
        expected_values.append({"offset": offset, "source_index": source,
                                "blend_source_index": shaded, "destination_before": destination,
                                "output": out})
        expected_blend_addresses.append(hex(blend_address + shaded * 256 + destination))
        if mode != 0x80:
            expected_shade_addresses.append(hex(shade_address + source))
    output = bytes(uc.mem_read(R.PIXELS, R.PIXEL_SIZE))
    if output != bytes(expected_frame):
        raise AssertionError("original CPU surface bytes disagree with independent synthetic LUT recipes")

    expected_source_reads = [
        {"address": hex(R.TEXTURE + (v + index // width) * 256 + u + (index % width)),
         "size": 1, "value_hex": f"{source:02x}"}
        for index, source in enumerate(samples)
    ]
    observed_source_reads = [{key: row[key] for key in ("address", "size", "value_hex")}
                             for row in texture_reads]
    if observed_source_reads != expected_source_reads:
        raise AssertionError("original source texture read addresses/values differ from uploaded bytes")
    if [row["address"] for row in blend_reads] != expected_blend_addresses:
        raise AssertionError("original selected blend LUT addresses differ from independent source/destination indices")
    if [row["value_hex"] for row in blend_reads] != [f"{row['output']:02x}" for row in expected_values]:
        raise AssertionError("original selected blend LUT values differ from the synthetic writer recipe")
    expected_shade_reads = expected_shade_addresses
    if [row["address"] for row in shade_reads] != expected_shade_reads:
        raise AssertionError("original shade row addresses differ from uploaded nonzero source indices")
    expected_shade_values = ([f"{shade_value(source):02x}" for source in samples if source != 0]
                             if mode != 0x80 else [])
    if [row["value_hex"] for row in shade_reads] != expected_shade_values:
        raise AssertionError("original shade row values differ from the synthetic writer recipe")
    if [row["offset"] for row in surface_reads] != [row["offset"] for row in expected_values]:
        raise AssertionError("original destination read offsets differ from nonzero source coverage")
    if [row["value_hex"] for row in surface_reads] != ["a5"] * len(expected_values):
        raise AssertionError("original destination reads did not see the seeded initial surface")
    if [(row["offset"], row["bytes_hex"]) for row in surface_stores] != [
            (row["offset"], f"{row['output']:02x}") for row in expected_values]:
        raise AssertionError("original surface stores differ from the synthetic LUT recipe")

    selected_route = ("0xe680", "0xc8f0") if mode == 0x80 else ("0xdd70", "0xc700")
    if not all(route in routes for route in selected_route):
        raise AssertionError(f"expected original raster route {selected_route}; observed {routes}")
    opposite_terminal = "0xc700" if mode == 0x80 else "0xc8f0"
    if opposite_terminal in routes:
        raise AssertionError(f"unexpected opposite rectangle kernel {opposite_terminal}; observed {routes}")
    expected_store_count = len(expected_values)
    if len(surface_stores) != expected_store_count:
        raise AssertionError(f"surface stores {len(surface_stores)} != {expected_store_count}")
    if len(blend_reads) != expected_store_count or len(surface_reads) != expected_store_count:
        raise AssertionError("blend/destination read traffic does not match nonzero source count")
    if mode != 0x80 and len(shade_reads) != expected_store_count:
        raise AssertionError("shade reads do not match nonzero source count")
    if mode == 0x80 and shade_reads:
        raise AssertionError("raw path unexpectedly read shade row")
    if (args.case == "shaded_zero" and
            not any(item["blend_source_index"] == 0 for item in expected_values)):
        raise AssertionError("shaded_zero discriminator did not produce source index zero")
    if args.case == "shaded_zero" and len(blend_reads) != 4:
        raise AssertionError("zero-valued shade result failed to continue into blend lookup")

    dirty_set_rva = "0xca9a" if mode == 0x80 else "0xc8d5"
    expected_dirty = [(dirty_set_rva, 1), ("0x6637", 0)]
    observed_dirty = [(event["pc_rva"], event["value"]) for event in dirty_events]
    if observed_dirty != expected_dirty:
        raise AssertionError(f"dirty flag set/clear events {observed_dirty} != {expected_dirty}")
    if services != ["StretchDIBits", "GdiFlush"]:
        raise AssertionError(f"unexpected surface service sequence {services}")
    code_after = {lo: bytes(uc.mem_read(BASE + lo, size)) for lo, size in spans}
    if before_code != code_after:
        raise AssertionError("original code changed during bounded execution")

    # Compact each route/access list as finite evidence; full tables, whole
    # texture frames, or disassembly are intentionally omitted.
    selected_trace = [hex(rva) for rva in trace if rva in {
        0xD420, 0xDD70, 0xDE9F, 0xC700, 0xE680, 0xE7C7, 0xC8F0,
        0xCAB0, 0xCC37, 0x6637, 0x6FF0, 0x70C0, 0x7140, 0x6620,
    }]
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_ORIGINAL_UPLOADED_BLEND_PASS",
        "original_process_executed": False,
        "function_match_promoted": False,
        "prediction_falsifier": __doc__,
        "case": args.case,
        "oracle": {"exe_sha256": sha(pe.data), "image_base": hex(pe.image_base)},
        "runtime": runtime,
        "runner": {"path": str(Path(__file__).resolve()), "sha256": sha(Path(__file__).read_bytes())},
        "mapper": {"path": str(helper.resolve()), "sha256": sha(helper.read_bytes())},
        "original_spans": [
            {"name": name, "rva": hex(lo), "size": hi - lo, "sha256": sha(before_code[lo])}
            for name, (lo, hi, _digest) in PINNED_SPANS.items()
        ],
        "input": {
            "packet_address": hex(packet_address), "packet_hex": packet_bytes.hex(),
            "packet_code": "0x2e", "rectangle_xywh_uv": [x, y, width, height, u, v],
            "packet_red_mode": hex(mode), "clut": "0x0000",
            "pagezero_selector_bits": hex(0x20 * selector), "selector": selector,
            "upload_rect_words_xywh": list(upload_rect), "upload_payload_hex": case["upload"].hex(),
            "a710_upload_instruction_count": upload_instruction_count,
            "dirty_flag_before_draw": dirty_before_draw,
            "initial_surface_byte": 0xA5, "surface_dimensions_pitch": [320, 240, 320],
            "texture_dimensions_pitch": [256, 256, 256],
        },
        "synthetic_table_writers": {
            "shade_row": {"address": hex(shade_address), "size": len(shade_row),
                          "row_index": shade_index,
                          "recipe": "shade[0x31]=0; otherwise ((index*29+71) XOR (index>>1) XOR ((index*index)>>5)) & 0xff",
                          "sha256": sha(shade_row)},
            "selected_blend_lut": {"address": hex(blend_address), "size": len(blend_table),
                                   "selector": selector,
                                   "recipe": "(source*37 + destination*13 + selector*59 + ((source XOR destination)*7) + ((source*destination)>>4) + 17) & 0xff; row-major source,destination",
                                   "sha256": sha(blend_table)},
            "note": "fixture tables were explicitly written before AC60; no original output or C implementation supplied these values",
        },
        "a710_texture_writes": upload_stores,
        "original_route_markers": selected_trace,
        "original_route_pair": list(selected_route),
        "texture_reads": texture_reads,
        "shade_row_reads": shade_reads,
        "selected_blend_lut_reads": blend_reads,
        "surface_destination_reads": surface_reads,
        "surface_stores": surface_stores,
        "expected_pixel_values_from_fixture_recipes": expected_values,
        "expected_blend_entry_addresses": expected_blend_addresses,
        "expected_shade_entry_addresses": expected_shade_addresses,
        "dirty_flag_events": dirty_events,
        "services": services,
        "frame_bytes": FRAME_BYTES,
        "frame_sha256": sha(output[:FRAME_BYTES]),
        "mapped_surface_bytes": R.PIXEL_SIZE,
        "mapped_surface_sha256": sha(output),
        "instruction_count_total": state["instructions"],
        "limits": [
            "one finite 2x2 connected A710 upload and AC60 textured rectangle in one pinned Unicorn image",
            "synthetic texture payload, one selected shade row, one selected blend table, and finite synthetic surface",
            "no original game process, original asset/table, C implementation, full frame, or whole-renderer claim",
        ],
    }
    receipt_path.parent.mkdir(parents=True, exist_ok=True)
    receipt_path.write_bytes((json.dumps(receipt, indent=2) + "\n").encode("utf-8"))
    print(json.dumps({"status": receipt["status"], "case": args.case,
                      "routes": list(selected_route), "surface_stores": len(surface_stores),
                      "shade_reads": len(shade_reads), "blend_reads": len(blend_reads),
                      "dirty_events": dirty_events, "receipt": str(receipt_path)}, indent=2))


if __name__ == "__main__":
    main()
