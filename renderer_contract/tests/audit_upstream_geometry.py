#!/usr/bin/env python3
"""Rerunnable static upstream geometry audit plus a bounded host-callback fixture."""
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

ROOT: Path | None = None
ANALYSIS_DEPS = Path(r"C:\tools\hercules\python")
ORACLE_DEPS = Path(r"C:\tools\hercules\renderer-oracle-python")
PE = capstone = unicorn = Uc = verify_runtime = None
UC_ARCH_X86 = UC_HOOK_CODE = UC_HOOK_MEM_WRITE = UC_MODE_32 = None
UC_PROT_EXEC = UC_PROT_READ = UC_PROT_WRITE = None
UC_X86_REG_EIP = UC_X86_REG_ESP = None

IMAGE_BASES = {"HERCULES.EXE": 0x400000, "ENG1.DLL": 0x10000000,
               "ENG3.DLL": 0x10000000, "TITLE.DLL": 0x10000000}
IMAGE_HASHES = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "ENG3.DLL": "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
    "TITLE.DLL": "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
}
HOST_TABLE_RVA = 0x361B8
HOST_TABLE_BYTES = 1000
HOST_TABLE_SHA = "52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8"

# Span lengths end on a RET/JMP boundary or a data-table boundary. Existing
# public receipts supply the source leads; this audit recomputes every span.
SPANS = {
    "HERCULES.EXE": {
        "slot102_three_word_inputs_B430": (0xB430, 0x6A, "code"),
        "slot107_bulk_transform_project_B320": (0xB320, 0x10F, "code"),
        "ENG3_global_transform_matrix_words": (0x394B8, 18, "data"),
        "slot111_camera_xyz_copy_B2F0": (0xB2F0, 0x1F, "code"),
        "slot117_FT4_three_vertex_copy_B5C0": (0xB5C0, 0x47, "code"),
        "slot101_word_vector_input_B6F0": (0xB6F0, 0x24, "code"),
        "slot104_projected_area_B4B0": (0xB4B0, 0x62, "code"),
        "slot106_transform_project_B920": (0xB920, 0x4E, "code"),
        "slot114_screen_xy_output_B720": (0xB720, 0x1A, "code"),
        "slot121_depth_word_B870": (0xB870, 0x10, "code"),
        "slot103_camera_xyz_to_translation_B840": (0xB840, 0x23, "code"),
        "slot108_rotation_matrix_B880": (0xB880, 0x32, "code"),
        "slot110_translation_B8C0": (0xB8C0, 0x28, "code"),
        "matrix_transform_B740": (0xB740, 0xCA, "code"),
        "projection_center_B970": (0xB970, 0x14, "code"),
        "projection_focal_B990": (0xB990, 0x0A, "code"),
        "slot164_angle_matrix_BB00": (0xBB00, 0x12C, "code"),
        "slot165_matrix_product_BC30": (0xBC30, 0x9F, "code"),
        "slot5_angle_lookup_words": (0x37480, 0x2000, "data"),
    },
    "ENG3.DLL": {
        "handler_table": (0x35010, 0x18, "data"),
        "geometry_prepare": (0x24770, 0x61, "code"),
        "handler3": (0x22E40, 0xBE, "code"),
        "shared_object_renderer": (0x8DA0, 0x28D, "code"),
        "FT4_writer_231A0": (0x231A0, 0x293, "code"),
        "alternate_2D_FT4_writer_90E0": (0x90E0, 0x33A, "code"),
        "wrapper_slot101": (0x1FCA0, 0x0F, "code"),
        "wrapper_slot102": (0x1FCB0, 0x19, "code"),
        "wrapper_slot106": (0x1FD00, 0x06, "code"),
        "wrapper_slot107": (0x1FD10, 0x06, "code"),
        "wrapper_slot108": (0x1FD20, 0x0F, "code"),
        "wrapper_slot110": (0x1FD40, 0x0F, "code"),
        "wrapper_slot111": (0x1FD50, 0x0F, "code"),
        "wrapper_slot114": (0x1FD80, 0x0F, "code"),
        "wrapper_slot117": (0x1FD90, 0x0F, "code"),
        "wrapper_slot121": (0x1FDD0, 0x0F, "code"),
        "wrapper_slot174": (0x1FFF0, 0x14, "code"),
        "wrapper_slot175": (0x20010, 0x0F, "code"),
    },
    "TITLE.DLL": {
        "mode_table": (0x220D0, 0x20, "data"),
        "mode5_FT4_writer_181E0": (0x181E0, 0x2DC, "code"),
        "mode6_dispatch_03D10": (0x03D10, 0x90, "code"),
        "mode6_alternate_2D_writer_03420": (0x03420, 0x320, "code"),
        "projection_visibility_helper_19470": (0x19470, 0x61, "code"),
        "clip_matrix_helper_196F0": (0x196F0, 0xF1, "code"),
        "projection_probe_helper_196C0": (0x196C0, 0x27, "code"),
        "TITLE_static_matrix_words": (0x260E0, 18, "data"),
        "wrapper_slot101": (0x0C760, 0x0F, "code"),
        "wrapper_slot102": (0x0C770, 0x19, "code"),
        "wrapper_slot103": (0x0C790, 0x06, "code"),
        "wrapper_slot106": (0x0C7B0, 0x06, "code"),
        "wrapper_slot107": (0x0C7C0, 0x06, "code"),
        "wrapper_slot108": (0x0C7D0, 0x0F, "code"),
        "wrapper_slot110": (0x0C7E0, 0x0F, "code"),
        "wrapper_slot111": (0x0C7F0, 0x0F, "code"),
        "wrapper_slot114": (0x0C810, 0x0F, "code"),
        "wrapper_slot117": (0x0C820, 0x0F, "code"),
        "wrapper_slot121": (0x0C850, 0x0F, "code"),
        "wrapper_slot164": (0x0CA40, 0x14, "code"),
        "wrapper_slot165": (0x0CA60, 0x14, "code"),
        "wrapper_slot174": (0x0CB10, 0x14, "code"),
        "wrapper_slot175": (0x0CB30, 0x0F, "code"),
    },
}

