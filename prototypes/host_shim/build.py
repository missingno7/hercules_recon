"""Build and test the experimental host shim (modern MSVC x86, not historical).

  python prototypes/host_shim/build.py --exe <unpatched HERCULES.EXE> --output work/host_shim_run

Builds hercules_shim.dll from host_shim.c plus the shared renderer_contract
sources, builds the synthetic harness, runs it, then runs ac60_oracle.py
against the original AC60 bytes. Writes receipt.json in --output.
Nothing here launches HERCULES.EXE or touches assets/.
"""
import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
CONTRACT = ROOT / "renderer_contract"
VCVARS = Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat")
CONTRACT_SOURCES = ["hercules_frame.c", "hercules_packets.c", "hercules_staging.c",
                    "hercules_state.c", "hercules_host_abi.c"]
CFLAGS = "/nologo /W3 /WX /O2 /MT /GS- /Oy-"


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def msvc(cmd, cwd):
    bat = Path(cwd) / "_msvc.bat"
    bat.write_text(f'@echo off\r\ncall "{VCVARS}" >nul\r\n{cmd}\r\n')
    r = subprocess.run(["cmd", "/d", "/c", str(bat)], cwd=cwd, capture_output=True, text=True)
    if r.returncode:
        sys.stdout.write(r.stdout + r.stderr)
        raise SystemExit(f"command failed: {cmd}")
    return r.stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", required=True)
    ap.add_argument("--output", required=True)
    args = ap.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    inc = f'/I "{HERE / "include"}" /I "{CONTRACT}"'
    srcs = " ".join(f'"{CONTRACT / s}"' for s in CONTRACT_SOURCES)
    msvc(f'cl {CFLAGS} /LD /DHERCULES_SHIM_BUILD {inc} "{HERE / "src" / "host_shim.c"}" {srcs} '
         f'/Fe:hercules_shim.dll /link /SAFESEH', out)
    msvc(f'cl {CFLAGS} /D_CRT_SECURE_NO_WARNINGS {inc} "{HERE / "tests" / "test_host_shim.c"}" hercules_shim.lib '
         f'/Fe:test_host_shim.exe /link /SAFESEH', out)
    version = msvc("cl 2>&1", out).strip().splitlines()[0]
    harness = subprocess.run([str(out / "test_host_shim.exe"), str(out)], cwd=out,
                             capture_output=True, text=True)
    sys.stdout.write(harness.stdout)
    oracle = subprocess.run([sys.executable, str(HERE / "tests" / "ac60_oracle.py"), args.exe,
                             str(out), str(out / "ac60_differential.json")],
                            capture_output=True, text=True)
    sys.stdout.write(oracle.stdout + oracle.stderr)
    dll = out / "hercules_shim.dll"
    with open(dll, "rb") as f:
        f.seek(0x3C)
        pe_off = int.from_bytes(f.read(4), "little")
        f.seek(pe_off + 4)
        machine = int.from_bytes(f.read(2), "little")
    receipt = {
        "kind": "host_shim_build_and_test",
        "compiler": version,
        "vcvars": str(VCVARS),
        "cflags": CFLAGS,
        "sources": {str(p.relative_to(ROOT)).replace("\\", "/"): sha256(p) for p in
                    [HERE / "include" / "hercules_shim.h", HERE / "src" / "host_shim.c",
                     HERE / "tests" / "test_host_shim.c", HERE / "tests" / "ac60_oracle.py"]
                    + [CONTRACT / s for s in CONTRACT_SOURCES]},
        "dll_sha256": sha256(dll),
        "dll_machine": hex(machine),
        "harness_exit": harness.returncode,
        "harness_summary": harness.stdout.strip().splitlines()[-1] if harness.stdout else "",
        "oracle_exit": oracle.returncode,
        "oracle": json.loads((out / "ac60_differential.json").read_text())
        if (out / "ac60_differential.json").exists() else None,
    }
    (out / "receipt.json").write_text(json.dumps(receipt, indent=1) + "\n")
    ok = harness.returncode == 0 and oracle.returncode == 0 and machine == 0x14C
    print("BUILD_AND_TEST_OK" if ok else "BUILD_AND_TEST_FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
