"""Build and verify the projection package against pinned original x86 callbacks."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import stat
import struct
import subprocess
import sys
from collections import Counter


SCRIPT_PATH = Path(__file__).resolve()
LANE = SCRIPT_PATH.parent
EXE_DEFAULT = Path(r"D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE")
EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
PREDICTION_TEXT = """# Projection contract experiment prediction

## Prediction

The native B6F0 input callback reads each of three local coordinates as a 16-bit WORD into three 32-bit input cells. B740 then computes three signed 32-bit camera coordinates from the input triple and a 3x3 fixed-point matrix, preserving full DWORD results in A8E71C/A8E728/A8E734 while clamping only camera Z to at least 1. B920 computes a signed focal-over-Z quotient with truncation toward zero, uses it to form projected XY, and stores full 32-bit intermediates in A8E740/A8E746; B720 returns their low WORDs. B970/B990 set the center and focal state. A passive slot-106 wrapper that invokes the existing callback once and reads the five output globals afterward should leave native memory writes, call order, and return registers identical to the unwrapped callback, except for its own private output stores; it should make no extra B840/B810 calls and change no other game state.

## Falsifier

The prediction is falsified by any observed read width, sign/zero-extension, matrix coefficient/layout, arithmetic width/order, divide rounding, clamp location, screen-width conversion, or setter storage differing from the C reference; by any synthetic input where native and C outputs diverge; or by any wrapper capture that adds a native call, changes native writes/order/register return, touches shared game state, or fails to preserve output/input stack ABI. If the read-only wrapper cannot be executed as an actual x86 callback bridge under the available bounded emulator, wrapper behavior remains unresolved rather than inferred from a C fixture.

