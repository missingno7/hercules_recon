from __future__ import annotations
import argparse, hashlib, json, struct, sys
from pathlib import Path

PINNED_EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
PINNED_PE_HELPER_SHA256 = "8da71eb87b5a88dd1d80a3fe180e2b94557e2c3017fb4e5c37c4a8c630af62ca"
PINNED_ENGINE_EXPORT_NAME_RVA = 0x36868
IMAGE_BASE_EXPECTED = 0x400000
DATA_DIRECTORY_NAMES = {
    0: "export", 1: "import", 2: "resource", 3: "exception", 4: "security",
    5: "base_relocation", 6: "debug", 7: "architecture", 8: "global_pointer",
    9: "tls", 10: "load_config", 11: "bound_import", 12: "iat",
    13: "delay_import", 14: "clr_runtime_header", 15: "reserved",
}

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def span_hash(pe, rva: int, size: int) -> str:
    return sha(pe.read_rva(rva, size))

def main() -> int:
    parser = argparse.ArgumentParser(description="Read-only startup/import/TLS metadata audit for pinned HERCULES.EXE")
    parser.add_argument("--original-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    root, exe, output = args.original_root.resolve(), args.exe.resolve(), args.output.resolve()
    work = root / "work"
    if work.resolve() != work:
        parser.error("refusing a redirected work directory")
    try:
        output.relative_to(work)
    except ValueError as error:
        parser.error("--output must stay under <original-root>/work")
    if output == exe or output.parent == exe.parent or exe.parent in output.parents:
        parser.error("--output must be outside the original executable directory")
    pe_helper = root / "scripts" / "pe.py"
    if sha(pe_helper.read_bytes()) != PINNED_PE_HELPER_SHA256:
        parser.error("scripts/pe.py hash mismatch")
    raw = exe.read_bytes()
    exe_sha = sha(raw)
    if exe_sha != PINNED_EXE_SHA256:
        parser.error("original executable hash mismatch")
    sys.path.insert(0, str(root / "scripts"))
    import env  # noqa: F401 - adds pinned analysis dependency path
    import capstone
    from pe import PE

    image = PE(exe)
    p = image.pe
    if image.image_base != IMAGE_BASE_EXPECTED or p.FILE_HEADER.Machine != 0x14C or p.OPTIONAL_HEADER.Magic != 0x10B:
        raise RuntimeError("pinned PE identity/layout mismatch")
    directories = []
    for index, entry in enumerate(p.OPTIONAL_HEADER.DATA_DIRECTORY):
        directories.append({"index": index, "name": DATA_DIRECTORY_NAMES.get(index, f"directory_{index}"),
                            "rva": hex(entry.VirtualAddress), "size": hex(entry.Size),
                            "present": bool(entry.VirtualAddress and entry.Size)})
    directory_by_name = {row["name"]: row for row in directories}
    tls = directory_by_name["tls"]
    tls_callbacks = []
    tls_layout = None
    if tls["present"]:
        if not hasattr(p, "DIRECTORY_ENTRY_TLS"):
            raise RuntimeError("TLS directory present but pefile did not parse it")
        t = p.DIRECTORY_ENTRY_TLS.struct
        callback_va = int(t.AddressOfCallBacks)
        tls_layout = {
            "directory_rva": tls["rva"], "directory_size": tls["size"],
            "start_address_of_raw_data": hex(int(t.StartAddressOfRawData)),
            "end_address_of_raw_data": hex(int(t.EndAddressOfRawData)),
            "address_of_index": hex(int(t.AddressOfIndex)),
            "address_of_callbacks": hex(callback_va),
            "size_of_zero_fill": int(t.SizeOfZeroFill),
            "characteristics": hex(int(t.Characteristics)),
        }
        if callback_va:
            callbacks_rva = callback_va - image.image_base
            for index in range(64):
                value = struct.unpack("<I", image.read_rva(callbacks_rva + 4 * index, 4))[0]
                if value == 0:
                    break
                tls_callbacks.append({"index": index, "va": hex(value), "rva": hex(value - image.image_base)})
            else:
                raise RuntimeError("TLS callback array exceeded bounded 64-entry scan")

    metadata = image.metadata()
    imports = metadata["imports"]
    module_names = [row["dll"] for row in imports]
    import_symbols = {row["dll"].lower(): row["symbols"] for row in imports}
    exports = metadata["exports"]
    target_names = {"DirectDrawCreate", "LoadLibraryA", "GetProcAddress", "FreeLibrary"}
    relevant = []
    for module, symbols in import_symbols.items():
        for symbol in symbols:
            if symbol["name"] in target_names:
                relevant.append({"dll": module, "name": symbol["name"],
                                 "iat_rva": hex(symbol["iat_rva"])})

    text = next(section for section in image.sections if section["name"] == ".text")
    text_bytes = image.read_rva(text["virtual_address"], text["raw_size"])
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    md.skipdata = True
    insns = list(md.disasm(text_bytes, IMAGE_BASE_EXPECTED + text["virtual_address"]))
    valid = [i for i in insns if i.id != 0 and i.mnemonic != ".byte"]
    entry_calls = {name: [] for name in ("LoadLibraryA", "GetProcAddress", "FreeLibrary", "DirectDrawCreate")}
    import_iat = {}
    for module, symbols in import_symbols.items():
        for symbol in symbols:
            if symbol["name"] in entry_calls:
                import_iat[symbol["name"]] = IMAGE_BASE_EXPECTED + symbol["iat_rva"]
    for insn in valid:
        if insn.mnemonic != "call":
            continue
        for operand in insn.operands:
            if operand.type != capstone.x86.X86_OP_MEM or operand.mem.base or operand.mem.index:
                continue
            address = operand.mem.disp & 0xFFFFFFFF
            for name, iat_va in import_iat.items():
                if address == iat_va:
                    entry_calls[name].append(hex(insn.address - IMAGE_BASE_EXPECTED))

    engine_export_name = image.read_rva(PINNED_ENGINE_EXPORT_NAME_RVA, 32).split(b"\0", 1)[0].decode("ascii", "replace")
    engine_like_static_modules = [name for name in module_names if "ENGINE" in name.upper() or "PC_DLL" in name.upper()]
    sections = [{"name": row["name"], "rva": hex(row["virtual_address"]),
                 "virtual_size": row["virtual_size"], "raw_size": row["raw_size"]}
                for row in image.sections]
    tls_sections = [row["name"] for row in image.sections if row["name"].lower() == ".tls"]
    manifest_count = sum(bool(row.get("path")) and row["path"][0] == 24 for row in metadata["resources"])
    thunk_rva = 0x10A20
    thunk_va = IMAGE_BASE_EXPECTED + thunk_rva
    thunk_call_sites = []
    for insn in valid:
        if insn.mnemonic == "call" and insn.operands and insn.operands[0].type == capstone.x86.X86_OP_IMM:
            if insn.operands[0].imm == thunk_va:
                thunk_call_sites.append(hex(insn.address - IMAGE_BASE_EXPECTED))
    thunk_code = image.read_rva(thunk_rva, 6)
    managed = directory_by_name["clr_runtime_header"]["present"]
    expected_descriptors = ["DDRAW.dll", "DSOUND.dll", "WINMM.dll", "KERNEL32.dll", "USER32.dll",
                            "GDI32.dll", "WINSPOOL.DRV", "ADVAPI32.dll", "SHELL32.dll", "COMCTL32.dll"]
    if module_names != expected_descriptors:
        raise RuntimeError(f"static import descriptor mismatch: {module_names!r}")
    if not directory_by_name["import"]["present"] or directory_by_name["delay_import"]["present"]:
        raise RuntimeError("expected ordinary imports and no delay-import directory")
    if tls["present"] or tls_callbacks or tls_sections:
        raise RuntimeError("unexpected EXE TLS directory/callback section")
    if managed or exports or p.OPTIONAL_HEADER.AddressOfEntryPoint != 0x11EA0 or manifest_count != 0:
        raise RuntimeError("managed/export/entrypoint/manifest metadata differs from pinned observation")
    if not any(s["name"] == "DirectDrawCreate" for s in import_symbols.get("ddraw.dll", [])):
        raise RuntimeError("pinned DDRAW.dll!DirectDrawCreate static import was not found")
    if engine_export_name != "PC_DLLEngineMain" or thunk_call_sites != ["0x7426"]:
        raise RuntimeError("pinned engine export name or DirectDraw thunk caller mismatch")
    if capstone.__version__ != "5.0.7" or sum(i.size for i in insns) != text["raw_size"]:
        raise RuntimeError("Capstone version or complete skipdata coverage mismatch")
    result = {
        "schema_version": 1,
        "scope": "Read-only PE metadata and bounded full .text import-call candidate inventory. No executable, DLL, game, or callback is run.",
        "audit_script_sha256": sha(Path(__file__).read_bytes()),
        "host": {"path": str(exe), "size": len(raw), "sha256": exe_sha,
                 "machine": hex(p.FILE_HEADER.Machine), "pe_magic": hex(p.OPTIONAL_HEADER.Magic),
                 "image_base": hex(image.image_base), "entry_point_rva": hex(p.OPTIONAL_HEADER.AddressOfEntryPoint),
                 "entry_point_va": hex(image.image_base + p.OPTIONAL_HEADER.AddressOfEntryPoint),
                 "subsystem": int(p.OPTIONAL_HEADER.Subsystem), "characteristics": hex(p.FILE_HEADER.Characteristics),
                 "dll_characteristics": hex(p.OPTIONAL_HEADER.DllCharacteristics),
                 "loader_flags": hex(p.OPTIONAL_HEADER.LoaderFlags), "sections": sections},
        "data_directories": directories,
        "managed_image": {"clr_runtime_header_present": managed,
                          "interpretation": "CLR directory is absent; no managed-image header is declared."},
        "resources": {"rt_manifest_resource_count": manifest_count},
        "startup_callbacks": {"tls_directory_present": tls["present"], "tls_directory": tls_layout,
                              "tls_callbacks": tls_callbacks,
                              "tls_section_names": tls_sections,
                              "exe_owned_tls_callback_count": len(tls_callbacks),
                              "exe_owned_tls_callbacks_observed": bool(tls_callbacks),
                              "interpretation": "A zero TLS directory means this EXE declares no TLS callback array; it does not inventory callbacks in imported DLLs or injected modules."},
        "imports": {"descriptor_names_file_order": module_names,
                    "file_order_is_runtime_load_order": False,
                    "static_ddraw_import_present": "ddraw.dll" in import_symbols,
                    "static_directdrawcreate_present": any(s["name"] == "DirectDrawCreate" for s in import_symbols.get("ddraw.dll", [])),
                    "relevant_imports": relevant,
                    "direct_import_call_candidates": {name: sites for name, sites in entry_calls.items()},
                    "engine_like_static_import_modules": engine_like_static_modules,
                    "all_import_descriptors_sha256": sha(json.dumps(imports, sort_keys=True).encode("utf-8"))},
        "exports": {"count": len(exports), "symbols": exports,
                    "host_exports_pc_dll_engine_main": any(row["name"] == "PC_DLLEngineMain" for row in exports)},
        "engine_resolution": {"engine_export_name_string_rva": hex(PINNED_ENGINE_EXPORT_NAME_RVA),
                              "name": engine_export_name,
                              "resolution_model": "host code imports LoadLibraryA/GetProcAddress and looks up this name; not a static HERCULES.EXE import",
                              "loadlibrarya_call_candidates": entry_calls["LoadLibraryA"],
                              "getprocaddress_call_candidates": entry_calls["GetProcAddress"],
                              "freelibrary_call_candidates": entry_calls["FreeLibrary"]},
        "static_import_lifecycle": {"ddraw_iat_call_candidates": entry_calls["DirectDrawCreate"],
                                    "directdraw_thunk": {"rva": hex(thunk_rva), "size": len(thunk_code),
                                                          "sha256": sha(thunk_code),
                                                          "call_sites": thunk_call_sites,
                                                          "dispatches_via_iat_rva": next((row["iat_rva"] for row in relevant
                                                              if row["dll"] == "ddraw.dll" and row["name"] == "DirectDrawCreate"), None)},
                                    "entry_point_rva": hex(p.OPTIONAL_HEADER.AddressOfEntryPoint),
                                    "app_local_selection_observed_for_host": False,
                                    "conditional_conclusion": "Under ordinary process startup, if the static DDRAW.dll import resolves to the candidate app-local module and that module has a successful DLL_PROCESS_ATTACH, its attach-time publication occurs during loader initialization before host EXE entry code. Therefore host-controlled dispatch through 0x79A0 cannot precede that attach. This conclusion is conditional on actual module selection and normal loader startup; it is not Hercules runtime integration."},
        "deployment_boundary": {"resolution_is_environment_dependent": True,
                                "conditions_not_proved_by_image": ["whether target system treats ddraw.dll as a Known DLL or applies redirection/side-by-side policy", "whether the candidate app-local module is present beside the deployed EXE", "whether its dependencies and required exports resolve", "whether attach returns success"],
                                "pinned_install_sidecar_or_manifest_observed": False},
        "tool": {"pe_helper": str(root / "scripts" / "pe.py"), "pe_helper_sha256": sha((root / "scripts" / "pe.py").read_bytes()),
                 "capstone_version": capstone.__version__, "text_rva": hex(text["virtual_address"]),
                 "text_raw_bytes": text["raw_size"], "skipdata_records": len(insns),
                 "valid_instructions": len(valid), "coverage_bytes": sum(i.size for i in insns),
                 "coverage_complete": sum(i.size for i in insns) == text["raw_size"]},
        "source_links": [
            {"title": "Microsoft Load-Time Dynamic Linking", "url": "https://learn.microsoft.com/en-us/windows/win32/dlls/load-time-dynamic-linking", "used_for": "Ordinary load-time dependency mapping and process-load DLL entry-point invocation; independent of API-set naming."},
            {"title": "Microsoft API set loader operation", "url": "https://learn.microsoft.com/en-us/windows/win32/apiindex/api-set-loader-operation", "used_for": "Microsoft states static imports resolve before code runs; this is applied only to this EXE's ordinary static-import startup."},
            {"title": "Microsoft DllMain entry point", "url": "https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain", "used_for": "DLL_PROCESS_ATTACH for a module loaded at process startup and static-vs-dynamic lpvReserved meaning; DllMain constraints."},
            {"title": "Microsoft PE format", "url": "https://learn.microsoft.com/en-us/windows/win32/debug/pe-format", "used_for": "Data-directory, import, export, and TLS callback metadata interpretation."},
            {"title": "Microsoft DLL search order", "url": "https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order", "used_for": "KnownDLL/redirection/search policy remains deployment-dependent."}
        ],
        "limits": ["A static PE import and no EXE TLS callbacks establish loader-before-host-entry ordering only for the ordinary process-startup path if DDRAW resolves to the candidate module.",
                   "This does not prove the app-local module is selected on any target deployment; KnownDLLs, loaded-module list, SxS/redirection, dependencies, and OS policy remain external.",
                   "Import descriptor order and linear-sweep callsites are metadata/candidates, not observed runtime load or call order.",
                   "This does not prove which callback is the first table copy or validate the real Hercules process; the synthetic early-loader fixture is separate evidence."]
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(output), "exe_sha256": exe_sha,
                      "entry_point_rva": result["host"]["entry_point_rva"],
                      "ddraw_static_import": result["imports"]["static_ddraw_import_present"],
                      "tls_callback_count": len(tls_callbacks), "exports": len(exports),
                      "engine_static_import_modules": engine_like_static_modules,
                      "loadlibrary_sites": entry_calls["LoadLibraryA"],
                      "coverage_complete": result["tool"]["coverage_complete"]}, indent=2))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
