#!/usr/bin/env python3
"""Audit the four unclassified TITLE mode-table entries from pinned PE bytes.

This is a read-only original-image audit. It uses the checkout's public PE
reader and installed Capstone 5.0.7; all generated JSON must be explicitly
placed under this checkout's work/ directory.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys


sys.dont_write_bytecode = True


def find_repo_root() -> Path:
    """Find the checkout root from either this scratch path or tests/."""
    script = Path(__file__).resolve()
    for candidate in (script.parent, *script.parents):
        if (candidate / "scripts" / "pe.py").is_file() and (candidate / "renderer_contract").is_dir():
            return candidate
    raise FileNotFoundError("could not locate checkout containing scripts/pe.py and renderer_contract")


REPO = find_repo_root()
sys.path.insert(0, str(REPO / "scripts"))
import env  # noqa: F401,E402
from pe import PE  # noqa: E402
import capstone  # noqa: E402


TITLE_SHA256 = "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b"
HERCULES_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
TITLE_BASE = 0x10000000
HERCULES_BASE = 0x00400000

# Mode-target extents are independently pinned to existing evidence or the
# next known function entry. Only the small direct-call/dataflow closure needed
# for these four table entries is included.
TITLE_SPANS = {
    "mode_table": (0x220D0, 0x20),
    "mode_2_list": (0x17B90, 0x60),
    "mode_3_clip_list": (0x17BF0, 0xC0),
    "mode_4_group": (0x17CB0, 0x2A0),
    "mode_6_sprite_quad_dispatch": (0x3D10, 0x90),
    "dynamic_child_renderer": (0x2DC0, 0x290),
    "group_child_setup": (0x2CC0, 0x100),
    "mode_0_scale_dispatch": (0x3050, 0xB0),
    "mode_1_ft4_writer": (0x3100, 0x320),
    "mode_5_known_ft4_writer": (0x181E0, 0x2DC),
    "mode_7_known_ft4_writer": (0x186E0, 0x2B3),
    "overlay_ft4_writer": (0x17F50, 0x290),
    "ft4_writer_03420": (0x3420, 0x320),
    "ft4_writer_03740": (0x3740, 0x270),
    "ft4_writer_039B0": (0x39B0, 0x360),
    "ft4_command_word": (0x189A0, 0x3C),
}

# Byte-checked sites are the smallest original spans supporting the report's
# call edges, tag stores, linked-list insertion, and cursor increments.
SITES = {
    0x17BA5: (4, "52188615804188edc3d3bc26865090376f64f434c5f269da73ebec9532c2d626"),
    0x17BDA: (5, "3349434b8f7654cd16db99c322d330ea1eb63b9f308dd31e7179e53f16c7f350"),
    0x17C08: (4, "52188615804188edc3d3bc26865090376f64f434c5f269da73ebec9532c2d626"),
    0x17C56: (5, "ab7f4dfa8dcf84ddf3841f4fe393e257556d65e9441a2af92f24408d94a844f3"),
    0x17C63: (5, "e38dca78e45c254b079b6e0ca9f43287dc3414f7678c06f1da0a1f026b024b78"),
    0x17C6B: (5, "7897373cfe51b80f157c07e069aed01a70d460b353c58eda9a9c8c81b51e6be1"),
    0x17C9C: (5, "287e381e0b3b37c64e8562a0fb8231d84623c31955b5da34a6bf72652bc4b22b"),
    0x17CDD: (5, "5ef32911ec7d60a4ffa1d3093ea1fe0c989c3034ccfef120e552f281e5b1431e"),
    0x17CEE: (5, "09a85bbad4d922f4acaa129119051240e1f723c8c1318fb809e66bcf2e6b624a"),
    0x17D09: (5, "c5ff616a177aa386843697c319536942e8d0895c06a1868687c718f3f733a876"),
    0x17DD3: (5, "d3362c1b320a8ebd34fb6d503121722fae6756bc2cd2795cc0a9e6f619602bbe"),
    0x17DF9: (4, "515d1d1a66c7e3faddbb8075a6d195e20af4f8f80e5d39e791591beb7ccdb3e9"),
    0x17E05: (2, "587e5ed0ff7c93c43c214b6cd2b6c4d93dd6b151748286cfd2f164494eb7c611"),
    0x17E0C: (2, "a53da58892b666a30b1af2dd65a1ea39829c3490ff505de5c07611f8b1380dcb"),
    0x17E13: (3, "1bec05278b5cbaf20616ad46f2a5d717ecd49587e3b7cb5eb5c894a288b16b78"),
    0x17E16: (5, "71fd2216be84d97f06f81cdb517cb3bdf183eb3e673956a8018dbb3f76218ab3"),
    0x17F1B: (5, "64118f410309ea52b72e2fcb20ad03aa3ccfb26e0aef66f1a1eb70e8daa7bf14"),
    0x17F29: (5, "851a0f6086f1af8cb15e892097a57d6d4a96731b4c1a45e4b24207ef9b30609c"),
    0x2FDE: (3, "934e3190389bcd9c5833c880518fa2b8b19a2deca93d267428c8e770ac4c5161"),
    0x300D: (7, "07ffe97b20c6841efa7efc790f71b3efb094a39c3334b4b9ba6e9fa0278dd013"),
    0x3D1D: (4, "67e284a451a2d71c4c43e1cc8d9b04a38fca8a4ad69266b66d9c73544071c2a9"),
    0x3D38: (6, "02d883c1820363bd17d01553d60826897aa2244f5eba66d1c507b960a1723eaf"),
    0x3D48: (5, "b6f729c616c1489bffd858f5f398cfc6d9b503253fc9c6186c91a62927026c51"),
    0x3D52: (5, "6248cc2224b841a47bdec11b8c085f06b6e7547588cfab03bb70ab9e086e5bb5"),
    0x3D5D: (5, "12d2f936aac5c660c3e2d296998d26efeb1a2755c068780fc3af1acf10924149"),
    0x3D68: (5, "b6f729c616c1489bffd858f5f398cfc6d9b503253fc9c6186c91a62927026c51"),
    0x3D7C: (5, "7376b0cb274be8f0a8084f049b18edf62bb53cc7a2592cccc121793893051152"),
    0x3D87: (5, "40cdad83a87af22b22ba2b67be6c3771f66901efdc37de42daa8a94c9ab330df"),
    0x33BB: (3, "fc8e692c1cd4e041941c2c707759b59330f639f98842dffa2390180cf012cf47"),
    0x33CD: (2, "3db07e485d45f7517b4010dec7d0dcb8f346ac6796685a2da74d7ee2bd67e6c4"),
    0x33CF: (2, "587e5ed0ff7c93c43c214b6cd2b6c4d93dd6b151748286cfd2f164494eb7c611"),
    0x33D3: (3, "e72aa9883b867c0a0ec62ef6e8dbc3f45e26af1ee7123732593b3ec2307cb644"),
    0x340E: (6, "9f6d2d31ac00e2c6788b19c8993e219ea81a6262b1eec26b63a0b67133f5f831"),
    0x36DD: (3, "fc8e692c1cd4e041941c2c707759b59330f639f98842dffa2390180cf012cf47"),
    0x36E6: (2, "3db07e485d45f7517b4010dec7d0dcb8f346ac6796685a2da74d7ee2bd67e6c4"),
    0x36E8: (2, "587e5ed0ff7c93c43c214b6cd2b6c4d93dd6b151748286cfd2f164494eb7c611"),
    0x36F9: (3, "e72aa9883b867c0a0ec62ef6e8dbc3f45e26af1ee7123732593b3ec2307cb644"),
    0x3730: (6, "9f6d2d31ac00e2c6788b19c8993e219ea81a6262b1eec26b63a0b67133f5f831"),
    0x3943: (3, "924188d8a625584577bbc9445f5326eb7d9abca9e1ba8ce5ed55ef03340c39ea"),
    0x3955: (2, "3db07e485d45f7517b4010dec7d0dcb8f346ac6796685a2da74d7ee2bd67e6c4"),
    0x3957: (2, "c55224d3f69c8257578311b99e9bf41fd65c977195aa2bbbc849a87ba1b21a38"),
    0x395C: (3, "e72aa9883b867c0a0ec62ef6e8dbc3f45e26af1ee7123732593b3ec2307cb644"),
    0x3997: (6, "9f6d2d31ac00e2c6788b19c8993e219ea81a6262b1eec26b63a0b67133f5f831"),
    0x3CA6: (3, "fc8e692c1cd4e041941c2c707759b59330f639f98842dffa2390180cf012cf47"),
    0x3CB8: (2, "3db07e485d45f7517b4010dec7d0dcb8f346ac6796685a2da74d7ee2bd67e6c4"),
    0x3CBA: (2, "587e5ed0ff7c93c43c214b6cd2b6c4d93dd6b151748286cfd2f164494eb7c611"),
    0x3CBE: (3, "e72aa9883b867c0a0ec62ef6e8dbc3f45e26af1ee7123732593b3ec2307cb644"),
    0x3CF9: (6, "9f6d2d31ac00e2c6788b19c8993e219ea81a6262b1eec26b63a0b67133f5f831"),
    0x189C1: (3, "b3f935724f6fd7fc33ad267a04c849747e6f9a4466bbaa48ae8b050c491fe54d"),
    0x189D1: (3, "5cd860557e7120bcd808c940152c3cea47966b65729028ccb428f32ab62645d3"),
    0x189D6: (5, "69b51e6cf1c5636bf3ab97fe64e77d03ec7518860c577451fac10de347a9e2a3"),
}

MODE_TARGETS = {
    0: (0x3050, 0xB0),
    1: (0x3100, 0x320),
    2: (0x17B90, 0x60),
    3: (0x17BF0, 0xC0),
    4: (0x17CB0, 0x2A0),
    5: (0x181E0, 0x2DC),
    6: (0x3D10, 0x90),
    7: (0x186E0, 0x2B3),
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def find_original_paths(title_arg: Path | None, exe_arg: Path | None) -> tuple[Path, Path]:
    if (title_arg is None) != (exe_arg is None):
        raise SystemExit("--title and --exe must be supplied together")
    if title_arg is not None and exe_arg is not None:
        title_path = Path(os.path.abspath(os.path.expanduser(str(title_arg))))
        exe_path = Path(os.path.abspath(os.path.expanduser(str(exe_arg))))
        if not title_path.is_file() or not exe_path.is_file():
            raise FileNotFoundError("explicit --title and --exe must both name files")
        return title_path, exe_path
    for candidate in (REPO, *REPO.parents):
        title_path = candidate / "assets" / "pc" / "HERCULES" / "TITLE.DLL"
        exe_path = candidate / "work" / "discs" / "pc_install" / "HERCULES.EXE"
        if title_path.is_file() and exe_path.is_file():
            return (Path(os.path.abspath(str(title_path))),
                    Path(os.path.abspath(str(exe_path))))
    raise FileNotFoundError("could not locate pinned assets/pc and work/discs originals above checkout")


def disassemble(pe: PE, rva: int, size: int, base: int) -> dict[int, object]:
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    body = pe.read_rva(rva, size)
    instructions = list(decoder.disasm(body, base + rva))
    cursor = base + rva
    for insn in instructions:
        if insn.address != cursor:
            raise ValueError("incomplete instruction decode at RVA " + hex(cursor - base))
        cursor += insn.size
    if cursor != base + rva + size:
        raise ValueError("incomplete instruction decode at span end " + hex(cursor - base))
    return {insn.address - base: insn for insn in instructions}


def direct_calls(instructions: dict[int, object], base: int) -> list[dict]:
    calls = []
    for rva, insn in sorted(instructions.items()):
        if insn.mnemonic != "call":
            continue
        insn_bytes = bytes(insn.bytes)
        row = {"rva": hex(rva), "size": insn.size,
               "sha256": sha256(insn_bytes),
               "instruction": (insn.mnemonic + " " + insn.op_str).rstrip()}
        if len(insn.operands) == 1 and insn.operands[0].type == capstone.x86.X86_OP_IMM:
            target = insn.operands[0].imm - base
            row["target_rva"] = hex(target)
        else:
            row["indirect"] = True
        calls.append(row)
    return calls


def original_span(pe: PE, module: str, rva: int, size: int) -> dict:
    body = pe.read_rva(rva, size)
    return {"module": module, "rva": hex(rva), "size": size, "sha256": sha256(body)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path,
                        help="JSON output path; must be inside this checkout's work/ directory")
    parser.add_argument("--title", type=Path,
                        help="optional explicit path to the pinned original TITLE.DLL")
    parser.add_argument("--exe", type=Path,
                        help="optional explicit path to the pinned original HERCULES.EXE")
    args = parser.parse_args()
    output = Path(os.path.abspath(os.path.expanduser(str(args.output))))
    work_root = Path(os.path.abspath(str(REPO / "work")))
    try:
        output.relative_to(work_root)
    except ValueError:
        raise SystemExit("refusing output outside checkout/work: " + str(output))
    if output == work_root:
        raise SystemExit("--output must name a file below checkout/work")

    title_path, host_path = find_original_paths(args.title, args.exe)
    title = PE(title_path)
    host = PE(host_path)
    if title.image_base != TITLE_BASE:
        raise ValueError("TITLE.DLL image base changed")
    if host.image_base != HERCULES_BASE:
        raise ValueError("HERCULES.EXE image base changed")
    if sha256(title.data) != TITLE_SHA256:
        raise ValueError("pinned TITLE.DLL file hash mismatch")
    if sha256(host.data) != HERCULES_SHA256:
        raise ValueError("pinned HERCULES.EXE file hash mismatch")
    if capstone.__version__ != "5.0.7":
        raise ValueError("expected Capstone 5.0.7, got " + capstone.__version__)

    title_insns: dict[str, dict[int, object]] = {}
    spans = []
    for name, (rva, size) in TITLE_SPANS.items():
        span_row = {"name": name, **original_span(title, "TITLE.DLL", rva, size)}
        if name == "mode_table":
            span_row["content_kind"] = "data"
        else:
            instructions = disassemble(title, rva, size, TITLE_BASE)
            title_insns[name] = instructions
            span_row["content_kind"] = "code"
            span_row["decode"] = "complete"
            span_row["instruction_count"] = len(instructions)
        spans.append(span_row)
    table_bytes = title.read_rva(*TITLE_SPANS["mode_table"])
    table_targets = [value - TITLE_BASE for value in struct.unpack("<8I", table_bytes)]
    expected_targets = [MODE_TARGETS[index][0] for index in range(8)]
    if table_targets != expected_targets:
        raise ValueError("TITLE mode table target map changed")

    table_span = original_span(title, "TITLE.DLL", 0x220D0, 0x20)
    if table_span["sha256"] != "415195e248ded9aefc3bba1701c680ed3f995fb163b29400793362439889d23c":
        raise ValueError("TITLE mode table span hash mismatch")

    site_rows = []
    all_title_instructions: dict[int, object] = {}
    for name, instructions in title_insns.items():
        all_title_instructions.update(instructions)
    for rva, (site_size, expected_sha256) in sorted(SITES.items()):
        observed = title.read_rva(rva, site_size)
        observed_sha256 = sha256(observed)
        if observed_sha256 != expected_sha256:
            raise ValueError("original instruction changed at TITLE.DLL+" + hex(rva))
        insn = all_title_instructions.get(rva)
        if insn is None or insn.size != site_size:
            raise ValueError("pinned site is not one complete decoded instruction at TITLE.DLL+" + hex(rva))
        site_rows.append({
            "rva": hex(rva),
            "size": site_size,
            "sha256": observed_sha256,
            "instruction": (insn.mnemonic + " " + insn.op_str).rstrip() if insn else None,
        })

    mode_rows = []
    for index, target in enumerate(table_targets):
        mode_rows.append({"index": index, "entry_rva": hex(0x220D0 + index * 4),
                          "target_rva": hex(target),
                          "span_size": MODE_TARGETS[index][1]})

    calls = {name: direct_calls(insns, TITLE_BASE) for name, insns in title_insns.items()}
    call_scan_spans = []
    for name, (rva, _size) in TITLE_SPANS.items():
        if name == "mode_table":
            continue
        call_scan_spans.append(name)
    direct_28c0 = []
    for name in call_scan_spans:
        for call in calls[name]:
            if call.get("target_rva") == "0x28c0":
                direct_28c0.append({"span": name, **call})

    host_span = original_span(host, "HERCULES.EXE", 0xAC60, 0x690)
    host_instructions = disassemble(host, 0xAC60, 0x690, HERCULES_BASE)
    host_span["content_kind"] = "code"
    host_span["decode"] = "complete"
    host_span["instruction_count"] = len(host_instructions)
    if host_span["sha256"] != "f7724c04bef582e378b183a12550ba841df1a6fa3660be51da71154aa5c525af":
        raise ValueError("pinned AC60 host span hash mismatch")
    packets_path = REPO / "renderer_contract" / "evidence" / "packets.json"
    module_packets_path = REPO / "renderer_contract" / "evidence" / "module_packets.json"
    packets = json.loads(packets_path.read_text(encoding="utf-8-sig"))
    module_packets = json.loads(module_packets_path.read_text(encoding="utf-8-sig"))
    ac60 = packets["consumer"]["dispatch"]
    ft4 = next(route for route in ac60["routes"] if route["group"] == "0x20")
    if ac60["tag_byte_offset"] != 7 or ft4["base_codes"].get("0x2c") != "FT4-compatible field reads":
        raise ValueError("public AC60 FT4 consumer evidence changed")
    if module_packets["consumer_68"]["normalized_commands"].get("0x68") != "0x68":
        raise ValueError("public AC60 0x68 consumer evidence changed")

    evidence = {
        "schema_version": 1,
        "status": "STATIC_OBSERVED",
        "scope": "TITLE.DLL mode-table indices 2, 3, 4, and 6, through direct child-render dispatch or direct tag/link writes; bounded helper reachability for 0x28c0.",
        "prediction": "Indices 2 and 3 are object-list dispatchers that hand eligible children to 0x02dc0; index 4 emits inline 0x68 tile records and can dispatch child objects; index 6 emits FT4-layout records whose tag high byte is 0x2c or 0x2e. No new AC60 family is predicted.",
        "falsifier": "A byte-pinned handler or directly reached writer stores an AC60 tag outside the already represented 0x68 or FT4 0x2c/0x2e routes, or the observed table/call edges differ.",
        "tool": {"pe_reader": "scripts/pe.py", "capstone_version": capstone.__version__,
                 "originals_read_only": True, "original_execution": False},
        "originals": {
            "TITLE.DLL": {"path": str(title_path), "sha256": TITLE_SHA256,
                          "image_base": hex(TITLE_BASE)},
            "HERCULES.EXE": {"path": str(host_path), "sha256": HERCULES_SHA256,
                             "image_base": hex(HERCULES_BASE)},
        },
        "spans": spans + [host_span],
        "mode_table": {"span": table_span, "entries": mode_rows,
                       "targets_are_image_rvas": True},
        "handlers": [
            {
                "index": 2, "target_rva": "0x17b90", "classification": "dynamic child-render producer",
                "observed": [
                    "Calls 0x195f0 on the parent; on -1, sets object byte +0x1f bit 0x08 and returns.",
                    "On success, reads a DWORD count followed by DWORD child pointers at object +0x120.",
                    "For each listed child, calls 0x04410, then 0x195f0, then 0x02dc0.",
                    "0x02dc0 selects the child renderer from the table at 0x220d0 using child byte +0x23; the family is child-mode-dependent, not a fixed tag at this handler."
                ],
                "direct_calls": calls["mode_2_list"],
            },
            {
                "index": 3, "target_rva": "0x17bf0", "classification": "clip-state plus dynamic child-render producer",
                "observed": [
                    "Calls 0x195f0 on the parent; on -1, sets object byte +0x1f bit 0x08 and returns.",
                    "Builds a clip rectangle from parent WORD fields +0x70/+0x72/+0x74 and the returned projected value, then calls 0x0ca40, 0x0c7d0, and 0x0c790.",
                    "For each non-null child accepted by 0x19470, calls 0x04410 and 0x02dc0.",
                    "The child-mode dispatch is the packet-producing boundary; no unique AC60 tag is stored by this handler."
                ],
                "direct_calls": calls["mode_3_clip_list"],
            },
            {
                "index": 4, "target_rva": "0x17cb0", "classification": "AC60 0x68 tile producer plus dynamic child rendering",
                "observed": [
                    "Checks the slot/type path and projection; an invalid projection sets object byte +0x1f bit 0x08 and returns.",
                    "For a span whose flags WORD at +0x0e has bit 0x8000, writes a 12-byte pool record: XY DWORD at +0x08, color DWORD 0x00ffffff at +0x04, tag byte 0x68 at +0x07, previous bucket head at +0x00, then replaces the selected OT bucket head.",
                    "Advances global pool cursor 0x2df40 by 0x0c for each inline tile record.",
                    "For nonzero span IDs with a loaded entry, calls 0x02cc0 and 0x02dc0 on the shared child object; 0x02dc0 performs child-mode table dispatch."
                ],
                "direct_calls": calls["mode_4_group"],
            },
            {
                "index": 6, "target_rva": "0x03d10", "classification": "AC60 FT4 producer",
                "observed": [
                    "Copies parent WORD +0x3e to WORD +0x10.",
                    "Selects direct writers 0x03100/0x03420 when the dimension-derived OR equals 0x100; otherwise selects 0x03740/0x039b0, with WORD +0x74 choosing the paired path.",
                    "Each writer obtains a command/color DWORD from 0x189a0, stores it at primitive +0x04, links the previous OT head through primitive +0x00, replaces the OT head, advances by 0x28 bytes per record, and writes the final pool cursor to 0x2df40.",
                    "0x189a0 clamps WORD +0x4a to 0..255 and returns a color word whose high byte is 0x2c, or 0x2e when flags DWORD +0x54 has bit 0x04."
                ],
                "direct_calls": calls["mode_6_sprite_quad_dispatch"],
                "writer_sites": [hex(site) for site in (0x33BB, 0x36DD, 0x3943, 0x3CA6)],
            },
        ],
        "dynamic_child_dispatch": {
            "span": next(row for row in spans if row["name"] == "dynamic_child_renderer"),
            "mode_source": {"rva": "0x2fde", "size": SITES[0x2FDE][0],
                            "sha256": SITES[0x2FDE][1],
                            "instruction": "mov al, byte ptr [esi+0x23]"},
            "indirect_call": {"rva": "0x300d", "size": SITES[0x300D][0],
                              "sha256": SITES[0x300D][1],
                              "instruction": "call dword ptr [eax*4+0x100220d0]"},
            "direct_calls": calls["dynamic_child_renderer"],
        },
        "direct_write_sites": site_rows,
        "helper_0x28c0": {
            "direct_calls_in_scanned_mode_and_renderer_spans": direct_28c0,
            "scanned_spans": call_scan_spans,
            "direct_call_found": bool(direct_28c0),
            "disposition": "No 0x28c0 direct call appears in the mode-target or bounded renderer span set. Its callsite arguments 0x7c/0x64 are not used as packet tags here; this audit establishes no link/tag write from those values.",
        },
        "consumer_evidence": {
            "ac60_span": host_span,
            "packets_json_sha256": sha256(packets_path.read_bytes()),
            "module_packets_json_sha256": sha256(module_packets_path.read_bytes()),
            "tag_byte_offset": ac60["tag_byte_offset"],
            "ft4_0x2c": ft4["base_codes"]["0x2c"],
            "ft4_raw_bit1_variants": ft4["rasterizer_bit1_clear"] + "/" + ft4["rasterizer_bit1_set"],
            "command_0x68_normalized": module_packets["consumer_68"]["normalized_commands"]["0x68"],
            "command_0x68_disposition": module_packets["consumer_68"]["disposition"],
        },
        "limits": [
            "Static instruction/dataflow observations only; no TITLE gameplay or original execution.",
            "Indices 2, 3, and 4 may transitively emit packets selected by child mode/state; this audit does not assign a single tag to those list/group wrappers.",
            "No synthetic fixture was added because the observed terminal routes are already represented AC60 tags 0x68 and 0x2c/0x2e.",
            "The 0x28c0 result is limited to direct calls in the listed mode-target and bounded renderer spans; it does not claim whole-module absence or infer tags from scalar arguments."
        ],
    }

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": evidence["status"], "output": str(output),
                      "title_spans": len(spans), "checked_instruction_sites": len(site_rows),
                      "mode_targets": len(mode_rows)}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
