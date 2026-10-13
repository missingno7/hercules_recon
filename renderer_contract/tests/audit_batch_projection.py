#!/usr/bin/env python3
"""Finite original-x86 B430/B320/B5C0 plus compiled cdecl observer fixture."""
from __future__ import annotations

import argparse
import hashlib
import importlib
import json
import os
from pathlib import Path
import stat
import struct
import subprocess
import sys

ANALYSIS_DEPS = Path(r"C:\tools\hercules\python")
ORACLE_DEPS = Path(r"C:\tools\hercules\renderer-oracle-python")
VC5_ROOT = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
VC5_CL = VC5_ROOT / "bin" / "cl.exe"
VC5_LINK = VC5_ROOT / "bin" / "link.exe"
VC5_CL_SHA256 = "bf9f9c74f756fed96e13f7f9a4273495c7dde0a1fb968e3ef6d760ad6d73dfeb"
VC5_LINK_SHA256 = "279544809e6353c8e622eb88aea94e86b0258f3c4b8a976373bf4d68a41b63e1"
VC5_C1_SHA256 = "e519dfe6f45cc0503b7aac43d125b2f156305269b89f0dc765c40d29627c0943"
VC5_C2_SHA256 = "6d1406af5f62647347cfa7175cd7e108b838d6841a546396f29252198a14d73d"
VC5_MANIFEST_SHA256 = "cc37f4e00434eacec0053d4562a1b2715fda46f8891f8c865c90e809772f0b03"
VC5_PRODUCT_VERSION = "5.00.7022"
VC5_DRIVER_VERSION = "11.00.7022"
EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
PREDICTION_SHA256 = "31a2b984598d5c87dcb0921b3b4dfc73fa47ce7be49ece23fb20f7488a2a70af"
HELPER_PREDICTION_SHA256 = "84e2eb6911f723ac00258a6b6ebad7cb25aa153429e24bd6aaaf56f8190b4045"
SHARED_HEADER_SHA256 = {
    "hercules_types.h": "e957f5f142850a66c3368d4c37913b5c4528d04a4b86e1c9e78e0cc653569b4d",
    "hercules_host_abi.h": "b51d75b165c4800aa9ef5431bdf6f6210ee289d36f5b1c000e1ad1048f52cf5b",
    "hercules_projection.h": "f1dd1e4540d33e47368f0a5e084b26f666343f4fe53edfa2ed65047cb8918106",
}
IMAGE_BASE = 0x400000
HOST_TABLE_RVA = 0x361B8
HOST_TABLE_SIZE = 1000
HOST_TABLE_SHA256 = "52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8"
SPANS = {
    "B430": (0xB430, 0x6A),
    "B320": (0xB320, 0x10F),
    "B5C0": (0xB5C0, 0x47),
}
SLOTS = {102: 0xB430, 107: 0xB320, 117: 0xB5C0}
SLOT_EXPECTED = {
    102: "B430 three signed-WORD XYZ pointer inputs",
    107: "B320 three-record transform and projection",
    117: "B5C0 three-vertex FT4 screen-XY copy",
}

INPUT = 0x10000000
PACKET = 0x11000000
STACK = 0x12000000
SENTINEL = 0x13000000
DLL_BASE = 0x20000000
POINT_PTRS = [INPUT + 0x20, INPUT + 0x28, INPUT + 0x30]
CAMERA_X = 0xA8E71C
CAMERA_Y = 0xA8E728
CAMERA_Z = 0xA8E734
SCREEN_X = 0xA8E740
SCREEN_Y = 0xA8E746
MATRIX = 0xA8E75C
TRANSLATION = 0xA8E770
CENTER = 0xA8E74C
FOCAL = 0xA8E754

ROOT: Path | None = None
REGS = {}
PE = Uc = capstone = verify_runtime = None
UC_ARCH_X86 = UC_HOOK_CODE = UC_HOOK_MEM_READ = UC_HOOK_MEM_WRITE = UC_MODE_32 = None
UC_PROT_EXEC = UC_PROT_READ = UC_PROT_WRITE = None
UcError = None
UC_X86_REG_EAX = UC_X86_REG_EBP = UC_X86_REG_EBX = None
UC_X86_REG_ECX = UC_X86_REG_EDI = UC_X86_REG_EDX = None
UC_X86_REG_EIP = UC_X86_REG_ESI = UC_X86_REG_ESP = None


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    raise AssertionError(message)


def same_path(left: Path, right: Path) -> bool:
    return os.path.normcase(os.path.abspath(str(left))) == os.path.normcase(os.path.abspath(str(right)))


def redirected(path: Path) -> bool:
    if path.is_symlink():
        return True
    try:
        attributes = path.lstat().st_file_attributes
    except (AttributeError, FileNotFoundError):
        return False
    return bool(attributes & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400))


def bootstrap(repo_arg: Path):
    global REGS
    global ROOT, PE, Uc, capstone, verify_runtime
    global UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MODE_32
    global UC_PROT_EXEC, UC_PROT_READ, UC_PROT_WRITE
    global UcError
    global UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX
    global UC_X86_REG_EDI, UC_X86_REG_EDX, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP

    lexical = Path(os.path.abspath(os.path.expanduser(str(repo_arg))))
    resolved = lexical.resolve(strict=False)
    if not same_path(lexical, resolved) or redirected(lexical):
        raise SystemExit("--repo-root must not resolve through a symlink or reparse point")
    if not (resolved / "scripts" / "pe.py").is_file():
        raise SystemExit("--repo-root must contain scripts/pe.py")
    work = lexical / "work"
    if not work.is_dir() or redirected(work) or not same_path(work, work.resolve(strict=False)):
        raise SystemExit("--repo-root/work must exist as a non-redirected directory")
    ROOT = resolved
    os.environ["HERCULES_PYTHON_DEPS"] = str(ANALYSIS_DEPS)
    sys.path[:0] = [str(ORACLE_DEPS), str(ANALYSIS_DEPS),
                    str(ROOT / "renderer_contract" / "tests"), str(ROOT / "scripts")]
    if not ORACLE_DEPS.is_dir() or not ANALYSIS_DEPS.is_dir():
        raise SystemExit("existing pinned Unicorn/analysis dependencies are unavailable")
    importlib.import_module("env")
    PE = importlib.import_module("pe").PE
    capstone = importlib.import_module("capstone")
    u = importlib.import_module("unicorn")
    Uc = u.Uc
    UcError = u.UcError
    for name in ("UC_ARCH_X86", "UC_HOOK_CODE", "UC_HOOK_MEM_READ", "UC_HOOK_MEM_WRITE", "UC_MODE_32",
                 "UC_PROT_EXEC", "UC_PROT_READ", "UC_PROT_WRITE"):
        globals()[name] = getattr(u, name)
    x86 = importlib.import_module("unicorn.x86_const")
    for name in ("UC_X86_REG_EAX", "UC_X86_REG_EBP", "UC_X86_REG_EBX", "UC_X86_REG_ECX",
                 "UC_X86_REG_EDI", "UC_X86_REG_EDX", "UC_X86_REG_EIP", "UC_X86_REG_ESI",
                 "UC_X86_REG_ESP"):
        globals()[name] = getattr(x86, name)
    REGS = {"eax": UC_X86_REG_EAX, "ebx": UC_X86_REG_EBX, "ebp": UC_X86_REG_EBP,
            "esi": UC_X86_REG_ESI, "edi": UC_X86_REG_EDI, "esp": UC_X86_REG_ESP}
    verify_runtime = importlib.import_module("oracle_common").verify_runtime
    return resolved


def path_overlaps(left: Path, right: Path) -> bool:
    left = Path(os.path.abspath(str(left)))
    right = Path(os.path.abspath(str(right)))
    try:
        left.relative_to(right)
        return True
    except ValueError:
        pass
    try:
        right.relative_to(left)
        return True
    except ValueError:
        return False


