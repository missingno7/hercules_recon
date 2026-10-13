"""Build, smoke-check, then optionally run the bounded pre-entry probe."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import struct
import subprocess
import sys

PINNED_TOOLCHAIN = Path(
    r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953"
)
EXPECTED_HOST_SHA256 = (
    "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
)
EXPECTED_TABLE_SHA256 = (
    "52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8"
)
ENTRY_RVA = 0x11EA0
TABLE_RVA = 0x361B8
TABLE_POINTER_RVA = 0x365A0
TABLE_WORDS = 250
TABLE_SIGNATURE = 0x5AC00CAC
SLOT89_MARKER = 0xF17E0089
DIAGNOSTIC_DLL_BASE = 0x18000000


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def verify_toolchain(root: Path, toolchain: Path) -> tuple[str, dict]:
    manifest = json.loads((root / "toolchains" / "manifest.json").read_text(
        encoding="utf-8"))
    matches = [row for row in manifest["toolchains"]
               if row["directory"] == toolchain.name]
    if len(matches) != 1:
        raise ValueError("Pinned VC5 directory is not uniquely listed in manifest")
    row = matches[0]
    verified = {}
    for record in row["files"]:
        path = toolchain / record["path"]
        if (not path.is_file() or path.stat().st_size != record["size"]
                or sha256(path) != record["sha256"]):
            raise ValueError("Pinned toolchain file mismatch: " + str(path))
        verified[record["path"]] = {
            "size": record["size"], "sha256": record["sha256"]
        }
    return row["id"], verified


class PE32:
    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        if self.data[:2] != b"MZ":
            raise ValueError("not a PE image: " + str(path))
        pe_offset = struct.unpack_from("<I", self.data, 0x3C)[0]
        if self.data[pe_offset:pe_offset + 4] != b"PE\0\0":
            raise ValueError("missing PE signature: " + str(path))
        coff = pe_offset + 4
        self.machine, self.section_count = struct.unpack_from("<HH", self.data, coff)
        self.optional_size = struct.unpack_from("<H", self.data, coff + 16)[0]
        self.optional = coff + 20
        if struct.unpack_from("<H", self.data, self.optional)[0] != 0x10B:
            raise ValueError("expected PE32 image: " + str(path))
        self.entry = struct.unpack_from("<I", self.data, self.optional + 16)[0]
        self.image_base = struct.unpack_from("<I", self.data, self.optional + 28)[0]
        self.size_headers = struct.unpack_from("<I", self.data, self.optional + 60)[0]
        self.number_dirs = struct.unpack_from("<I", self.data, self.optional + 92)[0]
        self.dirs = []
        for i in range(min(self.number_dirs, 16)):
            self.dirs.append(struct.unpack_from("<II", self.data,
                                                self.optional + 96 + 8 * i))
        self.sections = []
        sec_off = self.optional + self.optional_size
        for i in range(self.section_count):
            off = sec_off + i * 40
            name = self.data[off:off + 8].split(b"\0", 1)[0].decode(
                "ascii", "replace")
            virtual_size, rva, raw_size, raw_off = struct.unpack_from(
                "<IIII", self.data, off + 8)
            characteristics = struct.unpack_from("<I", self.data, off + 36)[0]
            self.sections.append({
                "name": name, "virtual_size": virtual_size, "rva": rva,
                "raw_size": raw_size, "raw_offset": raw_off,
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

    def section_for_rva(self, rva: int, size: int = 1) -> dict:
        for section in self.sections:
            delta = rva - section["rva"]
            if delta >= 0 and delta + size <= max(section["raw_size"],
                                                  section["virtual_size"]):
                return section
        raise ValueError("RVA has no section: 0x%x in %s" % (rva, self.path))

    def imports(self) -> dict[str, list[str]]:
        if len(self.dirs) < 2 or self.dirs[1][0] == 0:
            return {}
        rva = self.dirs[1][0]
        result: dict[str, list[str]] = {}
        for index in range(1024):
            off = self.rva_to_offset(rva + index * 20, 20)
            oft, timestamp, forwarder, name_rva, first = struct.unpack_from(
                "<IIIII", self.data, off)
            if not (oft or timestamp or forwarder or name_rva or first):
                break
            dll = self.cstring(self.rva_to_offset(name_rva))
            thunk_rva = oft or first
            names = []
            for j in range(4096):
                thunk_off = self.rva_to_offset(thunk_rva + j * 4, 4)
                value = struct.unpack_from("<I", self.data, thunk_off)[0]
                if value == 0:
                    break
                if value & 0x80000000:
                    names.append("#%d" % (value & 0xFFFF))
                else:
                    names.append(self.cstring(self.rva_to_offset(value + 2)))
            result[dll.lower()] = names
        return result

    def exports(self) -> dict[str, int]:
        if not self.dirs or self.dirs[0][0] == 0:
            return {}
        off = self.rva_to_offset(self.dirs[0][0], 40)
        fields = struct.unpack_from("<IIHHIIIIIII", self.data, off)
        number_functions = fields[6]
        number_names = fields[7]
        functions_rva, names_rva, ordinals_rva = fields[8:11]
        names = {}
        for index in range(number_names):
            name_rva = struct.unpack_from(
                "<I", self.data, self.rva_to_offset(names_rva + 4 * index, 4))[0]
            name = self.cstring(self.rva_to_offset(name_rva))
            ordinal = struct.unpack_from(
                "<H", self.data,
                self.rva_to_offset(ordinals_rva + 2 * index, 2))[0]
            if ordinal >= number_functions:
                raise ValueError("invalid export ordinal in " + str(self.path))
            function_rva = struct.unpack_from(
                "<I", self.data,
                self.rva_to_offset(functions_rva + 4 * ordinal, 4))[0]
            names[name] = function_rva
        return names

    def cstring(self, offset: int) -> str:
        end = self.data.find(b"\0", offset)
        if end < 0:
            raise ValueError("unterminated PE string in " + str(self.path))
        return self.data[offset:end].decode("ascii", "replace")


def run(command: list[str], cwd: Path, env: dict[str, str],
        commands: list[dict], timeout: int = 60) -> subprocess.CompletedProcess:
    try:
        result = subprocess.run(command, cwd=str(cwd), env=env,
                                capture_output=True, text=True,
                                errors="replace", timeout=timeout,
                                check=False)
    except subprocess.TimeoutExpired as error:
        stdout = error.stdout.decode("utf-8", "replace") if isinstance(
            error.stdout, bytes) else (error.stdout or "")
        stderr = error.stderr.decode("utf-8", "replace") if isinstance(
            error.stderr, bytes) else (error.stderr or "")
        commands.append({"argv": command, "cwd": str(cwd),
                         "exit_code": None, "timed_out": True,
                         "stdout": stdout, "stderr": stderr})
        raise RuntimeError("command timed out after %d seconds: %s\n%s%s" % (
            timeout, subprocess.list2cmdline(command), stdout, stderr))
    commands.append({
        "argv": command, "cwd": str(cwd), "exit_code": result.returncode,
        "stdout": result.stdout, "stderr": result.stderr,
    })
    if result.returncode:
        raise RuntimeError("command failed (%d): %s\n%s%s" % (
            result.returncode, subprocess.list2cmdline(command),
            result.stdout, result.stderr))
    return result


def source_hashes(paths: dict[str, Path]) -> dict[str, str]:
    return {name: sha256(path) for name, path in paths.items()}


def probe_sources(root: Path, package: Path) -> dict[str, Path]:
    src = package / "src"
    abi_dir = root / "renderer_contract"
    return {
        "ddraw_stub.c": src / "ddraw_stub.c",
        "debug_controller.c": src / "debug_controller.c",
        "probe_diag.h": src / "probe_diag.h",
        "ddraw.def": src / "ddraw.def",
        "smoke_target.c": src / "smoke_target.c",
        "hercules_host_abi.c": abi_dir / "hercules_host_abi.c",
        "hercules_host_abi.h": abi_dir / "hercules_host_abi.h",
        "hercules_types.h": abi_dir / "hercules_types.h",
        "build_probe.py": package / "build_probe.py",
    }


def parse_kv_lines(text: str) -> dict[str, str]:
    values = {}
    for line in text.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            if " " not in key:
                values[key] = value
    return values


def parse_kv_occurrences(text: str) -> dict[str, list[str]]:
    values: dict[str, list[str]] = {}
    for line in text.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            if " " not in key:
                values.setdefault(key, []).append(value)
    return values


def write_layout(path: Path, host: PE32, text_section: dict,
                 table_offset: int, probe_rva: int) -> None:
    content = """#ifndef HERCULES_PREENTRY_PROBE_LAYOUT_H
