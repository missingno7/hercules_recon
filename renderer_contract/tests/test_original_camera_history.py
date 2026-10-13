#!/usr/bin/env python3
"""Boundedly execute two controlled calls to original ENG1 camera history code."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

from audit_temporal_state import (
    EXPECTED_ORIGINALS,
    evidence_file,
    file_sha,
    import_pe,
    validate_locations,
)

ORACLE_DEPS = Path(r"C:\tools\hercules\renderer-oracle-python")
FUNCTION_RVA = 0x2FB70
FUNCTION_END_RVA = 0x2FF4E
FUNCTION_SIZE = FUNCTION_END_RVA - FUNCTION_RVA
FUNCTION_SHA256 = "45ab59bf5e487e2613e042167de4f3c93d2b9b701f89c492c73893a2963b4fe6"
ENG1_SHA256 = EXPECTED_ORIGINALS["ENG1.DLL"][0]
EXPECTED_UNICORN_DLL_SHA256 = (
    "010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1"
)
BASE = 0x10000000
CODE_PAGE = 0x1002F000
CAMERA_PAGE = 0x10070000
STACK = 0x12000000
SENTINEL = 0x13000000


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_u32(machine, rva: int) -> int:
    return struct.unpack("<I", bytes(machine.mem_read(BASE + rva, 4)))[0]


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path,
                        help="explicit hash-pinned original HERCULES.EXE path")
    parser.add_argument("--eng1", required=True, type=Path,
                        help="explicit hash-pinned original ENG1.DLL path")
    parser.add_argument("--output", required=True, type=Path,
                        help="receipt path, lexically and physically under repo-root/work")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    locations = validate_locations(args.repo_root, args.eng1, args.exe, args.output)
    repo_root = locations["repo_resolved"]
    evidence_path = evidence_file()
    evidence = json.loads(evidence_path.read_text(encoding="utf-8-sig"))
    bounded = evidence["bounded_equivalent"]
    if bounded.get("classification") != "BOUNDED_EQUIVALENT":
        raise ValueError("controlled original execution must remain separately classified")
    if evidence["static_observed"].get("classification") != "STATIC_OBSERVED":
        raise ValueError("static evidence classification changed")

    prediction_path = Path(__file__).resolve().parents[1] / "fixtures" / "temporal" / "prediction.md"
    prediction_sha = file_sha(prediction_path)
    if prediction_sha != bounded.get("prediction_sha256"):
        raise ValueError(f"preregistered prediction hash mismatch: {prediction_sha}")
    for relative, expected_sha in evidence["static_observed"]["input_sha256"].items():
        actual_sha = file_sha(repo_root / Path(relative))
        if actual_sha != expected_sha:
            raise ValueError(f"evidence input hash mismatch for {relative}: {actual_sha}")

    PE = import_pe(repo_root)
    images = {}
    for label, path_entry in locations["original_paths"].items():
        image = PE(path_entry["resolved"])
        image_sha = sha256(image.data)
        expected_sha, expected_base = EXPECTED_ORIGINALS[label]
        expected = evidence["oracle_pins"][label]
        if image_sha != expected_sha or image_sha != expected["sha256"]:
            raise ValueError(f"{label} hash mismatch: {image_sha}")
        if image.image_base != expected_base or image.image_base != int(expected["image_base"], 16):
            raise ValueError(f"{label} image-base mismatch: {image.image_base:#x}")
        images[label] = {"image": image, "sha256": image_sha,
                         "path": path_entry["resolved"]}
    eng1 = images["ENG1.DLL"]["image"]
    eng1_path = images["ENG1.DLL"]["path"]
    image_sha = images["ENG1.DLL"]["sha256"]
    if image_sha != ENG1_SHA256 or eng1.image_base != BASE:
        raise ValueError("ENG1.DLL image identity changed after validation")
    raw = eng1.read_rva(FUNCTION_RVA, FUNCTION_SIZE)
    body_sha = sha256(raw)
    if len(raw) != FUNCTION_SIZE or body_sha != FUNCTION_SHA256:
        raise ValueError("ENG1 0x2fb70 body differs from the pinned RET-bounded span")

    sys.path[:0] = [str(ORACLE_DEPS), str(repo_root / "renderer_contract" / "tests")]
    import unicorn
    from oracle_common import verify_runtime
    from unicorn import (
        UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE,
        UC_MODE_32, UC_PROT_EXEC, UC_PROT_READ, UC_PROT_WRITE, Uc,
    )
    from unicorn.x86_const import (
        UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX,
        UC_X86_REG_EDI, UC_X86_REG_EDX, UC_X86_REG_EIP, UC_X86_REG_EFLAGS,
        UC_X86_REG_ESI, UC_X86_REG_ESP,
    )

    runtime = verify_runtime(unicorn, ORACLE_DEPS)
    if runtime["native_dll_sha256"] != EXPECTED_UNICORN_DLL_SHA256:
        raise ValueError("Unicorn native DLL differs from the pinned existing runtime")

    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(CODE_PAGE, 0x1000, UC_PROT_READ | UC_PROT_WRITE)
    machine.mem_write(CODE_PAGE, eng1.read_rva(0x2F000, 0x1000))
    machine.mem_protect(CODE_PAGE, 0x1000, UC_PROT_READ | UC_PROT_EXEC)
    machine.mem_map(CAMERA_PAGE, 0x1000, UC_PROT_READ | UC_PROT_WRITE)
    machine.mem_map(STACK, 0x10000, UC_PROT_READ | UC_PROT_WRITE)

    approved_reads = {
        (0x70CA4, 4), (0x70CA0, 4), (0x70D10, 1),
        (0x70CFD, 1), (0x70CA8, 4),
    }
    approved_writes = {
        (0x70CC8, 4), (0x70CC4, 4), (0x70CB0, 4),
        (0x70CAC, 4), (0x70CB4, 4),
    }
    trace: list[int] = []
    case_writes: list[list[dict]] = [[] for _ in bounded["controlled_cases"]]
    observed_cases: list[dict] = []
    active_case = -1

    def on_code(_machine, address, _size, _user):
        rva = address - BASE
        if not FUNCTION_RVA <= rva < FUNCTION_END_RVA:
            raise RuntimeError(f"execution escaped pinned camera routine at VA {address:#x}")
        trace.append(rva)
        if len(trace) > len(case_writes) * 256:
            raise RuntimeError("camera probe exceeded its instruction budget")

    def on_mem_read(_machine, _access, address, size, _value, _user):
        if CAMERA_PAGE <= address < CAMERA_PAGE + 0x1000:
            cell = (address - BASE, size)
            if cell not in approved_reads:
                raise RuntimeError(f"unapproved camera-page read at {address:#x}, size {size}")

    def on_mem_write(_machine, _access, address, size, value, _user):
        nonlocal active_case
        if CAMERA_PAGE <= address < CAMERA_PAGE + 0x1000:
            cell = (address - BASE, size)
            if cell not in approved_writes:
                raise RuntimeError(f"unapproved camera-page write at {address:#x}, size {size}")
            if active_case < 0:
                raise RuntimeError("camera write occurred outside a controlled invocation")
            case_writes[active_case].append({
                "rva": hex(address - BASE),
                "size": size,
                "value_u32_hex": hex(value & 0xFFFFFFFF),
            })

    machine.hook_add(UC_HOOK_CODE, on_code)
    machine.hook_add(UC_HOOK_MEM_READ, on_mem_read)
    machine.hook_add(UC_HOOK_MEM_WRITE, on_mem_write)

    cases = bounded["controlled_cases"]
    for index, case in enumerate(cases):
        active_case = index
        x, y, z = (int(value, 16) for value in case["camera_xyz_u32"])
        machine.mem_write(BASE + 0x70CA0, struct.pack("<3I", x, y, z))
        machine.mem_write(BASE + 0x70CFD, b"\x00")
        machine.mem_write(BASE + 0x70D10, b"\x00\x00")
        before = [int(value, 16) for value in case["prior_xy_before_u32"]]
        if index == 0:
            machine.mem_write(BASE + 0x70CC4, struct.pack("<I", before[0]))
            machine.mem_write(BASE + 0x70CC8, struct.pack("<I", before[1]))
        actual_before = [read_u32(machine, rva) for rva in (0x70CC4, 0x70CC8)]
        if actual_before != before:
            raise RuntimeError(f"controlled case {index} prior seed mismatch: {actual_before!r}")

        stack_pointer = STACK + 0x8000 - 4
        machine.mem_write(stack_pointer, struct.pack("<I", SENTINEL))
        machine.reg_write(UC_X86_REG_ESP, stack_pointer)
        machine.reg_write(UC_X86_REG_EIP, BASE + FUNCTION_RVA)
        machine.reg_write(UC_X86_REG_EFLAGS, 0x202)
        for register, value in (
            (UC_X86_REG_EAX, 0xA1A1A1A1), (UC_X86_REG_EBX, 0xB2B2B2B2),
            (UC_X86_REG_ECX, 0xC3C3C3C3), (UC_X86_REG_EDX, 0xD4D4D4D4),
            (UC_X86_REG_ESI, 0xE5E5E5E5), (UC_X86_REG_EDI, 0xF6F6F6F6),
            (UC_X86_REG_EBP, 0x97979797),
        ):
            machine.reg_write(register, value)
        trace_start = len(trace)
        machine.emu_start(BASE + FUNCTION_RVA, SENTINEL, count=256)
        if machine.reg_read(UC_X86_REG_EIP) != SENTINEL:
            raise RuntimeError(f"camera routine did not return on controlled case {index}")

        case_trace = trace[trace_start:]
        if (len(case_trace) != 32 or not case_trace or case_trace[0] != FUNCTION_RVA
                or case_trace[-1] != 0x2FF4D):
            raise RuntimeError(f"unexpected entry/return trace in case {index}: {case_trace!r}")
        if 0x2FBAC in case_trace or 0x2FBFD in case_trace:
            raise RuntimeError(f"controlled case reached helper/follow body: {case_trace!r}")
        branch_path = [hex(rva) for rva in case_trace if rva in (0x2FBCB, 0x2FF23)]
        if branch_path != case["branch_path"]:
            raise RuntimeError(f"controlled branch path differs in case {index}: {branch_path!r}")

        writes = case_writes[index]
        expected_writes = case["writes"]
        if len(writes) != len(expected_writes):
            raise RuntimeError(f"controlled write count differs in case {index}: {writes!r}")
        for actual, expected in zip(writes, expected_writes):
            if (int(actual["rva"], 16) != int(expected[0], 16)
                    or actual["size"] != 4
                    or int(actual["value_u32_hex"], 16) != int(expected[1], 16)):
                raise RuntimeError(f"controlled writes differ in case {index}: {writes!r}")

        current_after = [read_u32(machine, rva) for rva in (0x70CA0, 0x70CA4, 0x70CA8)]
        prior_after = [read_u32(machine, rva) for rva in (0x70CC4, 0x70CC8)]
        if current_after != [x, y, z] or prior_after != [x, y]:
            raise RuntimeError(f"controlled case {index} state mismatch")
        expected_current = [int(value, 16) for value in case["current_xyz_after_u32"]]
        expected_prior = [int(value, 16) for value in case["prior_xy_after_u32"]]
        if current_after != expected_current or prior_after != expected_prior:
            raise RuntimeError(f"controlled case {index} diverged from evidence inventory")
        observed_cases.append({
            "camera_xyz_u32": [f"0x{value:08x}" for value in (x, y, z)],
            "prior_xy_before_u32": [f"0x{value:08x}" for value in before],
            "executed_instruction_count": len(case_trace),
            "branch_path": branch_path,
            "writes": [
                [row["rva"], f"0x{int(row['value_u32_hex'], 16):08x}"]
                for row in writes
            ],
            "current_xyz_after_u32": [f"0x{value:08x}" for value in current_after],
            "prior_xy_after_u32": [f"0x{value:08x}" for value in prior_after],
            "returned_inside_body": True,
        })

    output = locations["output_resolved"]
    output.parent.mkdir(parents=True, exist_ok=True)
    post = validate_locations(args.repo_root, args.eng1, args.exe, args.output)
    if post["output_resolved"] != output:
        raise ValueError("output path changed during test setup")
    receipt = {
        "schema": "renderer_temporal_camera_history_replay_v1",
        "status": "PASS_BOUNDED_EQUIVALENT",
        "classification": "BOUNDED_EQUIVALENT",
        "repo_root": {"lexical": str(locations["repo_lexical"]),
                      "resolved": str(locations["repo_resolved"])},
        "output": str(output),
        "prediction_fixture": {"path": str(prediction_path), "sha256": prediction_sha},
        "original": {
            "path": str(eng1_path), "sha256": image_sha,
            "image_base": hex(eng1.image_base), "routine_rva": hex(FUNCTION_RVA),
            "routine_end_rva_exclusive": hex(FUNCTION_END_RVA),
            "routine_size": FUNCTION_SIZE, "routine_sha256": body_sha,
            "code_patched": False,
        },
        "runtime": {
            "python_path": sys.executable,
            "python_version": sys.version.split()[0],
            **runtime,
            "instruction_budget_per_call": 256,
            "executed_instruction_count_total": len(trace),
            "out_of_body_instructions": 0,
            "executed_external_calls": 0,
            "other_original_images_or_game_process": False,
        },
        "controlled_cases": observed_cases,
        "scope": bounded["scope"],
        "limits": bounded["confidence_limits"],
    }
    output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": receipt["status"], "controlled_calls": len(cases),
                      "instructions": len(trace), "external_calls": 0,
                      "output": str(output)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
