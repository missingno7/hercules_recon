"""Compile the C89 packed-span model and replay captured original D60C steps."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys


def find_root(start: Path) -> Path:
    for parent in (start.resolve(), *start.resolve().parents):
        if ((parent / "scripts" / "pe.py").is_file() and
                (parent / "renderer_contract" / "tests" / "oracle_common.py").is_file() and
                (parent / "work").is_dir()):
            return parent
    raise RuntimeError("could not locate repository root")


ROOT = find_root(Path(__file__))
DEFAULT_PACKAGE = ROOT / "renderer_contract"
PACKAGE = DEFAULT_PACKAGE
SHARED_WIDTH = ROOT / "renderer_contract" / "hercules_types.h"
EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
BASE = 0x400000
ROUTE = "d60c_rowzero"
SPAN_HASHES = {
    "reciprocal_table_initializer": (0x72A0, 0x72CC,
        "3d59be43f3756224228dfdcc372a3bc59b7a7dbba4bbab8c79dd821cc958014b"),
    "d420_dispatch_and_rowzero_function": (0xD420, 0xDD70,
        "4645c60cf0a357ed4aae074c3487a2755384852d0874e2f12b352b994544e07e"),
    "d60c_rowzero_function": (0xD60C, 0xDD70,
        "2ed9dc4341703ee82f789528b427f6333e6d916bec11148fe2cfecff14119b4a"),
    "d60c_packed_uv_setup_and_loop": (0xDC35, 0xDCA0,
        "b8fe7ac9e77137bcd4cb967df60ed0638403f2f8b0853b2bfd572d76b8a48cb4"),
}
REG_NAMES = ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp")
HOOKS = {
    0xDC8A: "before_texture_read",
    0xDC8D: "after_texture_read",
    0xDC8F: "after_add_ebx_edx",
    0xDC91: "after_adc_cl_dl",
    0xDC92: "after_inc_edi",
    0xDC98: "before_add_eax_ebp",
    0xDC9A: "after_add_eax_ebp",
    0xDC9C: "after_adc_ch_dh",
    0xDC9E: "after_dec_bp_before_jg",
}

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--package", type=Path, default=DEFAULT_PACKAGE,
                    help="source package root (defaults to renderer_contract)")
parser.add_argument("--exe", required=True, type=Path)
parser.add_argument("--original-root", required=True, type=Path)
parser.add_argument("--receipt", required=True, type=Path)
parser.add_argument("--output", required=True, type=Path,
                    help="new or empty directory under ignored work/ for compiler outputs")
parser.add_argument("--cc", type=Path, default=Path(r"C:\msys64\mingw64\bin\gcc.exe"))
parser.add_argument("--cxx", type=Path, default=Path(r"C:\msys64\mingw64\bin\g++.exe"))
parser.add_argument("--vc5", type=Path,
                    default=Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953"))
parser.add_argument("--deps", type=Path,
                    default=Path(r"C:\tools\hercules\renderer-oracle-python"))
parser.add_argument("--analysis-deps", type=Path,
                    default=Path(r"C:\tools\hercules\python"))
ARGS = parser.parse_args()
ARGS.package = ARGS.package.resolve()
PACKAGE = ARGS.package
ARGS.exe = ARGS.exe.resolve()
ARGS.original_root = ARGS.original_root.resolve()
ARGS.receipt = ARGS.receipt.resolve()
ARGS.output = ARGS.output.resolve()
ARGS.cc = ARGS.cc.resolve()
ARGS.cxx = ARGS.cxx.resolve()
ARGS.vc5 = ARGS.vc5.resolve()
ARGS.deps = ARGS.deps.resolve()
ARGS.analysis_deps = ARGS.analysis_deps.resolve()
if ARGS.original_root != ROOT.resolve():
    parser.error("--original-root must be the repository root discovered from this runner")
try:
    PACKAGE.relative_to(ROOT.resolve())
except ValueError as error:
    raise SystemExit("--package must remain inside the repository") from error
if not PACKAGE.is_dir():
    parser.error("--package source directory is missing")
for label, path in (("receipt", ARGS.receipt), ("output", ARGS.output)):
    try:
        path.relative_to((ROOT / "work").resolve())
    except ValueError as error:
        raise SystemExit("--" + label + " must remain under ignored repository work/") from error
if PACKAGE == ARGS.output or PACKAGE in ARGS.output.parents or ARGS.output in PACKAGE.parents:
    parser.error("--output must be separate from the source package")
if ARGS.output.exists() and any(ARGS.output.iterdir()):
    parser.error("--output must be a new or empty directory")
if not ARGS.exe.is_file() or not SHARED_WIDTH.is_file():
    parser.error("the original executable or shared fixed-width header is missing")
if not ARGS.deps.is_dir() or not ARGS.analysis_deps.is_dir():
    parser.error("pinned Unicorn or analysis dependency directory is missing")

sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))
sys.path.insert(0, str(ROOT / "scripts"))
sys.path.insert(0, str(ARGS.analysis_deps))
sys.path.insert(0, str(ARGS.deps))

saved_argv = sys.argv[:]
sys.argv = [str(ROOT / "renderer_contract" / "tests" / "test_original_polygon_edges.py"),
            "--exe", str(ARGS.exe), "--receipt", str(ARGS.receipt),
            "--original-root", str(ROOT), "--deps", str(ARGS.deps),
            "--analysis-deps", str(ARGS.analysis_deps)]
helper_path = ROOT / "renderer_contract" / "tests" / "test_original_polygon_edges.py"
helper_spec = importlib.util.spec_from_file_location("phase7_packed_uv_polygon_helper", helper_path)
if helper_spec is None or helper_spec.loader is None:
    raise RuntimeError("could not load the established original-byte geometry helper")
geom = importlib.util.module_from_spec(helper_spec)
sys.modules[helper_spec.name] = geom
try:
    helper_spec.loader.exec_module(geom)
finally:
    sys.argv = saved_argv

from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDI, UC_X86_REG_EDX, UC_X86_REG_EFLAGS, UC_X86_REG_ESI,
)
from pe import PE
from match import DEFAULT_TOOLCHAIN, verify_toolchain
from capstone import CS_ARCH_X86, CS_MODE_32, Cs


REGISTERS = {
    "eax": UC_X86_REG_EAX, "ebx": UC_X86_REG_EBX,
    "ecx": UC_X86_REG_ECX, "edx": UC_X86_REG_EDX,
    "esi": UC_X86_REG_ESI, "edi": UC_X86_REG_EDI,
    "ebp": UC_X86_REG_EBP, "eflags": UC_X86_REG_EFLAGS,
}
GLOBALS = {
    "u_start": 0xA90990, "v_start": 0xA90994,
    "u_step": 0xA90938, "v_step": 0xA9093C,
    "destination": 0xA90940, "texture_base": 0xA909FC,
    "remaining_pixels": 0xA9096C,
}

PHASE7_SPEC = {
    "name": "phase7_long_span_v_feedback",
    "vertices": [(0, 0), (320, 0), (319, 361), (0, 361)],
    "clip": (0, 1, 320, 2),
    "uv_mode": "fractional_gradient",
    "attributes": [(0, 1 << 16), (0, 0), (0, 219 << 16), (0, 220 << 16)],
    "attribute_domain": "wire_unsigned_u8_q16",
}
PHASE6_SPEC = {
    "name": "phase6_packed_u_carry",
    "vertices": [(315, 1), (324, 2), (323, 8), (314, 7)],
    "clip": (0, 0, 320, 10),
    "uv_mode": "fractional_gradient",
    "attributes": [(250 << 16, 15 << 16), (5 << 16, 25 << 16),
                   (10 << 16, 230 << 16), (240 << 16, 220 << 16)],
    "attribute_domain": "wire_unsigned_u8_q16",
}
PREDICTION = {
    "family": "C89 model of the measured D60C packed span state",
    "prediction": "Initializing from native U/V start and step words, raw destination/texture-base registers, and visible count will reproduce the full DC35..DC87 register packing. Each advance(sampled_texel) will match every captured register boundary through DC9E, including full ECX, U ADD/ADC, texture-byte AL replacement, V ADD/ADC carry, and BP-only decrement, for the 319-sample feedback fixture and the prior 36-sample packed-U fixture.",
    "source_zero_prediction": "A zero texel suppresses the DC96 destination store but does not suppress DC98 ADD EAX,EBP, DC9A ADC CH,DH, or DC9C DEC BP; the C state therefore advances and records store_required=0.",
    "falsifier": "Any setup or per-sample native register mismatch, a zero-source run that does not advance or that stores, or a count endpoint/invalid-input check failure falsifies the model.",
    "scope": "Only the original D60C row-zero route with captured initial words and visible counts 1..320. No geometry-derived initialization, packet traversal, memory dereference, framebuffer rasterization, or CC50/E680 behavior is claimed.",
    "not_claimed": ["pixel coverage or span construction", "host texture or surface lifetime", "CC50 or E680 recurrence", "generic renderer equivalence"],
}
EXPECTED_SHIFT_INSTRUCTIONS = {
    0xDC44: ("shr", "ecx, 0x10"),
    0xDC4C: ("shr", "eax, 0x10"),
    0xDC5F: ("shr", "eax, 0x10"),
}


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def verify_setup_shift_instructions(pe) -> list[dict]:
    start = 0xDC35
    size = 0xDCA0 - start
    instructions = Cs(CS_ARCH_X86, CS_MODE_32).disasm(
        pe.read_rva(start, size), BASE + start)
    decoded = {instruction.address - BASE: (instruction.mnemonic,
                                             instruction.op_str)
               for instruction in instructions}
    facts = []
    for rva, expected in EXPECTED_SHIFT_INSTRUCTIONS.items():
        actual = decoded.get(rva)
        if actual != expected:
            raise RuntimeError("D60C setup shift instruction mismatch at " + hex(rva) +
                               ": expected " + repr(expected) + ", got " + repr(actual))
        facts.append({"rva": hex(rva), "mnemonic": actual[0],
                      "operands": actual[1],
                      "meaning": "logical right shift; zero-fill, not sign-extend"})
    return facts


def read32(uc, address: int) -> int:
    return int.from_bytes(bytes(uc.mem_read(address, 4)), "little")


def capture_native(pe, data_region, spec: dict, stride: int,
                   texel: int, label: str) -> dict:
    geom.SURFACE_STRIDE = stride
    geom.SURFACE_SIZE = stride * geom.SURFACE_HEIGHT
    case = geom.NativeCase(pe, data_region, spec, ROUTE)
    case.uc.mem_write(geom.TEXTURE, bytes([texel]) * 0x10000)
    pending_setup = None
    spans = []
    iterations = []
    current_span = None
    current = None

    def on_code(uc, address, _size, _user_data):
        nonlocal pending_setup, current_span, current
        rva = address - BASE
        if rva == 0xDC35:
            pending_setup = {name: read32(uc, cell) for name, cell in GLOBALS.items()}
        tag = HOOKS.get(rva)
        if tag is None:
            return
        registers = {name: uc.reg_read(register) & 0xFFFFFFFF
                     for name, register in REGISTERS.items()}
        if rva == 0xDC8A:
            if pending_setup is not None:
                current_span = {"setup": pending_setup, "iterations": []}
                spans.append(current_span)
                pending_setup = None
            if current_span is None:
                raise AssertionError(label + ": sample executed without a captured DC35 setup")
            current = {"index": len(iterations), "states": {}}
            iterations.append(current)
            current_span["iterations"].append(current)
        if current is not None:
            current["states"][tag] = registers

    case.uc.hook_add(UC_HOOK_CODE, on_code)
    result = case.run()
    if not spans or len(iterations) != len(case.source_reads):
        raise AssertionError(label + ": original setup/sample trace is incomplete")
    if not case.stopped or not case.reciprocal_init_stopped or case.error is not None:
        raise AssertionError(label + ": original bounded helper run did not finish cleanly")
    if result["route"] != ROUTE or result["entry_rva"] != "0xd420":
        raise AssertionError(label + ": unexpected original route/entry")
    if result["original_code_unchanged"] is not True:
        raise AssertionError(label + ": emulator changed pinned original code")
    if 0xD60C not in case.trace or 0xD549 in case.trace:
        raise AssertionError(label + ": expected generic D60C path was not reached")
    if len(iterations) not in (36, 319):
        raise AssertionError(label + ": visible count does not match the bounded fixture")
    if any(span["setup"]["remaining_pixels"] != len(span["iterations"])
           for span in spans):
        raise AssertionError(label + ": a captured setup count disagrees with its span samples")
    if len(case.surface_writes) != (len(iterations) if texel != 0 else 0):
        raise AssertionError(label + ": destination-store count disagrees with source-zero path")

    write_index = 0
    for index, (iteration, read) in enumerate(zip(iterations, case.source_reads)):
        iteration["source_read"] = read
        states = iteration["states"]
        required = ("before_texture_read", "after_texture_read", "after_add_ebx_edx",
                    "after_adc_cl_dl", "after_inc_edi", "before_add_eax_ebp",
                    "after_add_eax_ebp", "after_adc_ch_dh",
                    "after_dec_bp_before_jg")
        if iteration["index"] != index or any(key not in states for key in required):
            raise AssertionError(label + ": native register hooks are incomplete")
        before = states["before_texture_read"]
        if before["esi"] + before["ecx"] & 0xFFFFFFFF != geom.TEXTURE + read["offset"]:
            raise AssertionError(label + ": full ECX did not address the observed source byte")
        if read["u"] != before["ecx"] & 0xff or read["v"] != (before["ecx"] >> 8) & 0xff:
            raise AssertionError(label + ": native UV byte extraction disagrees with ECX")
        if read["value"] != texel:
            raise AssertionError(label + ": source texture byte changed unexpectedly")
        if texel != 0:
            write = case.surface_writes[write_index]
            if (write["pc"] != "0xdc96" or
                    geom.SURFACE + write["offset"] != states["after_inc_edi"]["edi"]):
                raise AssertionError(label + ": original store address disagrees with incremented EDI")
            write_index += 1
    if write_index != len(case.surface_writes):
        raise AssertionError(label + ": not all original stores were paired with samples")

    return {
        "label": label,
        "texel": texel,
        "count": len(iterations),
        "spans": spans,
        "iterations": iterations,
        "source_reads": case.source_reads,
        "surface_writes": case.surface_writes,
        "route": result["route"],
        "entry_rva": result["entry_rva"],
        "path_markers": result["path_markers"],
        "original_code_unchanged": result["original_code_unchanged"],
        "runtime_precondition": result["runtime_precondition"],
        "unexpected_access": case.error,
    }


def hexword(value: int) -> str:
    return "%08x" % (value & 0xFFFFFFFF)


def trace_input(span: dict, label: str) -> str:
    setup = span["setup"]
    iterations = span["iterations"]
    values = [setup["u_start"], setup["v_start"], setup["u_step"],
              setup["v_step"], setup["destination"], setup["texture_base"],
              setup["remaining_pixels"], len(iterations)]
    values.extend(iterations[0]["states"]["before_texture_read"][name]
                  for name in REG_NAMES)
    rows = ["F " + label + " " + " ".join(hexword(value) for value in values)]
    for iteration in iterations:
        read = iteration["source_read"]
        states = iteration["states"]
        before = states["before_texture_read"]
        after_read = states["after_texture_read"]
        after_u = states["after_add_ebx_edx"]
        after_adc_cl = states["after_adc_cl_dl"]
        after_inc_edi = states["after_inc_edi"]
        after_v_add = states["after_add_eax_ebp"]
        after_adc_ch = states["after_adc_ch_dh"]
        after_dec = states["after_dec_bp_before_jg"]
        source_address = (before["esi"] + before["ecx"]) & 0xFFFFFFFF
        fields = [read["value"], *(before[name] for name in REG_NAMES),
                  source_address, after_read["eax"], after_u["ebx"],
                  after_u["eflags"] & 1, after_adc_cl["ecx"],
                  after_inc_edi["edi"], 1 if read["value"] != 0 else 0,
                  after_v_add["eax"], after_v_add["eflags"] & 1,
                  after_adc_ch["ecx"], after_dec["ebp"]]
        rows.append("S " + " ".join(hexword(value) for value in fields))
    return "\n".join(rows) + "\n"


def run_process(argv, cwd: Path, env: dict, *, stdin: str | None = None) -> dict:
    result = subprocess.run(argv, cwd=str(cwd), env=env, input=stdin,
                            capture_output=True, text=True, errors="replace",
                            timeout=120, shell=False)
    return {"argv": [str(part) for part in argv], "returncode": result.returncode,
            "stdout": result.stdout, "stderr": result.stderr}


def require_command(row: dict, description: str) -> None:
    if row["returncode"] != 0:
        raise RuntimeError(description + " failed: " + row["stdout"] + row["stderr"])


def compile_and_replay(kind: str, build: Path, runs: list[dict], trace: str) -> dict:
    package = PACKAGE
    state_c = package / "hercules_packed_uv.c"
    unit_c = package / "tests" / "test_packed_uv.c"
    replay_c = package / "fixtures" / "packed_uv" / "replay_native_trace.c"
    cpp_test = package / "tests" / "test_cpp_headers.cpp"
    public_sources = sorted((ROOT / "renderer_contract").glob("*.c"))
    public_sources = [source for source in public_sources
                      if source.resolve() != state_c.resolve()]
    include_dirs = [package, ROOT / "renderer_contract"]
    suite = {"compiler_kind": kind, "commands": [], "executables": {}, "native_replay": None}
    if kind == "vc5":
        compiler = ARGS.vc5 / "bin" / "cl.exe"
        env = os.environ.copy()
        for key in ("CL", "_CL_", "LINK", "_LINK_"):
            env.pop(key, None)
        env["PATH"] = str(ARGS.vc5 / "bin") + os.pathsep + env.get("PATH", "")
        env["INCLUDE"] = str(ARGS.vc5 / "include")
        env["LIB"] = str(ARGS.vc5 / "lib")
        env["TMP"] = env["TEMP"] = str(build)
        if not (ARGS.vc5 / "bin" / "cl.exe").is_file():
            raise FileNotFoundError("pinned VC5 compiler is missing")
        for name, source in (("unit", unit_c), ("replay", replay_c)):
            exe = build / (name + ".exe")
            command = [str(compiler), "/nologo", "/W3", "/O2", "/MT", "/Za", "/TC",
                       *["/I" + str(path) for path in include_dirs],
                       "/Fe" + str(exe), str(source), str(state_c)]
            row = run_process(command, build, env)
            suite["commands"].append(row)
            require_command(row, "VC5 " + name + " build")
            suite["executables"][name] = {"path": str(exe),
                                               "sha256": hashlib.sha256(exe.read_bytes()).hexdigest()}
        state_obj = build / "hercules_packed_uv.obj"
        command = [str(compiler), "/nologo", "/W3", "/O2", "/MT", "/Za", "/TC",
                   *["/I" + str(path) for path in include_dirs],
                   "/c", str(state_c), "/Fo" + str(state_obj)]
        row = run_process(command, build, env)
        suite["commands"].append(row)
        require_command(row, "VC5 C89 utility object build")
        support_objects = []
        for source in public_sources:
            obj = build / (source.stem + ".obj")
            command = [str(compiler), "/nologo", "/W3", "/O2", "/MT", "/Za", "/TC",
                       *["/I" + str(path) for path in include_dirs],
                       "/c", str(source), "/Fo" + str(obj)]
            row = run_process(command, build, env)
            suite["commands"].append(row)
            require_command(row, "VC5 C header-suite support build: " + source.name)
            support_objects.append(obj)
        cpp_obj = build / "test_cpp_headers.obj"
        command = [str(compiler), "/nologo", "/W3", "/O2", "/MT", "/Za", "/TP",
                   *["/I" + str(path) for path in include_dirs],
                   "/c", str(cpp_test), "/Fo" + str(cpp_obj)]
        row = run_process(command, build, env)
        suite["commands"].append(row)
        require_command(row, "VC5 merged C++ header test compile")
        cpp_exe = build / "cpp.exe"
        command = [str(compiler), "/nologo", "/Fe" + str(cpp_exe),
                   str(cpp_obj), str(state_obj), *[str(path) for path in support_objects]]
        row = run_process(command, build, env)
        suite["commands"].append(row)
        require_command(row, "VC5 merged C++ header/link test")
        suite["executables"]["cpp"] = {"path": str(cpp_exe),
                                             "sha256": hashlib.sha256(cpp_exe.read_bytes()).hexdigest()}
    elif kind == "gcc":
        cc, cxx = ARGS.cc, ARGS.cxx
        env = os.environ.copy()
        include_flags = [part for path in include_dirs for part in ("-I", str(path))]
        state_obj = build / "hercules_packed_uv.o"
        command = [str(cc), "-std=c89", "-pedantic-errors", "-Wall", "-Wextra",
                   "-Werror", *include_flags, "-c", str(state_c), "-o", str(state_obj)]
        row = run_process(command, build, env)
        suite["commands"].append(row)
        require_command(row, "GCC C89 utility object build")
        for name, source in (("unit", unit_c), ("replay", replay_c)):
            exe = build / (name + (".exe" if os.name == "nt" else ""))
            command = [str(cc), "-std=c89", "-pedantic-errors", "-Wall", "-Wextra",
                       "-Werror", *include_flags, str(source), str(state_obj), "-o", str(exe)]
            row = run_process(command, build, env)
            suite["commands"].append(row)
            require_command(row, "GCC " + name + " build")
            suite["executables"][name] = {"path": str(exe),
                                               "sha256": hashlib.sha256(exe.read_bytes()).hexdigest()}
        support_objects = []
        for source in public_sources:
            obj = build / (source.stem + ".o")
            command = [str(cc), "-std=c89", "-pedantic-errors", "-Wall", "-Wextra",
                       "-Werror", *include_flags, "-c", str(source), "-o", str(obj)]
            row = run_process(command, build, env)
            suite["commands"].append(row)
            require_command(row, "GCC C header-suite support build: " + source.name)
            support_objects.append(obj)
        cpp_obj = build / "test_cpp_headers.o"
        command = [str(cxx), "-std=c++98", "-pedantic-errors", "-Wall", "-Wextra",
                   *include_flags, "-c", str(cpp_test), "-o", str(cpp_obj)]
        row = run_process(command, build, env)
        suite["commands"].append(row)
        require_command(row, "G++ merged C++98 header test compile")
        cpp_exe = build / ("cpp.exe" if os.name == "nt" else "cpp")
        command = [str(cxx), str(cpp_obj), str(state_obj),
                   *[str(path) for path in support_objects], "-o", str(cpp_exe)]
        row = run_process(command, build, env)
        suite["commands"].append(row)
        require_command(row, "G++ merged C++ header/link test")
        suite["executables"]["cpp"] = {"path": str(cpp_exe),
                                             "sha256": hashlib.sha256(cpp_exe.read_bytes()).hexdigest()}
    else:
        raise ValueError("unknown compiler kind")

    for name in ("unit", "cpp"):
        exe = Path(suite["executables"][name]["path"])
        row = run_process([str(exe)], build, env)
        suite["commands"].append(row)
        require_command(row, kind + " " + name + " run")
        suite["executables"][name]["test_output"] = row["stdout"].strip()
    replay = Path(suite["executables"]["replay"]["path"])
    row = run_process([str(replay)], build, env, stdin=trace)
    suite["commands"].append(row)
    require_command(row, kind + " original-trace replay")
    suite["native_replay"] = {"fixture_runs": len(runs), "trace_sha256": sha256_bytes(trace.encode("ascii")),
                              "output": row["stdout"].strip()}
    return suite


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def main() -> int:
    if sha256_bytes(ARGS.exe.read_bytes()) != EXE_SHA256:
        raise RuntimeError("--exe does not match the pinned original image")
    if ARGS.vc5 != DEFAULT_TOOLCHAIN.resolve():
        raise RuntimeError("--vc5 must be the repository's pinned VC5 toolchain")
    toolchain_id = verify_toolchain(ARGS.vc5)
    if not ARGS.cc.is_file() or not ARGS.cxx.is_file():
        raise FileNotFoundError("GCC/G++ compiler paths are missing")
    output = ARGS.output
    output.mkdir(parents=True, exist_ok=True)
    build_vc5 = output / "vc5"
    build_gcc = output / "gcc"
    build_vc5.mkdir()
    build_gcc.mkdir()

    pe = PE(ARGS.exe)
    if pe.image_base != BASE:
        raise RuntimeError("original image base mismatch")
    original_spans = []
    for name, (start, end, expected_hash) in SPAN_HASHES.items():
        data = pe.read_rva(start, end - start)
        actual = sha256_bytes(data)
        if actual != expected_hash:
            raise RuntimeError(name + " original span hash mismatch")
        original_spans.append({"module": "HERCULES.EXE", "name": name,
                               "rva": hex(start), "size": end - start,
                               "sha256": actual})
    setup_shift_instructions = verify_setup_shift_instructions(pe)
    data_region = next((BASE + section["virtual_address"],
                        max(section["virtual_size"], section["raw_size"]))
                       for section in pe.sections if section["name"] == ".data")
    runtime = geom.verify_runtime(geom.unicorn, ARGS.deps)
    suite_specs = [
        ("phase7_texel_01", PHASE7_SPEC, 320, 1),
        ("phase7_texel_ff", PHASE7_SPEC, 320, 255),
        ("phase7_texel_zero", PHASE7_SPEC, 320, 0),
        ("phase6_packed_u_texel_01", PHASE6_SPEC, 512, 1),
        ("phase6_packed_u_texel_ff", PHASE6_SPEC, 512, 255),
    ]
    runs = [capture_native(pe, data_region, spec, stride, texel, label)
            for label, spec, stride, texel in suite_specs]
    trace = "".join(trace_input(span, run["label"] + "_s" + str(index))
                     for run in runs for index, span in enumerate(run["spans"]))
    if len(runs[0]["iterations"]) != 319 or len(runs[1]["iterations"]) != 319:
        raise AssertionError("Phase 7 carry fixture did not capture complete 319-sample runs")
    p7_one = runs[0]["iterations"][192]["states"]
    p7_ff = runs[1]["iterations"][192]["states"]
    if (p7_one["after_texture_read"]["eax"] != 0x00CD4101 or
            p7_ff["after_texture_read"]["eax"] != 0x00CDFFFF or
            p7_one["before_add_eax_ebp"]["ebp"] != 0xFF32007F or
            p7_ff["before_add_eax_ebp"]["ebp"] != 0xFF32007F or
            p7_one["after_add_eax_ebp"]["eflags"] & 1 != 0 or
            p7_ff["after_add_eax_ebp"]["eflags"] & 1 != 1 or
            (runs[0]["source_reads"][193]["v"], runs[1]["source_reads"][193]["v"]) != (0, 1)):
        raise AssertionError("Phase 7 captured carry discriminator changed")
    if len(runs[2]["source_reads"]) != 319 or len(runs[2]["surface_writes"]) != 0:
        raise AssertionError("zero-texel native run did not show read-without-store advancement")
    p6_state = runs[3]["iterations"][3]["states"]
    p6_before = p6_state["before_texture_read"]
    p6_after_add = p6_state["after_add_ebx_edx"]
    p6_after_adc = p6_state["after_adc_cl_dl"]
    if (p6_before["ebx"] != 0x5555F9B5 or p6_before["edx"] != 0xAAAAFDE7 or
            p6_after_add["ebx"] != 0x0000F79C or p6_after_add["eflags"] & 1 != 1 or
            p6_after_adc["ecx"] & 0xff != 151 or
            runs[3]["source_reads"][4]["u"] != 151):
        raise AssertionError("prior 36-sample packed-U witness changed")
    if runs[3]["iterations"] != runs[4]["iterations"]:
        # Registers through V ADD do not necessarily stay identical after AL
        # substitution, so compare only the previously accepted source UV stream.
        stream_one = [[row["u"], row["v"]] for row in runs[3]["source_reads"]]
        stream_ff = [[row["u"], row["v"]] for row in runs[4]["source_reads"]]
        if stream_one != stream_ff:
            raise AssertionError("the earlier 36-sample texture variants no longer have equal UV streams")

    try:
        vc5_suite = compile_and_replay("vc5", build_vc5, runs, trace)
        vc5_block = None
    except OSError as error:
        vc5_suite = {"compiler_kind": "vc5", "status": "blocked_at_process_launch",
                     "error": str(error), "path": str(ARGS.vc5 / "bin" / "cl.exe")}
        vc5_block = str(error)
    gcc_suite = compile_and_replay("gcc", build_gcc, runs, trace)
    suites = [vc5_suite, gcc_suite]
    gcc_version = run_process([str(ARGS.cc), "--version"], build_gcc, os.environ.copy())
    gxx_version = run_process([str(ARGS.cxx), "--version"], build_gcc, os.environ.copy())
    previous_fixture = ROOT / "renderer_contract" / "tests" / "test_original_packed_uv.py"
    source_manifest = []
    for source in sorted(PACKAGE.rglob("*")):
        if source.is_file() and source.suffix.lower() in (".c", ".h", ".cpp", ".py", ".json"):
            source_manifest.append({"path": source.relative_to(PACKAGE).as_posix(),
                                    "sha256": sha256_bytes(source.read_bytes())})
    receipt = {
        "schema_version": 1,
        "status": ("PASS_GCC_VC5_BLOCKED_AT_PROCESS_LAUNCH" if vc5_block
                    else "PASS_C89_REGISTER_TRANSITION_REPLAY"),
        "prediction": PREDICTION,
        "scope": "C89 D60C packed span state replayed every captured sample boundary for two 319-sample texel runs, one 319-sample source-zero run, and both 36-sample packed-U texture runs.",
        "oracle": {"path": str(ARGS.exe), "sha256": EXE_SHA256,
                   "image_base": hex(BASE)},
        "original_spans": original_spans,
        "setup_shift_instructions": setup_shift_instructions,
        "native_fixtures": [{
            "label": run["label"], "texel": run["texel"],
            "route": run["route"], "entry_rva": run["entry_rva"],
            "sample_count": run["count"],
            "source_read_count": len(run["source_reads"]),
            "surface_write_count": len(run["surface_writes"]),
            "span_count": len(run["spans"]),
            "span_sample_counts": [len(span["iterations"]) for span in run["spans"]],
            "span_setup_globals": [span["setup"] for span in run["spans"]],
            "source_uv_sha256": sha256_bytes(json.dumps(
                [[row["u"], row["v"]] for row in run["source_reads"]],
                separators=(",", ":")).encode("ascii")),
            "original_code_unchanged": run["original_code_unchanged"],
            "unexpected_access": run["unexpected_access"],
        } for run in runs],
        "native_discriminators": {
            "phase7_pixel_192": {
                "texel_1_eax_read": hex(p7_one["after_texture_read"]["eax"]),
                "texel_ff_eax_read": hex(p7_ff["after_texture_read"]["eax"]),
                "ebp_both": hex(p7_one["before_add_eax_ebp"]["ebp"]),
                "texel_1_add_carry": p7_one["after_add_eax_ebp"]["eflags"] & 1,
                "texel_ff_add_carry": p7_ff["after_add_eax_ebp"]["eflags"] & 1,
                "next_v_bytes": [runs[0]["source_reads"][193]["v"],
                                 runs[1]["source_reads"][193]["v"]],
            },
            "phase6_pixel_317": {
                "ebx_before": hex(p6_before["ebx"]),
                "edx": hex(p6_before["edx"]),
                "ebx_after_add": hex(p6_after_add["ebx"]),
                "add_carry": p6_after_add["eflags"] & 1,
                "next_cl": p6_after_adc["ecx"] & 0xff,
                "next_sample_u": runs[3]["source_reads"][4]["u"],
            },
            "source_zero": {"reads": len(runs[2]["source_reads"]),
                            "stores": len(runs[2]["surface_writes"]),
                            "next_count": runs[2]["iterations"][0]["states"][
                                "after_dec_bp_before_jg"]["ebp"] & 0xffff},
        },
        "fixed_width_types": {"path": str(SHARED_WIDTH),
                              "sha256": sha256_bytes(SHARED_WIDTH.read_bytes())},
        "sources": {
            "package_files": source_manifest,
            "runner_path": str(Path(__file__).resolve()),
            "runner_sha256": sha256_bytes(Path(__file__).read_bytes()),
            "public_polygon_helper": {"path": str(helper_path),
                                      "sha256": sha256_bytes(helper_path.read_bytes())},
            "prior_36_sample_fixture": {"path": str(previous_fixture),
                                         "sha256": sha256_bytes(previous_fixture.read_bytes())},
        },
        "toolchains": {
            "vc5": {"path": str(ARGS.vc5), "id": toolchain_id,
                    "compiler_sha256": hashlib.sha256((ARGS.vc5 / "bin" / "cl.exe").read_bytes()).hexdigest()},
            "gcc": {"path": str(ARGS.cc), "sha256": hashlib.sha256(ARGS.cc.read_bytes()).hexdigest(),
                    "version": gcc_version["stdout"].splitlines()[0] if gcc_version["stdout"] else gcc_version["stderr"].splitlines()[0]},
            "gxx": {"path": str(ARGS.cxx), "sha256": hashlib.sha256(ARGS.cxx.read_bytes()).hexdigest(),
                    "version": gxx_version["stdout"].splitlines()[0] if gxx_version["stdout"] else gxx_version["stderr"].splitlines()[0]},
        },
        "runtime": {"engine": runtime["engine"],
                    "unicorn_version": runtime["unicorn_version"],
                    "native_dll_sha256": runtime["native_dll_sha256"],
                    "dependency_origin": runtime["dependency_origin"]},
        "c_suites": suites,
        "toolchain_limitation": vc5_block,
        "limits": [
            "The utility advances measured D60C registers only; it does not build spans or rasterize pixels.",
            "Source and destination addresses are raw 32-bit values; no memory is dereferenced by the C utility.",
            "Only the generic D60C row-zero route is covered. CC50 and E680 remain separate contracts.",
            "The 319/36 fixtures validate these states, not every legal register/input combination.",
        ],
        "invocation": {"original_root": str(ROOT), "receipt": str(ARGS.receipt),
                       "output": str(ARGS.output)},
    }
    ARGS.receipt.parent.mkdir(parents=True, exist_ok=True)
    ARGS.receipt.write_text(json.dumps(receipt, indent=2) + "\n",
                            encoding="utf-8", newline="\n")
    print(json.dumps({"status": receipt["status"],
                      "native_fixture_runs": len(runs),
                      "native_steps_replayed": sum(run["count"] for run in runs),
                      "c_suites": [{"compiler_kind": suite["compiler_kind"],
                                    "status": suite.get("status", "passed"),
                                    "native_replay": suite.get("native_replay")} for suite in suites],
                      "receipt": str(ARGS.receipt)}, indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        if ARGS.output.exists():
            ARGS.output.mkdir(parents=True, exist_ok=True)
            failure_receipt = {"status": "FAILED", "error": type(error).__name__ + ": " + str(error)}
            ARGS.receipt.parent.mkdir(parents=True, exist_ok=True)
            ARGS.receipt.write_text(json.dumps(failure_receipt, indent=2) + "\n", encoding="utf-8")
        raise