def check_no_redirect_components(path: Path, stop: Path) -> None:
    current = Path(os.path.abspath(str(path)))
    stop = Path(os.path.abspath(str(stop)))
    try:
        relative = current.relative_to(stop)
    except ValueError as exc:
        raise SystemExit(f"path escapes protected root {stop}: {path}") from exc
    current = stop
    if redirected(current) or not same_path(current, current.resolve(strict=False)):
        raise SystemExit(f"refusing redirected path component: {current}")
    for part in relative.parts:
        current = current / part
        if redirected(current) or not same_path(current, current.resolve(strict=False)):
            raise SystemExit(f"refusing redirected path component: {current}")


def guarded_paths(args) -> tuple[Path, Path, Path, Path, dict[str, Path]]:
    if ROOT is None:
        raise RuntimeError("runner has not been bootstrapped")
    lexical_work = ROOT / "work"
    output = args.output if args.output.is_absolute() else ROOT / args.output
    output = Path(os.path.abspath(str(output)))
    if ".." in Path(str(args.output)).parts:
        raise SystemExit("refusing --output containing a parent-directory component")
    try:
        relative = output.relative_to(lexical_work)
    except ValueError as exc:
        raise SystemExit("--output must stay under the lexical repo-root/work directory") from exc
    if not relative.parts or output.suffix.lower() != ".json":
        raise SystemExit("--output must name a .json file below work/")
    if any(part.lower() == "assets" for part in relative.parts):
        raise SystemExit("refusing --output under assets/")
    if "discs" in [part.lower() for part in relative.parts]:
        raise SystemExit("refusing --output under a discs/ original tree")
    check_no_redirect_components(output.parent, lexical_work)
    if output.exists() or redirected(output):
        raise SystemExit("--output must be a new file; existing artifacts are never overwritten")

    # The dedicated artifact directory must be absent or empty on entry.
    if output.parent.exists() and (not output.parent.is_dir() or any(output.parent.iterdir())):
        raise SystemExit("--output parent must be a new or empty artifact directory")

    resolved_work = lexical_work.resolve(strict=False)
    resolved_output = output.resolve(strict=False)
    try:
        resolved_output.relative_to(resolved_work)
    except ValueError as exc:
        raise SystemExit("resolved --output escapes repo-root/work") from exc
    if any(part.lower() == "assets" for part in resolved_output.relative_to(resolved_work).parts):
        raise SystemExit("refusing resolved --output under assets/")

    exe = Path(os.path.abspath(str(args.exe)))
    if ".." in Path(str(args.exe)).parts:
        raise SystemExit("--exe must not contain parent-directory components")
    check_no_redirect_components(exe, Path(exe.anchor))
    if not exe.is_file() or sha(exe.read_bytes()) != EXE_SHA256:
        raise SystemExit("--exe does not match the pinned original HERCULES.EXE hash")

    shared_package = ROOT / "renderer_contract"
    candidate_package = Path(__file__).resolve().parents[1]
    headers = {name: shared_package / name for name in SHARED_HEADER_SHA256}
    source = candidate_package / "fixtures" / "batch_projection" / "observer.c"
    prediction = candidate_package / "evidence" / "batch_projection_zero_argument_prediction.json"
    helper_prediction = candidate_package / "evidence" / "batch_projection_prediction.json"
    protected_reads = [exe, exe.parent, shared_package, candidate_package,
                       source, prediction, helper_prediction, *headers.values(),
                       Path(__file__).resolve()]
    for protected in protected_reads:
        protected_abs = Path(os.path.abspath(str(protected)))
        if path_overlaps(output, protected_abs) or path_overlaps(output.parent, protected_abs):
            raise SystemExit(f"--output overlaps protected input/read path: {protected_abs}")

    if not source.is_file() or not prediction.is_file() or not helper_prediction.is_file():
        raise SystemExit("observer source or pre-experiment prediction record is missing")
    for name, header in headers.items():
        if not header.is_file() or sha(header.read_bytes()) != SHARED_HEADER_SHA256[name]:
            raise SystemExit(f"shared ABI header missing or changed: {name}")
    if sha(prediction.read_bytes()) != PREDICTION_SHA256:
        raise SystemExit("zero-argument prediction/falsifier hash differs from its pre-experiment pin")
    if sha(helper_prediction.read_bytes()) != HELPER_PREDICTION_SHA256:
        raise SystemExit("preserved helper prediction hash differs from its pre-experiment pin")
    check_no_redirect_components(source, ROOT)
    check_no_redirect_components(prediction, ROOT)
    check_no_redirect_components(helper_prediction, ROOT)
    for header in headers.values():
        check_no_redirect_components(header, ROOT)

    output.parent.mkdir(parents=True, exist_ok=False) if not output.parent.exists() else None
    check_no_redirect_components(output.parent, lexical_work)
    return exe, output, source, shared_package, headers


def compile_observer(source: Path, build_dir: Path, package_include: Path):
    component_hashes = {"cl.exe": sha(VC5_CL.read_bytes()),
                        "c1.dll": sha((VC5_ROOT / "bin" / "c1.dll").read_bytes()),
                        "c2.exe": sha((VC5_ROOT / "bin" / "c2.exe").read_bytes())}
    expected_hashes = {"cl.exe": VC5_CL_SHA256, "c1.dll": VC5_C1_SHA256,
                       "c2.exe": VC5_C2_SHA256}
    if component_hashes != expected_hashes:
        fail(f"the existing pinned VC5 compiler component hashes changed: {component_hashes!r}")
    linker_hash = sha(VC5_LINK.read_bytes())
    if linker_hash != VC5_LINK_SHA256:
        fail(f"the existing pinned VC5 linker hash changed: {linker_hash}")
    if build_dir.exists() or redirected(build_dir):
        fail("refusing to reuse or overwrite an existing observer build directory")
    build_dir.mkdir(parents=False, exist_ok=False)
    if redirected(build_dir) or not same_path(build_dir, build_dir.resolve(strict=False)):
        fail("refusing a redirected VC5 build directory")
    obj = build_dir / "observer.obj"
    asm = build_dir / "observer.asm"
    candidate_path = build_dir / "batch_projection_fixture.dll"
    map_path = build_dir / "batch_projection_fixture.map"
    command = [str(VC5_CL), "/nologo", "/c", "/O2", "/Gd", "/Oy-", "/Za", "/W3",
               f"/I{package_include}", f"/Fo{obj}", f"/Fa{asm}", str(source)]
    link_command = [str(VC5_LINK), "/nologo", "/DLL", "/NOENTRY", "/NODEFAULTLIB",
                    "/INCREMENTAL:NO", "/OPT:NOREF", "/FIXED", f"/BASE:{DLL_BASE:#x}",
                    "/MACHINE:X86", f"/OUT:{candidate_path}", f"/MAP:{map_path}", str(obj)]
    try:
        environment = os.environ.copy()
        for key in ("CL", "_CL_", "LINK", "_LINK_"):
            environment.pop(key, None)
        environment["PATH"] = str(VC5_ROOT / "bin") + os.pathsep + environment.get("PATH", "")
        environment["INCLUDE"] = str(VC5_ROOT / "include")
        environment["LIB"] = str(VC5_ROOT / "lib")
        environment["TMP"] = str(build_dir)
        environment["TEMP"] = str(build_dir)
        completed = subprocess.run(command, cwd=str(ROOT), env=environment, check=False,
                                   capture_output=True, text=True)
        if completed.returncode != 0 or not obj.is_file() or not asm.is_file():
            fail("VC5 observer compilation failed: " + (completed.stdout + completed.stderr).strip())
        linked = subprocess.run(link_command, cwd=str(build_dir), env=environment, check=False,
                                capture_output=True, text=True)
    except OSError as exc:
        fail(f"VC5 compiler/linker could not be launched ({exc}); no fixture pass is claimed")
    compile_output = (completed.stdout + completed.stderr).strip()
    link_output = (linked.stdout + linked.stderr).strip()
    if linked.returncode != 0 or not candidate_path.is_file() or not map_path.is_file():
        fail("VC5 source-built PE link failed: " + link_output)
    post_hashes = {"cl.exe": sha(VC5_CL.read_bytes()),
                   "c1.dll": sha((VC5_ROOT / "bin" / "c1.dll").read_bytes()),
                   "c2.exe": sha((VC5_ROOT / "bin" / "c2.exe").read_bytes()),
                   "link.exe": sha(VC5_LINK.read_bytes())}
    if post_hashes != {**expected_hashes, "link.exe": VC5_LINK_SHA256}:
        fail(f"pinned VC5 compiler/linker component hashes changed during fixture build: {post_hashes!r}")
    obj_bytes = obj.read_bytes()
    return (obj, asm, obj_bytes, candidate_path, map_path, command, link_command,
            {"compile": compile_output, "link": link_output}, post_hashes)


