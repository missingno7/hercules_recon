"""Build a guarded source x86 DirectDrawCreate-forwarding probe.

The real DirectDraw API is called only when --run-api is explicitly supplied.
"""
from __future__ import annotations

import argparse
import ctypes
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import struct
import subprocess
import sys

from path_guard import (UnsafePathError, guard_work_output,
                        require_no_redirects)

DEFAULT_TOOLCHAIN = Path(
    r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953"
)
PINNED_SYSTEM_DLL_SHA256 = (
    "37113406967162585c9a67c252005757459d8efe87684bbc0ce184907d0566b0"
)
PINNED_SYSTEM_DLL_SIZE = 539648
PINNED_SYSTEM_DLL_VERSION = (10, 0, 26100, 8115)
PINNED_OS_BUILD = 26200


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


def version_info(path: Path) -> tuple[int, int, int, int]:
    version = ctypes.WinDLL("version", use_last_error=True)
    version.GetFileVersionInfoSizeW.argtypes = [ctypes.c_wchar_p,
                                                 ctypes.POINTER(ctypes.c_uint)]
    version.GetFileVersionInfoSizeW.restype = ctypes.c_uint
    version.GetFileVersionInfoW.argtypes = [ctypes.c_wchar_p, ctypes.c_uint,
                                            ctypes.c_uint, ctypes.c_void_p]
    version.GetFileVersionInfoW.restype = ctypes.c_int
    version.VerQueryValueW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p,
                                       ctypes.POINTER(ctypes.c_void_p),
                                       ctypes.POINTER(ctypes.c_uint)]
    version.VerQueryValueW.restype = ctypes.c_int
    ignored = ctypes.c_uint(0)
    size = version.GetFileVersionInfoSizeW(str(path), ctypes.byref(ignored))
    if size == 0:
        raise OSError(ctypes.get_last_error(), "GetFileVersionInfoSizeW failed")
    block = ctypes.create_string_buffer(size)
    if not version.GetFileVersionInfoW(str(path), 0, size, block):
        raise OSError(ctypes.get_last_error(), "GetFileVersionInfoW failed")
    pointer = ctypes.c_void_p()
    length = ctypes.c_uint(0)
    if not version.VerQueryValueW(block, "\\", ctypes.byref(pointer),
                                  ctypes.byref(length)):
        raise OSError(ctypes.get_last_error(), "VerQueryValueW failed")

    class FixedFileInfo(ctypes.Structure):
        _fields_ = [(name, ctypes.c_uint32) for name in (
            "signature", "structure_version", "file_version_ms",
            "file_version_ls", "product_version_ms", "product_version_ls",
            "file_flags_mask", "file_flags", "file_os", "file_type",
            "file_subtype", "file_date_ms", "file_date_ls")]

    fixed = ctypes.cast(pointer, ctypes.POINTER(FixedFileInfo)).contents
    if fixed.signature != 0xFEEF04BD:
        raise ValueError("system DirectDraw fixed version signature mismatch")
    return (fixed.file_version_ms >> 16, fixed.file_version_ms & 0xffff,
            fixed.file_version_ls >> 16, fixed.file_version_ls & 0xffff)


def pe_identity(path: Path) -> dict:
    raw = path.read_bytes()
    if len(raw) < 0x40 or raw[:2] != b"MZ":
        raise ValueError("system DirectDraw file is not a DOS/PE image")
    pe_offset = struct.unpack_from("<I", raw, 0x3c)[0]
    if pe_offset + 84 > len(raw) or raw[pe_offset:pe_offset + 4] != b"PE\0\0":
        raise ValueError("system DirectDraw file has no valid PE signature")
    machine = struct.unpack_from("<H", raw, pe_offset + 4)[0]
    timestamp = struct.unpack_from("<I", raw, pe_offset + 8)[0]
    optional = pe_offset + 24
    optional_magic = struct.unpack_from("<H", raw, optional)[0]
    if optional_magic != 0x10b:
        raise ValueError("pinned DirectDraw image is not PE32")
    image_size = struct.unpack_from("<I", raw, optional + 56)[0]
    return {"machine": machine, "timestamp": timestamp,
            "image_size": image_size, "optional_magic": optional_magic}


