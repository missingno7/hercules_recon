#!/usr/bin/env python3
"""Read-only, bounded audit of TITLE mode-7 command construction and packet frontiers."""
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
    script = Path(__file__).absolute()
    for candidate in (script.parent, *script.parents):
        if ((candidate / "scripts" / "pe.py").is_file()
                and (candidate / "renderer_contract").is_dir()):
            return candidate
    raise FileNotFoundError("could not locate checkout with scripts/pe.py and renderer_contract")


REPO = find_repo_root()
sys.path.insert(0, str(REPO / "scripts"))
import env  # noqa: F401,E402
from pe import PE  # noqa: E402
import capstone  # noqa: E402


TITLE_SHA256 = "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b"
HERCULES_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
TITLE_BASE = 0x10000000
HERCULES_BASE = 0x00400000

# Exact original code/data spans, with each code span fully decoded. The table
# is data and is deliberately not passed to Capstone.
TITLE_SPANS = {
    "mode_table": (0x220D0, 0x20, "415195e248ded9aefc3bba1701c680ed3f995fb163b29400793362439889d23c", "data"),
    "mode_2_child_route": (0x17B90, 0x60, "a2c89c494a260cee9c55c1dec26a7f1bc43be297492f37430ebfe11d347708fc", "code"),
    "child_dispatcher": (0x2DC0, 0x290, "4b5325e70cf3bac4c0fbc06d0c6c04c9daf4179e70c542338d45ba381be30a08", "code"),
    "mode_7_writer": (0x186E0, 0x2B3, "7353881ec5fa393a22c46d53ca59d492b41d24b6c4073180b0e939f3e37c8383", "code"),
    "command_color_helper": (0x189A0, 0x3C, "fed5051e785d04f377193fd255c278671587fdeabe9ecc2a4dedb7ce41e76b59", "code"),
}
HOST_CONSUMER = (0xAC60, 0x690, "f7724c04bef582e378b183a12550ba841df1a6fa3660be51da71154aa5c525af")

