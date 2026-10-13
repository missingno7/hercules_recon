"""Build and run only the source-built DirectDraw forwarding smoke target."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

from path_guard import UnsafePathError, guard_work_output

DEFAULT_TOOLCHAIN = Path(
    r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953"
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def verify_toolchain(root: Path, toolchain: Path) -> tuple[dict, dict]:
    manifest = json.loads((root / "toolchains" / "manifest.json").read_text(
        encoding="utf-8"))
    rows = [row for row in manifest["toolchains"]
            if row["directory"] == toolchain.name]
    if len(rows) != 1 or rows[0]["id"] != "msvc5_rtm":
        raise ValueError("expected the manifest-pinned msvc5_rtm toolchain")
    row = rows[0]
    verified = {}
    for record in row["files"]:
        path = toolchain / record["path"]
        if not path.is_file() or path.stat().st_size != record["size"] or \
                sha256(path) != record["sha256"]:
            raise ValueError("toolchain manifest mismatch: " + str(path))
        verified[record["path"]] = {
            "size": record["size"], "sha256": record["sha256"]
        }
    return row, verified


def run(command: list[str], cwd: Path, env: dict[str, str],
        commands: list[dict]) -> subprocess.CompletedProcess:
    result = subprocess.run(command, cwd=str(cwd), env=env, capture_output=True,
                            text=True, errors="replace", timeout=60,
                            check=False)
    commands.append({
        "argv": command, "cwd": str(cwd), "exit_code": result.returncode,
        "stdout": result.stdout, "stderr": result.stderr,
    })
    if result.returncode != 0:
        raise RuntimeError("command failed: " + subprocess.list2cmdline(command) +
                           "\n" + result.stdout + result.stderr)
    return result


def run_abnormal_stop(command: list[str], cwd: Path, env: dict[str, str],
                      commands: list[dict]) -> dict:
    """Kill only the source-built synthetic target after its completed trace."""
    process = subprocess.Popen(
        command, cwd=str(cwd), env=env, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, text=True, errors="replace")
    timed_out = False
    terminate_accepted = False
    try:
        stdout, stderr = process.communicate(timeout=2)
    except subprocess.TimeoutExpired:
        timed_out = True
        if process.poll() is None:
            try:
                process.terminate()
                terminate_accepted = True
            except OSError:
                terminate_accepted = process.poll() is not None
        stdout, stderr = process.communicate(timeout=5)
    record = {
        "argv": command, "cwd": str(cwd),
        "exit_code": process.returncode,
        "timed_out_as_requested": timed_out,
        "terminate_accepted": terminate_accepted,
        "process_reaped": process.poll() is not None,
        "stdout": stdout, "stderr": stderr,
    }
    commands.append(record)
    if not timed_out or not terminate_accepted or not record["process_reaped"] or \
            "ABNORMAL_READY=1" not in stdout or "RESULT=PASS" not in stdout or \
            process.returncode in (None, 0):
        raise RuntimeError("synthetic abnormal-stop cleanup was not verified:\n" +
                           stdout + stderr)
    return record


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--toolchain-dir", type=Path, default=DEFAULT_TOOLCHAIN)
    args = parser.parse_args()

    try:
        root, work, output = guard_work_output(args.repo_root,
                                                args.output_dir)
    except UnsafePathError as exc:
        parser.error(str(exc))
    package = Path(__file__).resolve().parent
    src = package / "src"
    output.mkdir(parents=True, exist_ok=True)
    build_dir = output / "build"
    build_dir.mkdir()

    toolchain = args.toolchain_dir.resolve()
    tool_record, tool_files = verify_toolchain(root, toolchain)
    cl = toolchain / "bin" / "cl.exe"
    link = toolchain / "bin" / "link.exe"
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(toolchain / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(toolchain / "include")
    env["LIB"] = str(toolchain / "lib")

    repo_abi = root / "renderer_contract"
    sources = {
        "presentation_probe.c": src / "presentation_probe.c",
        "presentation_probe.h": src / "presentation_probe.h",
        "presentation_probe.def": src / "presentation_probe.def",
        "synthetic_backend.c": src / "synthetic_backend.c",
        "synthetic_backend.def": src / "synthetic_backend.def",
        "synthetic_self_export.c": src / "synthetic_self_export.c",
        "synthetic_self_export.def": src / "synthetic_self_export.def",
        "synthetic_target.c": src / "synthetic_target.c",
        "hercules_host_abi.c": repo_abi / "hercules_host_abi.c",
        "hercules_host_abi.h": repo_abi / "hercules_host_abi.h",
        "hercules_types.h": repo_abi / "hercules_types.h",
    }
    for name, path in sources.items():
        if not path.is_file():
            raise FileNotFoundError(name + ": " + str(path))

    commands: list[dict] = []
    abi_obj = build_dir / "hercules_host_abi.obj"
    probe_obj = build_dir / "presentation_probe.obj"
    backend_obj = build_dir / "synthetic_backend.obj"
    self_export_obj = build_dir / "synthetic_self_export.obj"
    target_obj = build_dir / "synthetic_target.obj"
    probe_dll = build_dir / "ddraw.dll"
    probe_lib = build_dir / "ddraw.lib"
    backend_dll = build_dir / "synthetic_system_ddraw.dll"
    backend_lib = build_dir / "synthetic_system_ddraw.lib"
    self_export_dll = build_dir / "synthetic_self_export.dll"
    self_export_lib = build_dir / "synthetic_self_export.lib"
    target_exe = build_dir / "synthetic_target.exe"

    run([str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
         "/I" + str(repo_abi), "/Fo" + str(abi_obj),
         str(sources["hercules_host_abi.c"])], build_dir, env, commands)
    run([str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
         "/DPRESENTATION_SYNTHETIC", "/I" + str(repo_abi),
         "/I" + str(src), "/Fo" + str(probe_obj),
         str(sources["presentation_probe.c"])], build_dir, env, commands)
    run([str(link), "/NOLOGO", "/DLL", "/MACHINE:I386",
         "/BASE:0x18000000", "/FIXED:NO",
         "/DEF:" + str(sources["presentation_probe.def"]),
         "/OUT:" + str(probe_dll), "/IMPLIB:" + str(probe_lib),
         str(probe_obj), str(abi_obj), "kernel32.lib"],
        build_dir, env, commands)

    run([str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
         "/Fo" + str(backend_obj), str(sources["synthetic_backend.c"])],
        build_dir, env, commands)
    run([str(link), "/NOLOGO", "/DLL", "/MACHINE:I386",
         "/BASE:0x18100000", "/FIXED:NO",
         "/DEF:" + str(sources["synthetic_backend.def"]),
         "/OUT:" + str(backend_dll), "/IMPLIB:" + str(backend_lib),
         str(backend_obj), "kernel32.lib"], build_dir, env, commands)

    run([str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
         "/I" + str(repo_abi), "/I" + str(src), "/Fo" + str(target_obj),
         str(sources["synthetic_target.c"])], build_dir, env, commands)
    run([str(link), "/NOLOGO", "/MACHINE:I386", "/SUBSYSTEM:CONSOLE",
         "/BASE:0x400000", "/OUT:" + str(target_exe), str(target_obj),
         "kernel32.lib"], build_dir, env, commands)

    run([str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
         "/Fo" + str(self_export_obj),
         str(sources["synthetic_self_export.c"])],
        build_dir, env, commands)
    run([str(link), "/NOLOGO", "/DLL", "/MACHINE:I386",
         "/BASE:0x18200000", "/FIXED:NO",
         "/DEF:" + str(sources["synthetic_self_export.def"]),
         "/OUT:" + str(self_export_dll), "/IMPLIB:" + str(self_export_lib),
         str(self_export_obj), str(probe_lib), "kernel32.lib"],
        build_dir, env, commands)

    build_command_count = len(commands)
    smoke = run([str(target_exe), str(backend_dll)], build_dir, env, commands)
    if "RESULT=PASS" not in smoke.stdout or "FAIL=" in smoke.stdout:
        raise RuntimeError("synthetic forwarding smoke failed:\n" +
                           smoke.stdout + smoke.stderr)
    selfguard_smoke = run([str(target_exe), str(backend_dll), "--selfguard"],
                          build_dir, env, commands)
    if "SELF_MODULE_GUARD=PASS" not in selfguard_smoke.stdout or \
            "FAIL=" in selfguard_smoke.stdout:
        raise RuntimeError("synthetic self-forward guard smoke failed:\n" +
                           selfguard_smoke.stdout + selfguard_smoke.stderr)
    selfexport_smoke = run([str(target_exe), str(self_export_dll), "--selfexport"],
                           build_dir, env, commands)
    if "SELF_EXPORT_GUARD=PASS" not in selfexport_smoke.stdout or \
            "FAIL=" in selfexport_smoke.stdout:
        raise RuntimeError("synthetic self-export guard smoke failed:\n" +
                           selfexport_smoke.stdout + selfexport_smoke.stderr)

    abnormal_smoke = run_abnormal_stop(
        [str(target_exe), str(backend_dll), "--hold-after-pass"],
        build_dir, env, commands)

    outputs = [probe_dll, probe_lib, backend_dll, backend_lib, target_exe,
               self_export_dll, self_export_lib, abi_obj, probe_obj,
               backend_obj, self_export_obj, target_obj]
    receipt = {
        "schema": "hercules-presentation-forwarding-smoke-v1",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "scope": "Source-built x86 synthetic target and fake DirectDraw export only; no original host process or OS graphics API.",
        "toolchain": {
            "id": tool_record["id"],
            "repository": tool_record["repository"],
            "commit": tool_record["commit"],
            "archive_sha256": tool_record["archive_sha256"],
            "verified_files": tool_files,
        },
        "runtime": {"python": sys.version, "python_executable": sys.executable,
                    "python_executable_sha256": sha256(Path(sys.executable)),
                    "platform": platform.platform()},
        "source_sha256": {
            **{name: sha256(path) for name, path in sources.items()},
            "build_synthetic.py": sha256(Path(__file__).resolve()),
            "path_guard.py": sha256(package / "path_guard.py"),
        },
        "build_commands": commands[:build_command_count],
        "smoke_commands": commands[build_command_count:],
        "abnormal_stop": {
            "scope": "Source-built synthetic target only; it completes the clone/callback/forward trace, announces readiness, then blocks until the harness terminates and reaps it.",
            "verified": True,
            "result": abnormal_smoke,
        },
        "build_outputs_sha256": {
            path.name: sha256(path) for path in outputs
        },
        "smoke_result": "PASS",
        "smoke_stdout": smoke.stdout + selfguard_smoke.stdout +
                        selfexport_smoke.stdout + abnormal_smoke["stdout"],
        "limits": [
            "The fake backend is source-built and returns sentinel HRESULT/object values.",
            "The System32 resolution branch is code-reviewed but not called by this smoke.",
            "Synthetic self-module and forwarded self-export fixtures verify both recursion guards before any recursive DirectDraw call.",
            "No DirectDraw COM object, HWND, surface, GPU, original DLL, or gameplay was used.",
            "The callback trace records call boundaries and raw arguments only; it does not walk packet memory.",
        ],
    }
    receipt_path = output / "build_receipt.json"
    receipt_path.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n",
                            encoding="utf-8")
    print("RESULT=BUILD_PASS")
    print("RECEIPT=" + str(receipt_path))
    print(smoke.stdout, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
