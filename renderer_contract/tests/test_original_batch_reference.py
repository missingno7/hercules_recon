#!/usr/bin/env python3
"""Differentially compare the source-built B320 model with pinned original x86."""
from __future__ import annotations

import argparse
import hashlib
import importlib
import json
import os
from pathlib import Path
import stat
import struct
import sys

ANALYSIS_DEPS = Path(r"C:\tools\hercules\python")
ORACLE_DEPS = Path(r"C:\tools\hercules\renderer-oracle-python")
EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
B320_SHA256 = "11368c1fa7fd8cd91b1d7b7de522ad06aacc724991ff4f8d46d7eecf1e7cff97"
AUDIT_SHA256 = "f19cfa334b66cc66b6418589fb66a3c255e44902b12180c3400db04963eed4ff"
ORACLE_COMMON_SHA256 = "a024048d9acc6e18197e10a82427cd84351a90666e1626e610bf60c6105f1357"
PE_PARSER_SHA256 = "8da71eb87b5a88dd1d80a3fe180e2b94557e2c3017fb4e5c37c4a8c630af62ca"
OBSERVER_SHA256 = "cf52f7425ae6f36ff74c7ccaf7dea24fcc6a2a5e8310f44d89dbdc8957ed5d1c"
TYPES_SHA256 = "e957f5f142850a66c3368d4c37913b5c4528d04a4b86e1c9e78e0cc653569b4d"
HOST_ABI_SHA256 = "b51d75b165c4800aa9ef5431bdf6f6210ee289d36f5b1c000e1ad1048f52cf5b"
PROJECTION_H_SHA256 = "290b3922cd2ae474dc8dd2decf5c18e260bfccab9c79d49a2e5d24ea28dec216"
PROJECTION_C_SHA256 = "c0189e13f46f580dd79f2f834e7680a50a70e392b5f93ea9d5f78732f63492bf"
TEST_C_SHA256 = "1e55a561e124cbfb5b8dc671e57f2e1a40aa1c4bf35fc45f55c8f9f7cc7650fc"
TEST_CPP_SHA256 = "f881308b3dbadbd0954bee49552df606e1f9bc12157db747b2b829a149494c51"
PREDICTION_SHA256 = "9609be8188615a281f4b17f8b8b0782e7e539f79db7f3c8760c58b4eeb56ece5"
FIXTURE_SHA256 = "9b466a4d71ee121a5fdcd4894a5d2dee7936c9072c161384ef308d78e96c3abe"
DLL_BASE = 0x20000000
INPUT = 0x10000000
STACK = 0x12000000
SENTINEL = 0x13000000
CONFIG_OFFSET = 0x100
POINTS_OFFSET = 0x140
RESULT_OFFSET = 0x180
FAULT_OFFSET = 0x1B0
RAW_OFFSET = 0x1B4
CAMERA_CANARY = 0xA5A5A5A5
SCREEN_CANARY = 0xD00D
RAW_EAX_CANARY = 0xDEADBEEF


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


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


def check_components(path: Path, stop: Path) -> None:
    path = Path(os.path.abspath(str(path)))
    stop = Path(os.path.abspath(str(stop)))
    try:
        parts = path.relative_to(stop).parts
    except ValueError as exc:
        raise SystemExit(f"path escapes protected root {stop}: {path}") from exc
    current = stop
    if redirected(current) or not same_path(current, current.resolve(strict=False)):
        raise SystemExit(f"refusing redirected component: {current}")
    for part in parts:
        current = current / part
        if redirected(current) or not same_path(current, current.resolve(strict=False)):
            raise SystemExit(f"refusing redirected component: {current}")


def overlaps(left: Path, right: Path) -> bool:
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