# RVA -> (exact instruction size, SHA-256 of that instruction, expected decode).
# No original opcode byte strings are embedded or written to the receipt.
SITES = {
    0x17BDA: (5, "3349434b8f7654cd16db99c322d330ea1eb63b9f308dd31e7179e53f16c7f350", "call 0x10002dc0"),
    0x2FDE: (3, "934e3190389bcd9c5833c880518fa2b8b19a2deca93d267428c8e770ac4c5161", "mov al, byte ptr [esi + 0x23]"),
    0x2FE1: (2, "cefb6ab22ceb39e9ba061187c8ea79abd8c6dd3fb3031ce67c7cb6be1fe8b2e0", "test al, al"),
    0x2FE3: (2, "98a1d97320f14c574690067bb38dc63be3a43585486e2c332ec6794b6f3c2d88", "jne 0x10002ff7"),
    0x2FFF: (6, "3c827da6ee515386360632b6e997c5b5dbd474fdd92fc1b3fad406971ad5b570", "and ecx, 0xff"),
    0x3005: (5, "64dd0a859562e8fdb11f517b18abe886df82ba88f7a159e7a4bfaeaff2d05657", "and eax, 0xff"),
    0x300D: (7, "07ffe97b20c6841efa7efc790f71b3efb094a39c3334b4b9ba6e9fa0278dd013", "call dword ptr [eax*4 + 0x100220d0]"),
    0x186EC: (5, "f04a284f0bbb4c9ffd85c5cd02d84da559e26053d3382a442507cde2689f0ed6", "call 0x100189a0"),
    0x186FB: (4, "71fef67482285f78f815cd5b8a942fe6d8c29d468e22e98ff2dfe03e11eb174a", "mov dword ptr [esp + 0x38], eax"),
    0x18930: (4, "dbfbe1563bf47391b96e9eb3f809d98033b8fdaea3d32fd7afdb6db16e149953", "mov edx, dword ptr [esp + 0x38]"),
    0x18934: (3, "fc8e692c1cd4e041941c2c707759b59330f639f98842dffa2390180cf012cf47", "mov dword ptr [ecx + 4], edx"),
    0x18940: (4, "e6f9bfc39a3d5a1e691c4b90f111d14ed66bece56c46a2353d8c19f0214b7930", "mov edx, dword ptr [esp + 0x40]"),
    0x18944: (2, "245776e7794d6c1342d6b78eb8a4bdafc16c9d43fdf250b70ba8a2c110a02f65", "mov esi, dword ptr [edx]"),
    0x18946: (2, "3db07e485d45f7517b4010dec7d0dcb8f346ac6796685a2da74d7ee2bd67e6c4", "mov dword ptr [ecx], esi"),
    0x18948: (2, "587e5ed0ff7c93c43c214b6cd2b6c4d93dd6b151748286cfd2f164494eb7c611", "mov dword ptr [edx], ecx"),
    0x1894D: (3, "e72aa9883b867c0a0ec62ef6e8dbc3f45e26af1ee7123732593b3ec2307cb644", "add ecx, 0x28"),
    0x18988: (6, "9f6d2d31ac00e2c6788b19c8993e219ea81a6262b1eec26b63a0b67133f5f831", "mov dword ptr [0x1002df40], ecx"),
    0x189A6: (4, "fd034bd754ca5f062aa28014bb868b9bfbbc524e3dc32c7fcbe04451f8f77b0d", "mov cx, word ptr [edx + 0x4a]"),
    0x189C1: (3, "b3f935724f6fd7fc33ad267a04c849747e6f9a4466bbaa48ae8b050c491fe54d", "or ah, 0x2c"),
    0x189C4: (3, "1c7393d4db91e9ba925b0b4ee2ff01e73480f47dc6ba1a162defbd15ea7630eb", "shl eax, 8"),
    0x189C7: (2, "5e5c0068d905404b460383a94fd59017c4628df6432916ffcf3be3ada4be7f5d", "or eax, ecx"),
    0x189C9: (3, "1c7393d4db91e9ba925b0b4ee2ff01e73480f47dc6ba1a162defbd15ea7630eb", "shl eax, 8"),
    0x189CC: (2, "5e5c0068d905404b460383a94fd59017c4628df6432916ffcf3be3ada4be7f5d", "or eax, ecx"),
    0x189CE: (3, "f3fd9122b9767fe45c2d47fbd155efe5bb2d7f1fd45869458a484722c8b545d3", "mov cl, byte ptr [edx + 0x54]"),
    0x189D1: (3, "5cd860557e7120bcd808c940152c3cea47966b65729028ccb428f32ab62645d3", "test cl, 4"),
    0x189D6: (5, "69b51e6cf1c5636bf3ab97fe64e77d03ec7518860c577451fac10de347a9e2a3", "or eax, 0x2000000"),
}

PUBLIC_FILES = (
    "renderer_contract/evidence/packets.json",
    "renderer_contract/evidence/module_packets.json",
    "renderer_contract/evidence/eng3_handlers.json",
    "renderer_contract/evidence/title_handlers.json",
)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def find_originals(title_arg: Path | None, exe_arg: Path | None) -> tuple[Path, Path]:
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
    raise FileNotFoundError("could not find TITLE.DLL and HERCULES.EXE in checkout ancestors")


def disassemble(pe: PE, rva: int, size: int, base: int) -> dict[int, object]:
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    body = pe.read_rva(rva, size)
    instructions = list(decoder.disasm(body, base + rva))
    cursor = base + rva
    for insn in instructions:
        if insn.address != cursor:
            raise ValueError("incomplete code decode at RVA " + hex(cursor - base))
        cursor += insn.size
    if cursor != base + rva + size:
        raise ValueError("incomplete code decode at end RVA " + hex(cursor - base))
    return {insn.address - base: insn for insn in instructions}


def instruction_text(insn: object) -> str:
    return (insn.mnemonic + " " + insn.op_str).rstrip()