def map_exe(uc, exe):
    size_headers = exe.pe.OPTIONAL_HEADER.SizeOfHeaders
    uc.mem_map(exe.image_base, align_up(size_headers), UC_PROT_READ)
    uc.mem_write(exe.image_base, exe.data[:size_headers])
    mapped = []
    for section in exe.sections:
        size = align_up(max(section["virtual_size"], section["raw_size"]))
        va = exe.image_base + section["virtual_address"]
        chars = section["characteristics"]
        perms = UC_PROT_READ
        if chars & 0x80000000:
            perms |= UC_PROT_WRITE
        if chars & 0x20000000:
            perms |= UC_PROT_EXEC
        uc.mem_map(va, size, perms)
        if section["raw_size"]:
            uc.mem_write(va, exe.read_rva(section["virtual_address"], section["raw_size"]))
        if size > section["raw_size"]:
            uc.mem_write(va + section["raw_size"], b"\0" * (size - section["raw_size"]))
        mapped.append({"name": section["name"], "va": hex(va), "size": size})
    return mapped


def map_candidate(uc, candidate, base: int):
    executable = []
    writable = []
    mapped = []
    for section in candidate.sections:
        size = align_up(max(section["virtual_size"], section["raw_size"]))
        if not size:
            continue
        va = base + section["virtual_address"]
        chars = section["characteristics"]
        perms = UC_PROT_READ
        if chars & 0x80000000:
            perms |= UC_PROT_WRITE
            writable.append((va, va + size))
        if chars & 0x20000000:
            perms |= UC_PROT_EXEC
            executable.append((va, va + size))
        uc.mem_map(va, size, perms)
        if section["raw_size"]:
            uc.mem_write(va, candidate.read_rva(section["virtual_address"], section["raw_size"]))
        if size > section["raw_size"]:
            uc.mem_write(va + section["raw_size"], b"\0" * (size - section["raw_size"]))
        mapped.append({"name": section["name"], "va": hex(va), "size": size,
                       "characteristics": hex(chars)})
    if not executable or not writable:
        fail("source-built candidate PE lacks executable code or writable diagnostic state")
    return executable, writable, mapped


def pe_exports(image, base: int) -> dict[str, int]:
    result = {}
    directory = getattr(image.pe, "DIRECTORY_ENTRY_EXPORT", None)
    if directory is None:
        fail("source-built candidate PE has no exports")
    for symbol in directory.symbols:
        name = symbol.name.decode("ascii", "replace") if symbol.name else f"ordinal_{symbol.ordinal}"
        result[name] = base + symbol.address
        result[name.lstrip("_")] = base + symbol.address
    return result


def find_export(exports: dict[str, int], wanted: str) -> int:
    if wanted in exports:
        return exports[wanted]
    matches = {address for name, address in exports.items() if name.lstrip("_") == wanted}
    if len(matches) != 1:
        fail(f"candidate PE export is not uniquely found: {wanted}")
    return matches.pop()


def align_up(value: int, alignment: int = 0x1000) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def decode_spans(exe):
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    records = {}
    instructions = {}
    for name, (rva, size) in SPANS.items():
        raw = exe.read_rva(rva, size)
        insns = list(md.disasm(raw, exe.image_base + rva))
        consumed = sum(item.size for item in insns)
        if consumed != size or not insns or insns[0].address != exe.image_base + rva:
            fail(f"{name} did not fully decode: consumed {consumed:#x}/{size:#x}")
        if name == "B320" and (insns[-1].mnemonic != "ret" or insns[-1].address != exe.image_base + 0xB42E):
            fail("B320 span is not the pinned RET boundary at RVA B42E")
        if name in ("B430", "B5C0") and insns[-1].mnemonic not in ("ret", "retn"):
            fail(f"{name} span does not end at a RET boundary")
        records[name] = {"rva": hex(rva), "size": size, "sha256": sha(raw),
                         "instruction_count": len(insns),
                         "last_instruction": {"rva": hex(insns[-1].address - exe.image_base),
                                              "mnemonic": insns[-1].mnemonic,
                                              "operands": insns[-1].op_str}}
        instructions[name] = insns
    b320 = instructions["B320"]
    lines = [f"{item.address - exe.image_base:05x}: {item.mnemonic} {item.op_str}" for item in b320]
    must_have = [
        "sar eax, 0xc", "add eax, edx", "movsx eax, ax",
        "sar edx, 0xc", "add edx, eax", "movsx edx, dx",
        "sar eax, 0xc", "movsx edi, ax", "idiv edi", "je 0x40b3f3",
    ]
    for needle in must_have:
        if not any(line.endswith(needle) for line in lines):
            fail(f"B320 disassembly lacks expected instruction: {needle}")
    return records, instructions


def batch_lane_witnesses(instructions):
    expected = {
        0xB322: ("mov", "esi, 0xa8e71c"),
        0xB327: ("mov", "ecx, 0xa8e70e"),
        0xB367: ("mov", "dword ptr [esi], eax"),
        0xB3A3: ("mov", "dword ptr [esi + 0xc], edx"),
        0xB3E1: ("mov", "dword ptr [esi + 0x18], edi"),
        0xB403: ("add", "esi, 4"),
        0xB406: ("mov", "word ptr [ecx + 0x32], dx"),
        0xB414: ("add", "ecx, 2"),
        0xB422: ("mov", "word ptr [ecx + 0x36], ax"),
    }
    decoded = {item.address - IMAGE_BASE: item for item in instructions["B320"]}
    witnesses = {}
    for rva, (mnemonic, operands) in expected.items():
        item = decoded.get(rva)
        if item is None or item.mnemonic != mnemonic or item.op_str.lower() != operands:
            got = None if item is None else (item.mnemonic, item.op_str)
            fail(f"B320 lane-base/stride witness at {rva:#x} changed: {got!r}")
        witnesses[hex(rva)] = {"mnemonic": item.mnemonic, "operands": item.op_str}
    return witnesses


def s16(value: int) -> int:
    value &= 0xFFFF
    return value if value < 0x8000 else value - 0x10000


def u16(value: int) -> int:
    return value & 0xFFFF


def read_words(uc, address: int, count: int) -> list[int]:
    return list(struct.unpack("<" + "H" * count, bytes(uc.mem_read(address, count * 2))))


def read_i32s(uc, address: int, count: int) -> list[int]:
    return list(struct.unpack("<" + "i" * count, bytes(uc.mem_read(address, count * 4))))


REG_SEED = {"eax": 0xA11CE001, "ebx": 0xB22CE002, "ebp": 0xC33CE003,
            "esi": 0xD44CE004, "edi": 0xE55CE005}