def pin_environment(system_dll: Path) -> dict:
    expected_lexical = Path(os.environ["WINDIR"]) / "SysWOW64" / "ddraw.dll"
    system_lexical = Path(os.path.abspath(system_dll))
    if os.path.normcase(os.path.normpath(str(system_lexical))) != \
            os.path.normcase(os.path.normpath(str(expected_lexical))):
        raise ValueError("probe target must be this OS's SysWOW64\\ddraw.dll")
    expected_path = require_no_redirects(expected_lexical)
    system_dll = require_no_redirects(system_lexical)
    if os.path.normcase(str(system_dll)) != os.path.normcase(str(expected_path)):
        raise ValueError("system DirectDraw path resolves outside SysWOW64")
    if not system_dll.is_file():
        raise FileNotFoundError(system_dll)
    digest = sha256(system_dll)
    version = version_info(system_dll)
    size = system_dll.stat().st_size
    identity = pe_identity(system_dll)
    os_version = sys.getwindowsversion()
    environment = {
        "system_ddraw_path": str(system_dll),
        "system_ddraw_size": size,
        "system_ddraw_sha256": digest,
        "system_ddraw_file_version": ".".join(map(str, version)),
        "system_ddraw_pe_identity": {
            key: hex(value) for key, value in identity.items()
        },
        "os_version": os_version.platform_version or os_version.version,
        "os_build": os_version.build,
        "os_platform": platform.platform(),
        "python_version": sys.version,
        "python_executable": sys.executable,
        "python_executable_sha256": sha256(Path(sys.executable)),
    }
    if digest.lower() != PINNED_SYSTEM_DLL_SHA256 or \
            size != PINNED_SYSTEM_DLL_SIZE or \
            version != PINNED_SYSTEM_DLL_VERSION or identity["machine"] != 0x14c or \
            os_version.build != PINNED_OS_BUILD:
        raise ValueError("system DirectDraw or Windows runtime differs from pinned probe target")
    return environment


def run(command: list[str], cwd: Path, env: dict[str, str],
        commands: list[dict], timeout: int = 60) -> subprocess.CompletedProcess:
    result = subprocess.run(command, cwd=str(cwd), env=env, capture_output=True,
                            text=True, errors="replace", timeout=timeout,
                            check=False)
    commands.append({
        "argv": command, "cwd": str(cwd), "exit_code": result.returncode,
        "stdout": result.stdout, "stderr": result.stderr,
        "timeout_seconds": timeout,
    })
    if result.returncode != 0:
        raise RuntimeError("command failed: " + subprocess.list2cmdline(command) +
                           "\n" + result.stdout + result.stderr)
    return result