def guarded_paths(args):
    lexical_root = Path(os.path.abspath(str(args.repo_root)))
    resolved_root = lexical_root.resolve(strict=False)
    if not same_path(lexical_root, resolved_root) or redirected(lexical_root):
        raise SystemExit("--repo-root must not resolve through a symlink or reparse point")
    if not (resolved_root / "scripts" / "pe.py").is_file():
        raise SystemExit("--repo-root must contain scripts/pe.py")
    work = lexical_root / "work"
    if not work.is_dir() or redirected(work) or not same_path(work, work.resolve(strict=False)):
        raise SystemExit("--repo-root/work must be an existing non-redirected directory")

    package = Path(__file__).resolve().parents[1]
    if not package.is_dir() or redirected(package):
        raise SystemExit("candidate renderer_contract package is missing or redirected")
    check_components(package, resolved_root)
    try:
        package.relative_to(resolved_root)
    except ValueError:
        raise SystemExit("candidate package is outside the repository root")

    output = args.output if args.output.is_absolute() else lexical_root / args.output
    output = Path(os.path.abspath(str(output)))
    if ".." in Path(str(args.output)).parts:
        raise SystemExit("refusing --output containing a parent-directory component")
    try:
        relative = output.relative_to(work)
    except ValueError as exc:
        raise SystemExit("--output must stay under the lexical repo-root/work directory") from exc
    if not relative.parts or output.suffix.lower() != ".json":
        raise SystemExit("--output must name a .json file below work/")
    if any(part.lower() in ("assets", "discs") for part in relative.parts):
        raise SystemExit("refusing output under assets/ or a discs/ original tree")
    check_components(output.parent, work)
    if output.exists() or redirected(output):
        raise SystemExit("--output must be a new file; existing artifacts are never overwritten")
    if output.parent.exists() and (not output.parent.is_dir() or any(output.parent.iterdir())):
        raise SystemExit("--output parent must be new or empty")
    resolved_work = work.resolve(strict=False)
    try:
        output.resolve(strict=False).relative_to(resolved_work)
    except ValueError as exc:
        raise SystemExit("resolved --output escapes repo-root/work") from exc
    if overlaps(output, package):
        raise SystemExit("--output overlaps candidate source package")

    exe = Path(os.path.abspath(str(args.exe)))
    if ".." in Path(str(args.exe)).parts:
        raise SystemExit("--exe must not contain parent-directory components")
    check_components(exe, Path(exe.anchor))
    if not exe.is_file() or sha(exe) != EXE_SHA256:
        raise SystemExit("--exe does not match the pinned original HERCULES.EXE hash")
    if overlaps(output, exe) or overlaps(output.parent, exe.parent):
        raise SystemExit("--output overlaps the read-only original executable or its directory")

    root_package = resolved_root / "renderer_contract"
    candidate_files = {
        "hercules_projection.h": package / "hercules_projection.h",
        "hercules_projection.c": package / "hercules_projection.c",
        "test_projection.c": package / "tests" / "test_projection.c",
        "test_projection_cpp.cpp": package / "tests" / "test_projection_cpp.cpp",
        "projection_candidate_fixture.c": package / "fixtures" / "batch_reference" / "projection_candidate_fixture.c",
        "batch_reference_prediction.json": package / "evidence" / "batch_reference_prediction.json",
    }
    pins = {
        "hercules_projection.h": PROJECTION_H_SHA256,
        "hercules_projection.c": PROJECTION_C_SHA256,
        "test_projection.c": TEST_C_SHA256,
        "test_projection_cpp.cpp": TEST_CPP_SHA256,
        "projection_candidate_fixture.c": FIXTURE_SHA256,
        "batch_reference_prediction.json": PREDICTION_SHA256,
    }
    for name, path in candidate_files.items():
        if not path.is_file() or redirected(path) or sha(path) != pins[name]:
            raise SystemExit(f"candidate source/evidence is missing, redirected, or changed: {name}")
        check_components(path, package)

    protected_sources = {
        "audit_batch_projection.py": resolved_root / "renderer_contract" / "tests" / "audit_batch_projection.py",
        "oracle_common.py": resolved_root / "renderer_contract" / "tests" / "oracle_common.py",
        "pe.py": resolved_root / "scripts" / "pe.py",
        "native_observer.c": resolved_root / "renderer_contract" / "fixtures" / "batch_projection" / "observer.c",
        "hercules_types.h": root_package / "hercules_types.h",
        "hercules_host_abi.h": root_package / "hercules_host_abi.h",
    }
    protected_pins = {
        "audit_batch_projection.py": AUDIT_SHA256,
        "oracle_common.py": ORACLE_COMMON_SHA256,
        "pe.py": PE_PARSER_SHA256,
        "native_observer.c": OBSERVER_SHA256,
        "hercules_types.h": TYPES_SHA256,
        "hercules_host_abi.h": HOST_ABI_SHA256,
    }
    for name, path in protected_sources.items():
        if not path.is_file() or redirected(path) or sha(path) != protected_pins[name]:
            raise SystemExit(f"pinned native helper/shared source is missing or changed: {name}")
        check_components(path, resolved_root)
        if overlaps(output, path) or overlaps(output.parent, path):
            raise SystemExit(f"--output overlaps protected source: {path}")
    output.parent.mkdir(parents=True, exist_ok=True)
    check_components(output.parent, work)
    return resolved_root, package, exe, output, candidate_files, protected_sources