def run_route(exe, candidate, candidate_exports: dict[str, int], case: dict, route: str):
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    mapped = map_exe(uc, exe)
    candidate_code_ranges, candidate_data_ranges, mapped_candidate = map_candidate(
        uc, candidate, DLL_BASE)
    for address in (INPUT, PACKET, STACK):
        uc.mem_map(address, 0x1000, UC_PROT_READ | UC_PROT_WRITE)

    allowed = [(start, start + size) for start, size in SPANS.values()]
    code_events = []
    writes = []
    data_reads = []
    idiv_reached = False
    phase = "setup"
    state_pointer = None
    capture_pointer = None
    observer_entry = find_export(candidate_exports, "batch_projection_observe_slot107")

    def on_code(machine, address, size, _user):
        nonlocal idiv_reached
        if any(IMAGE_BASE + start <= address < IMAGE_BASE + end for start, end in allowed):
            rva = address - IMAGE_BASE
            if rva in (0xB430, 0xB320, 0xB5C0):
                slot = {0xB430: 102, 0xB320: 107, 0xB5C0: 117}[rva]
                code_events.append({"kind": "callback_entry", "slot": slot,
                                    "rva": hex(rva), "phase": phase})
            if rva == 0xB3F1:
                idiv_reached = True
            code_events.append({"kind": "native", "rva": hex(rva), "phase": phase})
            return
        if any(start <= address < end for start, end in candidate_code_ranges):
            kind = "fixture_callback_entry" if address == observer_entry else "fixture_pe"
            event = {"kind": kind, "rva": hex(address - DLL_BASE), "phase": phase}
            if kind == "fixture_callback_entry":
                event["name"] = "batch_projection_observe_slot107"
            code_events.append(event)
            return
        fail(f"execution escaped the pinned B430/B320/B5C0 spans and source-built fixture PE at {address:#x}")

    uc.hook_add(UC_HOOK_CODE, on_code)

    def in_range(address: int, size: int, start: int, end: int) -> bool:
        return start <= address and address + size <= end

    def on_read(machine, _access, address, size, _value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP)
        if IMAGE_BASE + 0xB430 <= pc < IMAGE_BASE + 0xB430 + SPANS["B430"][1]:
            current = "B430"
            allowed_reads = [
                (STACK, STACK + 0x1000),
                *[(ptr, ptr + 6) for ptr in POINT_PTRS],
            ]
        elif IMAGE_BASE + 0xB320 <= pc < IMAGE_BASE + 0xB320 + SPANS["B320"][1]:
            current = "B320"
            allowed_reads = [
                (STACK, STACK + 0x1000), (0xA8E708, 0xA8E71A),
                (CAMERA_X, SCREEN_X),
                (CENTER, FOCAL + 4), (MATRIX, MATRIX + 18),
                (TRANSLATION, TRANSLATION + 16),
            ]
        elif IMAGE_BASE + 0xB5C0 <= pc < IMAGE_BASE + 0xB5C0 + SPANS["B5C0"][1]:
            current = "B5C0"
            allowed_reads = [(STACK, STACK + 0x1000), (SCREEN_X, SCREEN_Y + 6),
                             (PACKET, PACKET + 0x40)]
        elif any(start <= pc < end for start, end in candidate_code_ranges):
            current = "observer" if phase == "call_observer" else "fixture_setup"
            allowed_reads = [(STACK, STACK + 0x1000)]
            if current == "observer":
                allowed_reads.extend([(CAMERA_X, CAMERA_Z + 12),
                                      (SCREEN_X, SCREEN_Y + 6)])
                if state_pointer is not None:
                    allowed_reads.append((state_pointer, state_pointer + 4))
        else:
            fail(f"data read from unknown code endpoint {pc:#x}")
        if not any(in_range(address, size, start, end) for start, end in allowed_reads):
            fail(f"unexpected {current} data read at {address:#x} size {size} from PC {pc:#x}")
        data_reads.append({"phase": current, "pc": hex(pc), "address": hex(address), "size": size})

    uc.hook_add(UC_HOOK_MEM_READ, on_read)

    def on_write(machine, _access, address, size, value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP)
        if IMAGE_BASE + 0xB430 <= pc < IMAGE_BASE + 0xB430 + SPANS["B430"][1]:
            current = "B430"
        elif IMAGE_BASE + 0xB320 <= pc < IMAGE_BASE + 0xB320 + SPANS["B320"][1]:
            current = "B320"
        elif IMAGE_BASE + 0xB5C0 <= pc < IMAGE_BASE + 0xB5C0 + SPANS["B5C0"][1]:
            current = "B5C0"
        elif any(start <= pc < end for start, end in candidate_code_ranges):
            current = "observer" if phase == "call_observer" else "fixture_setup"
        else:
            current = phase
        if current == "B430":
            allowed_writes = [(STACK, STACK + 0x1000), (0xA8E708, 0xA8E71A)]
        elif current == "B320":
            allowed_writes = [(STACK, STACK + 0x1000), (CAMERA_X, SCREEN_X),
                              (SCREEN_X, SCREEN_Y + 6)]
        elif current == "B5C0":
            allowed_writes = [(STACK, STACK + 0x1000), (PACKET, PACKET + 0x40)]
        elif current == "observer":
            allowed_writes = [(STACK, STACK + 0x1000)]
            if capture_pointer is not None:
                allowed_writes.append((capture_pointer, capture_pointer + 48))
        elif current == "fixture_setup":
            allowed_writes = [(STACK, STACK + 0x1000)]
            if state_pointer is not None:
                allowed_writes.append((state_pointer, state_pointer + 4))
        else:
            allowed_writes = []
        if not any(in_range(address, size, start, end) for start, end in allowed_writes):
            fail(f"unexpected {current} data write at {address:#x} size {size} from PC {pc:#x}")
        writes.append({"phase": current, "pc": hex(pc), "address": hex(address),
                       "size": size, "value": hex(value & ((1 << (size * 8)) - 1))})

    uc.hook_add(UC_HOOK_MEM_WRITE, on_write)

    # Reset the host scratch/state to finite, explicit input bytes.
    uc.mem_write(0xA8E708, b"\xA5" * 0x4C)
    uc.mem_write(0xA8E75C, struct.pack("<9h", *case["matrix"]))
    uc.mem_write(0xA8E770, struct.pack("<4i", *case["translation"]))
    uc.mem_write(CENTER, struct.pack("<2i", *case["center"]))
    uc.mem_write(FOCAL, struct.pack("<i", case["focal"]))
    uc.mem_write(SCREEN_X, struct.pack("<6H", *case["screen_seed"]))
    uc.mem_write(PACKET, b"\xCC" * 0x40)
    for ptr, point in zip(POINT_PTRS, case["points"]):
        uc.mem_write(ptr, struct.pack("<3h", *point))

    def call_address(entry: int, args: list[int], label: str):
        nonlocal phase
        phase = label
        sp = STACK + 0x700 - 4 * (len(args) + 1)
        uc.mem_write(sp, struct.pack("<" + "I" * (len(args) + 1), SENTINEL, *args))
        uc.reg_write(UC_X86_REG_ESP, sp)
        uc.reg_write(UC_X86_REG_EIP, entry)
        uc.emu_start(entry, SENTINEL, count=20000)
        if uc.reg_read(UC_X86_REG_EIP) != SENTINEL:
            fail(f"call target {entry:#x} did not return to the synthetic sentinel")
        return sp

    def call(rva: int, args: list[int]):
        return call_address(IMAGE_BASE + rva, args, f"call_{rva:04x}")

    # B430 proves the batch source layout as three caller-supplied signed-WORD XYZ pointers.
    call(0xB430, POINT_PTRS)
    input_scratch = {
        "x_words": read_words(uc, 0xA8E708, 3),
        "y_words": read_words(uc, 0xA8E70E, 3),
        "z_words": read_words(uc, 0xA8E714, 3),
    }
    expected_source = {"x_words": [u16(p[0]) for p in case["points"]],
                       "y_words": [u16(p[1]) for p in case["points"]],
                       "z_words": [u16(p[2]) for p in case["points"]]}
    if input_scratch != expected_source:
        fail(f"B430 input SoA does not match its three signed-WORD XYZ pointers: {input_scratch!r}")
    expected_b430_writes = []
    for i, point in enumerate(case["points"]):
        expected_b430_writes.extend([
            (0xA8E708 + 2 * i, 2, u16(point[0])),
            (0xA8E70E + 2 * i, 2, u16(point[1])),
            (0xA8E714 + 2 * i, 2, u16(point[2])),
        ])
    observed_b430_writes = [(int(row["address"], 16), row["size"], int(row["value"], 16))
                            for row in writes if row["phase"] == "B430" and
                            0xA8E708 <= int(row["address"], 16) < 0xA8E71A]
    if observed_b430_writes != expected_b430_writes:
        fail(f"B430 write operands/order differ from the three pointer inputs: {observed_b430_writes!r}")

    matrix_before = bytes(uc.mem_read(MATRIX, 18))
    translation_before = bytes(uc.mem_read(TRANSLATION, 16))
    if route == "observer":
        call_address(find_export(candidate_exports, "batch_projection_diagnostic_state"),
                     [], "fixture_get_state")
        state_pointer = uc.reg_read(UC_X86_REG_EAX)
        if not any(start <= state_pointer and state_pointer + 52 <= end
                   for start, end in candidate_data_ranges):
            fail(f"diagnostic state pointer is outside source-built PE writable data: {state_pointer:#x}")
        capture_pointer = state_pointer + 4
        setter_write_start = len(writes)
        call_address(find_export(candidate_exports, "batch_projection_set_saved_slot107"),
                     [IMAGE_BASE + 0xB320], "fixture_setup")
        setter_writes = writes[setter_write_start:]
        setter_state_writes = [row for row in setter_writes
                               if int(row["address"], 16) == state_pointer]
        if len(setter_state_writes) != 1 or setter_state_writes[0]["size"] != 4 or \
                int(setter_state_writes[0]["value"], 16) != IMAGE_BASE + 0xB320:
            fail(f"fixture saved-target setup did not write the B320 pointer exactly once: {setter_writes!r}")
        if any(not (STACK <= int(row["address"], 16) < STACK + 0x1000 or
                    int(row["address"], 16) == state_pointer) for row in setter_writes):
            fail(f"fixture saved-target setup wrote outside stack and state pointer: {setter_writes!r}")
        route_rva = None
        start = observer_entry
        phase = "call_observer"
        sp = STACK + 0x700 - 4
        uc.mem_write(sp, struct.pack("<I", SENTINEL))
    else:
        route_rva = 0xB320
        start = IMAGE_BASE + route_rva
        phase = "call_B320_direct"
        sp = STACK + 0x700 - 4
        uc.mem_write(sp, struct.pack("<I", SENTINEL))
    uc.reg_write(UC_X86_REG_ESP, sp)
    for name, register in REGS.items():
        if name in REG_SEED:
            uc.reg_write(register, REG_SEED[name])
    start_esp = uc.reg_read(UC_X86_REG_ESP)
    start_regs = {name: uc.reg_read(REGS[name]) for name in REG_SEED}
    uc.reg_write(UC_X86_REG_EIP, start)
    try:
        uc.emu_start(start, SENTINEL, count=50000)
        completed = True
        exception = None
    except UcError as exc:
        completed = False
        exception = {"type": type(exc).__name__, "message": str(exc)}
    if not completed:
        return {"route": route, "completed": False, "exception": exception,
                "exception_pc": hex(uc.reg_read(UC_X86_REG_EIP)),
                "idiv_reached": idiv_reached, "code_events": code_events,
                "writes": writes, "mapped_original_sections": mapped,
                "mapped_fixture_sections": mapped_candidate}
    if uc.reg_read(UC_X86_REG_EIP) != SENTINEL:
        fail(f"{route} did not return to the synthetic sentinel")
    end_regs = {name: uc.reg_read(REGS[name]) for name in REG_SEED}
    end_esp = uc.reg_read(UC_X86_REG_ESP)
    raw_eax = uc.reg_read(UC_X86_REG_EAX)
    if end_esp != start_esp + 4:
        fail(f"{route} changed caller-visible cdecl ESP: {start_esp:#x} -> {end_esp:#x}")
    for name in ("ebx", "ebp", "esi", "edi"):
        if end_regs[name] != start_regs[name]:
            fail(f"{route} did not preserve cdecl callee-saved {name.upper()}")
    if bytes(uc.mem_read(MATRIX, 18)) != matrix_before:
        fail(f"{route} modified the native 3x3 matrix")
    if bytes(uc.mem_read(TRANSLATION, 16)) != translation_before:
        fail(f"{route} modified native translation")

    camera = [[read_i32s(uc, CAMERA_X + i * 4, 1)[0],
               read_i32s(uc, CAMERA_Y + i * 4, 1)[0],
               read_i32s(uc, CAMERA_Z + i * 4, 1)[0]] for i in range(3)]
    screen_x_words = read_words(uc, SCREEN_X, 3)
    screen_y_words = read_words(uc, SCREEN_Y, 3)
    screen = [[s16(screen_x_words[i]), s16(screen_y_words[i])] for i in range(3)]
    # B5C0 consumes the three X/Y pairs into the packet after any observer copy.
    call(0xB5C0, [PACKET])
    packet_words = [read_words(uc, PACKET + offset, 1)[0]
                    for offset in (8, 10, 16, 18, 24, 26)]
    expected_packet_writes = [(PACKET + offset, 2, u16(value))
                              for offset, value in zip((8, 10, 16, 18, 24, 26),
                                                       [v for point in screen for v in point])]
    observed_packet_writes = [(int(row["address"], 16), row["size"], int(row["value"], 16))
                              for row in writes if row["phase"] == "B5C0" and
                              PACKET <= int(row["address"], 16) < PACKET + 0x40]
    if observed_packet_writes != expected_packet_writes:
        fail(f"B5C0 packet write order differs from the three screen pairs: {observed_packet_writes!r}")
    b320_writes = [row for row in writes if row["phase"] == "B320" and
                   ((CAMERA_X <= int(row["address"], 16) < 0xA8E740) or
                    (SCREEN_X <= int(row["address"], 16) < SCREEN_Y + 6))]
    b430_writes = [row for row in writes if row["phase"] == "B430" and
                   0xA8E708 <= int(row["address"], 16) < 0xA8E71A]
    b5c0_writes = [row for row in writes if row["phase"] == "B5C0" and
                   PACKET <= int(row["address"], 16) < PACKET + 0x40]
    observer_writes = [row for row in writes if row["phase"] == "observer"]
    observer_capture_writes = [row for row in observer_writes if capture_pointer is not None and
                               capture_pointer <= int(row["address"], 16) < capture_pointer + 48]
    b320_write_indices = [i for i, row in enumerate(writes) if row in b320_writes]
    observer_write_indices = [i for i, row in enumerate(writes) if row in observer_capture_writes]
    b5c0_write_indices = [i for i, row in enumerate(writes) if row["phase"] == "B5C0"]
    if not b5c0_write_indices:
        fail("B5C0 did not write the synthetic FT4 packet")
    if len(b430_writes) != 9 or len(b5c0_writes) != 6:
        fail(f"unexpected B430/B5C0 write counts: {len(b430_writes)}/{len(b5c0_writes)}")
    if any(MATRIX <= int(row["address"], 16) < TRANSLATION + 16
           for row in writes if row["phase"] in ("B320", "observer")):
        fail("B320 or the compiled observer wrote the transform matrix/translation range")
    if route == "observer":
        if len([e for e in code_events if e["kind"] == "callback_entry" and e["slot"] == 107]) != 1:
            fail("source-built zero-argument callback did not call B320 exactly once")
        if len([e for e in code_events if e["kind"] == "fixture_callback_entry" and
                e["name"] == "batch_projection_observe_slot107"]) != 1:
            fail("source-built zero-argument callback did not enter exactly once")
        captured_camera = read_i32s(uc, capture_pointer, 9)
        captured_screen = read_words(uc, capture_pointer + 36, 6)
        expected_camera = [coordinate for point in camera for coordinate in point]
        expected_screen = [u16(value) for point in screen for value in point]
        if captured_camera != expected_camera or captured_screen != expected_screen:
            fail(f"zero-argument callback capture mismatch: camera={captured_camera!r}, screen={captured_screen!r}")
        if not observer_capture_writes or not b320_write_indices:
            fail("compiled observer or B320 emitted no checked output writes")
        if min(observer_write_indices) <= max(b320_write_indices):
            fail(f"zero-argument callback began capturing before B320 completed all camera/screen writes: B320={b320_write_indices!r}, observer={observer_write_indices!r}, events={writes!r}")
        if max(observer_write_indices) >= min(b5c0_write_indices):
            fail("observer capture writes did not finish before B5C0 packet consumption")
        for row in observer_capture_writes:
            address = int(row["address"], 16)
            if not capture_pointer <= address < capture_pointer + 48:
                fail(f"zero-argument callback wrote outside its persistent capture record: {row!r}")
        for row in observer_writes:
            address = int(row["address"], 16)
            if not (STACK <= address < STACK + 0x1000 or
                    capture_pointer <= address < capture_pointer + 48):
                fail(f"zero-argument callback wrote outside its own stack and persistent capture record: {row!r}")
        if len(observer_capture_writes) != 15:
            fail(f"zero-argument callback should emit 9 DWORD and 6 WORD copy stores, got {len(observer_capture_writes)}")
    elif observer_writes:
        fail("direct B320 route unexpectedly wrote fixture PE capture memory")
    result = {
        "route": route, "completed": True, "raw_eax": hex(raw_eax),
        "registers_before": {name: hex(value) for name, value in start_regs.items()},
        "registers_after": {name: hex(value) for name, value in end_regs.items()},
        "caller_esp_before": hex(start_esp), "caller_esp_after": hex(end_esp),
        "caller_esp_delta": end_esp - start_esp,
        "callee_saved_preserved": all(end_regs[name] == start_regs[name]
                                       for name in ("ebx", "ebp", "esi", "edi")),
        "matrix_unchanged": bytes(uc.mem_read(MATRIX, 18)) == matrix_before,
        "translation_unchanged": bytes(uc.mem_read(TRANSLATION, 16)) == translation_before,
        "B430_input_scratch_WORD_SoA": input_scratch,
        "B430_input_scratch_write_order": b430_writes,
        "camera_XYZ_signed_DWORD_by_vertex": camera,
        "screen_XY_signed_WORD_by_vertex": screen,
        "packet_XY_words_offsets_08_0a_10_12_18_1a": [hex(x) for x in packet_words],
        "B5C0_packet_write_order": b5c0_writes,
        "native_B320_lane_write_order": b320_writes,
        "observer_capture_camera_DWORDs": read_i32s(uc, capture_pointer, 9) if route == "observer" else None,
        "observer_capture_screen_WORDs": [hex(x) for x in read_words(uc, capture_pointer + 36, 6)] if route == "observer" else None,
        "observer_callback_entry": hex(observer_entry - DLL_BASE) if route == "observer" else None,
        "observer_explicit_stack_argument_count": 0 if route == "observer" else None,
        "observer_stack_write_count": len(observer_writes) - len(observer_capture_writes),
        "observer_capture_write_count": len(observer_capture_writes),
        "observer_capture_after_all_B320_lane_writes": (min(observer_write_indices) > max(b320_write_indices)) if route == "observer" else None,
        "observer_capture_before_B5C0": (max(observer_write_indices) < min(b5c0_write_indices)) if route == "observer" else None,
        "callback_entry_counts": {str(slot): len([e for e in code_events
                                                  if e["kind"] == "callback_entry" and e["slot"] == slot])
                                   for slot in (102, 107, 117)},
        "stale_screen_seed_WORDs": [hex(x) for x in case["screen_seed"]],
        "code_event_count": len(code_events),
        "write_event_count": len(writes),
        "data_read_event_count": len(data_reads),
    }
    result["_writes"] = writes
    result["_mapped"] = mapped
    result["_mapped_fixture"] = mapped_candidate
    return result


