"""Reproducible MSVC x86 composition + synthetic-only integration/regressions.

Original EXE is read only by identity checks and the existing Unicorn oracle.
No original process is started. All executed targets are freshly source-built.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
PROBE = ROOT / "renderer_contract/fixtures/presentation_probe"
sys.path.insert(0, str(PROBE))
from build_postentry_synthetic import PE32
from path_guard import guard_work_output
sys.path.insert(0, str(ROOT / "prototypes/host_shim"))
from build import msvc, VCVARS

DEFAULT_CMAKE = Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe")
PINNED = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def fnv(data):
    value = 0xcbf29ce484222325
    for byte in data:
        value = ((value ^ byte) * 0x100000001b3) & 0xffffffffffffffff
    return value


def profile(path):
    pe = PE32(path)
    text = next(s for s in pe.sections if s["name"] == ".text")
    exports = pe.exports()
    table_rva = exports.get("CombinedTable", exports.get("_CombinedTable", 0x361b8))
    offset = pe.rva_to_offset(table_rva, 1000)
    return {"HS_HOST_BASE": int.from_bytes(pe.data[pe.optional + 28:pe.optional + 32], "little"),
            "HS_HOST_IMAGE_SIZE": int.from_bytes(pe.data[pe.optional + 56:pe.optional + 60], "little"),
            "HS_HOST_TEXT_RVA": text["rva"], "HS_HOST_TEXT_SIZE": text["virtual_size"],
            "HS_HOST_TEXT_HASH": fnv(pe.data[text["raw_offset"]:text["raw_offset"] + text["virtual_size"]]),
            "HS_HOST_TABLE_HASH": fnv(pe.data[offset:offset + 1000]),
            "HS_HOST_TABLE_RVA": table_rva,
            "HS_HOST_POINTER_RVA": exports.get("CombinedPointer", exports.get("_CombinedPointer", 0x365a0)),
            "HS_HOST_PRIMARY_RVA": exports.get("CombinedPrimary", exports.get("_CombinedPrimary", 0x68e780))}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--exe", type=Path, required=True, help="read-only pinned EXE for Unicorn")
    ap.add_argument("--output", type=Path, required=True, help="new directory below work/")
    ap.add_argument("--cmake", type=Path, default=DEFAULT_CMAKE)
    args = ap.parse_args()
    _, _, out = guard_work_output(ROOT, args.output)
    out.mkdir(parents=True)
    exe = args.exe.resolve(strict=True)
    if sha(exe) != PINNED:
        raise ValueError("expected pinned unmodified EXE (read-only oracle)")
    production_profile = profile(exe)
    assert production_profile["HS_HOST_TEXT_HASH"] == 0x080fb4c55acc7402
    assert production_profile["HS_HOST_TABLE_HASH"] == 0x97949ff63f2069e7
    cmake = str(args.cmake.resolve(strict=True))
    commands = []
    sources = [p for folder in [ROOT / "dropin_renderer", ROOT / "prototypes/host_shim", ROOT / "renderer_contract"]
               for p in folder.rglob("*") if p.suffix in {".c", ".cpp", ".h", ".hpp", ".def", ".py"}]
    sources += [ROOT / "dropin_renderer/CMakeLists.txt", ROOT / "renderer_contract/CMakeLists.txt"]
    source_hashes = {p.relative_to(ROOT).as_posix(): sha(p) for p in sorted(set(sources))}

    def run(argv, cwd=out, env=None):
        result = subprocess.run([str(a) for a in argv], cwd=cwd, env=env,
                                capture_output=True, text=True, errors="replace", timeout=300)
        commands.append({"argv": [str(a) for a in argv], "exit": result.returncode,
                         "stdout": result.stdout, "stderr": result.stderr})
        (out / "commands.json").write_text(json.dumps(commands, indent=1) + "\n", encoding="utf-8", newline="\n")
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        print(result.stdout[-2000:].strip(), flush=True)
        return result.stdout

    def compile(command, directory):
        output = msvc(command, directory)
        commands.append({"msvc": command, "exit": 0, "stdout": output})

    build = out / "build"
    run([cmake, "-S", ROOT / "dropin_renderer", "-B", build, "-G", "Visual Studio 17 2022",
         "-A", "Win32", "-DDROPIN_BUILD_HOST_SHIM=ON", f"-DPython3_EXECUTABLE={sys.executable}"])
    run([cmake, "--build", build, "--config", "Release"])
    ctest = Path(cmake).with_name("ctest.exe")
    run([ctest, "--test-dir", build, "-C", "Release", "--output-on-failure"])
    dll_dir = build / "Release"
    run([sys.executable, ROOT / "prototypes/host_shim/build.py", "--exe", exe,
         "--output", out / "standalone"])
    inc = f'/I "{ROOT / "dropin_renderer/proxy/include"}" /I "{ROOT / "prototypes/host_shim/include"}" /I "{ROOT / "renderer_contract"}" /I "{PROBE / "src"}" /I "{out}"'
    flags = "/nologo /W3 /WX /O2 /MT /GS- /Oy-"
    compile(f'cl {flags} /LD {inc} "{PROBE / "src/postentry_engine.c"}" /Fe:synthetic_engine.dll /link /DEF:"{PROBE / "src/postentry_engine.def"}"', out)
    slots = {81, 89, 101, 102, 106, 107, 114, 117, 129, 130, 131, 143, 148}
    words = ["COMBINED_TABLE_SIGNATURE"]
    for i in range(1, 250):
        words.append("(renderer_u32)host_chain" if i == 81 else
                     "(renderer_u32)host_submit" if i == 89 else
                     "(renderer_u32)host_unused" if i in slots else f"0xa300{i:04x}u")
    (out / "synthetic_table.h").write_text(",\n".join(words) + "\n", encoding="utf-8", newline="\n")
    for name, define in [("combined_host", ""), ("changed_text", "/DCOMBINED_UNUSED_RESULT=0x22110001"),
                         ("changed_table", "/DCOMBINED_TABLE_SIGNATURE=0x5ac00cad")]:
        compile(f'cl {flags} {define} {inc} "{ROOT / "dropin_renderer/tests/combined_host.c"}" "{dll_dir / "ddraw.lib"}" /Fe:{name}.exe /link /BASE:0x500000 /FIXED /DYNAMICBASE:NO', out)
    original = profile(out / "combined_host.exe")
    changed = profile(out / "changed_text.exe")
    bad_table = profile(out / "changed_table.exe")
    assert all(original[k] == changed[k] for k in original if k != "HS_HOST_TEXT_HASH")
    assert changed["HS_HOST_TEXT_HASH"] != original["HS_HOST_TEXT_HASH"]
    assert all(original[k] == bad_table[k] for k in original if k != "HS_HOST_TABLE_HASH")
    assert bad_table["HS_HOST_TABLE_HASH"] != original["HS_HOST_TABLE_HASH"]
    (out / "synthetic_host_profile.h").write_text("\n".join(
        f"#define {k} 0x{v:x}{'ULL' if 'HASH' in k else 'u'}" for k, v in original.items()) + "\n",
        encoding="utf-8", newline="\n")
    clean_env = dict(os.environ)
    clean_env.pop("HERCULES_SHIM_DISABLE", None)
    shutil.copy2(dll_dir / "ddraw.dll", out / "ddraw.dll")
    run([out / "combined_host.exe", out / "synthetic_engine.dll", "3"], env=clean_env)
    # Reuse the entire existing ABI/fault/FPU harness against the actual
    # combined DLL, then replay its resulting trace through original AC60.
    compile(f'cl {flags} /D_CRT_SECURE_NO_WARNINGS {inc} "{ROOT / "prototypes/host_shim/tests/test_host_shim.c"}" "{dll_dir / "ddraw.lib"}" /Fe:test_host_shim.exe', out)
    run([out / "test_host_shim.exe", out], env=clean_env)
    run([sys.executable, ROOT / "prototypes/host_shim/tests/ac60_oracle.py", exe,
         out, out / "ac60_differential.json"])
    synthetic_build = out / "synthetic_build"
    synthetic_flags = f'/DDROPIN_SYNTHETIC_HOST_PROFILE /I"{out}"'
    run([cmake, "-S", ROOT / "dropin_renderer", "-B", synthetic_build, "-G", "Visual Studio 17 2022",
         "-A", "Win32", "-DDROPIN_BUILD_HOST_SHIM=ON", "-DBUILD_TESTING=OFF",
         f"-DCMAKE_C_FLAGS={synthetic_flags}", f"-DCMAKE_CXX_FLAGS={synthetic_flags}"])
    run([cmake, "--build", synthetic_build, "--config", "Release", "--target", "ddraw"])
    shutil.copy2(synthetic_build / "Release/ddraw.dll", out / "ddraw.dll")
    for host, status, disabled in [("combined_host", "1", None), ("combined_host", "2", "1"),
                                   ("combined_host", "2", "forward-only"),
                                   ("changed_text", "3", None), ("changed_table", "4", None)]:
        env = dict(clean_env)
        if disabled is not None:
            env["HERCULES_SHIM_DISABLE"] = disabled
        run([out / (host + ".exe"), out / "synthetic_engine.dll", status], env=env)
    run([dll_dir / "proxy_system_smoke.exe", out / "ddraw.dll"], env=clean_env)
    portable = f'"{sys.executable}" "{ROOT / "renderer_contract/tests/run_portable_tests.py"}" --cc cl --cxx cl --output "{out / "portable"}"'
    compile(portable, out)
    tools_bin = VCVARS.parents[2] / "Tools/MSVC/14.44.35207/bin/Hostx64/x86"
    assert source_hashes == {p.relative_to(ROOT).as_posix(): sha(p) for p in sorted(set(sources))}, "sources changed during verification"
    assert PE32(dll_dir / "ddraw.dll").machine == 0x14c
    portable_receipt = json.loads((out / "portable/portable_tests.json").read_text())
    assert portable_receipt["summary"] == {"discovered": 25, "passed": 25, "failed": 0}
    print("Portable renderer contract: 25/25 passed", flush=True)
    receipt = {"kind": "combined_ddraw_synthetic_integration", "original_process_executed": False,
               "portable_summary": portable_receipt["summary"], "machine": "0x14c",
               "production_profile": production_profile, "synthetic_profile": original,
               "production_dll_sha256": sha(dll_dir / "ddraw.dll"),
               "synthetic_dll_sha256": sha(out / "ddraw.dll"),
               "oracle_sha256": sha(exe), "cmake_sha256": sha(cmake),
               "vcvars": str(VCVARS), "tools": {str(p): sha(p) for p in tools_bin.glob("*.exe") if p.name in {"cl.exe", "link.exe"}},
               "source_sha256": source_hashes,
               "commands": commands}
    (out / "receipt.json").write_text(json.dumps(receipt, indent=1) + "\n", encoding="utf-8", newline="\n")
    print("COMBINED_INTEGRATION_OK")


if __name__ == "__main__":
    main()
