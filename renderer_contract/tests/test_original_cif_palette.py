"""Run the original TITLE selector and CIF palette/table reader in Unicorn.

This fixture runs the unchanged host slot 145 selector (5760), loader (68C0),
archive reader (5210), CRT buffering code, and palette writer (6780) against the
pinned MAINMENU.CIF. Only the external Windows file and single-thread lock APIs
are modeled at imported call boundaries. It stops before 69A0's presentation
body; no game process, code patch, or machine-code stub is used.

The prior 71A0 state is seeded from the independently verified formulas and
hashes in native_palette_builder.json. This test does not execute 71A0.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TESTS = ROOT / "renderer_contract" / "tests"
FIXTURES = ROOT / "renderer_contract" / "fixtures"
sys.path.insert(0, str(TESTS))
sys.path.insert(0, str(FIXTURES / "eng1_real_mesh"))
from oracle_common import (  # noqa: E402
    DEFAULT_ANALYSIS_DEPS,
    DEFAULT_DEPS,
    EXPECTED_EXE_SHA256,
    sha256,
    verify_runtime,
)
from pathguard import _under_work, input_file  # noqa: E402

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--repo-root", type=Path, default=ROOT)
parser.add_argument("--exe", type=Path, required=True,
                    help="pinned original HERCULES.EXE")
parser.add_argument("--archive", type=Path, required=True,
                    help="pinned original HERCULES.FS")
parser.add_argument("--receipt", type=Path, required=True,
                    help="fresh JSON receipt below this checkout's work/")
parser.add_argument("--output", type=Path, required=True,
                    help="fresh output directory below this checkout's work/")
parser.add_argument("--deps", type=Path, default=DEFAULT_DEPS,
                    help="pinned Unicorn dependency directory")
parser.add_argument("--analysis-deps", type=Path, default=DEFAULT_ANALYSIS_DEPS,
                    help="existing pefile dependency directory")
ARGS = parser.parse_args()

REPO = ARGS.repo_root.resolve()
if REPO != ROOT.resolve():
    parser.error("--repo-root must be this checkout containing the test and work/ directory")
EXE_PATH = input_file(ARGS.exe, "original EXE")
ARCHIVE_PATH = input_file(ARGS.archive, "original archive")
RECEIPT_PATH = _under_work(REPO, ARGS.receipt)
OUTPUT_DIR = _under_work(REPO, ARGS.output)
if RECEIPT_PATH == OUTPUT_DIR or RECEIPT_PATH.is_relative_to(OUTPUT_DIR) or OUTPUT_DIR.is_relative_to(RECEIPT_PATH):
    parser.error("--receipt and --output must be separate paths")
if RECEIPT_PATH.exists() or OUTPUT_DIR.exists():
    parser.error("--receipt and --output must be fresh paths")
for target in (RECEIPT_PATH, OUTPUT_DIR):
    for protected in ((REPO / "assets").resolve(), (REPO / "work" / "discs").resolve()):
        if target.is_relative_to(protected) or protected.is_relative_to(target):
            parser.error("--receipt or --output overlaps protected game inputs")

ARGS.deps = ARGS.deps.resolve()
ARGS.analysis_deps = ARGS.analysis_deps.resolve()
if not ARGS.deps.is_dir() or not ARGS.analysis_deps.is_dir():
    parser.error("pinned Unicorn and analysis dependency directories must exist")
os.environ["HERCULES_PYTHON_DEPS"] = str(ARGS.analysis_deps)
sys.path.insert(0, str(ARGS.deps))
sys.path.insert(0, str(REPO / "scripts"))
from pe import PE  # noqa: E402
import unicorn  # noqa: E402
from unicorn import (  # noqa: E402
    Uc,
    UcError,
    UC_ARCH_X86,
    UC_HOOK_CODE,
    UC_HOOK_MEM_INVALID,
    UC_MODE_32,
    UC_PROT_EXEC,
    UC_PROT_READ,
    UC_PROT_WRITE,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EAX,
    UC_X86_REG_EBP,
    UC_X86_REG_EBX,
    UC_X86_REG_ECX,
    UC_X86_REG_EDI,
    UC_X86_REG_EFLAGS,
    UC_X86_REG_EIP,
    UC_X86_REG_ESI,
    UC_X86_REG_ESP,
)

BASE = 0x00400000
STACK = 0x11000000
STOP = 0x12000000
IOBUF = 0x13000000
PIO_PAGE = 0x14000000
LOCK_PAGE = 0x15000000
API_PAGE = 0x18000000
PALETTE_DIB = IOBUF + 0x10000
PALETTE_LOG = IOBUF + 0x11000
FILE_HANDLE = 7
MAX_INSNS = 20_000_000
PAGE = 0x1000

ARCHIVE_SHA256 = "affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4"
MEMBER_SHA256 = "11d04b7e7a51b8b6a6a99fe0b5fa7e436a11ca04fedf3c0a25033d5deeb2049b"
MEMBER_KEY = 0xE10004A6
MEMBER_ROW = 762
MEMBER_OFFSET = 38_040_626
MEMBER_LENGTH = 336_384
MEMBER_PATH = r"M:\GRAFIX\MAPS\TITLE\MAINMENU\MAP_PSX.CIF"
SELECTOR_PATH = r"o:\title\mainmenu\map_psx.cif"
SELECTOR_MODE = "rb"

SHADE = 0x0091C488
CLUT = 0x0092C490
CLUT_SIZE = 0x2000
BLEND = 0x0092E3D0
BLEND_SIZE = 0x40000
MODE_FLAG = 0x0092C488
GATE = 0x0096E3E0
LOGPAL_PTR = 0x0096E3F8
DIB_PTR = 0x0096E3F4
FILEPOS_TABLE = 0x0091A7D0
ARCHIVE_STREAM_PTR = 0x0091BBD0
ARCHIVE_ROW_COUNT = 0x0091BBD4
ARCHIVE_DIRECTORY = 0x009177D0
ARCHIVE_HANDLE_SENTINEL = 0x004365A4
CRT_IOB = 0x0043CB68
CRT_LOCKS = 0x0043A6A0
CRT_FD_TABLE = 0x00BA3EE0
CRT_FD_LIMIT = 0x00BA3FE0

SPAN_HASHES = {
    0x5760: (0x2A1, "e0f6e6f82df474c57a005e1f0575759f8a31421c548327254ceb5c293a77babc"),
    0x68C0: (0x0DB, "5c4d1c41f2a1fa2bc4eac7c5fe7a7f2b654a15acf9b59bb42139d9f8006b58a4"),
    0x5210: (0x0AC, "88c2e2a361d62fea30b2f9fa2833a7b46a863c4dc969b64f7fcc05d323d7c1dc"),
    0x6780: (0x087, "8e8230bf3f82228e5eceee44ac9fc1b5adb669fb4c1890c1050ffde5ce6a8a23"),
}
STARTUP_SHA256 = {
    "shade": "674caa96b371abd08aec5b3846dbbd9b73a5fcdb1c29ca3cadcff690c6858cef",
    "blend": "9c1ad11f46c29575a47bbcbc6f93ec4fc8566ae0e95b9b9231f906443cc3a507",
    "clut": "dc404a613fedaeb54034514bc6505f56b933caa5250299ba7d094377a51caa46",
}


def align_page(value: int) -> int:
    return (value + PAGE - 1) & ~(PAGE - 1)


def map_original(uc: Uc, pe: PE) -> list[dict[str, object]]:
    header_size = align_page(pe.pe.OPTIONAL_HEADER.SizeOfHeaders)
    uc.mem_map(BASE, header_size, UC_PROT_READ)
    uc.mem_write(BASE, pe.data[:pe.pe.OPTIONAL_HEADER.SizeOfHeaders])
    mapped = []
    for section in pe.sections:
        size = align_page(max(section["virtual_size"], section["raw_size"]))
        permissions = UC_PROT_READ
        if section["characteristics"] & 0x80000000:
            permissions |= UC_PROT_WRITE
        if section["characteristics"] & 0x20000000:
            permissions |= UC_PROT_EXEC
        address = BASE + section["virtual_address"]
        uc.mem_map(address, size, permissions)
        if section["raw_size"]:
            uc.mem_write(address, pe.read_rva(section["virtual_address"], section["raw_size"]))
        if size > section["raw_size"]:
            uc.mem_write(address + section["raw_size"], b"\0" * (size - section["raw_size"]))
        mapped.append({"name": section["name"], "address": hex(address), "size": hex(size)})
    return mapped


def startup_formula_state() -> tuple[bytes, bytes, bytes]:
    shade = bytes(min(255, (row * column) >> 7)
                  for row in range(256) for column in range(256))
    blend = bytearray()
    for selector in range(4):
        for row in range(256):
            for column in range(256):
                if selector == 0:
                    value = (row + column) >> 1
                elif selector == 1:
                    value = min(row + column, 255)
                elif selector == 2:
                    value = max(column - row, 0)
                else:
                    value = min((row >> 1) + column, 255)
                blend.append(value)
    clut = bytes(range(256)) * 32
    return shade, bytes(blend), clut


def archive_member(path: Path) -> tuple[bytes, bytes, dict[str, object]]:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            digest.update(chunk)
    if digest.hexdigest() != ARCHIVE_SHA256:
        raise AssertionError(f"HERCULES.FS SHA mismatch: {digest.hexdigest()}")
    with path.open("rb") as stream:
        directory = stream.read(0x3000)
        if len(directory) != 0x3000:
            raise AssertionError("truncated HFS directory")
        rows = [struct.unpack_from("<III", directory, index * 12) for index in range(1024)]
        active = next((index for index, row in enumerate(rows) if row[0] == 0), None)
        if active != 821 or any(rows[index][0] >= rows[index + 1][0] for index in range(active - 1)):
            raise AssertionError(f"unexpected HFS sorted directory: active_rows={active}")
        row = rows[MEMBER_ROW]
        if row != (MEMBER_KEY, MEMBER_OFFSET, MEMBER_LENGTH):
            raise AssertionError(f"MAINMENU.CIF archive row changed: {row!r}")
        stream.seek(MEMBER_OFFSET)
        member = stream.read(MEMBER_LENGTH)
    if len(member) != MEMBER_LENGTH or sha256(member) != MEMBER_SHA256:
        raise AssertionError("MAINMENU.CIF extent or SHA-256 mismatch")
    metadata = {
        "archive_sha256": digest.hexdigest(),
        "active_sorted_rows": active,
        "canonical_member_path": MEMBER_PATH,
        "zero_based_row": MEMBER_ROW,
        "directory_key": hex(MEMBER_KEY),
        "offset": MEMBER_OFFSET,
        "length": MEMBER_LENGTH,
        "sha256": sha256(member),
        "section_sha256": {
            "rgb_triplets": sha256(member[0:0x300]),
            "blend": sha256(member[0x300:0x40300]),
            "shade": sha256(member[0x40300:0x50300]),
            "clut": sha256(member[0x50300:0x52200]),
        },
    }
    return directory, member, metadata


def main() -> dict[str, object]:
    exe_before = sha256(EXE_PATH.read_bytes())
    if exe_before != EXPECTED_EXE_SHA256:
        raise AssertionError(f"HERCULES.EXE SHA mismatch: {exe_before}")
    directory, member, member_info = archive_member(ARCHIVE_PATH)
    runtime = verify_runtime(unicorn, ARGS.deps)
    pe = PE(EXE_PATH)
    if pe.image_base != BASE or getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        raise AssertionError("unexpected original PE image base or relocation directory")
    text_section = next(section for section in pe.sections if section["name"] == ".text")
    text_rva = text_section["virtual_address"]
    text_size = text_section["raw_size"]
    text_source = pe.read_rva(text_rva, text_size)
    text_source_sha = sha256(text_source)
    spans = []
    for rva, (length, expected) in SPAN_HASHES.items():
        actual = sha256(pe.read_rva(rva, length))
        if actual != expected:
            raise AssertionError(f"original span {rva:#x} changed: {actual}")
        spans.append({"rva": hex(rva), "length": length, "sha256": actual})

    shade_seed, blend_seed, clut_seed = startup_formula_state()
    seed_hashes = {
        "shade": sha256(shade_seed),
        "clut": sha256(clut_seed),
    }
    # 71A0 generates blend formulas, then writes all 32 CLUT ramps. The final
    # 0xC0 bytes at blend start alias CLUT row 32 columns 64..255.
    blend_after_71a0 = bytearray(blend_seed)
    blend_after_71a0[:0xC0] = clut_seed[0x1F40:0x2000]
    seed_hashes["blend"] = sha256(blend_after_71a0)
    if seed_hashes != STARTUP_SHA256:
        raise AssertionError(f"formula-seeded 71A0 state no longer matches accepted hashes: {seed_hashes}")

    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    mapped = map_original(uc, pe)
    for address, length, permissions in (
        (STACK, 0x100000, UC_PROT_READ | UC_PROT_WRITE),
        (STOP, PAGE, UC_PROT_READ | UC_PROT_WRITE),
        (IOBUF, 0x20000, UC_PROT_READ | UC_PROT_WRITE),
        (PIO_PAGE, PAGE, UC_PROT_READ | UC_PROT_WRITE),
        (LOCK_PAGE, PAGE, UC_PROT_READ | UC_PROT_WRITE),
        (API_PAGE, PAGE, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC),
    ):
        uc.mem_map(address, length, permissions)

    # Seed the prior 71A0 outputs from its already verified independent
    # formulas. The CLUT write order intentionally creates the 0xC0-byte alias.
    uc.mem_write(SHADE, shade_seed)
    uc.mem_write(BLEND, blend_seed)
    uc.mem_write(CLUT, clut_seed)
    uc.mem_write(MODE_FLAG, b"\0" * 8)
    uc.mem_write(GATE, struct.pack("<I", 0))

    # 6780 updates only entries; headers and tail bytes are canaries here.
    uc.mem_write(PALETTE_DIB, b"\xA5" * 0x1000)
    uc.mem_write(PALETTE_LOG, b"\xB6" * 0x1000)
    uc.mem_write(DIB_PTR, struct.pack("<I", PALETTE_DIB))
    uc.mem_write(LOGPAL_PTR, struct.pack("<I", PALETTE_LOG))

    # Seed the actual sorted HFS directory and a minimal measured VC5 FILE/fd
    # envelope. Original 5130/5210/CRT code performs all path hashing, lookup,
    # cursor updates, buffering, palette calls, and table reads.
    uc.mem_write(ARCHIVE_DIRECTORY, directory)
    uc.mem_write(ARCHIVE_ROW_COUNT, struct.pack("<I", 821))
    uc.mem_write(CRT_FD_TABLE, struct.pack("<I", PIO_PAGE))
    uc.mem_write(CRT_FD_LIMIT, struct.pack("<I", 32))
    uc.mem_write(ARCHIVE_STREAM_PTR, struct.pack("<I", CRT_IOB))
    uc.mem_write(ARCHIVE_HANDLE_SENTINEL, struct.pack("<I", 0xFFFFFFFF))
    uc.mem_write(CRT_IOB, struct.pack("<8I", IOBUF, 0, IOBUF, 0x9, FILE_HANDLE, 0, 0x200, 0))
    uc.mem_write(PIO_PAGE + FILE_HANDLE * 36, struct.pack("<9I", 0x44, 1, 0, 0, 0, 0, 0, 0, 0))
    uc.mem_write(CRT_LOCKS + 28 * 4, struct.pack("<I", LOCK_PAGE))
    uc.mem_write(0x0091A7D0, b"\xFF" * 0x1000)

    imports = {}
    for dll in pe.pe.DIRECTORY_ENTRY_IMPORT:
        for imported in dll.imports:
            if imported.name:
                imports[imported.name.decode("ascii", "replace")] = imported.address
    api = {
        "SetFilePointer": API_PAGE + 0x100,
        "ReadFile": API_PAGE + 0x200,
        "InitializeCriticalSection": API_PAGE + 0x280,
        "DeleteCriticalSection": API_PAGE + 0x2C0,
        "EnterCriticalSection": API_PAGE + 0x300,
        "LeaveCriticalSection": API_PAGE + 0x400,
    }
    for name, address in api.items():
        if name not in imports:
            raise AssertionError(f"pinned import table lacks {name}")
        uc.mem_write(imports[name], struct.pack("<I", address))

    trace: dict[str, object] = {
        "instruction_count": 0,
        "original_instruction_count": 0,
        "stopped_before_69a0_body": False,
        "failure": None,
        "open": None,
        "archive_handle": None,
        "reader_call_count": 0,
        "reader_sequence_ok": True,
        "reader_first": None,
        "reader_last": None,
        "palette_call_count": 0,
        "palette_call_sha256": None,
        "palette_call_first": None,
        "palette_call_last": None,
        "gate_mismatch": None,
        "api_counts": {name: 0 for name in api},
        "set_file_pointer": [],
        "read_file_events": [],
        "read_bytes_total": 0,
        "file_cursor": 0,
    }
    executed_calls: set[int] = set()
    instruction_stream = hashlib.sha256()
    palette_call_stream = bytearray()
    read_coverage = bytearray(MEMBER_LENGTH)

    def fail(message: str) -> None:
        if trace["failure"] is None:
            trace["failure"] = message
        uc.emu_stop()

    def ret_stdcall(emu: Uc, stack_bytes: int, eax: int | None = None) -> None:
        esp = emu.reg_read(UC_X86_REG_ESP)
        return_address = struct.unpack("<I", emu.mem_read(esp, 4))[0]
        if eax is not None:
            emu.reg_write(UC_X86_REG_EAX, eax & 0xFFFFFFFF)
        emu.reg_write(UC_X86_REG_ESP, esp + 4 + stack_bytes)
        emu.reg_write(UC_X86_REG_EIP, return_address)

    def on_code(emu: Uc, address: int, size: int, _user) -> None:
        trace["instruction_count"] = int(trace["instruction_count"]) + 1
        if address in api.values():
            name = next(key for key, value in api.items() if value == address)
            trace["api_counts"][name] = int(trace["api_counts"][name]) + 1
            if name == "SetFilePointer":
                esp = emu.reg_read(UC_X86_REG_ESP)
                handle, low, high_ptr, origin = struct.unpack("<4I", emu.mem_read(esp + 4, 16))
                if high_ptr != 0:
                    fail(f"unsupported SetFilePointer high-part pointer {high_ptr:#x}")
                    return
                offset = low if low < 0x80000000 else low - 0x100000000
                if origin == 0:
                    new_position = offset
                elif origin == 1:
                    new_position = int(trace["file_cursor"]) + offset
                elif origin == 2:
                    new_position = len(archive_bytes) + offset
                else:
                    fail(f"unsupported SetFilePointer origin {origin}")
                    return
                if handle != 0x44 or new_position < 0 or new_position > len(archive_bytes):
                    fail(f"invalid SetFilePointer request handle={handle:#x} position={new_position:#x}")
                    return
                trace["file_cursor"] = new_position
                trace["set_file_pointer"].append({
                    "handle": hex(handle), "offset": hex(offset), "origin": origin,
                    "result": hex(new_position),
                })
                ret_stdcall(emu, 16, new_position)
                return
            if name == "ReadFile":
                esp = emu.reg_read(UC_X86_REG_ESP)
                handle, destination, requested, count_pointer, overlapped = struct.unpack(
                    "<5I", emu.mem_read(esp + 4, 20))
                start = int(trace["file_cursor"])
                end = start + requested
                if handle != 0x44 or overlapped != 0 or end > len(archive_bytes):
                    fail(f"invalid ReadFile request handle={handle:#x} start={start:#x} size={requested:#x}")
                    return
                payload = archive_bytes[start:end]
                if len(payload) != requested:
                    fail("short read from pinned HERCULES.FS")
                    return
                member_start = start - MEMBER_OFFSET
                member_end = end - MEMBER_OFFSET
                if member_start < 0 or member_end > MEMBER_LENGTH or any(read_coverage[member_start:member_end]):
                    fail(f"physical read is outside or overlaps MAINMENU.CIF at {start:#x}")
                    return
                read_coverage[member_start:member_end] = b"\x01" * requested
                emu.mem_write(destination, payload)
                if count_pointer:
                    emu.mem_write(count_pointer, struct.pack("<I", requested))
                trace["read_file_events"].append({
                    "offset": hex(start), "length": requested,
                    "destination": hex(destination), "sha256": sha256(payload),
                })
                trace["file_cursor"] = end
                trace["read_bytes_total"] = int(trace["read_bytes_total"]) + requested
                ret_stdcall(emu, 20, 1)
                return
            if name in ("InitializeCriticalSection", "DeleteCriticalSection",
                        "EnterCriticalSection", "LeaveCriticalSection"):
                ret_stdcall(emu, 4, 0)
                return
            fail(f"unhandled imported API {name}")
            return

        if address == BASE + 0x69A0:
            trace["stopped_before_69a0_body"] = True
            emu.emu_stop()
            return

        rva = address - BASE
        if not (BASE + text_rva <= address < BASE + text_rva + text_size):
            fail(f"original control left .text at {address:#x}")
            return
        trace["original_instruction_count"] = int(trace["original_instruction_count"]) + 1
        instruction_stream.update(struct.pack("<II", rva, size))
        instruction_stream.update(pe.read_rva(rva, size))
        if rva in (0x5130, 0x5760, 0x68C0, 0x5210, 0x6780):
            executed_calls.add(rva)
        if rva == 0x5130:
            esp = emu.reg_read(UC_X86_REG_ESP)
            path_pointer, mode_pointer = struct.unpack("<2I", emu.mem_read(esp + 4, 8))
            path_bytes = bytearray()
            for index in range(256):
                value = emu.mem_read(path_pointer + index, 1)[0]
                if value == 0:
                    break
                path_bytes.append(value)
            mode_bytes = bytearray()
            for index in range(16):
                value = emu.mem_read(mode_pointer + index, 1)[0]
                if value == 0:
                    break
                mode_bytes.append(value)
            trace["open"] = {
                "path": path_bytes.decode("ascii", "replace"),
                "mode": mode_bytes.decode("ascii", "replace"),
            }
        elif rva == 0x68D6:
            trace["archive_handle"] = emu.reg_read(UC_X86_REG_EAX)
        elif rva == 0x5210:
            esp = emu.reg_read(UC_X86_REG_ESP)
            destination, item_size, item_count, handle = struct.unpack(
                "<4I", emu.mem_read(esp + 4, 16))
            position = 0xFFFFFFFF
            if handle > 0:
                position = struct.unpack("<I", emu.mem_read(FILEPOS_TABLE + (handle - 1) * 4, 4))[0]
            call_index = int(trace["reader_call_count"])
            expected = None
            if call_index < 256:
                expected = (call_index * 3, 3, 1, 0x1107FAF0)
            elif call_index == 256:
                expected = (0x300, 0x40000, 1, BLEND)
            elif call_index == 257:
                expected = (0x40300, 0x10000, 1, SHADE)
            elif call_index == 258:
                expected = (0x50300, 0x1F00, 1, CLUT)
            observed = (position, item_size, item_count, destination)
            if handle != MEMBER_ROW + 1 or observed != expected:
                trace["reader_sequence_ok"] = False
                fail(f"original reader call {call_index} differs: {observed!r} != {expected!r}, handle={handle}")
                return
            item = {
                "index": call_index,
                "handle": handle,
                "logical_position_before": hex(position),
                "destination": hex(destination),
                "item_size": item_size,
                "item_count": item_count,
            }
            if trace["reader_first"] is None:
                trace["reader_first"] = item
            trace["reader_last"] = item
            trace["reader_call_count"] = call_index + 1
        elif rva == 0x6780:
            esp = emu.reg_read(UC_X86_REG_ESP)
            index, red, green, blue = struct.unpack("<4I", emu.mem_read(esp + 4, 16))
            call_index = int(trace["palette_call_count"])
            source = member[index * 3:index * 3 + 3]
            if index == 0:
                expected_rgb = (0, 0, 0)
            elif index == 255:
                expected_rgb = (255, 255, 255)
            else:
                expected_rgb = tuple(source)
            actual_rgb = (red & 0xFF, green & 0xFF, blue & 0xFF)
            if index != call_index or actual_rgb != expected_rgb:
                fail(f"6780 call {call_index} differs: index={index}, rgb={actual_rgb}, expected={expected_rgb}")
                return
            palette_call_stream.extend((index & 0xFF, *actual_rgb))
            trace["palette_call_count"] = call_index + 1
            call_item = {"index": index, "rgb": list(actual_rgb)}
            if trace["palette_call_first"] is None:
                trace["palette_call_first"] = call_item
            trace["palette_call_last"] = call_item
        elif rva == 0x6982:
            if trace["gate_mismatch"] is None:
                trace["gate_mismatch"] = {
                    "row_pointer": hex(emu.reg_read(UC_X86_REG_ESI)),
                    "byte_index": emu.reg_read(UC_X86_REG_EAX),
                }

    def on_invalid(emu: Uc, access: int, address: int, size: int, value: int, _user) -> bool:
        fail(f"invalid memory access kind={access} address={address:#x} size={size} value={value:#x} eip={emu.reg_read(UC_X86_REG_EIP):#x}")
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_INVALID, on_invalid)

    stack_pointer = STACK + 0x80000
    uc.mem_write(stack_pointer, struct.pack("<3I", STOP, 0xFFFFFFFC, 0))
    for register, value in (
        (UC_X86_REG_EAX, 0),
        (UC_X86_REG_EBX, 0),
        (UC_X86_REG_ECX, 0),
        (UC_X86_REG_EDI, 0),
        (UC_X86_REG_ESI, 0),
        (UC_X86_REG_EBP, 0),
        (UC_X86_REG_ESP, stack_pointer),
        (UC_X86_REG_EFLAGS, 0x202),
    ):
        uc.reg_write(register, value)
    emu_error = None
    try:
        uc.emu_start(BASE + 0x5760, STOP, count=MAX_INSNS)
    except UcError as error:
        emu_error = str(error)
    if emu_error:
        raise AssertionError(f"Unicorn stopped with error: {emu_error}; trace={trace}")
    if trace["failure"] is not None:
        raise AssertionError(f"oracle execution failed: {trace['failure']}")
    if not trace["stopped_before_69a0_body"]:
        raise AssertionError("original loader did not reach the pre-69A0 stop boundary")
    if trace["open"] != {"path": SELECTOR_PATH, "mode": SELECTOR_MODE}:
        raise AssertionError(f"selector archive path differs: {trace['open']}")
    if trace["archive_handle"] != MEMBER_ROW + 1:
        raise AssertionError(f"original archive handle differs: {trace['archive_handle']}")
    if executed_calls != {0x5130, 0x5760, 0x68C0, 0x5210, 0x6780}:
        raise AssertionError(f"original call path differs: {sorted(hex(value) for value in executed_calls)}")
    if trace["reader_call_count"] != 259 or not trace["reader_sequence_ok"]:
        raise AssertionError(f"reader sequence mismatch: {trace}")
    if trace["palette_call_count"] != 256:
        raise AssertionError(f"palette writer call count differs: {trace['palette_call_count']}")
    if read_coverage != b"\x01" * MEMBER_LENGTH:
        raise AssertionError("physical original reads did not cover every CIF byte exactly once")
    if trace["read_bytes_total"] != MEMBER_LENGTH or trace["file_cursor"] != MEMBER_OFFSET + MEMBER_LENGTH:
        raise AssertionError("physical archive cursor/length differs from the CIF extent")

    logical_position = struct.unpack("<I", uc.mem_read(FILEPOS_TABLE + MEMBER_ROW * 4, 4))[0]
    if logical_position != 0xFFFFFFFF:
        raise AssertionError(f"closed archive handle position sentinel differs: {logical_position:#x}")
    if struct.unpack("<I", uc.mem_read(GATE, 4))[0] != 1:
        raise AssertionError("original resource gate was not raised")
    if trace["gate_mismatch"] != {"row_pointer": hex(0x92E390), "byte_index": 64}:
        raise AssertionError(f"first original gate mismatch differs: {trace['gate_mismatch']}")

    logical_palette = bytes(uc.mem_read(PALETTE_LOG + 4, 0x400))
    dib_entries = bytes(uc.mem_read(PALETTE_DIB + 0x28, 0x400))
    expected_logical = bytearray()
    expected_dib = bytearray()
    for index in range(256):
        red, green, blue = member[index * 3:index * 3 + 3]
        if index == 0:
            red, green, blue = 0, 0, 0
        elif index == 255:
            red, green, blue = 255, 255, 255
        expected_logical.extend((red, green, blue, 4))
        expected_dib.extend((blue, green, red, 0))
    if logical_palette != bytes(expected_logical):
        raise AssertionError("native logical palette bytes differ from RGB+flag4 mapping")
    if dib_entries != bytes(expected_dib):
        raise AssertionError("native DIB entries differ from BGR0 mapping")
    if bytes(uc.mem_read(PALETTE_DIB, 0x28)) != b"\xA5" * 0x28:
        raise AssertionError("palette helper changed the DIB header canary")
    if bytes(uc.mem_read(PALETTE_LOG, 4)) != b"\xB6" * 4:
        raise AssertionError("palette helper changed the LOGPALETTE header canary")
    if bytes(uc.mem_read(PALETTE_DIB + 0x28 + 0x400, 0x100)) != b"\xA5" * 0x100:
        raise AssertionError("palette helper changed bytes past DIB entries")
    if bytes(uc.mem_read(PALETTE_LOG + 4 + 0x400, 0x100)) != b"\xB6" * 0x100:
        raise AssertionError("palette helper changed bytes past logical palette entries")

    blend_actual = bytes(uc.mem_read(BLEND, 0x40000))
    shade_actual = bytes(uc.mem_read(SHADE, 0x10000))
    clut_actual = bytes(uc.mem_read(CLUT, 0x1F00))
    row32_actual = bytes(uc.mem_read(0x92E390, 0x100))
    if blend_actual != member[0x300:0x40300]:
        raise AssertionError("native blend write differs from CIF payload")
    if shade_actual != member[0x40300:0x50300]:
        raise AssertionError("native shade write differs from CIF payload")
    if clut_actual != member[0x50300:0x52200]:
        raise AssertionError("native CLUT write differs from CIF payload")
    expected_row32 = bytes(range(64)) + member[0x300:0x300 + 0xC0]
    if row32_actual != expected_row32:
        raise AssertionError("row32 alias does not retain startup prefix and overlay blend bytes")
    if bytes(uc.mem_read(0x92C488, 8)) != b"\0" * 8:
        raise AssertionError("resource loader changed the mode/gap bytes before the CLUT")
    if bytes(uc.mem_read(0x91C487, 1)) != b"\0":
        raise AssertionError("resource loader changed the guard byte before shade table")

    original_text_after = bytes(uc.mem_read(BASE + text_rva, text_size))
    if sha256(original_text_after) != text_source_sha:
        raise AssertionError("mapped original .text changed during CPU emulation")
    exe_after = sha256(EXE_PATH.read_bytes())
    if exe_after != exe_before:
        raise AssertionError("original executable on disk changed during the fixture")

    trace["palette_call_sha256"] = sha256(bytes(palette_call_stream))
    output_palette = OUTPUT_DIR / "mainmenu.logical_palette.bin"
    output_palette.write_bytes(logical_palette)
    if sha256(output_palette.read_bytes()) != sha256(logical_palette):
        raise AssertionError("raw palette output changed after write")
    receipt = {
        "schema_version": 1,
        "status": "PASS",
        "scope": "UNCHANGED_ORIGINAL_CPU: ran original host slot 145 selector 0x5760 through 0x68c0, 0x5210, original CRT buffered reader, and 0x6780; stopped at the first byte of 0x69a0 before its presentation/palette-update body. Only external SetFilePointer/ReadFile and single-thread critical-section imports were modeled. HFS file bytes came from the pinned archive by absolute cursor. No original Windows game process, machine-code stub, code patch, or canonical source substitution.",
        "oracle": {
            "path": str(EXE_PATH),
            "sha256_before": exe_before,
            "sha256_after": exe_after,
            "image_base": hex(pe.image_base),
            "whole_text_rva": hex(text_rva),
            "whole_text_size": text_size,
            "whole_text_sha256": text_source_sha,
            "executed_instruction_byte_stream_sha256": instruction_stream.hexdigest(),
            "source_span_hashes": spans,
            "executable_text_was_read_only": True,
        },
        "runtime": {
            "python": sys.executable,
            "python_version": platform.python_version(),
            **runtime,
        },
        "startup_seed": {
            "source": "independent formulas and byte hashes matching renderer_contract/evidence/native_palette_builder.json; this test did not run 0x71a0",
            "gate_initial": 0,
            "shade_sha256": seed_hashes["shade"],
            "blend_sha256": seed_hashes["blend"],
            "clut_sha256": seed_hashes["clut"],
            "mode_and_gap_bytes": "0000000000000000",
        },
        "archive": {
            "path": str(ARCHIVE_PATH),
            **member_info,
        },
        "execution": {
            "instruction_budget": MAX_INSNS,
            "instruction_count_including_api_hooks": trace["instruction_count"],
            "original_instruction_count": trace["original_instruction_count"],
            "stopped_before_69a0_body": trace["stopped_before_69a0_body"],
            "unknown_original_code_fails": True,
            "open_argument": trace["open"],
            "original_archive_handle": trace["archive_handle"],
            "reader_calls": {
                "count": trace["reader_call_count"],
                "first": trace["reader_first"],
                "last": trace["reader_last"],
                "sequence_matches_256_rgb_triplets_then_blend_shade_clut": trace["reader_sequence_ok"],
                "palette_triplet_calls": 256,
                "blend": {"logical_position": "0x300", "destination": hex(BLEND), "length": 0x40000},
                "shade": {"logical_position": "0x40300", "destination": hex(SHADE), "length": 0x10000},
                "clut": {"logical_position": "0x50300", "destination": hex(CLUT), "length": 0x1F00},
            },
            "palette_writer": {
                "calls": trace["palette_call_count"],
                "first": trace["palette_call_first"],
                "last": trace["palette_call_last"],
                "index_rgb_argument_stream_sha256": trace["palette_call_sha256"],
                "logical_entries_sha256": sha256(logical_palette),
                "logical_entries_match_rgb_flag4": True,
                "dib_entries_sha256": sha256(dib_entries),
                "dib_entries_match_bgr0": True,
                "first_nonendpoint": {
                    "cif_rgb": list(member[3:6]),
                    "logical_entry": list(logical_palette[4:8]),
                    "dib_entry": list(dib_entries[4:8]),
                },
                "forced_endpoints": {
                    "zero": {"logical": list(logical_palette[:4]), "dib": list(dib_entries[:4])},
                    "255": {"logical": list(logical_palette[-4:]), "dib": list(dib_entries[-4:])},
                    "source_endpoints_already_equal_forced_values": True,
                    "native_helper_endpoint_branches": "verified by original 0x6780 source span hash; runtime input endpoints in this real member are already black and white",
                },
                "raw_logical_palette_output": {
                    "path": str(output_palette.relative_to(REPO)).replace("\\", "/"),
                    "length": len(logical_palette),
                    "sha256": sha256(logical_palette),
                },
            },
            "tables": {
                "blend": {"destination": hex(BLEND), "length": 0x40000, "sha256": sha256(blend_actual), "matches_cif": True},
                "shade": {"destination": hex(SHADE), "length": 0x10000, "sha256": sha256(shade_actual), "matches_cif": True},
                "clut": {"destination": hex(CLUT), "length": 0x1F00, "sha256": sha256(clut_actual), "matches_cif": True},
                "row32_alias": {
                    "address": hex(0x92E390),
                    "length": 0x100,
                    "first_64_are_startup_clut_tail": True,
                    "last_192_are_blend_prefix": True,
                    "sha256": sha256(row32_actual),
                },
            },
            "gate": {
                "first_mismatch": trace["gate_mismatch"],
                "initial": 0,
                "final": struct.unpack("<I", uc.mem_read(GATE, 4))[0],
            },
            "file_service": {
                "modeled_imports": {name: hex(imports[name]) for name in api},
                "api_counts": trace["api_counts"],
                "set_file_pointer": trace["set_file_pointer"],
                "physical_read_events": trace["read_file_events"],
                "read_bytes_total": trace["read_bytes_total"],
                "coverage_exact_once": True,
                "archive_file_cursor_end": hex(trace["file_cursor"]),
                "archive_logical_position_after_close": hex(logical_position),
            },
        },
        "limitations": [
            "Prior 0x71a0 state is formula-seeded and hash-checked against its separate original builder receipt; this fixture does not execute 0x71a0.",
            "The title resource route establishes CIF palette/table consumption, not MENUB.TGA palette use or full screen presentation.",
            "Real CIF palette endpoints are already black and white, so runtime forcing is corroborated by the unchanged helper's endpoint branches rather than a contrasting archived endpoint value.",
        ],
    }
    return receipt


if __name__ == "__main__":
    archive_bytes = ARCHIVE_PATH.read_bytes()
    OUTPUT_DIR.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_DIR.mkdir(parents=False, exist_ok=False)
    receipt = main()
    RECEIPT_PATH.parent.mkdir(parents=True, exist_ok=True)
    if RECEIPT_PATH.exists():
        raise FileExistsError(RECEIPT_PATH)
    RECEIPT_PATH.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({
        "status": receipt["status"],
        "receipt": str(RECEIPT_PATH),
        "logical_palette_sha256": receipt["execution"]["palette_writer"]["logical_entries_sha256"],
        "read_bytes_total": receipt["execution"]["file_service"]["read_bytes_total"],
        "gate_final": receipt["execution"]["gate"]["final"],
    }, indent=2))
