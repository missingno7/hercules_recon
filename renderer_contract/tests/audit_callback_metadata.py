"""Audit selected host runtime-class, message-map, and vtable metadata.

Read-only. Requires the public bootstrap-gates receipt, pinned original PC
images, Capstone from the repository's configured Python dependency path, and
the locally installed MSVC 5.00/MFC headers whose hashes are pinned below.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

ORIGINALS = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "ENG3.DLL": "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
    "TITLE.DLL": "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
}
GATE_SPANS = {
    "event_callback_79a0": 0x79A0,
    "event_enable_writer_9820": 0x9820,
    "event_enable_writer_9840": 0x9840,
    "event_suppress_writer_9870": 0x9870,
}
LITERALS = {
    0x2E240: 0x79A0,
    0x2E5CC: 0x9820,
    0x2E62C: 0x9840,
    0x2E644: 0x9870,
}
CLASSES = {
    "CJoystickPage": {
        "runtime_rva": 0x2E290, "map_rva": 0x2E2A8, "entries_rva": 0x2E2B0,
        "vtable_rva": 0x2E310, "factory_rva": 0x8090, "factory_size": 0x5A,
        "constructor_rva": 0x8100, "constructor_size": 0xEE,
        "vptr_store_rva": 0x81D9, "runtime_getter_rva": 0x80F0,
        "map_getter_rva": 0x8360, "other_vptr_store_rvas": [0x822D],
        "pins": {
            "runtime": "efdefabcda01a9b4039d1616ce9200d739afb9622663ead2118eae52ef5cbdf5",
            "map": "11d54812474013ddc1fe921ae304abd1e80e9eca7c3dd1bd10fb6f03c83503f6",
            "entries": "4da07741cddc849842d3206f615f6299975bdcaf0192b2e66f5107c2ae78e905",
            "vtable": "b72e9abdd5caec0541e88c952e5b9949cd309c4717105dc13d45ed1860c9a2a4",
            "factory": "2c481822b3e022c6231b6ccce3c7985f9c85b8bdfab2a4b1c603dc62a1b078db",
            "constructor": "7144ca6fce9c561723d6236cf09f92a7c8f0c8a563666a1115d08c94487ae891",
            "constructor_store": "70e50fe5a6e4593267565578450986aa66dd806a591c36e4cce61f95a1058a01",
        },
    },
    "CKeyboardPage": {
        "runtime_rva": 0x2E408, "map_rva": 0x2E420, "entries_rva": 0x2E428,
        "vtable_rva": 0x2E470, "factory_rva": 0x8A30, "factory_size": 0x5A,
        "constructor_rva": 0x8AA0, "constructor_size": 0x136,
        "vptr_store_rva": 0x8BC1, "runtime_getter_rva": 0x8A90,
        "map_getter_rva": 0x8DC0, "other_vptr_store_rvas": [0x8C1D],
        "pins": {
            "runtime": "4cd58486cefb4ab0b39e01d2bd5f316a9b517c3d5f5f6ec59373d1f9011ee8e3",
            "map": "2d09e9325fe3d2fd6bee3b4d84698514a386f2f5fb65c52afc5e7158658954f1",
            "entries": "3a05fedd680127fed60ab3efefb6526711ae8d4542ae1deaf42d67ebce3d8af3",
            "vtable": "90e3e62f20c20eaac974a4368a1bdde4c8594336cec38770118714eea1f0a750",
            "factory": "452e73085935ae5a03df31dca2296f24ef36e9dc01c6442f189a3d1a34430fa9",
            "constructor": "47eb92965773485257946da7e69338038e5eb50ef29d5429f6f87cc20fa6f661",
            "constructor_store": "a776442f7bb2c71dbd78388ae1986be74a57e872849704e6071a14fe62b7c0ec",
        },
    },
    "CMainFrame": {
        "runtime_rva": 0x2E568, "map_rva": 0x2E580, "entries_rva": 0x2E588,
        "vtable_rva": 0x2E870, "factory_rva": 0x9420, "factory_size": 0x5A,
        "constructor_rva": 0x94A0, "constructor_size": 0x12,
        "vptr_store_rva": 0x94A8, "runtime_getter_rva": 0x9480,
        "map_getter_rva": 0x9490, "other_vptr_store_rvas": [0x94E0],
        "pins": {
            "runtime": "df3b79ae0bb34280bd012c7d380a6b6ae00201f1344a9a5cf75544d676b45225",
            "map": "c3a9aadbe3b7fa0108c3cf91dbcd10e99bf83da5da0c30e2e862791605bb5975",
            "entries": "43a94caa61152bd42a7b89f40ec1ee38cb7f87276ea2bbc4be0ac66621b32807",
            "vtable": "1d2536ac03198ff754dd11a85b080ef8218d84116480362f91285a1d16316eff",
            "factory": "a328772cea1f7bef617975cf5deb31f8a7193baeea598c1fdc92573c8fe5aecc",
            "constructor": "0326298792e34b1509b93fc357a37ad83c40f1bea00719bbb68d4873af2db5d5",
            "constructor_store": "e626c3ba0c4374fd26b9713b7b7f9ebd28f56b5fb06815738ed99a30c8e26671",
        },
    },
}
MFC_HEADER_HASHES = {
    "afx.h": "3ca0caeb968ceb2c99b81251dc29f7ca5ad275358633ff3f514b4fcd08a72902",
    "afxwin.h": "87603d834fcaa1d21b94ebc241f26164ab7698898b00ea774f249196540a7087",
    "afxmsg_.h": "9b590d7b43f3aed98491ed1bf8b7466e5faf6193e5d357284c320e8a0033fb67",
    "winuser.h": "df89c70f2c4392643f25d8b7e70d19b2052041d3bf4e2cb08f671ee6483a20ea",
    "README.md": "7641c6d9f64e3b14da391954528faf93d5f79e798418506639f7ac408feda213",
}
MFC_TOOL_ROOT = Path(r"C:\tools\hercules\msvc500sp2-4ebf02022705b4c9e9108d3ed3f286ed80ba2ed9")
EXPECTED_MESSAGES = {
    0x9820: {"rva": 0x2E5B8, "message": 0x001C, "code": 0,
            "id_first": 0, "id_last": 0, "signature": 0x0E},
    0x9840: {"rva": 0x2E618, "message": 0x0111, "code": 0,
            "id_first": 0xE146, "id_last": 0xE146, "signature": 0x0C},
    0x9870: {"rva": 0x2E630, "message": 0x0005, "code": 0,
            "id_first": 0, "id_last": 0, "signature": 0x11},
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def hx(value: int) -> str:
    return hex(value & 0xFFFFFFFF)


def find_repo(explicit: Path | None) -> Path:
    if explicit is not None:
        root = explicit.resolve()
        if (root / "renderer_contract").is_dir() and (root / "scripts" / "pe.py").is_file():
            return root
        raise ValueError(f"repo root lacks renderer_contract and scripts/pe.py: {root}")
    for parent in Path(__file__).resolve().parents:
        if (parent / "renderer_contract").is_dir() and (parent / "scripts" / "pe.py").is_file():
            return parent
    raise ValueError("cannot locate checkout; pass --repo-root or --original-root")


def find_image_dir(root: Path, requested: Path | None) -> Path:
    if requested is not None:
        candidates = [requested if requested.is_absolute() else root / requested]
    else:
        candidates = [root / "work" / "discs" / "pc_install",
                      root.parent / "discs" / "pc_install"]
    for candidate in candidates:
        resolved = candidate.resolve()
        if all((resolved / name).is_file() for name in ORIGINALS):
            return resolved
    raise ValueError("original PC images not found; pass --image-dir")


def section_for(pe, rva: int):
    for section in pe.sections:
        lo = section["virtual_address"]
        end = lo + section["raw_size"]
        if lo <= rva and rva < end:
            return section
    return None


def exact_span(pe, module: str, name: str, rva: int, size: int, section_name: str):
    section = section_for(pe, rva)
    if not section or section["name"] != section_name or rva + size > section["virtual_address"] + section["raw_size"]:
        raise ValueError(f"{module} {name} is not wholly file-backed in {section_name}: {hx(rva)}+{size:#x}")
    raw = pe.read_rva(rva, size)
    return {"module": module, "name": name, "rva": hx(rva), "size": size,
            "sha256": sha(raw)}


def c_string(pe, image_base: int, va: int, limit: int = 128):
    rva = va - image_base
    if not section_for(pe, rva):
        raise ValueError(f"class-name pointer is not file-backed: {hx(va)}")
    raw = pe.read_rva(rva, limit).split(b"\0", 1)[0]
    text = raw.decode("ascii")
    return {"value": text, "rva": hx(rva), "size_with_nul": len(raw) + 1,
            "sha256_with_nul": sha(raw + b"\0")}


def pointer_words(pe, rva: int, count: int):
    return list(struct.unpack("<" + "I" * count, pe.read_rva(rva, count * 4)))


def message_rows(pe, image_base: int, start_rva: int, maximum: int = 64):
    rows = []
    for index in range(maximum):
        rva = start_rva + index * 24
        raw = pe.read_rva(rva, 24)
        values = struct.unpack("<6I", raw)
        rows.append({"index": index, "rva": hx(rva), "size": 24,
                     "sha256": sha(raw), "fields": values})
        if values == (0, 0, 0, 0, 0, 0):
            span = pe.read_rva(start_rva, (index + 1) * 24)
            return rows, {"rva": hx(start_rva), "size": len(span), "sha256": sha(span)}
    raise ValueError(f"no message-map sentinel within {maximum} rows at {hx(start_rva)}")


def vtable(pe, image_base: int, rva: int, maximum: int = 128):
    rows = []
    raw = bytearray()
    for index in range(maximum):
        entry_rva = rva + index * 4
        value = struct.unpack("<I", pe.read_rva(entry_rva, 4))[0]
        target_rva = value - image_base
        target = section_for(pe, target_rva)
        if not target or target["name"] != ".text":
            break
        raw.extend(struct.pack("<I", value))
        rows.append({"slot": index, "entry_rva": hx(entry_rva), "target_rva": hx(target_rva)})
    return {"rva": hx(rva), "word_count": len(rows), "size": len(raw),
            "sha256": sha(bytes(raw)), "entries": rows}


def code_xrefs_to_interval(modules, capstone, target_image_base: int, lo_rva: int, hi_rva: int):
    result = {}
    for name, item in modules.items():
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        md.detail = True
        md.skipdata = True
        module_base = item["pe"].image_base
        hits = []
        for insn in md.disasm(item["text"], module_base + item["text_rva"]):
            if insn.id == 0:
                continue
            for operand in insn.operands:
                value = None
                if operand.type == capstone.x86.X86_OP_IMM:
                    value = operand.imm & 0xFFFFFFFF
                elif operand.type == capstone.x86.X86_OP_MEM and not operand.mem.base and not operand.mem.index:
                    value = operand.mem.disp & 0xFFFFFFFF
                if value is not None and target_image_base + lo_rva <= value < target_image_base + hi_rva:
                    hits.append({"site_rva": hx(insn.address - module_base),
                                 "mnemonic": insn.mnemonic, "operands": insn.op_str,
                                 "instruction_sha256": sha(bytes(insn.bytes)),
                                 "referenced_va": hx(value)})
        result[name] = hits
    return result


def aligned_pointer_roots(modules, image_base: int, lo_rva: int, hi_rva: int):
    hits = []
    for name, item in modules.items():
        pe = item["pe"]
        for section in pe.sections:
            raw = pe.read_rva(section["virtual_address"], section["raw_size"])
            for offset in range(0, len(raw) - 3, 4):
                value = struct.unpack_from("<I", raw, offset)[0]
                target = value - image_base
                if lo_rva <= target < hi_rva:
                    hits.append({"module": name, "section": section["name"],
                                 "storage_rva": hx(section["virtual_address"] + offset),
                                 "target_rva": hx(target)})
    return hits


def exact_pointer_locations(modules, needle_va: int):
    needle = struct.pack("<I", needle_va)
    hits = []
    for name, item in modules.items():
        pe = item["pe"]
        for section in pe.sections:
            raw = pe.read_rva(section["virtual_address"], section["raw_size"])
            offset = raw.find(needle)
            while offset >= 0:
                hits.append({"module": name, "section": section["name"],
                             "rva": hx(section["virtual_address"] + offset)})
                offset = raw.find(needle, offset + 1)
    return hits


def header_provenance(tool_root: Path, mfc_include: Path, win32_include: Path):
    files = {
        "afx.h": mfc_include / "afx.h",
        "afxwin.h": mfc_include / "afxwin.h",
        "afxmsg_.h": mfc_include / "afxmsg_.h",
        "winuser.h": win32_include / "winuser.h",
        "README.md": tool_root / "README.md",
    }
    result = {}
    contents = {}
    for name, path in files.items():
        raw = path.read_bytes()
        actual = sha(raw)
        if actual != MFC_HEADER_HASHES[name]:
            raise ValueError(f"installed header/tool readme hash mismatch for {path}: {actual}")
        result[name] = {"path": str(path), "sha256": actual, "size": len(raw)}
        contents[name] = raw.decode("latin-1")
    if "Product version: 5.00.7022" not in contents["README.md"]:
        raise ValueError("installed MSVC package README no longer confirms product version")
    for marker in ("struct CRuntimeClass", "m_lpszClassName", "m_nObjectSize",
                   "m_wSchema", "m_pfnCreateObject", "m_pBaseClass", "m_pNextClass"):
        if marker not in contents["afx.h"]:
            raise ValueError(f"pinned afx.h missing {marker}")
    for marker in ("struct AFX_MSGMAP", "pBaseMap", "lpEntries",
                   "struct AFX_MSGMAP_ENTRY", "nMessage", "nCode", "nID",
                   "nLastID", "nSig", "pfn", "BEGIN_MESSAGE_MAP", "END_MESSAGE_MAP"):
        if marker not in contents["afxwin.h"]:
            raise ValueError(f"pinned afxwin.h missing {marker}")
    enum_match = re.search(r"enum\s+AfxSig\s*\{(.*?)\};", contents["afxmsg_.h"], re.S)
    if not enum_match:
        raise ValueError("pinned afxmsg_.h has no AfxSig enum")
    enum_body = re.sub(r"/\*.*?\*/|//[^\n]*", "", enum_match.group(1), flags=re.S)
    signature_values = {}
    next_value = 0
    for item in enum_body.split(","):
        item = item.strip()
        if not item:
            continue
        match = re.fullmatch(r"(AfxSig_\w+)(?:\s*=\s*(0x[0-9a-fA-F]+|\d+))?", item)
        if not match:
            continue
        name, explicit = match.groups()
        value = int(explicit, 0) if explicit else next_value
        signature_values[value] = name
        next_value = value + 1
    windows_messages = {"WM_ACTIVATEAPP": 0x001C, "WM_COMMAND": 0x0111, "WM_SIZE": 0x0005}
    windows_source_lines = {}
    for name, value in windows_messages.items():
        found = re.search(rf"^#define\s+{name}\s+(0x[0-9A-Fa-f]+)\s*$", contents["winuser.h"], re.M)
        if not found or int(found.group(1), 16) != value:
            raise ValueError(f"pinned winuser.h does not define expected {name}={value:#x}")
        windows_source_lines[name] = contents["winuser.h"][:found.start()].count("\n") + 1
    signatures = {0x0E: "AfxSig_vww", 0x0C: "AfxSig_vv", 0x11: "AfxSig_vwii"}
    signature_source_lines = {}
    signature_declarations = {}
    for value, expected_name in signatures.items():
        if signature_values.get(value) != expected_name:
            raise ValueError(f"pinned AfxSig enum value {value:#x} is not {expected_name}")
        declaration_line = next(line for line in contents["afxmsg_.h"].splitlines()
                                if re.match(rf"^\s*{expected_name}\s*(?:,|=)", line))
        signature_source_lines[expected_name] = contents["afxmsg_.h"].splitlines().index(declaration_line) + 1
        signature_declarations[expected_name] = declaration_line.split("//", 1)[-1].strip()
    source_spans = {
        "afx.h": [(409, 430, "CRuntimeClass fields")],
        "afxwin.h": [(1162, 1169, "AFX_MSGMAP fields"),
                     (1175, 1215, "message-map macros and sentinel"),
                     (1849, 1856, "AFX_MSGMAP_ENTRY fields")],
        "afxmsg_.h": [(46, 124, "AfxSig enum and aliases")],
    }
    for filename, spans in source_spans.items():
        lines = contents[filename].splitlines()
        result[filename]["definition_spans"] = []
        for first, last, purpose in spans:
            excerpt = "\n".join(lines[first - 1:last]) + "\n"
            result[filename]["definition_spans"].append({
                "first_line": first, "last_line": last, "purpose": purpose,
                "sha256": sha(excerpt.encode("latin-1")),
            })
    result["metadata"] = {
        "origin": "Installed local tool package directory named msvc500sp2-4ebf02022705b4c9e9108d3ed3f286ed80ba2ed9. Its pinned README attributes the package to https://github.com/archaic-msvc/msvc500 and identifies Microsoft Visual C++ 5.00, product version 5.00.7022, driver 11.00.7022, linker 5.02.7132.",
            "windows_message_source_lines": windows_source_lines,
            "afx_signature_source_lines": signature_source_lines,
        "afx_signature_declarations": signature_declarations,
    }
    return result


def class_record(pe, image_base: int, module: str, name: str, spec, capstone):
    runtime_span = exact_span(pe, module, f"{name}.CRuntimeClass", spec["runtime_rva"], 24, ".rdata")
    runtime_raw = pe.read_rva(spec["runtime_rva"], 24)
    runtime = struct.unpack("<6I", runtime_raw)
    class_name = c_string(pe, image_base, runtime[0])
    base_rva = runtime[4] - image_base
    base_class_name = c_string(pe, image_base, struct.unpack("<I", pe.read_rva(base_rva, 4))[0])
    map_span = exact_span(pe, module, f"{name}.message_map_descriptor", spec["map_rva"], 8, ".rdata")
    map_raw = pe.read_rva(spec["map_rva"], 8)
    base_map_va, entries_va = struct.unpack("<2I", map_raw)
    base_map_section = section_for(pe, base_map_va - image_base)
    if not base_map_section or base_map_section["name"] != ".rdata":
        raise ValueError(f"{name} base map is not file-backed .rdata")
    if entries_va - image_base != spec["entries_rva"]:
        raise ValueError(f"{name} map descriptor does not point at expected entries")
    rows, entries_span = message_rows(pe, image_base, spec["entries_rva"])
    if entries_span["sha256"] != spec["pins"]["entries"]:
        raise ValueError(f"{name} message entries span hash mismatch")
    vtab = vtable(pe, image_base, spec["vtable_rva"])
    if vtab["sha256"] != spec["pins"]["vtable"]:
        raise ValueError(f"{name} vtable span hash mismatch")
    if runtime[3] != image_base + spec["factory_rva"]:
        raise ValueError(f"{name} runtime factory pointer mismatch")
    factory_span = exact_span(pe, module, f"{name}.factory", spec["factory_rva"], spec["factory_size"], ".text")
    constructor_span = exact_span(pe, module, f"{name}.constructor", spec["constructor_rva"], spec["constructor_size"], ".text")
    store_span = exact_span(pe, module, f"{name}.constructor_vptr_store", spec["vptr_store_rva"], 6, ".text")
    factory_raw = pe.read_rva(spec["factory_rva"], spec["factory_size"])
    ctor_raw = pe.read_rva(spec["constructor_rva"], spec["constructor_size"])
    store_raw = pe.read_rva(spec["vptr_store_rva"], 6)
    for key, raw in (("runtime", runtime_raw), ("map", map_raw), ("factory", factory_raw),
                     ("constructor", ctor_raw), ("constructor_store", store_raw)):
        if sha(raw) != spec["pins"][key]:
            raise ValueError(f"{name} {key} span hash mismatch")
    constructor_va = image_base + spec["constructor_rva"]
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    md.skipdata = True
    factory_insns = list(md.disasm(factory_raw, image_base + spec["factory_rva"]))
    if not any(ins.mnemonic == "call" and ins.op_str == hx(constructor_va) for ins in factory_insns):
        raise ValueError(f"{name} factory does not directly call its pinned constructor")
    store_insns = list(md.disasm(store_raw, image_base + spec["vptr_store_rva"]))
    if not any(ins.mnemonic == "mov" and hx(image_base + spec["vtable_rva"]) in ins.op_str
               for ins in store_insns):
        raise ValueError(f"{name} constructor vptr store does not point at its vtable")
    runtime_slot = next((x for x in vtab["entries"] if int(x["target_rva"], 16) == spec["runtime_getter_rva"]), None)
    map_slot = next((x for x in vtab["entries"] if int(x["target_rva"], 16) == spec["map_getter_rva"]), None)
    if not runtime_slot or not map_slot:
        raise ValueError(f"{name} vtable lacks its declared runtime/map getter target")
    getter_spans = []
    for getter_name, getter_rva, target_rva in (
        ("GetRuntimeClass", spec["runtime_getter_rva"], spec["runtime_rva"]),
        ("GetMessageMap", spec["map_getter_rva"], spec["map_rva"]),
    ):
        getter_span = exact_span(pe, module, f"{name}.{getter_name}", getter_rva, 6, ".text")
        getter = pe.read_rva(getter_rva, 6)
        decoded = list(md.disasm(getter, image_base + getter_rva))
        if not decoded or decoded[0].mnemonic != "mov" or hx(image_base + target_rva) not in decoded[0].op_str:
            raise ValueError(f"{name} {getter_name} does not return its pinned metadata address")
        getter_spans.append({**getter_span,
                             "vtable_slot": (runtime_slot if getter_name == "GetRuntimeClass" else map_slot)["slot"]})
    return {
        "name": name, "module": module,
        "runtime_class": {**runtime_span, "class_name": class_name,
                          "object_size": hx(runtime[1]), "schema": hx(runtime[2]),
                          "factory_va": hx(runtime[3]), "base_class_name": base_class_name,
                          "base_class_runtime_rva": hx(base_rva)},
        "factory_and_constructor": {
            "factory": {**factory_span,
                        "directly_calls_constructor_rva": hx(spec["constructor_rva"])},
            "constructor": {**constructor_span,
                            "stores_vtable_base_at": hx(spec["vptr_store_rva"])},
            "constructor_vptr_store": {**store_span, "vtable_base_va": hx(image_base + spec["vtable_rva"])},
        },
        "message_map": {"descriptor": map_span, "base_map_va": hx(base_map_va),
                         "base_map_rva": hx(base_map_va - image_base),
                         "entries_va": hx(entries_va), "entries": entries_span,
                         "entry_count_including_sentinel": len(rows)},
        "vtable": {"module": module, "rva": vtab["rva"], "size": vtab["size"], "sha256": vtab["sha256"],
                   "word_count": vtab["word_count"], "getters": getter_spans},
        "other_same_vptr_store_sites": [
            exact_span(pe, module, f"{name}.other_same_vptr_store", site, 6, ".text")
            for site in spec["other_vptr_store_rvas"]
        ],
    }, rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path)
    parser.add_argument("--original-root", type=Path,
                        help="alias for --repo-root: checkout containing renderer_contract and scripts/pe.py")
    parser.add_argument("--image-dir", type=Path)
    parser.add_argument("--mfc-include", type=Path,
                        default=MFC_TOOL_ROOT / "mfc" / "include")
    parser.add_argument("--win32-include", type=Path,
                        default=MFC_TOOL_ROOT / "include")
    parser.add_argument("--output", type=Path,
                        help="JSON receipt path under repo-root/work; defaults to work/renderer_callback_metadata/callback_metadata.json")
    args = parser.parse_args()
    if args.repo_root and args.original_root and args.repo_root.resolve() != args.original_root.resolve():
        parser.error("--repo-root and --original-root must resolve to the same checkout")
    root = find_repo(args.repo_root or args.original_root)
    image_dir = find_image_dir(root, args.image_dir)
    work_root = (root / "work").resolve()
    output = args.output or Path("work/renderer_callback_metadata/callback_metadata.json")
    output = output if output.is_absolute() else root / output
    output = output.resolve()
    try:
        output.relative_to(work_root)
    except ValueError:
        parser.error("--output must be under repo-root/work")
    try:
        output.relative_to(image_dir)
        parser.error("--output must be outside the original image directory")
    except ValueError:
        pass

    sys.path.insert(0, str(root / "scripts"))
    import env  # adds configured C:/tools/hercules/python dependencies
    import capstone
    from pe import PE

    bootstrap_path = root / "renderer_contract" / "evidence" / "bootstrap_gates.json"
    bootstrap_script = root / "renderer_contract" / "tests" / "audit_bootstrap_gates.py"
    if not bootstrap_path.is_file() or not bootstrap_script.is_file():
        parser.error("public bootstrap-gates JSON or audit script is missing from renderer_contract")
    bootstrap = json.loads(bootstrap_path.read_text(encoding="utf-8"))
    bootstrap_script_sha = sha(bootstrap_script.read_bytes())
    if bootstrap.get("audit_script_sha256") != bootstrap_script_sha:
        parser.error("public bootstrap-gates receipt does not pin the current public audit script")

    modules = {}
    for name, expected_sha in ORIGINALS.items():
        path = image_dir / name
        actual_sha = sha(path.read_bytes())
        if actual_sha != expected_sha:
            parser.error(f"original hash mismatch for {name}: {actual_sha}")
        pe = PE(path)
        text_section = next(sec for sec in pe.sections if sec["name"] == ".text")
        modules[name] = {
            "pe": pe, "sha256": actual_sha, "size": path.stat().st_size,
            "path": str(path), "text_rva": text_section["virtual_address"],
            "text": pe.read_rva(text_section["virtual_address"], text_section["raw_size"]),
        }
    host = modules["HERCULES.EXE"]["pe"]
    image_base = host.image_base
    if image_base != 0x400000:
        parser.error("unexpected HERCULES.EXE image base")

    target_spans = {}
    pinned = bootstrap.get("pinned_host_spans", {})
    for name, expected_rva in GATE_SPANS.items():
        span = pinned.get(name)
        if not span or int(span["rva"], 16) != expected_rva or span.get("module") != "HERCULES.EXE":
            parser.error(f"public bootstrap receipt does not pin {name} at {hx(expected_rva)}")
        raw_span = exact_span(host, "HERCULES.EXE", name, expected_rva, int(span["size"]), ".text")
        if raw_span["sha256"] != span["sha256"]:
            parser.error(f"public bootstrap source span hash mismatch: {name}")
        target_spans[name] = raw_span

    rdata = next(sec for sec in host.sections if sec["name"] == ".rdata")
    literals = {}
    for storage_rva, code_rva in LITERALS.items():
        if not (rdata["virtual_address"] <= storage_rva < rdata["virtual_address"] + rdata["raw_size"]):
            parser.error(f"literal storage {hx(storage_rva)} is not file-backed .rdata")
        target_va = struct.unpack("<I", host.read_rva(storage_rva, 4))[0]
        if target_va != image_base + code_rva:
            parser.error(f"literal {hx(storage_rva)} value changed: {hx(target_va)}")
        literals[hx(storage_rva)] = {"module": "HERCULES.EXE",
                                     "storage_rva": hx(storage_rva), "size": 4,
                                     "sha256": sha(host.read_rva(storage_rva, 4)),
                                     "target_va": hx(target_va), "target_rva": hx(code_rva)}

    classes = {}
    entry_rows_by_class = {}
    for name, spec in CLASSES.items():
        cls, rows = class_record(host, image_base, "HERCULES.EXE", name, spec, capstone)
        classes[name] = cls
        entry_rows_by_class[name] = rows
        if cls["runtime_class"]["sha256"] != spec["pins"]["runtime"]:
            parser.error(f"{name} runtime record hash mismatch")
        if cls["message_map"]["descriptor"]["sha256"] != spec["pins"]["map"]:
            parser.error(f"{name} map descriptor hash mismatch")
        if cls["factory_and_constructor"]["factory"]["sha256"] != spec["pins"]["factory"]:
            parser.error(f"{name} factory span hash mismatch")
        if cls["factory_and_constructor"]["constructor"]["sha256"] != spec["pins"]["constructor"]:
            parser.error(f"{name} constructor span hash mismatch")
        if cls["factory_and_constructor"]["constructor_vptr_store"]["sha256"] != spec["pins"]["constructor_store"]:
            parser.error(f"{name} constructor vptr-store hash mismatch")

    mfc_include = args.mfc_include.resolve()
    win32_include = args.win32_include.resolve()
    headers = header_provenance(mfc_include.parent.parent, mfc_include, win32_include)
    # Confirm custom include path options resolve to the same pinned files.
    for filename, path in (("afx.h", args.mfc_include / "afx.h"),
                           ("afxwin.h", args.mfc_include / "afxwin.h"),
                           ("afxmsg_.h", args.mfc_include / "afxmsg_.h"),
                           ("winuser.h", args.win32_include / "winuser.h")):
        if sha(path.read_bytes()) != MFC_HEADER_HASHES[filename]:
            parser.error(f"header override does not match pinned installed header {filename}")

    winuser_path = win32_include / "winuser.h"
    winuser_text = winuser_path.read_text(encoding="latin-1")
    mfc_sig = {}
    sig_enum_text = (mfc_include / "afxmsg_.h").read_text(encoding="latin-1")
    sig_enum = re.search(r"enum\s+AfxSig\s*\{(.*?)\};", re.sub(r"/\*.*?\*/|//[^\n]*", "", sig_enum_text, flags=re.S), re.S)
    assert sig_enum
    next_sig = 0
    for item in sig_enum.group(1).split(","):
        item = item.strip()
        match = re.fullmatch(r"(AfxSig_\w+)(?:\s*=\s*(0x[0-9a-fA-F]+|\d+))?", item)
        if match:
            sig_name, explicit = match.groups()
            next_sig = int(explicit, 0) if explicit else next_sig
            mfc_sig[next_sig] = sig_name
            next_sig += 1
    message_names = {0x001C: "WM_ACTIVATEAPP", 0x0111: "WM_COMMAND", 0x0005: "WM_SIZE"}
    message_rows = []
    for target_rva, expected in EXPECTED_MESSAGES.items():
        rows = entry_rows_by_class["CMainFrame"]
        matches = [row for row in rows if row["rva"] == hx(expected["rva"])]
        if len(matches) != 1:
            parser.error(f"expected one CMainFrame row at {hx(expected['rva'])}")
        row = matches[0]
        n_message, n_code, n_id, n_last_id, n_sig, pfn = row["fields"]
        actual = {"message": n_message, "code": n_code, "id_first": n_id,
                  "id_last": n_last_id, "signature": n_sig}
        if actual != {key: expected[key] for key in actual} or pfn != image_base + target_rva:
            parser.error(f"CMainFrame message-map row mismatch at {hx(expected['rva'])}")
        msg_name = message_names.get(n_message)
        if not msg_name or not re.search(rf"^#define\s+{msg_name}\s+0x0*{n_message:x}\s*$", winuser_text, re.M | re.I):
            parser.error(f"no pinned Windows header name for message {hx(n_message)}")
        sig_name = mfc_sig.get(n_sig)
        if not sig_name:
            parser.error(f"no pinned MFC AfxSig name for signature {hx(n_sig)}")
        message_line = headers["metadata"]["windows_message_source_lines"][msg_name]
        signature_line = headers["metadata"]["afx_signature_source_lines"][sig_name]
        signature_declaration = headers["metadata"]["afx_signature_declarations"][sig_name]
        message_rows.append({
            "module": "HERCULES.EXE", "handler_rva": hx(target_rva), "handler_va": hx(pfn),
            "entry_rva": row["rva"], "entry_size": 24, "entry_sha256": row["sha256"],
            "fields": {"nMessage": {"value": hx(n_message), "name": msg_name,
                                      "header": "winuser.h", "line": message_line},
                       "nCode": hx(n_code), "nID": hx(n_id), "nLastID": hx(n_last_id),
                       "nSig": {"value": hx(n_sig), "name": sig_name,
                                "declaration": signature_declaration,
                                "header": "afxmsg_.h", "line": signature_line},
                       "pfn": hx(pfn)},
        })

    pointer_cluster_rva, pointer_cluster_size = 0x2E210, 0x70
    pointer_cluster = exact_span(host, "HERCULES.EXE", "unowned_code_pointer_cluster",
                                 pointer_cluster_rva, pointer_cluster_size, ".rdata")
    if pointer_cluster["sha256"] != "37bbfdd6506f03425633e9248b34195a21cdd06aab860ade466b89871cbaaa2c":
        parser.error("unowned 0x2e210 pointer-cluster span hash mismatch")
    cluster_values = pointer_words(host, pointer_cluster_rva, pointer_cluster_size // 4)
    cluster_target_sections = []
    for index, value in enumerate(cluster_values):
        target = section_for(host, value - image_base) if value else None
        cluster_target_sections.append({"index": index, "entry_rva": hx(pointer_cluster_rva + index * 4),
                                        "target_rva": hx(value - image_base) if value else None,
                                        "target_section": target["name"] if target else None})
    if any(row["target_section"] != ".text" for row in cluster_target_sections[:-1]) or cluster_values[-1] != 0:
        parser.error("pointer-cluster shape no longer matches 27 text pointers followed by zero")
    cluster_xrefs = code_xrefs_to_interval(modules, capstone, image_base,
                                           pointer_cluster_rva, pointer_cluster_rva + pointer_cluster_size)
    pointer_roots = aligned_pointer_roots(modules, image_base,
                                          pointer_cluster_rva, pointer_cluster_rva + pointer_cluster_size)
    callback_79a0_locations = exact_pointer_locations(modules, image_base + 0x79A0)
    if callback_79a0_locations != [{"module": "HERCULES.EXE", "section": ".rdata", "rva": "0x2e240"}]:
        parser.error("0x79a0 code pointer occurrence differs from bounded source result")

    bootstrap_raw = bootstrap_path.read_bytes()
    result = {
        "schema_version": 1,
        "status": "STATIC_OBSERVED_BOUNDED_METADATA_AUDIT",
        "scope": "Original PC module bytes, selected C++ runtime-class/message-map/vtable data, constructors and getters, plus pinned installed header declarations. No MFC invocation, message dispatch, scheduler order, process launch, or gameplay is observed.",
        "inputs": {
            "audit_runner": {"intended_public_path": "renderer_contract/tests/audit_callback_metadata.py",
                              "sha256": sha(Path(__file__).read_bytes())},
            "public_bootstrap_gates_receipt": {"path": "renderer_contract/evidence/bootstrap_gates.json",
                                               "sha256": sha(bootstrap_raw),
                                               "audit_script_sha256": bootstrap_script_sha},
            "pe_helper_sha256": sha((root / "scripts" / "pe.py").read_bytes()),
            "capstone_version": capstone.__version__,
            "originals": {name: {"sha256": item["sha256"], "size": item["size"]}
                          for name, item in modules.items()},
            "installed_headers": headers,
        },
        "bootstrap_pinned_callback_functions": target_spans,
        "literal_locations": literals,
        "classes": classes,
        "cmainframe_callback_rows": message_rows,
        "unowned_79a0_cluster": {
            **pointer_cluster,
            "target_shape": cluster_target_sections,
            "function_pointer_occurrences": callback_79a0_locations,
            "direct_code_operand_xrefs_to_cluster": cluster_xrefs,
            "aligned_pointer_roots_to_cluster": pointer_roots,
            "frontier": "The 0x79a0 literal belongs to this dense code-pointer region, but these bounded references do not identify its table owner or slot consumer. Do not classify the region as a vtable, message map, or registration record.",
        },
        "interpretation": {
            "header_layout": "The pinned local VC5 MFC headers define six-field CRuntimeClass, two-pointer AFX_MSGMAP, and six-field AFX_MSGMAP_ENTRY. Their class-name/factory/base pointers, map descriptor fields, 24-byte entry rows, zero terminator, and AfxSig enumeration match the observed EXE records.",
            "original_code_links": "Separately, original getters return each runtime/map descriptor, the runtime factory calls its constructor, constructors store their corresponding vtable base, and those vtables contain GetRuntimeClass and GetMessageMap code pointers at slots 0 and 12. These static code links do not prove framework invocation.",
            "callback_classification": "0x9820, 0x9840, and 0x9870 are CMainFrame AFX_MSGMAP_ENTRY pfn values. They are message handlers, not virtual-method slots. The message and AfxSig names are emitted only after validation against the pinned winuser.h and afxmsg_.h hashes.",
        },
        "limits": "Byte scans and complete-section Capstone linear sweeps are static candidates, not runtime behavior or perfect instruction-alignment proofs. Names for custom control ID 0xE146 are intentionally omitted; no application resource mapping was established.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(output), "sha256": sha(output.read_bytes()),
                      "repo_root": str(root), "image_dir": str(image_dir),
                      "classes": list(classes), "callback_rows": len(message_rows),
                      "unowned_79a0_xrefs": sum(len(rows) for rows in cluster_xrefs.values()),
                      "unowned_79a0_pointer_roots": len(pointer_roots)}, indent=2))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, struct.error) as exc:
        raise SystemExit(f"audit failed: {exc}")
