#!/usr/bin/env python3
"""Compile the bounded C seam and compare it with original ENG3 machine-code cases."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

EXE_SHA = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
ENG3_SHA = "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7"
SEAM_SHA = "9c35f6c653549e59c844d647cf17c1b943dd4e085bb6317691a8fd65fca4a206"
EXE_BASE = 0x00400000
ENG_BASE = 0x10000000
TABLE_BASE = 0x21000000
STATE_BASE = 0x22000000
OBJECT = STATE_BASE + 0x2000
GEOMETRY = STATE_BASE + 0x3000
STACK = 0x0E000000
STACK_SIZE = 0x10000
STACK_TOP = STACK + STACK_SIZE - 0x100
SENTINEL = 0x0F000000
PYTHON_DEPS = Path(r"C:\tools\hercules\python")
ORACLE_DEPS = Path(r"C:\tools\hercules\renderer-oracle-python")
GCC_DEFAULT = Path(r"C:\msys64\mingw64\bin\gcc.exe")
CL_DEFAULT = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953\bin\cl.exe")
CL_SHA = "bf9f9c74f756fed96e13f7f9a4273495c7dde0a1fb968e3ef6d760ad6d73dfeb"
VC5_TREE_SHA = "29fc5aea67aa10a8135d65f8f371f7456bfc10ec8ef36dc0050ca5406bf9d92e"
CALLBACK_TABLE_SHA = "52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8"
CALLBACK_SLOTS = {
    101: (0x10040934, 0xB6F0), 106: (0x10040948, 0xB920),
    108: (0x10040950, 0xB880), 109: (0x10040954, 0xB8F0),
    114: (0x10040968, 0xB720), 121: (0x10040984, 0xB870),
    164: (0x10040A30, 0xBB00), 165: (0x10040A34, 0xBC30),
}
CALLBACK_RANGES = (
    (0xB6F0, 0x24), (0xB720, 0x1A), (0xB870, 0x10),
    (0xB880, 0x32), (0xB8F0, 0x22), (0xB920, 0x4E),
    (0xBB00, 0x12C), (0xBC30, 0x9F),
)
ENG3_SPANS = {
    "interval_row_writer_204c0": (0x204C0, 0x22A, "d7fdaeaa92e928fec0bbd482674672c662e03a822fa8e3b9c891320520c5a01f"),
    "indexed_parent_predicate_24a80": (0x24A80, 0x6F, "3a9fc30527d0f722cd6a1e16e8f5c5ba3b7f3e8bafa3fefc6ab2c1a172e89e85"),
    "parent_transform_helper_24bb0": (0x24BB0, 0x39B, "a0ad5e76036c8f94d63730e28691147bf7b8b80d64e2d9dc4a767c96868f026a"),
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def file_sha(path: Path) -> str:
    return sha(path.read_bytes())


def check_output_path(raw: str, repo: Path, inputs: list[Path]) -> Path:
    supplied = Path(raw)
    lexical = Path(os.path.abspath(supplied if supplied.is_absolute() else repo / supplied))
    resolved = lexical.resolve(strict=False)
    work_lexical = Path(os.path.abspath(repo / "work"))
    work_resolved = (repo / "work").resolve()
    if not lexical.is_relative_to(work_lexical) or not resolved.is_relative_to(work_resolved):
        raise ValueError("--output must stay lexically and physically beneath <repo-root>/work")
    if lexical.suffix.lower() != ".json" or lexical.exists() or lexical.is_symlink():
        raise ValueError("--output must be a fresh, nonexistent .json file")
    for item in inputs:
        if resolved == item.resolve() or resolved.is_relative_to(item.resolve().parent):
            raise ValueError("--output may not alias or be placed beneath an input's directory")
    return lexical


def load_runtime(args):
    sys.path[:0] = [str(args.python_deps), str(args.repo_root / "scripts"), str(args.oracle_deps)]
    from pe import PE
    import unicorn
    from unicorn.x86_const import (
        UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
        UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP,
        UC_X86_REG_EIP,
    )
    registers = {"eax": UC_X86_REG_EAX, "ebx": UC_X86_REG_EBX,
                 "ecx": UC_X86_REG_ECX, "edx": UC_X86_REG_EDX,
                 "ebp": UC_X86_REG_EBP, "esi": UC_X86_REG_ESI,
                 "edi": UC_X86_REG_EDI, "esp": UC_X86_REG_ESP,
                 "eip": UC_X86_REG_EIP}
    exe, eng3 = PE(args.exe), PE(args.eng3)
    if exe.image_base != EXE_BASE or eng3.image_base != ENG_BASE:
        raise RuntimeError("pinned preferred image base mismatch")
    for label, image, spans in (("ENG3", eng3, ENG3_SPANS),):
        for name, (rva, length, expected) in spans.items():
            actual = sha(image.read_rva(rva, length))
            if actual != expected:
                raise RuntimeError(f"{label} span {name} SHA-256 mismatch: {actual}")
    table = exe.read_rva(0x361B8, 0x3E8)
    if sha(table) != CALLBACK_TABLE_SHA:
        raise RuntimeError("pinned HERC callback table hash mismatch")
    return exe, eng3, unicorn, registers


def map_image(uc, image, base: int, unicorn) -> None:
    size = image.pe.OPTIONAL_HEADER.SizeOfImage
    mapped = (size + 0xFFF) & ~0xFFF
    uc.mem_map(base, mapped, unicorn.UC_PROT_ALL)
    uc.mem_write(base, image.pe.get_memory_mapped_image()[:size])


def u32(uc, address: int) -> int:
    return int.from_bytes(bytes(uc.mem_read(address, 4)), "little")


def put16(uc, address: int, value: int) -> None:
    uc.mem_write(address, (value & 0xFFFF).to_bytes(2, "little"))


def put32(uc, address: int, value: int) -> None:
    uc.mem_write(address, (value & 0xFFFFFFFF).to_bytes(4, "little"))


def new_machine(exe, eng3, unicorn):
    uc = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_32)
    map_image(uc, exe, EXE_BASE, unicorn)
    map_image(uc, eng3, ENG_BASE, unicorn)
    uc.mem_map(STACK, STACK_SIZE, unicorn.UC_PROT_ALL)
    uc.mem_map(SENTINEL, 0x1000, unicorn.UC_PROT_ALL)
    uc.mem_write(SENTINEL, b"\xf4")
    return uc


class BoundedTrace:
    def __init__(self, uc, allowed, unicorn, regs):
        self.uc = uc
        self.allowed = allowed
        self.regs = regs
        self.pcs = []
        self.unexpected_pc = None
        self.invalid = None
        self.hooks = [uc.hook_add(unicorn.UC_HOOK_CODE, self.on_code),
                      uc.hook_add(unicorn.UC_HOOK_MEM_INVALID, self.on_invalid)]

    def on_code(self, uc, address, size, user):
        if address == SENTINEL:
            uc.emu_stop()
            return
        if not any(lo <= address < hi for lo, hi in self.allowed):
            self.unexpected_pc = address
            uc.emu_stop()
            return
        self.pcs.append(address)

    def on_invalid(self, uc, access, address, size, value, user):
        self.invalid = {"pc": hex(uc.reg_read(self.regs["eip"])),
                        "address": hex(address), "size": size,
                        "access": access}
        return False

    def close(self):
        for hook in self.hooks:
            self.uc.hook_del(hook)


def invoke(uc, trace, start, args, regs, unicorn):
    put32(uc, STACK_TOP, SENTINEL)
    for index, value in enumerate(args):
        put32(uc, STACK_TOP + 4 + index * 4, value)
    uc.reg_write(regs["esp"], STACK_TOP)
    uc.reg_write(regs["eip"], start)
    error = None
    try:
        uc.emu_start(start, SENTINEL, count=2_000_000)
    except unicorn.UcError as exc:
        error = str(exc)
    returned = uc.reg_read(regs["eip"]) == SENTINEL
    if not error and trace.unexpected_pc is None and trace.invalid is None and not returned:
        error = "instruction limit reached before sentinel return"
    return {"returned_to_sentinel": returned,
            "eax": hex(uc.reg_read(regs["eax"])),
            "esp": hex(uc.reg_read(regs["esp"])),
            "instructions": len(trace.pcs),
            "unexpected_pc": hex(trace.unexpected_pc) if trace.unexpected_pc is not None else None,
            "invalid_memory": trace.invalid, "error": error}


def writer_run(exe, eng3, unicorn, regs, lower: int, extent: int):
    uc = new_machine(exe, eng3, unicorn)
    uc.mem_map(TABLE_BASE, 0x1000, unicorn.UC_PROT_ALL)
    uc.mem_map(STATE_BASE, 0x10000, unicorn.UC_PROT_ALL)
    put32(uc, ENG_BASE + 0x4078C, TABLE_BASE)
    uc.mem_write(ENG_BASE + 0x40638, b"\x00")
    uc.mem_write(STATE_BASE + 0x1000, b"\x00" * 16)
    trace = BoundedTrace(uc, [(ENG_BASE + 0x204C0, ENG_BASE + 0x206EA)], unicorn, regs)
    result = invoke(uc, trace, ENG_BASE + 0x204C0,
                    (STATE_BASE, STATE_BASE + 0x1000, 0, lower, extent, 0), regs, unicorn)
    row = bytes(uc.mem_read(TABLE_BASE, 12))
    out = {"execution": result, "row_bytes_hex": row.hex(),
           "row_u32": [int.from_bytes(row[i:i + 4], "little") for i in (0, 4, 8)],
           "completed_exact_writer_span": result["returned_to_sentinel"] and
               trace.unexpected_pc is None and trace.invalid is None}
    trace.close()
    return out


def install_callbacks(uc, exe):
    table = exe.read_rva(0x361B8, 0x3E8)
    installed = {}
    for slot, (cell, target_rva) in CALLBACK_SLOTS.items():
        source_target = int.from_bytes(table[slot * 4:slot * 4 + 4], "little")
        expected = EXE_BASE + target_rva
        if source_target != expected:
            raise RuntimeError(f"source callback slot {slot} target mismatch")
        put32(uc, cell, source_target)
        installed[str(slot)] = hex(source_target)
    return installed


def predicate_run(exe, eng3, unicorn, regs, row: bytes, label: str, coord: int):
    uc = new_machine(exe, eng3, unicorn)
    uc.mem_map(TABLE_BASE, 0x1000, unicorn.UC_PROT_ALL)
    uc.mem_map(STATE_BASE, 0x10000, unicorn.UC_PROT_ALL)
    uc.mem_write(TABLE_BASE, row)
    put32(uc, ENG_BASE + 0x4078C, TABLE_BASE)
    put32(uc, ENG_BASE + 0x40540, GEOMETRY)
    uc.mem_write(OBJECT, b"\x00" * 0x200)
    uc.mem_write(GEOMETRY, b"\x00" * 0x100)
    high_word = (coord * 128) & 0xFFFF
    object_bits = high_word << 16
    put32(uc, OBJECT, object_bits)
    put16(uc, OBJECT + 0x38, 0)
    installed = install_callbacks(uc, exe)
    allowed = [(ENG_BASE + 0x24A80, ENG_BASE + 0x24AEF),
               (ENG_BASE + 0x24BB0, ENG_BASE + 0x24F4B)]
    for start, length in ((0x1FD80, 0x0F), (0x1FDD0, 0x0F),
                          (0x1FCA0, 0x0F), (0x1FD00, 0x06)):
        allowed.append((ENG_BASE + start, ENG_BASE + start + length))
    allowed.extend((EXE_BASE + start, EXE_BASE + start + length)
                   for start, length in CALLBACK_RANGES)
    trace = BoundedTrace(uc, allowed, unicorn, regs)
    result = invoke(uc, trace, ENG_BASE + 0x24A80, (OBJECT,), regs, unicorn)
    reached = ENG_BASE + 0x24BB0 in trace.pcs or EXE_BASE + 0xB6F0 in trace.pcs
    trig_pointer = u32(uc, ENG_BASE + 0x407B4)
    if result["returned_to_sentinel"] and result["eax"] == "0xffffffff" and not reached:
        outcome = "REJECT_BEFORE_24BB0"
    elif reached and trig_pointer == 0 and result["invalid_memory"] is not None and \
            result["invalid_memory"]["address"] == "0x0" and \
            result["invalid_memory"]["pc"] == hex(ENG_BASE + 0x24C49):
        outcome = "REACHED_24BB0_THEN_UNRESOLVED_AT_407B4_NULL"
    else:
        outcome = "UNEXPECTED_OR_UNRESOLVED"
    out = {"case": label, "requested_coordinate": coord,
           "object_dword_hex": hex(object_bits), "decoded_coordinate_candidate":
               (((high_word - 0x10000) if high_word & 0x8000 else high_word) // 128),
           "native_outcome": outcome, "execution": result,
           "reached_24bb0": reached,
           "callback_targets_installed_from_pinned_original_table": installed,
           "uninitialized_trig_table_global": hex(trig_pointer)}
    trace.close()
    return out


def compile_and_run(source_dir: Path, work_dir: Path, gcc: Path, cl: Path):
    gcc = gcc.resolve()
    cl = cl.resolve()
    if not gcc.is_file() or not cl.is_file():
        raise RuntimeError("both GCC and the pinned VC5 cl.exe must exist")
    if file_sha(cl) != CL_SHA:
        raise RuntimeError("VC5 cl.exe does not match the pinned MSVC 5.0 RTM compiler")
    gcc_version = subprocess.run([str(gcc), "--version"], text=True,
                                 capture_output=True, check=True).stdout.splitlines()[0]
    seam = source_dir / "eng3_visibility_seam.c"
    harness = source_dir / "test_eng3_visibility_source.c"
    gcc_exe = work_dir / "eng3_visibility_gcc.exe"
    gcc_cmd = [str(gcc), "-std=c89", "-Wall", "-Wextra", "-Werror", "-O2",
               str(seam), str(harness), "-o", str(gcc_exe)]
    gcc_build = subprocess.run(gcc_cmd, cwd=work_dir, text=True, capture_output=True)
    if gcc_build.returncode:
        raise RuntimeError("GCC build failed:\n" + gcc_build.stdout + gcc_build.stderr)
    gcc_run = subprocess.run([str(gcc_exe)], cwd=work_dir, text=True, capture_output=True)
    if gcc_run.returncode or "C_SOURCE_FIXTURE_OK" not in gcc_run.stdout:
        raise RuntimeError("GCC fixture failed:\n" + gcc_run.stdout + gcc_run.stderr)

    vc_exe = work_dir / "eng3_visibility_vc5.exe"
    obj_dir = work_dir / "vc5_obj"
    obj_dir.mkdir()
    vc_root = cl.parent.parent
    cl_cmd = [str(cl), "/nologo", "/W4", "/WX", "/O2",
              "/I" + str(vc_root / "include"),
              "/Fe" + str(vc_exe), "/Fo" + str(obj_dir) + "\\",
              str(seam), str(harness), "/link", "/LIBPATH:" + str(vc_root / "lib")]
    cl_build = subprocess.run(cl_cmd, cwd=work_dir, text=True, capture_output=True)
    if cl_build.returncode:
        raise RuntimeError("pinned VC5 build failed:\n" + cl_build.stdout + cl_build.stderr)
    vc_run = subprocess.run([str(vc_exe)], cwd=work_dir, text=True, capture_output=True)
    if vc_run.returncode or "C_SOURCE_FIXTURE_OK" not in vc_run.stdout:
        raise RuntimeError("VC5 fixture failed:\n" + vc_run.stdout + vc_run.stderr)
    return {
        "gcc": {"path": str(gcc), "sha256": file_sha(gcc), "version": gcc_version,
                "command": gcc_cmd,
                "build_stdout": gcc_build.stdout + gcc_build.stderr,
                "run_stdout": gcc_run.stdout, "run_stderr": gcc_run.stderr},
        "vc5": {"path": str(cl), "sha256": file_sha(cl),
                "toolchain_bundle_commit": "8abf95ce980161ad87b0b02402269cce76988953",
                "toolchain_bundle_tree_sha256": VC5_TREE_SHA, "command": cl_cmd,
                "build_stdout": cl_build.stdout + cl_build.stderr,
                "run_stdout": vc_run.stdout, "run_stderr": vc_run.stderr},
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--eng3", required=True, type=Path)
    parser.add_argument("--output", required=True)
    parser.add_argument("--python-deps", type=Path, default=PYTHON_DEPS)
    parser.add_argument("--oracle-deps", type=Path, default=ORACLE_DEPS)
    parser.add_argument("--gcc", type=Path, default=GCC_DEFAULT)
    parser.add_argument("--cl", type=Path, default=CL_DEFAULT)
    args = parser.parse_args()
    args.repo_root = args.repo_root.resolve()
    args.exe = args.exe.resolve()
    args.eng3 = args.eng3.resolve()
    source_dir = args.repo_root / "renderer_contract" / "fixtures" / "eng3_visibility_source"
    seam_path = source_dir / "eng3_visibility_seam.c"
    harness_path = source_dir / "test_eng3_visibility_source.c"
    if not (args.repo_root / "scripts/pe.py").is_file():
        parser.error("--repo-root must contain scripts/pe.py")
    if not args.python_deps.is_dir() or not args.oracle_deps.is_dir():
        parser.error("pinned Capstone/PE and Unicorn dependency directories must exist")
    inputs = [args.exe, args.eng3, seam_path, harness_path, Path(__file__).resolve()]
    output = check_output_path(args.output, args.repo_root, inputs)
    for path, expected, label in ((args.exe, EXE_SHA, "HERCULES.EXE"),
                                  (args.eng3, ENG3_SHA, "ENG3.DLL"),
                                  (seam_path, SEAM_SHA, "readable C seam")):
        actual = file_sha(path)
        if actual != expected:
            raise RuntimeError(f"{label} SHA-256 mismatch: {actual}")
    if not harness_path.is_file():
        raise RuntimeError("fixture harness missing")
    work_root = args.repo_root / "work"
    work_root.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="eng3_visibility_source_", dir=work_root) as temporary:
        build_dir = Path(temporary).resolve()
        if not build_dir.is_relative_to(work_root.resolve()):
            raise RuntimeError("temporary compiler outputs escaped work/")
        compilers = compile_and_run(source_dir, build_dir, args.gcc, args.cl)
    exe, eng3, unicorn, regs = load_runtime(args)
    normal = writer_run(exe, eng3, unicorn, regs, 10, 10)
    wrap = writer_run(exe, eng3, unicorn, regs, 0x7FFFFFF8, 0x10)
    if not normal["completed_exact_writer_span"] or not wrap["completed_exact_writer_span"]:
        raise RuntimeError("original writer did not finish inside its exact span")
    if normal["row_bytes_hex"] != "000000000a00000014000000":
        raise RuntimeError("original normal writer bytes falsified the C candidate")
    if wrap["row_bytes_hex"][8:] != "f8ffff7f08000080":
        raise RuntimeError("original endpoint wrap bytes falsified the C candidate")
    cases = [("lower_minus_one", 9), ("lower", 10),
             ("upper_minus_one", 19), ("upper", 20), ("negative_one", -1)]
    boundaries = [predicate_run(exe, eng3, unicorn, regs, bytes.fromhex(normal["row_bytes_hex"]),
                                label, coord) for label, coord in cases]
    expected_outcomes = ["REJECT_BEFORE_24BB0",
                         "REACHED_24BB0_THEN_UNRESOLVED_AT_407B4_NULL",
                         "REACHED_24BB0_THEN_UNRESOLVED_AT_407B4_NULL",
                         "REJECT_BEFORE_24BB0", "REJECT_BEFORE_24BB0"]
    if [case["native_outcome"] for case in boundaries] != expected_outcomes:
        raise RuntimeError("original predicate route diverged from pre-recorded prediction")
    if any(case["execution"]["unexpected_pc"] is not None for case in boundaries):
        raise RuntimeError("predicate reached an unapproved code address")
    output_obj = {
        "schema_version": 1,
        "proofs": {
            "binary_span_identity": "STATIC_OBSERVED",
            "bounded_source_seam": "BOUNDED_EQUIVALENT",
            "native_pretransform_branches": "BEHAVIOR_VERIFIED",
            "positive_transform_tail": "UNRESOLVED",
            "full_gate4_live_bounds_and_emission": "UNRESOLVED"
        },
        "scope": "Controlled row endpoint stores, modulo-32-bit wrap, signed high-WORD SAR7 decoding, half-open signed interval predicate, four requested native boundaries plus one negative shift control. No live resource/table initialization and no positive transform success are claimed.",
        "prediction_falsifier": {"path": str(source_dir / "prediction_falsifier.md"),
                                 "sha256": file_sha(source_dir / "prediction_falsifier.md")},
        "originals": {"HERCULES.EXE": {"path": str(args.exe), "sha256": EXE_SHA},
                      "ENG3.DLL": {"path": str(args.eng3), "sha256": ENG3_SHA}},
        "readable_source": {"path": str(seam_path), "sha256": SEAM_SHA,
                            "harness_sha256": file_sha(harness_path)},
        "compiler_fixtures": compilers,
        "native_writer": {"normal_lower_extent": [10, 10], "normal": normal,
                          "wrap_lower_extent_hex": ["0x7ffffff8", "0x10"], "wrap": wrap},
        "native_predicate_cases": boundaries,
        "interpretation": {
            "bounded_source_seam": "The C writer's +4/+8 DWORD endpoints and modulo-2^32 addition agree byte-for-byte with original 0x204C0 for the controlled zero-count path. The +0 selector is outside the C helper; only preservation is tested in C. The C coordinate/predicate route agrees with original 0x24A80's branch before transform entry for 9, 20, and -1, and the branch reaches 0x24BB0 for 10 and 19.",
            "positive_tail": "The 10/19 original cases enter 0x24BB0 and stop at instruction 0x10024C49 reading through ENG3 global 0x100407B4, whose controlled value is null. These cases remain UNRESOLVED after predicate entry. No success stub or fabricated scene/trig table is supplied.",
            "limits": ["Controlled row arguments are not a real Cyclops runtime bounds table.", "This does not run 0x20190 or the bounds initializer successfully; the earlier null allocation at 0x40540 remains open.", "The source helper is a bounded semantic candidate, not a complete recovered 0x204C0/0x24A80 implementation."]
        }
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(output_obj, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(output), "gcc_fixture": "PASS", "vc5_fixture": "PASS",
                      "normal_writer_bytes": normal["row_bytes_hex"],
                      "wrap_writer_bytes": wrap["row_bytes_hex"],
                      "native_decisions": [x["native_outcome"] for x in boundaries]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