def cases():
    identity = [4096, 0, 0, 0, 4096, 0, 0, 0, 4096]
    common = {"center": [50, 70], "screen_seed": [0x1111, 0x2222, 0x3333,
                                                       0x4444, 0x5555, 0x6666]}
    return [
        {**common, "name": "sum_before_sar12_and_signed_z_paths",
         "prediction": "2048*1 + 2048*1 is shifted once after accumulation to camera X=1; the three points exercise positive, negative, and zero Z with preseeded screen outputs.",
         "matrix": [2048, 2048, 0, 0, 4096, 0, 0, 0, 4096],
         "translation": [0, 0, 0, 0x13579BDF], "focal": 256,
         "points": [(1, 1, 100), (1, 1, -100), (1, 1, 0)],
         "expected_camera_x": [1, 1, 1], "zero_z_index": 2},
        {**common, "name": "signed_word_positive_overflow",
         "prediction": "4096*32767 SAR12 gives 32767; adding translation +1 narrows low WORD 0x8000 and sign-extends to -32768.",
         "matrix": identity, "translation": [1, 0, 0, 0x13579BDF], "focal": 0,
         "points": [(32767, 2, 1000), (0, 3, 1000), (1, 4, 1000)],
         "expected_camera_x": [-32768, 1, 2]},
        {**common, "name": "signed_word_negative_overflow",
         "prediction": "4096*(-32768) SAR12 gives -32768; adding translation -1 narrows low WORD 0x7fff and sign-extends to 32767.",
         "matrix": identity, "translation": [-1, 0, 0, 0x13579BDF], "focal": 0,
         "points": [(-32768, 2, 1000), (0, 3, 1000), (1, 4, 1000)],
         "expected_camera_x": [32767, -1, 0]},
    ]


