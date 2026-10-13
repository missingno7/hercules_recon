"""Read-only compact import and renderer-table audit for the pinned host PE."""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

PINNED_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
SOURCE_RVA = 0x361B8
SOURCE_BYTES = 250 * 4
POINTER_RVA = 0x365A0
DIRECTDRAW_THUNK_RVA = 0x10A20


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def span(pe, rva: int, size: int) -> dict:
    return {"rva": hex(rva), "size": size, "sha256": sha(pe.read_rva(rva, size))}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-root", type=Path, required=True,
                        help="reconstruction repo root containing scripts/pe.py")
    parser.add_argument("--exe", type=Path, required=True,
                        help="read-only pinned work/discs/pc_install/HERCULES.EXE")
    parser.add_argument("--output", type=Path, required=True,
                        help="JSON receipt path under original-root/work")
    args = parser.parse_args()
    root = args.original_root.resolve()
    exe = args.exe.resolve()
    output = args.output.resolve()
    work_path = root / "work"
    work = work_path.resolve()
    if work != work_path:
        parser.error("refusing a redirected work directory; output must stay in checkout/work")
    if not (root / "scripts" / "pe.py").is_file():
        parser.error("--original-root must contain scripts/pe.py")
    if not output.is_relative_to(work):
        parser.error("--output must be under original-root/work")
    if output == exe:
        parser.error("--output must not overwrite the input executable")

    sys.path.insert(0, str(root / "scripts"))
    import env  # noqa: F401,E402  (configures the repository's pinned analysis deps)
    import capstone  # noqa: E402
    from pe import PE  # noqa: E402

    file_bytes = exe.read_bytes()
    file_sha = sha(file_bytes)
    if file_sha != PINNED_SHA256:
        parser.error(f"refusing non-pinned host PE: SHA-256 is {file_sha}")
    pe = PE(exe)
    metadata = pe.metadata()
    base = pe.image_base
    code_section = next(s for s in pe.sections if s["name"] == ".text")
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    md.skipdata = True
    text_rva = code_section["virtual_address"]
    text_va = base + text_rva
    text_raw_size = code_section["raw_size"]
    text_data = pe.read_rva(text_rva, text_raw_size)
    code = list(md.disasm(text_data, text_va))

    # Record complete raw-section coverage. Capstone emits id==0 pseudo-instructions
    # for bytes it could not decode; those are coverage records, never operands.
    decoded_instructions = [i for i in code if i.id != 0 and i.mnemonic != ".byte"]
    skipped_pseudoops = [i for i in code if i.id == 0 or i.mnemonic == ".byte"]
    covered_cursor = text_va
    sweep_gaps = []
    covered_bytes = 0
    for i in code:
        if i.address > covered_cursor:
            sweep_gaps.append({"start_rva": hex(covered_cursor - base),
                               "end_rva": hex(i.address - base),
                               "size": i.address - covered_cursor})
        elif i.address < covered_cursor:
            sweep_gaps.append({"start_rva": hex(i.address - base),
                               "end_rva": hex(covered_cursor - base),
                               "size": covered_cursor - i.address,
                               "overlap": True})
        covered_cursor = max(covered_cursor, i.address + i.size)
        covered_bytes += i.size
    if covered_cursor < text_va + text_raw_size:
        sweep_gaps.append({"start_rva": hex(covered_cursor - base),
                           "end_rva": hex(text_rva + text_raw_size),
                           "size": text_va + text_raw_size - covered_cursor,
                           "trailing": True})

    # Preserve the old no-skipdata stop point as a diagnostic, not as scan scope.
    old_md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    old_code = list(old_md.disasm(text_data, text_va))
    old_end_va = old_code[-1].address + old_code[-1].size if old_code else text_va
    old_first_stop_rva = old_end_va - base if old_end_va < text_va + text_raw_size else None

    imports = {}
    descriptor_order = []
    for descriptor in metadata["imports"]:
        dll = descriptor["dll"]
        descriptor_order.append(dll)
        for symbol in descriptor["symbols"]:
            imports[(dll.lower(), symbol["name"])] = {
                "dll": dll, "name": symbol["name"], "iat_rva": symbol["iat_rva"],
                "iat_va": base + symbol["iat_rva"]}

    def iat_calls(entry: dict) -> list[int]:
        target = entry["iat_va"]
        return [i.address - base for i in code if i.mnemonic == "call" and any(
            op.type == capstone.x86.X86_OP_MEM and op.mem.base == 0 and op.mem.index == 0
            and (op.mem.disp & 0xFFFFFFFF) == target for op in i.operands)]

    def relative_calls(target_rva: int) -> list[int]:
        target = base + target_rva
        return [i.address - base for i in code if i.mnemonic in ("call", "jmp") and any(
            op.type == 2 and op.imm == target for op in i.operands)]  # X86_OP_IMM

    relevant_names = ("DirectDrawCreate", "LoadLibraryA", "GetProcAddress", "FreeLibrary",
                      "CreateDIBSection", "CreateWindowExA", "GetMessageA", "ShowWindow",
                      "StretchDIBits", "SetPaletteEntries", "SelectPalette", "RealizePalette")
    relevant_imports = []
    for (dll, name), entry in imports.items():
        if name in relevant_names:
            relevant_imports.append({"dll": entry["dll"], "name": name,
                                     "iat_rva": hex(entry["iat_rva"]),
                                     "iat_call_sites": [hex(x) for x in iat_calls(entry)]})

    directdraw = imports[("ddraw.dll", "DirectDrawCreate")]
    src_va = base + SOURCE_RVA
    src_end = src_va + SOURCE_BYTES
    table_fixed = {"reads": 0, "writes": 0, "unknown": 0, "slots_referenced": set(),
                   "slot_summary": {}}
    indexed = {"references": 0, "writes": 0}
    pointer = {"reads": 0, "writes": 0, "unknown": 0, "sites": []}
    for insn in decoded_instructions:
        rva = insn.address - base
        for operand in insn.operands:
            if operand.type != 3:  # X86_OP_MEM in the pinned Capstone API
                continue
            mem = operand.mem
            disp = mem.disp & 0xFFFFFFFF
            if mem.base == 0 and mem.index == 0:
                if src_va <= disp < src_end:
                    slot = (disp - src_va) // 4
                    table_fixed["slots_referenced"].add(slot)
                    row = table_fixed["slot_summary"].setdefault(
                        slot, {"reads": 0, "writes": 0, "unknown": 0,
                               "first_ref_rva": hex(rva), "last_ref_rva": hex(rva)})
                    row["last_ref_rva"] = hex(rva)
                    if operand.access & 2:  # CS_AC_WRITE
                        table_fixed["writes"] += 1
                        row["writes"] += 1
                    elif operand.access & 1:  # CS_AC_READ
                        table_fixed["reads"] += 1
                        row["reads"] += 1
                    else:
                        table_fixed["unknown"] += 1
                        row["unknown"] += 1
                if disp == base + POINTER_RVA:
                    access = ("writes" if operand.access & 2 else
                              "reads" if operand.access & 1 else "unknown")
                    pointer[access] += 1
                    pointer["sites"].append({"rva": hex(rva), "mnemonic": insn.mnemonic,
                                             "access": access})
            elif mem.base == 0 and mem.index != 0 and src_va <= disp < src_end:
                indexed["references"] += 1
                if operand.access & 2:
                    indexed["writes"] += 1

    resource_types = sorted({r["path"][0] for r in metadata["resources"] if r.get("path")})
    manifest_count = sum(bool(r.get("path")) and r["path"][0] == 24 for r in metadata["resources"])
    directories = pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY
    section_name = next((s["name"] for s in pe.sections
                         if s["virtual_address"] <= SOURCE_RVA < s["virtual_address"] + s["raw_size"]), None)
    table = pe.read_rva(SOURCE_RVA, SOURCE_BYTES)
    pointer_raw = pe.read_rva(POINTER_RVA, 4)
    pointer_value = struct.unpack("<I", pointer_raw)[0]
    engine_name = pe.read_rva(0x36868, 32).split(b"\0", 1)[0].decode("ascii", "replace")

    result = {
        "schema_version": 1,
        "scope": "Pinned host PE only; read-only; no game launch or file writes except this receipt.",
        "audit_script_sha256": sha(Path(__file__).read_bytes()),
        "pe_helper": {"path": str(root / "scripts" / "pe.py"),
                      "sha256": sha((root / "scripts" / "pe.py").read_bytes()),
                      "capstone_version": capstone.__version__},
        "host": {"path": str(exe), "size": len(file_bytes), "sha256": file_sha,
                 "machine": hex(pe.pe.FILE_HEADER.Machine), "optional_magic": hex(pe.pe.OPTIONAL_HEADER.Magic),
                 "image_base": hex(base), "entry_point_rva": hex(pe.pe.OPTIONAL_HEADER.AddressOfEntryPoint),
                 "subsystem": pe.pe.OPTIONAL_HEADER.Subsystem,
                 "linker_version": f"{pe.pe.OPTIONAL_HEADER.MajorLinkerVersion}.{pe.pe.OPTIONAL_HEADER.MinorLinkerVersion}",
                 "dll_characteristics": hex(pe.pe.OPTIONAL_HEADER.DllCharacteristics),
                 "loader_flags": hex(pe.pe.OPTIONAL_HEADER.LoaderFlags),
                 "relocations": {"rva": hex(directories[5].VirtualAddress), "size": hex(directories[5].Size)},
                 "delay_imports": {"rva": hex(directories[13].VirtualAddress), "size": hex(directories[13].Size)},
                 "resource_type_ids": resource_types, "rt_manifest_count": manifest_count},
        "text_sweep": {"method": "Capstone x86-32 linear sweep over complete raw .text with skipdata enabled",
                       "section_rva": hex(text_rva), "raw_size": text_raw_size,
                       "sweep_record_count": len(code),
                       "decoded_instruction_count": len(decoded_instructions),
                       "decoded_instruction_bytes": sum(i.size for i in decoded_instructions),
                       "skipdata_pseudoop_count": len(skipped_pseudoops),
                       "skipdata_bytes": sum(i.size for i in skipped_pseudoops),
                       "covered_record_bytes": covered_bytes,
                       "coverage_complete": covered_bytes == text_raw_size and not sweep_gaps,
                       "last_record_rva": hex(code[-1].address - base) if code else None,
                       "last_record_size": code[-1].size if code else 0,
                       "last_record_end_rva": hex(covered_cursor - base),
                       "gaps_or_overlaps": sweep_gaps[:16], "gap_or_overlap_count": len(sweep_gaps),
                       "old_no_skipdata_first_stop_rva": hex(old_first_stop_rva) if old_first_stop_rva is not None else None,
                       "old_no_skipdata_instruction_count": len(old_code),
                       "skipped_byte_rvas": [hex(i.address - base) for i in skipped_pseudoops[:16]],
                       "alignment_limit": "Linear sweep classifies byte sequences that decode as x86 instructions without CFG/function-boundary validation; embedded data can decode as instructions. Skipdata pseudo-ops contribute coverage but no operands."},
        "imports": {"descriptor_names_file_order": descriptor_order,
                    "warning": "PE descriptor order is not observed runtime load order.",
                    "relevant": relevant_imports,
                    "DirectDrawCreate_thunk": {"rva": hex(DIRECTDRAW_THUNK_RVA),
                                                "call_sites": [hex(x) for x in relative_calls(DIRECTDRAW_THUNK_RVA)]}},
        "dynamic_engine_loader": {"LoadLibraryA_sites": [hex(x) for x in iat_calls(imports[("kernel32.dll", "LoadLibraryA")])],
                                   "GetProcAddress_sites": [hex(x) for x in iat_calls(imports[("kernel32.dll", "GetProcAddress")])],
                                   "FreeLibrary_sites": [hex(x) for x in iat_calls(imports[("kernel32.dll", "FreeLibrary")])],
                                   "resolved_export_name": engine_name, "export_name_rva": hex(0x36868),
                                   "selected_module_path": "unproved"},
        "table": {"rva": hex(SOURCE_RVA), "va": hex(src_va), "bytes": SOURCE_BYTES,
                  "sha256": sha(table), "section": section_name,
                  "direct_fixed_address_refs": {"reads": table_fixed["reads"], "writes": table_fixed["writes"],
                                                 "unknown": table_fixed["unknown"],
                                                 "distinct_slots_referenced": len(table_fixed["slots_referenced"]),
                                                 "slots": sorted(table_fixed["slots_referenced"]),
                                                 "slot_summary": {str(k): v for k, v in sorted(table_fixed["slot_summary"].items())}},
                  "absolute_indexed_refs": indexed,
                  "scan_limit": "Complete raw .text linear sweep with undecodable bytes represented by skipdata pseudo-ops; does not rule out embedded-data false decodes, external mutation, or unresolved register/alias writes."},
        "source_pointer": {"rva": hex(POINTER_RVA), "va": hex(base + POINTER_RVA),
                           "initial_value": hex(pointer_value), "initial_bytes_sha256": sha(pointer_raw),
                           "direct_code_refs": pointer["sites"],
                           "direct_writes": pointer["writes"]},
        "timing": {"graphics_setup_rva": hex(0x72A0),
                   "graphics_setup_call_site": [hex(x) for x in relative_calls(0x72A0)],
                   "directdraw_setup_call_site_rva": hex(0x741A),
                   "directdrawcreate_thunk_call_sites": [hex(x) for x in relative_calls(DIRECTDRAW_THUNK_RVA)],
                   "directdraw_call_arguments": "GUID=NULL; output=&0x96e3e4; outer=NULL",
                   "source_pointer_read_rva": hex(0x5EF3), "first_dispatch_rva": hex(0x5F03),
                   "span_sha256": {"graphics_init": span(pe, 0x5E10, 0x7F),
                                    "graphics_setup": span(pe, 0x72A0, 0x214),
                                    "directdraw_call": span(pe, 0x7420, 0x12),
                                    "source_pointer_read": span(pe, 0x5EF3, 6),
                                    "first_dispatch": span(pe, 0x5F03, 2)},
                   "order_claim": "0x72a0 -> 0x71a0/DirectDrawCreate at 0x7426 -> source pointer read 0x5ef3 -> first engine dispatch 0x5f03; clone must be published before dispatch."},
        "bootstrap_candidate": "App-local DDRAW.dll shim may forward DirectDrawCreate unchanged, then clone/patch selected table entries and publish image_base+0x365a0 before returning. This is a static timing candidate, not host integration or proof of deployment resolution.",
        "preserve": ["original HWND", "DirectDraw object and COM vtables", "surface/backbuffer and lost-surface paths",
                     "palette and resource writers", "lock/unlock/clear/present", "movie callback/surface path"],
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(output), "sha256": sha(output.read_bytes()),
                      "host_sha256": file_sha, "table_bytes": SOURCE_BYTES,
                      "table_write_refs": table_fixed["writes"], "indexed_write_refs": indexed["writes"],
                      "pointer_write_refs": pointer["writes"]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
