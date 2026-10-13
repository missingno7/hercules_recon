#!/usr/bin/env python3
"""Rerun the bounded screen-writer inventory against four hash-pinned images.

Read-only PE inspection. The exact-cell slot-149 search is a targeted decoded
candidate scan, not a reachability proof or a closed-writer claim.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import struct
import sys
from pathlib import Path


EVIDENCE_REL = Path("renderer_contract/evidence/screen_ownership.json")
EXPECTED_NAMES = ("HERCULES.EXE", "ENG1.DLL", "ENG3.DLL", "TITLE.DLL")
IMPORTS_OF_INTEREST = {
    "TextOutA", "ExtTextOutA", "DrawTextA", "TabbedTextOutA", "GrayStringA",
    "StretchDIBits", "GdiFlush", "FillRect", "BitBlt", "PatBlt",
    "SetPaletteEntries", "SelectPalette", "RealizePalette", "CreatePalette",
    "CreateDIBSection", "GetDC", "GetWindowDC", "ReleaseDC", "GetClientRect",
    "BeginPaint", "EndPaint", "InvalidateRect", "ValidateRect", "UpdateWindow",
    "ExcludeUpdateRgn", "ScrollWindow", "GetMessageA", "PeekMessageA",
    "TranslateMessage", "DispatchMessageA", "SendMessageA", "PostMessageA",
    "CallWindowProcA", "mciSendStringA",
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def norm(path: Path) -> str:
    return os.path.normcase(str(path))


def is_within(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def related(path: Path, other: Path) -> bool:
    return is_within(path, other) or is_within(other, path)


def exact_local_path(path: Path, label: str, must_exist: bool) -> Path:
    if not path.is_absolute():
        raise SystemExit(f"{label} must be an explicit absolute path: {path}")
    lexical = Path(os.path.abspath(str(path)))
    try:
        # strict=True can fail on managed Windows workspaces even for readable
        # files. strict=False still resolves existing junction/symlink parents;
        # existence is checked separately when the caller requires it.
        resolved = path.resolve(strict=False)
    except OSError as exc:
        raise SystemExit(f"cannot resolve {label}: {exc}") from exc
    if must_exist and not path.exists():
        raise SystemExit(f"{label} does not exist: {path}")
    if norm(lexical) != norm(resolved):
        raise SystemExit(f"{label} redirects through a symlink/junction: {lexical} -> {resolved}")
    return lexical


def parse_args() -> argparse.Namespace:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo-root", type=Path, required=True)
    ap.add_argument("--exe", type=Path, required=True)
    ap.add_argument("--eng1", type=Path, required=True)
    ap.add_argument("--eng3", type=Path, required=True)
    ap.add_argument("--title", type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    ap.add_argument("--analysis-deps", type=Path, required=True,
                    help="existing analysis dependency directory, normally C:/tools/hercules/python")
    args = ap.parse_args()

    args.repo_root = exact_local_path(args.repo_root, "--repo-root", True)
    args.work = exact_local_path(args.repo_root / "work", "repository work directory", True)
    if not args.work.is_dir():
        ap.error("repository work path is not a directory")
    args.assets = Path(os.path.abspath(str(args.repo_root / "assets")))
    args.originals = {}
    for key, supplied in zip(EXPECTED_NAMES, (args.exe, args.eng1, args.eng3, args.title)):
        path = exact_local_path(supplied, f"--{key.lower()}", True)
        if not path.is_file() or path.name.lower() != key.lower():
            ap.error(f"expected original file named {key}: {path}")
        args.originals[key] = path
    original_dirs = {path.parent for path in args.originals.values()}
    if len(original_dirs) != 1:
        ap.error("all four explicit original paths must share one directory")
    args.original_dir = next(iter(original_dirs))
    expected_original_dir = Path(os.path.abspath(str(args.repo_root.parent / "discs" / "pc_install")))
    if norm(args.original_dir) != norm(expected_original_dir):
        ap.error("originals must be the explicit pc_install directory adjacent to this worktree")
    exact_local_path(args.original_dir, "original-image directory", True)

    output_lexical = exact_local_path(args.output, "--output", False)
    if not output_lexical.parent.is_dir():
        ap.error("--output parent must already exist")
    output_parent = exact_local_path(output_lexical.parent, "--output parent", True)
    args.output = output_parent / output_lexical.name
    output_resolved = args.output.resolve(strict=False)
    if norm(output_resolved) != norm(args.output):
        ap.error("--output redirects through a symlink/junction")
    if not is_within(args.output, args.work) or not is_within(output_resolved, args.work.resolve()):
        ap.error("--output must remain under repository work in lexical and resolved paths")
    if related(args.output, args.original_dir) or related(output_resolved, args.original_dir.resolve()):
        ap.error("--output may not overlap the original-image directory")
    if related(args.output, args.assets) or related(output_resolved, args.assets.resolve(strict=False)):
        ap.error("--output may not overlap assets")
    if any(norm(args.output) == norm(p) for p in args.originals.values()):
        ap.error("--output may not overwrite an original image")

    args.analysis_deps = exact_local_path(args.analysis_deps, "--analysis-deps", True)
    if not args.analysis_deps.is_dir():
        ap.error("--analysis-deps must be an existing directory")
    sys.path.insert(0, str(args.analysis_deps))
    sys.path.insert(0, str(args.repo_root / "scripts"))
    return args


def text_section(pe):
    section = next((s for s in pe.sections if s["name"] == ".text"), None)
    if section is None:
        raise ValueError("missing .text section")
    return section["virtual_address"], pe.read_rva(section["virtual_address"], section["raw_size"])


def raw_e8_candidates(pe, target_rvas: set[int]) -> dict[str, list[str]]:
    found = {target: [] for target in target_rvas}
    start, raw = text_section(pe)
    for off in range(max(0, len(raw) - 4)):
        if raw[off] != 0xE8:
            continue
        target = start + off + 5 + struct.unpack_from("<i", raw, off + 1)[0]
        if target in found:
            found[target].append(start + off)
    return {f"0x{k:x}": [f"0x{x:x}" for x in sorted(v)] for k, v in sorted(found.items())}


def direct_iat_candidates(pe) -> dict[str, list[str]]:
    start, raw = text_section(pe)
    result = {}
    for module in getattr(pe.pe, "DIRECTORY_ENTRY_IMPORT", []):
        dll = module.dll.decode("ascii", "replace") if isinstance(module.dll, bytes) else str(module.dll)
        for imp in module.imports:
            if not imp.name:
                continue
            name = imp.name.decode("ascii", "replace")
            if name not in IMPORTS_OF_INTEREST:
                continue
            needle = b"\xff\x15" + struct.pack("<I", imp.address)
            sites = []
            pos = 0
            while True:
                at = raw.find(needle, pos)
                if at < 0:
                    break
                sites.append(start + at)
                pos = at + 1
            if sites:
                result[f"{dll}!{name}"] = [f"0x{x:x}" for x in sites]
    return dict(sorted(result.items()))


def slot149_cell_candidates(pe, cell_va: int, capstone_module, x86_mem_type: int) -> list[dict]:
    start, raw = text_section(pe)
    md = capstone_module.Cs(capstone_module.CS_ARCH_X86, capstone_module.CS_MODE_32)
    md.detail = True
    md.skipdata = True
    hits = []
    for ins in md.disasm(raw, pe.image_base + start):
        if not ins.id or ins.mnemonic not in ("call", "jmp"):
            continue
        for op in ins.operands:
            if op.type != x86_mem_type:
                continue
            if op.mem.base or op.mem.index:
                continue
            if (op.mem.disp & 0xFFFFFFFF) == cell_va:
                hits.append({"rva": f"0x{ins.address - pe.image_base:x}",
                             "mnemonic": ins.mnemonic, "operands": ins.op_str})
    return hits


def bounded_mci_calls(pe, capstone_module) -> dict:
    ranges = ((0x9C30, 0xA0), (0x9E60, 0xF3), (0x9F60, 0x143), (0xA120, 0x20))
    md = capstone_module.Cs(capstone_module.CS_ARCH_X86, capstone_module.CS_MODE_32)
    calls = []
    other_register_calls = []
    iat_loads = []
    for start, size in ranges:
        code = pe.read_rva(start, size)
        for ins in md.disasm(code, pe.image_base + start):
            rva = ins.address - pe.image_base
            if ins.mnemonic == "mov" and "0xba5b14" in ins.op_str.lower():
                iat_loads.append(f"0x{rva:x}")
            if ins.mnemonic == "call" and ins.op_str.lower() in {"esi", "ebp"}:
                row = f"0x{rva:x}"
                other_register_calls.append(row)
                if row != "0x9f27":
                    calls.append(row)
    return {"indirect_register_calls": calls,
            "other_indirect_register_calls": [x for x in other_register_calls if x not in calls],
            "iat_loads": iat_loads,
            "code_spans": [{"rva": f"0x{start:x}", "size": size,
                            "sha256": sha(pe.read_rva(start, size))}
                           for start, size in ranges]}


def main() -> int:
    args = parse_args()
    from pe import PE
    import capstone
    from capstone.x86 import X86_OP_MEM

    evidence_path = exact_local_path(args.repo_root / EVIDENCE_REL, "screen ownership evidence", True)
    evidence = json.loads(evidence_path.read_text(encoding="utf-8-sig"))
    if evidence.get("schema") != "hercules-screen-ownership-evidence-v1":
        raise SystemExit("unexpected screen ownership evidence schema")
    prov = evidence["provenance"]
    expected_hashes = prov["original_sha256"]
    images = {}
    for name, path in args.originals.items():
        data = path.read_bytes()
        actual = sha(data)
        if actual != expected_hashes[name]:
            raise SystemExit(f"original hash mismatch for {name}: {actual}")
        images[name] = PE(path)

    pinned_inputs = prov["pinned_audit_inputs"]
    pinned_actual = {}
    for rel, expected in pinned_inputs.items():
        path = exact_local_path(args.repo_root / Path(rel), f"pinned evidence {rel}", True)
        if not is_within(path, args.repo_root) or not is_within(path.resolve(strict=False), args.repo_root.resolve()):
            raise SystemExit(f"pinned evidence escapes repository: {rel}")
        if related(path, args.assets) or related(path.resolve(strict=False), args.assets.resolve(strict=False)):
            raise SystemExit(f"pinned evidence may not overlap assets: {rel}")
        if related(path, args.original_dir) or related(path.resolve(strict=False), args.original_dir.resolve()):
            raise SystemExit(f"pinned evidence may not overlap originals: {rel}")
        actual = sha(path.read_bytes())
        if actual != expected:
            raise SystemExit(f"pinned evidence hash mismatch: {rel}")
        pinned_actual[rel] = actual

    host = images["HERCULES.EXE"]
    table_expected = prov["callback_table"]
    table_bytes = host.read_rva(int(table_expected["rva"], 16), table_expected["size"])
    if sha(table_bytes) != table_expected["sha256"]:
        raise SystemExit("host callback table hash mismatch")
    cell_results = {}
    for slot_text, cell in table_expected["selected_cells"].items():
        slot = int(slot_text)
        cell_rva = int(cell["cell_rva"], 16)
        target = struct.unpack("<I", host.read_rva(cell_rva, 4))[0]
        if target != int(cell["target_va"], 16):
            raise SystemExit(f"host callback slot {slot} target mismatch")
        cell_results[slot_text] = {"cell_rva": cell["cell_rva"], "target_va": f"0x{target:08x}"}

    span_results = {}
    for rva_text, expected in prov["host_spans"].items():
        rva = int(rva_text, 16)
        actual = sha(host.read_rva(rva, expected["size"]))
        if actual != expected["sha256"]:
            raise SystemExit(f"host span hash mismatch at {rva_text}")
        span_results[rva_text] = {"size": expected["size"], "sha256": actual}

    e8_expected = prov["raw_direct_e8_call_candidates"]
    targets = {int(k, 16) for k in e8_expected}
    e8_actual = raw_e8_candidates(host, targets)
    normalized_e8 = {k.lower(): [x.lower() for x in v] for k, v in e8_expected.items()}
    if e8_actual != normalized_e8:
        raise SystemExit("raw direct E8 call-candidate inventory changed")

    iat_all_actual = direct_iat_candidates(host)
    mci_rows = {k: v for k, v in iat_all_actual.items() if k.lower().endswith("!mcisendstringa")}
    iat_actual = {k: v for k, v in iat_all_actual.items() if k not in mci_rows}
    iat_expected = prov["window_paint_message_import_calls"]
    normalized_iat = {k: [x.lower() for x in v] for k, v in iat_expected.items()}
    if iat_actual != normalized_iat:
        raise SystemExit("targeted direct imported-call candidate inventory changed")
    mci = prov["mci_external_calls"]
    if len(mci_rows) != 1 or next(iter(mci_rows.values())) != [x.lower() for x in mci["direct_call_rvas"]]:
        raise SystemExit("bounded MCI direct IAT candidate sites changed")
    for rva_text, literal in mci["command_literals"].items():
        expected_literal = literal.encode("ascii") + b"\0"
        if host.read_rva(int(rva_text, 16), len(expected_literal)) != expected_literal:
            raise SystemExit(f"bounded MCI command literal changed at {rva_text}")
    mci_routes = bounded_mci_calls(host, capstone)
    if mci_routes["indirect_register_calls"] != mci["indirect_register_call_rvas_in_bounded_audio_spans"]:
        raise SystemExit("bounded indirect MCI call sites changed")
    if mci_routes["other_indirect_register_calls"] != mci["other_indirect_register_calls_in_these_audio_spans"]:
        raise SystemExit("other bounded register-call candidates changed")
    if mci_routes["iat_loads"] != mci["iat_load_rvas_in_bounded_audio_spans"]:
        raise SystemExit("bounded MCI IAT load sites changed")
    if mci_routes["code_spans"] != mci["bounded_audio_code_spans"]:
        raise SystemExit("bounded MCI code-span hashes changed")

    cross_rel = prov["cross_module_table_copy"]["source"]
    cross_path = exact_local_path(args.repo_root / cross_rel, "cross-module evidence", True)
    if not is_within(cross_path, args.repo_root):
        raise SystemExit("cross-module evidence escapes repository")
    cross = json.loads(cross_path.read_text(encoding="utf-8-sig"))
    scan_expected = prov["cross_module_table_copy"]["slot149_candidate_scan"]
    scan_results = {}
    for name in ("ENG1.DLL", "ENG3.DLL", "TITLE.DLL"):
        module = cross["modules"][name]
        scan_row = scan_expected["results"][name]
        if module["sha256"] != expected_hashes[name]:
            raise SystemExit(f"cross-module receipt hash mismatch for {name}")
        copy = module["copy_and_validation"]
        if copy["copy_bytes"] != 1000 or copy["signature"] != "0x5ac00cac":
            raise SystemExit(f"unexpected copied callback table metadata for {name}")
        base = int(module["image_base"], 16) + int(copy["copy_destination_rva"], 16)
        cell_va = base + 149 * 4
        if f"0x{cell_va:08x}" != scan_row["slot149_cell_va"]:
            raise SystemExit(f"slot-149 copied-table cell changed for {name}")
        prior = [x for x in module["direct_table_dispatches"] if x.get("slot") == 149]
        if prior != scan_row["prior_selected_table_dispatches"]:
            raise SystemExit(f"prior selected slot-149 dispatch receipt changed for {name}")
        pe = images[name]
        candidates = slot149_cell_candidates(pe, cell_va, capstone, X86_OP_MEM)
        if candidates != scan_row["decoded_absolute_call_or_jump_cell_candidates"]:
            raise SystemExit(f"targeted slot-149 exact-cell candidate scan changed for {name}")
        scan_results[name] = {"slot149_cell_va": f"0x{cell_va:08x}",
                              "prior_selected_table_dispatches": prior,
                              "decoded_absolute_call_or_jump_cell_candidates": candidates}

    report = {
        "schema": "hercules-screen-ownership-audit-result-v1",
        "status": "PASS",
        "scope": "Pinned PE spans/table bindings, raw direct-call and imported-call candidates, and targeted exact copied-cell call/jmp scan. No runtime or reachability proof.",
        "original_sha256": {name: expected_hashes[name] for name in EXPECTED_NAMES},
        "pinned_evidence_sha256": pinned_actual,
        "host_callback_table_sha256": table_expected["sha256"],
        "selected_cells": cell_results,
        "verified_host_spans": span_results,
        "raw_direct_e8_call_candidate_targets": len(e8_actual),
        "verified_targeted_iat_groups": len(iat_actual),
        "verified_mci_direct_iat_sites": next(iter(mci_rows.values())),
        "verified_mci_bounded_routes": mci_routes,
        "slot149_candidate_scan": {
            "criterion": scan_expected["criterion"],
            "results": scan_results,
            "limits": scan_expected["limits"],
        },
        "tool_provenance": {
            "python": sys.version.split()[0],
            "pefile": __import__("pefile").__version__,
            "capstone": capstone.__version__,
            "pe_helper": str(args.repo_root / "scripts" / "pe.py"),
            "analysis_dependency_directory": str(args.analysis_deps),
            "audit_script_sha256": sha(Path(__file__).read_bytes()),
        },
    }
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": report["status"], "output": str(args.output),
                      "verified_host_spans": len(span_results),
                      "callback_cells": len(cell_results),
                      "slot149_exact_cell_candidates": {
                          k: len(v["decoded_absolute_call_or_jump_cell_candidates"])
                          for k, v in scan_results.items()}}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
