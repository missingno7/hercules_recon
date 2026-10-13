"""Build/run the source-only x86 import-resolution surrogate using pinned VC5."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

PINNED_TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
EXPECTED_DDRAW_LIB_SHA256 = "09632af2299d3d7ea44c86cea9ca267671488d87abeaecd7c8d99ac0ac71c7d1"


def sha_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def verify_pinned_toolchain(root: Path, toolchain: Path) -> str:
    manifest = json.loads((root / "toolchains" / "manifest.json").read_text(encoding="utf-8"))
    matches = [entry for entry in manifest["toolchains"] if entry["directory"] == toolchain.name]
    if len(matches) != 1:
        raise ValueError("Toolchain is not uniquely identified in toolchains/manifest.json")
    entry = matches[0]
    for record in entry["files"]:
        path = toolchain / record["path"]
        if not path.is_file() or path.stat().st_size != record["size"] or sha_file(path) != record["sha256"]:
            raise ValueError(f"Pinned toolchain file mismatch: {path}")
    return entry["id"]


def run(command: list[str], cwd: Path, env: dict[str, str], log: list[dict]) -> subprocess.CompletedProcess:
    result = subprocess.run(command, cwd=cwd, env=env, capture_output=True, text=True,
                            errors="replace", check=False, timeout=60)
    log.append({"argv": command, "cwd": str(cwd), "exit_code": result.returncode,
                "stdout": result.stdout, "stderr": result.stderr})
    if result.returncode:
        raise RuntimeError(f"Command failed with exit {result.returncode}: {subprocess.list2cmdline(command)}")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-root", type=Path, required=True,
                        help="reconstruction repo root containing toolchains/manifest.json")
    parser.add_argument("--output-dir", type=Path, required=True,
                        help="explicit private build/run directory under original-root/work")
    parser.add_argument("--toolchain-dir", type=Path, default=PINNED_TOOLCHAIN)
    parser.add_argument("--ddraw-import-lib", type=Path, default=Path(r"C:\tools\dx6sdk\lib\ddraw.lib"))
    args = parser.parse_args()
    root = args.original_root.resolve()
    out = args.output_dir.resolve()
    toolchain = args.toolchain_dir.resolve()
    import_lib = args.ddraw_import_lib.resolve()
    if not (root / "toolchains" / "manifest.json").is_file():
        parser.error("--original-root must contain toolchains/manifest.json")
    work_path = root / "work"
    work = work_path.resolve()
    if work != work_path:
        parser.error("refusing a redirected work directory; generated files must stay in checkout/work")
    if not out.is_relative_to(work):
        parser.error("--output-dir must be under original-root/work")
    if out.exists() and any(out.iterdir()):
        parser.error("--output-dir must be new or empty")
    toolchain_id = verify_pinned_toolchain(root, toolchain)
    if toolchain_id != "msvc5_rtm":
        raise ValueError(f"Expected msvc5_rtm, found {toolchain_id}")
    if not import_lib.is_file() or sha_file(import_lib) != EXPECTED_DDRAW_LIB_SHA256:
        raise ValueError("DX6 DirectDraw import library is missing or differs from the recorded SHA-256")

    fixture = Path(__file__).resolve().parent
    sources = {name: fixture / name for name in ("ddraw_stub.c", "ddraw.def", "probe.c")}
    for path in sources.values():
        if not path.is_file():
            raise FileNotFoundError(path)
    out.mkdir(parents=True, exist_ok=True)
    fallback = out / "system_fallback"
    fallback.mkdir(parents=True, exist_ok=True)
    cl = toolchain / "bin" / "cl.exe"
    link = toolchain / "bin" / "link.exe"
    obj_stub = out / "ddraw_stub.obj"
    obj_probe = out / "probe.obj"
    local_dll = out / "ddraw.dll"
    probe_exe = out / "probe.exe"
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(toolchain / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(toolchain / "include")
    env["LIB"] = str(toolchain / "lib")
    temporary = out / "tmp"
    temporary.mkdir()
    env["TEMP"] = str(temporary)
    env["TMP"] = str(temporary)
    commands = [
        [str(cl), "/nologo", "/c", "/O2", "/MT", "/Fo" + str(obj_stub), str(sources["ddraw_stub.c"])],
        [str(link), "/NOLOGO", "/DLL", "/MACHINE:I386", "/DEF:" + str(sources["ddraw.def"]),
         "/OUT:" + str(local_dll), "/IMPLIB:" + str(out / "ddraw.lib"), str(obj_stub), "kernel32.lib"],
        [str(cl), "/nologo", "/c", "/O2", "/MT", "/Fo" + str(obj_probe), str(sources["probe.c"])],
        [str(link), "/NOLOGO", "/MACHINE:I386", "/SUBSYSTEM:CONSOLE", "/OUT:" + str(probe_exe),
         str(obj_probe), str(import_lib), "kernel32.lib"],
    ]
    log: list[dict] = []
    for command in commands:
        run(command, out, env, log)

    copied_probe = fallback / "probe.exe"
    shutil.copy2(probe_exe, copied_probe)
    local_run = run([str(probe_exe)], out, env, log)
    fallback_run = run([str(copied_probe)], fallback, env, log)
    expected_local = str(local_dll)
    if local_run.returncode != 0 or expected_local.lower() not in local_run.stdout.lower():
        raise RuntimeError("Surrogate did not report loading its sibling source-built ddraw.dll")
    if fallback_run.returncode != 0 or "MODULE=" not in fallback_run.stdout:
        raise RuntimeError("Surrogate no-stub fallback run failed")

    receipt = {
        "scope": "SOURCE_BUILT_X86_SURROGATE_ONLY; import-resolution probe; no Hercules launch; no DirectDrawCreate call, GPU, COM object, or product proxy.",
        "builder_script_sha256": sha_file(Path(__file__).resolve()),
        "toolchain": {"id": toolchain_id, "path": str(toolchain), "verification": "listed file sizes and SHA-256 values matched toolchains/manifest.json"},
        "import_library": {"path": str(import_lib), "sha256": sha_file(import_lib), "role": "link-time import symbol for the fixture only"},
        "sources": {name: {"path": str(path), "sha256": sha_file(path)} for name, path in sources.items()},
        "commands": [{"argv": c, "cwd": str(out)} for c in commands],
        "run_commands": [{"argv": [str(probe_exe)], "cwd": str(out)},
                         {"argv": [str(copied_probe)], "cwd": str(fallback)}],
        "build_log": log,
        "outputs": {name: {"sha256": sha_file(path), "size": path.stat().st_size}
                    for name, path in (("probe.exe", probe_exe), ("local ddraw.dll", local_dll),
                                       ("fallback probe.exe", copied_probe))},
        "runs": {"sibling_stub": {"exit_code": local_run.returncode, "stdout": local_run.stdout,
                                   "stderr": local_run.stderr},
                 "no_local_stub": {"exit_code": fallback_run.returncode, "stdout": fallback_run.stdout,
                                   "stderr": fallback_run.stderr,
                                   "path_caveat": "x86 process reports C:\\WINDOWS\\SYSTEM32\\DDRAW.dll; this string does not identify the physical WoW64 file path."}},
        "interpretation": "Demonstrates app-local import resolution for this ordinary x86 surrogate on the current machine only; it does not establish Hercules host integration or target deployment behavior.",
    }
    receipt_path = out / "receipt.json"
    receipt_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": str(receipt_path), "receipt_sha256": sha_file(receipt_path),
                      "toolchain": toolchain_id, "probe_sha256": sha_file(probe_exe),
                      "local_ddraw_sha256": sha_file(local_dll),
                      "local_stub_path_reported": expected_local.lower() in local_run.stdout.lower(),
                      "fallback_exit_code": fallback_run.returncode}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