def trap_case(exe, candidate, candidate_exports, case):
    """Record signed IDIV overflow only as an architectural trap, never a result."""
    reports = []
    for route in ("direct", "observer"):
        row = run_route(exe, candidate, candidate_exports, case, route)
        if (row.get("completed") or not row.get("idiv_reached") or
                row.get("exception_pc") != hex(IMAGE_BASE + 0xB3F1)):
            fail(f"signed IDIV overflow was not reported as a reached, non-success trap: {row!r}")
        reports.append({"route": route, "completed": False,
                        "idiv_rva": "0xB3F1", "idiv_reached": True,
                        "exception_pc": row["exception_pc"],
                        "emulator_exception": row.get("exception"),
                        "interpretation": "expected signed 32-bit IDIV overflow trap; no projection result is claimed"})
    return reports


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    repo = bootstrap(args.repo_root)
    exe_path, output_path, source, package_include, shared_headers = guarded_paths(args)
    exe = PE(exe_path)
    if sha(exe.data) != EXE_SHA256 or exe.image_base != IMAGE_BASE:
        fail("original HERCULES.EXE hash or preferred base differs from pinned evidence")
    if capstone.__version__ != "5.0.7":
        fail(f"expected Capstone 5.0.7, got {capstone.__version__}")
    runtime = verify_runtime(importlib.import_module("unicorn"), ORACLE_DEPS)
    spans, instructions = decode_spans(exe)
    lane_witnesses = batch_lane_witnesses(instructions)
    table = exe.read_rva(HOST_TABLE_RVA, HOST_TABLE_SIZE)
    if sha(table) != HOST_TABLE_SHA256:
        fail("pinned HERC host callback table hash mismatch")
    targets = struct.unpack("<250I", table)
    slot_records = {}
    for slot, rva in SLOTS.items():
        target_va = targets[slot]
        if target_va - exe.image_base != rva:
            fail(f"host callback slot {slot} does not resolve to {rva:#x}")
        slot_records[str(slot)] = {"slot": slot, "table_offset": hex(slot * 4),
                                   "target_va": hex(target_va), "target_rva": hex(rva),
                                   "signature_scope": SLOT_EXPECTED[slot]}

    if not source.is_file() or redirected(source):
        fail("compiled observer source is missing or redirected")
    build_dir = output_path.parent / "compiled_observer"
    (obj_path, asm_path, obj_bytes, candidate_path, map_path, compile_command,
     link_command, build_output, post_compile_hashes) = compile_observer(
        source, build_dir, package_include)
    candidate = PE(candidate_path)
    if candidate.image_base != DLL_BASE or candidate.pe.FILE_HEADER.Machine != 0x14C:
        fail("VC5 output is not the pinned-base 32-bit x86 fixture PE")
    if getattr(candidate.pe, "DIRECTORY_ENTRY_IMPORT", []):
        fail("source-built fixture PE unexpectedly imports external code or data")
    reloc = candidate.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5]
    if reloc.VirtualAddress != 0 or reloc.Size != 0:
        fail("source-built fixed-base fixture PE unexpectedly contains relocations")
    candidate_exports = pe_exports(candidate, DLL_BASE)
    required_exports = ("batch_projection_set_saved_slot107",
                        "batch_projection_observe_slot107",
                        "batch_projection_diagnostic_state")
    for name in required_exports:
        find_export(candidate_exports, name)
    observer_entry_rva = find_export(candidate_exports, "batch_projection_observe_slot107") - DLL_BASE
    text_section = next((section for section in candidate.sections if section["name"] == ".text"), None)
    if text_section is None:
        fail("source-built fixture PE has no .text section")
    fixture_text_hash = sha(candidate.read_rva(text_section["virtual_address"],
                                               text_section["raw_size"]))

    case_records = []
    first_mapped = None
    for case in cases():
        direct = run_route(exe, candidate, candidate_exports, case, "direct")
        observer = run_route(exe, candidate, candidate_exports, case, "observer")
        if not direct["completed"] or not observer["completed"]:
            fail(f"finite case unexpectedly trapped: {case['name']}")
        if direct["raw_eax"] != observer["raw_eax"]:
            fail(f"raw EAX mismatch in {case['name']}: {direct['raw_eax']} vs {observer['raw_eax']}")
        for key in ("B430_input_scratch_WORD_SoA", "B430_input_scratch_write_order",
                    "camera_XYZ_signed_DWORD_by_vertex", "screen_XY_signed_WORD_by_vertex",
                    "packet_XY_words_offsets_08_0a_10_12_18_1a", "B5C0_packet_write_order",
                    "native_B320_lane_write_order"):
            if direct[key] != observer[key]:
                fail(f"direct/observer {key} mismatch in {case['name']}")
        if any(direct["callback_entry_counts"][str(slot)] != 1 or
               observer["callback_entry_counts"][str(slot)] != 1 for slot in (102, 107, 117)):
            fail(f"B320 route count differs from exactly once in {case['name']}")
        if direct["camera_XYZ_signed_DWORD_by_vertex"][0][0] != case["expected_camera_x"][0]:
            fail(f"X lane did not exhibit predicted low-word behavior in {case['name']}")
        observed_x = [point[0] for point in direct["camera_XYZ_signed_DWORD_by_vertex"]]
        if observed_x != case["expected_camera_x"]:
            fail(f"camera X lanes do not match the discriminating prediction in {case['name']}: {observed_x!r}")
        expected_packet = [u16(value) for point in direct["screen_XY_signed_WORD_by_vertex"] for value in point]
        observed_packet = [int(value, 16) for value in direct["packet_XY_words_offsets_08_0a_10_12_18_1a"]]
        if observed_packet != expected_packet:
            fail(f"B5C0 packet XY order differs from B320 screen lanes in {case['name']}")
        for lane in ("ebx", "ebp", "esi", "edi"):
            if direct["registers_after"][lane] != direct["registers_before"][lane] or observer["registers_after"][lane] != observer["registers_before"][lane]:
                fail(f"callee-saved register {lane} changed in {case['name']}")
        if case.get("zero_z_index") is not None:
            index = case["zero_z_index"]
            after = [u16(direct["screen_XY_signed_WORD_by_vertex"][index][0]),
                     u16(direct["screen_XY_signed_WORD_by_vertex"][index][1])]
            seeds = [case["screen_seed"][index], case["screen_seed"][3 + index]]
            if after == seeds:
                fail("zero-Z path left both preseeded screen lanes stale")
        if first_mapped is None:
            first_mapped = direct["_mapped"]
        direct_summary = {key: direct[key] for key in (
            "raw_eax", "callee_saved_preserved", "registers_before", "registers_after",
            "caller_esp_before", "caller_esp_after", "caller_esp_delta", "matrix_unchanged",
            "translation_unchanged", "B430_input_scratch_WORD_SoA",
            "camera_XYZ_signed_DWORD_by_vertex", "screen_XY_signed_WORD_by_vertex",
            "packet_XY_words_offsets_08_0a_10_12_18_1a", "callback_entry_counts")}
        observer_summary = {key: observer[key] for key in (
            "raw_eax", "callee_saved_preserved", "registers_before", "registers_after",
            "caller_esp_before", "caller_esp_after", "caller_esp_delta", "matrix_unchanged",
            "translation_unchanged", "observer_capture_camera_DWORDs",
            "observer_capture_screen_WORDs", "observer_callback_entry",
            "observer_explicit_stack_argument_count", "observer_stack_write_count",
            "observer_capture_write_count", "observer_capture_after_all_B320_lane_writes",
            "observer_capture_before_B5C0", "callback_entry_counts")}
        compact_write_order = [[row["pc"], row["address"], row["size"], row["value"]]
                               for row in direct["native_B320_lane_write_order"]]
        compact_b430_write_order = [[row["pc"], row["address"], row["size"], row["value"]]
                                    for row in direct["B430_input_scratch_write_order"]]
        compact_b5c0_write_order = [[row["pc"], row["address"], row["size"], row["value"]]
                                    for row in direct["B5C0_packet_write_order"]]
        case_records.append({
            "name": case["name"], "prediction": case["prediction"],
            "matrix_words": case["matrix"], "translation_dwords": case["translation"],
            "focal_dword": case["focal"], "center_dwords": case["center"],
            "source_signed_WORD_XYZ_triplets": [list(point) for point in case["points"]],
            "direct": direct_summary,
            "compiled_observer": observer_summary,
            "B430_input_scratch_write_order_pc_address_size_value": compact_b430_write_order,
            "native_B320_lane_write_order_pc_address_size_value": compact_write_order,
            "B5C0_packet_write_order_pc_address_size_value": compact_b5c0_write_order,
            "equivalence": {"raw_EAX_equal": True, "camera_and_screen_equal": True,
                            "native_B320_lane_write_order_equal": True,
                            "B430_input_and_B5C0_packet_equal": True,
                            "compiled_observer_captures_after_B320": observer["observer_capture_after_all_B320_lane_writes"]},
        })

    overflow = {"name": "signed_idiv_overflow_min_int_div_minus_one",
                "matrix": [4096, 0, 0, 0, 4096, 0, 0, 0, 4096],
                "translation": [0, 0, 0, 0x13579BDF], "center": [0, 0],
                "focal": 32768, "points": [(1, 1, -1), (0, 0, 1), (0, 0, 1)],
                "screen_seed": [0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666]}
    overflow_reports = trap_case(exe, candidate, candidate_exports, overflow)

    report = {
        "schema_version": 1,
        "status": "PASS",
        "proofs": {
            "prediction_and_falsifier": {
                "prediction_record": "renderer_contract/evidence/batch_projection_zero_argument_prediction.json",
                "prediction_sha256": PREDICTION_SHA256,
                "preserved_helper_prediction_record": "renderer_contract/evidence/batch_projection_prediction.json",
                "preserved_helper_prediction_sha256": HELPER_PREDICTION_SHA256,
                "prediction": "A VC5-compiled source-built x86 PE will expose a genuine zero-argument diagnostic callback that invokes its saved B320 target exactly once, retains raw EAX as renderer_raw32 ABI bits, copies all camera DWORD and screen WORD lanes into persistent fixture state, preserves cdecl callee-saved registers and caller stack position, and leaves matrix/translation unchanged. Direct and callback B320 outputs and native write order should match for each predeclared finite state.",
                "falsifier": "Any compile/link failure, DLL import or relocation, missing/ambiguous export, unknown PC or data endpoint, saved target different from B320, callback entry or B320 count other than one, capture/raw-EAX/lane mismatch, native write-order mismatch, caller ESP or callee-saved mismatch, state write outside the saved pointer and capture record, or matrix/translation change fails the fixture. The signed IDIV INT_MIN/-1 case must trap at B3F1 in both routes, never produce an accepted result.",
            },
            "static_observed": {
                "proof_level": "STATIC_OBSERVED",
                "scope": "Pinned original HERCULES.EXE callback table, exact B430/B320/B5C0 spans, complete x86 decode, and finite-invocation lane-write instructions only.",
                "target_sha256": EXE_SHA256,
                "image_base": hex(exe.image_base),
                "host_callback_table": {"rva": hex(HOST_TABLE_RVA), "size": HOST_TABLE_SIZE,
                                        "sha256": HOST_TABLE_SHA256, "slots": slot_records},
                "spans": spans,
                "B320_lane_base_stride_instruction_witnesses": lane_witnesses,
                "batch_arithmetic": "Each B320 IMUL and ADD keeps its low 32-bit result; the three signed products accumulate with 32-bit wrap, SAR is applied to the completed row sum by 12, the translation DWORD is added with 32-bit arithmetic, and MOVSX sign-extends AX (the low WORD) into the camera DWORD lane. Nonzero Z uses signed IDIV; zero Z skips IDIV and carries focal<<16 into projection. This fixture samples those effects only and does not establish a general source-level transform model.",
                "native_array_lanes": {
                    "B430_source_scratch": "Three caller pointers each reference signed WORD XYZ; B430 deinterleaves the three input records into source X WORDs at A8E708+2*i, Y at A8E70E+2*i, and Z at A8E714+2*i.",
                    "B320_camera": "X signed DWORD lanes start A8E71C, Y at A8E728, Z at A8E734; each vertex lane is 4 bytes.",
                    "B320_screen": "X signed WORD lanes start A8E740, Y at A8E746; each vertex lane is 2 bytes. B320 writes per-vertex X then Y.",
                    "B5C0_packet": "B5C0 reads corresponding X/Y screen lane i and writes packet WORD pairs at offsets +08/+0A, +10/+12, +18/+1A.",
                },
                "observed_instruction_witnesses": {
                    "source_sum_shift_narrow": ["B34B add eax,edx", "B357 add eax,edx", "B359 SAR eax,12", "B362 add translation", "B364 movsx eax,ax", "B367 store camera X"],
                    "zero_Z_projection": ["B3E4 load focal", "B3E9 shift left 16", "B3EC test Z", "B3EE JE B3F3", "B3F0 CDQ", "B3F1 IDIV Z"],
                    "screen_words": ["B406 store screen X WORD", "B422 store screen Y WORD"],
                },
            },
            "bounded_equivalent": {
                "proof_level": "BOUNDED_EQUIVALENT",
                "scope": "Three finite synthetic source triplet sets through unchanged original B430 -> B320 -> B5C0, compared with a zero-argument diagnostic callback in a freshly VC5-compiled source-built PE. The callback uses persistent saved-target/capture state and calls B320 once. One signed IDIV overflow case is classified only as a trap.",
                "result": "BOUNDED_ORIGINAL_X86_ZERO_ARGUMENT_CALLBACK_FIXTURE_PASS",
                "callback_abi_fixture": {
                    "slot102_B430": "The finite fixture supplies three cdecl stack pointers to signed-WORD XYZ triplets and confirms B430's source scratch writes; this is a tested call boundary, not a historical source declaration.",
                    "slot107_B320": "The direct route and saved-target call both invoke B320 without explicit stack arguments. No historical source signature or semantic return contract is established.",
                    "slot117_B5C0": "The finite fixture supplies one cdecl stack pointer to its synthetic packet and confirms the six packet WORD stores; this is a tested call boundary, not a historical source declaration.",
                    "source_built_callback": "The fixture PE exports a true zero-argument cdecl diagnostic callback with persistent saved-target/capture state. It calls B320 exactly once and stores nine camera DWORD and six screen WORD lanes.",
                    "raw_eax_view": "renderer_raw32 preserves the saved target's raw EAX bits as a physical x86 ABI observation; no semantic return meaning or historical B320 source type is inferred.",
                    "explicit_stack_argument_count": 0,
                },
                "cases": case_records,
                "signed_idiv_overflow_traps": overflow_reports,
                "limits": [
                    "No execution of ENG1/ENG3/TITLE producers, Windows host process, renderer, GPU, SDL, assets, or gameplay.",
                    "No broad batch C projection model, B740 substitution, all-matrix proof, or general camera/center/focal setup proof.",
                    "The source-built zero-argument callback proves only the pinned VC5 diagnostic PE and tested finite states; it is not an installed engine callback replacement or source-identification claim.",
                    "The zero-Z case establishes that these selected screen lanes are overwritten for the tested nonzero XY and focal state; it does not cover every focal, overflow, or center value.",
                    "The signed IDIV overflow case is an expected non-success trap at B3F1 and carries no projection output claim.",
                ],
            },
        },
        "tooling": {
            "python": sys.version.split()[0], "capstone": capstone.__version__,
            "unicorn": runtime,
            "analysis_dependency_root": str(ANALYSIS_DEPS),
            "oracle_dependency_root": str(ORACLE_DEPS),
            "vc5": {"origin": "existing pinned local MSVC 5.00 toolchain", "root": str(VC5_ROOT),
                    "product_version": VC5_PRODUCT_VERSION, "compiler_file_version": VC5_DRIVER_VERSION,
                    "linker_file_version": "5.00.7022",
                    "compiler_components_sha256": post_compile_hashes,
                    "toolchain_manifest_sha256": VC5_MANIFEST_SHA256,
                    "compile_command": compile_command, "link_command": link_command,
                    "build_output": build_output,
                    "observer_source_sha256": sha(source.read_bytes()),
                    "observer_object_sha256": sha(obj_bytes),
                    "fixture_pe_sha256": sha(candidate_path.read_bytes()),
                    "fixture_map_sha256": sha(map_path.read_bytes()),
                    "fixture_text_sha256": fixture_text_hash,
                    "fixture_image_base": hex(candidate.image_base),
                    "fixture_callback_entry_rva": hex(observer_entry_rva),
                    "fixture_exports": {name: hex(find_export(candidate_exports, name) - DLL_BASE)
                                         for name in required_exports},
                    "assembly_listing_sha256": sha(asm_path.read_bytes()),
                    "generated_object": str(obj_path.relative_to(ROOT)),
                    "generated_listing": str(asm_path.relative_to(ROOT)),
                    "generated_fixture_pe": str(candidate_path.relative_to(ROOT)),
                    "generated_fixture_map": str(map_path.relative_to(ROOT))},
            "shared_abi_headers": {
                name: {"path": str(path.relative_to(ROOT)), "sha256": sha(path.read_bytes())}
                for name, path in shared_headers.items()},
            "execution_contract": {
                "mapped_original_exe_sections_at_preferred_base": True,
                "executed_native_spans": ["B430", "B320", "B5C0"],
                "executed_source_built_fixture_pe": True,
                "executed_zero_argument_diagnostic_callback": True,
                "fixture_pe_imports_external_code_or_data": False,
                "fixture_pe_relocations": False,
                "synthetic_memory_for_input_records_stack_packet_and_original_host_state": True,
                "original_machine_code_modified": False,
                "unknown_pc_or_memory_endpoint": "fixture fails",
                "read_allowlists": {
                    "B430": ["synthetic stack", "synthetic INPUT page for three pointed records"],
                    "B320": ["synthetic stack", "source WORD scratch 0xA8E708..0xA8E719", "camera DWORD lanes 0xA8E71C..0xA8E73F", "center/focal 0xA8E74C..0xA8E757", "matrix WORDs 0xA8E75C..0xA8E76D", "translation DWORDs 0xA8E770..0xA8E77F"],
                    "B5C0": ["synthetic stack", "screen WORD lanes 0xA8E740..0xA8E74B", "synthetic packet buffer"],
                    "observer": ["synthetic stack", "persistent fixture saved-target pointer DWORD", "camera DWORD lanes 0xA8E71C..0xA8E73F", "screen WORD lanes 0xA8E740..0xA8E74B"],
                },
                "write_allowlists": {
                    "B430": ["synthetic stack", "source WORD scratch 0xA8E708..0xA8E719"],
                    "B320": ["synthetic stack", "camera DWORD lanes 0xA8E71C..0xA8E73F", "screen WORD lanes 0xA8E740..0xA8E74B"],
                    "B5C0": ["synthetic stack", "synthetic packet buffer"],
                    "fixture_setup": ["synthetic stack", "persistent fixture saved-target pointer DWORD"],
                    "observer": ["synthetic stack", "persistent fixture camera DWORD and screen WORD capture record"],
                },
                "instruction_limit_per_callback": 50000,
            },
        },
    }
    output_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": report["status"], "output": str(output_path),
                      "proofs": {key: value.get("proof_level", "PREDICTION_AND_FALSIFIER")
                                 for key, value in report["proofs"].items()},
                      "cases": [row["name"] for row in case_records],
                      "idiv_overflow": "expected trap at B3F1"}, indent=2))


if __name__ == "__main__":
    main()