#define HERCULES_PREENTRY_PROBE_LAYOUT_H
#define HOST_IMAGE_BASE 0x%08xUL
#define HOST_ENTRY_RVA 0x%08xUL
#define HOST_TABLE_RVA 0x%08xUL
#define HOST_TABLE_POINTER_RVA 0x%08xUL
#define HOST_TABLE_RAW_OFFSET 0x%08xUL
#define HOST_TEXT_RVA 0x%08xUL
#define HOST_TEXT_RAW_OFFSET 0x%08xUL
#define HOST_TEXT_RAW_SIZE 0x%08xUL
#define PROBE_DIAGNOSTIC_RVA 0x%08xUL
#endif
""" % (host.image_base, host.entry, TABLE_RVA, TABLE_POINTER_RVA,
       table_offset, text_section["rva"], text_section["raw_offset"],
       text_section["raw_size"], probe_rva)
    path.write_text(content, encoding="ascii")


def guard_paths(root: Path, output: Path, output_argument: Path,
                original: Path,
                parser: argparse.ArgumentParser) -> None:
    work_candidate = root / "work"
    if work_candidate.is_symlink() or work_candidate.is_junction():
        parser.error("refusing redirected checkout work directory")
    work = work_candidate.resolve()
    if output == work or not output.is_relative_to(work):
        parser.error("--output-dir must be a child of --repo-root/work")
    lexical_work = Path(os.path.abspath(work_candidate))
    lexical_output = Path(os.path.abspath(output_argument))
    try:
        relative_output = lexical_output.relative_to(lexical_work)
    except ValueError:
        parser.error("--output-dir must lexically stay under --repo-root/work")
    cursor = lexical_work
    for part in relative_output.parts:
        cursor = cursor / part
        if cursor.is_symlink() or cursor.is_junction():
            parser.error("refusing redirected output path: " + str(cursor))

    assets = (root / "assets").resolve()
    if output.is_relative_to(assets) or assets.is_relative_to(output):
        parser.error("output must stay separate from the assets tree")
    original_directory = original.parent.resolve()
    if (output.is_relative_to(original_directory) or
            original_directory.is_relative_to(output)):
        parser.error("output must stay separate from the original executable ancestry")


def build(args: argparse.Namespace, root: Path, original: Path,
          output: Path) -> dict:
    if output.exists() and any(output.iterdir()):
        raise ValueError("build-only requires a new or empty --output-dir")
    output.mkdir(parents=True, exist_ok=True)
    build_dir = output / "build"
    build_dir.mkdir()

    host_sha = sha256(original)
    if host_sha != EXPECTED_HOST_SHA256:
        raise ValueError("pinned original HERCULES.EXE full SHA-256 mismatch")
    host = PE32(original)
    if (host.machine != 0x14C or host.image_base != 0x400000 or
            host.entry != ENTRY_RVA or len(host.dirs) < 6 or
            host.dirs[5] != (0, 0)):
        raise ValueError("pinned host PE identity/layout changed")
    host_imports = host.imports()
    if host_imports.get("ddraw.dll") != ["DirectDrawCreate"]:
        raise ValueError("DDRAW import set changed: %r" %
                         host_imports.get("ddraw.dll"))
    table_section = host.section_for_rva(TABLE_RVA, TABLE_WORDS * 4)
    table_offset = host.rva_to_offset(TABLE_RVA, TABLE_WORDS * 4)
    table_bytes = host.data[table_offset:table_offset + TABLE_WORDS * 4]
    if hashlib.sha256(table_bytes).hexdigest() != EXPECTED_TABLE_SHA256:
        raise ValueError("host source-table bytes do not match pinned evidence")
    text_section = host.section_for_rva(0x1000)
    if text_section["name"] != ".text" or not text_section["raw_size"]:
        raise ValueError("unexpected .text mapping")

    toolchain_id, toolchain_files = verify_toolchain(root, args.toolchain_dir)
    if toolchain_id != "msvc5_rtm":
        raise ValueError("expected msvc5_rtm toolchain, found " + toolchain_id)
    cl = args.toolchain_dir / "bin" / "cl.exe"
    link = args.toolchain_dir / "bin" / "link.exe"
    if not cl.is_file() or not link.is_file():
        raise FileNotFoundError("pinned VC5 cl.exe/link.exe missing")
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(args.toolchain_dir / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(args.toolchain_dir / "include")
    env["LIB"] = str(args.toolchain_dir / "lib")

    package = Path(__file__).resolve().parent
    src = package / "src"
    abi_dir = root / "renderer_contract"
    sources = probe_sources(root, package)
    for label, path in sources.items():
        if not path.is_file():
            raise FileNotFoundError(label + ": " + str(path))
    commands: list[dict] = []
    helper_obj = build_dir / "hercules_host_abi.obj"
    stub_obj = build_dir / "ddraw_stub.obj"
    smoke_obj = build_dir / "smoke_target.obj"
    controller_obj = build_dir / "debug_controller.obj"
    dll = build_dir / "ddraw.dll"
    implib = build_dir / "ddraw.lib"
    smoke_exe = build_dir / "smoke_target.exe"
    controller = build_dir / "debug_controller.exe"

    run([str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
         "/I" + str(abi_dir), "/Fo" + str(helper_obj),
         str(sources["hercules_host_abi.c"])], build_dir, env, commands)
    run([str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
         "/I" + str(abi_dir), "/I" + str(src), "/Fo" + str(stub_obj),
         str(sources["ddraw_stub.c"])], build_dir, env, commands)
    run([str(link), "/NOLOGO", "/DLL", "/MACHINE:I386",
         "/BASE:0x%08x" % DIAGNOSTIC_DLL_BASE, "/FIXED:NO",
         "/DEF:" + str(sources["ddraw.def"]), "/OUT:" + str(dll),
         "/IMPLIB:" + str(implib), str(stub_obj), str(helper_obj),
         "kernel32.lib"], build_dir, env, commands)

    dll_pe = PE32(dll)
    if dll_pe.image_base != DIAGNOSTIC_DLL_BASE:
        raise ValueError("diagnostic DLL preferred base is not isolated")
    exports = dll_pe.exports()
    if "DirectDrawCreate" not in exports or "ProbeDiagnostic" not in exports:
        raise ValueError("stub DLL is missing required named exports")
    diag_rva = exports["ProbeDiagnostic"]
    diag_section = dll_pe.section_for_rva(diag_rva, 4)
    if (diag_section["characteristics"] & 0x80000000) == 0:
        raise ValueError("ProbeDiagnostic export is not in writable data")
    layout = build_dir / "probe_layout.h"
    write_layout(layout, host, text_section, table_offset, diag_rva)

    run([str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
         "/Fo" + str(smoke_obj), str(sources["smoke_target.c"])],
        build_dir, env, commands)
    run([str(link), "/NOLOGO", "/MACHINE:I386", "/SUBSYSTEM:CONSOLE",
         "/BASE:0x400000", "/OUT:" + str(smoke_exe), str(smoke_obj)],
        build_dir, env, commands)
    smoke_pe = PE32(smoke_exe)
    if smoke_pe.image_base != host.image_base:
        raise ValueError("smoke target did not use the expected base")

    run([str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
         "/I" + str(abi_dir), "/I" + str(src), "/I" + str(build_dir),
         "/Fo" + str(controller_obj), str(sources["debug_controller.c"])],
        build_dir, env, commands)
    run([str(link), "/NOLOGO", "/MACHINE:I386", "/SUBSYSTEM:CONSOLE",
         "/BASE:0x400000", "/OUT:" + str(controller),
         str(controller_obj), "kernel32.lib", "th32.lib"],
        build_dir, env, commands)

    smoke = run([str(controller), "--smoke", str(smoke_exe), str(build_dir),
                 "0x%x" % smoke_pe.entry], build_dir, env, commands,
                timeout=30)
    smoke_fields = parse_kv_lines(smoke.stdout)
    if "STOPPED_BEFORE_ENTRY=1" not in smoke.stdout or \
            "CHILD_TERMINATED=1" not in smoke.stdout or \
            "TERMINATE_PROCESS_ACCEPTED=1" not in smoke.stdout or \
            "HARDWARE_BP_CONFIRMED=1" not in smoke.stdout or \
            "BREAKPOINT_HW_B0=1" not in smoke.stdout or \
            "BREAKPOINT_EXEC_CONFIG=1" not in smoke.stdout or \
            "RESULT=PASS" not in smoke.stdout:
        raise RuntimeError("controller smoke test did not prove bounded stop:\n" +
                           smoke.stdout + smoke.stderr)
    failure_smoke = run([str(controller), "--failure-smoke",
                         str(smoke_exe), str(build_dir)],
                        build_dir, env, commands, timeout=30)
    if "FAILURE_INJECTED_BEFORE_BREAKPOINT=1" not in failure_smoke.stdout or \
            "FAILURE_PATH_NO_ENTRY=1" not in failure_smoke.stdout or \
            "STOPPED_BEFORE_ENTRY=0" not in failure_smoke.stdout or \
            "TERMINATE_PROCESS_ACCEPTED=1" not in failure_smoke.stdout or \
            "CHILD_TERMINATED=1" not in failure_smoke.stdout or \
            "RESULT=PASS" not in failure_smoke.stdout or \
            "EXCEPTION=" in failure_smoke.stdout:
        raise RuntimeError("controller failure-path smoke did not fail closed:\n" +
                           failure_smoke.stdout + failure_smoke.stderr)

    receipt = {
        "schema": "hercules-preentry-debug-probe-build-v1",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "scope": "SOURCE_BUILT_CONTROLLER_AND_DDRAW_STUB_ONLY; host process not launched in build-only mode.",
        "pinned_host": {
            "path": str(original), "size": len(host.data),
            "sha256": host_sha, "machine": "0x%04x" % host.machine,
            "image_base": "0x%08x" % host.image_base,
            "entry_rva": "0x%08x" % host.entry,
            "relocation_directory": ["0x%08x" % host.dirs[5][0],
                                      "0x%08x" % host.dirs[5][1]],
            "ddraw_imports": host_imports["ddraw.dll"],
            "source_table": {
                "rva": "0x%08x" % TABLE_RVA,
                "pointer_rva": "0x%08x" % TABLE_POINTER_RVA,
                "raw_offset": "0x%08x" % table_offset,
                "words": TABLE_WORDS, "signature": "0x%08x" % TABLE_SIGNATURE,
                "sha256": EXPECTED_TABLE_SHA256,
            },
            "text": {
                "rva": "0x%08x" % text_section["rva"],
                "raw_offset": "0x%08x" % text_section["raw_offset"],
                "raw_size": text_section["raw_size"],
            },
        },
        "fixture": {
            "stub_exports": exports,
            "diagnostic_dll_preferred_base": "0x%08x" % dll_pe.image_base,
            "diagnostic_base_note": "Fixture-only base chosen away from engine DLL preferred base 0x10000000; no historical image-layout claim.",
            "diagnostic_rva": "0x%08x" % diag_rva,
            "marker_slot": 89,
            "marker_value": "0x%08x" % SLOT89_MARKER,
            "attach_behavior": "clone the 250-DWORD table with existing helper; change slot 89 only; publish pointer cell; no DirectDrawCreate, COM, GPU, window, or loader-forwarding calls.",
            "directdrawcreate_behavior": "increments diagnostic count, nulls output, returns E_NOTIMPL; no system forwarding.",
        },
        "toolchain": {"id": toolchain_id, "path": str(args.toolchain_dir),
                      "manifest_files_verified": toolchain_files},
        "runtime": {"python": sys.version, "python_executable": sys.executable,
                    "platform": platform.platform(),
                    "windows_version": list(sys.getwindowsversion())},
        "sources_sha256": source_hashes(sources),
        "generated_layout_sha256": sha256(layout),
        "build_commands": commands,
        "build_outputs_sha256": {
            path.relative_to(output).as_posix(): sha256(path)
            for path in (dll, controller, smoke_exe, helper_obj, stub_obj,
                         smoke_obj, controller_obj, implib, layout)
        },
        "smoke_test": {
            "result": "PASS", "stdout": smoke.stdout,
            "stderr": smoke.stderr,
            "breakpoint_registers": {
                "dr0": smoke_fields.get("BREAKPOINT_DR0"),
                "dr6": smoke_fields.get("BREAKPOINT_DR6"),
                "dr7": smoke_fields.get("BREAKPOINT_DR7"),
                "b0_hit": smoke_fields.get("BREAKPOINT_HW_B0"),
                "execution_configuration": smoke_fields.get(
                    "BREAKPOINT_EXEC_CONFIG"),
                "confirmed": smoke_fields.get("HARDWARE_BP_CONFIRMED"),
            },
            "termination": {
                "terminate_process_accepted": smoke_fields.get(
                    "TERMINATE_PROCESS_ACCEPTED"),
                "child_terminated": smoke_fields.get("CHILD_TERMINATED"),
                "exit_process_event_drained": smoke_fields.get(
                    "EXIT_PROCESS_EVENT_DRAINED"),
                "kill_on_exit_mode": smoke_fields.get("KILL_ON_EXIT_MODE"),
            },
            "target_sha256": sha256(smoke_exe),
        },
        "failure_path_smoke": {
            "result": "PASS", "stdout": failure_smoke.stdout,
            "stderr": failure_smoke.stderr,
            "child_entry_reached": False,
            "child_terminated": "CHILD_TERMINATED=1" in failure_smoke.stdout,
        },
        "result": "BUILD_PASS",
        "limit": "The smoke test uses only a source-built target. No Hercules process is launched by this stage.",
    }
    return receipt


def run_target(args: argparse.Namespace, root: Path, original: Path,
               output: Path) -> dict:
    build_receipt_path = output / "build_receipt.json"
    if not build_receipt_path.is_file():
        raise ValueError("run-target requires build_receipt.json from build-only")
    build_receipt = json.loads(build_receipt_path.read_text(encoding="utf-8"))
    if build_receipt.get("result") != "BUILD_PASS" or \
            build_receipt.get("smoke_test", {}).get("result") != "PASS" or \
            build_receipt.get("failure_path_smoke", {}).get("result") != "PASS":
        raise ValueError("source-built controller smoke gate did not pass")
    current_sources = source_hashes(
        probe_sources(root, Path(__file__).resolve().parent))
    if build_receipt.get("sources_sha256") != current_sources:
        expected = build_receipt.get("sources_sha256", {})
        changed = sorted(name for name in set(expected) | set(current_sources)
                         if expected.get(name) != current_sources.get(name))
        raise ValueError("source files changed since build-only: " +
                         ", ".join(changed))
    build_dir = output / "build"
    controller = build_dir / "debug_controller.exe"
    stub = build_dir / "ddraw.dll"
    for relative, expected_hash in build_receipt.get(
            "build_outputs_sha256", {}).items():
        artifact = output / Path(relative)
        if not artifact.is_file() or sha256(artifact) != expected_hash:
            raise ValueError("build output changed since build receipt: " +
                             relative)
    current_host_hash = sha256(original)
    if current_host_hash != EXPECTED_HOST_SHA256:
        raise ValueError("original host SHA changed before isolated copy")
    isolated = output / "isolated"
    if isolated.exists():
        raise ValueError("isolated run directory must not already exist")
    isolated.mkdir()
    isolated_exe = isolated / "HERCULES.EXE"
    isolated_stub = isolated / "ddraw.dll"
    shutil.copyfile(original, isolated_exe)
    shutil.copyfile(stub, isolated_stub)
    isolated_hash = sha256(isolated_exe)
    if isolated_hash != current_host_hash:
        raise ValueError("isolated executable is not byte-identical to pinned host")
    if sha256(original) != EXPECTED_HOST_SHA256:
        raise ValueError("original executable changed during isolated copy")

    toolchain_id, _ = verify_toolchain(root, args.toolchain_dir)
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(args.toolchain_dir / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(args.toolchain_dir / "include")
    env["LIB"] = str(args.toolchain_dir / "lib")
    command = [str(controller), "--host", str(isolated_exe), str(isolated)]
    try:
        result = subprocess.run(command, cwd=str(isolated), env=env,
                                capture_output=True, text=True,
                                errors="replace", timeout=30, check=False)
        controller_result = {
            "exit_code": result.returncode, "stdout": result.stdout,
            "stderr": result.stderr, "timed_out": False,
        }
    except subprocess.TimeoutExpired as error:
        controller_result = {
            "exit_code": None,
            "stdout": error.stdout.decode("utf-8", "replace")
            if isinstance(error.stdout, bytes) else (error.stdout or ""),
            "stderr": error.stderr.decode("utf-8", "replace")
            if isinstance(error.stderr, bytes) else (error.stderr or ""),
            "timed_out": True,
        }
    stdout = controller_result["stdout"]
    controller_fields = parse_kv_lines(stdout)
    controller_occurrences = parse_kv_occurrences(stdout)
    required_fields = {
        "APP_LOCAL_DDRAW_SELECTED": "1",
        "DIAGNOSTIC_VALID": "1",
        "POINTER_PUBLISHED": "1",
        "CLONE_IN_DDRAW_MODULE": "1",
        "SOURCE_TABLE_UNCHANGED": "1",
        "CLONE_UNSELECTED_PRESERVED": "1",
        "EXE_TEXT_UNCHANGED": "1",
        "HARDWARE_BP_CONFIRMED": "1",
        "BREAKPOINT_HW_B0": "1",
        "BREAKPOINT_EXEC_CONFIG": "1",
        "BREAKPOINT_EIP": "0x00411ea0",
        "BREAKPOINT_EXCEPTION_ADDRESS": "0x00411ea0",
        "BREAKPOINT_DR0": "0x00411ea0",
        "BREAKPOINT_DR7": "0x00000001",
        "TERMINATE_PROCESS_ACCEPTED": "1",
        "STOPPED_BEFORE_ENTRY": "1",
        "CHILD_TERMINATED": "1",
        "EXIT_PROCESS_EVENT_DRAINED": "1",
        "DIRECTDRAWCREATE_CALLS": "0",
        "DDRAW_BASE": "0x18000000",
        "ATTACH_STATE": "0x00000002",
        "ATTACH_CALLS": "1",
        "TABLE_POINTER_CELL": "0x004365a0",
        "SOURCE_TABLE": "0x004361b8",
        "SOURCE_SLOT89": "0x0040ac60",
        "CLONE_SLOT89": "0xf17e0089",
        "KILL_ON_EXIT_MODE": "1",
        "RESULT": "PASS",
    }
    mismatched = {}
    for key, expected in required_fields.items():
        actual = controller_occurrences.get(key, [])
        if not actual or any(value != expected for value in actual):
            mismatched[key] = {"actual_occurrences": actual,
                               "expected_each": expected}
    ddraw_paths = controller_occurrences.get("DDRAW_PATH", [])
    expected_path = os.path.normcase(str(isolated_stub))
    path_ok = bool(ddraw_paths) and all(
        os.path.normcase(value) == expected_path for value in ddraw_paths)
    pointer_values = controller_occurrences.get("TABLE_POINTER_CURRENT", [])
    clone_values = controller_occurrences.get("CLONE_TABLE", [])
    pointer_ok = (len(pointer_values) == len(clone_values) and
                  bool(pointer_values) and
                  all(left == right for left, right in
                      zip(pointer_values, clone_values)))
    source_word1_values = controller_occurrences.get("SOURCE_WORD1", [])
    clone_word1_values = controller_occurrences.get("CLONE_WORD1", [])
    source_clone_word1_ok = (
        len(source_word1_values) == len(clone_word1_values) and
        bool(source_word1_values) and
        all(left == right for left, right in
            zip(source_word1_values, clone_word1_values)))
    pass_gates = (controller_result["exit_code"] == 0 and
                  not controller_result["timed_out"] and not mismatched and
                  path_ok and pointer_ok and source_clone_word1_ok)
    receipt = {
        "schema": "hercules-preentry-debug-probe-run-v1",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "build_receipt_sha256": sha256(build_receipt_path),
        "sources_sha256": current_sources,
        "build_outputs_sha256": build_receipt["build_outputs_sha256"],
        "scope": "ISOLATED_BYTE_IDENTICAL_HOST_STOPPED_AT_ENTRY_HARDWARE_BREAKPOINT; no entry/gameplay executed.",
        "original": {"path": str(original), "sha256_before_copy": current_host_hash,
                     "sha256_after_copy": sha256(original)},
        "isolated": {"exe_path": str(isolated_exe),
                     "exe_sha256": isolated_hash,
                     "stub_path": str(isolated_stub),
                     "stub_sha256": sha256(isolated_stub)},
        "toolchain_id": toolchain_id,
        "runtime": {"python": sys.version, "python_executable": sys.executable,
                    "platform": platform.platform(),
                    "windows_version": list(sys.getwindowsversion())},
        "controller_command": command,
        "controller_wall_timeout_seconds": 30,
        "controller_result": controller_result,
        "breakpoint_registers": {
            "dr0": controller_fields.get("BREAKPOINT_DR0"),
            "dr6": controller_fields.get("BREAKPOINT_DR6"),
            "dr7": controller_fields.get("BREAKPOINT_DR7"),
            "b0_hit": controller_fields.get("BREAKPOINT_HW_B0"),
            "execution_configuration": controller_fields.get(
                "BREAKPOINT_EXEC_CONFIG"),
            "confirmed": controller_fields.get("HARDWARE_BP_CONFIRMED"),
        },
        "termination": {
            "terminate_process_accepted": controller_fields.get(
                "TERMINATE_PROCESS_ACCEPTED"),
            "child_terminated": controller_fields.get("CHILD_TERMINATED"),
            "exit_process_event_drained": controller_fields.get(
                "EXIT_PROCESS_EVENT_DRAINED"),
            "kill_on_exit_mode": controller_fields.get("KILL_ON_EXIT_MODE"),
        },
        "exact_field_checks": {
            "required_values": required_fields,
            "mismatched_values": mismatched,
            "app_local_path_matches_isolated_sibling": path_ok,
            "pointer_matches_clone": pointer_ok,
            "clone_word1_matches_source": source_clone_word1_ok,
        },
        "result": "PASS" if pass_gates else "FAIL",
        "falsifier": "Any non-app-local ddraw selection, attach diagnostic mismatch, pointer/source/text mismatch, marker mismatch, pre-entry API call, wrong breakpoint EIP, missing termination, or timeout fails this bounded startup observation.",
        "limits": [
            "This observes one isolated startup on the current Windows loader and environment only.",
            "It does not establish app-local selection on other Windows versions or deployment layouts.",
            "It stops at the PE entrypoint before its first instruction; no host entry code/gameplay runs.",
            "The diagnostic stub does not load/forward system DirectDraw and creates no COM object, HWND, or GPU resource.",
        ],
    }
    (output / "host_run_receipt.json").write_text(
        json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print("RESULT=" + receipt["result"])
    print("HOST_RUN_RECEIPT=" + str(output / "host_run_receipt.json"))
    print(stdout, end="")
    if controller_result["stderr"]:
        print(controller_result["stderr"], file=sys.stderr, end="")
    return receipt


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--build-only", action="store_true",
                      help="build source and run only the harmless smoke target")
    mode.add_argument("--run-target", action="store_true",
                      help="copy the pinned host and stop it before its entry instruction")
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--original-exe", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--toolchain-dir", type=Path, default=PINNED_TOOLCHAIN)
    args = parser.parse_args()
    root = args.repo_root.resolve()
    original = args.original_exe.resolve()
    output_argument = Path(os.path.abspath(args.output_dir))
    output = output_argument.resolve()
    if not (root / "toolchains" / "manifest.json").is_file():
        parser.error("--repo-root must be the renderer checkout with toolchains")
    if not original.is_file():
        parser.error("--original-exe must name the pinned read-only host")
    guard_paths(root, output, output_argument, original, parser)
    args.toolchain_dir = args.toolchain_dir.resolve()
    try:
        if args.build_only:
            receipt = build(args, root, original, output)
            (output / "build_receipt.json").write_text(
                json.dumps(receipt, indent=2, sort_keys=True) + "\n",
                encoding="utf-8")
            print("RESULT=BUILD_PASS")
            print("BUILD_RECEIPT=" + str(output / "build_receipt.json"))
            print("CONTROLLER_SHA256=" + sha256(
                output / "build" / "debug_controller.exe"))
            print("DDRAW_STUB_SHA256=" + sha256(
                output / "build" / "ddraw.dll"))
            print(receipt["smoke_test"]["stdout"], end="")
            return 0
        receipt = run_target(args, root, original, output)
        return 0 if receipt["result"] == "PASS" else 1
    except Exception as error:
        print("ERROR=" + str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
