"""Run a real archived title bitmap through the unchanged finite PC renderer.

Prediction: the pinned archive member MENUB.TGA is a 320x256 indexed image;
original A640/A710 uploads its pixel plane into the ordinary texture pages, and
original AC60/CAB0 with two controlled sprite strips samples its top 320x240
indices into the controlled CPU surface. Falsifiers include changed source or
original bytes, an unexpected route/endpoint, upload/sample disagreement, or a
visible surface mismatch. The packet chain is a test fixture, not a claimed
TITLE-produced ordering table. No Windows game process or palette conversion.
"""
from __future__ import annotations

import argparse
from collections import Counter
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
SOURCE = 0x13000000
SOURCE_MAP_SIZE = 0x16000
MAX_UPLOAD_INSNS = 4_000_000
MAX_FRAME_INSNS = 4_000_000
EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
ARCHIVE_SHA256 = "affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4"
MEMBER_SHA256 = "04e83ebe21b7fcb806ed27d5eb6de003b7188ef10b977d0c776fb9ea30129c38"
MEMBER_KEY = 0x7A7D26D6
MEMBER_ROW = 584
MEMBER_OFFSET = 29_372_902
MEMBER_LENGTH = 82_706
MEMBER_NAME = r"M:\GRAFIX\MOVIE\SCREENS\MENUB.TGA"
PIXEL_OFFSET = 0x312
WIDTH = 320
HEIGHT = 256
VISIBLE_HEIGHT = 240
VISIBLE_BYTES = WIDTH * VISIBLE_HEIGHT
UPLOAD_BYTES = WIDTH * HEIGHT
TEXTURE_SIZE = 0x100000
TEXTURE_SENTINEL = 0xCC
SURFACE_GUARD = 0xA5