# Copied-table wrappers whose transfer operand is checked against base + slot*4.
WRAPPERS = {
    "ENG3.DLL": {
        101: (0x1FCA0, 0x0F), 102: (0x1FCB0, 0x19),
        106: (0x1FD00, 0x06), 107: (0x1FD10, 0x06),
        108: (0x1FD20, 0x0F), 110: (0x1FD40, 0x0F), 111: (0x1FD50, 0x0F),
        114: (0x1FD80, 0x0F), 117: (0x1FD90, 0x0F), 121: (0x1FDD0, 0x0F),
        174: (0x1FFF0, 0x14), 175: (0x20010, 0x0F),
    },
    "TITLE.DLL": {
        101: (0x0C760, 0x0F), 102: (0x0C770, 0x19),
        103: (0x0C790, 0x06), 106: (0x0C7B0, 0x06), 107: (0x0C7C0, 0x06),
        108: (0x0C7D0, 0x0F), 110: (0x0C7E0, 0x0F), 111: (0x0C7F0, 0x0F),
        114: (0x0C810, 0x0F), 117: (0x0C820, 0x0F), 121: (0x0C850, 0x0F),
        164: (0x0CA40, 0x14), 165: (0x0CA60, 0x14),
        174: (0x0CB10, 0x14), 175: (0x0CB30, 0x0F),
    },
}

# Stack-passing pins from the selected polygon producers. Cleanup is explicit
# for pointer-bearing callbacks; B320/B920 are zero-argument calls.
CALLSITE_ABI = {
    "ENG3.DLL": {0x231A0: {
        102: (0x23380, 0x1FCB0, 0x23385, "add esp, 0xc"),
        107: (0x23388, 0x1FD10, None, None),
        117: (0x233AA, 0x1FD90, 0x233B4, "add esp, 4"),
        101: (0x233C1, 0x1FCA0, 0x233C6, "add esp, 4"),
        106: (0x233C9, 0x1FD00, None, None),
        114: (0x2340A, 0x1FD80, 0x23413, "add esp, 4"),
    }, 0x24770: {
        121: (0x247BB, 0x1FDD0, 0x247C0, "add esp, 4"),
    }},
    "TITLE.DLL": {0x181E0: {
        102: (0x183EC, 0x0C770, 0x183F1, "add esp, 0xc"),
        107: (0x183F4, 0x0C7C0, None, None),
        117: (0x1841C, 0x0C820, 0x18426, "add esp, 4"),
        101: (0x18433, 0x0C760, 0x18438, "add esp, 4"),
        106: (0x1843B, 0x0C7B0, None, None),
        114: (0x18479, 0x0C810, 0x18482, "add esp, 4"),
    }, 0x19470: {
        121: (0x194BB, 0x0C850, 0x194C0, "add esp, 4"),
    }},
}

# Expected host targets for the selected slots; read from the checked source table.
SELECTED_SLOTS = [5, 101, 102, 103, 104, 106, 107, 108, 109, 110, 111, 114, 117, 121, 164, 165, 174, 175]


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def bootstrap(repo_arg: Path) -> Path:
    global ROOT, PE, capstone, unicorn, Uc, verify_runtime
    global UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_MODE_32
    global UC_PROT_EXEC, UC_PROT_READ, UC_PROT_WRITE, UC_X86_REG_EIP, UC_X86_REG_ESP
    lexical = Path(os.path.abspath(os.path.expanduser(str(repo_arg))))
    resolved = lexical.resolve(strict=False)
    if not _same_path(lexical, resolved):
        raise SystemExit("--repo-root must not resolve through a symlink or reparse point")
    if not (resolved / "scripts" / "pe.py").is_file():
        raise SystemExit("--repo-root must contain scripts/pe.py")
    ROOT = resolved
    os.environ["HERCULES_PYTHON_DEPS"] = str(ANALYSIS_DEPS)
    for entry in (ORACLE_DEPS, ROOT / "renderer_contract" / "tests", ROOT / "scripts"):
        sys.path.insert(0, str(entry))
    if not ANALYSIS_DEPS.is_dir() or not ORACLE_DEPS.is_dir():
        raise SystemExit("the existing pinned analysis/oracle dependency directories are unavailable")
    importlib.import_module("env")
    PE = importlib.import_module("pe").PE
    capstone = importlib.import_module("capstone")
    unicorn = importlib.import_module("unicorn")
    u = importlib.import_module("unicorn")
    Uc = u.Uc
    for name in ("UC_ARCH_X86", "UC_HOOK_CODE", "UC_HOOK_MEM_WRITE", "UC_MODE_32",
                 "UC_PROT_EXEC", "UC_PROT_READ", "UC_PROT_WRITE"):
        globals()[name] = getattr(u, name)
    x86_const = importlib.import_module("unicorn.x86_const")
    UC_X86_REG_EIP = x86_const.UC_X86_REG_EIP
    UC_X86_REG_ESP = x86_const.UC_X86_REG_ESP
    verify_runtime = importlib.import_module("oracle_common").verify_runtime
    return resolved


def _same_path(left: Path, right: Path) -> bool:
    return os.path.normcase(os.path.abspath(str(left))) == os.path.normcase(os.path.abspath(str(right)))


def _is_redirect(path: Path) -> bool:
    if path.is_symlink():
        return True
    try:
        attributes = path.lstat().st_file_attributes
    except (AttributeError, FileNotFoundError):
        return False
    return bool(attributes & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400))


def fail(message: str) -> None:
    raise AssertionError(message)


