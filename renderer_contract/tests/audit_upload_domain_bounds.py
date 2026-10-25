#!/usr/bin/env python3
"""Verify a bounded, read-only upload-producer domain receipt."""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path


PINNED_IMAGES = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "ENG3.DLL": "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
    "TITLE.DLL": "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
}
PINNED_BASES = {
    "HERCULES.EXE": 0x00400000,
    "ENG1.DLL": 0x10000000,
    "ENG3.DLL": 0x10000000,
    "TITLE.DLL": 0x10000000,
}
SPANS = {
    "HERCULES.EXE": {
        "interface_source_prefix": (0x361B8, 8, "f46f52d6fa0bd2eed01689542e64c1718f5dd9e71f56d72a9e951d30fe15981e"),
        "callback_slot_19_entry": (0x36204, 4, "5a09ff956c26e1b19855ea2335e512c4ba34337bf0478f20bc998e0a7033a14a"),
        "title_interface_pointer_cell": (0x365A0, 4, "808fbb8b89f159778e8ec1322e9c7a53c570dfe17e6e565367f97a9bee2a0e3b"),
        "title_entry_call_sequence": (0x5EF3, 0x15, "8a42ce650f2075b54528f031bab80b2a0c2111c1e57ad3405f83b465eb7ea387"),
        "context_writer": (0x1BD0, 0x123, "f2af2f9497e022748805220aac39872fdc31492b3fb2e5f582237c81f51a095d"),
    },
    "ENG1.DLL": {
        "setter_call_35de": (0x35D8, 0x0B, "7bc60fd6c0c5b99c30384977b980fa69a911a52999308cfe1f49bf44b9135d40"),
        "setter_call_e7e3": (0xE7BE, 0x2A, "141d9af16f37335fd432dada50d956f4128062699baeb207fab98ef4e21a4f7c"),
        "setter_call_ec4b": (0xEC3F, 0x11, "81f1d584d607e454a56fde9c73908b379a43eab408b5cae9df12b10892d4d8c2"),
    },
    "ENG3.DLL": {
        "constructor_state_seed": (0x78F0, 0x120, "ac95e14392dff97a4e713865ad3a2d25badac32e6cfdbe1e283f556d9d95df37"),
        "split_upload_consumer": (0x7C5B, 0x76, "d110326ba906a84a81c35ffe17b220865af67faa59e59dcb54f5e22a6b59babd"),
        "split_upload_direct_route": (0x8BFB, 0x1D, "3ff50db4d714ff259dcd345bab06aa45c6c6dacb591ffe1094ccf3a9f6d09276"),
        "event_dispatch_index": (0x1DBDE, 0x2D, "0bccaa11ac8f5ed7e831807e728f4124752b15bf9668b21546f4f6af78cc4a0e"),
        "signed_word_state_store": (0x1DE32, 7, "0fe158892e8ce11406b10fefcf02ba54565e395241cd3b57b4ba604b46a6a13f"),
        "event_jump_table_slot": (0x1DF74, 4, "178d83c544457f76f48b084baa8e3b760e9e9fb06fc209aa440d8eb692e07814"),
        "pointer_literal_setter_86ae": (0x86A6, 0x0D, "596d155fbdedfa590ece9af46dc6af6e931d47a3a230c078936121ab94673ba5"),
        "pointer_literal_setter_1a95d": (0x1A94D, 0x15, "e3c16b9b6bb7a225f61c9334c34f66318ce87e7c814ab33fc21289ac25c9bfb7"),
        "resource_rect_copy": (0x8072, 132, "ac57433c61a52059f5c014cb39c05355930e9c964df6675c69a1f744900c7ab4"),
    },
    "TITLE.DLL": {
        "interface_copy_entry": (0xC1F0, 0x28, "f92bd5e5ecce891c097a854e5c60e761dc32a007546c0ced3a2b7e1f83867a2d"),
        "interface_copy_1000_bytes": (0xC1FB, 0x16, "47c7e2ef476b7cab0e3fb816270b1bf73e7df67138f82d327848256320b3684d"),
        "two_argument_setter_wrapper": (0xC370, 0x14, "fe2eef0b402eb5976ce65f9ad73d9425c4feca53c9643509aa179516f7db3ac8"),
        "zero_y_setter_call": (0x1640B, 0x1C, "9c32656a24b1e143c4adaa21ed84a9799b903828c432d8d375d2a1830c7b7e24"),
        "y_256_setter_then_context_read_1": (0x16FEC, 0x6B, "bff62b2e44559d1b6b940e72b18830ce22549bcd5d0da3cc007ec01e5eb17f6a"),
        "y_256_setter_then_context_read_2": (0x17398, 0x6D, "866b68017a009fb45e44332d94a617fe8d797e146172e86ab8fec889e12962d2"),
    },
}
SOURCE_HASHES = {
    "src/title/behavioral/title_entry.c": "cc21299115b5914079353dd5950c6e7288e9d59e83915e7ee6af28abcbb049a7",
    "src/title/behavioral/input_boot.c": "2ae0bdaa63c0cc1599f68909aec8242b2eebed566958c65788e93c180a19a04c",
    "src/eng1/frame_commands.c": "5088e6070b68a441ccdd3df79c9b061b3005c8fc25b40082d0ddeecff9692381",
    "renderer_contract/evidence/frame.json": "b81f7e1f86bb8b3976ed666261d741000b2e7e400d35d18be6ce0068ce69bb5b",
    "renderer_contract/evidence/upload_producer_sites.json": "4c5d60924b94244ea932a0e188226f573299dd5946049cca5011997ce8ded6a9",
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def find_checkout(start: Path) -> Path:
    for parent in (start, *start.parents):
        if (parent / "scripts" / "pe.py").is_file():
            return parent.resolve()
    raise SystemExit("could not locate checkout containing scripts/pe.py")


def inside(child: Path, parent: Path) -> bool:
    try:
        child.relative_to(parent)
        return True
    except ValueError:
        return False


def decode_text(pe, capstone):
    text = next((s for s in pe.sections if s["name"] == ".text"), None)
    if text is None:
        raise ValueError("missing .text section")
    raw = pe.read_rva(text["virtual_address"], text["raw_size"])
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    decoder.skipdata = True
    instructions = list(decoder.disasm(raw, pe.image_base + text["virtual_address"]))
    walked = sum(item.size for item in instructions)
    if walked != len(raw):
        raise ValueError("linear sweep did not cover the file-backed .text bytes")
    return text, instructions, walked


def direct_call_sites(pe, instructions, rva):
    from capstone.x86 import X86_OP_IMM

    target = pe.image_base + rva
    return sorted(
        item.address - pe.image_base
        for item in instructions
        if item.id
        and item.mnemonic == "call"
        and item.operands
        and item.operands[0].type == X86_OP_IMM
        and item.operands[0].imm == target
    )


def disp_sites(instructions, base_name, displacement):
    from capstone.x86 import X86_OP_MEM

    out = []
    for item in instructions:
        if not item.id:
            continue
        for operand in item.operands:
            if operand.type == X86_OP_MEM and operand.mem.base:
                if item.reg_name(operand.mem.base) == base_name and operand.mem.disp == displacement:
                    out.append(item.address)
    return sorted(out)


def absolute_write_sites(instructions, va, capstone):
    from capstone.x86 import X86_OP_MEM

    out = []
    for item in instructions:
        if not item.id:
            continue
        for operand in item.operands:
            if (
                operand.type == X86_OP_MEM
                and operand.mem.base == 0
                and operand.mem.index == 0
                and operand.mem.disp == va
                and operand.access & capstone.CS_AC_WRITE
            ):
                out.append(item.address)
    return sorted(out)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-root", type=Path, help="checkout containing scripts/pe.py")
    parser.add_argument("--exe", type=Path, required=True, help="read-only pinned HERCULES.EXE")
    parser.add_argument("--image-dir", type=Path, required=True, help="read-only directory containing ENG1/ENG3/TITLE DLLs")
    parser.add_argument("--output", type=Path, required=True, help="JSON receipt path under checkout/work")
    args = parser.parse_args()

    root = (args.original_root or find_checkout(Path(__file__).resolve())).resolve()
    if not (root / "scripts" / "pe.py").is_file() or not (root / "renderer_contract").is_dir():
        raise SystemExit("--original-root must name this checkout")
    output = args.output.resolve()
    if not inside(output, (root / "work").resolve()):
        raise SystemExit("--output must resolve inside checkout/work")
    if output.exists() and output.is_dir():
        raise SystemExit("--output must name a file")

    sys.path.insert(0, str(root / "scripts"))
    import env  # noqa: F401
    from pe import PE
    import capstone

    paths = {
        "HERCULES.EXE": args.exe.resolve(),
        "ENG1.DLL": (args.image_dir / "ENG1.DLL").resolve(),
        "ENG3.DLL": (args.image_dir / "ENG3.DLL").resolve(),
        "TITLE.DLL": (args.image_dir / "TITLE.DLL").resolve(),
    }
    if paths["HERCULES.EXE"].name.upper() != "HERCULES.EXE":
        raise SystemExit("--exe must identify HERCULES.EXE")

    images = {}
    image_receipt = {}
    for name, path in paths.items():
        pe = PE(path)
        digest = sha256(pe.data)
        if digest != PINNED_IMAGES[name]:
            raise SystemExit(f"{name} SHA-256 does not match the pinned input")
        if pe.image_base != PINNED_BASES[name]:
            raise SystemExit(f"{name} image base does not match the pinned input")
        images[name] = pe
        image_receipt[name] = {"sha256": digest, "image_base": f"0x{pe.image_base:08x}"}

    span_receipt = {}
    for name, spans in SPANS.items():
        pe = images[name]
        span_receipt[name] = {}
        for label, (rva, size, expected) in spans.items():
            actual = sha256(pe.read_rva(rva, size))
            if expected != "PENDING" and actual != expected:
                raise SystemExit(f"{name} {label} span hash mismatch")
            span_receipt[name][label] = {
                "rva": f"0x{rva:x}",
                "size": size,
                "sha256": actual,
                "proof": "PINNED_FILE_BACKED_SPAN",
            }

    source_receipt = {}
    for rel, expected in SOURCE_HASHES.items():
        path = root / rel
        if not path.is_file() or sha256(path.read_bytes()) != expected:
            raise SystemExit(f"source/evidence identity mismatch: {rel}")
        source_receipt[rel] = {"sha256": expected}

    sweeps = {}
    decodes = {}
    for name in ("ENG3.DLL", "TITLE.DLL", "HERCULES.EXE"):
        text, instructions, walked = decode_text(images[name], capstone)
        decodes[name] = instructions
        raw_bytes = text["raw_size"]
        sweeps[name] = {
            "section": ".text",
            "file_backed_bytes": raw_bytes,
            "linear_stream_bytes": walked,
            "all_file_backed_bytes_walked": walked == raw_bytes,
            "capstone_version": capstone.__version__,
            "limitations": "Covers the decoded linear stream only; overlapping instruction streams, computed calls and non-file-backed code are not covered.",
        }

    herc = images["HERCULES.EXE"]
    callback_target = int.from_bytes(herc.read_rva(0x361B8 + 0x4C, 4), "little")
    if callback_target != 0x00401BD0:
        raise SystemExit("host callback table slot 19 target mismatch")
    interface_pointer = int.from_bytes(herc.read_rva(0x365A0, 4), "little")
    if interface_pointer != 0x004361B8:
        raise SystemExit("TITLE interface source pointer cell changed")
    interface_prefix = herc.read_rva(0x361B8, 8)
    if int.from_bytes(interface_prefix[0:4], "little") != 0x5AC00CAC:
        raise SystemExit("TITLE interface source signature changed")
    shared_context_pointer = int.from_bytes(interface_prefix[4:8], "little")
    if shared_context_pointer != 0x00615C98:
        raise SystemExit("TITLE/HERC interface context pointer changed")

    eng3_calls = direct_call_sites(images["ENG3.DLL"], decodes["ENG3.DLL"], 0x7B90)
    if eng3_calls != [0x8C07]:
        raise SystemExit("ENG3 split-uploader direct-call set changed")
    title_setters = direct_call_sites(images["TITLE.DLL"], decodes["TITLE.DLL"], 0xC370)
    if title_setters != [0x1641F, 0x16FFF, 0x173A7]:
        raise SystemExit("TITLE setter direct-call set changed")
    title_reads = disp_sites(decodes["TITLE.DLL"], "ecx", 0x138)
    if title_reads != [0x10017032, 0x100173E0]:
        raise SystemExit("TITLE context+0x138 read set changed")
    herc_writes = disp_sites(decodes["HERCULES.EXE"], "ecx", 0x138)
    if herc_writes != [0x401C0E]:
        raise SystemExit("HERC context+0x138 direct write set changed")
    context_pointer_direct_writes = absolute_write_sites(decodes["HERCULES.EXE"], 0x004361BC, capstone)
    if context_pointer_direct_writes:
        raise SystemExit("HERC interface word1 has a direct absolute write in the file-backed linear sweep")

    # Verify the bounded producer facts at their named addresses without
    # embedding or publishing original instruction bytes.
    eng3_map = {item.address - images["ENG3.DLL"].image_base: item for item in capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32).disasm(images["ENG3.DLL"].read_rva(0x1DE32, 7), images["ENG3.DLL"].image_base + 0x1DE32)}
    if eng3_map.get(0x1DE32) is None or eng3_map[0x1DE32].mnemonic != "movsx" or eng3_map[0x1DE32].op_str != "edx, word ptr [eax + 2]":
        raise SystemExit("ENG3 signed-word command operand changed")
    if eng3_map.get(0x1DE36) is None or eng3_map[0x1DE36].op_str != "dword ptr [esi + 0x4c], edx":
        raise SystemExit("ENG3 object+0x4c write changed")
    event_target = int.from_bytes(images["ENG3.DLL"].read_rva(0x1DF74, 4), "little")
    if event_target != 0x1001DE32:
        raise SystemExit("ENG3 event command jump-table slot changed")

    title_pointers = {
        "setter_table_va": "0x1002bb8c",
        "context_pointer_cell_rva": "0x2bb44",
        "context_word_address_expression": "read_u32(TITLE image base + 0x2bb44) + 0x138",
        "direct_setter_call_rvas": [f"0x{x:x}" for x in title_setters],
        "context_word_read_rvas": [f"0x{x - images['TITLE.DLL'].image_base:x}" for x in title_reads],
        "read_sequences": [
            {"setter_call_rva": "0x16fff", "second_argument": "0x0100", "read_rva": "0x17032", "rect_words": {"x": "0x0390", "y": "context+0x138", "w": "0x0050", "h": "0x00c8"}},
            {"setter_call_rva": "0x173a7", "second_argument": "0x0100", "read_rva": "0x173e0", "rect_words": {"x": "0x0390", "y": "context+0x138", "w": "0x0050", "h": "0x00c8"}},
        ],
        "zero_setter": {"call_rva": "0x1641f", "second_argument": "0", "subsequent_context_138_read_in_local_sequence": False},
        "rect_units": "These are raw 16-bit producer fields; no pixel-unit or inclusive/exclusive dimension interpretation is inferred here.",
    }

    original_spans = [
        {"module": module, "rva": record["rva"], "size": record["size"], "sha256": record["sha256"], "label": label}
        for module, spans in span_receipt.items()
        for label, record in spans.items()
    ]
    report = {
        "schema_version": 1,
        "scope": "Bounded static upload-producer domain audit; no resources are extracted and no game/runtime execution occurs.",
        "inputs": image_receipt,
        "public_helpers": {
            "scripts/pe.py": {"sha256": sha256((root / "scripts" / "pe.py").read_bytes())},
            "scripts/env.py": {"sha256": sha256((root / "scripts" / "env.py").read_bytes())},
        },
        "audit_runner": {
            "path": "tests/audit_upload_domain_bounds.py",
            "sha256": sha256(Path(__file__).read_bytes()),
        },
        "source_and_existing_evidence": source_receipt,
        "original_spans": original_spans,
        "host_context_writer": {
            "table_rva": "0x361b8",
            "table_slot_offset": "0x4c",
            "table_slot_index": 19,
            "target_va": f"0x{callback_target:08x}",
            "target_rva": "0x1bd0",
            "written_context_offset": "0x138",
            "native_stack_mapping": "The host body pushes EBX and makes a returning helper call, then pushes two local pointer arguments. At the store, [ESP+0x20] resolves to host-entry ESP+0x14: the two-argument trampoline caller's second argument. The trampoline forwards its first two caller arguments to host slot+0x4c; this is not the caller's third/fourth/fifth argument.",
            "context_alias_and_lifetime": "TITLE receives a pointer to the static HERC interface table; the source table word1 is the exact cell reread by the HERC host writer. TITLE copies the same initial pointer to its own table word1. No direct absolute write to the source word1 was found in the HERC file-backed .text sweep; indirect writes remain outside scope.",
            "direct_write_rvas": [f"0x{x - herc.image_base:x}" for x in herc_writes],
            "other_module_pointer_literal_calls": [
                {"module": "ENG1.DLL", "wrapper_rva": "0x2c2f0", "call_rva": "0x35de", "second_argument": "0x1004d914"},
                {"module": "ENG1.DLL", "wrapper_rva": "0x2c2f0", "call_rva": "0xe7e3", "second_argument": "0x1005193c"},
                {"module": "ENG1.DLL", "wrapper_rva": "0x2c2f0", "call_rva": "0xec4b", "second_argument": "0x10051a98"},
                {"module": "ENG3.DLL", "wrapper_rva": "0x1f980", "call_rva": "0x86ae", "second_argument": "0x10035224"},
                {"module": "ENG3.DLL", "wrapper_rva": "0x1f980", "call_rva": "0x1a95d", "second_argument": "0x100381c8"},
            ],
            "interpretation_limit": "These second arguments are pointer literals in their own modules. If they reach this shared host slot and share the same context, the host stores their low WORD; they do not prove that TITLE later reads those values or that an upload occurs after module transition.",
        },
        "title_host_context_alias": {
            "herc_title_entry_call_rva": "0x5f03",
            "herc_interface_pointer_cell_va": "0x4365a0",
            "herc_interface_pointer_initial_value": f"0x{interface_pointer:08x}",
            "original_table_va": "0x4361b8",
            "original_word0_signature": "0x5ac00cac",
            "original_word1_va": "0x4361bc",
            "original_word1_initial_value": f"0x{shared_context_pointer:08x}",
            "host_context_acquisition_cell_va": "0x4361bc",
            "title_copy_destination_va": "0x1002bb40",
            "title_copy_bytes": 1000,
            "title_context_pointer_after_copy_va": "0x1002bb44",
            "same_initial_pointer_proven": True,
            "direct_absolute_write_sites_to_original_word1": [f"0x{x - herc.image_base:x}" for x in context_pointer_direct_writes],
            "interpretation_limit": "The source table is in writable .data. The file-backed HERC .text linear sweep finds no direct absolute write to its word1 cell, so the copied TITLE context and each HERC host acquisition use the same pinned initial pointer. Arbitrary indirect writes are outside this bounded scan.",
        },
        "title_context_138": title_pointers,
        "eng3_split_state": {
            "constructor_seed": 0,
            "event_operand": "For event command -0x20, a signed 16-bit word at event-record+2 is sign-extended into object+0x4c; the event dispatch index resolves to the pinned jump-table slot at RVA 0x1df74. No local range check is present in the bounded writer span.",
            "consumer_rectangles": [
                {"name": "first", "rect_words": {"x": 127, "y": "(uint16_t)(state + 0x200)", "w": 127, "h": "(uint16_t)(0x100 - state)"}},
                {"name": "second", "rect_words": {"x": 127, "y": "(uint16_t)state", "w": 127, "h": 256}},
            ],
            "rect_units": "These are raw 16-bit producer fields; no pixel-unit or inclusive/exclusive dimension interpretation is inferred here.",
            "post_upload_update": "state = (state + byte(object+0x21)) & 0xff; this normalization occurs after both upload calls and does not bound their current input.",
            "encoded_operand_domain": "The command encoding is any signed 16-bit value; its low word covers 0..65535 as the upload WORD input. Actual resource command values and whether a particular event precedes a particular upload remain unresolved.",
            "reachability_limit": "A direct object-list call reaches the split consumer, and the event interpreter contains a signed-word writer for the same object offset. The evidence does not establish same-object event/upload chronology or actual loaded resource operands, so it does not prove which high-y values are produced at runtime.",
            "host_address_limit": "The proven high-y A710 address helper is limited to y=512..1023. Values outside that band, even if representable in the producer WORD, have no claimed host-address formula here.",
        },
        "resource_rectangles": {
            "eng3_copy": "The pinned ENG3 parser copies four WORD values from each loaded record into a RECT and uses sign-extended width/height in source addressing; the inspected path has no x/y clipping or rectangle-bound validation.",
            "actual_values": "Resource bytes and their values were not inspected. No descriptor schema or producer range constraint is established by the existing evidence.",
            "eng1_source_note": "The existing ENG1 behavioral source draft copies four WORD descriptor values and computes frame offsets from signed width/height. This is source evidence only, not proof of original source identity or a resource bound.",
        },
        "linear_scans": {
            "ENG3.DLL": sweeps["ENG3.DLL"],
            "TITLE.DLL": sweeps["TITLE.DLL"],
            "HERCULES.EXE": sweeps["HERCULES.EXE"],
            "eng3_split_direct_call_rvas": [f"0x{x:x}" for x in eng3_calls],
            "title_setter_direct_call_rvas": [f"0x{x:x}" for x in title_setters],
            "title_context_read_rvas": [f"0x{x - images['TITLE.DLL'].image_base:x}" for x in title_reads],
            "host_context_write_rvas": [f"0x{x - herc.image_base:x}" for x in herc_writes],
        },
        "prediction_and_falsifier": {
            "prediction": "ENG3 split state might admit resource-driven values, while TITLE context y might be statically fixed at its local reads; resource RECT values might be copied without bounds checks.",
            "falsifier": "A bounded scan finds every reachable ENG3 assignment range-checked, TITLE reads without a preceding fixed setter, or a descriptor schema enforcing coordinate bounds.",
            "result": "Partly supported: ENG3 has an unbounded signed-word event assignment followed by a low-word consumer and post-upload byte normalization; TITLE's two observed local reads each follow a y=0x100 setter, and the static HERC interface word1 equals both the host's context cell and TITLE's copied word1. No direct absolute write to that source word is found in the file-backed HERC .text sweep, though indirect writes remain outside scope. Descriptor value bounds remain unresolved.",
        },
        "limits": [
            "Static direct references and instruction spans do not prove runtime object identity, resource contents, scheduler order, or indirect mutation of the writable interface word1 cell.",
            "Capstone linear sweeps cover file-backed .text only and may miss overlapping streams, computed calls, and code outside those sections.",
            "The producer WORD encoding domain is not a claim that actual resources contain every value in that domain.",
        ],
    }

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(output), "sha256": sha256(output.read_bytes()), "status": "verified"}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
