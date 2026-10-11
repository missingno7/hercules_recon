"""Compare recovered TITLE renderers against original x86 execution; test oracle only."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'work/title_behavioral/differential/render'
SOURCE = ROOT / 'tests/title_render_differential.c'
sys.path.insert(0, str(ROOT / "scripts"))
from env import ROOT as REPO
from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain
from pe import PE

BUILD = REPO / "work/title_behavioral/build"
DLL = BUILD / "TITLE.DLL"
MAP = BUILD / "TITLE.map"
REPORT = BUILD / "report.json"
ORIGINAL = REPO / "assets/pc/HERCULES/TITLE.DLL"


def symbols_from_map(path):
    out = {}
    for line in path.read_text(errors="ignore").splitlines():
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})", line)
        if m:
            out[m.group(1)] = int(m.group(2), 16)
    return out


IMAGE_BASE = None


def rva(symbols, name):
    if name not in symbols:
        raise ValueError("Missing linked symbol: " + name)
    value = symbols[name] - IMAGE_BASE
    if value < 0:
        raise ValueError("Mapped address precedes candidate image base: " + name)
    return value


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    if digest(ORIGINAL.read_bytes()) != 'b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b':
        raise RuntimeError('Original TITLE oracle identity changed')
    report_bytes = REPORT.read_bytes()
    build_report = json.loads(report_bytes)
    expected_sha = build_report.get("dll_sha256")
    if not expected_sha or digest(DLL.read_bytes()) != expected_sha:
        raise RuntimeError("Candidate DLL does not match the fresh build report")
    context = build_report.get("context_sha256", {})
    for rel, expected in context.items():
        if digest((REPO / rel).read_bytes()) != expected:
            raise RuntimeError("Build context changed after link: " + rel)
    symbols = symbols_from_map(MAP)
    global IMAGE_BASE
    IMAGE_BASE = PE(DLL).image_base
    addresses = {
        "TITLE_17F50": rva(symbols, "_title_17f50"),
        "TITLE_181E0": rva(symbols, "_title_181e0"),
        "TITLE_186E0": rva(symbols, "_title_186e0"),
        "TITLE_18BF0": rva(symbols, "_title_18bf0"),
        "TITLE_184C0": rva(symbols, "_title_184c0"),
        "G_260C0": rva(symbols, "_g_260c0"),
        "G_264F8": rva(symbols, "_g_264f8"),
        "G_260F0": rva(symbols, "_g_260f0"),
        "G_2CBE0": rva(symbols, "_g_2cbe0"),
        "G_2CC04": rva(symbols, "_g_2cc04"),
        "G_2DF40": rva(symbols, "_g_2df40"),
        "G_2DFA0": rva(symbols, "_g_2dfa0"),
        "G_2D340": rva(symbols, "_g_2d340"),
        "G_2CBEC": rva(symbols, "_g_2cbec"),
        "G_2CBF4": rva(symbols, "_g_2cbf4"),
        "INTERFACE": rva(symbols, "_g_engine_interface"),
    }
    header = OUT / "candidate_rvas.h"
    header.write_text("\n".join(f"#define CANDIDATE_{k}_RVA 0x{v:x}UL" for k, v in addresses.items()) + "\n")
    staged = OUT / "TITLE_RECON.DLL"
    shutil.copyfile(DLL, staged)
    tc = verify_toolchain(DEFAULT_TOOLCHAIN)
    env = os.environ.copy()
    for key in ("CL", "_CL_", "LINK", "_LINK_", "HERCULES_TITLE_TRACE"):
        env.pop(key, None)
    env["PATH"] = str(DEFAULT_TOOLCHAIN / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(DEFAULT_TOOLCHAIN / "include")
    env["LIB"] = str(DEFAULT_TOOLCHAIN / "lib")
    exe = OUT / "native_emit_diff.exe"
    command = [str(DEFAULT_TOOLCHAIN / "bin/cl.exe"), "/nologo", "/O2", "/Gy", "/ML", "/W3",
               "/Fo" + str(OUT) + os.sep, "/Fe" + str(exe), '/I' + str(OUT), str(SOURCE),
               "/link", "/INCREMENTAL:NO"]
    compile_result = subprocess.run(command, cwd=OUT, env=env, capture_output=True, text=True, timeout=120)
    compile_text = compile_result.stdout + compile_result.stderr
    (OUT / "compile.log").write_text(compile_text, encoding="utf-8")
    if compile_result.returncode or not exe.is_file():
        raise RuntimeError("Fixture compile failed; see " + str(OUT / "compile.log"))
    run_command = [str(exe), str(ORIGINAL), str(staged)]
    result = subprocess.run(run_command, cwd=OUT, env=env, capture_output=True, text=True, timeout=120)
    runtime_text = result.stdout + result.stderr
    (OUT / "runtime.log").write_text(runtime_text, encoding="utf-8")
    if digest(DLL.read_bytes()) != expected_sha or REPORT.read_bytes() != report_bytes:
        raise RuntimeError("Fresh candidate build changed during the differential")
    record = {
        "scope": "Original TITLE.DLL native execution versus the freshly linked source DLL for 17f50, 181e0, 184c0, 186e0, and 18bf0. Host interface calls and the part handler are controlled identically in both modules. Compares 512 deterministic states per function, including complete 40-byte primitive records, ordering-table storage, object/scratch effects, cursor, and the clipping return value.",
        "toolchain": tc,
        "toolchain_path": str(DEFAULT_TOOLCHAIN),
        "candidate_sha256": expected_sha,
        "original_sha256": digest(ORIGINAL.read_bytes()),
        "candidate_rvas": {k: hex(v) for k, v in addresses.items()},
        "native_rvas": {"title_17f50": "0x17f50", "title_181e0": "0x181e0", "title_184c0": "0x184c0", "title_186e0": "0x186e0", "title_18bf0": "0x18bf0", "g_260c0": "0x260c0", "g_260f0": "0x260f0", "g_264f8": "0x264f8", "g_2cbe0": "0x2cbe0", "g_2cc04": "0x2cc04", "g_2cbec": "0x2cbec", "g_2cbf4": "0x2cbf4", "g_2df40": "0x2df40", "g_2dfa0": "0x2dfa0", "g_2d340": "0x2d340", "engine_interface": "0x2bb40"},
        "fixture_sha256": digest(exe.read_bytes()),
        "fixture_source_sha256": digest(SOURCE.read_bytes()),
        "candidate_rva_header_sha256": digest(header.read_bytes()),
        "fixture_command": command,
        "run_command": run_command,
        "compile_exit_code": compile_result.returncode,
        "runtime_exit_code": result.returncode,
        "compile_output": compile_text,
        "runtime_output": runtime_text,
        "limitations": ["Synthetic bounded stream states; not a game capture.", "Host graphics routines are stubbed and their output semantics are not covered.", "Only one mesh polygon and UV table entry are exercised; no full resource decode or screen-scale fixture."],
        "build_context_sha256": context,
    }
    (OUT / "native_emit_diff.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(runtime_text.strip() or compile_text.strip())
    print("report:", OUT / "native_emit_diff.json")
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