def guarded_output(value: Path, input_paths: dict[str, Path]) -> Path:
    if ROOT is None:
        raise RuntimeError("runner has not been bootstrapped")
    raw_parts = Path(str(value)).parts
    if ".." in raw_parts:
        raise SystemExit("refusing --output containing a parent-directory component")
    lexical_root = ROOT
    lexical_work = lexical_root / "work"
    if not lexical_work.is_dir() or _is_redirect(lexical_work):
        raise SystemExit("refusing output because work/ is missing or redirected")
    resolved_work = lexical_work.resolve(strict=False)
    if not _same_path(lexical_work, resolved_work):
        raise SystemExit("refusing output because work/ resolves outside its lexical path")
    output = value if value.is_absolute() else lexical_root / value
    output = Path(os.path.abspath(str(output)))
    try:
        relative = output.relative_to(lexical_work)
    except ValueError as exc:
        raise SystemExit(f"refusing --output outside lexical work/: {output}") from exc
    for name, source in input_paths.items():
        if _same_path(output, source):
            raise SystemExit(f"refusing --output equal to original input {name}")
    if not relative.parts or output.suffix.lower() != ".json":
        raise SystemExit("--output must name a .json file below work/")
    if any(part.lower() == "assets" for part in relative.parts):
        raise SystemExit("refusing --output under assets/")
    forbidden_tree = lexical_work / "discs" / "pc_install"
    try:
        output.relative_to(forbidden_tree)
    except ValueError:
        pass
    else:
        raise SystemExit("refusing --output under the original pc_install tree")

    # Check every existing path component before creating anything. This rejects
    # symlinks/junctions that could redirect an otherwise lexical work/ path.
    current = lexical_work
    for part in relative.parts[:-1]:
        current = current / part
        if _is_redirect(current):
            raise SystemExit(f"refusing redirected output parent: {current}")
    if _is_redirect(output):
        raise SystemExit(f"refusing redirected output file: {output}")
    output.parent.mkdir(parents=True, exist_ok=True)
    current = lexical_work
    for part in relative.parts[:-1]:
        current = current / part
        if _is_redirect(current):
            raise SystemExit(f"refusing redirected output parent: {current}")
    resolved_output = output.resolve(strict=False)
    try:
        resolved_output.relative_to(resolved_work)
    except ValueError as exc:
        raise SystemExit(f"refusing resolved output outside work/: {resolved_output}") from exc
    if not _same_path(output, resolved_output):
        raise SystemExit("refusing output whose resolved and lexical paths differ")
    if _is_redirect(output):
        raise SystemExit(f"refusing redirected output file: {output}")
    resolved_assets = (ROOT / "assets").resolve(strict=False)
    try:
        resolved_output.relative_to(resolved_assets)
    except ValueError:
        pass
    else:
        raise SystemExit("refusing resolved output under assets/")
    for name, source in input_paths.items():
        if _same_path(resolved_output, source):
            raise SystemExit(f"refusing resolved output equal to original input {name}")
    return output


def complete_decode(md, pe: PE, rva: int, size: int):
    raw = pe.read_rva(rva, size)
    insns = list(md.disasm(raw, pe.image_base + rva))
    cursor = pe.image_base + rva
    for ins in insns:
        if ins.address != cursor:
            fail(f"decode gap at {rva:#x}+{cursor - (pe.image_base + rva):#x}")
        cursor += ins.size
    if cursor != pe.image_base + rva + size:
        fail(f"incomplete decode for {rva:#x}+{size:#x}")
    return raw, insns


def calls(insns, image_base: int):
    rows = []
    for ins in insns:
        if ins.mnemonic != "call":
            continue
        target = None
        if len(ins.operands) == 1 and ins.operands[0].type == capstone.x86.X86_OP_IMM:
            target = ins.operands[0].imm - image_base
        rows.append({"rva": hex(ins.address - image_base), "bytes": bytes(ins.bytes).hex(),
                     "target_rva": None if target is None else hex(target),
                     "indirect": target is None, "operand": ins.op_str})
    return rows


def callback_callsite_evidence(decoded, images):
    evidence = {}
    for module, functions in CALLSITE_ABI.items():
        module_rows = {}
        for producer_rva, slots in functions.items():
            insns = decoded[(module, producer_rva)]
            by_rva = {ins.address - images[module].image_base: (index, ins)
                      for index, ins in enumerate(insns)}
            for slot, (call_rva, wrapper_rva, cleanup_rva, cleanup_text) in slots.items():
                if call_rva not in by_rva:
                    fail(f"missing producer callsite {module}+{call_rva:#x}")
                index, call_ins = by_rva[call_rva]
                if (call_ins.mnemonic != "call" or not call_ins.operands
                        or call_ins.operands[0].type != capstone.x86.X86_OP_IMM
                        or call_ins.operands[0].imm - images[module].image_base != wrapper_rva):
                    fail(f"producer callsite does not target slot {slot}: {module}+{call_rva:#x}")
                row = {"slot": slot, "call_rva": hex(call_rva),
                       "call_bytes": bytes(call_ins.bytes).hex(),
                       "wrapper_rva": hex(wrapper_rva), "stack_arguments": 0 if cleanup_rva is None else None}
                if cleanup_rva is not None:
                    cleanup = next((ins for ins in insns[index + 1:]
                                    if ins.address - images[module].image_base == cleanup_rva), None)
                    if cleanup is None or f"{cleanup.mnemonic} {cleanup.op_str}" != cleanup_text:
                        fail(f"caller cleanup changed for {module} slot {slot}")
                    row["caller_cleanup"] = {"rva": hex(cleanup_rva), "bytes": bytes(cleanup.bytes).hex(),
                                              "instruction": cleanup_text}
                    row["stack_arguments"] = 3 if slot == 102 else 1
                else:
                    row["stack_arguments"] = 0
                module_rows[str(slot)] = row
        evidence[module] = module_rows
    return evidence


def pe_permissions(chars: int) -> int:
    perms = 0
    if chars & 0x40000000:
        perms |= UC_PROT_READ
    if chars & 0x80000000:
        perms |= UC_PROT_WRITE
    if chars & 0x20000000:
        perms |= UC_PROT_EXEC
    return perms


def align_up(value: int, alignment: int = 0x1000) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def map_original_exe(uc: Uc, pe: PE):
    size_headers = pe.pe.OPTIONAL_HEADER.SizeOfHeaders
    uc.mem_map(pe.image_base, align_up(size_headers), UC_PROT_READ)
    uc.mem_write(pe.image_base, pe.data[:size_headers])
    mapped = [{"name": "headers", "va": hex(pe.image_base), "size": align_up(size_headers)}]
    for section in pe.sections:
        virtual_size = max(section["virtual_size"], section["raw_size"])
        mapped_size = align_up(virtual_size)
        va = pe.image_base + section["virtual_address"]
        uc.mem_map(va, mapped_size, pe_permissions(section["characteristics"]))
        if section["raw_size"]:
            uc.mem_write(va, pe.read_rva(section["virtual_address"], section["raw_size"]))
        if mapped_size > section["raw_size"]:
            uc.mem_write(va + section["raw_size"], b"\0" * (mapped_size - section["raw_size"]))
        mapped.append({"name": section["name"], "va": hex(va), "size": mapped_size})
    return mapped