def batch_cases(oracle):
    common = {"center": [50, 70],
              "screen_seed": [0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666]}
    base = oracle.cases()
    extra = [
        {**common, "name": "negative_sar12_rounds_toward_negative_infinity",
         "prediction": "A row sum of -1 SAR12s to -1, not zero.",
         "matrix": [1, 0, 0, 0, 4096, 0, 0, 0, 4096],
         "translation": [0, 0, 0, 0x13579BDF], "focal": 0,
         "points": [(-1, 10, 1000), (0, 0, 1000), (0, 0, 1000)],
         "expected_camera_x": [-1, 0, 0]},
        {**common, "name": "three_product_row_sum_wraps_low32",
         "prediction": "Three signed WORD products wrap their row sum to low32 before SAR12 and low-WORD narrowing.",
         "matrix": [32767, 32767, 32767, 0, 4096, 0, 0, 0, 4096],
         "translation": [0, 0, 0, 0x13579BDF], "focal": 0,
         "points": [(32767, 32767, 32767), (1, 2, 3), (-1, -2, -3)],
         "expected_camera_x": [-48, 47, -48]},
    ]
    return base + extra


def trap_case():
    return {"name": "point1_signed_idiv_overflow_min_int_div_minus_one",
            "matrix": [4096, 0, 0, 0, 4096, 0, 0, 0, 4096],
            "translation": [0, 0, 0, 0x13579BDF], "center": [0, 0],
            "focal": 32768, "points": [(0, 0, 1), (1, 2, -1), (3, 4, 5)],
            "screen_seed": [0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666]}


def config_bytes(case):
    return struct.pack("<9h2x3i2iI", *case["matrix"], *case["translation"][:3],
                       *case["center"], case["focal"])


def points_bytes(case):
    flat = [coordinate for point in case["points"] for coordinate in point]
    return struct.pack("<9h", *flat)


def seed_result_bytes(case):
    camera = [CAMERA_CANARY] * 9
    seed = case["screen_seed"]
    screen = [seed[0], seed[3], seed[1], seed[4], seed[2], seed[5]]
    return struct.pack("<9I6H", *(camera + screen))


