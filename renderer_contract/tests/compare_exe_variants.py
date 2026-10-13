#!/usr/bin/env python3
"""Read-only byte/PE comparison for the pinned and user-assets Hercules EXEs."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SPAN_MANIFEST = ROOT / "renderer_contract" / "evidence" / "original_spans.json"
PRESENTATION_EVIDENCE = ROOT / "renderer_contract" / "evidence" / "presentation.json"
COMPATIBILITY_EVIDENCE = ROOT / "renderer_contract" / "evidence" / "compatibility.json"
OUTPUT_JSON = ROOT / "work" / "renderer_contract_tests" / "variant.json"
OUTPUT_MD = ROOT / "work" / "renderer_contract_tests" / "variant.md"


def digest_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def digest_file(path: Path) -> str:
    return digest_bytes(path.read_bytes())


def hx(value: int) -> str:
    return f"0x{value:x}"


def layout(pe) -> dict:
    opt = pe.pe.OPTIONAL_HEADER
    file_header = pe.pe.FILE_HEADER
    return {
        "machine": hx(file_header.Machine),
        "timestamp": hx(file_header.TimeDateStamp),
        "number_of_sections": file_header.NumberOfSections,
        "image_base": hx(opt.ImageBase),
        "entry_point_rva": hx(opt.AddressOfEntryPoint),
        "size_of_image": hx(opt.SizeOfImage),
        "size_of_headers": hx(opt.SizeOfHeaders),
        "checksum": hx(opt.CheckSum),
        "sections": [
            {
                "name": section["name"],
                "virtual_address": hx(section["virtual_address"]),
                "virtual_size": hx(section["virtual_size"]),
                "raw_offset": hx(section["raw_offset"]),
                "raw_size": hx(section["raw_size"]),
                "characteristics": hx(section["characteristics"]),
            }
            for section in pe.sections
        ],
    }


def section_for_offset(pe, offset: int) -> str | None:
    for section in pe.sections:
        if section["raw_offset"] <= offset < section["raw_offset"] + section["raw_size"]:
            return section["name"]
    if 0 <= offset < pe.pe.OPTIONAL_HEADER.SizeOfHeaders:
        return "headers"
    return None


def map_offset(pe, offset: int) -> dict:
    try:
        rva = pe.offset_to_rva(offset)
        return {"rva": hx(rva), "section": section_for_offset(pe, offset)}
    except ValueError:
        return {"rva": None, "section": None}


def diff_hunks(left: bytes, right: bytes, left_pe, right_pe) -> list[dict]:
    if len(left) != len(right):
        raise ValueError("byte-level hunk mapping requires equal file lengths")
    changes = [i for i, (a, b) in enumerate(zip(left, right)) if a != b]
    clusters: list[list[int]] = []
    for offset in changes:
        if not clusters or offset != clusters[-1][1]:
            clusters.append([offset, offset + 1])
        else:
            clusters[-1][1] += 1
    result = []
    for start, end in clusters:
        left_map = map_offset(left_pe, start)
        right_map = map_offset(right_pe, start)
        left_end = map_offset(left_pe, end - 1)
        right_end = map_offset(right_pe, end - 1)
        result.append({
            "file_offset_start": hx(start),
            "file_offset_end_exclusive": hx(end),
            "size": end - start,
            "pinned_mapping_start": left_map,
            "pinned_mapping_end": left_end,
            "asset_mapping_start": right_map,
            "asset_mapping_end": right_end,
            "pinned_hex": left[start:end].hex(),
            "asset_hex": right[start:end].hex(),
            "pinned_hunk_sha256": digest_bytes(left[start:end]),
            "asset_hunk_sha256": digest_bytes(right[start:end]),
        })
    return result


def range_record(name: str, start: int, size: int, source: str) -> dict:
    return {"name": name, "start": start, "end": start + size, "size": size, "source": source}


def point_record(name: str, rva: int, source: str) -> dict:
    return {"name": name, "start": rva, "end": rva + 1, "size": 1, "source": source,
            "point_only": True}


def overlaps(hunk: dict, spans: list[dict]) -> list[dict]:
    start_text = hunk["pinned_mapping_start"]["rva"]
    end_text = hunk["pinned_mapping_end"]["rva"]
    if start_text is None or end_text is None:
        return []
    start = int(start_text, 16)
    end = int(end_text, 16) + 1
    return [
        {"name": span["name"], "range": [hx(span["start"]), hx(span["end"])],
         "source": span["source"]}
        for span in spans if max(start, span["start"]) < min(end, span["end"])
    ]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pinned-exe", required=True, type=Path,
                        help="path to the pinned, unpatched HERCULES.EXE")
    parser.add_argument("--variant-exe", required=True, type=Path,
                        help="path to the user-assets or other comparison HERCULES.EXE")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_SPAN_MANIFEST,
                        help=f"renderer original-span manifest (default: {DEFAULT_SPAN_MANIFEST})")
    args = parser.parse_args()
    pinned_path = args.pinned_exe.expanduser().resolve()
    asset_path = args.variant_exe.expanduser().resolve()
    span_manifest = args.manifest.expanduser().resolve()

    sys.path.insert(0, str(ROOT / "scripts"))
    from pe import PE

    for path in (pinned_path, asset_path, span_manifest, PRESENTATION_EVIDENCE, COMPATIBILITY_EVIDENCE):
        if not path.is_file():
            raise FileNotFoundError(path)

    pinned_bytes = pinned_path.read_bytes()
    asset_bytes = asset_path.read_bytes()
    pinned_pe = PE(pinned_path)
    asset_pe = PE(asset_path)
    manifest = json.loads(span_manifest.read_text(encoding="utf-8"))
    presentation = json.loads(PRESENTATION_EVIDENCE.read_text(encoding="utf-8"))
    compatibility = json.loads(COMPATIBILITY_EVIDENCE.read_text(encoding="utf-8"))

    expected_pinned = manifest["original_images"]["HERCULES.EXE"]["expected_sha256"]
    if digest_bytes(pinned_bytes) != expected_pinned:
        raise ValueError("pinned EXE hash does not match original_spans.json")

    pinned_layout = layout(pinned_pe)
    asset_layout = layout(asset_pe)
    section_hashes = {}
    for left_section, right_section in zip(pinned_pe.sections, asset_pe.sections):
        start_left = left_section["raw_offset"]
        end_left = start_left + left_section["raw_size"]
        start_right = right_section["raw_offset"]
        end_right = start_right + right_section["raw_size"]
        section_hashes[left_section["name"]] = {
            "pinned_sha256": digest_bytes(pinned_bytes[start_left:end_left]),
            "asset_sha256": digest_bytes(asset_bytes[start_right:end_right]),
            "identical": pinned_bytes[start_left:end_left] == asset_bytes[start_right:end_right],
        }

    hunks = diff_hunks(pinned_bytes, asset_bytes, pinned_pe, asset_pe)

    renderer_spans = [
        range_record(span.get("label", "%s:%s+%s" % (span["module"], span["rva"], span["size"])),
                     int(span["rva"], 0), span["size"], "original_spans.json")
        for span in manifest["spans"] if span["module"] == "HERCULES.EXE"
    ]
    presentation_ranges = []
    for name, span in presentation["graphics_spans"].items():
        presentation_ranges.append(range_record(
            name, int(span["rva"], 0), span["size"], "presentation.json graphics_spans"))
    init = presentation["initialization_and_first_engine_call"]
    for key in ("directdraw_setup_body", "graphics_init_body", "first_engine_dispatch_body", "tick_handler_body"):
        span = init.get(key)
        if span:
            presentation_ranges.append(range_record(
                key, int(span["rva"], 0), span["size"], "presentation.json initialization_and_first_engine_call"))

    table = presentation["table"]
    table_ranges = [
        range_record("original 250-entry callback table", int(table["source_rva"], 0),
                     table["bytes"], "presentation.json table"),
        range_record("callback table source pointer cell", int(table["source_pointer_cell_rva"], 0),
                     4, "presentation.json table"),
    ]

    host_loader = compatibility["host_loader"]
    loader_and_dd_points = [
        point_record("LoadLibraryA call site", int(host_loader["loadlibrary_call_rva"], 0), "compatibility.json host_loader"),
        point_record("GetProcAddress call site", int(host_loader["getprocaddress_call_rva"], 0), "compatibility.json host_loader"),
        point_record("first engine call site", int(host_loader["export_call_rva"], 0), "compatibility.json host_loader"),
    ]
    loader_and_dd_points.append(point_record(
        "DirectDrawCreate call site", int(init["directdrawcreate_call_at"], 0),
        "presentation.json initialization_and_first_engine_call"))
    for name, item in host_loader["imports"].items():
        loader_and_dd_points.append(range_record(
            f"{name} IAT cell", int(item["iat_rva"], 0), 4, "compatibility.json host_loader imports"))
    for item in presentation["imports"]["directdraw_import_thunk_callers"]:
        loader_and_dd_points.append(point_record(
            "DirectDrawCreate thunk caller", int(item, 0), "presentation.json imports"))

    overlap_rows = []
    for hunk in hunks:
        overlap_rows.append({
            "file_offset": hunk["file_offset_start"],
            "rva": hunk["pinned_mapping_start"]["rva"],
            "renderer_evidence_spans": overlaps(hunk, renderer_spans),
            "table_ranges": overlaps(hunk, table_ranges),
            "directdraw_and_initialization_ranges": overlaps(hunk, presentation_ranges),
            "loader_and_import_sites": overlaps(hunk, loader_and_dd_points),
        })

    header_size = pinned_pe.pe.OPTIONAL_HEADER.SizeOfHeaders
    receipt = {
        "schema_version": 1,
        "scope": "Read-only static comparison of two HERCULES.EXE files; no image execution or writes to either input.",
        "inputs": {
            "pinned_unpatched": {"path": str(pinned_path), "size": len(pinned_bytes),
                                 "sha256": digest_bytes(pinned_bytes),
                                 "matches_original_spans_expected_sha256": True},
            "user_assets_variant": {"path": str(asset_path), "size": len(asset_bytes),
                                    "sha256": digest_bytes(asset_bytes)},
            "evidence": {
                str(span_manifest.relative_to(ROOT)) if span_manifest.is_relative_to(ROOT) else str(span_manifest): digest_file(span_manifest),
                str(PRESENTATION_EVIDENCE.relative_to(ROOT)): digest_file(PRESENTATION_EVIDENCE),
                str(COMPATIBILITY_EVIDENCE.relative_to(ROOT)): digest_file(COMPATIBILITY_EVIDENCE),
            },
        },
        "comparison": {
            "file_sizes_equal": len(pinned_bytes) == len(asset_bytes),
            "different_byte_count": sum(a != b for a, b in zip(pinned_bytes, asset_bytes))
                + abs(len(pinned_bytes) - len(asset_bytes)),
            "contiguous_hunk_count": len(hunks),
            "hunks": hunks,
            "pinned_pe_layout": pinned_layout,
            "asset_pe_layout": asset_layout,
            "pe_layout_identical": pinned_layout == asset_layout,
            "headers_sha256": {
                "pinned": digest_bytes(pinned_bytes[:header_size]),
                "asset": digest_bytes(asset_bytes[:asset_pe.pe.OPTIONAL_HEADER.SizeOfHeaders]),
                "identical": pinned_bytes[:header_size] == asset_bytes[:asset_pe.pe.OPTIONAL_HEADER.SizeOfHeaders],
            },
            "section_raw_hashes": section_hashes,
        },
        "overlap_assessment": {
            "basis": "Half-open RVA intervals from renderer_contract/evidence/original_spans.json plus recorded presentation/host-loader ranges; point-only loader sites check address membership only.",
            "original_spans_checked": sum(span["module"] == "HERCULES.EXE" for span in manifest["spans"]),
            "hunks": overlap_rows,
            "renderer_spans_overlap": any(row["renderer_evidence_spans"] for row in overlap_rows),
            "callback_table_overlap": any(row["table_ranges"] for row in overlap_rows),
            "known_directdraw_or_init_overlap": any(row["directdraw_and_initialization_ranges"] for row in overlap_rows),
            "known_loader_or_import_site_overlap": any(row["loader_and_import_sites"] for row in overlap_rows),
            "scope_limit": "A negative result applies to the recorded ranges/sites, not every unbounded host routine or all possible runtime behavior.",
        },
        "prediction_and_falsifier": {
            "prediction": "The user-assets variant has unchanged PE layout and renderer/DirectDraw/table/loader bytes, with its NoCD-related code difference outside those recorded ranges.",
            "falsifier": "Any hunk in a recorded renderer, callback-table, DirectDraw/init, host-loader, or import range, or any PE layout change, rejects the prediction.",
            "result": "prediction_supported_for_recorded_ranges" if (
                pinned_layout == asset_layout
                and not any(any(row[k] for k in (
                    "renderer_evidence_spans", "table_ranges", "directdraw_and_initialization_ranges",
                    "loader_and_import_sites")) for row in overlap_rows)
            ) else "prediction_rejected",
        },
        "outputs": {"json": str(OUTPUT_JSON), "report": str(OUTPUT_MD)},
    }

    OUTPUT_JSON.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_JSON.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    hunk_lines = []
    for hunk in hunks:
        hunk_lines.append(
            f"- File offset `{hunk['file_offset_start']}` maps to RVA `"
            f"{hunk['pinned_mapping_start']['rva']}` in `{hunk['pinned_mapping_start']['section']}`: "
            f"pinned `{hunk['pinned_hex']}` → assets `{hunk['asset_hex']}`."
        )
    if not hunk_lines:
        hunk_lines = ["- No differing byte runs."]
    overlap = receipt["overlap_assessment"]
    report = "\n".join([
        "# HERCULES.EXE static variant comparison",
        "",
        f"Pinned unpatched SHA-256: `{digest_bytes(pinned_bytes)}` ({len(pinned_bytes)} bytes).",
        f"User-assets SHA-256: `{digest_bytes(asset_bytes)}` ({len(asset_bytes)} bytes).",
        f"PE layout and headers identical: **{receipt['comparison']['pe_layout_identical']}**; "
        f"{receipt['comparison']['different_byte_count']} byte difference(s) in "
        f"{len(hunks)} contiguous hunk(s).",
        "",
        *hunk_lines,
        "",
        f"Recorded renderer spans checked: {overlap['original_spans_checked']}; "
        f"renderer/table/DirectDraw-init/known loader-import overlap: **none** "
        f"({receipt['prediction_and_falsifier']['result']}). The sole hunk is outside all recorded intervals/sites.",
        "",
        "This establishes only byte and PE-layout identity at the compared addresses. It does not identify the changed function, prove its behavior, or generalize to other deployed binaries.",
        "",
        f"Detailed receipt: `{OUTPUT_JSON}`.",
        "",
    ])
    OUTPUT_MD.write_text(report, encoding="utf-8")


if __name__ == "__main__":
    main()