def fixture(exe_path: Path, exe: PE):
    runtime = verify_runtime(unicorn, ORACLE_DEPS)
    if not ORACLE_DEPS.is_dir() or not ANALYSIS_DEPS.is_dir():
        fail("pinned existing Unicorn/analysis dependencies are unavailable")
    if unicorn.__version__ != "2.1.4":
        fail(f"expected Unicorn 2.1.4, got {unicorn.__version__}")

    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    mapped = map_original_exe(uc, exe)
    INPUT, PACKET, STACK, SENTINEL = 0x10000000, 0x11000000, 0x12000000, 0x13000000
    for address in (INPUT, PACKET, STACK):
        uc.mem_map(address, 0x1000, UC_PROT_READ | UC_PROT_WRITE)
    allowed = [
        (0xB2F0, 0xB30F), (0xB320, 0xB42F), (0xB430, 0xB49A),
        (0xB5C0, 0xB607), (0xB6F0, 0xB714), (0xB720, 0xB73A),
        (0xB740, 0xB80A), (0xB870, 0xB880), (0xB920, 0xB96E),
    ]
    trace = []
    scratch_writes = []

    def on_code(machine, address, size, user):
        rva = address - 0x400000
        if not any(start <= rva < end for start, end in allowed):
            fail(f"fixture escaped reviewed host callback closure at VA {address:#x}")
        trace.append(rva)
        if len(trace) > 10000:
            fail("fixture instruction cap exceeded")

    uc.hook_add(UC_HOOK_CODE, on_code)

    def on_mem_write(machine, access, address, size, value, user):
        if 0xA8E740 <= address < 0xA8E750:
            scratch_writes.append({"address": hex(address), "size": size, "value": hex(value)})

    uc.hook_add(UC_HOOK_MEM_WRITE, on_mem_write)

    def call(rva: int, args: list[int] | None = None):
        args = args or []
        sp = STACK + 0x800 - 4 * (len(args) + 1)
        uc.mem_write(sp, struct.pack("<" + "I" * (len(args) + 1), SENTINEL, *args))
        uc.reg_write(UC_X86_REG_ESP, sp)
        uc.reg_write(UC_X86_REG_EIP, 0x400000 + rva)
        uc.emu_start(0x400000 + rva, SENTINEL, count=10000)
        if uc.reg_read(UC_X86_REG_EIP) != SENTINEL:
            fail(f"host callback {rva:#x} did not return to fixture sentinel")

    # Identity 4.12-like matrix, zero X/Y translation, positive camera Z,
    # and small focal/center values make signed projection observable.
    matrix = [4096, 0, 0, 0, 4096, 0, 0, 0, 4096]
    uc.mem_write(0xA8E75C, struct.pack("<9h", *matrix))
    uc.mem_write(0xA8E770, struct.pack("<4i", 0, 0, 1000, 0))
    uc.mem_write(0xA8E74C, struct.pack("<2i", 320, 240))
    uc.mem_write(0xA8E754, struct.pack("<i", 256))

    points = [(-20, 10, 0), (10, 20, 0), (30, -15, 0)]
    point_ptrs = [INPUT + 0x20, INPUT + 0x28, INPUT + 0x30]
    for ptr, point in zip(point_ptrs, points):
        uc.mem_write(ptr, struct.pack("<3h", *point))
    uc.mem_write(0xA8E740, struct.pack("<6H", *([0xDEAD] * 6)))
    uc.mem_write(PACKET, b"\xCC" * 0x40)
    call(0xB430, point_ptrs)
    call(0xB320)
    camera_bulk = [
        list(struct.unpack("<3i", bytes(uc.mem_read(0xA8E71C + i * 4, 4)) +
                           bytes(uc.mem_read(0xA8E728 + i * 4, 4)) +
                           bytes(uc.mem_read(0xA8E734 + i * 4, 4))))
        for i in range(3)
    ]
    expected_camera_bulk = [[x, y, z + 1000] for x, y, z in points]
    if camera_bulk != expected_camera_bulk:
        fail(f"B430/B320 did not transform each signed-WORD XYZ triplet in order: {camera_bulk!r}")
    scratch = list(struct.unpack("<6H", bytes(uc.mem_read(0xA8E740, 12))))
    call(0xB5C0, [PACKET])
    packet_xy = list(struct.unpack("<6H", b"".join(
        bytes(uc.mem_read(PACKET + off, 2)) for off in (8, 10, 16, 18, 24, 26)
    )))

    def s16(v):
        return v if v < 0x8000 else v - 0x10000

    # B320 stores X at .740/.742/.744 and Y at .746/.748/.74a. B5C0 pairs
    # output lane i from those ordered arrays into FT4 vertex slot i.
    expected_bulk_reads = [scratch[0], scratch[3], scratch[1], scratch[4], scratch[2], scratch[5]]
    expected_bulk_screen = [314, 242, 322, 245, 327, 236]
    if packet_xy != expected_bulk_screen:
        fail(f"identity matrix fixture produced unexpected ordered screen XY: {packet_xy!r}")
    if packet_xy != expected_bulk_reads:
        fail(f"B5C0 did not copy its statically pinned scratch offsets: {packet_xy!r}; scratch={scratch!r}; writes={scratch_writes!r}")

    # Verify the one-point path independently through the same native host code.
    one_point = (-20, 10, 0)
    uc.mem_write(INPUT + 0x100, struct.pack("<3h", *one_point))
    uc.mem_write(INPUT + 0x110, b"\x00" * 8)
    call(0xB6F0, [INPUT + 0x100])
    call(0xB920)
    call(0xB720, [INPUT + 0x110])
    point_xy = list(struct.unpack("<2h", bytes(uc.mem_read(INPUT + 0x110, 4))))
    single_camera = [struct.unpack("<i", bytes(uc.mem_read(address, 4)))[0]
                     for address in (0xA8E71C, 0xA8E728, 0xA8E734)]
    call(0xB870, [INPUT + 0x114])
    depth_word = struct.unpack("<H", bytes(uc.mem_read(INPUT + 0x114, 2)))[0]
    expected_one = [314, 242]
    if point_xy != expected_one:
        fail(f"single-point projection mismatch: {point_xy!r} != {expected_one!r}; camera={single_camera!r}; config={[struct.unpack('<i', bytes(uc.mem_read(a, 4)))[0] for a in (0xA8E74C, 0xA8E750, 0xA8E754)]!r}")
    if depth_word != 250:
        fail(f"slot121 depth-word output changed: {depth_word}")

    return {
        "proof_level": "BOUNDED_EQUIVALENT",
        "status": "BOUNDED_ORIGINAL_CALLBACK_FIXTURE_PASS",
        "scope": "identity-matrix fixture against unchanged HERCULES.EXE callbacks only; no DLL execution, renderer, GPU, SDL, assets, or gameplay. It checks this sample and lane/packet association, not general B320/B740 arithmetic equivalence",
        "unicorn": runtime,
        "mapped_original_sections": mapped,
        "execution": {"instruction_count": len(trace), "unique_rvas": len(set(trace)),
                      "executed_rva_ranges": [[hex(a), hex(b)] for a, b in allowed
                                               if any(a <= pc < b for pc in trace)]},
        "inputs": {"source_signed_word_xyz_triplets": [list(p) for p in points],
                   "matrix_words_9": matrix, "translation_dwords_4": [0, 0, 1000, 0],
                   "focal_dword": 256, "center_dwords": [320, 240]},
        "bulk_three_record_route": {
            "calls": ["B430(slot 102)", "B320(slot 107)", "B5C0(slot 117)"],
            "camera_xyz_dwords_by_input_triplet": camera_bulk,
            "screen_scratch_words_740_through_74a": [hex(x) for x in scratch],
            "packet_xy_words_at_8_10_16_18_24_26": [hex(x) for x in packet_xy],
            "native_copy_offsets_match": packet_xy == expected_bulk_reads,
            "association": "B430 lays each pointer's signed WORD XYZ triplet at host scratch stride 6; B320 transforms input triplet i into camera/screen lane i; B5C0 pairs that lane into FT4 packet vertex slot i. The fixture confirms this with three distinct vectors under identity matrix plus Z translation. Mesh-object identity and coordinate units remain caller-specific.",
        },
        "single_point_route": {
            "calls": ["B6F0(slot 101)", "B920(slot 106, internally B740)", "B720(slot 114)"],
            "source_signed_word_xyz_triplet": list(one_point), "screen_xy_words": point_xy,
            "camera_xyz_dwords": single_camera,
            "expected_screen_xy_words": expected_one,
            "camera_depth_word_from_B870": depth_word,
        },
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path,
                        help="repository root containing scripts/pe.py and renderer_contract/evidence")
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--eng1", required=True, type=Path)
    parser.add_argument("--eng3", required=True, type=Path)
    parser.add_argument("--title", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path,
                        help="JSON output; must remain lexically and resolved under repo-root/work")
    parser.add_argument("--no-fixture", action="store_true",
                        help="static-only run; default also checks unchanged HERC callbacks in a bounded fixture")
    args = parser.parse_args()
    bootstrap(args.repo_root)
    paths = {"HERCULES.EXE": args.exe.resolve(), "ENG1.DLL": args.eng1.resolve(),
             "ENG3.DLL": args.eng3.resolve(), "TITLE.DLL": args.title.resolve()}
    output_path = guarded_output(args.output, paths)
    images = {name: PE(path) for name, path in paths.items()}
    for name, pe in images.items():
        if not paths[name].is_file() or sha(pe.data) != IMAGE_HASHES[name]:
            fail(f"{name} path/hash does not match the pinned original: {paths[name]}")
        if pe.image_base != IMAGE_BASES[name]:
            fail(f"unexpected {name} image base {pe.image_base:#x}")
    if capstone.__version__ != "5.0.7":
        fail(f"expected Capstone 5.0.7, got {capstone.__version__}")

    evidence_dir = ROOT / "renderer_contract" / "evidence"
    evidence_names = ["original_spans.json", "geometry.json", "compatibility.json",
                      "module_packets.json", "eng3_handlers.json", "title_handlers.json",
                      "producer_reachability.json"]
    evidence_hashes = {name: sha((evidence_dir / name).read_bytes()) for name in evidence_names}
    geometry = json.loads((evidence_dir / "geometry.json").read_text(encoding="utf-8-sig"))
    eng3_receipt = json.loads((evidence_dir / "eng3_handlers.json").read_text(encoding="utf-8-sig"))
    title_receipt = json.loads((evidence_dir / "title_handlers.json").read_text(encoding="utf-8-sig"))
    if geometry["oracles"]["ENG1.DLL"]["sha256"] != IMAGE_HASHES["ENG1.DLL"]:
        fail("existing ENG1 geometry baseline hash does not match the explicit ENG1 original")

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    span_records = {}
    decoded = {}
    for module, specs in SPANS.items():
        pe = images[module]
        for name, (rva, size, kind) in specs.items():
            raw = pe.read_rva(rva, size)
            row = {"module": module, "rva": hex(rva), "size": size,
                   "end_rva_exclusive": hex(rva + size), "sha256": sha(raw),
                   "content_kind": kind, "boundary": "exact data interval" if kind == "data" else "complete decode",
                   "byte_coverage": "all bytes in [rva, rva+size)"}
            if kind == "code":
                body, insns = complete_decode(md, pe, rva, size)
                if body != raw:
                    fail(f"internal span mismatch for {module}+{rva:#x}")
                decoded[(module, rva)] = insns
                row["instruction_count"] = len(insns)
                row["final_instruction"] = {"rva": hex(insns[-1].address - pe.image_base),
                                             "mnemonic": insns[-1].mnemonic,
                                             "operands": insns[-1].op_str}
                if name.startswith("wrapper_"):
                    if insns[-1].mnemonic not in ("ret", "retn", "jmp"):
                        fail(f"wrapper {module}+{rva:#x} has no transfer boundary")
            span_records[f"{module}+{rva:#x}"] = row

    # Bind slot targets to the pinned static HERC interface table.
    host = images["HERCULES.EXE"]
    table_raw = host.read_rva(HOST_TABLE_RVA, HOST_TABLE_BYTES)
    if sha(table_raw) != HOST_TABLE_SHA:
        fail("HERC host interface table hash mismatch")
    table = struct.unpack("<250I", table_raw)
    targets = {slot: table[slot] - host.image_base for slot in SELECTED_SLOTS}
    expected = {5: 0x37480, 101: 0xB6F0, 102: 0xB430, 103: 0xB840, 104: 0xB4B0, 106: 0xB920,
                107: 0xB320, 108: 0xB880, 109: 0xB8F0, 110: 0xB8C0,
                111: 0xB2F0, 114: 0xB720, 117: 0xB5C0, 121: 0xB870,
                164: 0xBB00, 165: 0xBC30, 174: 0xB970, 175: 0xB990}
    if targets != expected:
        fail(f"host callback targets differ: {targets!r}")

    wrapper_records = {}
    for module, slots in WRAPPERS.items():
        pe = images[module]
        base_rva = int(geometry["copied_wrapper_evidence"][module]["inferred_copied_table_base_rva"], 16)
        for slot, (rva, size) in slots.items():
            raw, insns = complete_decode(md, pe, rva, size)
            cell_va = pe.image_base + base_rva + slot * 4
            refs = []
            for ins in insns:
                if not ins.operands:
                    continue
                for op in ins.operands:
                    if op.type == capstone.x86.X86_OP_MEM and op.mem.base == 0 and op.mem.index == 0:
                        if op.mem.disp == cell_va:
                            refs.append(ins)
            if not refs:
                fail(f"{module} wrapper at {rva:#x} does not reference slot {slot} cell {cell_va:#x}")
            wrapper_records[f"{module}:slot{slot}"] = {
                "wrapper_rva": hex(rva), "size": size, "sha256": sha(raw),
                "copied_table_base_rva": hex(base_rva), "cell_va": hex(cell_va),
                "host_target_rva": hex(targets[slot]),
                "transfer_sites": [{"rva": hex(i.address - pe.image_base),
                                    "mnemonic": i.mnemonic, "operand": i.op_str,
                                    "instruction_sha256": sha(bytes(i.bytes))} for i in refs],
            }

    # Exact call closures for selected producer and projection spans.
    # Keep the bounded direct-call checks local and explicit. 22E40's call list
    # is pinned separately because its complete handler span contains a longer path.
    for (module, rva), expected_targets in {
        ("ENG3.DLL", 0x24770): [0x1FCA0, 0x1FD00, 0x1FD80, 0x1FDD0],
        ("ENG3.DLL", 0x8DA0): [0x23720, 0x23720, 0x9DA0, 0xA9A0,
                                 0xA8B0, 0xA9D0, 0x9030, 0x231A0],
        ("ENG3.DLL", 0x231A0): [0x1FD20, 0x1FD50, 0x1FD40, 0x1FCB0,
                                  0x1FD10, 0x1FD90, 0x1FCA0, 0x1FD00, 0x1FD80],
        ("ENG3.DLL", 0x90E0): [0x23B00],
        ("TITLE.DLL", 0x181E0): [0x1DD0, 0x196F0, 0x0C790, 0x189A0,
                                   0x0C770, 0x0C7C0, 0x0C820, 0x0C760,
                                   0x0C7B0, 0x0C810, 0x1DD0],
        ("TITLE.DLL", 0x196F0): [0x0CA40, 0x0CA60, 0x0C7D0, 0x0CA60, 0x0C7D0],
        ("TITLE.DLL", 0x19470): [0x0C760, 0x0C7B0, 0x0C810, 0x0C850],
        ("TITLE.DLL", 0x196C0): [0x0C760, 0x0C7B0],
        ("TITLE.DLL", 0x03D10): [0x03420, 0x03100, 0x039B0, 0x03740],
        ("TITLE.DLL", 0x03420): [0x189A0],
    }.items():
        insns = decoded[(module, rva)]
        actual = [ins.operands[0].imm - images[module].image_base for ins in insns
                  if ins.mnemonic == "call" and ins.operands
                  and ins.operands[0].type == capstone.x86.X86_OP_IMM]
        if actual != expected_targets:
            fail(f"direct-call sequence mismatch at {module}+{rva:#x}: {actual!r}")

    mode_table = images["TITLE.DLL"].read_rva(0x220D0, 0x20)
    mode_targets = [v - images["TITLE.DLL"].image_base for v in struct.unpack("<8I", mode_table)]
    if mode_targets[5] != 0x181E0 or mode_targets[6] != 0x03D10:
        fail(f"TITLE mode indices 5/6 changed: {mode_targets!r}")
    if sha(mode_table) != title_receipt["mode_table"]["span"]["sha256"]:
        fail("TITLE mode table hash differs from existing pinned handler receipt")

    eng3_table = images["ENG3.DLL"].read_rva(0x35010, 0x18)
    eng3_targets = [hex(v - images["ENG3.DLL"].image_base) for v in struct.unpack("<6I", eng3_table)]
    if eng3_targets != eng3_receipt["handler_table"]["target_rvas_by_index"]:
        fail(f"ENG3 handler-table targets differ from existing pinned receipt: {eng3_targets!r}")
    if sha(eng3_table) != eng3_receipt["handler_table"]["sha256"]:
        fail("ENG3 handler-table hash differs from existing pinned receipt")

    # No direct rel32 calls from these selected DLL producer bodies resolve into HERC.
    direct_host_calls = []
    for (module, rva) in [("ENG3.DLL", 0x22E40), ("ENG3.DLL", 0x24770), ("ENG3.DLL", 0x8DA0),
                          ("ENG3.DLL", 0x231A0), ("ENG3.DLL", 0x90E0),
                          ("TITLE.DLL", 0x181E0), ("TITLE.DLL", 0x19470),
                          ("TITLE.DLL", 0x196C0), ("TITLE.DLL", 0x196F0),
                          ("TITLE.DLL", 0x03D10),
                          ("TITLE.DLL", 0x03420)]:
        for row in calls(decoded[(module, rva)], images[module].image_base):
            if row["target_rva"] is not None:
                target_va = images[module].image_base + int(row["target_rva"], 16)
                if host.image_base <= target_va < host.image_base + host.pe.OPTIONAL_HEADER.SizeOfImage:
                    direct_host_calls.append({"module": module, **row})
    if direct_host_calls:
        fail(f"unexpected direct rel32 HERC calls in bounded DLL spans: {direct_host_calls!r}")
    callsite_evidence = callback_callsite_evidence(decoded, images)

    receipt = {
        "schema_version": 1,
        "status": "PASS",
        "scope": "One selected ENG3 polygon route and one selected TITLE polygon route, module-specific. No source identity, whole-module, rendered-pixel, or gameplay claim.",
        "proofs": {
            "static_observed": {
                "proof_level": "STATIC_OBSERVED",
                "scope": "Hash-pinned original spans, complete code-span decode, copied-table callback binding, and selected producer dataflow.",
            },
            "bounded_equivalent": None,
        },
        "prediction_and_falsifier": {
            "prediction": "ENG3's selected FT4 producer will use copied-table 3D host services for a three-point batch and a one-point fourth vertex; TITLE's selected FT4 producer will have an independent 3D callback path plus a separate direct 2D producer.",
            "falsifiers": ["Selected producer lacks the expected 101/102/106/107/114/117 route.",
                           "The packet destination cannot be tied to a cursor and packet writer.",
                           "TITLE's mode-5 producer table route does not resolve to 0x181e0."],
            "prediction_record": "work/renderer_phase14/cross_geometry/prediction.txt",
        },
        "tooling": {"python": sys.version.split()[0], "capstone": capstone.__version__,
                    "analysis_dependency_root": str(ANALYSIS_DEPS),
                    "oracle_dependency_root": str(ORACLE_DEPS),
                    "external_tool_origin": "Existing local C:\\tools\\hercules\\python and C:\\tools\\hercules\\renderer-oracle-python installs; no dependency added or modified."},
        "oracles": {name: {"path": str(paths[name]), "sha256": sha(images[name].data),
                            "image_base": hex(images[name].image_base)} for name in paths},
        "evidence_input_sha256": evidence_hashes,
        "host_interface": {"rva": hex(HOST_TABLE_RVA), "bytes": HOST_TABLE_BYTES,
                            "sha256": HOST_TABLE_SHA,
                            "selected_slot_targets": {str(k): hex(v) for k, v in targets.items()}},
        "spans": span_records,
        "copied_table_wrappers": wrapper_records,
        "producer_callback_callsites": callsite_evidence,
        "eng3": {
            "selected_route": "Pinned ENG3 handler-table index 3 (0x22e40) -> 0x24770 projection/depth gate -> 0x8da0 shared renderer -> conditional 0x231a0 FT4 producer at 0x901d; handler index 1 reaches alternate 2D writer 0x90e0.",
            "source_records": "0x231a0 prepares three local signed-WORD XYZ triplets, each passed by pointer through slot 102/B430; it prepares a fourth three-WORD triplet for slot 101/B6F0. The callback boundary proves the 3x signed-WORD layout; object/resource units and object identity remain unresolved. The separate 0x24770 helper reads three object DWORD fields, arithmetic-shifts each right 16, and narrows the result to WORDs before slot 101.",
            "setup": "0x231a0 passes global pointer 0x100394b8 through slot 108/B880, which loads nine WORD coefficients into A8E75c..A8E76c. It then sends A8E71c/28/34 through slot 111/B2F0 to current context+0x78 and passes context+0x78 through slot 110/B8C0, which loads four translation DWORDs into A8E770..A8E77c; the fourth source DWORD is not written by B2F0 in this sequence. Center/focal writes via 174/175 are present elsewhere, not in 0x231a0.",
            "projection_and_packet": "slot 102/B430 loads three signed-WORD XYZ triplets into host input scratch; slot 107/B320 transforms input triplet i into camera/screen lane i; slot 117/B5C0 pairs lane i to FT4 packet vertex i at offsets +08/+0a, +10/+12, +18/+1a. The fourth triplet uses slots 101/B6F0 -> 106/B920 -> 114/B720, with B720 writing to packet+0x20. The cursor is 0x10043164; producer stride is 0x28; it links through an OT-head pointer derived from current view 0x10041448.",
            "visibility": "Handler 0x22e40 has a parent 0x24a80 reject path that sets object byte +0x1f bit 0x08. Per child, 0x24770 returns true only when the slot-121/B870 output word (camera Z arithmetic-shifted right 2) is greater than 0x40; only then does the path call 0xa410 and 0x8da0. 0x231a0 itself has additional flag/state gates. No claim that this is the only ENG3 cull/clip path.",
            "alternate_2d": "Pinned ENG3 handler-table index 1 reaches 0x90e0, whose only direct call is local command/color helper 0x23b00; it writes FT4 coordinates from existing WORD fields/resource deltas without slots 101/106/114.",
            "direct_host_calls_bypassing_table_in_checked_spans": [],
        },
        "title": {
            "selected_route": "Pinned TITLE mode table index 5 -> 0x181e0 FT4 producer; index 6 -> 0x03d10 direct-writer dispatcher, including the bounded 0x03420 2D route.",
            "source_records": "0x181e0 assembles three local signed-WORD XYZ triplets from object/resource fields and passes their pointers through slot 102/B430; its fourth triplet goes through slot 101/B6F0. The callback boundary proves the 3x signed-WORD layout; object identity and coordinate unit are not inferred.",
            "setup": "The selected writer conditionally calls 0x196f0 when 0x1002d328 is nonzero. Its 0xCA40 and 0xCA60 helpers are copied-table wrappers to slot 164 at HERC RVA 0xBB00 and slot 165 at 0xBC30; the former reads the 4096-WORD lookup table reached through HERC table cell 5 (RVA 0x37480), and the latter combines the local values with the pinned TITLE matrix at RVA 0x260E0. The result is passed through slot 108/B880 at 0xC7D0. The writer then tail-calls 0xC790, which is slot 103/B840: B840 copies current camera XYZ dwords into host translation cells. The selected callback and data dependencies are byte-pinned, but the source/meaning of the computed matrix and coordinate units remain unresolved. Slots 174/175 exist in TITLE wrappers at 0xCB10/0xCB30 but are not called by 0x181e0 itself.",
            "projection_and_packet": "The selected producer uses slots 102/B430 -> 107/B320 -> 117/B5C0 for its first three signed-WORD XYZ triplets, then 101/B6F0 -> 106/B920 -> 114/B720 for its fourth. B5C0 pairs each transformed input lane to the matching FT4 packet vertex slot. Packet cursor is 0x1002df40, stride 0x28; order head derives from the current view at 0x1002cc04. Object identity and coordinate units remain unresolved.",
            "visibility_and_clip": "The selected writer has a conditional matrix setup through 0x196f0, then copies current camera XYZ into translation via slot 103/B840. The bounded mode-5 producer does not call slot 104/B4B0. A separate TITLE mode-3 path calls 0x19470 on a child and accepts it only when camera Z>>2 > 0x40, then passes the same child pointer to dynamic mode dispatch. The 0x19470 output at child+0..+4 is a projection/depth gate; selected mode-5 packet construction consumes separately derived local geometry.",
            "alternate_2d": "Mode 6 dispatch 0x03d10 selects among direct FT4 writers including 0x03420; that writer's only direct call is 0x189a0 and it writes 0x28-stride packets without 101/106/114 geometry calls. Its source positions are signed WORD fields at object+0x0c/+0x0e plus resource byte deltas.",
            "direct_host_calls_bypassing_table_in_checked_spans": [],
        },
        "cross_module_readiness": {
            "ENG1": "Separate prior STATIC_OBSERVED baseline: reviewed point emitters at 0x2e600/0x2e6c0/0x2ed10 call slot 101/B6F0 -> 106/B920 -> 114/B720; this report only hash-checks ENG1 and cites its existing evidence, without transferring ENG1 producer or culling conclusions to ENG3/TITLE.",
            "ENG3": "The bounded 3D FT4 route associates the global nine-WORD matrix, three signed-WORD XYZ triplets, output camera/screen lanes, fourth one-triplet projection, and 0x28-byte packet destination through copied-table callbacks. Each B320 input triplet maps through B5C0 to the corresponding packet vertex. The fourth translation dword remains unresolved.",
            "TITLE": "Mode-5 3D FT4 route and mode-6 direct 2D route are statically established. The mode-5 writer conditionally seeds its matrix through a closed slot164/slot165 callback and lookup-table path, loads it via slot108/B880, then copies camera XYZ through slot103/B840. Its triplet-to-output-lane-to-packet-vertex association matches ENG3, while projection center/focal setup remains external to the selected writer.",
            "common_reusable_facts": "Host callbacks B6F0/B920/B720 implement a signed WORD triplet input, matrix transform, perspective projection, and caller XY output. Camera Z is clamped to at least 1 in B740; B870 exposes Z>>2 as a WORD. This does not establish identical per-module source records or all producer routes.",
        },
        "eng1_baseline": {
            "proof_level": "STATIC_OBSERVED",
            "evidence_path": "renderer_contract/evidence/geometry.json",
            "evidence_sha256": evidence_hashes["geometry.json"],
            "producer_spans": geometry["eng1_original_producer_spans"],
            "comparison_limit": "The pre-existing ENG1 trace is a separate baseline. This audit does not infer ENG3 or TITLE source records, culling, or alternate producers from ENG1.",
        },
        "callback_contracts": {
            "transform_precision": {
                "proof_level": "STATIC_OBSERVED",
                "slot_106_B740_single_point": "The three signed-product terms accumulate with low-32-bit register wrap; the row sum is arithmetic-shifted right 12, translation is added, and the low WORD is sign-extended to a DWORD. The Z result is clamped to at least 1. This describes B740 only.",
                "slot_107_B320_batch": "Separately, each batch row accumulates signed products in 32-bit registers, arithmetic-shifts the row sum right 12, adds its translation DWORD, then sign-extends the low WORD into the camera DWORD lane. No minimum-Z clamp appears in the checked B320 span; a zero Z skips its IDIV. This is a static description, not inferred from B740 or validated generally by the identity fixture.",
                "fixture_limit": "The identity-matrix fixture checks its chosen sample and the vertex-to-lane-to-packet association; it cannot distinguish general arithmetic order, low-WORD narrowing, or clamp behavior.",
                "source_coordinate_units": "unknown",
            },
            "batch_three_vertex": {
                "slot_102_B430": {
                    "signature": "void __cdecl B430(const int16_t *v0_xyz, const int16_t *v1_xyz, const int16_t *v2_xyz)",
                    "input": "three pointer arguments in vertex order; each addresses 3 signed WORDs (X,Y,Z), 6 bytes per vertex",
                },
                "slot_107_B320": {
                    "signature": "void __cdecl B320(void)",
                    "camera_xyz_lanes": "Observed stores: [ESI] at B367, [ESI+0x0C] at B3A3, [ESI+0x18] at B3E1; ESI starts A8E71C and advances 4 at B403. Thus X/Y/Z signed DWORD lanes start at A8E71C/A8E728/A8E734, each with 4-byte lane stride.",
                    "screen_xy_lanes": "Observed stores: [ECX+0x32] at B406 and [ECX+0x36] at B422; ECX starts A8E70E and advances 2 at B414. Thus signed WORD X/Y lanes start at A8E740/A8E746, each with 2-byte lane stride.",
                },
                "slot_117_B5C0": {
                    "signature": "void __cdecl B5C0(void *ft4_packet)",
                    "packet_xy": "copies lane i to FT4 vertex i at offsets +0x08/+0x0A, +0x10/+0x12, +0x18/+0x1A",
                },
            },
            "single_vertex": {
                "slot_101_B6F0": {"signature": "void __cdecl B6F0(const int16_t *xyz)", "input": "one signed WORD XYZ triplet; 6 bytes"},
                "slot_106_B920": {"signature": "void __cdecl B920(void)", "result": "signed DWORD camera XYZ plus signed WORD projected XY host globals"},
                "slot_114_B720": {"signature": "void __cdecl B720(int16_t *screen_xy)", "output": "two signed WORDs (4 bytes) to caller destination"},
                "slot_121_B870": {"signature": "void __cdecl B870(uint16_t *depth)", "output": "one WORD equal to camera Z arithmetic-shifted right two"},
            },
        },
        "bounded_fixture": None if args.no_fixture else fixture(paths["HERCULES.EXE"], images["HERCULES.EXE"]),
        "limits": ["Static PE observations plus a finite unchanged-original HERC callback fixture only.",
                   "The fixture does not execute ENG3.DLL or TITLE.DLL and does not test game/runtime callback publication.",
                   "No rasterizer/GPU/SDL, full-frame, asset parsing, original modification, source identity, or packet consumer behavior is claimed.",
                   "Source values and packet XY offsets are proven at the callback seam; no mesh-object identity, coordinate units, rasterized visibility, or complete-frame behavior is claimed.",
                   "Calls beyond the recorded spans and computed/global module aliases remain outside this audit."],
    }
    if not args.no_fixture:
        receipt["proofs"]["bounded_equivalent"] = {
            "proof_level": "BOUNDED_EQUIVALENT",
            "scope": "Finite identity-matrix sample against unchanged HERCULES.EXE callbacks only; checks sample values and lane/packet association, not general B320/B740 arithmetic equivalence; no ENG1/ENG3/TITLE producer execution.",
            "result": receipt["bounded_fixture"]["status"],
        }
    output_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({"output": str(output_path), "status": receipt["status"],
                      "images": {n: sha(p.data) for n, p in images.items()},
                      "span_count": len(span_records), "wrapper_count": len(wrapper_records),
                      "fixture": None if args.no_fixture else receipt["bounded_fixture"]["status"]}, indent=2))


if __name__ == "__main__":
    main()