# These measured function/helper spans are narrow enough to identify the exact
# original paths, and their hashes are checked against the pinned PE bytes.
SPANS = (
    (0xA340, 0xA343, "A340", "4bc724f3b1d0caf4fe369c18cba3102e6c4ea057f63fe1587e3973134a7f755e"),
    (0xA640, 0xA701, "A640", "87443dceeccacd19b42212b19c9e88ae0ba7d6cb2ec5476addedf61954c77068"),
    (0xA710, 0xA8E5, "A710", "30bd84ea8724152e3bfb0e2c521650399913bcec115f1aef489768085981d0fe"),
    (0xAC60, 0xB2F0, "AC60", "f7724c04bef582e378b183a12550ba841df1a6fa3660be51da71154aa5c525af"),
    (0xCAB0, 0xCC45, "CAB0", "5ca22ca070b69abaf98a0da5c794b0642cf816026b7ff87c794d6a82f6d9c71f"),
    (0x6FF0, 0x70B7, "LOCK", "d6f022e5f7103bec40e16000f8a4e8e08ea186c337d828f29afd46f4f8d3e88e"),
    (0x70C0, 0x7136, "REMAP", "f5225f837714b2dc9379a6e1358bef73dbfcf9bc32393c47581acf62f4ba5d40"),
    (0x7140, 0x716E, "UNLOCK", "7d70b14e30cf0c37b0215c471953a0472b799067aecbb0eb368e9cf994ff7f64"),
    (0x6620, 0x66F5, "PRESENT", "52a5fe4fd4d5d08288467e6e8d5daef54f3bb176218847788e9d0fbcea7f0287"),
    (0xBD80, 0xBD96, "BD80", "6a9578c5be0c980bc35b29642d3ac389e829a0c5666022353cb88c04f19db64f"),
)
ALLOWED_BY_PHASE = {
    "a340": ("A340",),
    "upload": ("A340", "A640", "A710", "BD80"),
    "frame": ("AC60", "CAB0", "LOCK", "REMAP", "UNLOCK", "PRESENT", "BD80"),
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def archive_member(archive_path: Path) -> tuple[bytes, dict]:
    digest = hashlib.sha256()
    with archive_path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            digest.update(chunk)
    if digest.hexdigest() != ARCHIVE_SHA256:
        raise AssertionError("HERCULES.FS does not match the pinned archive SHA-256")
    with archive_path.open("rb") as stream:
        directory = stream.read(0x3000)
        stream.seek(MEMBER_OFFSET)
        member = stream.read(MEMBER_LENGTH)
    if len(directory) != 0x3000 or len(member) != MEMBER_LENGTH:
        raise AssertionError("archive directory or MENUB extent is truncated")
    row = struct.unpack_from("<III", directory, MEMBER_ROW * 12)
    if row != (MEMBER_KEY, MEMBER_OFFSET, MEMBER_LENGTH):
        raise AssertionError(f"MENUB directory identity mismatch: {row!r}")
    if sha(member) != MEMBER_SHA256:
        raise AssertionError("MENUB.TGA content SHA-256 mismatch")
    if member[:18] != bytes.fromhex("000101000000011800000000400100010800"):
        raise AssertionError("unexpected indexed-TGA header")
    id_length, map_type, image_type = member[:3]
    map_length = struct.unpack_from("<H", member, 5)[0]
    map_depth = member[7]
    width, height = struct.unpack_from("<HH", member, 12)
    depth, descriptor = member[16:18]
    image_offset = 18 + id_length + (map_length * ((map_depth + 7) // 8))
    if (map_type, image_type, width, height, depth, descriptor, image_offset) != (
        1, 1, WIDTH, HEIGHT, 8, 0, PIXEL_OFFSET
    ):
        raise AssertionError("MENUB.TGA header does not describe the pinned 320x256 indexed plane")
    if len(member) != PIXEL_OFFSET + UPLOAD_BYTES:
        raise AssertionError("MENUB member extent is not header + 320x256 pixels")
    info = {
        "canonical_name": MEMBER_NAME,
        "directory_key": hex(MEMBER_KEY),
        "zero_based_directory_row": MEMBER_ROW,
        "archive_offset": MEMBER_OFFSET,
        "archive_member_length": len(member),
        "archive_member_sha256": sha(member),
        "tga_header_hex": member[:18].hex(),
        "id_length": id_length,
        "color_map_type": map_type,
        "image_type": image_type,
        "color_map_length": map_length,
        "color_map_entry_depth": map_depth,
        "width": width,
        "height": height,
        "pixel_depth": depth,
        "descriptor": descriptor,
        "pixel_offset": PIXEL_OFFSET,
        "pixel_plane_length": UPLOAD_BYTES,
        "pixel_plane_sha256": sha(member[PIXEL_OFFSET:]),
    }
    return member, info


def mapper_module(exe: Path):
    helper = ROOT / "renderer_contract/fixtures/composition/raster/tests/run_original_composition_raster.py"
    old_argv = sys.argv[:]
    sys.argv = [str(helper), "--repo-root", str(ROOT), "--exe", str(exe),
                "--deps", "C:/tools/hercules/renderer-oracle-python",
                "--analysis-deps", "C:/tools/hercules/python"]
    try:
        spec = importlib.util.spec_from_file_location("original_title_screen_mapper", helper)
        if not spec or not spec.loader:
            raise RuntimeError("could not load original raster mapper helper")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module, helper
    finally:
        sys.argv = old_argv


def expected_upload(source_pixels: bytes, texture_base: int) -> tuple[bytearray, bytearray]:
    expected = bytearray((TEXTURE_SENTINEL,)) * TEXTURE_SIZE
    coverage = bytearray(TEXTURE_SIZE)
    for row in range(HEIGHT):
        row_native = 0x800 + (row & 0xFF)
        for column in range(WIDTH):
            byte_x = 640 + column
            address = (texture_base + row_native * 0x100 +
                       ((byte_x & 0x700) << 8) + (byte_x & 0xFF))
            offset = address - texture_base
            expected[offset] = source_pixels[row * WIDTH + column]
            coverage[offset] = 1
    return expected, coverage


def expected_reads(texture_base: int) -> bytearray:
    expected = bytearray(TEXTURE_SIZE)
    for width, x_base, u_base in ((128, 0xA0000, 128), (192, 0xB0000, 0)):
        for row in range(VISIBLE_HEIGHT):
            start = x_base + row * 0x100 + u_base
            expected[start:start + width] = b"\x01" * width
    return expected


def pack_sprite(R, link: int, x: int, u: int, width: int, height: int) -> bytes:
    return R.packet(0x64, 0x18, link, {
        4: bytes((0x80, 0x80, 0x80)),
        8: R.s16(x), 10: R.s16(0), 12: bytes((u, 0)),
        14: R.u16(0), 16: R.u16(width), 18: R.u16(height),
    })


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True,
                        help="fresh ignored-work directory for extracted input and visible frame")
    args = parser.parse_args()

    sys.path.insert(0, str(ROOT / "renderer_contract/fixtures/eng1_real_mesh"))
    from pathguard import _under_work, input_file  # noqa: E402

    exe = input_file(args.exe, "original EXE")
    archive = input_file(args.archive, "original HERCULES.FS archive")
    receipt_path = _under_work(ROOT, args.receipt)
    output_dir = _under_work(ROOT, args.output)
    private_root = (ROOT / "work").resolve()
    if not receipt_path.is_relative_to(private_root) or not output_dir.is_relative_to(private_root):
        raise ValueError("receipt and outputs must stay under work/")
    if output_dir == private_root or receipt_path == private_root:
        raise ValueError("pass fresh child paths beneath the private scratch root")
    if output_dir.exists() or receipt_path.exists():
        raise ValueError("output directory and receipt must be fresh")
    if output_dir.is_relative_to(receipt_path) or receipt_path.is_relative_to(output_dir):
        raise ValueError("receipt path and extracted-output directory must not overlap")
    for target in (output_dir, receipt_path):
        for protected in ((ROOT / "assets").resolve(), (ROOT / "work/discs").resolve()):
            if target.is_relative_to(protected) or protected.is_relative_to(target):
                raise ValueError("output path overlaps assets or original game inputs")

    if sha(exe.read_bytes()) != EXE_SHA256:
        raise AssertionError("HERCULES.EXE does not match the pinned original image")
    member, asset_info = archive_member(archive)
    output_dir.mkdir(parents=True, exist_ok=False)
    extracted_path = output_dir / "MENUB.TGA"
    extracted_path.write_bytes(member)
    if sha(extracted_path.read_bytes()) != MEMBER_SHA256:
        raise AssertionError("private extracted MENUB.TGA changed while writing")
    source_pixels = member[PIXEL_OFFSET:]

    R, mapper_path = mapper_module(exe)
    runtime = verify_runtime(R.unicorn, Path("C:/tools/hercules/renderer-oracle-python"))
    pe = R.PE(exe)
    if pe.image_base != BASE or getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        raise AssertionError("unexpected original PE image base or relocation directory")
    if sha(pe.data) != EXE_SHA256:
        raise AssertionError("PE helper bytes differ from pinned original image")

    uc = R.Uc(R.UC_ARCH_X86, R.UC_MODE_32)
    mapped_sections = R.map_original(uc, pe)
    for address, length, permissions in (
        (R.INPUT, 0x4000, R.UC_PROT_READ | R.UC_PROT_WRITE),
        (R.STACK, R.STACK_SIZE, R.UC_PROT_READ | R.UC_PROT_WRITE),
        (R.PIXELS, R.PIXEL_SIZE, R.UC_PROT_READ | R.UC_PROT_WRITE),
        (R.DIB_INFO, R.PAGE, R.UC_PROT_READ | R.UC_PROT_WRITE),
        (R.STRETCH_STUB, R.PAGE, R.UC_PROT_READ | R.UC_PROT_WRITE | R.UC_PROT_EXEC),
        (R.SENTINEL, R.PAGE, R.UC_PROT_READ | R.UC_PROT_WRITE | R.UC_PROT_EXEC),
        (SOURCE, SOURCE_MAP_SIZE, R.UC_PROT_READ | R.UC_PROT_WRITE),
    ):
        uc.mem_map(address, length, permissions)
    uc.mem_write(SOURCE, member)
    source_address = SOURCE + PIXEL_OFFSET
    if bytes(uc.mem_read(source_address, UPLOAD_BYTES)) != source_pixels:
        raise AssertionError("emulator input pixel plane does not match archived member")

    code_before = {}
    spans_receipt = []
    for start, end, name, expected_hash in SPANS:
        raw = bytes(uc.mem_read(BASE + start, end - start))
        digest = sha(raw)
        if digest != expected_hash:
            raise AssertionError(f"pinned {name} span changed: {digest}")
        code_before[start] = raw
        spans_receipt.append({"name": name, "rva": hex(start), "size": end - start,
                              "sha256": digest})

    # Upload destination is prefilled to make writes and untouched VRAM visible.
    uc.mem_write(R.TEXTURE, bytes((TEXTURE_SENTINEL,)) * TEXTURE_SIZE)
    expected_tex, upload_coverage_expected = expected_upload(source_pixels, R.TEXTURE)
    uc.mem_write(R.PIXELS, bytes(VISIBLE_BYTES) + bytes((SURFACE_GUARD,)) * (R.PIXEL_SIZE - VISIBLE_BYTES))

    # Exact cdecl args from TITLE's +0x208 callback, with the measured word rect.
    stack = R.STACK + R.STACK_SIZE - 0x2000
    upload_args = (source_address, 2, 0, WIDTH, HEIGHT, 160, 256)
    uc.mem_write(stack, struct.pack("<8I", R.SENTINEL, *upload_args))

    # Controlled finite sprite chain: selectors 20/22 address the two physical
    # VRAM banks containing x=640..959 at y=256. E1 high byte doubles as tag.
    addresses = {
        "e1_page20": R.INPUT + 0x1000,
        "sprite_left": R.INPUT + 0x1040,
        "e1_page22": R.INPUT + 0x1080,
        "sprite_right": R.INPUT + 0x10C0,
        "terminator": R.INPUT + 0x1100,
    }
    packets = {
        "e1_page20": R.packet(0xE1, 8, addresses["sprite_left"], {
            4: struct.pack("<I", 0xE1000014),
        }),
        "sprite_left": pack_sprite(R, addresses["e1_page22"], 0, 128, 128, VISIBLE_HEIGHT),
        "e1_page22": R.packet(0xE1, 8, addresses["sprite_right"], {
            4: struct.pack("<I", 0xE1000016),
        }),
        "sprite_right": pack_sprite(R, addresses["terminator"], 128, 0, 192, VISIBLE_HEIGHT),
        "terminator": R.packet(0x40, 8, 0, {}),
    }
    for name, packet in packets.items():
        uc.mem_write(addresses[name], packet)
    ot = R.INPUT + 0x100
    R.put32(uc, ot, addresses["e1_page20"])

    cells = {
        0x00BA01FC: b"\0", 0x00BA020C: b"\0", 0x0096E698: b"\0",
        0x0096E3D8: struct.pack("<I", R.PIXELS),
        0x0096E3F4: struct.pack("<I", R.DIB_INFO),
        0x0096E400: struct.pack("<I", R.HDC_TOKEN),
        0x00BA0220: struct.pack("<I", WIDTH),
        0x00BA0224: struct.pack("<I", VISIBLE_HEIGHT),
        0x00BA5664: struct.pack("<I", R.STRETCH_STUB),
        0x00BA56F4: struct.pack("<I", R.FLUSH_STUB),
        0x0043A590: b"\0",
        0x00A90A04: struct.pack("<I", 0x7800),
        0x0096E3E0: bytes(4),
        0x00436A0C: b"\x80",
        0x00A8E780: struct.pack("<I", 0x31000000),
        0x00A8E788: bytes(4),
    }
    for address, value in cells.items():
        uc.mem_write(address, value)

    state = {"phase": "idle", "stop": None, "instructions": Counter(),
             "code_counts": Counter(), "routes": [], "a710_entries": [],
             "dirty_events": [], "services": [], "unexpected": []}
    input_read_bytes = 0
    input_read_events = 0
    input_read_min = None
    input_read_max = 0
    input_read_coverage = bytearray(UPLOAD_BYTES)
    input_read_pcs = Counter()
    texture_write_bytes = 0
    texture_write_events = 0
    texture_write_coverage = bytearray(TEXTURE_SIZE)
    texture_write_pcs = Counter()
    texture_read_bytes = 0
    texture_read_events = 0
    texture_read_counts = bytearray(TEXTURE_SIZE)
    texture_read_pcs = Counter()
    surface_write_bytes = 0
    surface_write_events = 0
    surface_write_coverage = bytearray(VISIBLE_BYTES)
    surface_write_pcs = Counter()
    surface_write_outside = []
    stop_detail = {}

    def fail_on_invalid(machine, access, address, size, value, _user):
        raise AssertionError(f"unmapped memory endpoint {address:#x} size={size} access={access}")

    def on_code(machine, address, _size, _user):
        phase = state["phase"]
        state["instructions"][phase] += 1
        limit = MAX_UPLOAD_INSNS if phase == "upload" else MAX_FRAME_INSNS if phase == "frame" else 1000
        if state["instructions"][phase] > limit:
            raise AssertionError(f"{phase} instruction budget exceeded")
        if address == R.SENTINEL:
            state["stop"] = "sentinel_return"
            stop_detail[phase] = {"eip": hex(address), "eax": hex(machine.reg_read(R.UC_X86_REG_EAX))}
            machine.emu_stop()
            return
        if address in (R.STRETCH_STUB, R.FLUSH_STUB):
            if phase != "frame":
                raise AssertionError(f"unexpected host service during {phase}: {address:#x}")
            esp = machine.reg_read(R.UC_X86_REG_ESP)
            return_address = R.u32(machine, esp)
            arg_count = 13 if address == R.STRETCH_STUB else 0
            args = [R.u32(machine, esp + 4 + index * 4) for index in range(arg_count)]
            name = "StretchDIBits" if address == R.STRETCH_STUB else "GdiFlush"
            state["services"].append({"name": name, "return_rva": hex(return_address - BASE),
                                      "arguments": args})
            machine.reg_write(R.UC_X86_REG_EIP, return_address)
            machine.reg_write(R.UC_X86_REG_ESP, esp + 4 + (52 if arg_count else 0))
            machine.reg_write(R.UC_X86_REG_EAX, 1)
            return
        rva = address - BASE
        owner = next((name for start, end, name, _hash in SPANS if start <= rva < end), None)
        if owner is None or owner not in ALLOWED_BY_PHASE.get(phase, ()):
            raise AssertionError(f"unexpected original PC during {phase}: {rva:#x}")
        state["code_counts"][owner] += 1
        if rva in (0xA340, 0xA640, 0xA710, 0xAC60, 0xCAB0, 0xBD80):
            state["routes"].append({"phase": phase, "rva": hex(rva)})
        if phase == "upload" and rva == 0xA710:
            esp = machine.reg_read(R.UC_X86_REG_ESP)
            rect_ptr = R.u32(machine, esp + 4)
            source_ptr = R.u32(machine, esp + 8)
            rect = struct.unpack("<4h", bytes(machine.mem_read(rect_ptr, 8)))
            state["a710_entries"].append({"source": hex(source_ptr), "rect_pointer": hex(rect_ptr),
                                           "rect_signed_words": list(rect)})

    def on_read(machine, _access, address, size, _value, _user):
        nonlocal input_read_bytes, input_read_events, input_read_min, input_read_max
        nonlocal texture_read_bytes, texture_read_events
        if state["phase"] == "upload" and source_address <= address < source_address + UPLOAD_BYTES:
            if address + size > source_address + UPLOAD_BYTES:
                raise AssertionError("original upload read crossed the archived pixel plane")
            input_read_bytes += size
            input_read_events += 1
            input_read_min = address if input_read_min is None else min(input_read_min, address)
            input_read_max = max(input_read_max, address + size)
            input_read_pcs[hex(machine.reg_read(R.UC_X86_REG_EIP) - BASE)] += 1
            for index in range(size):
                offset = address + index - source_address
                input_read_coverage[offset] = min(255, input_read_coverage[offset] + 1)
        elif state["phase"] == "upload" and SOURCE <= address < SOURCE + len(member):
            raise AssertionError(f"original upload read header/palette bytes at {address:#x}")
        if state["phase"] == "frame" and R.TEXTURE <= address < R.TEXTURE + TEXTURE_SIZE:
            if address + size > R.TEXTURE + TEXTURE_SIZE:
                raise AssertionError("sprite texture read escaped ordinary texture backing")
            texture_read_bytes += size
            texture_read_events += 1
            texture_read_pcs[hex(machine.reg_read(R.UC_X86_REG_EIP) - BASE)] += 1
            for index in range(size):
                offset = address + index - R.TEXTURE
                texture_read_counts[offset] = min(255, texture_read_counts[offset] + 1)

    def on_write(machine, _access, address, size, value, _user):
        nonlocal texture_write_bytes, texture_write_events
        nonlocal surface_write_bytes, surface_write_events
        pc = machine.reg_read(R.UC_X86_REG_EIP) - BASE
        if SOURCE <= address < SOURCE + SOURCE_MAP_SIZE:
            raise AssertionError(f"original code modified the archived input buffer at {address:#x}")
        if R.TEXTURE <= address < R.TEXTURE + TEXTURE_SIZE:
            if state["phase"] != "upload":
                raise AssertionError(f"texture mutation occurred during {state['phase']}")
            if address + size > R.TEXTURE + TEXTURE_SIZE:
                raise AssertionError("upload write escaped ordinary texture backing")
            texture_write_bytes += size
            texture_write_events += 1
            texture_write_pcs[hex(pc)] += 1
            for index in range(size):
                texture_write_coverage[address + index - R.TEXTURE] = 1
        if R.PIXELS <= address < R.PIXELS + R.PIXEL_SIZE:
            if state["phase"] != "frame":
                raise AssertionError(f"surface mutation occurred during {state['phase']}")
            if address + size > R.PIXELS + VISIBLE_BYTES:
                surface_write_outside.append({"address": hex(address), "size": size, "pc_rva": hex(pc)})
                raise AssertionError("original raster wrote outside the 320x240 visible surface")
            surface_write_bytes += size
            surface_write_events += 1
            surface_write_pcs[hex(pc)] += 1
            for index in range(size):
                surface_write_coverage[address + index - R.PIXELS] = 1
        if address == 0x0043A590:
            state["dirty_events"].append({"phase": state["phase"], "pc_rva": hex(pc),
                                          "value": value & 0xFF})

    uc.hook_add(R.UC_HOOK_CODE, on_code)
    uc.hook_add(R.UC_HOOK_MEM_READ, on_read)
    uc.hook_add(R.UC_HOOK_MEM_WRITE, on_write)
    uc.hook_add(R.UC_HOOK_MEM_INVALID, fail_on_invalid)

    # A340 is separately run and checked because the title callback chain has
    # this mode-query peer in the same unchanged image; it returns zero.
    state.update(phase="a340", stop=None)
    uc.mem_write(stack, struct.pack("<I", R.SENTINEL))
    uc.reg_write(R.UC_X86_REG_ESP, stack)
    uc.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
    uc.emu_start(BASE + 0xA340, 0, count=1000)
    if state["stop"] != "sentinel_return":
        raise AssertionError("original A340 did not return to the sentinel")
    a340_return = stop_detail["a340"]["eax"]
    if int(a340_return, 16) != 0 or state["code_counts"]["A340"] != 2:
        raise AssertionError(f"unexpected A340 return/instruction count: {a340_return}, {state['code_counts']['A340']}")

    state.update(phase="upload", stop=None)
    uc.mem_write(stack, struct.pack("<8I", R.SENTINEL, *upload_args))
    uc.reg_write(R.UC_X86_REG_ESP, stack)
    uc.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
    uc.emu_start(BASE + 0xA640, 0, count=MAX_UPLOAD_INSNS)
    if state["stop"] != "sentinel_return":
        raise AssertionError(f"original A640/A710 did not return: {state['instructions']['upload']} instructions")
    if len(state["a710_entries"]) != 1:
        raise AssertionError(f"expected one A710 entry, got {state['a710_entries']}")
    a710 = state["a710_entries"][0]
    if a710["source"] != hex(source_address) or a710["rect_signed_words"] != [320, 256, 160, 256]:
        raise AssertionError(f"A640 supplied unexpected A710 input: {a710}")
    uploaded_texture = bytes(uc.mem_read(R.TEXTURE, TEXTURE_SIZE))
    if uploaded_texture != bytes(expected_tex):
        mismatch = next(i for i, (actual, expected) in enumerate(zip(uploaded_texture, expected_tex))
                        if actual != expected)
        raise AssertionError(f"A640/A710 texture mismatch at backing offset {mismatch:#x}")
    if bytes(texture_write_coverage) != bytes(upload_coverage_expected):
        raise AssertionError("observed upload write coverage differs from mapped MENUB pixel plane")
    if (input_read_bytes != UPLOAD_BYTES or texture_write_bytes != UPLOAD_BYTES or
            bytes(input_read_coverage) != b"\x01" * UPLOAD_BYTES):
        raise AssertionError(f"upload extent mismatch: input reads={input_read_bytes}, texture writes={texture_write_bytes}")

    # AC60 consumes the genuine uploaded bytes in this same Unicorn image.
    initial_surface = bytes(VISIBLE_BYTES) + bytes((SURFACE_GUARD,)) * (R.PIXEL_SIZE - VISIBLE_BYTES)
    state.update(phase="frame", stop=None)
    uc.mem_write(stack, struct.pack("<II", R.SENTINEL, ot))
    uc.reg_write(R.UC_X86_REG_ESP, stack)
    uc.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
    uc.emu_start(BASE + 0xAC60, 0, count=MAX_FRAME_INSNS)
    if state["stop"] != "sentinel_return":
        raise AssertionError(f"original AC60 frame did not return: {state['instructions']['frame']} instructions")

    visible = bytes(uc.mem_read(R.PIXELS, VISIBLE_BYTES))
    mapped_surface = bytes(uc.mem_read(R.PIXELS, R.PIXEL_SIZE))
    expected_visible = source_pixels[:VISIBLE_BYTES]
    if visible != expected_visible:
        mismatch = next(i for i, (actual, expected) in enumerate(zip(visible, expected_visible))
                        if actual != expected)
        raise AssertionError(f"original 320x240 indices differ from archived source at {mismatch:#x}: "
                             f"{visible[mismatch]:02x}!={expected_visible[mismatch]:02x}")
    if mapped_surface[VISIBLE_BYTES:] != initial_surface[VISIBLE_BYTES:]:
        raise AssertionError("original renderer changed the mapped surface guard tail")
    expected_read_coverage = expected_reads(R.TEXTURE)
    if bytes(texture_read_counts) != bytes(expected_read_coverage):
        raise AssertionError("CAB0 texture-read coverage differs from the two controlled source strips")
    expected_nonzero = sum(value != 0 for value in expected_visible)
    if surface_write_bytes != expected_nonzero or sum(surface_write_coverage) != expected_nonzero:
        raise AssertionError(f"surface writes do not match transparent-index-zero rule: "
                             f"{surface_write_bytes} bytes, expected {expected_nonzero}")
    if [event["name"] for event in state["services"]] != ["StretchDIBits", "GdiFlush"]:
        raise AssertionError(f"unexpected surface-tail services: {state['services']}")
    dirty_set = [event for event in state["dirty_events"] if event["value"] == 1]
    dirty_clear = [event for event in state["dirty_events"] if event["value"] == 0]
    if not dirty_set or not dirty_clear:
        raise AssertionError(f"missing dirty request or present clear event: {state['dirty_events']}")
    if any(event["pc_rva"] != "0xcc37" for event in dirty_set):
        raise AssertionError(f"unexpected original dirty-request PC: {dirty_set}")
    if any(event["pc_rva"] != "0x6637" for event in dirty_clear):
        raise AssertionError(f"unexpected original dirty-clear PC: {dirty_clear}")
    if any(count != 1 for count in texture_read_counts if count > 1):
        raise AssertionError("some screen texels were sampled more than once")
    code_after = {start: bytes(uc.mem_read(BASE + start, len(raw)))
                  for start, raw in code_before.items()}
    if code_after != code_before:
        raise AssertionError("original PE code bytes changed during finite execution")

    frame_path = output_dir / "MENUB_visible_320x240.indexed.raw"
    frame_path.write_bytes(visible)
    if sha(frame_path.read_bytes()) != sha(visible):
        raise AssertionError("visible frame output changed while writing")
    state_counts = {name: state["code_counts"][name] for name in sorted(state["code_counts"])}
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_ORIGINAL_TITLE_SCREEN_INDEX_PASS",
        "prediction_before_run": {
            "prediction": "Unchanged A640/A710 uploads the archived MENUB indexed pixel plane; controlled AC60/CAB0 sprites sampling page selectors 20 and 22 reproduce the top 320x240 source indices in the original CPU surface.",
            "falsifiers": [
                "pinned archive/member or original image identity mismatch",
                "A640 does not reach A710 with the measured {320,256,160,256} rectangle",
                "upload write coverage or sprite sample coverage differs from the independently mapped source pixels",
                "visible 320x240 output differs from archived source indices",
                "unknown original PC, memory endpoint, dirty request/clear site, or changed original code",
            ],
            "recorded_before_native_execution": True,
        },
        "scope": (
            "Actual archived MENUB.TGA header and indexed pixels uploaded through original A640/A710, "
            "then consumed by unchanged original AC60/CAB0 in the same Unicorn image into a controlled "
            "320x240 indexed CPU surface. The E1/sprite packet chain is an explicit test fixture, not a "
            "claim about TITLE's produced ordering table. Palette conversion, game-process execution, "
            "whole renderer, and gameplay are excluded."
        ),
        "original_process_executed": False,
        "runner": {
            "path": str(Path(__file__).relative_to(ROOT)).replace("\\", "/"),
            "sha256": sha(Path(__file__).read_bytes()),
        },
        "oracle": {
            "path": str(exe), "sha256": EXE_SHA256, "image_base": hex(BASE),
            "base_relocations": 0,
            "helper_mapper": str(mapper_path.relative_to(ROOT)).replace("\\", "/"),
            "helper_mapper_sha256": sha(mapper_path.read_bytes()),
            "spans": spans_receipt,
            "code_bytes_unchanged_after_execution": True,
        },
        "archive": {"path": str(archive), "sha256": ARCHIVE_SHA256, **asset_info,
                    "private_extracted_copy": str(extracted_path.relative_to(ROOT)).replace("\\", "/")},
        "execution": {
            **runtime,
            "architecture": "x86-32",
            "original_sections_mapped_at_preferred_base": True,
            "mapped_original_sections": [
                {"name": item["name"],
                 "va": item["va"] if isinstance(item["va"], str) else hex(item["va"]),
                 "size": item["size"] if isinstance(item["size"], str) else hex(item["size"])}
                for item in mapped_sections
            ],
            "unknown_pc_or_memory_endpoint_fails": True,
            "instruction_budgets": {"A640 upload chain": MAX_UPLOAD_INSNS,
                                    "AC60 frame chain": MAX_FRAME_INSNS},
            "instruction_counts": dict(state["instructions"]),
            "executed_original_span_instruction_counts": state_counts,
            "a340": {"rva": "0xa340", "return_eax": a340_return,
                     "original_instruction_count": state["code_counts"]["A340"],
                     "instruction_count_including_sentinel_hook": state["instructions"]["a340"]},
            "upload_call": {
                "entry_rva": "0xa640",
                "cdecl_arguments": [hex(value) for value in upload_args],
                "a710_entry_count": len(state["a710_entries"]),
                "a710_entry": state["a710_entries"][0],
                "a710_destination_rect_signed_words": [320, 256, 160, 256],
                "a710_instructions_included_in_upload_count": True,
                "sentinel_return": stop_detail["upload"],
            },
            "packet_fixture": {
                "ordering_table_address": hex(ot),
                "chain_order": ["e1_page20", "sprite_left", "e1_page22", "sprite_right", "terminator"],
                "packet_addresses": {name: hex(value) for name, value in addresses.items()},
                "packet_sizes": {name: len(value) for name, value in packets.items()},
                "packet_hex": {name: value.hex() for name, value in packets.items()},
                "e1_raw_words": ["0xe1000014", "0xe1000016"],
                "sprites": [
                    {"tag": "0x64", "x": 0, "y": 0, "u": 128, "v": 0,
                     "width": 128, "height": VISIBLE_HEIGHT, "page_selector": 20},
                    {"tag": "0x64", "x": 128, "y": 0, "u": 0, "v": 0,
                     "width": 192, "height": VISIBLE_HEIGHT, "page_selector": 22},
                ],
                "initial_visible_surface_index": 0,
                "remap_control_byte": "0x80",
            },
            "texture_upload": {
                "source_address": hex(source_address),
                "source_extent_bytes": UPLOAD_BYTES,
                "source_read_bytes": input_read_bytes,
                "source_read_events": input_read_events,
                "source_read_first_address": hex(input_read_min),
                "source_read_last_exclusive": hex(input_read_max),
                "source_read_pc_counts": dict(input_read_pcs),
                "source_read_coverage_matches_archived_plane": True,
                "destination_base": hex(R.TEXTURE),
                "destination_write_bytes": texture_write_bytes,
                "destination_write_events": texture_write_events,
                "destination_unique_byte_count": sum(texture_write_coverage),
                "destination_write_pc_counts": dict(texture_write_pcs),
                "destination_min_address": hex(R.TEXTURE + next(i for i, value in enumerate(texture_write_coverage) if value)),
                "destination_max_exclusive": hex(R.TEXTURE + len(texture_write_coverage) -
                    next(i for i, value in enumerate(reversed(texture_write_coverage)) if value)),
                "uploaded_pixel_plane_sha256": sha(source_pixels),
                "uploaded_physical_texture_sha256": sha(uploaded_texture),
                "independent_address_mapping": "base + (0x800+(y&255))*0x100 + ((byte_x&0x700)<<8) + (byte_x&0xff), byte_x=640+column",
                "upload_write_coverage_matches_expected": True,
            },
            "frame_consumption": {
                "original_routes": state["routes"],
                "texture_read_bytes": texture_read_bytes,
                "texture_read_events": texture_read_events,
                "texture_unique_byte_count": sum(value != 0 for value in texture_read_counts),
                "texture_read_pc_counts": dict(texture_read_pcs),
                "texture_read_coverage_matches_two_strips": True,
                "surface_visible_extent_bytes": VISIBLE_BYTES,
                "surface_write_bytes": surface_write_bytes,
                "surface_write_events": surface_write_events,
                "surface_unique_byte_count": sum(surface_write_coverage),
                "surface_write_pc_counts": dict(surface_write_pcs),
                "surface_nonzero_source_indices": expected_nonzero,
                "surface_zero_source_indices": VISIBLE_BYTES - expected_nonzero,
                "initial_surface_index": 0,
                "visible_frame_sha256": sha(visible),
                "visible_frame_source_sha256": sha(expected_visible),
                "visible_frame_byte_identical_to_source_top_240_rows": True,
                "visible_frame_output": str(frame_path.relative_to(ROOT)).replace("\\", "/"),
                "mapped_storage_extent_bytes": R.PIXEL_SIZE,
                "mapped_storage_sha256_including_guard_tail": sha(mapped_surface),
                "mapped_storage_guard_tail_unchanged": True,
                "dirty_request_and_clear_events": state["dirty_events"],
                "present_services": state["services"],
                "sentinel_return": stop_detail["frame"],
            },
        },
    }
    receipt_path.parent.mkdir(parents=True, exist_ok=True)
    receipt_path.write_bytes((json.dumps(receipt, indent=2) + "\n").encode("utf-8"))
    print(json.dumps({
        "status": receipt["status"], "receipt": str(receipt_path),
        "frame_sha256": receipt["execution"]["frame_consumption"]["visible_frame_sha256"],
        "upload_instructions": state["instructions"]["upload"],
        "frame_instructions": state["instructions"]["frame"],
        "surface_writes": surface_write_bytes,
        "dirty_events": state["dirty_events"],
    }, indent=2))


if __name__ == "__main__":
    main()
