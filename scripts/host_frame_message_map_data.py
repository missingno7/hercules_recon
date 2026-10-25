"""Fresh pinned-VC5 compile and static COFF-vs-retail check for CMainFrame map data.

This bounded verifier never links a game image or executes the EXE, message map,
MFC callback, DLL, or process entry point. It uses the repository's tracked PE/COFF
readers and writes only beneath work/.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
COMPILER_SHA256 = "bf9f9c74f756fed96e13f7f9a4273495c7dde0a1fb968e3ef6d760ad6d73dfeb"
ORACLE_DEFAULT = Path(r"D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE")
ORACLE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
IMAGE_BASE = 0x400000
MAP_RVA = 0x2E580
MAP_BYTES = 0x2F0
DIR32 = 0x06

# Each row is the native pointer target and its source symbol hypothesis. All
# app callback names/access qualifiers are provisional except existing OnCreate
# and SYSKEY declarations; OnCreate is protected in the canonical shared header.
ROW_TARGETS = [
    (0x4094F0, "?OnCreate@CMainFrame@@IAEHPAUtagCREATESTRUCTA@@@Z"),
    (0x409620, "?OnRva_00009620@CMainFrame@@QAEXXZ"),
    (0x409820, "?OnRva_00009820@CMainFrame@@QAEXHPAUHTASK__@@@Z"),
    (0x4097A0, "?OnRva_000097A0@CMainFrame@@QAEHXZ"),
    (0x4097C0, "?OnRva_000097C0@CMainFrame@@QAEXPAVCWnd@@@Z"),
    (0x4097F0, "?OnRva_000097F0@CMainFrame@@QAEXXZ"),
    (0x409840, "?OnRva_00009840@CMainFrame@@QAEXXZ"),
    (0x409870, "?OnRva_00009870@CMainFrame@@QAEXIHH@Z"),
    (0x409740, "?OnRva_00009740@CMainFrame@@QAEXXZ"),
    (0x409900, "?OnRva_00009900@CMainFrame@@QAEHPAVCDC@@@Z"),
    (0x409910, "?OnRva_00009910@CMainFrame@@QAEXIJ@Z"),
    (0x409950, "?OnRva_00009950@CMainFrame@@QAEXIVCPoint@@@Z"),
    (0x409950, "?OnRva_00009950@CMainFrame@@QAEXIVCPoint@@@Z"),
    (0x409950, "?OnRva_00009950@CMainFrame@@QAEXIVCPoint@@@Z"),
    (0x409990, "?OnRva_00009990@CMainFrame@@QAEXXZ"),
    (0x409A60, "?OnRva_00009A60@CMainFrame@@QAEXXZ"),
    (0x409A90, "?OnRva_00009A90@CMainFrame@@QAEXPAVCCmdUI@@@Z"),
    (0x409BC0, "?OnRva_00009BC0@CMainFrame@@QAEHPAVCWnd@@II@Z"),
    (0x409AB0, "?OnSysKeyDown@CMainFrame@@QAEXIII@Z"),
    (0x409B20, "?OnSysKeyUp@CMainFrame@@QAEXIII@Z"),
    (0x409970, "?OnRva_00009970@CMainFrame@@QAEXPAVCMenu@@@Z"),
    (0x409A70, "?OnRva_00009A70@CMainFrame@@QAEXXZ"),
    (0x409A80, "?OnRva_00009A80@CMainFrame@@QAEXXZ"),
    (0x409B40, "?OnRva_00009B40@CMainFrame@@QAEXIII@Z"),
    (0x4096C0, "?OnRva_000096C0@CMainFrame@@QAEXXZ"),
    (0x409BF0, "?OnRva_00009BF0@CMainFrame@@QAEXPAVCCmdUI@@@Z"),
    (0x409C10, "?OnRva_00009C10@CMainFrame@@QAEXPAVCCmdUI@@@Z"),
    (0x42869B, "?OnHelpFinder@CWnd@@QAEXXZ"),
    (0x428668, "?OnHelp@CFrameWnd@@IAEXXZ"),
    (0x42869B, "?OnHelpFinder@CWnd@@QAEXXZ"),
]
BASE_MAP_SYMBOL = "?messageMap@CFrameWnd@@1UAFX_MSGMAP@@B"
ENTRY_SYMBOL = "?_messageEntries@CMainFrame@@0PBUAFX_MSGMAP_ENTRY@@B"


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def find_root(start: Path) -> Path:
    for p in (start, *start.parents):
        if (p / "scripts" / "coff.py").is_file() and (p / "toolchains" / "manifest.json").is_file():
            return p
    raise RuntimeError("cannot find repository root from verifier location")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    here = Path(__file__).resolve()
    root = find_root(here.parent)
    ap.add_argument("--source", type=Path, default=root / 'src/pc/host_frame_message_map.cpp')
    ap.add_argument("--native-exe", type=Path, default=ORACLE_DEFAULT)
    ap.add_argument("--output", type=Path, default=root / "work" / "host_message_map_data")
    args = ap.parse_args()
    source = args.source.resolve()
    header = source.parent / "host_system_keys.h"
    native = args.native_exe.resolve()
    out = args.output.resolve()
    if not source.is_file() or not header.is_file():
        raise FileNotFoundError(f"map source and sibling host_system_keys.h are required: {source}")
    if not out.is_relative_to((root / "work").resolve()):
        raise ValueError("all generated data must remain under repository work/")
    if sha(native.read_bytes()) != ORACLE_SHA256:
        raise RuntimeError("retail EXE identity pin mismatch")
    compiler = TOOLCHAIN / "bin" / "cl.exe"
    if sha(compiler.read_bytes()) != COMPILER_SHA256:
        raise RuntimeError("pinned VC5 compiler identity mismatch")

    sys.path.insert(0, str(root / "scripts"))
    sys.path.insert(0, r"C:\tools\hercules\python")
    from coff import COFF
    from pe import PE
    from match import verify_toolchain
    if verify_toolchain(TOOLCHAIN) != "msvc5_rtm":
        raise RuntimeError("historical toolchain identity changed")

    sdk_files = {
        "afx.h": TOOLCHAIN / "mfc" / "include" / "afx.h",
        "afxwin.h": TOOLCHAIN / "mfc" / "include" / "afxwin.h",
        "afxmsg_.h": TOOLCHAIN / "mfc" / "include" / "afxmsg_.h",
        "windows.h": TOOLCHAIN / "include" / "windows.h",
        "nafxcw.lib": TOOLCHAIN / "mfc" / "lib" / "nafxcw.lib",
    }
    sdk_hashes = {name: {"path": str(path), "sha256": sha(path.read_bytes())} for name, path in sdk_files.items()}
    source_before, header_before = source.read_bytes(), header.read_bytes()
    out.mkdir(parents=True, exist_ok=True)
    build = out / "build"
    build.mkdir(parents=True, exist_ok=True)
    obj = build / "cmainframe_message_map.obj"
    include_dirs = [TOOLCHAIN / "include", TOOLCHAIN / "mfc" / "include", source.parent]
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(TOOLCHAIN / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = ";".join(str(p) for p in include_dirs)
    env["LIB"] = ";".join([str(TOOLCHAIN / "lib"), str(TOOLCHAIN / "mfc" / "lib")])
    command = [str(compiler), "/nologo", "/c", "/Gy", "/O2", "/MT",
               *["/I" + str(p) for p in include_dirs], "/Fo" + str(obj), str(source)]
    proc = subprocess.run(command, cwd=build, env=env, capture_output=True, text=True, errors="replace")
    if proc.returncode or not obj.is_file():
        raise RuntimeError(f"pinned VC5 compile failed ({proc.returncode}): {proc.stdout}{proc.stderr}")
    if source.read_bytes() != source_before or header.read_bytes() != header_before:
        raise RuntimeError("source/header changed during compile")
    if sha(native.read_bytes()) != ORACLE_SHA256:
        raise RuntimeError("retail oracle changed during compile")
    for item in sdk_hashes.values():
        if sha(Path(item["path"]).read_bytes()) != item["sha256"]:
            raise RuntimeError("SDK/archive input changed during compile")

    coff = COFF(obj)
    symbols = [s for s in coff.symbols.values() if s["storage"] == 2]
    map_syms = [s for s in symbols if s["name"] == "?messageMap@CMainFrame@@1UAFX_MSGMAP@@B"]
    entry_syms = [s for s in symbols if s["name"] == ENTRY_SYMBOL]
    if len(map_syms) != 1 or len(entry_syms) != 1:
        raise RuntimeError("expected one CMainFrame messageMap and one entries data symbol")
    map_symbol, entries_symbol = map_syms[0], entry_syms[0]
    if map_symbol["section"] <= 0 or map_symbol["section"] != entries_symbol["section"]:
        raise RuntimeError("map and entries do not share the expected static data section")
    section = coff.sections[map_symbol["section"] - 1]
    if section["name"] != ".rdata" or len(section["data"]) != MAP_BYTES or section["size"] != MAP_BYTES:
        raise RuntimeError("expected one complete 752-byte .rdata contribution")
    if map_symbol["value"] != 0 or entries_symbol["value"] != 8:
        raise RuntimeError("map fields do not point at header offset 0 and entries offset 8")
    if any(s["name"] == "?GetMessageMap@CMainFrame@@MBEPBUAFX_MSGMAP@@XZ" and s["section"] > 0 for s in symbols):
        raise RuntimeError("map data TU unexpectedly defines the existing message-map getter")

    pe = PE(native)
    if pe.image_base != IMAGE_BASE:
        raise RuntimeError("unexpected native image base")
    native_map = pe.read_rva(MAP_RVA, MAP_BYTES)
    if len(ROW_TARGETS) != 30 or native_map[-24:] != bytes(24):
        raise RuntimeError("native map does not have the pinned 30 rows plus zero sentinel")
    pointer_targets = {
        0: (0x430B70, BASE_MAP_SYMBOL),
        4: (0x42E588, ENTRY_SYMBOL),
    }
    for index, (target_va, symbol) in enumerate(ROW_TARGETS):
        pointer_targets[8 + index * 24 + 20] = (target_va, symbol)
    native_pointer_offsets = set(pointer_targets)
    for offset, (target_va, symbol) in pointer_targets.items():
        actual = struct.unpack_from("<I", native_map, offset)[0]
        if actual != target_va:
            raise RuntimeError(f"native pointer at map+{offset:#x} is {actual:#x}, expected {target_va:#x}")
    relocations = section["relocations"]
    reloc_by_offset = {r["offset"]: r for r in relocations}
    if len(relocations) != 32 or len(reloc_by_offset) != 32:
        raise RuntimeError("expected exactly 32 unique data pointer relocations")
    for offset, (target_va, expected_symbol) in pointer_targets.items():
        rel = reloc_by_offset.get(offset)
        if rel is None or rel["type"] != DIR32 or rel["symbol"] != expected_symbol:
            raise RuntimeError(f"relocation mismatch at +{offset:#x}: observed {rel}, expected DIR32 {expected_symbol}")
        if struct.unpack_from("<I", section["data"], offset)[0] != 0:
            raise RuntimeError(f"unexpected nonzero object addend at +{offset:#x}")

    # Compare all 752 source/native bytes after clearing the map's 32 pointer slots.
    actual_masked = bytearray(section["data"])
    native_masked = bytearray(native_map)
    for offset in native_pointer_offsets:
        actual_masked[offset:offset + 4] = bytes(4)
        native_masked[offset:offset + 4] = bytes(4)
    if actual_masked != native_masked:
        differing = [i for i, (a, b) in enumerate(zip(actual_masked, native_masked)) if a != b]
        raise RuntimeError(f"map scalar-field/sentinel mismatch at offsets {differing[:24]}")

    report = {
        "scope": "fresh pinned-VC5 static message-map data comparison only; no link, callback dispatch, game process, or real API execution",
        "root": str(root),
        "source": {"path": str(source), "sha256": sha(source_before), "bytes": len(source_before)},
        "header": {"path": str(header), "sha256": sha(header_before), "bytes": len(header_before)},
        "native_exe": {"path": str(native), "sha256": sha(native.read_bytes()), "map_rva": hex(MAP_RVA), "map_bytes": hex(MAP_BYTES), "map_sha256": sha(native_map)},
        "toolchain": {"compiler": str(compiler), "compiler_sha256": COMPILER_SHA256,
                      "sdk_and_archive": sdk_hashes},
        "compile": {"command": command, "returncode": proc.returncode,
                    "stdout": proc.stdout, "stderr": proc.stderr,
                    "object": str(obj), "object_sha256": sha(obj.read_bytes()), "object_bytes": obj.stat().st_size},
        "map_symbols": {"map": map_symbol, "entries": entries_symbol,
                        "section": section["name"], "section_bytes": section["size"],
                        "relocation_count": len(relocations), "all_relocations_DIR32": all(r["type"] == DIR32 for r in relocations)},
        "comparison": {"scalar_fields_and_zero_sentinel_equal": True, "pointer_slots_equal_after_relocation": True,
                       "native_pointer_slots": len(pointer_targets), "relocation_offsets_and_symbols_equal": True,
                       "getter_defined_in_data_TU": False},
        "relocation_targets": [
            {"offset": hex(offset), "target_va": hex(target_va), "target_rva": hex(target_va - IMAGE_BASE),
             "symbol": symbol, "type": "IMAGE_REL_I386_DIR32"}
            for offset, (target_va, symbol) in sorted(pointer_targets.items())
        ],
        "limits": [
            "Provisional OnRva labels and access are symbolic placeholders, not recovered handler source or proof of original member access.",
            "The map's pointer relocations are checked against the pinned native address values; this data check is not FUNCTION_MATCH or proof of module/link layout.",
            "The map does not supply implementations for provisional app callbacks.",
        ],
    }
    result = out / "public_message_map_data_verifier.json"
    result.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": str(result), "map_sha256": sha(native_map),
                      "source_sha256": sha(source_before), "header_sha256": sha(header_before),
                      "object_sha256": sha(obj.read_bytes()), "size": section["size"],
                      "relocations": len(relocations), "scalars_equal": True}, indent=2))

if __name__ == "__main__":
    main()