def parse_output(output: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in output.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            values[key] = value
    return values


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--toolchain-dir", type=Path, default=DEFAULT_TOOLCHAIN)
    parser.add_argument("--run-api", action="store_true",
                            help="explicitly call DirectDrawCreate once and release its object")
    parser.add_argument("--timeout-seconds", type=int, default=10)
    args = parser.parse_args()
    if not args.run_api:
        parser.error("this helper requires --run-api; use build_probe.py for synthetic-only mode")
    if args.timeout_seconds < 1 or args.timeout_seconds > 30:
        parser.error("--timeout-seconds must be between 1 and 30")

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

    system_ddraw = Path(os.environ["WINDIR"]) / "SysWOW64" / "ddraw.dll"
    environment_pin = pin_environment(system_ddraw)
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

    abi = root / "renderer_contract"
    sources = {
        "presentation_probe.c": src / "presentation_probe.c",
        "presentation_probe.h": src / "presentation_probe.h",
        "presentation_probe.def": src / "presentation_probe.def",
        "system_directdraw_target.c": src / "system_directdraw_target.c",
        "hercules_host_abi.c": abi / "hercules_host_abi.c",
        "hercules_host_abi.h": abi / "hercules_host_abi.h",
        "hercules_types.h": abi / "hercules_types.h",
    }
    for name, path in sources.items():
        if not path.is_file():
            raise FileNotFoundError(name + ": " + str(path))

    commands: list[dict] = []
    abi_obj = build_dir / "hercules_host_abi.obj"
    probe_obj = build_dir / "presentation_probe.obj"
    target_obj = build_dir / "system_directdraw_target.obj"
    candidate = build_dir / "ddraw.dll"
    target = build_dir / "system_directdraw_probe.exe"
    import_lib = build_dir / "ddraw.lib"

    run([str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
         "/I" + str(abi), "/Fo" + str(abi_obj),
         str(sources["hercules_host_abi.c"])], build_dir, env, commands)
    run([str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
         "/I" + str(abi), "/I" + str(src), "/Fo" + str(probe_obj),
         str(sources["presentation_probe.c"])], build_dir, env, commands)
    run([str(link), "/NOLOGO", "/DLL", "/MACHINE:I386",
         "/BASE:0x18000000", "/FIXED:NO",
         "/DEF:" + str(sources["presentation_probe.def"]),
         "/OUT:" + str(candidate), "/IMPLIB:" + str(import_lib),
         str(probe_obj), str(abi_obj), "kernel32.lib"],
        build_dir, env, commands)
    run([str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
         "/I" + str(abi), "/I" + str(src), "/Fo" + str(target_obj),
         str(sources["system_directdraw_target.c"])],
        build_dir, env, commands)
    run([str(link), "/NOLOGO", "/MACHINE:I386", "/SUBSYSTEM:CONSOLE",
         "/BASE:0x400000", "/OUT:" + str(target), str(target_obj),
         "kernel32.lib"], build_dir, env, commands)

    receipt = {
        "schema": "hercules-system-directdraw-api-probe-v2",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "scope": "Explicit source-built x86 system mode loads its sibling candidate, calls only DirectDrawCreate(NULL, &object, NULL), records logical/canonical/mapped module identity in that same process, releases any returned object through IUnknown::Release, then exits. No Hercules/original engine, HWND, cooperative-level, surface, palette, display-mode, or gameplay API is invoked.",
        "explicit_api_run_requested": True,
        "original_hercules_forwarding": "UNRESOLVED",
        "environment_pin": environment_pin,
        "toolchain": {
            "id": tool_record["id"], "repository": tool_record["repository"],
            "commit": tool_record["commit"],
            "archive_sha256": tool_record["archive_sha256"],
            "verified_files": tool_files,
        },
        "runtime": {"python": sys.version,
                    "python_executable": sys.executable,
                    "python_executable_sha256": sha256(Path(sys.executable)),
                    "platform": platform.platform()},
        "source_sha256": {
            **{name: hashlib.sha256(path.read_bytes()).hexdigest()
               for name, path in sources.items()},
            "build_system_probe.py": sha256(Path(__file__).resolve()),
            "path_guard.py": sha256(package / "path_guard.py"),
        },
        "build_commands": commands,
        "outputs": {path.name: sha256(path) for path in
                    (candidate, import_lib, target, probe_obj, abi_obj,
                     target_obj)},
        "api_run": None,
        "limits": [
            "The real API call may initialize the active display driver/runtime and allocate transient driver/system resources; those internal effects are outside this probe's control.",
            "The probe does not call SetCooperativeLevel, create surfaces or palettes, open a window, or request any display mode.",
            "A returned object is released once through its IUnknown vtable; the loaded DLL references end with the short-lived process.",
            "The result is real-system API/ABI evidence only and does not prove forwarding from an original Hercules host or engine behavior.",
        ],
    }

    run_status = "API_RUN"
    run_stdout = ""
    run_stderr = ""
    run_exit_code = None
    run_timed_out = False
    parsed = {}
    command = [str(target), str(candidate)]
    try:
        completed = subprocess.run(
            command, cwd=str(build_dir), env=env, capture_output=True,
            text=True, errors="replace", timeout=args.timeout_seconds,
            check=False)
        run_stdout = completed.stdout
        run_stderr = completed.stderr
        run_exit_code = completed.returncode
        parsed = parse_output(run_stdout)
        if completed.returncode == 0 and parsed.get("RESULT") == "API_RETURNED":
            run_status = "API_RETURNED"
        else:
            run_status = "PROBE_INFRASTRUCTURE_ERROR"
        receipt["probe_run"] = {
            "argv": command, "cwd": str(build_dir),
            "timeout_seconds": args.timeout_seconds,
            "exit_code": run_exit_code,
            "stdout": run_stdout, "stderr": run_stderr,
            "parsed_output": parsed, "status": run_status,
        }
    except subprocess.TimeoutExpired as exc:
        run_timed_out = True
        run_status = "API_TIMEOUT"
        run_stdout = exc.stdout.decode(errors="replace") if isinstance(exc.stdout, bytes) else (exc.stdout or "")
        run_stderr = exc.stderr.decode(errors="replace") if isinstance(exc.stderr, bytes) else (exc.stderr or "")
        receipt["probe_run"] = {
            "argv": command, "cwd": str(build_dir),
            "timeout_seconds": args.timeout_seconds,
            "exit_code": None, "stdout": run_stdout, "stderr": run_stderr,
            "parsed_output": parse_output(run_stdout),
            "status": run_status,
            "process_action": "subprocess timeout terminated the source-built probe process",
        }

    after_hash = sha256(system_ddraw)
    receipt["system_dll_sha256_after"] = after_hash
    if after_hash != environment_pin["system_ddraw_sha256"]:
        run_status = "SYSTEM_DLL_CHANGED"
        receipt["probe_run"]["status"] = run_status
    if run_status == "API_RETURNED":
        observed_path = parsed.get("FORWARD_MODULE_PATH", "")
        observed_file_path = parsed.get("FORWARD_FILE_PATH", "")
        if observed_file_path.startswith("\\\\?\\"):
            observed_file_path = observed_file_path[4:]
        expected_file_path = environment_pin["system_ddraw_path"]
        if parsed.get("FORWARD_STATE") != "1":
            run_status = "FORWARDER_NOT_READY"
        elif os.path.normcase(os.path.normpath(observed_file_path)) != \
                os.path.normcase(os.path.normpath(expected_file_path)):
            run_status = "UNEXPECTED_PHYSICAL_FILE_PATH"
        else:
            mapped_machine = int(parsed.get("FORWARD_MACHINE", "0x0"), 16)
            mapped_timestamp = int(parsed.get("FORWARD_TIMESTAMP", "0x0"), 16)
            mapped_size = int(parsed.get("FORWARD_IMAGE_SIZE", "0x0"), 16)
            identity = environment_pin["system_ddraw_pe_identity"]
            expected_identity = {key: int(value, 16)
                                 for key, value in identity.items()}
            if (mapped_machine != expected_identity["machine"] or
                    mapped_timestamp != expected_identity["timestamp"] or
                    mapped_size != expected_identity["image_size"]):
                run_status = "MAPPED_IMAGE_IDENTITY_MISMATCH"
            elif not observed_path or not parsed.get("FORWARD_REQUESTED_PATH") or \
                    int(parsed.get("FORWARD_EXPORT", "0x0"), 16) == 0:
                run_status = "INCOMPLETE_FORWARD_IDENTITY"
            elif "API_HRESULT" not in parsed or "API_OBJECT" not in parsed:
                run_status = "MISSING_API_RESULT"
            elif "API_RELEASE_CALLED" not in parsed or \
                    "API_RELEASE_RETURN" not in parsed:
                run_status = "MISSING_RELEASE_OBSERVATION"
            else:
                hresult = int(parsed["API_HRESULT"], 16)
                object_pointer = int(parsed["API_OBJECT"], 16)
                release_called = parsed.get("API_RELEASE_CALLED") == "1"
                release_seen = "API_RELEASE_RETURN" in parsed
                if object_pointer and not (release_called and release_seen):
                    run_status = "RETURNED_OBJECT_NOT_RELEASED"
                elif hresult == 0 and object_pointer and release_called:
                    run_status = "API_SUCCESS_OBJECT_RELEASE_CALLED"
                elif hresult != 0:
                    run_status = "API_FAILURE_RECORDED"
                elif hresult == 0 and not object_pointer:
                    run_status = "API_SUCCESS_WITH_NULL_OBJECT"
                else:
                    run_status = "API_RESULT_RECORDED"
        receipt["probe_run"]["status"] = run_status
        receipt["same_process_identity_observed"] = all(
            parsed.get(key) for key in (
                "FORWARD_REQUESTED_PATH", "FORWARD_MODULE_PATH",
                "FORWARD_FILE_PATH", "FORWARD_MACHINE",
                "FORWARD_TIMESTAMP", "FORWARD_IMAGE_SIZE",
                "FORWARD_EXPORT"))

    receipt["final_status"] = run_status
    receipt_path = output / "system_probe_receipt.json"
    receipt_path.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n",
                            encoding="utf-8")
    print("RESULT=" + run_status)
    print("RECEIPT=" + str(receipt_path))
    print(run_stdout, end="")
    if run_stderr:
        print("PROBE_STDERR=" + run_stderr, file=sys.stderr)
    return 3 if run_timed_out or run_status in {
        "PROBE_INFRASTRUCTURE_ERROR", "SYSTEM_DLL_CHANGED",
        "UNEXPECTED_PHYSICAL_FILE_PATH", "MAPPED_IMAGE_IDENTITY_MISMATCH",
        "INCOMPLETE_FORWARD_IDENTITY", "FORWARDER_NOT_READY",
        "MISSING_API_RESULT", "MISSING_RELEASE_OBSERVATION",
        "RETURNED_OBJECT_NOT_RELEASED",
    } else 0


if __name__ == "__main__":
    raise SystemExit(main())
