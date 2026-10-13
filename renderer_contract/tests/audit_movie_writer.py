#!/usr/bin/env python3
"""Self-contained, read-only static audit of TITLE's HERC movie callback route.

The repository root, both hash-pinned original images, prediction fixture,
analysis dependency directory, and fresh receipt destination are explicit CLI
arguments. Original paths are not inferred from this script's location.
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

EXPECTED_IMAGES = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "TITLE.DLL": "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
}
EXPECTED_SPANS = {
    "HERCULES.EXE": {
        (0x3140, 0x2A0): "a3b3201d1429ac76007a12f609e940297047862381d0c082049c1355c6f66f4f",
        (0x37A0, 944): "e3751f9a2cd66c6bdcb73aabc960b998db071b5084bf037325b374e865cf7564",
        (0x5450, 752): "f6c59f71ae671a28343db80cef138db0a68fa2b682fb8cea275f756b5cb41ec2",
        (0x5350, 0xFB): "cbadfbb80c4a71b0ec30b8730500f4aa41e850992ff0898b09537e8204948fc1",
        (0x6810, 0x39): "d747ce15a91809f80796c0c5eefa557277338546105d38ec555f7707d8fe24c6",
        (0x69A0, 178): "d86d895eb034d76962f072d1fda7dab14a5616d0e5fb4931020d9aa88e9fb1eb",
        (0x361B8, 1000): "52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8",
    },
    "TITLE.DLL": {
        (0x9980, 0xC0): "1e89e9472a49b241cdc943e1dc7df17e1eae761b7277f25af6858647ae92922d",
        (0xC9E0, 0x14): "309525c57cc1bb73d1c3e11d99348ebbeabc028a10ff47ec6a2c7d16dc8b4d19",
    },
}
CALLBACK_CELL_HERC_RVA = 0x36408
CALLBACK_CELL_TITLE_RVA = 0x2BD90
TITLE_INTERFACE_BASE_RVA = 0x2BB40
CALLBACK_SLOT = 148
HISTORICAL_PROTOCOL = {
    "path": "evidence/movie_callback_protocol_2026_10_11.json",
    "sha256": "e107d0ae262d3e63be77c89d2025f5dba1bc3d5b03070538aa5a7f3b62bb4ab4",
    "use": "Historical provenance only; this audit does not read or require this file.",
}


def fail(message: str) -> None:
    raise SystemExit(f"audit failed: {message}")


def no_symlink_path(path: Path) -> None:
    current = Path(path.anchor)
    for part in path.parts[1:]:
        current = current / part
        if current.is_symlink():
            fail(f"symlink path component is not accepted: {current}")


def contained(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def lexical_path(value: str, repo_root: Path, allow_assets_read: bool = False) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = repo_root / path
    if ".." in path.parts:
        fail(f"parent traversal is not accepted: {value}")
    lexical = Path(os.path.abspath(path))
    no_symlink_path(lexical)
    resolved = lexical.resolve(strict=False)
    if lexical != resolved:
        fail(f"path redirects through a resolved path: {value}")
    if not allow_assets_read and any(part.lower() == "assets" for part in lexical.parts):
        fail(f"assets paths are not accepted: {value}")
    return lexical


def resolve_input(value: str, repo_root: Path, expected_name: str) -> Path:
    resolved = lexical_path(value, repo_root, allow_assets_read=True)
    if not resolved.is_file():
        fail(f"input file does not exist: {value}")
    if resolved.name.upper() != expected_name:
        fail(f"expected input named {expected_name}, got {resolved.name}")
    return resolved


def resolve_work_path(value: str, repo_root: Path, role: str, must_exist: bool) -> Path:
    lexical = lexical_path(value, repo_root)
    work_root = repo_root / "work"
    if not contained(lexical, work_root):
        fail(f"{role} must remain inside {work_root}: {value}")
    if lexical.suffix.lower() != ".json":
        fail(f"{role} must be a .json file: {value}")
    if must_exist and not lexical.is_file():
        fail(f"{role} does not exist: {value}")
    if not must_exist and lexical.exists():
        fail(f"output must be a new file: {value}")
    if lexical.exists() and lexical.is_dir():
        fail(f"{role} must name a file, not a directory")
    return lexical


def reject_overlap(output: Path, inputs: list[Path]) -> None:
    for input_path in inputs:
        if output == input_path or contained(output, input_path) or contained(input_path, output):
            fail(f"output overlaps an input path: {input_path}")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def hx(value: int) -> str:
    return f"0x{value:x}"


def span(pe, rva: int, size: int) -> dict:
    data = pe.get_data(rva, size)
    if len(data) != size:
        fail(f"short PE span at {hx(rva)}: {len(data)} != {size}")
    digest = sha256(data)
    expected = EXPECTED_SPANS[Path(pe.__path__).name.upper()].get((rva, size))
    if expected is None or digest != expected:
        fail(f"unrecognized pinned span {Path(pe.__path__).name} {hx(rva)}+{hx(size)}: {digest}")
    return {"rva": hx(rva), "size": size, "sha256": digest}


def decode_span(md, pe, rva: int, size: int) -> dict[int, dict]:
    data = pe.get_data(rva, size)
    decoded = {}
    for ins in md.disasm(data, pe.OPTIONAL_HEADER.ImageBase + rva):
        ins_rva = ins.address - pe.OPTIONAL_HEADER.ImageBase
        decoded[ins_rva] = {
            "bytes": ins.bytes.hex(),
            "mnemonic": ins.mnemonic,
            "operands": ins.op_str,
        }
    return decoded


def require_instruction(decoded: dict[int, dict], rva: int, bytecode: str) -> dict:
    item = decoded.get(rva)
    if item is None or item["bytes"] != bytecode:
        fail(f"unexpected instruction at {hx(rva)}: {item}")
    return {"rva": hx(rva), "bytes": bytecode}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--title-dll", required=True)
    parser.add_argument("--prediction-json", required=True)
    parser.add_argument("--analysis-deps", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    repo_input = Path(args.repo_root)
    repo_root = repo_input.resolve(strict=False)
    if not repo_root.is_dir() or Path(os.path.abspath(repo_input)) != repo_root:
        fail("repo root must resolve directly to an existing directory")
    no_symlink_path(repo_root)
    work_root = repo_root / "work"
    if not work_root.is_dir():
        fail(f"repository work directory does not exist: {work_root}")
    if work_root.resolve(strict=False) != work_root:
        fail("repository work directory redirects through a resolved path")
    no_symlink_path(work_root)

    dep_path = Path(args.analysis_deps).resolve(strict=False)
    if not dep_path.is_dir():
        fail(f"analysis dependency directory does not exist: {dep_path}")
    sys.path.insert(0, str(dep_path))
    try:
        import capstone
        import pefile
    except ImportError as exc:
        fail(f"analysis dependency is unavailable from {dep_path}: {exc}")

    exe_path = resolve_input(args.exe, repo_root, "HERCULES.EXE")
    title_path = resolve_input(args.title_dll, repo_root, "TITLE.DLL")
    prediction_path = lexical_path(args.prediction_json, repo_root)
    if not contained(prediction_path, repo_root) or not prediction_path.is_file():
        fail("prediction must be an existing fixture inside the supplied repository")
    prediction_bytes = prediction_path.read_bytes()
    if sha256(prediction_bytes) != "eb43326173d53a8b66792103bfed4761e5d47ed3104ba6e01aca44613dddd42f":
        fail("prediction differs from the recorded pre-family fixture")
    try:
        prediction = json.loads(prediction_bytes)
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        fail(f"prediction JSON is invalid: {exc}")
    if prediction.get("family") != "TITLE movie callback to HERC slot 148 / RVA 0x5450":
        fail("prediction fixture family does not match this audit")
    output_path = resolve_work_path(args.output, repo_root, "output", False)
    reject_overlap(output_path, [exe_path, title_path, prediction_path])
    images = {}
    pe_by_name = {}
    for name, path in (("HERCULES.EXE", exe_path), ("TITLE.DLL", title_path)):
        raw = path.read_bytes()
        digest = sha256(raw)
        if digest != EXPECTED_IMAGES[name]:
            fail(f"{name} hash mismatch: {digest}")
        pe = pefile.PE(data=raw, fast_load=False)
        expected_base = 0x400000 if name == "HERCULES.EXE" else 0x10000000
        if pe.OPTIONAL_HEADER.ImageBase != expected_base:
            fail(f"unexpected {name} image base: {hx(pe.OPTIONAL_HEADER.ImageBase)}")
        pe.__path__ = path
        pe_by_name[name] = pe
        images[name] = {"path": str(path), "sha256": digest, "image_base": hx(expected_base)}

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    herc = pe_by_name["HERCULES.EXE"]
    title = pe_by_name["TITLE.DLL"]

    span_receipts = {
        "HERCULES.EXE": [
            span(herc, 0x3140, 0x2A0),
            span(herc, 0x37A0, 944),
            span(herc, 0x5450, 752),
            span(herc, 0x5350, 0xFB),
            span(herc, 0x6810, 0x39),
            span(herc, 0x69A0, 178),
            span(herc, 0x361B8, 1000),
        ],
        "TITLE.DLL": [span(title, 0x9980, 0xC0), span(title, 0xC9E0, 0x14)],
    }
    herc_movie = decode_span(md, herc, 0x5450, 752)
    drive_resolver = decode_span(md, herc, 0x5350, 0xFB)
    herc_open = decode_span(md, herc, 0x3140, 0x2A0)
    herc_tick = decode_span(md, herc, 0x37A0, 944)
    palette_transition = decode_span(md, herc, 0x69A0, 178)
    saved_palette = decode_span(md, herc, 0x6810, 0x39)
    title_caller = decode_span(md, title, 0x9980, 0xC0)
    title_thunk = decode_span(md, title, 0xC9E0, 0x14)

    # Call-site contract: outer TITLE formal 1 and formal 4 are forwarded in
    # that order to the two-argument table wrapper.
    title_call_checks = [
        require_instruction(title_caller, 0x9980, "a144bb0210"),
        require_instruction(title_caller, 0x99AE, "8b7c240c"),
        require_instruction(title_caller, 0x9A1C, "8b442418"),
        require_instruction(title_caller, 0x9A23, "50"),
        require_instruction(title_caller, 0x9A24, "57"),
        require_instruction(title_caller, 0x9A25, "e8b62f0000"),
    ]
    wrapper_checks = [
        require_instruction(title_thunk, 0xC9E0, "8b442408"),
        require_instruction(title_thunk, 0xC9E4, "8b4c2404"),
        require_instruction(title_thunk, 0xC9E8, "50"),
        require_instruction(title_thunk, 0xC9E9, "51"),
        require_instruction(title_thunk, 0xC9EA, "ff1590bd0210"),
        require_instruction(title_thunk, 0xC9F0, "83c408"),
        require_instruction(title_thunk, 0xC9F3, "c3"),
    ]
    callback_value = struct.unpack("<I", herc.get_data(CALLBACK_CELL_HERC_RVA, 4))[0]
    if callback_value != 0x405450:
        fail(f"HERC slot-148 cell resolves to {hx(callback_value)}")
    if CALLBACK_CELL_TITLE_RVA - TITLE_INTERFACE_BASE_RVA != CALLBACK_SLOT * 4:
        fail("TITLE wrapper cell does not map to measured slot 148")

    herc_checks = []
    for rva, data in {
        0x54A9: "e862130000",      # snapshot current palette entry through 6810
        0x545E: "8b842434040000",  # first incoming word, filename formatting input
        0x5469: "50",              # push movie identifier to formatter
        0x546A: "6858664300",      # path-format string
        0x546F: "51",              # destination path buffer
        0x5470: "e8cbba0000",      # format path
        0x5478: "8d542430",        # same formatted local buffer to path resolver
        0x547C: "52",              # path resolver argument
        0x547D: "e8cefeffff",      # helper 5350: mutate/probe first path byte
        0x54BD: "8d542410",        # local height output address for helper arg 5
        0x54C1: "8d442418",        # local width output address for helper arg 4
        0x54C5: "52",              # push height output address
        0x54C6: "8b1540e4b900",    # IDirectSound global for helper arg 2
        0x54CC: "8d4c2418",        # local pixel output address for helper arg 3
        0x54D0: "50",              # push width output address
        0x54D1: "51",              # push pixel output address
        0x54D2: "8d442448",        # local formatted filename buffer
        0x54D6: "52",              # push sound-device argument
        0x54D7: "50",              # push filename argument
        0x54D8: "e863dcffff",      # helper 3140: header/open and output pointers
        0x5586: "8d4c241c",        # optional palette-output local address
        0x558F: "e80ce2ffff",      # helper 37a0: advance/decode and optional palette output
        0x55B3: "8a543002",        # palette component 3
        0x55B7: "8a4c3001",        # palette component 2
        0x55BC: "8a1430",          # palette component 1
        0x55CA: "83c603",          # advance palette source by three bytes
        0x55CE: "81fe00030000",    # 256 * 3 palette bytes
        0x55E8: "e8031a0000",      # lock current CPU-visible surface
        0x5600: "8b742418",        # movie width/source row bytes
        0x5604: "a11402ba00",      # destination row pitch
        0x5609: "3bc6",            # equal-pitch fast-path discriminator
        0x560D: "8b4c2410",        # movie height
        0x5616: "8b742414",        # source pixel pointer
        0x561F: "f3a5",            # contiguous rows, dword copy
        0x5626: "f3a4",            # contiguous tail bytes
        0x5636: "8b742418",        # width in per-row path
        0x563A: "8b151002ba00",    # locked destination base
        0x5644: "0faf3d1402ba00",  # y * destination pitch
        0x564B: "0faff0",          # y * movie width
        0x564E: "8b6c2414",        # source pixel base
        0x565B: "f3a5",            # row dwords
        0x5665: "3b442410",        # height loop bound
        0x566B: "c60590a5430001",  # set dirty after successful copy route
        0x5672: "e8c91a0000",      # unlock
        0x5677: "e8a40f0000",      # present
        0x569B: "8b842444040000",  # second incoming control word
        0x56A2: "50",              # pass it to internal input helper
        0x56A3: "e8b8c8ffff",      # internal 0x1f60 helper
    }.items():
        herc_checks.append(require_instruction(herc_movie, rva, data))

    helper_checks = []
    for rva, data in {
        0x3154: "8b44243c",       # retrieve original path argument
        0x3169: "50",             # pass current path buffer to CreateFileA
        0x3177: "ff153858ba00",  # CreateFileA
        0x319B: "ff152458ba00",  # ReadFile header
        0x31A9: "813d38384a004554560a",  # magic check
        0x31C7: "b900500000",    # 0x5000 dwords cleared = 80 KiB
        0x31CE: "bf881b6000",    # pixel storage at 0x601b88
        0x32FA: "e8c1edffff",    # internal frame-state init helper
        0x331F: "890dac5b6100",  # publish frame count global
        0x3327: "c700881b6000",  # *pixels output = 0x601b88
        0x3335: "8b153c384a00",  # width header field
        0x3345: "8b0d40384a00",  # height header field
        0x3379: "ff152058ba00",  # CreateThread
    }.items():
        helper_checks.append(require_instruction(herc_open, rva, data))
    drive_resolver_checks = []
    for rva, data in {
        0x5351: "a038c09100",     # resolver initialization flag
        0x535D: "0c01",           # mark drive-mask cache initialized
        0x535F: "a238c09100",     # store initialized flag
        0x5364: "ff155c57ba00",   # KERNEL32!GetLogicalDrives
        0x536A: "a334c09100",     # cache logical-drive mask
        0x536F: "a034664300",     # optional candidate enumeration gate
        0x537F: "bf18c09100",     # candidate table base 0x91C018
        0x5384: "8b356057ba00",   # KERNEL32!GetDriveTypeA pointer
        0x53BE: "ffd6",           # query candidate drive type
        0x53C0: "83f805",         # retain raw GetDriveTypeA result == 5
        0x53CE: "888818c09100",   # append candidate byte to 0x91C018 table
        0x53E0: "a044c49100",     # preferred byte at 0x91C444
        0x53E5: "8b742414",       # original caller-provided path buffer
        0x53F0: "8806",           # overwrite path buffer byte 0 with preferred byte
        0x53F2: "e8d9bf0000",     # probe preferred path via internal 0x4113D0
        0x5405: "a148c49100",     # candidate count
        0x5410: "8a8718c09100",   # read next candidate from 0x91C018+EDI
        0x5418: "a244c49100",     # update preferred byte with candidate
        0x541E: "8806",           # overwrite path buffer byte 0 with candidate
        0x5420: "e8abbf0000",     # probe candidate path via internal 0x4113D0
    }.items():
        drive_resolver_checks.append(require_instruction(drive_resolver, rva, data))
    for rva, data in {
        0x39D6: "8b442420",       # optional palette-output pointer
        0x39E5: "c70000000000",   # clear optional palette output
        0x39F3: "8b15c85b6100",   # frame chunk data source
        0x39F9: "83c205",         # palette pointer = chunk + 5
        0x39FC: "8910",           # publish optional pointer
        0x3A28: "68881b6000",     # decoded-frame destination 0x601b88
        0x3A2D: "e8aee7ffff",     # internal frame decode helper
    }.items():
        helper_checks.append(require_instruction(herc_tick, rva, data))
    palette_checks = [
        require_instruction(palette_transition, 0x69AB, "e890070000"),  # unlock
        require_instruction(palette_transition, 0x69B6, "e825050000"),  # black clear
        require_instruction(palette_transition, 0x69BE, "e85dfcffff"),  # present before palette API
        require_instruction(palette_transition, 0x69C3, "a00c02ba00"),  # mode selector
        require_instruction(palette_transition, 0x6A40, "ff156856ba00"),  # GDI palette entries
        require_instruction(palette_transition, 0x6A48, "e8b3fcffff"),  # GDI select/realize helper
    ]
    palette_checks.extend([
        require_instruction(saved_palette, 0x6810, "0fbf442404"),
        require_instruction(saved_palette, 0x6815, "8b0df8e39600"),
        require_instruction(saved_palette, 0x681B, "8a548104"),
        require_instruction(saved_palette, 0x6826, "8811"),
        require_instruction(saved_palette, 0x6838, "8b0df8e39600"),
    ])

    format_data = herc.get_data(0x36658, 32)
    format_string = format_data.split(b"\0", 1)[0]
    if format_string != b"e:\\data\\%d.etv":
        fail(f"unexpected movie path format: {format_string!r}")

    import_lookup = {}
    for descriptor in herc.DIRECTORY_ENTRY_IMPORT:
        for sym in descriptor.imports:
            if sym.name:
                import_lookup[sym.address] = {
                    "dll": descriptor.dll.decode("ascii", errors="replace"),
                    "name": sym.name.decode("ascii", errors="replace"),
                }
    endpoint_addresses = [0xBA5838, 0xBA5824, 0xBA5820, 0xBA58B0, 0xBA5B24, 0xBA5B1C, 0xBA576C, 0xBA575C, 0xBA5760]
    endpoints = {hx(address): import_lookup.get(address, {"unresolved": True})
                 for address in endpoint_addresses}
    for address in endpoint_addresses:
        if address not in import_lookup:
            fail(f"expected import endpoint missing at {hx(address)}")

    result = {
        "schema": "hercules-title-movie-writer-audit-v1",
        "scope": "Finite read-only static audit of TITLE's direct slot-148 caller and HERC 0x5450 movie file/frame/copy path; no original execution, playback, window, or graphics API calls.",
        "prediction_outcome": "PARTLY_FALSIFIED: callback route and pitched copy confirmed; TITLE passes a movie identifier and raw control word rather than frame or palette pointers.",
        "prediction": prediction.get("prediction"),
        "falsifier": prediction.get("falsifier"),
        "provenance": {
            "images": images,
            "spans": span_receipts,
            "prediction": {"path": str(prediction_path), "sha256": sha256(prediction_bytes), "created_before_family_inspection": prediction.get("created_before_family_inspection")},
            "historical_protocol": HISTORICAL_PROTOCOL,
            "runner": {"path": str(Path(sys.argv[0]).resolve()), "sha256": sha256(Path(sys.argv[0]).read_bytes())},
            "callback_table": {
                "HERCULES.EXE": {"table_rva": hx(0x361B8), "size": 1000, "sha256": "52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8", "slot": CALLBACK_SLOT, "cell_rva": hx(CALLBACK_CELL_HERC_RVA), "target_va": hx(callback_value), "target_rva": "0x5450"},
                "TITLE.DLL": {"interface_base_rva": hx(TITLE_INTERFACE_BASE_RVA), "wrapper_cell_rva": hx(CALLBACK_CELL_TITLE_RVA), "offset": hx(CALLBACK_CELL_TITLE_RVA - TITLE_INTERFACE_BASE_RVA), "slot": CALLBACK_SLOT},
            },
            "runtime": {"python": platform.python_version(), "pefile": pefile.__version__, "capstone": capstone.__version__, "execution": "none"},
        },
        "route": {
            "title_direct_caller": {
                "function_rva": "0x9980",
                "callsite_rva": "0x9A25",
                "call_target_rva": "0xC9E0",
                "arguments": [
                    {"host_arg": 1, "source": "TITLE 0x9980's first formal, held in EDI", "consumer_view": "movie_id", "proof": "TITLE 0x99AE loads [entry ESP+4] into EDI; 0x9A24 pushes EDI."},
                    {"host_arg": 2, "source": "TITLE 0x9980's fourth raw stack formal, loaded from entry [ESP+0x10]", "consumer_view": "input_abort_mask (pre-existing physical-protocol label)", "proof": "TITLE 0x9A1C loads [current ESP+0x18] after two saved registers; 0x9A23 pushes it unchanged."},
                ],
                "checks": title_call_checks,
            },
            "title_wrapper": {
                "rva": "0xC9E0",
                "forwarding": "pushes original second argument, then original first argument, and calls [TITLE interface base + 0x250] at 0xC9EA; stack order makes host arg1=TITLE first, host arg2=TITLE fourth.",
                "checks": wrapper_checks,
            },
            "host_callback": {
                "rva": "0x5450",
                "incoming_slots_observed": 2,
                "argument_1": "movie identifier formatted with the original e:\\data\\%d.etv literal; helper 0x5350 may rewrite the first byte before open",
                "argument_2": "forwarded later to internal 0x1F60; the full original prototype and caller-side semantic name are not inferred here.",
                "format_string": {"rva": "0x36658", "bytes": format_string.decode("ascii"), "sha256": sha256(format_data[:len(format_string) + 1])},
                "helper_order": [
                    {"site": "0x547D", "target": "0x5350", "role": "original host path-resolution service; probes preferred and candidate drive bytes by mutating the formatted buffer"},
                    {"site": "0x54D8", "target": "0x3140", "role": "open the post-resolution ETV path, read header, initialize pixel storage and decoder state"},
                    {"site": "0x558F", "target": "0x37A0", "role": "advance/decode current frame; optional output pointer names palette bytes"},
                    {"site": "0x55C2", "target": "0x6780", "role": "write palette triples into host palette shadow tables when helper returned a palette pointer"},
                    {"site": "0x55D8", "target": "0x69A0", "role": "apply palette before frame copy"},
                    {"site": "0x55E8", "target": "0x6FF0", "role": "lock current surface/DIB view"},
                    {"site": "0x566B", "target": "0x43A590", "role": "set dirty after the frame-copy branch"},
                    {"site": "0x5672", "target": "0x7140", "role": "unlock"},
                    {"site": "0x5677", "target": "0x6620", "role": "present"},
                    {"site": "0x570A", "target": "0x6780", "role": "restore/update palette entries in later cleanup branch"},
                    {"site": "0x5720", "target": "0x69A0", "role": "apply the cleanup palette"},
                ],
                "resource_path_resolution": {
                    "format_literal": "e:\\data\\%d.etv",
                    "formatted_buffer": "local buffer formed at 0x54D2; same pointer is formatted at 0x5470, passed to resolver at 0x547D, then passed to 0x3140 at 0x54D8",
                    "resolver_rva": "0x5350",
                    "preferred_byte": {"global_va": "0x91C444", "origin": "UNRESOLVED"},
                    "candidate_search": {"gate_global_va": "0x436634", "drive_mask_import": "KERNEL32.dll!GetLogicalDrives", "drive_type_import": "KERNEL32.dll!GetDriveTypeA", "raw_return_filter": 5, "candidate_table_va": "0x91C018", "probe_helper_rva": "0x4113D0"},
                    "mutation_sites": ["0x53F0: preferred byte -> [ESI] (path byte 0)", "0x541E: candidate byte -> [ESI] (path byte 0)"],
                    "open_argument": "0x3140 receives the same buffer pointer and passes it to CreateFileA; no subsequent buffer mutation is observed before that call",
                    "selected_drive": "UNRESOLVED",
                    "classification": "Original host resource lookup/open service to preserve. This audit does not reconstruct filesystem behavior or execute native filesystem APIs.",
                    "checks": drive_resolver_checks,
                },
                "open_helper_argument_order": [
                    "arg1: formatted path buffer after 0x5350 first-byte resolution (local address formed at 0x54D2)",
                    "arg2: HERC sound-device global at 0xB9E440",
                    "arg3: local pixel-output slot at ESP+0x14",
                    "arg4: local width-output slot at ESP+0x18",
                    "arg5: local height-output slot at ESP+0x10",
                ],
                "frame_step_call": "0x37A0 receives a pointer to local optional-palette output at ESP+0x1C as its first argument; its second argument comes from the low-word result of internal 0xFB10 and is not semantically named here.",
                "checks": herc_checks,
                "palette_helper_checks": palette_checks,
            },
        },
        "frame_layout": {
            "file": {"path_format_literal": "e:\\data\\%d.etv", "runtime_path": "UNRESOLVED drive byte: HERC 0x5350 mutates byte 0 of the formatted buffer before HERC 0x3140 passes it to KERNEL32.dll!CreateFileA", "open_endpoint": "KERNEL32.dll!CreateFileA", "header_read_endpoint": "KERNEL32.dll!ReadFile", "header_read_bytes": "0x348", "magic_value": "0x0A565445", "header_fields": {"width": "DWORD at 0x4A383C (header + 4)", "height": "DWORD at 0x4A3840 (header + 8)", "frame_count": "DWORD at 0x4A3844 (header + 0xC)"}},
            "decoded_storage": {"address": "0x601B88", "initial_clear_bytes": 81920, "pixel_output": "HERC 0x3140 publishes this fixed global through the pixel-output argument", "writer": "HERC 0x37A0 invokes internal 0x21E0 decoder with destination 0x601B88", "lifetime": "borrowed/reused global; no per-frame allocation or owned return is observed", "capacity_limit": "UNRESOLVED: 80 KiB is an observed clear extent, not a proven maximum frame extent."},
            "copy": {
                "pixel_format": "byte-indexed data; the callback copies bytes directly and performs no pixel conversion/remap",
                "remap": "No call to 0x70C0 occurs in the complete pinned 0x5450 span.",
                "source_origin": "0x601B88 as returned through the helper output; no source x/y offset is applied",
                "source_row_bytes": "movie width from header +4",
                "destination_origin": "current BA0210 locked-view base (x=0,y=0)",
                "destination_row_pitch": "BA0214, published by lock helper; fast path when it equals movie width",
                "height": "movie height from header +8",
                "extent": "width bytes x height rows; contiguous height*width copy for equal pitch, otherwise per-row copy from src+y*width to dst+y*BA0214",
                "clipping": "No width/height clipping or surface-bound comparison appears in the complete pinned 0x5450 span. Out-of-bounds source/destination safety is UNRESOLVED.",
                "dirty_unlock_present_order": ["copy", "store 0x43A590=1", "call 0x7140", "call 0x6620"],
            },
            "palette": {
                "output": "0x37A0 writes an optional pointer to current chunk + 5 when its palette-change byte is set; otherwise it clears the output pointer.",
                "consumer": "0x5450 consumes 256 successive 3-byte entries through 0x6780, then calls 0x69A0(0) before locking/copying; later cleanup writes entries through 0x6780 and calls 0x69A0(1).",
                "save_restore": "Before playback, 0x5450 reads current palette triples from 0x96E3F8 through 0x6810 into its stack buffer; cleanup writes those saved triples back through 0x6780.",
                "0x69A0_side_effect_order": ["0x7140 unlock", "0x6EE0 clear RGB(0,0,0)", "0x6620 present", "mode-specific GDI/DirectDraw palette application"],
                "movie_call_effects": "The full 0x69A0(0)/(1) prefix is not palette-only: each call enters the host clear/present path before its palette API. The 0x5450 frame copy follows the first call; the cleanup call follows movie playback and can clear/present again.",
                "borrowed_lifetime": "No explicit lease is returned. The caller consumes the pointer immediately before the next 0x37A0 call; concurrent mutation/reentrancy is not excluded.",
            },
        },
        "library_endpoints": {
            hx(a): import_lookup[a] for a in endpoint_addresses
        },
        "limits": [
            "This proves the statically observed TITLE 0x9980 -> wrapper 0xC9E0 -> host slot 148 -> HERC 0x5450 route, not that every runtime movie route uses it.",
            "The frame writer bypasses packet slot 89; 0x5450 does not directly call 0xAC60. Its message-pump/reentrant callbacks and cross-thread serialization are not closed by this static proof.",
            "No actual ETV file, movie playback, window, DIB, DirectDraw, audio device, MCI, or screen pixels were opened or executed.",
            "The decoder helper 0x21E0, complete file/container bounds, short-read/error cleanup, output buffer maximum, caller scheduling, and source-pointer lease are not fully recovered.",
        ],
    }
    output = output_path
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.parent.resolve(strict=False) != output.parent:
        fail("output parent redirects through a resolved path")
    no_symlink_path(output.parent)
    if output.exists():
        fail("output appeared after validation; refusing to overwrite")
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"result": str(output), "sha256": sha256(output.read_bytes()), "images": images}, indent=2))


if __name__ == "__main__":
    main()
