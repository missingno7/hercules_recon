#!/usr/bin/env python3
"""Replay two bounded native ENG3 interval/depth decisions with original x86."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import struct
import sys
from collections import Counter
from pathlib import Path

sys.dont_write_bytecode = True

EXE_SHA = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
ENG3_SHA = "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7"
EXE_BASE, ENG_BASE = 0x00400000, 0x10000000
TABLE_RVA, TABLE_SIZE = 0x361B8, 0x3E8
TABLE_SHA = "52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8"
SINE_RVA, SINE_SIZE = 0x37480, 0x2000
SINE_VA, SINE_FILE_OFFSET = 0x437480, 0x35880
SINE_SHA = "e62146d85a8a12fad1c8982008754fa515be2e521a982f65d991ed19e64a2737"
PREDICTION_SHA = "ed89164219eb5a8732aa9d7c9dc68dd87dd9e536ebc952b6bc95e4b3ebcb91d0"
UNICORN_VERSION = "2.1.4"
UNICORN_DLL_SHA = "010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1"
STATE_BASE, STACK, STACK_SIZE, SENTINEL = 0x22000000, 0x0E000000, 0x10000, 0x0F000000
STACK_TOP = STACK + STACK_SIZE - 0x100
TABLE_BASE = 0x21000000
OBJECT, GEOMETRY = STATE_BASE + 0x2000, STATE_BASE + 0x3000
SLOT4_TARGET = 0x00BA25C0

# Exact function/callback spans already present in the public viewport audit.
ENG3_SPANS = {
    "interval_row_writer_204c0": (0x204C0, 0x22A, "d7fdaeaa92e928fec0bbd482674672c662e03a822fa8e3b9c891320520c5a01f"),
    "indexed_parent_predicate_24a80": (0x24A80, 0x6F, "3a9fc30527d0f722cd6a1e16e8f5c5ba3b7f3e8bafa3fefc6ab2c1a172e89e85"),
    "parent_transform_helper_24bb0": (0x24BB0, 0x39B, "a0ad5e76036c8f94d63730e28691147bf7b8b80d64e2d9dc4a767c96868f026a"),
    "success_epilogue_24aef": (0x24AEF, 5, "cadc250d09720749067905ca17f6f4be7be66a84e4f6bfe9fef3bf140b943d19"),
    "wrapper_slot101_1fca0": (0x1FCA0, 0x0F, "ee6fddcba7224cafa291397c8a3ada0fd1ff8ef20b9cca5e9ea3a6c7ded280fb"),
    "wrapper_slot106_1fd00": (0x1FD00, 6, "11f370d92cc7151d75bd2d3788180bcf296810f1dd23836c264c36db31f1c12d"),
    "wrapper_slot108_1fd20": (0x1FD20, 0x0F, "b559bb2957dc01e24ab7e7a8e58421878facaf08b8fa6a727728dedc4fb52604"),
    "wrapper_slot109_1fd30": (0x1FD30, 0x0F, "ea18065631499c89565a3c1e655e1f604b4ddf406d670713a2ebf36ca92b370a"),
    "wrapper_slot114_1fd80": (0x1FD80, 0x0F, "d8ceea8c3db0d736e2601fb6502f57f9161786534dd7d7457e3a678e360e3b3f"),
    "wrapper_slot121_1fdd0": (0x1FDD0, 0x0F, "2769c9935c5b55efed131e9b1e5c17fbbc53dd0133209baaffdfba50da5b9675"),
    "wrapper_slot164_1ff40": (0x1FF40, 0x14, "6527d48ff16542d8847d5d7ce64f1ff295a7489c83b042f6107c3a2a3c4b19cf"),
    "wrapper_slot165_1ff60": (0x1FF60, 0x14, "b2e7174c3489fd947129a6e0b651c3a07ba8d31f3c945f1f31ffc60c9bdc2f7a"),
    "setter_wrapper_slot174_1fff0": (0x1FFF0, 0x14, "732321acbb3d747239a2956a7ef2c46a12806611a40d8d861c77b97e2e84ec3a"),
    "setter_wrapper_slot175_20010": (0x20010, 0x0F, "67aa4e51772e9ad59532733edbe54a684051ce122418311439f6d9703607fb6b"),
}
HERC_SPANS = {
    "slot101_vertex_input_B6F0": (0xB6F0, 0x24, "6a6848a640a84133abcc29bf3b0c01e2fb1638f36e916ed8e4f68890210aff29"),
    "slot106_batch_projection_B920": (0xB920, 0x4E, "055adff43e62ef00b06e5313ee1701ac404cfe6b3286d75648e5c3265ae7ba86"),
    "slot108_matrix_copy_B880": (0xB880, 0x32, "b890d113710e54184d9b505fe9c2a80214ec58f7980ebc2700e16f86bc7a8063"),
    "slot109_translation_copy_B8F0": (0xB8F0, 0x22, "0c72ecba43935848d1e76f61c5ed99b5e4b74efe1f491237aa4bce108332b6b0"),
    "slot114_screen_xy_B720": (0xB720, 0x1A, "51f3989776f1410d8ffa246269a37fac2b44590d0f6bcf1cdca4937d0b7b0843"),
    "slot121_depth_word_B870": (0xB870, 0x10, "07f74e0d8df01e3a43e4adc89a64b0c1e9c92a33bda390d01bca4accb327fb6c"),
    "slot164_lookup_BB00": (0xBB00, 0x12C, "eb77c95173af56f9ae8bb0646959e66ef24bed75a7122545b18169d30ab79217"),
    "slot165_q12_scale_BC30": (0xBC30, 0x9F, "8cf8663405ada692c0a993543b4dc1af1681774963575c3cd50c0364b161d040"),
    "slot174_projection_center_B970": (0xB970, 0x14, "2526cc71c8d43572f1aabfacc70678e699908b02a13cf0b72f832b1e4062d252"),
    "slot175_projection_focal_B990": (0xB990, 0x0A, "c44e898f7d175fbc52e1761ebbf409c20d9579c27174ca44a631d31909e8c68a"),
    "observed_slot106_service_B740": (0xB740, 0xCA, "f745b08e23d8f718c7e269b2642a32a2ea852a1aa86c88dd774df9f555158f2c"),
}
CALLBACK_TARGETS = {
    5: 0x00437480, 101: 0x0040B6F0, 106: 0x0040B920,
    108: 0x0040B880, 109: 0x0040B8F0, 114: 0x0040B720,
    121: 0x0040B870, 164: 0x0040BB00, 165: 0x0040BC30,
    174: 0x0040B970, 175: 0x0040B990,
}
ENG3_WRAPPERS = {
    rva: size for name, (rva, size, _) in ENG3_SPANS.items()
    if name.startswith("wrapper_") or name.startswith("setter_wrapper_")
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def import_file(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load public utility: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def check_output(raw: str, root: Path, inputs: list[Path]) -> Path:
    given = Path(raw)
    lexical = Path(os.path.abspath(given if given.is_absolute() else root / given))
    work_lexical = Path(os.path.abspath(root / "work"))
    work_resolved = (root / "work").resolve()
    resolved = lexical.resolve(strict=False)
    if not lexical.is_relative_to(work_lexical) or not resolved.is_relative_to(work_resolved):
        raise ValueError("--output must stay lexically and physically below <repo-root>/work")
    if lexical.suffix.lower() != ".json" or lexical.exists() or lexical.is_symlink():
        raise ValueError("--output must be a fresh, nonexistent .json path")
    current = lexical.parent
    while current != root and current != current.parent:
        if current.is_symlink():
            raise ValueError("--output path may not traverse a symbolic link")
        current = current.parent
    assets = (root / "assets").resolve(strict=False)
    if resolved.is_relative_to(assets):
        raise ValueError("--output may not target assets")
    for item in inputs:
        item_resolved = item.resolve()
        if resolved == item_resolved or resolved.is_relative_to(item_resolved.parent):
            raise ValueError("--output may not alias an input or be placed under its directory")
    return lexical


def file_backed(image, base: int, address: int, size: int) -> bool:
    rva = address - base
    if size <= 0 or rva < 0:
        return False
    try:
        first = image.rva_to_offset(rva)
        last = image.rva_to_offset(rva + size - 1)
        return last == first + size - 1
    except Exception:
        return False


def covered(ranges, address: int, size: int) -> bool:
    start, end = address, address + size
    cursor = start
    for low, high in sorted((low, high) for low, high in ranges if low < end and start < high):
        if low > cursor:
            return False
        cursor = max(cursor, high)
        if cursor >= end:
            return True
    return cursor >= end


def pin_spans(exe, eng3) -> dict:
    result = {}
    for name, (rva, length, expected) in ENG3_SPANS.items():
        actual = sha(eng3.read_rva(rva, length))
        if actual != expected:
            raise RuntimeError(f"ENG3 span {name} hash mismatch: {actual}")
        result[f"ENG3+{rva:#x}"] = {"name": name, "length": length, "sha256": actual}
    for name, (rva, length, expected) in HERC_SPANS.items():
        actual = sha(exe.read_rva(rva, length))
        if actual != expected:
            raise RuntimeError(f"HERCULES span {name} hash mismatch: {actual}")
        result[f"HERCULES.EXE+{rva:#x}"] = {"name": name, "length": length, "sha256": actual}
    return result


class RouteTrace:
    """Bounded instruction trace with direct memory reads inside read hooks."""
    def __init__(self, uc, allowed, unicorn, regs, images, initialized, phase,
                 input_ranges=(), slot4_target=SLOT4_TARGET):
        self.uc, self.allowed, self.unicorn, self.regs = uc, allowed, unicorn, regs
        self.images, self.initialized, self.phase = images, list(initialized), phase
        self.input_ranges = list(input_ranges)
        self.slot4_target = slot4_target
        self.pcs, self.unexpected_pc, self.invalid = [], None, None
        self.read_count = 0
        self.trig_reads, self.slot4_reads, self.slot4_writes = [], [], []
        self.unbacked_reads, self.input_write_violations = [], []
        self.write_count = 0
        self.hooks = [
            uc.hook_add(unicorn.UC_HOOK_CODE, self.on_code),
            uc.hook_add(unicorn.UC_HOOK_MEM_READ, self.on_read),
            uc.hook_add(unicorn.UC_HOOK_MEM_WRITE, self.on_write),
            uc.hook_add(unicorn.UC_HOOK_MEM_INVALID, self.on_invalid),
        ]

    def on_code(self, uc, address, size, user):
        if address == SENTINEL:
            uc.emu_stop()
            return
        if not any(low <= address < high for low, high in self.allowed):
            self.unexpected_pc = address
            uc.emu_stop()
            return
        self.pcs.append(address)

    def on_read(self, uc, access, address, size, hook_value, user):
        # Unicorn's READ hook value is not the source operand on this backend.
        # Read the mapped bytes explicitly before the instruction executes.
        observed = int.from_bytes(bytes(uc.mem_read(address, size)), "little")
        pc = uc.reg_read(self.regs["eip"])
        self.read_count += 1
        if (address < self.slot4_target + 6 and
                self.slot4_target < address + size and size <= 4):
            self.slot4_reads.append({"pc": hex(pc), "address": hex(address),
                                     "size": size, "observed_value": hex(observed)})
        sine_start, sine_end = SINE_VA, SINE_VA + SINE_SIZE
        if address < sine_end and sine_start < address + size:
            row = {"pc": hex(pc), "address": hex(address),
                   "offset": address - SINE_VA, "size": size,
                   "observed_value": hex(observed),
                   "unicorn_hook_value_diagnostic_only": hex(hook_value & ((1 << (size * 8)) - 1))}
            if address < sine_start or address + size > sine_end:
                row["fully_inside_pinned_slice"] = False
            else:
                row["fully_inside_pinned_slice"] = True
                expected = int.from_bytes(
                    self.images["exe"].read_rva(SINE_RVA + address - SINE_VA, size), "little")
                if observed != expected:
                    raise RuntimeError("direct memory read disagrees with original file-backed sine bytes")
            self.trig_reads.append(row)
        for label, image, base in (("exe", self.images["exe"], EXE_BASE),
                                   ("eng3", self.images["eng3"], ENG_BASE)):
            image_size = image.pe.OPTIONAL_HEADER.SizeOfImage
            if base <= address and address + size <= base + image_size:
                if not file_backed(image, base, address, size) and not covered(self.initialized, address, size):
                    self.unbacked_reads.append({"module": label, "pc": hex(pc),
                                                "address": hex(address), "size": size,
                                                "observed_value": hex(observed)})
                break

    def on_write(self, uc, access, address, size, value, user):
        pc = uc.reg_read(self.regs["eip"])
        self.write_count += 1
        event = {"phase": self.phase, "pc": hex(pc), "address": hex(address),
                 "size": size,
                 "value": hex(value & ((1 << (size * 8)) - 1)) if size <= 4 else None}
        if address < self.slot4_target + 6 and self.slot4_target < address + size:
            self.slot4_writes.append(event)
        self.initialized.append((address, address + size))
        for low, high, label in self.input_ranges:
            if low < address + size and address < high:
                self.input_write_violations.append({**event, "input": label})

    def on_invalid(self, uc, access, address, size, value, user):
        self.invalid = {"phase": self.phase,
                        "pc": hex(uc.reg_read(self.regs["eip"])),
                        "address": hex(address), "size": size, "access": access}
        return False

    def close(self):
        for hook in self.hooks:
            self.uc.hook_del(hook)


def span_ranges():
    ranges = [(ENG_BASE + rva, ENG_BASE + rva + size)
              for name, (rva, size, _) in ENG3_SPANS.items()
              if name != "interval_row_writer_204c0"]
    ranges += [(EXE_BASE + rva, EXE_BASE + rva + size)
               for rva, size, _ in HERC_SPANS.values()]
    return ranges


def setup_case(base, exe, eng3, unicorn, regs, table, row, coord):
    uc = base.new_machine(exe, eng3, unicorn)
    uc.mem_map(TABLE_BASE, 0x1000, unicorn.UC_PROT_ALL)
    uc.mem_map(STATE_BASE, 0x10000, unicorn.UC_PROT_ALL)

    # Copy the complete original 0x3E8-byte prefix, including pointer slots 4/5.
    uc.mem_write(ENG_BASE + 0x407A0, table)
    if bytes(uc.mem_read(ENG_BASE + 0x407A0, len(table))) != table:
        raise RuntimeError("full HERC table prefix did not round-trip unchanged")
    if int.from_bytes(uc.mem_read(ENG_BASE + 0x407B4, 4), "little") != SINE_VA:
        raise RuntimeError("copied host slot 5 did not set ENG3 global 0x407B4")

    uc.mem_write(TABLE_BASE, row)
    base.put32(uc, ENG_BASE + 0x4078C, TABLE_BASE)
    base.put32(uc, ENG_BASE + 0x40540, GEOMETRY)
    uc.mem_write(GEOMETRY, b"\0" * 0x200)
    # Both cases retain the exact v4 record contents; coordinate 9 rejects
    # before consulting it, while coordinate 10 uses byte +0xF to derive Z.
    uc.mem_write(GEOMETRY + coord * 16 + 0xF, b"\x05")
    for address in (0x405A0, 0x405A4, 0x405A8, 0x405B0):
        base.put32(uc, ENG_BASE + address, 0)
    base.put16(uc, ENG_BASE + 0x405AC, 0)
    base.put16(uc, ENG_BASE + 0x40640, 0)
    # These are three Q12 scale factors consumed by HERC+0xBC30 at +0/+4/+8,
    # not an asserted parent matrix layout.
    q12_factors = (4096, 4096, 4096)
    uc.mem_write(ENG_BASE + 0x39518, struct.pack("<9i", *q12_factors, 0, 0, 0, 0, 0, 0))
    uc.mem_write(OBJECT, b"\0" * 0x200)
    object_x = ((coord * 128) & 0xFFFF) << 16
    base.put32(uc, OBJECT, object_x)
    base.put16(uc, OBJECT + 0x38, 0)

    initialized = [
        (ENG_BASE + 0x407A0, ENG_BASE + 0x407A0 + len(table)),
        (ENG_BASE + 0x4078C, ENG_BASE + 0x40790),
        (ENG_BASE + 0x40540, ENG_BASE + 0x40544),
        (ENG_BASE + 0x405A0, ENG_BASE + 0x405B4),
        (ENG_BASE + 0x40640, ENG_BASE + 0x40642),
        (ENG_BASE + 0x39518, ENG_BASE + 0x3953C),
    ]
    setter_allowed = [(ENG_BASE + 0x1FFF0, ENG_BASE + 0x20004),
                      (ENG_BASE + 0x20010, ENG_BASE + 0x2001F),
                      (EXE_BASE + 0xB970, EXE_BASE + 0xB984),
                      (EXE_BASE + 0xB990, EXE_BASE + 0xB99A)]
    setter_trace = RouteTrace(uc, setter_allowed, unicorn, regs,
                              {"exe": exe, "eng3": eng3}, initialized,
                              "camera_setters")
    center_result = base.invoke(uc, setter_trace, ENG_BASE + 0x1FFF0,
                                (0, 0), regs, unicorn)
    focal_result = base.invoke(uc, setter_trace, ENG_BASE + 0x20010,
                               (256,), regs, unicorn)
    center = [int.from_bytes(uc.mem_read(0x00A8E74C, 4), "little"),
              int.from_bytes(uc.mem_read(0x00A8E750, 4), "little")]
    focal = int.from_bytes(uc.mem_read(0x00A8E754, 4), "little")
    setter_ok = (center_result["returned_to_sentinel"] and
                 focal_result["returned_to_sentinel"] and
                 setter_trace.unexpected_pc is None and setter_trace.invalid is None and
                 not setter_trace.unbacked_reads and center == [0, 0] and focal == 256)
    if not setter_ok:
        setter_trace.close()
        raise RuntimeError("original center/focal wrappers failed their controlled setup")
    setter_receipt = {"center_call": center_result, "focal_call": focal_result,
                      "write_count": setter_trace.write_count,
                      "center": center, "focal": focal,
                      "unexpected_pc": setter_trace.unexpected_pc,
                      "invalid_memory": setter_trace.invalid,
                      "unbacked_reads": setter_trace.unbacked_reads}
    initialized = setter_trace.initialized
    setter_trace.close()

    inputs = [
        (TABLE_BASE, TABLE_BASE + len(row), "controlled_interval_row"),
        (ENG_BASE + 0x407A0, ENG_BASE + 0x407A0 + len(table), "copied_full_host_prefix"),
        (ENG_BASE + 0x4078C, ENG_BASE + 0x40790, "bounds_table_pointer"),
        (ENG_BASE + 0x40540, ENG_BASE + 0x40544, "geometry_pointer_slot0"),
        (ENG_BASE + 0x405A0, ENG_BASE + 0x405B4, "zero_angle_globals"),
        (ENG_BASE + 0x40640, ENG_BASE + 0x40642, "zero_angle_word"),
        (ENG_BASE + 0x39518, ENG_BASE + 0x3953C, "three_q12_factors_and_zero_tail"),
        (OBJECT, OBJECT + 4, "object_coordinate"),
        (OBJECT + 0x38, OBJECT + 0x3A, "object_record_index"),
        (GEOMETRY, GEOMETRY + 0x200, "controlled_geometry_records"),
        (0x00A8E74C, 0x00A8E758, "setter_installed_center_focal"),
    ]
    trace = RouteTrace(uc, span_ranges(), unicorn, regs,
                       {"exe": exe, "eng3": eng3}, initialized,
                       "native_24a80", inputs)
    return uc, object_x, q12_factors, setter_receipt, trace


def run_case(base, exe, eng3, unicorn, regs, table, row, coord):
    uc, object_x, factors, setter_receipt, trace = setup_case(
        base, exe, eng3, unicorn, regs, table, row, coord)
    slot4 = int.from_bytes(table[4 * 4:4 * 4 + 4], "little")
    if slot4 != SLOT4_TARGET:
        raise RuntimeError("source slot 4 target no longer matches the pinned scratch cell")
    if bytes(uc.mem_read(GEOMETRY + coord * 16, 16))[0xF] != 5:
        raise RuntimeError("controlled record byte +0xF was not installed")
    before_object = bytes(uc.mem_read(OBJECT, 0x200))
    before_record = bytes(uc.mem_read(GEOMETRY + coord * 16, 16))
    before_prefix = bytes(uc.mem_read(ENG_BASE + 0x407A0, TABLE_SIZE))
    result = base.invoke(uc, trace, ENG_BASE + 0x24A80, (OBJECT,), regs, unicorn)
    after_object = bytes(uc.mem_read(OBJECT, 0x200))
    after_prefix = bytes(uc.mem_read(ENG_BASE + 0x407A0, TABLE_SIZE))

    camera_xyz = [int.from_bytes(uc.mem_read(address, 4), "little", signed=True)
                  for address in (0x00A8E71C, 0x00A8E728, 0x00A8E734)]
    screen_xy = [int.from_bytes(uc.mem_read(address, 2), "little", signed=True)
                 for address in (0x00A8E740, 0x00A8E746)]
    matrix_words = [int.from_bytes(uc.mem_read(0x00A8E75C + 2 * i, 2),
                                   "little", signed=True) for i in range(9)]
    translation = [int.from_bytes(uc.mem_read(0x00A8E770 + 4 * i, 4),
                                  "little", signed=True) for i in range(3)]
    projected_words = [int.from_bytes(after_object[o:o + 2], "little", signed=True)
                       for o in (0x0C, 0x0E, 0x10)]
    span_counts = Counter()
    for pc in trace.pcs:
        if ENG_BASE <= pc < ENG_BASE + eng3.pe.OPTIONAL_HEADER.SizeOfImage:
            for label, (rva, size, _) in ENG3_SPANS.items():
                if ENG_BASE + rva <= pc < ENG_BASE + rva + size:
                    span_counts["ENG3+" + hex(rva)] += 1
                    break
        elif EXE_BASE <= pc < EXE_BASE + exe.pe.OPTIONAL_HEADER.SizeOfImage:
            for label, (rva, size, _) in HERC_SPANS.items():
                if EXE_BASE + rva <= pc < EXE_BASE + rva + size:
                    span_counts["HERCULES.EXE+" + hex(rva)] += 1
                    break

    reached_helper = ENG_BASE + 0x24BB0 in trace.pcs
    reached_tail = ENG_BASE + 0x24AEF in trace.pcs
    unbacked_before_close = list(trace.unbacked_reads)
    invalid, unexpected_pc = trace.invalid, trace.unexpected_pc
    violations = list(trace.input_write_violations)
    slot4_writes, slot4_reads = list(trace.slot4_writes), list(trace.slot4_reads)
    trig_reads = list(trace.trig_reads)
    output = {
        "coordinate": coord,
        "object_x_dword": hex(object_x),
        "selected_record_offset": hex(coord * 16),
        "selected_record_stride_bytes": 16,
        "selected_record_before_hex": before_record.hex(),
        "record_byte_plus_0xf": before_record[0xF],
        "setup": setter_receipt,
        "native_call": result,
        "reached_24bb0": reached_helper,
        "reached_success_tail_24aef": reached_tail,
        "camera_xyz_dwords": camera_xyz,
        "screen_xy_words": screen_xy,
        "generated_orientation_words": matrix_words,
        "translation_dwords": translation,
        "projected_object_words_at_plus_0c_0e_10": projected_words,
        "slot4_scratch_target_va": hex(slot4),
        "slot4_scratch_writes": slot4_writes,
        "slot4_scratch_reads_direct_from_memory": slot4_reads,
        "sine_table_reads_direct_from_memory": trig_reads,
        "read_hook_value_mismatches": sum(
            1 for item in trig_reads
            if item["observed_value"] != item["unicorn_hook_value_diagnostic_only"]),
        "uninitialized_non_file_backed_reads": unbacked_before_close,
        "unexpected_pc": hex(unexpected_pc) if unexpected_pc is not None else None,
        "invalid_memory": invalid,
        "controlled_input_write_violations": violations,
        "callback_prefix_unchanged": before_prefix == after_prefix,
        "object_coordinate_and_record_index_unchanged": (
            after_object[:4] == before_object[:4] and
            after_object[0x38:0x3A] == before_object[0x38:0x3A]),
        "executed_span_instruction_counts": dict(sorted(span_counts.items())),
        "executed_instruction_count": len(trace.pcs),
        "controlled_q12_scale_factors_at_bc30_offsets_0_4_8": list(factors),
    }
    trace.close()
    return output


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--eng3", required=True, type=Path)
    parser.add_argument("--output", required=True)
    parser.add_argument("--python-deps", type=Path, default=Path(r"C:\tools\hercules\python"))
    parser.add_argument("--oracle-deps", type=Path, default=Path(r"C:\tools\hercules\renderer-oracle-python"))
    args = parser.parse_args()
    args.repo_root = args.repo_root.resolve()
    args.exe, args.eng3 = args.exe.resolve(), args.eng3.resolve()
    base_path = args.repo_root / "renderer_contract/fixtures/eng3_visibility_source/run_eng3_visibility_source.py"
    audit_path = args.repo_root / "renderer_contract/tests/audit_viewport_visibility.py"
    seam_path = args.repo_root / "renderer_contract/fixtures/eng3_visibility_source/eng3_visibility_seam.c"
    prior_evidence = args.repo_root / "renderer_contract/evidence/eng3_interval_source.json"
    association_evidence = args.repo_root / "renderer_contract/evidence/eng3_geometry_association.json"
    runner_path = Path(__file__).resolve()
    prediction_path = runner_path.parents[1] / "fixtures/eng3_depth_accept/prediction_falsifier.md"
    for required in (base_path, audit_path, seam_path, prior_evidence,
                     association_evidence, prediction_path, args.repo_root / "scripts/pe.py"):
        if not required.is_file():
            parser.error(f"missing public input: {required}")
    if not args.python_deps.is_dir() or not args.oracle_deps.is_dir():
        parser.error("the pinned PE/Capstone and Unicorn dependency directories are required")
    inputs = [args.exe, args.eng3, base_path, audit_path, seam_path,
              prior_evidence, association_evidence, prediction_path, runner_path]
    output_path = check_output(args.output, args.repo_root, inputs)
    if sha(args.exe.read_bytes()) != EXE_SHA or sha(args.eng3.read_bytes()) != ENG3_SHA:
        raise RuntimeError("original full-file SHA-256 mismatch")
    if sha(prediction_path.read_bytes()) != PREDICTION_SHA:
        raise RuntimeError("the immutable pre-experiment prediction hash changed")
    if not args.repo_root.joinpath("work").is_dir():
        raise RuntimeError("--repo-root must have an existing work directory")

    # Use only public host/ENG3 fixture utilities; this candidate has no import
    # path into any work/renderer_phase* experiment lane.
    base = import_file("public_eng3_visibility_source_runner", base_path)
    audit = import_file("public_viewport_visibility_audit", audit_path)
    exe, eng3, unicorn, regs = base.load_runtime(args)
    if unicorn.__version__ != UNICORN_VERSION:
        raise RuntimeError(f"expected Unicorn {UNICORN_VERSION}, got {unicorn.__version__}")
    unicorn_module_path = Path(unicorn.__file__).resolve()
    try:
        unicorn_module_path.relative_to(args.oracle_deps.resolve())
    except ValueError as exc:
        raise RuntimeError("Unicorn did not load from the pinned oracle dependency directory") from exc
    unicorn_dll = unicorn_module_path.parent / "lib" / "unicorn.dll"
    unicorn_dll_sha = sha(unicorn_dll.read_bytes())
    if unicorn_dll_sha != UNICORN_DLL_SHA:
        raise RuntimeError("pinned Unicorn native DLL hash mismatch")
    audit_eng3_pins = set(audit.ENG3_SPANS.values())
    audit_herc_pins = set(audit.HERC_SPANS.values())
    for name, pin in ENG3_SPANS.items():
        if name not in ("interval_row_writer_204c0", "success_epilogue_24aef") and pin not in audit_eng3_pins:
            raise RuntimeError(f"ENG3 span {name} is not present in the public static audit")
    assoc = json.loads(association_evidence.read_text(encoding="utf-8"))
    b740 = next((span for span in assoc["raw_execution"]["unique_covered_original_spans"]
                 if span.get("module") == "HERCULES.EXE" and
                 span.get("rva") == "0xb740" and span.get("size") == 202), None)
    b740_pin = (0xB740, 0xCA, "f745b08e23d8f718c7e269b2642a32a2ea852a1aa86c88dd774df9f555158f2c")
    if b740 is None or b740.get("sha256") != b740_pin[2]:
        raise RuntimeError("observed HERC+0xB740 service is not supported by public geometry evidence")
    if any(pin not in audit_herc_pins for name, pin in HERC_SPANS.items()
           if name != "observed_slot106_service_B740"):
        raise RuntimeError("a HERC span pin is absent from the public viewport audit")
    if HERC_SPANS["observed_slot106_service_B740"] != b740_pin:
        raise RuntimeError("local HERC+0xB740 pin differs from published route evidence")
    pins = pin_spans(exe, eng3)
    prefix = exe.read_rva(TABLE_RVA, TABLE_SIZE)
    if sha(prefix) != TABLE_SHA:
        raise RuntimeError("copied host prefix hash mismatch")
    sine = exe.read_rva(SINE_RVA, SINE_SIZE)
    if sha(sine) != SINE_SHA or exe.rva_to_offset(SINE_RVA) != SINE_FILE_OFFSET:
        raise RuntimeError("file-backed sine slice hash/offset mismatch")
    table_slots = {slot: int.from_bytes(prefix[slot * 4:slot * 4 + 4], "little")
                   for slot in CALLBACK_TARGETS}
    table_slots[4] = int.from_bytes(prefix[4 * 4:4 * 4 + 4], "little")
    for slot, expected in CALLBACK_TARGETS.items():
        if table_slots[slot] != expected:
            raise RuntimeError(f"full original host prefix slot {slot} target mismatch")
    if table_slots[4] != SLOT4_TARGET:
        raise RuntimeError("slot 4 data pointer does not match observed scratch cell")
    data_section = next((section for section in exe.sections if section["name"] == ".data"), None)
    slot4_rva = SLOT4_TARGET - EXE_BASE
    if data_section is None:
        raise RuntimeError("source HERC image has no .data section")
    zero_fill_start = data_section["virtual_address"] + data_section["raw_size"]
    zero_fill_end = data_section["virtual_address"] + data_section["virtual_size"]
    if not (zero_fill_start <= slot4_rva and slot4_rva + 6 <= zero_fill_end):
        raise RuntimeError("slot-4 target is not the pinned .data zero-fill scratch span")

    writer = base.writer_run(exe, eng3, unicorn, regs, 10, 10)
    if (not writer["completed_exact_writer_span"] or
            writer["row_bytes_hex"] != "000000000a00000014000000"):
        raise RuntimeError("original 0x204C0 failed to write the controlled [0,10,20] row")
    row = bytes.fromhex(writer["row_bytes_hex"])
    cases = [run_case(base, exe, eng3, unicorn, regs, prefix, row, coord)
             for coord in (9, 10)]
    reject, accept = cases
    if (reject["native_call"]["eax"] != "0xffffffff" or
            reject["reached_24bb0"] or reject["executed_instruction_count"] != 18):
        raise RuntimeError("coordinate 9 failed its predicted pre-helper rejection")
    expected_scratch = [
        {"address": hex(SLOT4_TARGET), "size": 2, "value": "0x0"},
        {"address": hex(SLOT4_TARGET + 4), "size": 2, "value": "0x280"},
        {"address": hex(SLOT4_TARGET + 2), "size": 2, "value": "0x0"},
    ]
    actual_scratch = [{"address": x["address"], "size": x["size"], "value": x["value"]}
                      for x in accept["slot4_scratch_writes"]]
    if actual_scratch != expected_scratch:
        raise RuntimeError("original ENG3 did not write the measured slot-4 scratch vector")
    expected_reads = [
        {"address": hex(SLOT4_TARGET), "size": 2, "observed_value": "0x0"},
        {"address": hex(SLOT4_TARGET + 2), "size": 2, "observed_value": "0x0"},
        {"address": hex(SLOT4_TARGET + 4), "size": 2, "observed_value": "0x280"},
    ]
    actual_reads = [{"address": x["address"], "size": x["size"],
                     "observed_value": x["observed_value"]}
                    for x in accept["slot4_scratch_reads_direct_from_memory"]]
    if (accept["native_call"]["eax"] != "0x0" or not accept["reached_24bb0"] or
            not accept["reached_success_tail_24aef"] or actual_reads != expected_reads or
            accept["camera_xyz_dwords"] != [0, 0, 640] or
            accept["projected_object_words_at_plus_0c_0e_10"] != [0, 0, 160]):
        raise RuntimeError("coordinate 10 did not pass the measured native depth route")
    for case in cases:
        if (case["unexpected_pc"] is not None or case["invalid_memory"] is not None or
                case["uninitialized_non_file_backed_reads"] or
                case["controlled_input_write_violations"] or not case["callback_prefix_unchanged"]):
            raise RuntimeError(f"controlled native case did not close cleanly: {case['coordinate']}")
    if not accept["sine_table_reads_direct_from_memory"]:
        raise RuntimeError("accepted path did not directly observe the pinned file-backed table")
    if (len(accept["sine_table_reads_direct_from_memory"]) != 8 or
            accept["read_hook_value_mismatches"] != 4 or
            any(not item["fully_inside_pinned_slice"]
                for item in accept["sine_table_reads_direct_from_memory"])):
        raise RuntimeError("direct source-table read characterization changed")

    prior = json.loads(prior_evidence.read_text(encoding="utf-8"))
    if prior.get("proof_levels", {}).get("bounded_source_seam") != "BOUNDED_EQUIVALENT" or \
            prior.get("proof_levels", {}).get("binary_span_identity") != "STATIC_OBSERVED":
        raise RuntimeError("public prior ENG3 interval source evidence has unexpected proof labels")
    prior_source_hashes = prior.get("source_sha256", {})
    for relative, path in (("renderer_contract/fixtures/eng3_visibility_source/eng3_visibility_seam.c", seam_path),
                           ("renderer_contract/fixtures/eng3_visibility_source/run_eng3_visibility_source.py", base_path)):
        if prior_source_hashes.get(relative) != sha(path.read_bytes()):
            raise RuntimeError(f"public source-seam evidence no longer pins {relative}")
    report = {
        "schema_version": 1,
        "proofs": {
            "original_code_span_identity": "STATIC_OBSERVED",
            "prior_public_c_interval_seam": "BOUNDED_EQUIVALENT",
            "coordinate_9_native_interval_reject": "BEHAVIOR_VERIFIED",
            "coordinate_10_native_depth_accept": "BEHAVIOR_VERIFIED",
            "live_scene_visibility_through_emission": "UNRESOLVED",
        },
        "scope": "Two finite controlled calls to original ENG3 0x24A80, with its original 0x24BB0 helper and pinned original HERC callbacks. Coordinate 9 rejects before the helper. Coordinate 10 derives slot-4 scratch, passes through the original transform and slot-121 projection, then returns EAX=0. No live resource, current object list, packet destination, emitted polygon, or gameplay is claimed.",
        "originals": {
            "HERCULES.EXE": {"path": str(args.exe), "sha256": EXE_SHA},
            "ENG3.DLL": {"path": str(args.eng3), "sha256": ENG3_SHA},
        },
        "public_code_sources": {
            "native_fixture_runner": {"path": "renderer_contract/fixtures/eng3_visibility_source/run_eng3_visibility_source.py", "sha256": sha(base_path.read_bytes())},
            "static_span_audit": {"path": "renderer_contract/tests/audit_viewport_visibility.py", "sha256": sha((args.repo_root / "renderer_contract/tests/audit_viewport_visibility.py").read_bytes())},
            "prior_geometry_association_evidence": {"path": "renderer_contract/evidence/eng3_geometry_association.json", "sha256": sha(association_evidence.read_bytes()), "b740_span": b740},
            "c_interval_candidate": {"path": "renderer_contract/fixtures/eng3_visibility_source/eng3_visibility_seam.c", "sha256": sha(seam_path.read_bytes())},
            "prior_source_evidence": {"path": "renderer_contract/evidence/eng3_interval_source.json", "sha256": sha(prior_evidence.read_bytes()), "proof_levels": prior["proof_levels"]},
            "candidate_prediction": {"path": "renderer_contract/fixtures/eng3_depth_accept/prediction_falsifier.md", "sha256": PREDICTION_SHA},
            "runner": {"path": "renderer_contract/tests/audit_eng3_depth_accept.py", "sha256": sha(Path(__file__).read_bytes())},
        },
        "runtime": {"engine": "Unicorn x86-32 CPU emulation", "version": unicorn.__version__,
                    "module_path": str(unicorn_module_path),
                    "native_dll_sha256": unicorn_dll_sha,
                    "source_pe_reader_path": str(args.repo_root / "scripts/pe.py"),
                    "source_pe_reader_sha256": sha((args.repo_root / "scripts/pe.py").read_bytes())},
        "pinned_spans": pins,
        "callback_prefix": {
            "source_rva": hex(TABLE_RVA), "destination_eng3_va": hex(ENG_BASE + 0x407A0),
            "length": TABLE_SIZE, "sha256": TABLE_SHA, "copied_byte_for_byte": True,
            "slot_targets": {str(slot): hex(value) for slot, value in table_slots.items()},
            "slot4_scratch_target": hex(SLOT4_TARGET),
            "slot4_target_section": ".data",
            "slot4_target_zero_fill_rva_range": [hex(zero_fill_start), hex(zero_fill_end)],
            "slot4_target_used_rva_span": [hex(slot4_rva), hex(slot4_rva + 6)],
            "slot5_source_va": hex(SINE_VA), "slot5_data_rva": hex(SINE_RVA),
            "slot5_file_offset": hex(SINE_FILE_OFFSET), "slot5_data_size": SINE_SIZE,
            "slot5_data_sha256": SINE_SHA,
            "slot5_signed_word_samples": {hex(index): int.from_bytes(sine[index * 2:index * 2 + 2], "little", signed=True)
                                           for index in (0, 0x400, 0x800, 0xC00, 0xFFF)},
        },
        "controlled_writer_row": {"input": {"lower": 10, "extent": 10}, **writer},
        "controlled_native_inputs": {
            "coordinates": [9, 10], "object_x_bits": [reject["object_x_dword"], accept["object_x_dword"]],
            "row_bytes_hex": row.hex(), "geometry_record_stride_bytes": 16,
            "selected_record_byte_plus_0xf": 5,
            "slot165_q12_scale_factors_at_dword_offsets_0_4_8": [4096, 4096, 4096],
            "zero_angle_globals": ["ENG3+0x405a0", "ENG3+0x405a4", "ENG3+0x405a8", "ENG3+0x405b0", "ENG3+0x405ac", "ENG3+0x40640"],
            "translation_observed": [0, 0, 0],
            "center_and_focal": {"center": [0, 0], "focal": 256, "setup": "original ENG3 wrappers 0x1fff0/0x20010 through original HERC slots 174/175"},
            "state_limit": "Controlled fixture values only; no runtime table, matrix object, scene, camera, or level binding is inferred.",
        },
        "cases": cases,
        "historical_hook_receipt": {
            "path": "work/renderer_phase19/eng3_prefix_tail/runs/positive_tail_v1.json",
            "sha256": "c8662081c0e4318f59fcb5b8d09d62462a32ec52ceb72f8d6c6805e74c066b40",
            "note": "Preserved unchanged as historical. Its Unicorn read-hook value at slot-5 offset 0 was zero; corrected receipts use uc.mem_read during the hook and observe 0x1000 from the pinned file-backed table.",
        },
        "unresolved": [
            "No runtime bound-table or geometry-record producer/resource identity is established.",
            "No live caller object-list flags, visibility traversal, 0x8DA0 route, or 0x231A0 packet emission was executed.",
            "EAX=0 proves this controlled helper call avoids the caller's -1 rejection value only; it does not prove emitted geometry.",
        ],
    }
    output_path.parent.mkdir(parents=True, exist_ok=True)
    # Recheck after creating output parents, then create exclusively.
    output_path = check_output(str(output_path), args.repo_root, inputs)
    with output_path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"status": "PASS", "output": str(output_path),
                      "cases": [{"coordinate": c["coordinate"],
                                 "eax": c["native_call"]["eax"],
                                 "reached_24bb0": c["reached_24bb0"],
                                 "camera_xyz": c["camera_xyz_dwords"],
                                 "projected_words": c["projected_object_words_at_plus_0c_0e_10"],
                                 "instruction_count": c["executed_instruction_count"]}
                                for c in cases]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