def load_public_evidence() -> tuple[dict, dict, dict, dict, dict]:
    paths = [REPO / relative for relative in PUBLIC_FILES]
    decoded = [json.loads(path.read_text(encoding="utf-8-sig")) for path in paths]
    return tuple(decoded) + ({path.as_posix().replace(REPO.as_posix() + "/", ""): sha256(path.read_bytes())
                              for path in paths},)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path,
                        help="JSON output path; must be below this checkout's work/ directory")
    parser.add_argument("--title", type=Path, help="optional explicit original TITLE.DLL path")
    parser.add_argument("--exe", type=Path, help="optional explicit original HERCULES.EXE path")
    args = parser.parse_args()

    output = Path(os.path.abspath(os.path.expanduser(str(args.output))))
    work_root = Path(os.path.abspath(str(REPO / "work")))
    try:
        output.relative_to(work_root)
    except ValueError:
        raise SystemExit("refusing output outside checkout/work: " + str(output))
    if output == work_root:
        raise SystemExit("--output must name a file below checkout/work")

    title_path, exe_path = find_originals(args.title, args.exe)
    title = PE(title_path)
    host = PE(exe_path)
    if title.image_base != TITLE_BASE or host.image_base != HERCULES_BASE:
        raise ValueError("original PE image base mismatch")
    if sha256(title.data) != TITLE_SHA256:
        raise ValueError("pinned TITLE.DLL hash mismatch")
    if sha256(host.data) != HERCULES_SHA256:
        raise ValueError("pinned HERCULES.EXE hash mismatch")
    if capstone.__version__ != "5.0.7":
        raise ValueError("expected Capstone 5.0.7, got " + capstone.__version__)

    title_code: dict[str, dict[int, object]] = {}
    span_rows = []
    for name, (rva, size, expected_hash, kind) in TITLE_SPANS.items():
        observed_hash = sha256(title.read_rva(rva, size))
        if observed_hash != expected_hash:
            raise ValueError("TITLE.DLL span hash mismatch: " + name)
        row = {"name": name, "module": "TITLE.DLL", "rva": hex(rva),
               "size": size, "sha256": observed_hash, "content_kind": kind}
        if kind == "code":
            decoded = disassemble(title, rva, size, TITLE_BASE)
            title_code[name] = decoded
            row["decode"] = "complete"
            row["instruction_count"] = len(decoded)
        span_rows.append(row)

    host_rva, host_size, host_hash = HOST_CONSUMER
    if sha256(host.read_rva(host_rva, host_size)) != host_hash:
        raise ValueError("pinned AC60 consumer span hash mismatch")
    host_instructions = disassemble(host, host_rva, host_size, HERCULES_BASE)

    mode_table = title.read_rva(*TITLE_SPANS["mode_table"][:2])
    targets = [word - TITLE_BASE for word in struct.unpack("<8I", mode_table)]
    if targets[2] != 0x17B90 or targets[7] != 0x186E0:
        raise ValueError("TITLE mode-table indices 2 or 7 changed")

    all_code: dict[int, object] = {}
    for code in title_code.values():
        all_code.update(code)
    site_rows = []
    for rva, (size, expected_hash, expected_text) in sorted(SITES.items()):
        insn = all_code.get(rva)
        if insn is None or insn.size != size:
            raise ValueError("site is not a complete decoded instruction at " + hex(rva))
        observed_hash = sha256(title.read_rva(rva, size))
        if observed_hash != expected_hash or instruction_text(insn) != expected_text:
            raise ValueError("pinned instruction mismatch at TITLE.DLL+" + hex(rva))
        site_rows.append({"module": "TITLE.DLL", "rva": hex(rva), "size": size, "sha256": observed_hash,
                          "instruction": expected_text})

    mode2_call = all_code[0x17BDA]
    if len(mode2_call.operands) != 1 or mode2_call.operands[0].imm - TITLE_BASE != 0x2DC0:
        raise ValueError("mode-2 child route no longer reaches 0x2dc0")
    if all_code[0x2FDE].mnemonic != "mov" or all_code[0x2FDE].op_str != "al, byte ptr [esi + 0x23]":
        raise ValueError("child mode source changed")
    if all_code[0x300D].mnemonic != "call" or "eax*4 + 0x100220d0" not in all_code[0x300D].op_str:
        raise ValueError("child table dispatch changed")

    packets, module_packets, eng3_handlers, title_handlers, source_hashes = load_public_evidence()
    dispatch = packets["consumer"]["dispatch"]
    if dispatch["tag_byte_offset"] != 7:
        raise ValueError("AC60 tag offset changed")
    polygon = next(row for row in dispatch["routes"] if row["group"] == "0x20")
    sprites = next(row for row in dispatch["routes"] if row["group"] == "0x60")
    if polygon["base_codes"].get("0x24") is None or sprites["base_codes"].get("0x74") is None or sprites["base_codes"].get("0x7c") is None:
        raise ValueError("consumer-only packet rows changed")
    if module_packets["title"]["mode_table"]["target_rvas_by_index"][7] != "0x186e0":
        raise ValueError("public TITLE mode table evidence changed")
    if module_packets["eng3"]["handler_table"]["target_rvas_by_index"] != ["0x9030", "0x90e0", "0x22de0", "0x22e40", "0x22f00", "0x23440"]:
        raise ValueError("public ENG3 handler map changed")
    eng1_mesh = packets["producer_evidence"]["eng1_mesh"]
    eng3_sprite = packets["producer_evidence"]["eng3_sprite64"]
    if eng1_mesh["native_span"]["rva"] != "0x2df20" or eng3_sprite["native_span"]["rva"] != "0x72a0":
        raise ValueError("public ENG1/ENG3 producer evidence changed")
    if not isinstance(eng3_handlers, dict) or title_handlers["mode_table"]["entries"][7]["target_rva"] != "0x186e0":
        raise ValueError("public handler evidence is incomplete")

    evidence = {
        "schema_version": 1,
        "status": "STATIC_OBSERVED",
        "scope": "Bounded TITLE child-mode dispatch and mode-7 writer command construction; cross-module producer frontier reused only from cited public receipts.",
        "prediction": "TITLE mode 7's computed command DWORD resolves to the existing FT4 family. The currently audited ENG1/ENG3/TITLE producer paths will not establish exact FT3 or fixed-sprite tag/link/stride writes.",
        "falsifier": "A pinned original producer reachable through an audited dispatch path stores one of AC60 tags 0x24/0x26, 0x74/0x76, or 0x7c/0x7e and links that record into an order table with an observed cursor advance or equivalent bounded extent.",
        "proof_boundary": "Static observations only. Conditional dispatch is not evidence of a runtime object instance; no whole-module producer absence, gameplay, pixel behavior, or allocation-capacity claim is made.",
        "tool": {"pe_reader": "scripts/pe.py", "capstone_version": capstone.__version__,
                 "originals_read_only": True, "original_execution": False,
                 "complete_decode_required_for_code_spans": True},
        "originals": {
            "TITLE.DLL": {"path": str(title_path), "sha256": TITLE_SHA256, "image_base": hex(TITLE_BASE)},
            "HERCULES.EXE": {"path": str(exe_path), "sha256": HERCULES_SHA256, "image_base": hex(HERCULES_BASE)},
        },
        "original_spans": span_rows + [{"name": "AC60_consumer", "module": "HERCULES.EXE",
                                        "rva": hex(host_rva), "size": host_size, "sha256": host_hash,
                                        "content_kind": "code", "decode": "complete",
                                        "instruction_count": len(host_instructions)}],
        "title_mode_7_reachability": {
            "conditional_path": [
                {"site_rva": "0x17bda", "instruction": "call 0x10002dc0",
                 "meaning": "TITLE mode-2 list handler dispatches each eligible child."},
                {"site_rva": "0x2fde", "instruction": "mov al, byte ptr [esi + 0x23]",
                 "meaning": "Child mode comes from byte +0x23."},
                {"site_rva": "0x3005", "instruction": "and eax, 0xff",
                 "meaning": "The mode byte is zero-extended as the table index."},
                {"site_rva": "0x300d", "instruction": "call dword ptr [eax*4 + 0x100220d0]",
                 "meaning": "Mode 7 selects TITLE.DLL RVA 0x186e0 from the pinned table."},
            ],
            "mode_table_targets": [hex(target) for target in targets],
            "runtime_caveat": "The path is conditional on an eligible child whose mode byte is 7; no gameplay instance was observed.",
        },
        "title_mode_7_command": {
            "writer_span": {"rva": "0x186e0", "size": 0x2B3, "sha256": TITLE_SPANS["mode_7_writer"][2]},
            "helper_span": {"rva": "0x189a0", "size": 0x3C, "sha256": TITLE_SPANS["command_color_helper"][2]},
            "derivation": [
                "0x186e0 calls 0x189a0 and keeps its returned DWORD in a local stack slot.",
                "0x189a0 zero-extends WORD object+0x4a, clamps values above 255 to 255, and replicates the resulting byte into RGB.",
                "The helper ORs 0x2c into AH before two byte shifts; the resulting DWORD's byte +3 is 0x2c.",
                "If byte object+0x54 has bit 0x04, the helper ORs 0x02000000, making byte +3 0x2e. These are the only two command-byte outcomes in the helper.",
                "The writer stores the returned DWORD at record+0x04, writes the previous OT head at record+0x00, replaces that head, advances by 0x28 per record, and publishes cursor 0x2df40.",
                "AC60 reads command byte +0x07; existing packet evidence classifies base 0x2c as FT4-compatible, including the 0x2e bit-1 variant.",
            ],
            "tag_domain": ["0x2c", "0x2e"],
            "producer_stride": "0x28 bytes; not a backing allocation capacity claim",
            "not_emitted_by_this_path": ["0x24", "0x74", "0x7c"],
        },
        "unresolved_consumer_only_frontier": [
            {"base_tag": "0x24", "raw_tags": ["0x24", "0x26"], "family": "FT3-compatible",
             "consumer_minimum": polygon["access_minimums"]["0x24"],
             "missing_witness": "No exact original first-party command store plus order-link/cursor or bounded record extent is established by the cited producer paths."},
            {"base_tag": "0x74", "raw_tags": ["0x74", "0x76"], "family": "fixed 8x8 sprite",
             "consumer_minimum": sprites["access_minimums"]["0x74"],
             "missing_witness": "No exact original first-party fixed-sprite command store plus order-link/cursor or bounded record extent is established by the cited producer paths."},
            {"base_tag": "0x7c", "raw_tags": ["0x7c", "0x7e"], "family": "fixed 16x16 sprite",
             "consumer_minimum": sprites["access_minimums"]["0x7c"],
             "missing_witness": "No exact original first-party fixed-sprite command store plus order-link/cursor or bounded record extent is established by the cited producer paths."},
        ],
        "bounded_cross_module_frontier": {
            "ENG1": "Existing packet evidence pins native mesh producer 0x2df20 to the GT4 helper and 0x34 stride; the existing tile/draw routes establish 0x68/0x64/FT4 behavior, not FT3 or fixed sprites.",
            "ENG3": "Existing handler evidence pins table entries 0-5: FT4 writers, delegated object rendering, tag-0x68 writer, and FT4 writer; the separately audited native sprite writer 0x72a0 emits variable sprite tag 0x64 at stride 0x14.",
            "TITLE": "The mode-7 computed writer emits FT4 0x2c/0x2e. Existing title mode and handler receipts identify known FT4, 0x68, and delegated-child routes; no FT3/fixed-sprite record writer is established by those audited routes.",
            "scope_limit": "These are bounded audited routes and public receipts, not an exhaustive scan of every DLL function or resource-driven path.",
        },
        "reused_public_evidence": {
            "files": source_hashes,
            "consumer_families": {
                "FT3": polygon["base_codes"]["0x24"],
                "fixed_8x8": sprites["base_codes"]["0x74"],
                "fixed_16x16": sprites["base_codes"]["0x7c"],
            },
            "producer_refs": [
                "renderer_contract/evidence/packets.json#producer_evidence.eng1_mesh",
                "renderer_contract/evidence/packets.json#producer_evidence.eng3_sprite64",
                "renderer_contract/evidence/module_packets.json#title",
                "renderer_contract/evidence/eng3_handlers.json",
                "renderer_contract/evidence/title_handlers.json#handlers",
            ],
        },
        "checked_instruction_sites": site_rows,
        "limits": [
            "AC60 access minima 0x1e and 0x0e do not establish producer stride, allocation size, or runtime producer reachability.",
            "No first-party FT3 or fixed-sprite producer is claimed absent from the whole executable or DLLs.",
            "TITLE mode-7 output is statically FT4-shaped; existing bounded producer fixture coverage is reused and does not prove pixels or gameplay.",
            "No arguments to helper 0x28c0 are interpreted as packet tags without a record write/link path.",
        ],
    }

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": evidence["status"], "title_mode7_tag_domain": evidence["title_mode_7_command"]["tag_domain"],
                      "bounded_spans": len(span_rows), "checked_sites": len(site_rows), "output": str(output)}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
