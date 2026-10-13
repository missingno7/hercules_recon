#!/usr/bin/env python3
"""Read-only, hash-pinned ENG3 viewport and visibility trace.

This audit decodes only the pinned ENG3 spans named below. It never executes an
original image and writes only a fresh JSON receipt under the selected repo's
work directory.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import struct
import sys
from pathlib import Path

X86_OP_IMM = 2
X86_OP_MEM = 3

EXPECTED_IMAGES = {
    "HERCULES.EXE": {
        "sha256": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
        "image_base": 0x400000,
    },
    "ENG3.DLL": {
        "sha256": "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
        "image_base": 0x10000000,
    },
}

# (RVA, byte count, expected SHA-256). Central ENG3 functions are complete
# RET-terminated bodies; wrapper spans can include a trailing alignment NOP.
ENG3_SPANS = {
    "parent_handler_22e40": (0x22E40, 0xBE, "02f8a482d847ad9a5e2c09004c6620c7e3bec0c6a60ab9747e9646a419fafd59"),
    "child_projection_gate_24770": (0x24770, 0x61, "5c0953eac674c686ec00e387a1bc196b4f84865a8df3f1d6b770f805b4d04f27"),
    "indexed_parent_predicate_24a80": (0x24A80, 0x6F, "3a9fc30527d0f722cd6a1e16e8f5c5ba3b7f3e8bafa3fefc6ab2c1a172e89e85"),
    "parent_transform_helper_24bb0": (0x24BB0, 0x39B, "a0ad5e76036c8f94d63730e28691147bf7b8b80d64e2d9dc4a767c96868f026a"),
    "shared_object_renderer_8da0": (0x8DA0, 0x28D, "0996e678d14d7404ea0f35e928bacf3956eb387bfd4416171e3e86776f84c057"),
    "ft4_producer_231a0": (0x231A0, 0x293, "9ab5bed4f7535a97f69661b394b86fd851fbf8c631a873705f1c3a7c89567c96"),
    "wrapper_slot101_1fca0": (0x1FCA0, 0x0F, "ee6fddcba7224cafa291397c8a3ada0fd1ff8ef20b9cca5e9ea3a6c7ded280fb"),
    "wrapper_slot106_1fd00": (0x1FD00, 0x06, "11f370d92cc7151d75bd2d3788180bcf296810f1dd23836c264c36db31f1c12d"),
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
    "slot165_matrix_product_BC30": (0xBC30, 0x9F, "8cf8663405ada692c0a993543b4dc1af1681774963575c3cd50c0364b161d040"),
    "slot166_zero_return_A340": (0xA340, 0x03, "4bc724f3b1d0caf4fe369c18cba3102e6c4ea057f63fe1587e3973134a7f755e"),
    "slot174_projection_center_B970": (0xB970, 0x14, "2526cc71c8d43572f1aabfacc70678e699908b02a13cf0b72f832b1e4062d252"),
    "slot175_projection_focal_B990": (0xB990, 0x0A, "c44e898f7d175fbc52e1761ebbf409c20d9579c27174ca44a631d31909e8c68a"),
    "source_callback_table": (0x361B8, 0x3E8, "52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8"),
}

ENG3_TABLE_BASE_VA = 0x100407A0
HOST_TABLE_RVA = 0x361B8
HOST_TARGETS = {
    101: 0xB6F0,
    106: 0xB920,
    108: 0xB880,
    109: 0xB8F0,
    114: 0xB720,
    121: 0xB870,
    164: 0xBB00,
    165: 0xBC30,
    166: 0xA340,
    174: 0xB970,
    175: 0xB990,
}
WRAPPER_CELLS = {
    0x1FCA0: (101, 0x10040934, "call"),
    0x1FD00: (106, 0x10040948, "jmp"),
    0x1FD20: (108, 0x10040950, "call"),
    0x1FD30: (109, 0x10040954, "call"),
    0x1FD80: (114, 0x10040968, "call"),
    0x1FDD0: (121, 0x10040984, "call"),
    0x1FF40: (164, 0x10040A30, "call"),
    0x1FF60: (165, 0x10040A34, "call"),
    0x1FFF0: (174, 0x10040A58, "call"),
    0x20010: (175, 0x10040A5C, "call"),
}

DIRECT_CALL_SITES = {
    "parent_handler_22e40": {
        0x22E4B: 0x24A80,
        0x22EA6: 0x1FF40,
        0x22EB3: 0x1FD20,
        0x22EBB: 0x1FCD0,
        0x22ED6: 0x24770,
        0x22EE3: 0xA410,
        0x22EEC: 0x8DA0,
    },
    "indexed_parent_predicate_24a80": {
        0x24AB9: 0x24BB0,
        0x24AC7: 0x1FD80,
        0x24AD3: 0x1FDD0,
    },
    "child_projection_gate_24770": {
        0x2479D: 0x1FCA0,
        0x247A5: 0x1FD00,
        0x247AF: 0x1FD80,
        0x247BB: 0x1FDD0,
    },
    "parent_transform_helper_24bb0": {
        0x24CB4: 0x1FF40,
        0x24CC6: 0x1FF60,
        0x24CD3: 0x1FD20,
        0x24CE0: 0x1FD30,
        0x24E36: 0x1FF40,
        0x24E48: 0x1FF60,
        0x24E55: 0x1FD20,
        0x24E62: 0x1FD30,
        0x24F33: 0x1FCA0,
        0x24F3B: 0x1FD00,
    },
    "shared_object_renderer_8da0": {0x901D: 0x231A0},
}

SETTER_WRAPPERS = {0x1FFF0, 0x20010}
SELECTED_FUNCTIONS = (
    "parent_handler_22e40",
    "child_projection_gate_24770",
    "indexed_parent_predicate_24a80",
    "parent_transform_helper_24bb0",
    "shared_object_renderer_8da0",
    "ft4_producer_231a0",
)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def add_under(path: Path, root: Path) -> bool:
    try:
        path.relative_to(root)
        return True
    except ValueError:
        return False


def output_path(raw: str, repo_root: Path, inputs: list[Path]) -> Path:
    supplied = Path(raw)
    if not supplied.is_absolute():
        supplied = repo_root / supplied
    lexical = Path(os.path.abspath(supplied))
    resolved = lexical.resolve(strict=False)
    work_lexical = Path(os.path.abspath(repo_root / "work"))
    work_resolved = (repo_root / "work").resolve()
    if not add_under(lexical, work_lexical) or not add_under(resolved, work_resolved):
        fail("--output must resolve lexically and physically beneath <repo-root>/work")
    if lexical.suffix.lower() != ".json":
        fail("--output must end in .json")
    if lexical.is_symlink() or lexical.exists():
        fail("--output must be a new, nonexistent file (no symlink or overwrite)")
    if any(resolved == item.resolve() for item in inputs):
        fail("--output aliases an input original")
    return lexical


def decode(md, image, rva: int, size: int):
    raw = image.read_rva(rva, size)
    insns = list(md.disasm(raw, rva))
    if not insns or insns[0].address != rva or insns[-1].address + insns[-1].size != rva + size:
        fail(f"incomplete instruction coverage at {rva:#x}+{size:#x}")
    cursor = rva
    for ins in insns:
        if ins.address != cursor:
            fail(f"instruction gap at {cursor:#x}")
        cursor += ins.size
    return insns


def pin_spans(image, spans: dict, *, decode_code: bool, md=None, ret_terminated=()):
    pinned = {}
    for name, (rva, size, expected) in spans.items():
        raw = image.read_rva(rva, size)
        actual = sha(raw)
        if actual != expected:
            fail(f"{name} hash mismatch at {rva:#x}: {actual}")
        item = {
            "rva": hex(rva),
            "size": size,
            "sha256": actual,
            "byte_coverage": f"all {size} bytes in [{rva:#x},{rva + size:#x}) are file-backed",
        }
        if decode_code:
            insns = decode(md, image, rva, size)
            item["decoded_instruction_count"] = len(insns)
            item["final_instruction"] = {
                "rva": hex(insns[-1].address),
                "mnemonic": insns[-1].mnemonic,
                "operands": insns[-1].op_str,
            }
            if name in ret_terminated and insns[-1].mnemonic != "ret":
                fail(f"{name} no longer ends in RET")
        pinned[name] = item
    return pinned


def all_selected_calls(md, image, pinned_spans):
    decoded = {}
    for name in SELECTED_FUNCTIONS:
        rva, size, _ = ENG3_SPANS[name]
        decoded[name] = decode(md, image, rva, size)
    checked = {}
    for name, sites in DIRECT_CALL_SITES.items():
        ins_by_address = {ins.address: ins for ins in decoded[name]}
        rows = []
        for site, target in sites.items():
            ins = ins_by_address.get(site)
            if (ins is None or ins.mnemonic != "call" or not ins.operands
                    or ins.operands[0].type != X86_OP_IMM or ins.operands[0].imm != target):
                fail(f"expected direct call {site:#x} -> {target:#x} in {name}")
            rows.append({"site": hex(site), "target_rva": hex(target)})
        checked[name] = rows

    setter_calls = []
    setter_cells = []
    for name, insns in decoded.items():
        for ins in insns:
            if ins.mnemonic == "call" and ins.operands and ins.operands[0].type == X86_OP_IMM:
                if ins.operands[0].imm in SETTER_WRAPPERS:
                    setter_calls.append({"function": name, "site": hex(ins.address),
                                         "target_rva": hex(ins.operands[0].imm)})
            if ins.mnemonic == "call" and ins.operands and ins.operands[0].type == X86_OP_MEM:
                mem = ins.operands[0].mem
                if mem.base == 0 and mem.index == 0 and mem.disp in (0x10040A58, 0x10040A5C):
                    setter_cells.append({"function": name, "site": hex(ins.address),
                                         "cell_va": hex(mem.disp)})
    if setter_calls or setter_cells:
        fail("a pinned selected function directly calls or invokes setter slot 174/175")
    return decoded, checked, {"direct_wrapper_calls": setter_calls, "indirect_table_calls": setter_cells}


def wrapper_binding(md, image):
    records = []
    for rva, (slot, expected_cell, expected_mnemonic) in WRAPPER_CELLS.items():
        name = next(n for n, (start, _, _) in ENG3_SPANS.items() if start == rva)
        start, size, _ = ENG3_SPANS[name]
        insns = decode(md, image, start, size)
        transfers = []
        for ins in insns:
            if ins.mnemonic in ("call", "jmp") and ins.operands and ins.operands[0].type == X86_OP_MEM:
                mem = ins.operands[0].mem
                if mem.base == 0 and mem.index == 0:
                    transfers.append((ins, mem.disp))
        if len(transfers) != 1:
            fail(f"wrapper {rva:#x} did not have exactly one absolute indirect transfer")
        transfer, cell = transfers[0]
        if cell != expected_cell or transfer.mnemonic != expected_mnemonic:
            fail(f"wrapper {rva:#x} callback cell/transfer changed")
        computed_slot = (cell - ENG3_TABLE_BASE_VA) // 4
        if cell < ENG3_TABLE_BASE_VA or (cell - ENG3_TABLE_BASE_VA) % 4 or computed_slot != slot:
            fail(f"wrapper {rva:#x} cell does not map to expected slot {slot}")
        records.append({
            "wrapper_rva": hex(rva),
            "transfer_rva": hex(transfer.address),
            "transfer": transfer.mnemonic,
            "cell_va": hex(cell),
            "slot": slot,
        })
    return records


def check_key_instructions(decoded):
    expected = {
        "indexed_parent_predicate_24a80": {
            0x24A88: ("sar", "eax, 0x10"),
            0x24A8B: ("movsx", "ecx, ax"),
            0x24A8E: ("xor", "eax, eax"),
            0x24A90: ("mov", "ax, word ptr [esi + 0x38]"),
            0x24A94: ("sar", "ecx, 7"),
            0x24A97: ("lea", "edx, [eax + eax*2]"),
            0x24A9A: ("mov", "eax, dword ptr [0x1004078c]"),
            0x24A9F: ("lea", "eax, [eax + edx*4]"),
            0x24AA2: ("cmp", "ecx, dword ptr [eax + 4]"),
            0x24AA5: ("jge", "0x24aad"),
            0x24AAD: ("cmp", "ecx, dword ptr [eax + 8]"),
            0x24AB0: ("jl", "0x24ab8"),
            0x24ADB: ("sub", "edx, 0x20"),
            0x24AD8: ("movsx", "edx, word ptr [esi]"),
            0x24AE1: ("cmp", "edx, 0x620"),
            0x24AE7: ("jbe", "0x24aef"),
        },
        "child_projection_gate_24770": {
            0x247C5: ("cmp", "word ptr [esi], 0x40"),
            0x247CA: ("setg", "al"),
        },
        "parent_handler_22e40": {
            0x22E53: ("cmp", "eax, -1"),
            0x22E56: ("jne", "0x22e63"),
            0x22E58: ("or", "byte ptr [esi + 0x1f], 8"),
            0x22EE0: ("je", "0x22ef4"),
        },
    }
    checked = {}
    for name, sites in expected.items():
        ins_by_address = {ins.address: ins for ins in decoded[name]}
        rows = []
        for address, (mnemonic, operands) in sites.items():
            ins = ins_by_address.get(address)
            if ins is None or ins.mnemonic != mnemonic or ins.op_str != operands:
                fail(f"expected instruction {address:#x}: {mnemonic} {operands}")
            rows.append({"rva": hex(address), "mnemonic": mnemonic, "operands": operands})
        checked[name] = rows
    return checked


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, help="repository root containing scripts/ and work/")
    parser.add_argument("--exe", required=True, help="hash-matching original HERCULES.EXE")
    parser.add_argument("--eng3", required=True, help="hash-matching original ENG3.DLL")
    parser.add_argument("--output", required=True, help="new .json receipt strictly beneath <repo-root>/work")
    args = parser.parse_args()

    repo_root = Path(os.path.abspath(args.repo_root)).resolve(strict=False)
    exe_path = Path(os.path.abspath(args.exe)).resolve(strict=False)
    eng3_path = Path(os.path.abspath(args.eng3)).resolve(strict=False)
    if not repo_root.is_dir():
        fail("--repo-root must name an existing directory")
    if not exe_path.is_file() or not eng3_path.is_file():
        fail("--exe and --eng3 must name existing files")
    out_path = output_path(args.output, repo_root, [exe_path, eng3_path])
    if not (repo_root / "scripts" / "pe.py").is_file():
        fail("--repo-root does not contain scripts/pe.py")
    sys.path.insert(0, str(repo_root / "scripts"))
    import env  # noqa: F401 - public local dependency setup
    import capstone
    from pe import PE

    if capstone.__version__ != "5.0.7":
        fail(f"expected Capstone 5.0.7, found {capstone.__version__}")
    images = {
        "HERCULES.EXE": PE(exe_path),
        "ENG3.DLL": PE(eng3_path),
    }
    image_records = {}
    for name, image in images.items():
        expected = EXPECTED_IMAGES[name]
        actual = sha(image.data)
        if actual != expected["sha256"]:
            fail(f"{name} full-image SHA-256 mismatch: {actual}")
        if image.image_base != expected["image_base"]:
            fail(f"{name} image base mismatch: {image.image_base:#x}")
        image_records[name] = {
            "path": str(image.path.resolve()),
            "sha256": actual,
            "image_base": hex(image.image_base),
        }

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    herc_code_spans = {name: spec for name, spec in HERC_SPANS.items()
                       if name != "source_callback_table"}
    herc_pins = pin_spans(images["HERCULES.EXE"], herc_code_spans,
                          decode_code=True, md=md)
    table_rva, table_size, table_sha256 = HERC_SPANS["source_callback_table"]
    table_bytes = images["HERCULES.EXE"].read_rva(table_rva, table_size)
    if sha(table_bytes) != table_sha256:
        fail("HERC source callback table hash mismatch")
    herc_pins["source_callback_table"] = {
        "rva": hex(table_rva),
        "size": table_size,
        "sha256": sha(table_bytes),
        "byte_coverage": f"all {table_size} bytes in [{table_rva:#x},{table_rva + table_size:#x}) are file-backed",
        "content_kind": "data",
    }
    eng3_pins = pin_spans(
        images["ENG3.DLL"],
        ENG3_SPANS,
        decode_code=True,
        md=md,
        ret_terminated=(
            "parent_handler_22e40", "child_projection_gate_24770",
            "indexed_parent_predicate_24a80", "parent_transform_helper_24bb0",
            "shared_object_renderer_8da0", "ft4_producer_231a0",
        ),
    )
    decoded, call_sites, setter_absence = all_selected_calls(md, images["ENG3.DLL"], eng3_pins)
    key_instructions = check_key_instructions(decoded)
    wrapper_map = wrapper_binding(md, images["ENG3.DLL"])

    host_table = table_bytes
    host_targets = {}
    for slot, expected_rva in HOST_TARGETS.items():
        absolute = struct.unpack_from("<I", host_table, slot * 4)[0]
        target_rva = absolute - images["HERCULES.EXE"].image_base
        if target_rva != expected_rva:
            fail(f"HERC source table slot {slot} target changed: {target_rva:#x}")
        host_targets[str(slot)] = {
            "table_cell_rva": hex(HOST_TABLE_RVA + slot * 4),
            "target_va": hex(absolute),
            "target_rva": hex(target_rva),
        }

    report = {
        "schema_version": 1,
        "status": "PASS",
        "proof_level": "STATIC_OBSERVED",
        "scope": "Hash-pinned static trace of one ENG3 parent/child visibility route and its fixed-width host callback bindings. No original execution or rendered-output claim.",
        "originals": image_records,
        "tooling": {
            "python": sys.version.split()[0],
            "capstone": capstone.__version__,
            "pe_reader": "scripts/pe.py",
            "mode": "32-bit x86 static decode; no machine execution",
        },
        "span_pins": {
            "ENG3.DLL": eng3_pins,
            "HERCULES.EXE": herc_pins,
        },
        "callback_wrapper_cells": wrapper_map,
        "source_host_table": {
            "rva": hex(HOST_TABLE_RVA),
            "size": 0x3E8,
            "sha256": HERC_SPANS["source_callback_table"][2],
            "selected_targets": host_targets,
        },
        "selected_direct_call_sites": call_sites,
        "selected_setter_wrapper_absence": {
            "scope": list(SELECTED_FUNCTIONS),
            "slot_174_wrapper_rva": "0x1fff0",
            "slot_175_wrapper_rva": "0x20010",
            "result": setter_absence,
            "claim": "No direct calls to the slot-174/175 wrappers and no indirect calls through their copied-table cells occur in these six pinned spans.",
        },
        "predicate_assertions": key_instructions,
        "observed_arithmetic": {
            "indexed_coordinate": "signed high WORD of object DWORD at arg+0, arithmetic-shifted right 7",
            "bounds_index": "WORD at arg+0x38 is zero-extended to 32 bits; LEA sequence forms 3*index then base + 4*(3*index), i.e. 32-bit base + 12*index",
            "bounds_record_reads": "signed 32-bit lower at record+4 and upper at record+8; accepts lower <= coordinate < upper",
            "parent_depth_gate": "slot 121 writes a WORD at object+0x10; signed WORD is sign-extended, 0x20 subtracted in 32-bit arithmetic, and accepted when unsigned result <= 0x620 (depth 0x20..0x640 inclusive)",
            "child_depth_gate": "slot 121 writes a WORD at child+4; signed WORD comparison requires depth > 0x40 (strict)",
        },
        "visibility_effects": {
            "parent_reject": "ENG3 0x22E40 sets bit 0x08 at object byte+0x1F and returns before child traversal when 0x24A80 returns -1",
            "child_reject": "ENG3 0x24770 returns false unless its signed depth WORD is greater than 0x40; 0x22E40 skips 0xA410/0x8DA0 for a false result",
            "screen_rectangle": "No projected X/Y rectangle comparison appears in these pinned parent/child gates; parent projected XY is written before its depth test.",
        },
        "unresolved": [
            "Runtime pointer value and table contents reached through ENG3 VA 0x1004078C, including bound values and their units/meaning.",
            "Producer of slot-174/175 projection-center/focal state and actual runtime center/focal values.",
            "Whether the indexed bounds represent a camera frustum, level/sector limits, or another domain.",
            "No 160x120 runtime center or widescreen-adjusted bounds are established by this static trace.",
        ],
        "limits": [
            "The HERCULES.EXE callback table source bytes establish callback target RVAs, not runtime initialization order or call-time global values.",
            "The selected-span absence check is local to the six named ENG3 bodies and does not claim whole-module absence.",
            "This audit does not execute the producer, reconstruct full camera setup, infer table allocation/layout beyond the observed address formula, or model all projection arithmetic.",
            "320x240 is an established native presentation size; 160x120 is only half-size arithmetic unless independently sourced as a runtime argument.",
        ],
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    data = json.dumps(report, indent=2, sort_keys=True) + "\n"
    try:
        with out_path.open("x", encoding="utf-8", newline="\n") as stream:
            stream.write(data)
    except FileExistsError:
        fail(f"--output already exists: {out_path}")
    print(json.dumps({"status": "PASS", "proof_level": "STATIC_OBSERVED", "output": str(out_path)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