def reconstruct_native(case, native, oracle):
    camera = [CAMERA_CANARY] * 9
    seed = case["screen_seed"]
    screen = [seed[0], seed[3], seed[1], seed[4], seed[2], seed[5]]
    writes = native.get("_writes", native.get("writes", []))
    output_writes = []
    camera_bases = {oracle.CAMERA_X: 0, oracle.CAMERA_Y: 1, oracle.CAMERA_Z: 2}
    for row in writes:
        if row["phase"] != "B320":
            continue
        address = int(row["address"], 16)
        size = int(row["size"])
        value = int(row["value"], 16)
        updated = False
        for base, lane in camera_bases.items():
            if base <= address < base + 12:
                delta = address - base
                if size != 4 or delta % 4 != 0:
                    fail(f"native camera lane write has unexpected width/alignment: {row!r}")
                index = (delta // 4) * 3 + lane
                camera[index] = value & 0xffffffff
                output_writes.append(row)
                updated = True
                break
        if updated:
            continue
        for base, lane in ((oracle.SCREEN_X, 0), (oracle.SCREEN_Y, 1)):
            if base <= address < base + 6:
                delta = address - base
                if size != 2 or delta % 2 != 0:
                    fail(f"native screen lane write has unexpected width/alignment: {row!r}")
                index = (delta // 2) * 2 + lane
                screen[index] = value & 0xffff
                output_writes.append(row)
                updated = True
                break
    expected_addresses = []
    points_complete = 3 if native["completed"] else 1
    for point_index in range(points_complete):
        expected_addresses.extend([
            oracle.CAMERA_X + point_index * 4,
            oracle.CAMERA_Y + point_index * 4,
            oracle.CAMERA_Z + point_index * 4,
            oracle.SCREEN_X + point_index * 2,
            oracle.SCREEN_Y + point_index * 2,
        ])
    if not native["completed"]:
        expected_addresses.extend([oracle.CAMERA_X + 4, oracle.CAMERA_Y + 4, oracle.CAMERA_Z + 4])
    observed_addresses = [int(row["address"], 16) for row in output_writes]
    if observed_addresses != expected_addresses:
        fail(f"native B320 output write order differs from the bounded contract: {observed_addresses!r}")
    if native["completed"]:
        expected_camera = [value & 0xffffffff for point in native["camera_XYZ_signed_DWORD_by_vertex"] for value in point]
        expected_screen = [value & 0xffff for point in native["screen_XY_signed_WORD_by_vertex"] for value in point]
        if camera != expected_camera or screen != expected_screen:
            fail("native completed lane-write reconstruction differs from direct output summary")
    else:
        if native.get("exception_pc") != hex(oracle.IMAGE_BASE + 0xB3F1) or not native.get("idiv_reached"):
            fail("native fault did not reach the pinned B320 IDIV instruction")
    return camera, screen, output_writes


def run_candidate(oracle, candidate, exports, case):
    uc = oracle.Uc(oracle.UC_ARCH_X86, oracle.UC_MODE_32)
    # The projection implementation is reentrant and has no global state, so
    # this PE legitimately has no writable section. Map its declared sections
    # directly instead of using audit_batch_projection's observer-specific
    # helper, which requires writable diagnostic globals.
    code_ranges = []
    data_ranges = []
    mapped = []
    for section in candidate.sections:
        size = oracle.align_up(max(section["virtual_size"], section["raw_size"]))
        if not size:
            continue
        va = DLL_BASE + section["virtual_address"]
        chars = section["characteristics"]
        perms = oracle.UC_PROT_READ
        if chars & 0x80000000:
            perms |= oracle.UC_PROT_WRITE
            data_ranges.append((va, va + size))
        if chars & 0x20000000:
            perms |= oracle.UC_PROT_EXEC
            code_ranges.append((va, va + size))
        uc.mem_map(va, size, perms)
        if section["raw_size"]:
            uc.mem_write(va, candidate.read_rva(section["virtual_address"], section["raw_size"]))
        if size > section["raw_size"]:
            uc.mem_write(va + section["raw_size"], b"\0" * (size - section["raw_size"]))
        mapped.append({"name": section["name"], "va": hex(va), "size": size,
                       "characteristics": hex(chars)})
    if not code_ranges:
        fail("source-built candidate PE lacks executable code")
    uc.mem_map(INPUT, 0x1000, oracle.UC_PROT_READ | oracle.UC_PROT_WRITE)
    uc.mem_map(STACK, 0x1000, oracle.UC_PROT_READ | oracle.UC_PROT_WRITE)

    config_address = INPUT + CONFIG_OFFSET
    points_address = INPUT + POINTS_OFFSET
    result_address = INPUT + RESULT_OFFSET
    fault_address = INPUT + FAULT_OFFSET
    raw_address = INPUT + RAW_OFFSET
    uc.mem_write(config_address, config_bytes(case))
    uc.mem_write(points_address, points_bytes(case))
    uc.mem_write(result_address, seed_result_bytes(case))
    uc.mem_write(fault_address, struct.pack("<I", 0xffffffff))
    uc.mem_write(raw_address, struct.pack("<I", RAW_EAX_CANARY))

    code_events = []
    reads = []
    writes = []

    def candidate_pc(address):
        return any(start <= address < end for start, end in code_ranges)

    def on_code(machine, address, size, _user):
        if not candidate_pc(address):
            fail(f"candidate execution escaped its source-built PE at {address:#x}")
        code_events.append({"pc": hex(address), "size": size})

    uc.hook_add(oracle.UC_HOOK_CODE, on_code)

    def in_range(address, size, start, end):
        return start <= address and address + size <= end

    read_ranges = [(config_address, config_address + 44),
                   (points_address, points_address + 18),
                   (result_address, result_address + 48),
                   (fault_address, fault_address + 4),
                   (raw_address, raw_address + 4)]
    write_ranges = [(result_address, result_address + 48),
                    (fault_address, fault_address + 4),
                    (raw_address, raw_address + 4)]

    def on_read(machine, _access, address, size, _value, _user):
        pc = machine.reg_read(oracle.UC_X86_REG_EIP)
        if not candidate_pc(pc):
            fail(f"candidate data read from unknown code endpoint {pc:#x}")
        if not (in_range(address, size, STACK, STACK + 0x1000) or
                any(in_range(address, size, start, end) for start, end in read_ranges)):
            fail(f"candidate read escaped input/result/stack allowlists: {address:#x}+{size:#x} from {pc:#x}")
        reads.append({"pc": hex(pc), "address": hex(address), "size": size})

    def on_write(machine, _access, address, size, value, _user):
        pc = machine.reg_read(oracle.UC_X86_REG_EIP)
        if not candidate_pc(pc):
            fail(f"candidate data write from unknown code endpoint {pc:#x}")
        if not (in_range(address, size, STACK, STACK + 0x1000) or
                any(in_range(address, size, start, end) for start, end in write_ranges)):
            fail(f"candidate write escaped result/diagnostic/stack allowlists: {address:#x}+{size:#x} from {pc:#x}")
        writes.append({"pc": hex(pc), "address": hex(address), "size": size,
                       "value": hex(value & ((1 << (size * 8)) - 1))})

    uc.hook_add(oracle.UC_HOOK_MEM_READ, on_read)
    uc.hook_add(oracle.UC_HOOK_MEM_WRITE, on_write)

    start = oracle.find_export(exports, "hercules_batch_reference_run")
    sp = STACK + 0x700 - 4 * 6
    uc.mem_write(sp, struct.pack("<6I", SENTINEL, config_address, points_address,
                                 result_address, fault_address, raw_address))
    uc.reg_write(oracle.UC_X86_REG_ESP, sp)
    for name, register in oracle.REGS.items():
        if name in oracle.REG_SEED:
            uc.reg_write(register, oracle.REG_SEED[name])
    start_esp = uc.reg_read(oracle.UC_X86_REG_ESP)
    start_regs = {name: uc.reg_read(oracle.REGS[name]) for name in oracle.REG_SEED}
    uc.reg_write(oracle.UC_X86_REG_EIP, start)
    uc.emu_start(start, SENTINEL, count=50000)
    if uc.reg_read(oracle.UC_X86_REG_EIP) != SENTINEL:
        fail("candidate entry did not return to the synthetic sentinel")
    end_esp = uc.reg_read(oracle.UC_X86_REG_ESP)
    end_regs = {name: uc.reg_read(oracle.REGS[name]) for name in oracle.REG_SEED}
    if end_esp != start_esp + 4:
        fail(f"candidate changed caller-visible cdecl ESP: {start_esp:#x} -> {end_esp:#x}")
    if any(end_regs[name] != start_regs[name] for name in ("ebx", "ebp", "esi", "edi")):
        fail("candidate changed a cdecl callee-saved register")

    result_words = struct.unpack("<9I6H", bytes(uc.mem_read(result_address, 48)))
    camera, screen = result_words[:9], result_words[9:]
    fault_point = struct.unpack("<I", bytes(uc.mem_read(fault_address, 4)))[0]
    raw_eax = struct.unpack("<I", bytes(uc.mem_read(raw_address, 4)))[0]
    status = uc.reg_read(oracle.UC_X86_REG_EAX)
    return {
        "status": status,
        "fault_point": fault_point,
        "raw_eax": raw_eax,
        "camera_xyz_bits": list(camera),
        "projected_xy": list(screen),
        "caller_esp_delta": end_esp - start_esp,
        "callee_saved_preserved": True,
        "code_event_count": len(code_events),
        "data_read_count": len(reads),
        "data_write_count": len(writes),
        "candidate_output_writes": writes,
        "candidate_result_writes": [row for row in writes
                                    if result_address <= int(row["address"], 16) < result_address + 48],
        "candidate_diagnostic_writes": [row for row in writes
                                         if fault_address <= int(row["address"], 16) < raw_address + 4],
        "candidate_mapped_sections": mapped,
    }


def native_lane_sequence(output_writes, oracle):
    sequence = []
    for row in output_writes:
        address = int(row["address"], 16)
        value = int(row["value"], 16)
        tag = None
        for base, lane in ((oracle.CAMERA_X, "x"),
                           (oracle.CAMERA_Y, "y"),
                           (oracle.CAMERA_Z, "z")):
            if base <= address < base + 12:
                delta = address - base
                if row["size"] != 4 or delta % 4:
                    fail(f"unexpected native camera store: {row!r}")
                tag = (delta // 4, "camera_" + lane, value)
                break
        if tag is None:
            for base, lane in ((oracle.SCREEN_X, "x"), (oracle.SCREEN_Y, "y")):
                if base <= address < base + 6:
                    delta = address - base
                    if row["size"] != 2 or delta % 2:
                        fail(f"unexpected native screen store: {row!r}")
                    tag = (delta // 2, "screen_" + lane, value)
                    break
        if tag is None:
            fail(f"native output write is outside the five B320 output lanes: {row!r}")
        sequence.append(tag)
    return sequence


def candidate_lane_sequence(candidate_writes, result_address):
    sequence = []
    for row in candidate_writes:
        address = int(row["address"], 16)
        value = int(row["value"], 16)
        offset = address - result_address
        if 0 <= offset < 36 and row["size"] == 4 and offset % 4 == 0:
            index = offset // 4
            sequence.append((index // 3, "camera_" + ("x", "y", "z")[index % 3], value))
        elif 36 <= offset < 48 and row["size"] == 2 and (offset - 36) % 2 == 0:
            index = (offset - 36) // 2
            sequence.append((index // 2, "screen_" + ("x", "y")[index % 2], value))
        else:
            fail(f"unexpected candidate result store: {row!r}")
    return sequence


def verify_pe(oracle, path, expected_export):
    pe = oracle.PE(path)
    if pe.image_base != DLL_BASE or pe.pe.FILE_HEADER.Machine != 0x14c:
        fail("source-built PE has an unexpected image base or machine architecture")
    if getattr(pe.pe, "DIRECTORY_ENTRY_IMPORT", []):
        fail("source-built PE unexpectedly imports external code or data")
    reloc = pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5]
    if reloc.VirtualAddress != 0 or reloc.Size != 0:
        fail("source-built fixed-base PE unexpectedly contains relocations")
    exports = oracle.pe_exports(pe, DLL_BASE)
    oracle.find_export(exports, expected_export)
    return pe, exports


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    root, package, exe_path, output, candidate_files, protected_sources = guarded_paths(args)
    sys.path[:0] = [str(root / "renderer_contract" / "tests"), str(root / "scripts")]
    oracle = importlib.import_module("audit_batch_projection")
    oracle.bootstrap(root)
    exe = oracle.PE(exe_path)
    if sha(exe_path) != EXE_SHA256 or exe.image_base != oracle.IMAGE_BASE:
        fail("pinned original EXE hash or image base changed")
    if oracle.capstone.__version__ != "5.0.7":
        fail(f"expected Capstone 5.0.7, got {oracle.capstone.__version__}")
    runtime = oracle.verify_runtime(importlib.import_module("unicorn"), ORACLE_DEPS)
    spans, _instructions = oracle.decode_spans(exe)
    if spans["B320"]["sha256"] != B320_SHA256:
        fail("pinned B320 code span hash changed")

    native_source = protected_sources["native_observer.c"]
    native_build = output.parent / "compiled_native_observer"
    (native_obj, native_asm, native_obj_bytes, native_dll, native_map,
     native_compile, native_link, native_build_output, native_components) = oracle.compile_observer(
        native_source, native_build, root / "renderer_contract")
    native_pe, native_exports = verify_pe(oracle, native_dll,
        "batch_projection_observe_slot107")
    for symbol in ("batch_projection_set_saved_slot107", "batch_projection_diagnostic_state"):
        oracle.find_export(native_exports, symbol)

    candidate_source = candidate_files["projection_candidate_fixture.c"]
    candidate_build = output.parent / "compiled_candidate"
    (candidate_obj, candidate_asm, candidate_obj_bytes, candidate_dll, candidate_map,
     candidate_compile, candidate_link, candidate_build_output, candidate_components) = oracle.compile_observer(
        candidate_source, candidate_build, root / "renderer_contract")
    candidate_pe, candidate_exports = verify_pe(oracle, candidate_dll,
        "hercules_batch_reference_run")
    if len(candidate_pe.pe.DIRECTORY_ENTRY_EXPORT.symbols) != 1:
        fail("candidate PE must expose only the one diagnostic batch entry")

    vectors = batch_cases(oracle)
    vectors.append(trap_case())
    case_records = []
    for case in vectors:
        native = oracle.run_route(exe, native_pe, native_exports, case, "direct")
        native_camera, native_screen, native_output_writes = reconstruct_native(case, native, oracle)
        candidate = run_candidate(oracle, candidate_pe, candidate_exports, case)
        native_sequence = native_lane_sequence(native_output_writes, oracle)
        result_address = INPUT + RESULT_OFFSET
        candidate_sequence = candidate_lane_sequence(candidate["candidate_result_writes"], result_address)
        if candidate_sequence != native_sequence:
            fail(f"candidate/native output lane write prefix differs for {case['name']}: "
                 f"{candidate_sequence!r} != {native_sequence!r}")
        native_success = native["completed"]
        if native_success:
            expected_status = 0
            expected_fault = 3
            expected_raw = int(native["raw_eax"], 16)
            if candidate["status"] != expected_status or candidate["fault_point"] != expected_fault:
                fail(f"candidate status/fault index mismatch for {case['name']}")
            if candidate["raw_eax"] != expected_raw or candidate["raw_eax"] == RAW_EAX_CANARY:
                fail(f"candidate raw EAX output mismatch or was not written for {case['name']}")
        else:
            expected_status = 1
            expected_fault = 1
            expected_raw = RAW_EAX_CANARY
            if native.get("exception_pc") != hex(oracle.IMAGE_BASE + 0xB3F1):
                fail(f"native B320 case did not trap at IDIV for {case['name']}")
            if candidate["status"] != expected_status or candidate["fault_point"] != expected_fault:
                fail("candidate did not report the native point-1 IDIV overflow as non-success")
            if candidate["raw_eax"] != RAW_EAX_CANARY:
                fail("candidate overwrote raw_eax canary on non-success")
        if candidate["camera_xyz_bits"] != native_camera:
            fail(f"candidate/native camera XYZ mismatch for {case['name']}")
        if candidate["projected_xy"] != native_screen:
            fail(f"candidate/native projected XY mismatch for {case['name']}")
        seeded = struct.unpack("<9I6H", seed_result_bytes(case))
        camera_written = {point * 3 + lane for point, field, _value in candidate_sequence
                          for lane, suffix in enumerate(("camera_x", "camera_y", "camera_z"))
                          if field == suffix}
        screen_written = {point * 2 + lane for point, field, _value in candidate_sequence
                          for lane, suffix in enumerate(("screen_x", "screen_y"))
                          if field == suffix}
        untouched_camera_preserved = all(
            candidate["camera_xyz_bits"][index] == seeded[index]
            for index in range(9) if index not in camera_written)
        untouched_screen_preserved = all(
            candidate["projected_xy"][index] == seeded[9 + index]
            for index in range(6) if index not in screen_written)
        if not untouched_camera_preserved or not untouched_screen_preserved:
            fail(f"candidate overwrote a result canary outside native output prefix for {case['name']}")
        if not native_success and candidate["raw_eax"] != RAW_EAX_CANARY:
            fail("candidate wrote raw_eax on native trap")
        case_records.append({
            "name": case["name"],
            "input": {"matrix_words": case["matrix"],
                      "translation_dwords": case["translation"],
                      "center_dwords": case["center"], "focal_dword": case["focal"],
                      "source_signed_WORD_XYZ_triplets": [list(p) for p in case["points"]]},
            "native": {"completed": native_success,
                       "exception_pc": native.get("exception_pc"),
                       "idiv_reached": native.get("idiv_reached"),
                       "raw_eax": native.get("raw_eax"),
                       "camera_xyz_bits": [hex(v) for v in native_camera],
                       "projected_xy": [hex(v) for v in native_screen],
                       "output_write_count": len(native_output_writes),
                       "logical_output_lane_write_order": [list(row) for row in native_sequence],
                       "output_write_order_pc_address_size_value": [
                           [w["pc"], w["address"], w["size"], w["value"]]
                           for w in native_output_writes]},
            "candidate": {"status_EAX": candidate["status"],
                          "fault_point": candidate["fault_point"],
                          "raw_eax": hex(candidate["raw_eax"]),
                          "camera_xyz_bits": [hex(v) for v in candidate["camera_xyz_bits"]],
                          "projected_xy": [hex(v) for v in candidate["projected_xy"]],
                          "caller_esp_delta": candidate["caller_esp_delta"],
                          "callee_saved_preserved": candidate["callee_saved_preserved"],
                          "code_event_count": candidate["code_event_count"],
                          "data_read_count": candidate["data_read_count"],
                          "data_write_count": candidate["data_write_count"],
                          "result_output_write_count": len(candidate["candidate_result_writes"]),
                          "logical_output_lane_write_order": [list(row) for row in candidate_sequence],
                          "candidate_result_writes": candidate["candidate_result_writes"],
                          "candidate_diagnostic_writes": candidate["candidate_diagnostic_writes"],
                          "untouched_camera_canaries_preserved": untouched_camera_preserved,
                          "untouched_screen_canaries_preserved": untouched_screen_preserved},
            "comparison": {"camera_equal": True, "screen_equal": True,
                           "status_and_failure_index_match": True,
                           "output_lane_write_prefix_matches": True,
                           "raw_eax_matches_on_success": native_success,
                           "raw_eax_canary_preserved_on_fault": not native_success,
                           "native_output_write_prefix_reconstructed": True},
        })

    source_hashes = {name: sha(path) for name, path in candidate_files.items()}
    protected_hashes = {name: sha(path) for name, path in protected_sources.items()}
    native_span = spans["B320"]
    native_output_store_count = sum(row["native"]["output_write_count"] for row in case_records)
    candidate_result_store_count = sum(row["candidate"]["result_output_write_count"] for row in case_records)
    candidate_diagnostic_store_count = sum(len(row["candidate"]["candidate_diagnostic_writes"])
                                           for row in case_records)
    candidate_code_event_count = sum(row["candidate"]["code_event_count"] for row in case_records)
    candidate_read_count = sum(row["candidate"]["data_read_count"] for row in case_records)
    candidate_write_count = sum(row["candidate"]["data_write_count"] for row in case_records)
    report = {
        "schema_version": 1,
        "status": "PASS",
        "proof_level": "BOUNDED_ORIGINAL_X86_CANDIDATE_DIFFERENTIAL",
        "scope": "Five completed synthetic B320 cases and one point-1 signed IDIV overflow case; candidate C reference is compiled into an import-free fixed-base x86 PE and run in Unicorn. Original direct B430/B320/B5C0 route and candidate route receive identical config/point values.",
        "prediction": {"path": str(candidate_files["batch_reference_prediction.json"].relative_to(root)),
                       "sha256": PREDICTION_SHA256},
        "original": {"path": str(exe_path), "sha256": sha(exe_path),
                     "image_base": hex(exe.image_base), "B320_span": native_span,
                     "native_routes": ["B430", "B320", "B5C0"],
                     "native_completed_case_flags": {row["name"]: row["native"].get("completed")
                                                      for row in case_records}},
        "candidate_fixture": {
            "source_path": str(candidate_source.relative_to(root)) if candidate_source.is_relative_to(root) else str(candidate_source),
            "source_sha256": sha(candidate_source),
            "compiled_fixture_sha256": sha(candidate_dll),
            "compiled_object_sha256": hashlib.sha256(candidate_obj_bytes).hexdigest(),
            "compiled_map_sha256": sha(candidate_map),
            "text_assembly_sha256": sha(candidate_asm),
            "image_base": hex(candidate_pe.image_base),
            "imports": False, "relocations": False,
            "exports": ["hercules_batch_reference_run"],
            "compile_command": candidate_compile, "link_command": candidate_link,
            "build_output": candidate_build_output,
            "toolchain_component_sha256": candidate_components,
            "mapped_in_unicorn": True,
            "executed_candidate_code_event_count": candidate_code_event_count,
            "read_allowlist": ["config view", "three point views", "joined result view", "fault-point DWORD", "raw-EAX DWORD", "synthetic stack"],
            "write_allowlist": ["joined result view", "fault-point DWORD", "raw-EAX DWORD", "synthetic stack"],
            "unknown_PC_or_memory_endpoint": "fails"
        },
        "native_observer_fixture": {
            "source_path": str(native_source.relative_to(root)),
            "source_sha256": sha(native_source),
            "compiled_fixture_sha256": sha(native_dll),
            "compiled_object_sha256": hashlib.sha256(native_obj_bytes).hexdigest(),
            "compiled_map_sha256": sha(native_map),
            "toolchain_component_sha256": native_components,
            "purpose": "Existing pinned audit helper is used to run the unchanged original route; this receipt does not claim a fresh observer-vs-direct differential beyond the prior bounded fixture receipt."
        },
        "cases": case_records,
        "case_counts": {"completed_native_and_candidate_equal": 5,
                        "native_IDIV_traps_candidate_reports_non_success": 1,
                        "raw_eax_checked_on_success": 5,
                        "raw_eax_canary_checks_on_fault": 1},
        "execution_counts": {
            "native_completed_cases": 5,
            "native_IDIV_trap_cases": 1,
            "native_B320_output_stores": native_output_store_count,
            "candidate_PE_code_events": candidate_code_event_count,
            "candidate_PE_data_reads": candidate_read_count,
            "candidate_PE_data_writes_including_stack": candidate_write_count,
            "candidate_result_lane_stores": candidate_result_store_count,
            "candidate_diagnostic_stores": candidate_diagnostic_store_count,
            "native_and_candidate_fault_output_prefix_stores": case_records[-1]["native"]["output_write_count"]
                + case_records[-1]["candidate"]["result_output_write_count"]
        },
        "source_hashes": {**source_hashes, **{f"helper:{k}": v for k, v in protected_hashes.items()},
                           "runner": sha(Path(__file__)),
                           "shared_types": sha(protected_sources["hercules_types.h"]),
                           "shared_host_abi": sha(protected_sources["hercules_host_abi.h"])},
        "runtime": {"python": sys.version.split()[0], "capstone": oracle.capstone.__version__,
                    "unicorn": runtime,
                    "analysis_dependency_root": str(ANALYSIS_DEPS),
                    "oracle_dependency_root": str(ORACLE_DEPS)},
        "limits": [
            "The candidate entry has a test-fixture cdecl signature; this is not a historical B320 C declaration or host callback replacement.",
            "The result/config structs are tool-side joined views; original host arrays are separate lanes.",
            "Only five completed vectors plus one trap vector are differentially compared; this is not exhaustive all-input equivalence.",
            "No B740 Z clamp is applied. No viewport policy, culling, producer behavior, renderer, GPU, or pixels are covered.",
            "No original Windows process was started; the pinned HERCULES.EXE is only executed by Unicorn."
        ]
    }
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": report["status"], "output": str(output),
                      "case_counts": report["case_counts"],
                      "candidate_sha256": report["candidate_fixture"]["compiled_fixture_sha256"]}, indent=2))


if __name__ == "__main__":
    main()
