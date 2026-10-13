"""Build and run synthetic-only post-entry publication/copy/forward probes.

This controller is deliberately not an original-Hercules launcher. The
--original branch is a hard block which returns before CreateProcess is used.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import struct
import subprocess
import sys

from build_synthetic import sha256, verify_toolchain
from path_guard import (UnsafePathError, guard_work_output,
                        require_no_redirects)

DEFAULT_TOOLCHAIN = Path(
    r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953"
)
EXPECTED_MACHINE = 0x14C
EXPECTED_PE32 = 0x10B
SECTION_EXECUTE = 0x20000000
SECTION_WRITE = 0x80000000


class PE32:
    """Small read-only PE32 export/section reader for fixture-built images."""

    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        if self.data[:2] != b"MZ":
            raise ValueError("not a PE image: " + str(path))
        pe_offset = struct.unpack_from("<I", self.data, 0x3C)[0]
        if self.data[pe_offset:pe_offset + 4] != b"PE\0\0":
            raise ValueError("missing PE signature: " + str(path))
        coff = pe_offset + 4
        self.machine, self.section_count = struct.unpack_from(
            "<HH", self.data, coff)
        optional_size = struct.unpack_from("<H", self.data, coff + 16)[0]
        self.optional = coff + 20
        if struct.unpack_from("<H", self.data, self.optional)[0] != EXPECTED_PE32:
            raise ValueError("expected PE32 image: " + str(path))
        self.size_headers = struct.unpack_from(
            "<I", self.data, self.optional + 60)[0]
        self.number_dirs = struct.unpack_from(
            "<I", self.data, self.optional + 92)[0]
        self.dirs = [struct.unpack_from("<II", self.data,
                                        self.optional + 96 + 8 * index)
                     for index in range(min(self.number_dirs, 16))]
        self.sections = []
        section_offset = self.optional + optional_size
        for index in range(self.section_count):
            offset = section_offset + index * 40
            name = self.data[offset:offset + 8].split(b"\0", 1)[0].decode(
                "ascii", "replace")
            virtual_size, rva, raw_size, raw_offset = struct.unpack_from(
                "<IIII", self.data, offset + 8)
            characteristics = struct.unpack_from(
                "<I", self.data, offset + 36)[0]
            self.sections.append({
                "name": name, "virtual_size": virtual_size, "rva": rva,
                "raw_size": raw_size, "raw_offset": raw_offset,
                "characteristics": characteristics,
            })

    def rva_to_offset(self, rva: int, size: int = 1) -> int:
        if rva < self.size_headers and rva + size <= self.size_headers:
            return rva
        for section in self.sections:
            delta = rva - section["rva"]
            if delta >= 0 and delta + size <= section["raw_size"]:
                return section["raw_offset"] + delta
        raise ValueError("RVA has no raw file mapping: 0x%x in %s" %
                         (rva, self.path))

    def section_for_rva(self, rva: int) -> dict:
        for section in self.sections:
            delta = rva - section["rva"]
            if delta >= 0 and delta < max(section["raw_size"],
                                          section["virtual_size"]):
                return section
        raise ValueError("RVA has no section: 0x%x in %s" %
                         (rva, self.path))

    def exports(self) -> dict[str, int]:
        if not self.dirs or self.dirs[0][0] == 0:
            return {}
        export_offset = self.rva_to_offset(self.dirs[0][0], 40)
        fields = struct.unpack_from("<IIHHIIIIIII", self.data,
                                    export_offset)
        function_count, name_count = fields[6], fields[7]
        functions_rva, names_rva, ordinals_rva = fields[8:11]
        exports = {}
        for index in range(name_count):
            name_rva = struct.unpack_from(
                "<I", self.data,
                self.rva_to_offset(names_rva + index * 4, 4))[0]
            name_offset = self.rva_to_offset(name_rva)
            end = self.data.find(b"\0", name_offset)
            if end < 0:
                raise ValueError("unterminated export name: " + str(self.path))
            name = self.data[name_offset:end].decode("ascii", "replace")
            ordinal = struct.unpack_from(
                "<H", self.data,
                self.rva_to_offset(ordinals_rva + index * 2, 2))[0]
            if ordinal >= function_count:
                raise ValueError("invalid export ordinal: " + str(self.path))
            exports[name] = struct.unpack_from(
                "<I", self.data,
                self.rva_to_offset(functions_rva + ordinal * 4, 4))[0]
        return exports


def run(command: list[str], cwd: Path, env: dict[str, str],
        commands: list[dict], timeout: int = 60,
        expected_exit: int = 0) -> subprocess.CompletedProcess:
    try:
        result = subprocess.run(command, cwd=str(cwd), env=env,
                                capture_output=True, text=True,
                                errors="replace", timeout=timeout,
                                check=False)
    except subprocess.TimeoutExpired as exc:
        stdout = exc.stdout.decode("utf-8", "replace") if isinstance(
            exc.stdout, bytes) else (exc.stdout or "")
        stderr = exc.stderr.decode("utf-8", "replace") if isinstance(
            exc.stderr, bytes) else (exc.stderr or "")
        commands.append({"argv": command, "cwd": str(cwd),
                         "exit_code": None, "timed_out": True,
                         "stdout": stdout, "stderr": stderr})
        raise RuntimeError("command timeout: " +
                           subprocess.list2cmdline(command) +
                           "\n" + stdout + stderr)
    commands.append({"argv": command, "cwd": str(cwd),
                     "exit_code": result.returncode,
                     "stdout": result.stdout, "stderr": result.stderr})
    if result.returncode != expected_exit:
        raise RuntimeError("unexpected exit %d (wanted %d): %s\n%s%s" % (
            result.returncode, expected_exit,
            subprocess.list2cmdline(command), result.stdout, result.stderr))
    return result


def require_tokens(name: str, result: subprocess.CompletedProcess,
                   tokens: tuple[str, ...]) -> None:
    missing = [token for token in tokens
               if token not in result.stdout]
    if missing:
        raise RuntimeError(name + " missing output tokens " +
                           repr(missing) + "\n" + result.stdout +
                           result.stderr)


def image_identity(path: Path) -> dict:
    pe = PE32(path)
    if pe.machine != EXPECTED_MACHINE:
        raise ValueError("expected x86 PE image: " + str(path))
    optional_magic = int.from_bytes(pe.data[pe.optional:pe.optional + 2],
                                    "little")
    if optional_magic != EXPECTED_PE32:
        raise ValueError("expected PE32 optional header: " + str(path))
    size_of_image = int.from_bytes(pe.data[pe.optional + 56:
                                            pe.optional + 60], "little")
    if size_of_image <= 0:
        raise ValueError("invalid SizeOfImage: " + str(path))
    return {"pe": pe, "size_of_image": size_of_image,
            "machine": pe.machine, "optional_magic": optional_magic}


def require_export(image: dict, name: str, executable: bool) -> int:
    pe: PE32 = image["pe"]
    exports = pe.exports()
    export_name = name if name in exports else "_" + name
    if export_name not in exports:
        raise ValueError("required export is absent: " + name +
                         " in " + str(pe.path))
    rva = exports[export_name]
    if rva <= 0 or rva >= image["size_of_image"]:
        raise ValueError("export RVA outside image: " + name)
    section = pe.section_for_rva(rva)
    flags = section["characteristics"]
    if executable and not flags & SECTION_EXECUTE:
        raise ValueError("breakpoint export is not in executable section: " +
                         name)
    if not executable and not flags & SECTION_WRITE:
        raise ValueError("state export is not in writable section: " + name)
    return rva


def layout_header(host_path: Path, engine_path: Path,
                  proxy_path: Path, destination: Path) -> dict:
    host = image_identity(host_path)
    engine = image_identity(engine_path)
    proxy = image_identity(proxy_path)
    offsets = {
        "POSTENTRY_HOST_STATE_RVA": require_export(
            host, "SyntheticHostState", False),
        "POSTENTRY_ENGINE_STATE_RVA": require_export(
            engine, "SyntheticEngineState", False),
        "POSTENTRY_ENGINE_AFTER_COPY_RVA": require_export(
            engine, "SyntheticEngineAfterCopy", True),
        "POSTENTRY_ENGINE_AFTER_WARM_RVA": require_export(
            engine, "SyntheticEngineAfterWarmCopy", True),
        "POSTENTRY_ENGINE_AFTER_RENDER_RVA": require_export(
            engine, "SyntheticEngineAfterRender", True),
    }
    text = ["#ifndef HERCULES_POSTENTRY_LAYOUT_H",
            "#define HERCULES_POSTENTRY_LAYOUT_H", ""]
    for name, value in offsets.items():
        text.append("#define %s 0x%08xUL" % (name, value))
    text.extend(("#define POSTENTRY_HOST_IMAGE_SIZE 0x%08xUL" %
                 host["size_of_image"],
                 "#define POSTENTRY_PROXY_IMAGE_SIZE 0x%08xUL" %
                 proxy["size_of_image"],
                 "#define POSTENTRY_ENGINE_IMAGE_SIZE 0x%08xUL" %
                 engine["size_of_image"], "", "#endif", ""))
    destination.write_text("\n".join(text), encoding="ascii")
    return {
        "host": {"sha256": sha256(host_path),
                 "size_of_image": host["size_of_image"],
                 "state_export_rva": offsets["POSTENTRY_HOST_STATE_RVA"]},
        "proxy": {"sha256": sha256(proxy_path),
                  "size_of_image": proxy["size_of_image"]},
        "engine": {"sha256": sha256(engine_path),
                   "size_of_image": engine["size_of_image"],
                   "state_export_rva": offsets["POSTENTRY_ENGINE_STATE_RVA"],
                   "marker_export_rvas": {
                       name: offsets[name] for name in offsets
                       if name.startswith("POSTENTRY_ENGINE_AFTER_")}},
        "generated_header_sha256": sha256(destination),
        "marker_boundary": "Native exported C function entry RVAs parsed from the source-built engine PE export directory; each is checked to lie in an executable section before hardware breakpoints are configured.",
    }


def source_inventory(package: Path, contract: Path) -> dict[str, str]:
    paths = [path for path in package.rglob("*") if path.is_file() and
             path.suffix.lower() in (".py", ".c", ".h", ".def")]
    paths.extend(contract / name for name in (
        "hercules_host_abi.c", "hercules_host_abi.h", "hercules_types.h"))
    result = {}
    package_root = package.resolve()
    contract_root = contract.resolve()
    for path in paths:
        resolved = path.resolve(strict=True)
        if not (resolved.is_relative_to(package_root) or
                resolved.is_relative_to(contract_root)):
            raise ValueError("source inventory path escaped inputs: " +
                             str(path))
        key = (str(path.relative_to(package)) if
               path.is_relative_to(package) else
               "renderer_contract/" + path.name)
        result[key] = sha256(resolved)
    return dict(sorted(result.items()))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--toolchain-dir", type=Path, default=DEFAULT_TOOLCHAIN)
    args = parser.parse_args()

    try:
        root, work, output = guard_work_output(args.repo_root,
                                                args.output_dir)
    except UnsafePathError as exc:
        parser.error(str(exc))
    try:
        package = require_no_redirects(Path(__file__).parent)
        contract = require_no_redirects(root / "renderer_contract")
    except (UnsafePathError, OSError) as exc:
        parser.error("fixture source path is redirected: " + str(exc))
    if not package.is_relative_to(root) or not contract.is_relative_to(root):
        parser.error("fixture and contract sources must stay under repo root")
    src = package / "src"
    output.mkdir(parents=True, exist_ok=True)
    build = output / "build"
    build.mkdir()
    toolchain = args.toolchain_dir.resolve(strict=True)
    tool_record, tool_files = verify_toolchain(root, toolchain)
    cl = toolchain / "bin" / "cl.exe"
    link = toolchain / "bin" / "link.exe"
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(toolchain / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(toolchain / "include")
    env["LIB"] = str(toolchain / "lib")

    # Rebuild the already-reviewed forwarding candidate and its fake-backend
    # tests in a fresh descendant; the system DirectDraw mode is never invoked.
    base_output = output / "base"
    base_command = [sys.executable, "-B",
                    str(package / "build_synthetic.py"),
                    "--repo-root", str(root),
                    "--output-dir", str(base_output),
                    "--toolchain-dir", str(toolchain)]
    commands: list[dict] = []
    base = run(base_command, root, env, commands, timeout=180)
    base_receipt_path = base_output / "build_receipt.json"
    base_receipt = json.loads(base_receipt_path.read_text(encoding="utf-8"))
    if base_receipt.get("smoke_result") != "PASS":
        raise RuntimeError("nested candidate/fake-backend smoke did not pass")
    proxy_dll = base_output / "build" / "ddraw.dll"

    host_sources = {
        "postentry_host.c": src / "postentry_host.c",
        "postentry_engine.c": src / "postentry_engine.c",
        "postentry_engine.def": src / "postentry_engine.def",
        "postentry_controller.c": src / "postentry_controller.c",
        "postentry_shared.h": src / "postentry_shared.h",
        "presentation_probe.c": src / "presentation_probe.c",
        "presentation_probe.h": src / "presentation_probe.h",
        "presentation_probe.def": src / "presentation_probe.def",
    }
    for name, path in host_sources.items():
        if not path.is_file():
            raise FileNotFoundError(name + ": " + str(path))
    abi_sources = {
        "hercules_host_abi.c": contract / "hercules_host_abi.c",
        "hercules_host_abi.h": contract / "hercules_host_abi.h",
        "hercules_types.h": contract / "hercules_types.h",
    }
    for name, path in abi_sources.items():
        if not path.is_file():
            raise FileNotFoundError(name + ": " + str(path))

    engine_obj = build / "postentry_engine.obj"
    host_obj = build / "postentry_host.obj"
    controller_obj = build / "postentry_controller.obj"
    abi_obj = build / "hercules_host_abi.obj"
    engine_dll = build / "synthetic_engine.dll"
    engine_lib = build / "synthetic_engine.lib"
    host_exe = build / "synthetic_host.exe"
    controller_exe = build / "postentry_controller.exe"
    layout = build / "postentry_layout.h"

    common = ["/nologo", "/W3", "/Od", "/MT"]
    include_args = ["/I" + str(contract), "/I" + str(src)]
    for source_name, object_path in (
            ("postentry_engine.c", engine_obj),
            ("postentry_host.c", host_obj)):
        run([str(cl), *common, "/c", *include_args,
             "/Fo" + str(object_path), str(host_sources[source_name])],
            build, env, commands)

    run([str(cl), *common, "/c", "/I" + str(contract),
         "/Fo" + str(abi_obj), str(abi_sources["hercules_host_abi.c"])],
        build, env, commands)

    run([str(link), "/NOLOGO", "/DLL", "/MACHINE:I386",
         "/BASE:0x10000000", "/FIXED:NO",
         "/DEF:" + str(host_sources["postentry_engine.def"]),
         "/OUT:" + str(engine_dll), "/IMPLIB:" + str(engine_lib),
         str(engine_obj), "kernel32.lib"], build, env, commands)
    run([str(link), "/NOLOGO", "/MACHINE:I386", "/SUBSYSTEM:CONSOLE",
         "/BASE:0x400000",
         "/OUT:" + str(host_exe), str(host_obj), str(abi_obj),
         "kernel32.lib"], build, env, commands)

    image_layout = layout_header(host_exe, engine_dll, proxy_dll, layout)
    run([str(cl), *common, "/c", *include_args, "/I" + str(build),
         "/Fo" + str(controller_obj),
         str(host_sources["postentry_controller.c"])], build, env, commands)
    run([str(link), "/NOLOGO", "/MACHINE:I386", "/SUBSYSTEM:CONSOLE",
         "/BASE:0x400000", "/OUT:" + str(controller_exe),
         str(controller_obj), "kernel32.lib"], build, env, commands)

    # Confirm generated exports and mapped-image sizes from the final linked
    # artifacts before allowing the debugger controller to start a child.
    final_layout = layout_header(host_exe, engine_dll, proxy_dll, layout)
    if image_layout != final_layout:
        raise RuntimeError("PE layout changed between generation and final check")
    executed_image_paths = {
        "controller": controller_exe, "host": host_exe,
        "engine": engine_dll, "proxy": proxy_dll,
    }
    executed_image_hashes_before = {
        name: sha256(path) for name, path in executed_image_paths.items()
    }

    nested_build_commands = base_receipt.get("build_commands", [])
    warning_lines = []
    for command_record in nested_build_commands + commands:
        diagnostic = command_record.get("stdout", "") + command_record.get(
            "stderr", "")
        warning_lines.extend(line.strip() for line in diagnostic.splitlines()
                             if "warning C" in line or "warning LNK" in line)
    if warning_lines:
        raise RuntimeError("VC5 produced warnings:\n" +
                           "\n".join(warning_lines))

    smoke_start = len(commands)
    mode_expectations = {
        "positive": (0, ("RESULT=PASS", "COLD_COPY_WORDS=250",
                          "COLD_COPY_MATCHES_PUBLISHED_CLONE=1",
                          "SLOT81_WRAPPER_INSTALLED=1",
                          "SLOT89_WRAPPER_INSTALLED=1",
                          "SYNTHETIC_CALLBACK_PAIR_COMPLETED=1",
                          "STOP_AFTER_ONE_SYNTHETIC_CALLBACK_PAIR",
                          "CHILD_TERMINATED=1",
                          "EXIT_PROCESS_EVENT_DRAINED=1")),
        "coldwarm": (0, ("RESULT=PASS", "COLD_COPY_WORDS=250",
                          "WARM_COPY_WORDS=250",
                          "WARM_COPY_MATCHES_PUBLISHED_CLONE=1",
                          "SYNTHETIC_CALLBACK_PAIR_COMPLETED=1",
                          "CHILD_TERMINATED=1",
                          "EXIT_PROCESS_EVENT_DRAINED=1")),
        "invalidsig": (0, ("RESULT=PASS", "INVALID_SIGNATURE_REJECTED",
                            "EXIT_PROCESS_EVENT_DRAINED=1")),
        "timeout": (0, ("RESULT=PASS", "CONTROLLER_TIMEOUT",
                         "TIMEOUT_AFTER_COLD_COPY=1",
                         "TIMEOUT_CLEANUP_VERIFIED",
                         "CHILD_TERMINATED=1",
                         "EXIT_PROCESS_EVENT_DRAINED=1")),
    }
    tests = {}
    for mode, (exit_code, tokens) in mode_expectations.items():
        result = run([str(controller_exe), "--synthetic", str(host_exe),
                      str(proxy_dll), str(engine_dll), mode],
                     build, env, commands, timeout=30,
                     expected_exit=exit_code)
        require_tokens(mode, result, tokens)
        tests[mode] = {"exit_code": result.returncode,
                       "result": "PASS", "stdout": result.stdout,
                       "stderr": result.stderr,
                       "required_tokens": list(tokens)}

    original_block = run([str(controller_exe), "--original"], build, env,
                         commands, timeout=5, expected_exit=3)
    require_tokens("original_gate", original_block,
                   ("STATE=ORIGINAL_GATE_BLOCKED", "ORIGINAL_TARGET=NOTRUN",
                    "REGISTRY_PROFILE_ISOLATION=UNPROVEN",
                    "WINDOWED_STARTUP_AND_MODE_RESTORE=UNPROVEN",
                    "ROOT_AUTHORIZATION=REQUIRED",
                    "ORIGINAL_LAUNCH_CODE=DISABLED", "RESULT=BLOCKED"))
    tests["original_block"] = {
        "exit_code": original_block.returncode,
        "result": "BLOCKED_AS_DESIGNED", "stdout": original_block.stdout,
        "stderr": original_block.stderr,
        "required_tokens": ["ORIGINAL_TARGET=NOTRUN",
                            "ORIGINAL_LAUNCH_CODE=DISABLED"]}

    executed_image_hashes_after = {
        name: sha256(path) for name, path in executed_image_paths.items()
    }
    if executed_image_hashes_before != executed_image_hashes_after:
        raise RuntimeError("an executed image changed during the smoke run")

    outputs = [engine_obj, host_obj, controller_obj, abi_obj, engine_dll,
               engine_lib, host_exe, controller_exe, layout]
    receipt = {
        "schema": "hercules-postentry-synthetic-controller-v1",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "scope": "Source-built x86 host, proxy DLL, and engine DLL only. No original game process, OS DirectDraw API, HWND, COM surface, cooperative level, palette, display mode, GPU, assets, or gameplay.",
        "original_target": "NOTRUN",
        "original_host_forwarding": "UNRESOLVED",
        "original_gate": {
            "launch_code_compiled": False,
            "--original": "returns BLOCKED before any process creation; requires separately reviewed prerequisites and root authorization before future implementation",
            "registry_profile_isolation": "UNPROVEN",
            "windowed_startup_and_mode_restore": "UNPROVEN",
        },
        "toolchain": {
            "id": tool_record["id"],
            "repository": tool_record["repository"],
            "commit": tool_record["commit"],
            "archive_sha256": tool_record["archive_sha256"],
            "verified_files": tool_files,
        },
        "runtime": {
            "python_version": sys.version,
            "python_executable": sys.executable,
            "python_executable_sha256": sha256(Path(sys.executable)),
            "platform": platform.platform(),
        },
        "build_configuration": {
            "compiler_flags": ["/W3", "/Od", "/MT"],
            "controller_method": "x86 debug events plus DR0/DR1/DR2 execution hardware breakpoints at source-built engine marker export RVAs; no instruction patching",
            "marker_rvas_verified_in_executable_sections": True,
            "loaded_image_paths_and_pe_sizes_verified": True,
            "engine_copy_count": 250,
            "callback_slots": [81, 89],
            "stop_boundary": "one completed synthetic slot81/slot89 forwarding pair; terminate child before any subsequent frame",
            "raw_slot89_eax": "observed from the private physical x86 cdecl wrapper signature, while the public semantic callback type remains void",
        },
        "source_sha256": source_inventory(package, contract),
        "nested_forwarding_fixture": {
            "path": str(base_receipt_path),
            "sha256": sha256(base_receipt_path),
            "result": base_receipt.get("smoke_result"),
            "stdout": base.stdout,
            "stderr": base.stderr,
        },
        "image_layout": image_layout,
        "executed_image_sha256_before_and_after": {
            "before": executed_image_hashes_before,
            "after": executed_image_hashes_after,
            "unchanged_during_run": True,
        },
        "compiler_warning_count": len(warning_lines),
        "compiler_warnings": warning_lines,
        "build_commands": commands[:smoke_start],
        "synthetic_tests": tests,
        "smoke_commands": commands[smoke_start:],
        "output_sha256": {path.name: sha256(path) for path in outputs},
        "final_status": "PASS",
        "limits": [
            "The engine, host, and marker functions are synthetic source-built fixtures; this does not observe any original engine's table copy.",
            "The synthetic render callback pair is not a GPU presentation or actual displayed frame.",
            "The positive case verifies clone publication, a native 250-word cold copy, installed selected wrappers, one completed forwarded callback pair, and termination/reaping.",
            "Cold/warm verifies two synthetic full-table copies; timeout verifies cleanup after a cold-copy stop without rendering.",
            "The blocked original branch tests only the controller safety gate; it does not prove registry/profile isolation or a safe original-game startup boundary.",
            "No display takeover, mode ownership, shutdown restoration, or race closure is established.",
        ],
    }
    receipt_path = output / "postentry_synthetic_receipt.json"
    receipt_path.write_text(json.dumps(receipt, indent=2, sort_keys=True) +
                            "\n", encoding="utf-8")
    print("RESULT=PASS")
    print("RECEIPT=" + str(receipt_path))
    for mode, row in tests.items():
        print("TEST_%s=%s" % (mode.upper(), row["result"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