This is a bounded source-only reconstruction contract, not a FUNCTION_MATCH or renderer acceptance claim.
"""
UNICORN_DEPS = Path(r"C:\tools\hercules\renderer-oracle-python")
ANALYSIS_DEPS = Path(r"C:\tools\hercules\python")
VC5 = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
VC5_CL_SHA256 = "bf9f9c74f756fed96e13f7f9a4273495c7dde0a1fb968e3ef6d760ad6d73dfeb"
VC5_LINK_SHA256 = "279544809e6353c8e622eb88aea94e86b0258f3c4b8a976373bf4d68a41b63e1"
DLL_BASE = 0x10000000
EXE_BASE = 0x00400000
GAME_STATE = 0x00A8E700
GAME_STATE_SIZE = 0x88
STACK_BASE = 0x0D000000
STACK_SIZE = 0x10000
STACK_TOP = STACK_BASE + STACK_SIZE - 0x100
SENTINEL = 0x0E000000
CONFIG = 0x0C000000
INPUT = 0x0C000100
RESULT = 0x0C000200
TRANSLATION_SOURCE = 0x0C000300
OUTPUT_BUFFER = 0x0C000400
INSTRUCTION_BUDGET = 20000

SPAN_SIZES = {
    0xB6F0: 0x24,
    0xB720: 0x1A,
    0xB740: 0xCA,
    0xB810: 0x25,
    0xB840: 0x23,
    0xB880: 0x32,
    0xB8F0: 0x22,
    0xB920: 0x4E,
    0xB970: 0x14,
    0xB990: 0x0A,
}

def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def discover_repo_root() -> Path:
    starts = (SCRIPT_PATH.parent, Path.cwd().resolve())
    for start in starts:
        for candidate in (start, *start.parents):
            if (candidate / "scripts" / "pe.py").is_file():
                return candidate.resolve()
    raise RuntimeError("cannot discover checkout root containing scripts/pe.py")


def is_relative_to(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def reject_reparse_components(path: Path) -> None:
    current = Path(path.anchor)
    reparse_flag = getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400)
    for part in path.parts[1:]:
        current = current / part
        try:
            info = current.lstat()
        except FileNotFoundError:
            continue
        if current.is_symlink() or (getattr(info, "st_file_attributes", 0) & reparse_flag):
            raise ValueError(f"refusing symlink/junction path component: {current}")


def resolve_sources(package: Path) -> dict[str, Path]:
    if (package / "test_projection.c").is_file():
        return {
            "core_h": package / "hercules_projection.h",
            "core_c": package / "hercules_projection.c",
            "test_c": package / "test_projection.c",
            "test_cpp": package / "test_projection_cpp.cpp",
            "bridge": package / "hercules_projection_bridge_diagnostic.c",
        }
    return {
        "core_h": package / "hercules_projection.h",
        "core_c": package / "hercules_projection.c",
        "test_c": package / "tests" / "test_projection.c",
        "test_cpp": package / "tests" / "test_projection_cpp.cpp",
        "bridge": package / "fixtures" / "projection" / "hercules_projection_bridge_diagnostic.c",
    }


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path,
                        help="checkout containing scripts/pe.py; discovered from this script by default")
    parser.add_argument("--package", type=Path,
                        help="source package directory; defaults to renderer_contract or this private flat package")
    parser.add_argument("--exe", type=Path, default=EXE_DEFAULT,
                        help="read-only pinned original HERCULES.EXE")
    parser.add_argument("--output", type=Path,
                        help="ignored build/report directory strictly under checkout work/")
    parser.add_argument("--vc5", type=Path, default=VC5,
                        help="pinned VC5 RTM toolchain directory")
    parser.add_argument("--deps", type=Path, default=UNICORN_DEPS)
    parser.add_argument("--analysis-deps", type=Path, default=ANALYSIS_DEPS)
    args = parser.parse_args()
    args.repo_root = (args.repo_root.resolve() if args.repo_root is not None
                      else discover_repo_root())
    if not (args.repo_root / "scripts" / "pe.py").is_file():
        parser.error("--repo-root must contain scripts/pe.py")
    default_package = LANE if (LANE / "hercules_projection.c").is_file() else args.repo_root / "renderer_contract"
    package_arg = args.package if args.package is not None else default_package
    if not package_arg.is_absolute():
        package_arg = args.repo_root / package_arg
    args.package = Path(os.path.abspath(package_arg))
    allowed_package_roots = (args.repo_root / "renderer_contract", args.repo_root / "work")
    if not any(is_relative_to(args.package, allowed) for allowed in allowed_package_roots):
        parser.error("--package must be under checkout renderer_contract/ or work/")
    try:
        reject_reparse_components(args.package)
    except ValueError as error:
        parser.error(str(error))
    args.exe = args.exe.resolve()
    if args.output is None:
        args.output = args.repo_root / "work" / "renderer_phase14" / "projection" / "public_runner_output"
    elif not args.output.is_absolute():
        args.output = args.repo_root / args.output
    args.output = Path(os.path.abspath(args.output))
    work_root = Path(os.path.abspath(args.repo_root / "work"))
    try:
        relative_output = args.output.relative_to(work_root)
        if not relative_output.parts:
            raise ValueError
    except ValueError:
        parser.error("--output must be strictly inside checkout work/")
    try:
        reject_reparse_components(args.output)
    except ValueError as error:
        parser.error(str(error))
    resolved_output = args.output.resolve(strict=False)
    resolved_work = work_root.resolve(strict=False)
    if not is_relative_to(resolved_output, resolved_work) or resolved_output == resolved_work:
        parser.error("resolved --output must remain strictly inside checkout work/")
    protected_roots = [args.exe.parent.resolve(strict=False),
                       (args.repo_root / "assets").resolve(strict=False)]
    if any(args.output == protected or is_relative_to(args.output, protected) or
           is_relative_to(protected, args.output) for protected in protected_roots):
        parser.error("--output overlaps the original-install or assets tree")
    if not args.exe.is_file() or sha256_file(args.exe) != EXE_SHA256:
        parser.error("--exe is missing or does not match the pinned HERCULES.EXE SHA-256")
    args.vc5 = args.vc5.resolve()
    args.deps = args.deps.resolve()
    args.analysis_deps = args.analysis_deps.resolve()
    for required in (args.vc5 / "bin" / "cl.exe", args.vc5 / "bin" / "link.exe"):
        if not required.is_file():
            parser.error(f"missing pinned compiler/linker: {required}")
    if sha256_file(args.vc5 / "bin" / "cl.exe") != VC5_CL_SHA256:
        parser.error("VC5 compiler hash differs from pinned RTM toolchain")
    if sha256_file(args.vc5 / "bin" / "link.exe") != VC5_LINK_SHA256:
        parser.error("VC5 linker hash differs from pinned RTM toolchain")
    if not args.deps.is_dir() or not args.analysis_deps.is_dir():
        parser.error("pinned Unicorn and analysis dependency directories must exist")
    args.source_files = resolve_sources(args.package)
    missing = [str(path) for path in args.source_files.values() if not path.is_file()]
    if missing:
        parser.error("missing projection package sources: " + ", ".join(missing))
    return args


ARGS = parse_args()
ROOT = ARGS.repo_root
PACKAGE = ARGS.package
SOURCE_FILES = ARGS.source_files
os.environ["HERCULES_PYTHON_DEPS"] = str(ARGS.analysis_deps)
sys.path[:0] = [str(ARGS.deps), str(ROOT / "scripts")]
sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))

from oracle_common import verify_runtime  # noqa: E402
from pe import PE  # noqa: E402
import unicorn  # noqa: E402
from unicorn import (  # noqa: E402
    Uc,
    UC_ARCH_X86,
    UC_HOOK_CODE,
    UC_HOOK_MEM_READ,
    UC_HOOK_MEM_WRITE,
    UC_MODE_32,
    UC_PROT_EXEC,
    UC_PROT_READ,
    UC_PROT_WRITE,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EAX,
    UC_X86_REG_EBP,
    UC_X86_REG_EBX,
    UC_X86_REG_ECX,
    UC_X86_REG_EDI,
    UC_X86_REG_EDX,
    UC_X86_REG_EIP,
    UC_X86_REG_ESI,
    UC_X86_REG_ESP,
    UC_X86_REG_EFLAGS,
)


def run_command(command: list[str], cwd: Path, env: dict) -> dict:
    completed = subprocess.run(command, cwd=cwd, env=env, capture_output=True,
                               text=True, errors="replace", timeout=60)
    result = {
        "argv": command,
        "returncode": completed.returncode,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
    }
    if completed.returncode != 0:
        raise RuntimeError(json.dumps(result, indent=2))
    return result


def build_candidate(output: Path) -> tuple[Path, dict]:
    output.mkdir(parents=True, exist_ok=True)
    temp_dir = output / "tmp"
    temp_dir.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    for key in ("CL", "_CL_", "LINK", "_LINK_"):
        env.pop(key, None)
    env["PATH"] = str(ARGS.vc5 / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(ARGS.vc5 / "include")
    env["LIB"] = str(ARGS.vc5 / "lib")
    env["TEMP"] = str(temp_dir)
    env["TMP"] = str(temp_dir)
    compiler = ARGS.vc5 / "bin" / "cl.exe"
    linker = ARGS.vc5 / "bin" / "link.exe"
    objects = []
    build_commands = []
    include_dirs = [PACKAGE, ROOT / "renderer_contract"]
    private_include_overlay = PACKAGE / "test_include"
    if private_include_overlay.is_dir():
        include_dirs.insert(0, private_include_overlay)
    compile_sources = (SOURCE_FILES["core_c"], SOURCE_FILES["bridge"],
                       SOURCE_FILES["test_c"], SOURCE_FILES["test_cpp"])
    for source in compile_sources:
        obj = output / (source.stem + ".obj")
        command = [str(compiler), "/nologo", "/c", "/O2", "/Za", "/W3",
                   "/DHERCULES_PROJECTION_BUILD_EXPORTS=1",
                   *("/I" + str(directory) for directory in include_dirs),
                   "/Fo" + str(obj), str(source)]
        build_commands.append(run_command(command, output, env))
        if not obj.is_file():
            raise RuntimeError(f"compiler did not create {obj}")
        objects.append(obj)
    dll = output / "hercules_projection_fixture.dll"
    map_path = output / "hercules_projection_fixture.map"
    link_command = [str(linker), "/nologo", "/DLL", "/NOENTRY", "/NODEFAULTLIB",
                    "/INCREMENTAL:NO", "/OPT:NOREF", "/FIXED", f"/BASE:{DLL_BASE:#x}",
                    "/MACHINE:X86", f"/OUT:{dll}", f"/MAP:{map_path}",
                    *(str(obj) for obj in objects)]
    build_commands.append(run_command(link_command, output, env))
    if not dll.is_file():
        raise RuntimeError("linker did not create the x86 candidate DLL")
    manifest = {
        "compiler": str(compiler),
        "compiler_sha256": sha256_file(compiler),
        "compiler_version": "Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 11.00.7022 for 80x86",
        "linker": str(linker),
        "linker_sha256": sha256_file(linker),
        "linker_version": "Microsoft (R) Incremental Linker Version 5.00.7022",
        "flags": {"compile": ["/nologo", "/c", "/O2", "/Za", "/W3"],
                  "link": ["/DLL", "/NOENTRY", "/NODEFAULTLIB", "/INCREMENTAL:NO",
                           "/OPT:NOREF", "/FIXED", f"/BASE:{DLL_BASE:#x}", "/MACHINE:X86"]},
        "commands": build_commands,
        "candidate_dll_sha256": sha256_file(dll),
        "map_file_sha256": sha256_file(map_path),
        "sources": {
            **{str(path.relative_to(ROOT)): sha256_file(path)
               for path in (*SOURCE_FILES.values(), SCRIPT_PATH)},
            "renderer_contract/hercules_host_abi.h": sha256_file(
                ROOT / "renderer_contract" / "hercules_host_abi.h"),
            "renderer_contract/hercules_types.h": sha256_file(
                ROOT / "renderer_contract" / "hercules_types.h"),
        },
    }
    return dll, manifest


def _permissions(characteristics: int) -> int:
    permissions = 0
    if characteristics & 0x40000000:
        permissions |= UC_PROT_READ
    if characteristics & 0x80000000:
        permissions |= UC_PROT_WRITE
    if characteristics & 0x20000000:
        permissions |= UC_PROT_EXEC
    return permissions


def map_pe(uc, image: PE, base: int) -> list[tuple[int, int]]:
    ranges = []
    mapped = []
    for section in image.pe.sections:
        size = max(section.Misc_VirtualSize, section.SizeOfRawData)
        if size == 0:
            continue
        address = base + section.VirtualAddress
        start = address & ~0xfff
        end = (address + size + 0xfff) & ~0xfff
        if any(start < old_end and old_start < end for old_start, old_end in mapped):
            raise RuntimeError(f"overlapping mapped PE section at {address:#x}")
        uc.mem_map(start, end - start, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC)
        raw = image.pe.get_data(section.VirtualAddress, section.SizeOfRawData)
        if raw:
            uc.mem_write(address, raw)
        uc.mem_protect(start, end - start, _permissions(section.Characteristics))
        mapped.append((start, end))
        if section.Characteristics & 0x20000000:
            ranges.append((address, address + size))
    return ranges


def pe_exports(image: PE, base: int) -> dict[str, int]:
    result = {}
    for symbol in image.pe.DIRECTORY_ENTRY_EXPORT.symbols:
        name = symbol.name.decode("ascii", "replace") if symbol.name else f"ordinal_{symbol.ordinal}"
        result[name] = base + symbol.address
        result[name.lstrip("_")] = base + symbol.address
    return result


def find_export(exports: dict[str, int], wanted: str) -> int:
    if wanted in exports:
        return exports[wanted]
    matches = [address for name, address in exports.items() if name.lstrip("_") == wanted]
    if len(set(matches)) != 1:
        raise RuntimeError(f"candidate export not uniquely found: {wanted}; exports={sorted(exports)}")
    return matches[0]


def read_u16(uc, address: int) -> int:
    return struct.unpack("<H", bytes(uc.mem_read(address, 2)))[0]


def read_u32(uc, address: int) -> int:
    return struct.unpack("<I", bytes(uc.mem_read(address, 4)))[0]


def write_u32(uc, address: int, value: int) -> None:
    uc.mem_write(address, struct.pack("<I", value & 0xffffffff))


class Trace:
    def __init__(self, uc, original_ranges: list[tuple[int, int]], candidate_ranges: list[tuple[int, int]]):
        self.uc = uc
        self.original_ranges = original_ranges
        self.candidate_ranges = candidate_ranges
        self.mode = "setup"
        self.original_allowed = [
            (EXE_BASE + start, EXE_BASE + start + size, start)
            for start, size in SPAN_SIZES.items()
        ]
        self.original_pcs = []
        self.reads = []
        self.writes = []
        self.events = []
        self.instruction_count = 0
        uc.hook_add(UC_HOOK_CODE, self.on_code)
        uc.hook_add(UC_HOOK_MEM_READ, self.on_read)
        uc.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def _in_ranges(self, address: int, ranges: list[tuple[int, int]]) -> bool:
        return any(start <= address < end for start, end in ranges)

    def on_code(self, uc, address, size, _user):
        self.instruction_count += 1
        self.events.append(("code", self.mode, address, size))
        if self._in_ranges(address, self.original_ranges):
            allowed = next((start for lo, hi, start in self.original_allowed if lo <= address < hi), None)
            if allowed is None:
                raise RuntimeError(f"unexpected original EXE instruction reached at RVA {address - EXE_BASE:#x}")
            self.original_pcs.append((self.mode, address - EXE_BASE))
        elif not self._in_ranges(address, self.candidate_ranges):
            raise RuntimeError(f"execution left pinned original/candidate code at {address:#x}")

    def on_read(self, uc, access, address, size, value, _user):
        pc = uc.reg_read(UC_X86_REG_EIP)
        self.reads.append((self.mode, pc, address, size, value))
        self.events.append(("read", self.mode, pc, address, size, value))

    def on_write(self, uc, access, address, size, value, _user):
        pc = uc.reg_read(UC_X86_REG_EIP)
        self.writes.append((self.mode, pc, address, size, value))
        self.events.append(("write", self.mode, pc, address, size, value))

    def clear(self, mode: str):
        self.mode = mode
        self.original_pcs.clear()
        self.reads.clear()
        self.writes.clear()
        self.events.clear()
        self.instruction_count = 0


def invoke(uc, trace: Trace, entry: int, args: tuple[int, ...] = (),
           seed_nonvolatile: bool = False) -> dict:
    sp = STACK_TOP
    uc.mem_write(sp, struct.pack("<I", SENTINEL))
    for index, value in enumerate(args):
        write_u32(uc, sp + 4 + index * 4, value)
    if seed_nonvolatile:
        uc.reg_write(UC_X86_REG_EBX, 0x13579bdf)
        uc.reg_write(UC_X86_REG_ESI, 0x2468ace0)
        uc.reg_write(UC_X86_REG_EDI, 0x31415926)
        uc.reg_write(UC_X86_REG_EBP, 0x27182818)
    uc.reg_write(UC_X86_REG_EAX, 0x10203040)
    uc.reg_write(UC_X86_REG_ECX, 0x20304050)
    uc.reg_write(UC_X86_REG_EDX, 0x30405060)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
    uc.reg_write(UC_X86_REG_ESP, sp)
    uc.reg_write(UC_X86_REG_EIP, entry)
    trace.instruction_count = 0
    uc.emu_start(entry, SENTINEL, count=INSTRUCTION_BUDGET)
    if uc.reg_read(UC_X86_REG_EIP) != SENTINEL:
        raise RuntimeError(f"function {entry:#x} did not return to the cdecl sentinel")
    return {
        "eax": uc.reg_read(UC_X86_REG_EAX),
        "ebx": uc.reg_read(UC_X86_REG_EBX),
        "esi": uc.reg_read(UC_X86_REG_ESI),
        "edi": uc.reg_read(UC_X86_REG_EDI),
        "ebp": uc.reg_read(UC_X86_REG_EBP),
        "esp": uc.reg_read(UC_X86_REG_ESP),
        "entry_esp": sp,
        "instruction_count": trace.instruction_count,
    }


def signed32(value: int) -> int:
    value &= 0xffffffff
    return value if value < 0x80000000 else value - 0x100000000


def case_rows() -> list[dict]:
    identity = [4096, 0, 0, 0, 4096, 0, 0, 0, 4096]
    return [
        {"name": "identity_center", "matrix": identity, "translation": [0, 0, 0],
         "center": [160, 120], "focal": 256, "xyz": [3, -4, 64]},
        {"name": "negative_fraction_floor", "matrix": identity, "translation": [0, 0, 0],
         "center": [100, 100], "focal": 256, "xyz": [-3, 5, 7]},
        {"name": "zero_depth_clamps", "matrix": identity, "translation": [0, 0, 0],
         "center": [160, 120], "focal": 256, "xyz": [10, -10, 0]},
        {"name": "negative_depth_clamps", "matrix": identity, "translation": [0, 0, 0],
         "center": [0, 0], "focal": 256, "xyz": [-7, 9, -32768]},
        {"name": "row_major_mixing", "matrix": [4096, 2048, -1024,
             0, 4096, 0, 1024, 0, 4096], "translation": [0, 0, 0],
         "center": [80, 60], "focal": 128, "xyz": [8, -6, 4]},
        {"name": "coefficient_sum_wrap", "matrix": [32767, -32768, 32767,
             -32768, 32767, -32768, 32767, 32767, 32767],
         "translation": [0x7fffffff, -0x80000000, 0x7fffffff],
         "center": [0x7fffffff, -0x80000000], "focal": 0x0001ffff,
         "xyz": [32767, -32768, 32767]},
        {"name": "focal_shift_wrap", "matrix": identity, "translation": [0, 0, 0],
         "center": [12, -13], "focal": 0x12348000, "xyz": [2, -3, 1]},
        {"name": "negative_idiv_trunc_zero", "matrix": identity, "translation": [0, 0, 0],
         "center": [0, 0], "focal": 0x00008001, "xyz": [7, -5, 3]},
        {"name": "screen_word_wrap", "matrix": identity, "translation": [0, 0, 0],
         "center": [0x7fffffff, -0x80000000], "focal": 4096, "xyz": [32767, -32768, 1]},
        {"name": "signed16_input_edges", "matrix": [2048, 0, 0, 0, 4096, 0, 0, 0, 4096],
         "translation": [-1, 7, 3], "center": [-17, 31], "focal": 32,
         "xyz": [-32768, 32767, 1]},
        {"name": "translated_rotation", "matrix": [0, -4096, 0,
             4096, 0, 0, 0, 0, 4096], "translation": [23, -14, 2],
         "center": [64, 48], "focal": 96, "xyz": [11, 5, 32]},
        {"name": "low_word_projection", "matrix": identity, "translation": [0, 0, 0],
         "center": [65535, 65536], "focal": 128, "xyz": [200, -150, 3]},
        {"name": "precision_camera_x_one", "matrix": identity, "translation": [0, 0, 0],
         "center": [160, 120], "focal": 256, "xyz": [1, 0, 1000]},
        {"name": "precision_camera_x_two", "matrix": identity, "translation": [0, 0, 0],
         "center": [160, 120], "focal": 256, "xyz": [2, 0, 1000]},
    ]


def pack_case(case: dict) -> tuple[bytes, bytes, bytes]:
    matrix = tuple(case["matrix"])
    translation = tuple(case["translation"])
    center = tuple(case["center"])
    focal = case["focal"] & 0xffffffff
    config = struct.pack("<9h2x3ii iI".replace(" ", ""),
                         *matrix, *translation, center[0], center[1], focal)
    if len(config) != 44:
        raise AssertionError(f"unexpected C89 projection_config size: {len(config)}")
    input_bytes = struct.pack("<3h", *case["xyz"])
    translation_source = bytearray(32)
    struct.pack_into("<3i", translation_source, 20, *translation)
    return config, input_bytes, bytes(translation_source)


def target_spans(pe: PE) -> list[dict]:
    spans = []
    for rva, size in SPAN_SIZES.items():
        code = pe.read_rva(rva, size)
        spans.append({"rva": hex(rva), "size": size, "sha256": sha256_bytes(code)})
    return spans


def setup_case(uc, trace: Trace, case: dict, config: bytes, input_bytes: bytes,
               translation_source: bytes) -> dict:
    uc.mem_write(GAME_STATE, b"\xa5" * GAME_STATE_SIZE)
    uc.mem_write(CONFIG, config)
    uc.mem_write(INPUT, input_bytes)
    uc.mem_write(TRANSLATION_SOURCE, translation_source)
    trace.clear("native_setup")
    setup_calls = [
        (EXE_BASE + 0xB880, (CONFIG,)),
        (EXE_BASE + 0xB8F0, (TRANSLATION_SOURCE,)),
        (EXE_BASE + 0xB970, (case["center"][0] & 0xffffffff,
                              case["center"][1] & 0xffffffff)),
        (EXE_BASE + 0xB990, (case["focal"] & 0xffffffff,)),
        (EXE_BASE + 0xB6F0, (INPUT,)),
    ]
    results = []
    for entry, args in setup_calls:
        results.append(invoke(uc, trace, entry, args))

    matrix_native = bytes(uc.mem_read(0x00A8E75C, 18))
    matrix_expected = config[:18]
    if matrix_native != matrix_expected:
        raise AssertionError(f"{case['name']}: B880 matrix copy was not 18 contiguous packed bytes")
    translation_native = tuple(struct.unpack("<iii", bytes(uc.mem_read(0x00A8E770, 12))))
    if translation_native != tuple(case["translation"]):
        raise AssertionError(f"{case['name']}: B8F0 translation cells differ from input+0x14")
    if read_u32(uc, 0x00A8E74C) != (case["center"][0] & 0xffffffff) or \
            read_u32(uc, 0x00A8E750) != (case["center"][1] & 0xffffffff):
        raise AssertionError(f"{case['name']}: B970 center setter did not copy both DWORDs")
    if read_u32(uc, 0x00A8E754) != (case["focal"] & 0xffffffff):
        raise AssertionError(f"{case['name']}: B990 focal setter did not copy its DWORD")
    matrix_writes = [(address, size) for mode, pc, address, size, _ in trace.writes
                     if EXE_BASE + 0xB880 <= pc < EXE_BASE + 0xB8B2 and
                     0x00A8E75C <= address < 0x00A8E76E]
    if matrix_writes != [(0x00A8E75C, 4), (0x00A8E760, 4),
                         (0x00A8E764, 4), (0x00A8E768, 4),
                         (0x00A8E76C, 2)]:
        raise AssertionError(f"{case['name']}: B880 copy widths/order differ: {matrix_writes}")
    translation_writes = [(address, size) for mode, pc, address, size, _ in trace.writes
                          if EXE_BASE + 0xB8F0 <= pc < EXE_BASE + 0xB912 and
                          0x00A8E770 <= address < 0x00A8E77C]
    if translation_writes != [(0x00A8E770, 4), (0x00A8E774, 4),
                              (0x00A8E778, 4)]:
        raise AssertionError(f"{case['name']}: B8F0 translation writes differ: {translation_writes}")
    center_writes = [(address, size) for mode, pc, address, size, _ in trace.writes
                     if EXE_BASE + 0xB970 <= pc < EXE_BASE + 0xB984 and
                     0x00A8E74C <= address < 0x00A8E754]
    focal_writes = [(address, size) for mode, pc, address, size, _ in trace.writes
                    if EXE_BASE + 0xB990 <= pc < EXE_BASE + 0xB99A and
                    address == 0x00A8E754]
    if center_writes != [(0x00A8E74C, 4), (0x00A8E750, 4)] or \
            focal_writes != [(0x00A8E754, 4)]:
        raise AssertionError(f"{case['name']}: center/focal setter widths differ")
    input_words = tuple(struct.unpack("<3h", bytes(uc.mem_read(INPUT, 6))))
    if input_words != tuple(case["xyz"]):
        raise AssertionError(f"{case['name']}: input fixture is not three signed WORDs")
    input_reads = [(address - INPUT, size) for mode, pc, address, size, _ in trace.reads
                   if EXE_BASE + 0xB6F0 <= pc < EXE_BASE + 0xB714 and
                   INPUT <= address < INPUT + 6]
    if input_reads != [(0, 2), (2, 2), (4, 2)]:
        raise AssertionError(f"{case['name']}: B6F0 read widths/order differ: {input_reads}")
    b6f0_global_writes = [(address, size) for mode, pc, address, size, _ in trace.writes
                          if EXE_BASE + 0xB6F0 <= pc < EXE_BASE + 0xB714 and
                          0x00A8E700 <= address < 0x00A8E780]
    if b6f0_global_writes != [(0x00A8E708, 2), (0x00A8E70E, 2), (0x00A8E714, 2)]:
        raise AssertionError(f"{case['name']}: B6F0 global writes differ: {b6f0_global_writes}")
    input_return = results[-1]["eax"]
    expected_input_return = (INPUT & 0xffff0000) | (case["xyz"][2] & 0xffff)
    if input_return != expected_input_return:
        raise AssertionError(f"{case['name']}: B6F0 raw EAX differs from its final MOV AX result")
    center_setter = results[2]
    focal_setter = results[3]
    if center_setter["eax"] != (case["center"][0] & 0xffffffff):
        raise AssertionError(f"{case['name']}: B970 raw EAX differs from first center DWORD")
    if focal_setter["eax"] != (case["focal"] & 0xffffffff):
        raise AssertionError(f"{case['name']}: B990 raw EAX differs from focal DWORD")
    return {"setup_calls": len(setup_calls), "native_input_reads": input_reads,
            "native_input_writes": [(hex(address), size) for address, size in b6f0_global_writes],
            "matrix_copy_bytes": len(matrix_native),
            "translation_values": list(translation_native),
            "B6F0_raw_eax": hex(input_return),
            "B970_raw_eax": hex(center_setter["eax"]),
            "B990_raw_eax": hex(focal_setter["eax"])}


def game_writes(trace: Trace) -> list[tuple[int, int, int, int]]:
    return [(pc - EXE_BASE, address, size, value)
            for _, pc, address, size, value in trace.writes
            if EXE_BASE <= pc < EXE_BASE + 0x800000 and
            GAME_STATE <= address < GAME_STATE + GAME_STATE_SIZE]


def run_case(uc, trace: Trace, exports: dict[str, int], case: dict) -> dict:
    config, input_bytes, translation_source = pack_case(case)
    setup_evidence = setup_case(uc, trace, case, config, input_bytes, translation_source)
    uc.mem_write(CONFIG, config)
    uc.mem_write(INPUT, input_bytes)
    uc.mem_write(RESULT, b"\xcc" * 20)
    trace.clear("c_reference")
    invoke(uc, trace, find_export(exports, "hercules_projection_eval"),
           (CONFIG, INPUT, RESULT))
    reference = struct.unpack("<iiiHHi", bytes(uc.mem_read(RESULT, 20)))
    reference_result = {
        "camera_xyz": list(reference[:3]),
        "projected_xy_words": list(reference[3:5]),
        "return_eax": reference[5],
    }

    uc.mem_write(0x00A8E71C, b"\x5a" * 4)
    uc.mem_write(0x00A8E728, b"\x5a" * 4)
    uc.mem_write(0x00A8E734, b"\x5a" * 4)
    uc.mem_write(0x00A8E740, b"\x5a" * 2)
    uc.mem_write(0x00A8E746, b"\x5a" * 2)
    native_before = bytes(uc.mem_read(GAME_STATE, GAME_STATE_SIZE))
    trace.clear("native_b920_baseline")
    native = invoke(uc, trace, EXE_BASE + 0xB920, seed_nonvolatile=True)
    baseline_writes = game_writes(trace)
    native_after = bytes(uc.mem_read(GAME_STATE, GAME_STATE_SIZE))
    native_xyz = [signed32(read_u32(uc, cell)) for cell in
                  (0x00A8E71C, 0x00A8E728, 0x00A8E734)]
    native_xy = [read_u16(uc, 0x00A8E740), read_u16(uc, 0x00A8E746)]
    if native_xyz != reference_result["camera_xyz"]:
        raise AssertionError(f"{case['name']}: C camera XYZ {reference_result['camera_xyz']} != native {native_xyz}")
    if native_xy != reference_result["projected_xy_words"]:
        raise AssertionError(f"{case['name']}: C projected XY {reference_result['projected_xy_words']} != native {native_xy}")
    if signed32(native["eax"]) != reference_result["return_eax"]:
        raise AssertionError(f"{case['name']}: C return EAX differs from native B920")
    if native["esp"] != native["entry_esp"] + 4:
        raise AssertionError(f"{case['name']}: direct B920 did not return cdecl stack to caller")
    if native_before == native_after:
        raise AssertionError(f"{case['name']}: native B920 unexpectedly changed no camera/projection state")
    camera_x_writes = [(address, size) for _, address, size, _ in baseline_writes
                       if address == 0x00A8E71C]
    camera_y_writes = [(address, size) for _, address, size, _ in baseline_writes
                       if address == 0x00A8E728]
    camera_z_writes = [(value, size) for _, address, size, value in baseline_writes
                       if address == 0x00A8E734]
    projected_writes = [(address, size) for _, address, size, _ in baseline_writes
                        if address in (0x00A8E740, 0x00A8E746)]
    if camera_x_writes != [(0x00A8E71C, 4)] or camera_y_writes != [(0x00A8E728, 4)]:
        raise AssertionError(f"{case['name']}: B740 camera X/Y output width/order differs")
    if len(camera_z_writes) not in (1, 2) or any(size != 4 for _, size in camera_z_writes):
        raise AssertionError(f"{case['name']}: B740 camera Z write width/count differs")
    if (len(camera_z_writes) == 2 and signed32(camera_z_writes[-1][0]) != 1) or \
            (len(camera_z_writes) == 1 and signed32(camera_z_writes[0][0]) < 1):
        raise AssertionError(f"{case['name']}: B740 Z clamp was not applied before return")
    if projected_writes != [(0x00A8E740, 2), (0x00A8E746, 2)]:
        raise AssertionError(f"{case['name']}: B920 projected outputs are not ordered WORD writes")

    b740_reads = [(address, size) for _, pc, address, size, _ in trace.reads
                  if EXE_BASE + 0xB740 <= pc < EXE_BASE + 0xB80A]
    matrix_reads = [(address, size) for address, size in b740_reads
                    if 0x00A8E75C <= address < 0x00A8E76E]
    expected_matrix_reads = [
        (0x00A8E75E, 2), (0x00A8E75C, 2), (0x00A8E760, 2),
        (0x00A8E762, 2), (0x00A8E764, 2), (0x00A8E766, 2),
        (0x00A8E76A, 2), (0x00A8E768, 2), (0x00A8E76C, 2),
    ]
    if matrix_reads != expected_matrix_reads:
        raise AssertionError(f"{case['name']}: B740 matrix reads differ: {matrix_reads}")
    translation_reads = [(address, size) for address, size in b740_reads
                         if 0x00A8E770 <= address < 0x00A8E77C]
    if translation_reads != [(0x00A8E770, 4), (0x00A8E774, 4), (0x00A8E778, 4)]:
        raise AssertionError(f"{case['name']}: B740 translation reads differ: {translation_reads}")
    point_input_reads = [(address, size) for address, size in b740_reads
                         if address in (0x00A8E708, 0x00A8E70E, 0x00A8E714)]
    if point_input_reads != [(0x00A8E708, 2), (0x00A8E70E, 2), (0x00A8E714, 2)]:
        raise AssertionError(f"{case['name']}: B740 point input widths/order differ: {point_input_reads}")
    projection_reads = [(address, size) for _, pc, address, size, _ in trace.reads
                        if EXE_BASE + 0xB920 <= pc < EXE_BASE + 0xB96E and
                        address in (0x00A8E71C, 0x00A8E728, 0x00A8E734,
                                    0x00A8E74C, 0x00A8E750, 0x00A8E754)]
    if projection_reads != [(0x00A8E754, 4), (0x00A8E734, 4),
                            (0x00A8E728, 4), (0x00A8E71C, 4),
                            (0x00A8E74C, 4), (0x00A8E750, 4)]:
        raise AssertionError(f"{case['name']}: B920 focal/camera/center read widths/order differ: {projection_reads}")

    uc.mem_write(OUTPUT_BUFFER, b"\x00" * 4)
    trace.clear("native_b720_output")
    screen_output = invoke(uc, trace, EXE_BASE + 0xB720, (OUTPUT_BUFFER,))
    output_xy = list(struct.unpack("<HH", bytes(uc.mem_read(OUTPUT_BUFFER, 4))))
    if output_xy != native_xy:
        raise AssertionError(f"{case['name']}: B720 did not copy projected low WORDs")
    if screen_output["eax"] != OUTPUT_BUFFER:
        raise AssertionError(f"{case['name']}: B720 raw EAX is not its output pointer")
    b720_reads = [(address, size) for _, pc, address, size, _ in trace.reads
                  if EXE_BASE + 0xB720 <= pc < EXE_BASE + 0xB73A and
                  address in (0x00A8E740, 0x00A8E746)]
    if b720_reads != [(0x00A8E740, 2), (0x00A8E746, 2)]:
        raise AssertionError(f"{case['name']}: B720 projected source reads differ: {b720_reads}")

    # Reset only callback output cells so the wrapped native callback sees the
    # same game-state values as the direct baseline fixture.
    uc.mem_write(0x00A8E71C, b"\x5a" * 4)
    uc.mem_write(0x00A8E728, b"\x5a" * 4)
    uc.mem_write(0x00A8E734, b"\x5a" * 4)
    uc.mem_write(0x00A8E740, b"\x5a" * 2)
    uc.mem_write(0x00A8E746, b"\x5a" * 2)
    game_state_before_wrapper = bytes(uc.mem_read(GAME_STATE, GAME_STATE_SIZE))
    trace.clear("compiled_slot106_wrapper")
    wrapped = invoke(uc, trace, find_export(exports, "hercules_projection_diagnostic_capture_slot106"),
                     seed_nonvolatile=True)
    wrapper_events = list(trace.events)
    wrapper_memory_writes = list(trace.writes)
    wrapper_memory_reads = list(trace.reads)
    wrapper_code_trace = list(trace.original_pcs)
    wrapped_writes = game_writes(trace)
    game_state_after_wrapper = bytes(uc.mem_read(GAME_STATE, GAME_STATE_SIZE))
    native_visits = Counter(rva for _, rva in wrapper_code_trace)
    if native_visits[0xB920] != 1 or native_visits[0xB740] != 1:
        raise AssertionError(f"{case['name']}: wrapper callback counts differ: {native_visits}")
    if native_visits[0xB840] or native_visits[0xB810]:
        raise AssertionError(f"{case['name']}: wrapper added B840/B810 native calls")
    if wrapped_writes != baseline_writes:
        raise AssertionError(f"{case['name']}: wrapper native game writes differ from direct baseline")
    if game_state_after_wrapper != native_after:
        raise AssertionError(f"{case['name']}: wrapper game-state result differs from direct B920")
    if wrapped["eax"] != native["eax"]:
        raise AssertionError(f"{case['name']}: wrapper did not preserve raw B920 EAX")
    for state_name in ("ebx", "esi", "edi", "ebp"):
        if wrapped[state_name] != native[state_name]:
            raise AssertionError(f"{case['name']}: wrapper changed callee-saved {state_name}")
    if wrapped["esp"] != wrapped["entry_esp"] + 4 or wrapped["esp"] != native["esp"]:
        raise AssertionError(f"{case['name']}: wrapper changed caller-cleaned stack depth")

    wrapper_output_reads = [(address, size) for mode, pc, address, size, _ in trace.reads
                            if mode == "compiled_slot106_wrapper" and
                            not (EXE_BASE <= pc < EXE_BASE + 0x800000) and
                            address in (0x00A8E71C, 0x00A8E728, 0x00A8E734,
                                        0x00A8E740, 0x00A8E746)]
    expected_wrapper_reads = [(0x00A8E71C, 4), (0x00A8E728, 4),
                              (0x00A8E734, 4), (0x00A8E740, 2),
                              (0x00A8E746, 2)]
    if wrapper_output_reads != expected_wrapper_reads:
        raise AssertionError(f"{case['name']}: wrapper did not perform exactly five ordered output reads: {wrapper_output_reads}")
    original_code_event_indexes = [index for index, event in enumerate(wrapper_events)
                                   if event[0] == "code" and
                                   EXE_BASE <= event[2] < EXE_BASE + 0x800000]
    wrapper_read_event_indexes = [index for index, event in enumerate(wrapper_events)
                                  if event[0] == "read" and
                                  event[2] not in range(EXE_BASE, EXE_BASE + 0x800000) and
                                  event[3] in (0x00A8E71C, 0x00A8E728, 0x00A8E734,
                                               0x00A8E740, 0x00A8E746)]
    if not original_code_event_indexes or not wrapper_read_event_indexes or \
            max(original_code_event_indexes) >= min(wrapper_read_event_indexes):
        raise AssertionError(f"{case['name']}: wrapper read outputs before the native callback returned")
    trace.clear("compiled_capture_data_getter")
    capture_ptr_call = invoke(uc, trace, find_export(exports, "hercules_projection_diagnostic_capture_data"))
    capture_ptr = capture_ptr_call["eax"]
    capture_raw = bytes(uc.mem_read(capture_ptr, 16))
    captured = struct.unpack("<IIIHH", capture_raw)
    captured_xyz = [signed32(value) for value in captured[:3]]
    captured_xy = list(captured[3:])
    if captured_xyz != native_xyz or captured_xy != native_xy:
        raise AssertionError(f"{case['name']}: private capture record does not match native outputs")
    private_capture_writes = [
        {"pc": hex(pc), "address": hex(address), "size": size, "value": hex(value & 0xffffffff)}
        for _, pc, address, size, value in wrapper_memory_writes
        if capture_ptr <= address < capture_ptr + 16
    ]
    private_write_addresses = [(address, size)
                               for _, _, address, size, _ in wrapper_memory_writes
                               if capture_ptr <= address < capture_ptr + 16]
    expected_private_writes = [(capture_ptr, 4), (capture_ptr + 4, 4),
                               (capture_ptr + 8, 4), (capture_ptr + 12, 2),
                               (capture_ptr + 14, 2)]
    if private_write_addresses != expected_private_writes:
        raise AssertionError(f"{case['name']}: wrapper private output write layout/order differs")
    unexpected_writes = [
        (pc, address, size) for _, pc, address, size, _ in wrapper_memory_writes
        if not (GAME_STATE <= address < GAME_STATE + GAME_STATE_SIZE or
                capture_ptr <= address < capture_ptr + 16 or
                STACK_BASE <= address < STACK_BASE + STACK_SIZE)
    ]
    if unexpected_writes:
        raise AssertionError(f"{case['name']}: wrapper wrote outside callback state, private output, and stack: {unexpected_writes}")
    baseline_game_write_detail = [
        {"pc_rva": hex(pc), "address": hex(address), "size": size,
         "value": hex(value & ((1 << (size * 8)) - 1))}
        for pc, address, size, value in baseline_writes
    ]
    wrapped_game_write_detail = [
        {"pc_rva": hex(pc), "address": hex(address), "size": size,
         "value": hex(value & ((1 << (size * 8)) - 1))}
        for pc, address, size, value in wrapped_writes
    ]
    native_call_order = [hex(rva) for _, rva in wrapper_code_trace if rva in (0xB920, 0xB740)]

    z_clamped = case["xyz"][2] <= 0 and native_xyz[2] == 1
    result = {
        "name": case["name"],
        "input_xyz_words": case["xyz"],
        "matrix_words_row_major": case["matrix"],
        "translation_dwords": case["translation"],
        "center_dwords": case["center"],
        "focal_dword": hex(case["focal"] & 0xffffffff),
        "reference_result": reference_result,
        "native_result": {"camera_xyz": native_xyz,
                           "projected_xy_words": native_xy,
                           "screen_xy_output": output_xy,
                           "return_eax": signed32(native["eax"])},
        "wrapped_result": {"camera_xyz": captured_xyz,
                           "projected_xy_words": captured_xy,
                           "return_eax": signed32(wrapped["eax"]),
                           "stack_balanced": wrapped["esp"] == wrapped["entry_esp"] + 4,
                           "callee_saved_preserved": True},
        "native_calls_during_wrapper": {hex(rva): count for rva, count in sorted(native_visits.items())},
        "native_entry_order_during_wrapper": native_call_order,
        "native_read_order": {
            "B740_matrix_words": [{"va": hex(address), "size": size}
                                  for address, size in matrix_reads],
            "B740_point_words": [{"va": hex(address), "size": size}
                                 for address, size in point_input_reads],
            "B740_translation_dwords": [{"va": hex(address), "size": size}
                                        for address, size in translation_reads],
            "B920_focal_camera_center_dwords": [{"va": hex(address), "size": size}
                                                for address, size in projection_reads],
            "B720_screen_words": [{"va": hex(address), "size": size}
                                  for address, size in b720_reads],
        },
        "native_write_order": {"direct_B920": baseline_game_write_detail,
                               "inside_wrapper": wrapped_game_write_detail,
                               "identical": baseline_game_write_detail == wrapped_game_write_detail},
        "native_game_writes_identical_to_direct_baseline": wrapped_writes == baseline_writes,
        "game_state_after_wrapper_equals_direct_baseline": game_state_after_wrapper == native_after,
        "wrapper_post_callback_reads": [{"va": hex(address), "size": size}
                                         for address, size in wrapper_output_reads],
        "wrapper_reads_follow_native_return": True,
        "wrapper_private_capture_writes": private_capture_writes,
        "private_capture_record": {"address": hex(capture_ptr), "size": len(capture_raw),
                                   "layout": ["camera_x:u32", "camera_y:u32", "camera_z:u32",
                                              "projected_x:u16", "projected_y:u16"]},
        "z_clamped_to_one": z_clamped,
        "native_registers": {"direct_eax": hex(native["eax"]),
                              "wrapped_eax": hex(wrapped["eax"]),
                              "direct_esp_after_ret": hex(native["esp"]),
                              "wrapped_esp_after_ret": hex(wrapped["esp"]),
                              "ebx": hex(wrapped["ebx"]), "esi": hex(wrapped["esi"]),
                              "edi": hex(wrapped["edi"]), "ebp": hex(wrapped["ebp"])},
        "setup": setup_evidence,
        "assertions": 44,
    }
    return result


def main() -> int:
    output = ARGS.output
    output.mkdir(parents=True, exist_ok=True)
    dll, build = build_candidate(output)
    exe = PE(ARGS.exe)
    candidate = PE(dll)
    if exe.image_base != EXE_BASE:
        raise RuntimeError(f"unexpected original image base {exe.image_base:#x}")
    if candidate.image_base != DLL_BASE:
        raise RuntimeError(f"unexpected candidate image base {candidate.image_base:#x}")
    if candidate.pe.OPTIONAL_HEADER.DllCharacteristics & 0x40:
        raise RuntimeError("candidate is not fixed-base; unexpected relocation directory")
    if candidate.pe.FILE_HEADER.Machine != 0x14c:
        raise RuntimeError("candidate DLL is not x86")
    if getattr(candidate.pe, "DIRECTORY_ENTRY_IMPORT", []):
        raise RuntimeError("candidate DLL unexpectedly imports external code or data")
    reloc = candidate.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5]
    if reloc.VirtualAddress != 0 or reloc.Size != 0:
        raise RuntimeError("candidate DLL unexpectedly has base relocations")
    exports = pe_exports(candidate, DLL_BASE)
    for name in ("hercules_projection_eval", "hercules_projection_test_all",
                 "hercules_projection_cpp_linkage_probe",
                 "hercules_projection_diagnostic_set_original_slot106",
                 "hercules_projection_diagnostic_capture_slot106",
                 "hercules_projection_diagnostic_capture_data"):
        find_export(exports, name)

    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    original_ranges = map_pe(uc, exe, EXE_BASE)
    candidate_ranges = map_pe(uc, candidate, DLL_BASE)
    uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(CONFIG, 0x1000, UC_PROT_READ | UC_PROT_WRITE)
    trace = Trace(uc, original_ranges, candidate_ranges)
    trace.clear("bridge_setup")
    set_callback = find_export(exports, "hercules_projection_diagnostic_set_original_slot106")
    invoke(uc, trace, set_callback, (EXE_BASE + 0xB920,))
    slot_pointer_writes = [event for event in trace.writes
                           if DLL_BASE <= event[2] < DLL_BASE + 0x100000]
    if not slot_pointer_writes:
        raise AssertionError("compiled wrapper callback pointer was not written by setup function")

    trace.clear("package_expected_vector_tests")
    vector_test = invoke(uc, trace, find_export(exports, "hercules_projection_test_all"))
    if vector_test["eax"] != 0:
        raise AssertionError(f"package's 12 original-oracle expected vectors failed: {vector_test['eax']}")
    trace.clear("package_cpp_linkage_test")
    cpp_test = invoke(uc, trace, find_export(exports, "hercules_projection_cpp_linkage_probe"))
    if cpp_test["eax"] != 1:
        raise AssertionError("C++-compiled caller failed to link/call the C reference through extern-C header")

    cases = [run_case(uc, trace, exports, case) for case in case_rows()]
    precision_cases = {row["name"]: row for row in cases
                       if row["name"] in ("precision_camera_x_one", "precision_camera_x_two")}
    if set(precision_cases) != {"precision_camera_x_one", "precision_camera_x_two"}:
        raise AssertionError("both named precision discriminator cases must execute")
    precision_one = precision_cases["precision_camera_x_one"]["native_result"]
    precision_two = precision_cases["precision_camera_x_two"]["native_result"]
    if precision_one["camera_xyz"][0] == precision_two["camera_xyz"][0]:
        raise AssertionError("precision discriminator did not preserve distinct camera X values")
    if precision_one["projected_xy_words"][0] != precision_two["projected_xy_words"][0]:
        raise AssertionError("precision discriminator did not collide in native screen X WORD")
    if precision_one["projected_xy_words"][0] != 160:
        raise AssertionError("precision discriminator native screen X is not the expected center WORD")
    runtime = verify_runtime(unicorn, ARGS.deps)
    spans = target_spans(exe)
    report = {
        "schema_version": 1,
        "status": "BOUNDED_X86_DIFFERENTIAL_AND_BRIDGE_OBSERVED",
        "scope": "Package C89 reference and diagnostic one-record slot-106 bridge checked with twelve historical reference vectors plus two named precision cases, all co-executed against pinned original B6F0/B740/B920/B720 x86 callbacks in Unicorn.",
        "prediction": PREDICTION_TEXT,
        "prediction_sha256": sha256_bytes(PREDICTION_TEXT.encode("utf-8")),
        "oracle": {"path": str(ARGS.exe), "sha256": sha256_file(ARGS.exe),
                   "image_base": hex(exe.image_base)},
        "toolchain": {"id": "msvc5_rtm", "source": "toolchains/manifest.json",
                      "manifest_path": str(ROOT / "toolchains" / "manifest.json"),
                      "manifest_sha256": sha256_file(ROOT / "toolchains" / "manifest.json"),
                      "repository": "https://github.com/archaic-msvc/msvc500",
                      "commit": "8abf95ce980161ad87b0b02402269cce76988953",
                      **build},
        "runtime": {"engine": runtime["engine"],
                    "unicorn_version": runtime["unicorn_version"],
                    "unicorn_module_path": runtime["unicorn_module_path"],
                    "native_dll_sha256": runtime["native_dll_sha256"],
                    "python_executable": sys.executable,
                    "python_executable_sha256": sha256_file(Path(sys.executable)),
                    "unicorn_python_sha256": sha256_file(Path(unicorn.__file__)),
                    "architecture": "x86 32-bit",
                    "original_sections_mapped_at_0x400000": True,
                    "original_code_patched": False,
                    "original_windows_process_started": False,
                    "candidate_bridge_image_base": hex(DLL_BASE),
                    "candidate_machine": "IMAGE_FILE_MACHINE_I386",
                    "candidate_imports": [],
                    "candidate_relocations": 0,
                    "instruction_budget_per_call": INSTRUCTION_BUDGET},
        "harness": {"package_directory": str(PACKAGE),
                    "path": str(Path(__file__).resolve()),
                    "sha256": sha256_file(Path(__file__).resolve()),
                    "oracle_common_path": str(ROOT / "renderer_contract" / "tests" / "oracle_common.py"),
                    "oracle_common_sha256": sha256_file(ROOT / "renderer_contract" / "tests" / "oracle_common.py"),
                    "pe_helper_path": str(ROOT / "scripts" / "pe.py"),
                    "pe_helper_sha256": sha256_file(ROOT / "scripts" / "pe.py"),
                    "shared_width_header_path": str(ROOT / "renderer_contract" / "hercules_types.h"),
                    "shared_width_header_sha256": sha256_file(ROOT / "renderer_contract" / "hercules_types.h"),
                    "host_abi_header_path": str(ROOT / "renderer_contract" / "hercules_host_abi.h"),
                    "host_abi_header_sha256": sha256_file(ROOT / "renderer_contract" / "hercules_host_abi.h")},
        "callback_slots": {"101": {"target_rva": "0xb6f0",
                                     "physical_signature": "renderer_raw32 (__cdecl *)(const renderer_u16 *xyz_words)",
                                     "args": ["WORD xyz[3]"],
                                     "raw_eax": "upper 16 bits of input pointer with the final input WORD replacing low 16 bits via MOV AX"},
                           "106": {"target_rva": "0xb920",
                                     "physical_signature": "renderer_raw32 (__cdecl *)(void)", "args": [],
                                     "raw_eax": "full 32-bit post-center Y sum; wrapper returns same EAX"},
                           "114": {"target_rva": "0xb720",
                                     "physical_signature": "renderer_raw32 (__cdecl *)(renderer_u16 *xy_words)",
                                     "args": ["WORD xy[2]"],
                                     "raw_eax": "the output pointer"}},
        "matrix_and_projection_contract": {
            "B880_matrix_source": "18 contiguous packed bytes: nine signed WORDs in row-major 3x3 order; target copies four DWORDs plus a final WORD into A8E75C..A8E76D.",
            "B740_matrix": "each signed 16-bit matrix/input multiply and each ADD wrap to low 32 bits; row sum uses arithmetic SAR 12, then adds a DWORD translation with low-32-bit wrap and retains sign-extended low WORD as a DWORD camera coordinate.",
            "B740_row_order": "cameraX=m[0]*x+m[1]*y+m[2]*z+tx; cameraY=m[3]*x+m[4]*y+m[5]*z+ty; cameraZ=m[6]*x+m[7]*y+m[8]*z+tz, with SAR 12 after each three-product row sum and signed-low-WORD retention before the Z clamp.",
            "B8F0_translation_source": "three DWORDs at input+0x14 copied to A8E770/A8E774/A8E778.",
            "B840_translation_alias": "The separately measured B840 snapshot writes camera XYZ into A8E770/A8E774/A8E778, aliasing the translation DWORDs read by B740. An extra B840 call therefore mutates inputs for a later transform and is not a passive observation.",
            "B6F0_input": "exact WORD reads at +0,+2,+4; values are signed only when B740 MOVSX loads the cells.",
            "B740_camera_z": "sign-extended low WORD is stored; values below 1 are overwritten with 1.",
            "B920_reciprocal": "focal is a DWORD; SHL 16 wraps to low 32 bits, CDQ/IDIV performs signed division truncating toward zero; B740 clamp makes denominator >=1 on this path.",
            "B920_projection": "low-32-bit signed products, arithmetic SAR 16 (rounds negative values down), wrapped DWORD center addition, then low WORD stores at A8E740/A8E746.",
            "B720_output": "copies the two projected WORDs unchanged; there is no screen-rectangle clamp in B920/B720.",
            "projection_center_and_focal": "B970 copies two 32-bit center arguments; B990 copies one 32-bit focal argument. Neither setter validates or clamps values.",
            "screen_bounds": "No fixed width/height bounds are applied by these callbacks. Screen values wrap to 16 bits at B920 and B720 preserves those words. Captured camera Z has already been clamped to at least 1; its pre-clamp value is unavailable after B920 returns.",
        },
        "prediction_assessment": {
            "result": "PARTIALLY_FALSIFIED",
            "falsified_details": [
                "B6F0 writes three WORD cells, not three DWORD cells.",
                "B920 stores projected X/Y directly as WORDs at A8E740/A8E746; B720 copies those WORDs unchanged.",
            ],
            "supported_details": [
                "B740 leaves full sign-extended DWORD camera X/Y/Z after its Z clamp.",
                "The slot-106 wrapper invokes B920 once, reads the five live results after return, and adds no B840/B810 call or game-state write.",
            ],
        },
        "native_spans": spans,
        "cases": cases,
        "precision_reprojection_policy": {
            "classification": "future private render policy illustration; not a native equation or viewport modification",
            "native_collision": {
                "case_names": ["precision_camera_x_one", "precision_camera_x_two"],
                "camera_xyz": [precision_one["camera_xyz"], precision_two["camera_xyz"]],
                "screen_xy_words": [precision_one["projected_xy_words"], precision_two["projected_xy_words"]],
                "camera_x_differs": precision_one["camera_xyz"][0] != precision_two["camera_xyz"][0],
                "native_screen_x_words_equal": precision_one["projected_xy_words"][0] == precision_two["projected_xy_words"][0],
            },
            "formula": "private_x = viewport_center_x + effective_focal * camera_x / camera_z; private_y = viewport_center_y + effective_focal * camera_y / camera_z",
            "high_resolution_4_3": {
                "canvas": [640, 480], "viewport_xywh": [0, 0, 640, 480],
                "center": [320, 240], "effective_focal": 512,
                "exact_rational_pixel_xy": [["40064/125", "240"], ["40128/125", "240"]],
            },
            "letterboxed_16_9": {
                "canvas": [640, 480], "viewport_xywh": [0, 60, 640, 360],
                "center": [320, 240], "effective_focal": 384,
                "exact_rational_pixel_xy": [["40048/125", "240"], ["40096/125", "240"]],
                "policy": "vertical-FOV lock to the 360-pixel active viewport, with 60-pixel top/bottom bars",
            },
            "limitations": [
                "The captured camera coordinates have already been narrowed to sign-extended low WORDs.",
                "Camera Z has already been clamped to at least 1 before capture; the pre-clamp value is unavailable.",
                "No wider visibility or unclamped camera data is available from this five-field passive snapshot.",
            ],
        },
        "summary": {"case_count": len(cases),
                    "original_oracle_vector_table_passed": vector_test["eax"] == 0,
                    "cpp_extern_c_linkage_probe_passed": cpp_test["eax"] == 1,
                    "c_reference_matches_native_camera_and_screen_outputs": True,
                    "compiled_slot106_wrapper_cases": len(cases),
                    "wrapper_original_B920_calls_per_case": 1,
                    "wrapper_original_B740_calls_per_case": 1,
                    "wrapper_extra_B840_B810_calls": 0,
                    "wrapper_game_write_sequences_match_direct_B920": True,
                    "wrapper_preserves_raw_EAX_cdecl_stack_and_callee_saved_registers": True,
                    "native_bridge_assertion_count": sum(row["assertions"] for row in cases),
                    "package_vector_groups_passed": 12,
                    "precision_discriminator_cases": 2,
                    "precision_discriminator_assertions": 3,
                    "precision_native_screen_x_collision_proven": True,
                    "cpp_linkage_assertions_passed": 1,
                    "assertion_count": sum(row["assertions"] for row in cases) + 16},
        "proof_boundary": [
            "The package reference is a fixed-width C89 behavioral candidate using renderer_contract/hercules_types.h; it is not reconstructed historical source or a FUNCTION_MATCH.",
            "Config and result views are tool-side joined views, not a contiguous original host structure. The diagnostic bridge is one static record and is not a production capture buffer.",
            "The diagnostic bridge is compiled x86 code executed in the same Unicorn address space as pinned original callback bytes; it proves these fourteen synthetic callback paths only (twelve historical reference vectors and two precision cases).",
            "The two precision cases show distinct retained camera X values but the same native screen X WORD. Higher-resolution rational coordinates are a future private rendering policy example, not a native viewport claim.",
            "B840 is excluded because its observed camera snapshot aliases and overwrites the B740 translation inputs; the bridge makes no extra B840/B810 calls.",
            "This does not install a replacement callback into a running game, validate callback-table publication/lifetime/concurrency, or associate captured samples with 3D packets.",
            "No original Windows process, file write to original bytes, assets change, canonical source, recovery state, or Git state is involved.",
        ],
    }
    report_path = output / "report.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
    markdown = [
        "# Projection package native differential",
        "",
        f"Status: **{report['status']}**.",
        "",
        f"The package C89 reference and diagnostic slot-106 wrapper were checked against {len(cases)} synthetic original-x86 cases (12 historical vectors and two precision cases); the 12-value expected-vector test and C++ linkage probe also passed. Camera XYZ, projected WORD pairs, B720 output pairs, and B920 raw EAX matched. The diagnostic wrapper invoked B920 once (which invoked B740 once), read five result fields after return, made no B840/B810 calls, and produced the same original game-state write sequence and final game state as direct B920. It preserved raw EAX, caller-cleaned ESP, and EBX/ESI/EDI/EBP in these runs.",
        "",
        "The source describes nine packed signed WORD matrix cells in row-major order, three DWORD translations, a minimum camera Z of 1, signed focal division with truncation toward zero, and low-WORD screen output. These callbacks apply no fixed screen rectangle; output wraps at 16 bits.",
        "",
        "This is a bounded Unicorn execution of the original pinned callback bytes and separately compiled candidate C. It is not a live installed wrapper, a packet-association proof, or source/function acceptance.",
        "The precision cases prove that native screen X WORD equality does not imply camera-coordinate equality. The rational 4:3 and 16:9 coordinates are future private reprojection examples from the captured WORD camera values, with low-WORD quantization and Z clamp limitations.",
        "",
        f"Receipt: `{report_path}` (SHA-256 `{sha256_file(report_path)}`).",
        "",
        "## Cases",
        "",
        "| Case | Input XYZ | Camera XYZ | Screen XY words | Clamp |",
        "|---|---:|---:|---:|---|",
    ]
    for row in cases:
        markdown.append(f"| {row['name']} | {row['input_xyz_words']} | {row['native_result']['camera_xyz']} | {row['native_result']['projected_xy_words']} | {row['z_clamped_to_one']} |")
    markdown.extend(["", "## Native spans", "", "| RVA | Size | SHA-256 |", "|---:|---:|---|"])
    for span in spans:
        markdown.append(f"| {span['rva']} | {span['size']:#x} | `{span['sha256']}` |")
    report_md = output / "report.md"
    report_md.write_text("\n".join(markdown) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({"status": report["status"], "cases": len(cases),
                      "assertions": report["summary"]["assertion_count"],
                      "report": str(report_path), "report_sha256": sha256_file(report_path)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
