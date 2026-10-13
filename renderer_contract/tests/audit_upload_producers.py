#!/usr/bin/env python3
"""Audit pinned first-party upload callers without exporting instruction text."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys


EXPECTED_IMAGES = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "ENG3.DLL": "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
    "TITLE.DLL": "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
}
EXPECTED_ENG3_CALL_RVAS = [
    0x07C8B, 0x07CCC, 0x080F1, 0x0A29D,
    0x0AA84, 0x1AA6E, 0x1AAA2, 0x250C8,
]
EXPECTED_SPANS = {
    "eng3_split_uploader": (0x07C5B, 0x07CD1,
                            "d110326ba906a84a81c35ffe17b220865af67faa59e59dcb54f5e22a6b59babd"),
    "eng3_resource_rectangle_copy": (0x08072, 0x080F6,
                                      "ac57433c61a52059f5c014cb39c05355930e9c964df6675c69a1f744900c7ab4"),
    "eng3_tile_group_upload": (0x0A1D0, 0x0A2A2,
                               "b5c9b358a9b6c1ab57b4e6bbe31b6bff085a7e2d6b6b6dc3924694c921750195"),
    "eng3_palette_rect_upload": (0x0AA70, 0x0AA89,
                                 "751ad1d66ee9650a28a4637c3cbbab22cb5b4f6f7dc29745b7bee4e002a9231e"),
    "eng3_other_rectangle_uploads": (0x1AA30, 0x1AAB0,
                                     "c50d7feba8f549ef61d8c1d2d3181a74a79d238182679a8044705aad5ee2c2ec"),
    "eng3_startup_panel_upload": (0x250AA, 0x250D0,
                                  "3fade3fa8eabc3a909d0fae27ba2da24432325e0684114926e4974f9a64cc2e7"),
}
EXPECTED_HOST_SPANS = {
    "a710_entry": (0x0A710, 0x0A8E5,
                   "30bd84ea8724152e3bfb0e2c521650399913bcec115f1aef489768085981d0fe"),
    "a710_high_y_branch": (0x0A71F, 0x0A781,
                           "49802eb955b9960f9c0346fc0d1ada9ebee8889e430b4ddb39b88e3b19c6360b"),
    "bd80_notification_gate": (0x0BD80, 0x0BD96,
                               "6a9578c5be0c980bc35b29642d3ac389e829a0c5666022353cb88c04f19db64f"),
}
EXPECTED_PUBLIC_HELPERS = {
    "scripts/env.py": "1b743fc8adbb5df32b167fd44c39b8026bb66115ad81d68a32979c38e45047e4",
    "scripts/pe.py": "8da71eb87b5a88dd1d80a3fe180e2b94557e2c3017fb4e5c37c4a8c630af62ca",
    "renderer_contract/tests/oracle_common.py": "a024048d9acc6e18197e10a82427cd84351a90666e1626e610bf60c6105f1357",
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-root", required=True, type=Path,
                        help="checkout containing scripts/pe.py and renderer_contract/")
    parser.add_argument("--exe", required=True, type=Path,
                        help="pinned original HERCULES.EXE")
    parser.add_argument("--image-dir", type=Path,
                        help="directory containing the pinned first-party DLL images")
    parser.add_argument("--output", required=True, type=Path,
                        help="receipt path below --original-root/work")
    args = parser.parse_args()
    args.original_root = args.original_root.resolve()
    args.exe = args.exe.resolve()
    args.image_dir = (args.image_dir or
                      args.original_root / "work" / "discs" / "pc_install").resolve()
    args.output = (args.output if args.output.is_absolute() else
                   args.original_root / args.output).resolve()
    work = (args.original_root / "work").resolve()
    try:
        args.output.relative_to(work)
    except ValueError:
        parser.error("--output must resolve below --original-root/work")
    if args.output == work:
        parser.error("--output must name a file below --original-root/work")
    if not (args.original_root / "scripts" / "pe.py").is_file():
        parser.error("--original-root must contain scripts/pe.py")
    if not (args.original_root / "renderer_contract" / "tests" / "oracle_common.py").is_file():
        parser.error("--original-root must contain renderer_contract/tests/oracle_common.py")
    if not (args.original_root / "renderer_contract" / "hercules_textures.h").is_file():
        parser.error("--original-root must contain renderer_contract/hercules_textures.h")
    sys.path[:0] = [str(args.original_root / "scripts"),
                    str(args.original_root / "renderer_contract" / "tests")]
    return args, parser


def load_public_helpers(root: Path):
    import env  # noqa: PLC0415; public project helper configures pinned analysis deps.
    from pe import PE  # noqa: PLC0415
    from oracle_common import sha256 as public_sha256  # noqa: PLC0415
    helper_hashes = {}
    for relative, expected in EXPECTED_PUBLIC_HELPERS.items():
        digest = public_sha256((root / relative).read_bytes())
        if digest != expected:
            raise ValueError(f"public helper identity changed: {relative} ({digest})")
        helper_hashes[relative] = digest
    return PE, helper_hashes


def find_section(pe, name: str):
    for section in pe.sections:
        if section["name"] == name:
            return section
    raise ValueError(f"missing PE section {name}")


def file_backed_span(pe, rva: int, size: int) -> bytes:
    raw = pe.read_rva(rva, size)
    if len(raw) != size:
        raise ValueError(f"short file-backed range at RVA {rva:#x}")
    return raw


def read_u16_words(pe, rva: int):
    raw = file_backed_span(pe, rva, 8)
    return [int.from_bytes(raw[index:index + 2], "little") for index in (0, 2, 4, 6)]


def main():
    args, parser = parse_args()
    PE, helper_hashes = load_public_helpers(args.original_root)
    try:
        from capstone import Cs, CS_ARCH_X86, CS_MODE_32, x86_const
        import capstone
    except ImportError as error:
        parser.error(f"Capstone must be available through public scripts/env.py: {error}")

    paths = {"HERCULES.EXE": args.exe}
    paths.update({name: args.image_dir / name
                  for name in ("ENG1.DLL", "ENG3.DLL", "TITLE.DLL")})
    images = {}
    identities = {}
    for name, path in paths.items():
        digest = sha256(path.read_bytes())
        if digest != EXPECTED_IMAGES[name]:
            parser.error(f"pinned image identity mismatch for {name}: {digest}")
        pe = PE(path)
        images[name] = pe
        identities[name] = {"sha256": digest, "image_base": f"0x{pe.image_base:08x}"}

    eng1 = images["ENG1.DLL"]
    eng3 = images["ENG3.DLL"]
    herc = images["HERCULES.EXE"]
    rectangles = {
        "ENG1_cold_panel": read_u16_words(eng1, 0x5DF00),
        "ENG3_startup_panel": read_u16_words(eng3, 0x39868),
        "ENG3_global_35100_initial": read_u16_words(eng3, 0x35100),
        "ENG3_global_35120": read_u16_words(eng3, 0x35120),
    }
    expected_rectangles = {
        "ENG1_cold_panel": [0, 0x200, 0x80, 0x200],
        "ENG3_startup_panel": [0, 0x200, 0x80, 0x200],
        "ENG3_global_35100_initial": [0x17F, 0, 1, 0x100],
        "ENG3_global_35120": [0x300, 0xE0, 0x100, 0x20],
    }
    if rectangles != expected_rectangles:
        parser.error(f"pinned rectangle data changed: {rectangles!r}")

    text = find_section(eng3, ".text")
    block = eng3.data[text["raw_offset"]:text["raw_offset"] + text["raw_size"]]
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    decoder.skipdata = True
    base = eng3.image_base
    direct_calls = []
    decoded_count = 0
    decoded_bytes = 0
    for instruction in decoder.disasm(block, base + text["virtual_address"]):
        decoded_count += 1
        decoded_bytes += instruction.size
        if (instruction.mnemonic == "call" and instruction.operands and
                instruction.operands[0].type == x86_const.X86_OP_IMM and
                instruction.operands[0].imm == base + 0x1FE30):
            direct_calls.append(instruction.address - base)
    if decoded_bytes != len(block):
        parser.error("Capstone linear sweep did not account for every file-backed .text byte")
    if direct_calls != EXPECTED_ENG3_CALL_RVAS:
        parser.error("ENG3 direct-call scan no longer matches the pinned caller set")

    code_spans = {}
    for label, (start, end, expected_hash) in EXPECTED_SPANS.items():
        raw = file_backed_span(eng3, start, end - start)
        digest = sha256(raw)
        if digest != expected_hash:
            parser.error(f"ENG3 source span identity changed for {label}: {digest}")
        code_spans[label] = {
            "rva": f"0x{start:05x}", "size": end - start,
            "sha256": digest, "proof": "PINNED_FILE_BACKED_CODE_SPAN",
        }
    host_spans = {}
    for label, (start, end, expected_hash) in EXPECTED_HOST_SPANS.items():
        raw = file_backed_span(herc, start, end - start)
        digest = sha256(raw)
        if digest != expected_hash:
            parser.error(f"HERCULES source span identity changed for {label}: {digest}")
        host_spans[label] = {
            "rva": f"0x{start:05x}", "size": end - start,
            "sha256": digest, "proof": "PINNED_FILE_BACKED_CODE_SPAN",
        }

    result = {
        "schema_version": 2,
        "scope": "Static first-party upload-producer receipt; no resources are extracted and no game execution occurs.",
        "inputs": identities,
        "public_helpers": helper_hashes,
        "rectangles": {
            key: {"u16_words": [f"0x{value:04x}" for value in words],
                  "proof": "PINNED_FILE_BACKED_LITERAL"}
            for key, words in rectangles.items()
        },
        "ENG3_slot129_wrapper": {
            "target_rva": "0x1fe30", "direct_call_rvas": [f"0x{rva:05x}" for rva in direct_calls],
            "caller_set_match": True,
            "scan": {
                "section": ".text", "file_backed_bytes": text["raw_size"],
                "decoded_items_including_data_skips": decoded_count,
                "decoded_stream_bytes": decoded_bytes,
                "all_file_backed_bytes_walked": True,
                "capstone_version": capstone.__version__,
                "method": "Capstone 32-bit linear sweep with data-skip enabled",
                "completeness": "Complete only for this decoded linear stream; overlapping instruction streams, computed calls, and calls outside file-backed .text are not covered.",
            },
        },
        "code_spans": {**code_spans, **host_spans},
        "fixed_high_y_upload": {
            "rectangle_words": rectangles["ENG1_cold_panel"],
            "width_times_height_bytes": rectangles["ENG1_cold_panel"][2] * rectangles["ENG1_cold_panel"][3] * 2,
            "eng1_rect_rva": "0x5df00", "eng3_rect_rva": "0x39868",
            "eng3_wrapper_call_rva": "0x250c8",
            "proof": "FILE_BACKED_RECT_LITERALS_PLUS_PINNED_CALLER_SPAN_AND_NATIVE_FIXTURE",
        },
        "domain_findings": [
            {"module": "ENG1", "producer": "cold-start panel", "domain": "fixed x=0,y=512,width=128,height=512", "status": "PROVEN_LITERAL_RECT"},
            {"module": "ENG3", "producer": "startup panel", "domain": "fixed x=0,y=512,width=128,height=512", "status": "PROVEN_LITERAL_RECT_AND_SLOT129_CALL"},
            {"module": "ENG3", "producer": "split uploader", "domain": "first y derives from object state plus 512; second y derives from object state", "status": "STATE_RANGE_UNRESOLVED"},
            {"module": "ENG1/ENG3", "producer": "resource-record uploaders", "domain": "four RECT words copied from loaded records", "status": "RESOURCE_VALUES_UNRESOLVED"},
            {"module": "TITLE", "producer": "context upload", "domain": "y from context field at offset 0x138", "status": "WRITER_AND_RANGE_UNRESOLVED"},
        ],
        "limits": [
            "The startup rectangle proves the address band reached by that specific first-party producer, not ownership/allocation extent or a bound for every callback input.",
            "Literal fixed rectangles and linear-sweep callsites do not close resource values or runtime state fields.",
            "Computed call targets, overlapping code streams, non-file-backed code, and dynamic resource/state values remain outside this scan.",
        ],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": "STATIC_IMAGE_RECEIPT",
                      "eng3_wrapper_callers": len(direct_calls),
                      "high_y_rect": rectangles["ENG1_cold_panel"],
                      "receipt": str(args.output)}, indent=2))


if __name__ == "__main__":
    main()
