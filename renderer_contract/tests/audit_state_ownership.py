"""Reproducible static ownership audit for the public renderer seam."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
from collections import Counter
from pathlib import Path


IMAGE_SHA256 = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "ENG3.DLL": "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
    "TITLE.DLL": "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
}

PUBLIC_EVIDENCE_SHA256 = {
    "closure.json": "2142cb1d0c7a49c9fe9ebd3bd186099583431755205efc3022d9a29a7cab9e30",
    "compatibility.json": "82b8b183ad54d629fef5bd977ca8ac579c3708af36879b18c8536303a983c61d",
    "frame.json": "b81f7e1f86bb8b3976ed666261d741000b2e7e400d35d18be6ce0068ce69bb5b",
    "presentation.json": "c98085c9f39b8c532817d90f5617067517a1157a8ede56042d778fde6c2d829a",
    "textures.json": "8743c94b086765d4d4dff50633b31d953506b2dc0184de10b8a357bbd399767a",
    "native_dispatch.json": "966617e243816275b70e7080396a75c69a09fd1afc26c01ed001a464c10b9151",
}
RECOVERY_SHA256 = "d6858a378f8360ed713b6786e7dc14c09d728d12afa8b759afd0544c01efd597"

TARGETS = {
    "sprite_destination_pointer_A90944": (0xA90944, 0xA90948),
    "blend_table_pointer_A90948": (0xA90948, 0xA9094C),
    "polygon_workspace_A909B0_A90A14": (0xA909B0, 0xA90A14),
    "page_index_A8E6F4": (0xA8E6F4, 0xA8E6F8),
    "blend_selector_A8E6F8": (0xA8E6F8, 0xA8E6FC),
    "alternate_page_flag_A8E6FC": (0xA8E6FC, 0xA8E700),
    "draw_gate_BA01FC": (0xBA01FC, 0xBA01FD),
    "lock_state_BA01F0": (0xBA01F0, 0xBA01F1),
    "surface_pointer_BA0210": (0xBA0210, 0xBA0214),
    "surface_pitch_BA0214": (0xBA0214, 0xBA0218),
    "f3_right_output_43A488": (0x43A488, 0x43A48C),
    "f3_bottom_output_43A48C": (0x43A48C, 0x43A490),
    "frame_dirty_flag_43A590": (0x43A590, 0x43A591),
    "color_change_flag_96E698": (0x96E698, 0x96E699),
    "color_rgb_96E699_96E69C": (0x96E699, 0x96E69C),
}
PAGE_BASES = {0x96E6E0, 0xA6E6E0}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def repository_root(path: Path) -> Path:
    for candidate in (path.resolve(), *path.resolve().parents):
        if ((candidate / "scripts" / "pe.py").is_file() and
                (candidate / "renderer_contract" / "tests" / "oracle_common.py").is_file() and
                (candidate / "renderer_contract" / "evidence" / "closure.json").is_file()):
            return candidate
    raise RuntimeError("--original-root is not a checkout with public renderer evidence")


def find_original(root: Path, suffix: str, override: Path | None) -> Path:
    if override is not None:
        result = override.resolve()
        if not result.is_file():
            raise FileNotFoundError(result)
        return result
    for ancestor in (root, *root.parents):
        candidate = ancestor / Path(suffix)
        if candidate.is_file():
            return candidate.resolve()
    raise FileNotFoundError(
        f"could not locate {suffix}; pass its explicit --{Path(suffix).stem.lower()} path"
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-root", required=True, type=Path,
                        help="checkout containing scripts/pe.py and renderer_contract/")
    parser.add_argument("--output", required=True, type=Path,
                        help="destination for the compact JSON report")
    parser.add_argument("--exe", type=Path, help="override the pinned original HERCULES.EXE path")
    parser.add_argument("--eng1", type=Path, help="override the pinned original ENG1.DLL path")
    parser.add_argument("--eng3", type=Path, help="override the pinned original ENG3.DLL path")
    parser.add_argument("--title", type=Path, help="override the pinned original TITLE.DLL path")
    return parser.parse_args()


def access_kind(insn, operand, capstone) -> str:
    if insn.mnemonic.lower() == "lea":
        return "address_materialization"
    flags = operand.access
    reads = bool(flags & capstone.CS_AC_READ)
    writes = bool(flags & capstone.CS_AC_WRITE)
    if reads and writes:
        return "read_write"
    if writes:
        return "write"
    if reads:
        return "read"
    return "unknown_access"


def summarized_sites(rows: list[dict]) -> dict:
    counts = Counter((row["access"], row.get("width", 0)) for row in rows)
    return {
        "count": len(rows),
        "by_access_width": [
            {"access": access, "width": width, "count": count}
            for (access, width), count in sorted(counts.items())
        ],
    }


def merge_intervals(intervals: list[tuple[int, int]]) -> list[tuple[int, int]]:
    merged: list[tuple[int, int]] = []
    for start, end in sorted(intervals):
        if not merged or start > merged[-1][1]:
            merged.append((start, end))
        else:
            merged[-1] = (merged[-1][0], max(merged[-1][1], end))
    return merged


def complement_intervals(start: int, end: int, intervals: list[tuple[int, int]]) -> list[tuple[int, int]]:
    gaps = []
    cursor = start
    for lo, hi in merge_intervals(intervals):
        lo, hi = max(lo, start), min(hi, end)
        if lo > cursor:
            gaps.append((cursor, lo))
        cursor = max(cursor, hi)
    if cursor < end:
        gaps.append((cursor, end))
    return gaps


def scan_image(name: str, path: Path, expected_sha: str, pe_type, capstone, x86,
               dependencies: list[dict], recovery_functions: list[dict]) -> dict:
    data = path.read_bytes()
    digest = sha(data)
    if digest != expected_sha:
        raise AssertionError(f"{name} SHA-256 {digest} != pinned original {expected_sha}")
    pe = pe_type(str(path))
    def owner(rva: int):
        matches = [dep for dep in dependencies
                   if dep.get("module", "HERCULES.EXE") == name and dep["start"] <= rva < dep["end"]]
        return min(matches, key=lambda dep: dep["end"] - dep["start"]) if matches else None

    source_names = ("section_linear_sweep", "recovery_function_inventory",
                    "bounded_renderer_closure")
    hits = {source: {key: {bucket: [] for bucket in ("inside", "outside", "module")}
                     for key in TARGETS} for source in source_names}
    address_immediates = {source: {key: [] for key in TARGETS} for source in source_names}
    page_base_immediates = {source: [] for source in source_names}
    sections = []
    strict_sections = []

    def scan_stream(code: bytes, start_rva: int, source: str,
                    function: dict | None = None) -> dict:
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        md.detail = True
        md.skipdata = True
        decoded_bytes = 0
        skipped_bytes = 0
        pseudoop_count = 0
        instruction_count = 0
        end_rva = start_rva
        decode_gaps = []
        gap_start = None
        gap_end = None
        base = pe.image_base + start_rva
        for insn in md.disasm(code, base):
            rva = insn.address - pe.image_base
            insn_end = rva + insn.size
            end_rva = insn_end
            if insn.id == 0 or insn.mnemonic.startswith("."):
                skipped_bytes += insn.size
                pseudoop_count += 1
                if gap_start is None:
                    gap_start = rva
                gap_end = insn_end
                continue
            if gap_start is not None:
                decode_gaps.append({"start_rva": hex(gap_start), "end_rva_exclusive": hex(gap_end),
                                    "size": gap_end - gap_start})
                gap_start = None
            decoded_bytes += insn.size
            instruction_count += 1
            dependency = owner(rva) if name == "HERCULES.EXE" else None
            provenance = {"scan_source": source}
            if function is not None:
                provenance.update({"function_id": function["id"],
                                   "function_status": function["status"],
                                   "function_start_rva": hex(function["start"]),
                                   "function_size": function["size"]})
            for operand in insn.operands:
                if operand.type == x86.X86_OP_MEM:
                    mem = operand.mem
                    if mem.base == 0 and mem.index == 0 and mem.segment == 0:
                        address = mem.disp & 0xFFFFFFFF
                        for key, (lo, hi) in TARGETS.items():
                            if lo <= address < hi:
                                row = {"site_rva": hex(rva), "address": hex(address),
                                       "width": operand.size,
                                       "access": access_kind(insn, operand, capstone),
                                       "instruction": f"{insn.mnemonic} {insn.op_str}",
                                       **provenance}
                                if name != "HERCULES.EXE":
                                    hits[source][key]["module"].append(row)
                                elif dependency is None:
                                    hits[source][key]["outside"].append(row)
                                else:
                                    row["dependency"] = dependency["name"]
                                    row["dependency_classification"] = dependency["classification"]
                                    hits[source][key]["inside"].append(row)
                elif operand.type == x86.X86_OP_IMM:
                    value = operand.imm & 0xFFFFFFFF
                    if value in PAGE_BASES:
                        page_base_immediates[source].append({
                            "site_rva": hex(rva), "value": hex(value),
                            "instruction": f"{insn.mnemonic} {insn.op_str}", **provenance})
                    for key, (lo, hi) in TARGETS.items():
                        if lo <= value < hi:
                            address_immediates[source][key].append({
                                "site_rva": hex(rva), "address": hex(value),
                                "access": "address_immediate_candidate",
                                "instruction": f"{insn.mnemonic} {insn.op_str}", **provenance})
        if gap_start is not None:
            decode_gaps.append({"start_rva": hex(gap_start), "end_rva_exclusive": hex(gap_end),
                                "size": gap_end - gap_start})
        return {
            "start_rva": hex(start_rva), "end_rva_exclusive": hex(end_rva),
            "expected_bytes": len(code), "decoded_instruction_count": instruction_count,
            "decoded_bytes": decoded_bytes, "pseudoop_count": pseudoop_count,
            "skipped_bytes": skipped_bytes,
            "byte_coverage_complete": decoded_bytes + skipped_bytes == len(code) and end_rva == start_rva + len(code),
            "decode_gaps": decode_gaps,
        }

    for section in pe.sections:
        if not section["raw_size"] or not (section["characteristics"] & 0x20000000):
            continue
        section_rva = section["virtual_address"]
        code = pe.read_rva(section_rva, section["raw_size"])
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        strict_bytes = 0
        strict_instruction_count = 0
        for insn in md.disasm(code, pe.image_base + section_rva):
            strict_instruction_count += 1
            strict_bytes += insn.size
        strict_stop = section_rva + strict_bytes
        strict_sections.append({
            "name": section["name"], "start_rva": hex(section_rva),
            "end_rva_exclusive": hex(section_rva + section["raw_size"]),
            "strict_decoded_instruction_count": strict_instruction_count,
            "strict_decoded_bytes": strict_bytes,
            "first_strict_decode_stop_rva": (hex(strict_stop)
                                               if strict_bytes < len(code) else None),
        })

        linear = scan_stream(code, section_rva, "section_linear_sweep")
        if not linear["byte_coverage_complete"]:
            raise AssertionError(f"linear sweep failed to cover {section['name']} in {name}")
        sections.append({
            "name": section["name"], "rva": hex(section_rva),
            "size": section["raw_size"], "sha256": sha(code),
            "section_linear_sweep": linear,
        })

        matching = []
        for row in recovery_functions:
            target_leaf = row.get("target", "").replace("\\", "/").split("/")[-1].upper()
            if target_leaf != name or row.get("target_sha256") != expected_sha:
                continue
            if not row.get("rva") or not row.get("size"):
                continue
            start = int(row["rva"], 0)
            size = int(row["size"])
            if section_rva <= start and start + size <= section_rva + section["raw_size"]:
                matching.append({"id": row["id"], "status": row.get("status", "unknown"),
                                 "start": start, "size": size})
        recovery_anchors = [(row["start"], row["start"] + row["size"]) for row in matching]
        overlap_count = 0
        ordered = sorted(matching, key=lambda item: item["start"])
        for left, right in zip(ordered, ordered[1:]):
            if left["start"] + left["size"] > right["start"]:
                overlap_count += 1
        recovery_scan_stats = {"span_count": len(matching), "input_bytes": 0,
                               "decoded_instruction_count": 0, "decoded_bytes": 0,
                               "skipped_bytes": 0, "pseudoop_count": 0,
                               "decode_gaps": [],
                               "status_counts": dict(Counter(row["status"] for row in matching))}
        for function in matching:
            function_code = pe.read_rva(function["start"], function["size"])
            function_scan = scan_stream(function_code, function["start"],
                                        "recovery_function_inventory", function)
            if not function_scan["byte_coverage_complete"]:
                raise AssertionError(f"recovery span scan incomplete: {function['id']}")
            for key in ("expected_bytes", "decoded_instruction_count", "decoded_bytes",
                        "skipped_bytes", "pseudoop_count"):
                stat_key = "input_bytes" if key == "expected_bytes" else key
                recovery_scan_stats[stat_key] += function_scan[key]
            recovery_scan_stats["decode_gaps"].extend(
                [{**gap, "function_id": function["id"], "function_status": function["status"]}
                 for gap in function_scan["decode_gaps"]])

        closure_matching = []
        for dependency in dependencies:
            if dependency.get("module", "HERCULES.EXE") != name:
                continue
            start, end = dependency["start"], dependency["end"]
            if not (section_rva <= start and end <= section_rva + section["raw_size"]):
                continue
            dependency_code = pe.read_rva(start, end - start)
            if sha(dependency_code) != dependency["sha256"]:
                raise AssertionError(f"bounded span hash changed: {dependency['name']} at {start:#x}")
            closure_matching.append({"id": dependency["name"],
                                     "status": dependency["classification"],
                                     "start": start, "size": end - start})
        closure_anchors = [(row["start"], row["start"] + row["size"]) for row in closure_matching]
        closure_scan_stats = {"span_count": len(closure_matching), "input_bytes": 0,
                              "decoded_instruction_count": 0, "decoded_bytes": 0,
                              "skipped_bytes": 0, "pseudoop_count": 0,
                              "decode_gaps": [],
                              "classification_counts": dict(Counter(row["status"] for row in closure_matching))}
        for function in closure_matching:
            function_code = pe.read_rva(function["start"], function["size"])
            function_scan = scan_stream(function_code, function["start"],
                                        "bounded_renderer_closure", function)
            if not function_scan["byte_coverage_complete"]:
                raise AssertionError(f"closure span scan incomplete: {function['id']}")
            for key in ("expected_bytes", "decoded_instruction_count", "decoded_bytes",
                        "skipped_bytes", "pseudoop_count"):
                stat_key = "input_bytes" if key == "expected_bytes" else key
                closure_scan_stats[stat_key] += function_scan[key]
            closure_scan_stats["decode_gaps"].extend(
                [{**gap, "function_id": function["id"], "classification": function["status"]}
                 for gap in function_scan["decode_gaps"]])

        covered = merge_intervals(recovery_anchors + closure_anchors)
        unanchored = complement_intervals(section_rva, section_rva + section["raw_size"], covered)
        sections[-1]["known_code_anchors"] = {
            "recovery_function_inventory": {
                "source": "recovery.json",
                **recovery_scan_stats,
                "span_overlap_count": overlap_count,
                "anchored_bytes": sum(end - start for start, end in merge_intervals(recovery_anchors)),
                "byte_coverage_complete": recovery_scan_stats["decoded_bytes"] +
                                           recovery_scan_stats["skipped_bytes"] ==
                                           recovery_scan_stats["input_bytes"],
            },
            "bounded_renderer_closure": {
                "source": "renderer_contract/evidence/closure.json",
                **closure_scan_stats,
                "anchored_bytes": sum(end - start for start, end in merge_intervals(closure_anchors)),
                "byte_coverage_complete": closure_scan_stats["decoded_bytes"] +
                                           closure_scan_stats["skipped_bytes"] ==
                                           closure_scan_stats["input_bytes"],
            },
            "combined_anchored_bytes": sum(end - start for start, end in covered),
            "unanchored_bytes": sum(end - start for start, end in unanchored),
            "unanchored_ranges": [{"start_rva": hex(start), "end_rva_exclusive": hex(end),
                                   "size": end - start} for start, end in unanchored],
        }

    result_targets = {}
    for key in TARGETS:
        result_targets[key] = {}
        for source in source_names:
            groups = hits[source][key]
            internal_counts = Counter((row["dependency"], row["dependency_classification"], row["access"])
                                      for row in groups["inside"])
            result_targets[key][source] = {
                "direct_absolute_memory": {
                    "inside_bounded_closure": [
                        {"dependency": dep, "classification": classification,
                         "access": access, "count": count}
                        for (dep, classification, access), count in sorted(internal_counts.items())
                    ],
                    "outside_bounded_closure": {
                        **summarized_sites(groups["outside"]),
                        "sites": sorted(groups["outside"], key=lambda row: int(row["site_rva"], 16)),
                    },
                    "module_direct": {
                        **summarized_sites(groups["module"]),
                        "sites": sorted(groups["module"], key=lambda row: int(row["site_rva"], 16)),
                    },
                },
                "address_immediate_candidates": sorted(
                    address_immediates[source][key], key=lambda row: int(row["site_rva"], 16)),
            }
        combined_outside = {}
        for source in source_names:
            for row in hits[source][key]["outside"]:
                signature = (row["site_rva"], row["address"], row["width"], row["access"], row["instruction"])
                if signature not in combined_outside:
                    combined_outside[signature] = {k: v for k, v in row.items() if k != "scan_source"}
                    combined_outside[signature]["observed_by"] = []
                if source not in combined_outside[signature]["observed_by"]:
                    combined_outside[signature]["observed_by"].append(source)
        result_targets[key]["unique_outside_candidate_rows"] = sorted(
            combined_outside.values(), key=lambda row: int(row["site_rva"], 16))

    return {
        "sha256": digest,
        "image_base": hex(pe.image_base),
        "file_backed_executable_sections": sections,
        "strict_decode_first_stops": strict_sections,
        "targets": result_targets,
        "page_base_address_immediates": {
            source: sorted(rows, key=lambda row: int(row["site_rva"], 16))
            for source, rows in page_base_immediates.items()
        },
        "function_inventory_source": "recovery.json",
    }


def span(pe_type, exe: Path, rva: int, size: int) -> dict:
    pe = pe_type(str(exe))
    data = pe.read_rva(rva, size)
    return {"rva": hex(rva), "size": size, "sha256": sha(data), "bytes": data}


def callback_comparison(pe_type, exe: Path, base_relocations: int) -> dict:
    pe = pe_type(str(exe))
    if pe.image_base != 0x400000:
        raise AssertionError(f"unexpected EXE image base {pe.image_base:#x}")
    if base_relocations != 0:
        raise AssertionError("public native-dispatch evidence reports base relocations")
    ac00 = span(pe_type, exe, 0xAC00, 8)
    bd80 = span(pe_type, exe, 0xBD80, 22)
    call_site = span(pe_type, exe, 0xB2E2, 5)
    if ac00["bytes"] != bytes.fromhex("e87b11000033c0c3"):
        raise AssertionError("AC00 wrapper bytes changed")
    if bd80["bytes"] != bytes.fromhex("a180e7a80085c0740c8b0d88e7a80085c97402ffe0c3"):
        raise AssertionError("BD80 callback-gate bytes changed")
    if call_site["bytes"][0] != 0xE8:
        raise AssertionError("AC60 completion call is no longer a direct CALL")
    displacement = int.from_bytes(call_site["bytes"][1:], "little", signed=True)
    target_rva = 0xB2E2 + 5 + displacement
    if target_rva != 0xBD80:
        raise AssertionError("AC60 tail call no longer targets BD80")
    for value in (ac00, bd80, call_site):
        del value["bytes"]
    return {
        "source_table_slot_0x140_target": "AC00 (public compatibility.json)",
        "ac00_wrapper": {
            **ac00,
            "observed_operations": ["call BD80", "xor eax,eax", "ret"],
            "explicit_arguments_prepared": [],
            "stack_argument_contract": "AC00 does not read or adjust stack arguments; arguments supplied by an external caller are unresolved",
            "post_callback_effect": "sets EAX to zero before returning to its caller",
        },
        "bd80_gate": {
            **bd80,
            "observed_operations": [
                "load callback target from [0xA8E780]",
                "return if target is null",
                "load gate word from [0xA8E788]",
                "return if gate is zero",
                "tail-jump through callback target",
            ],
            "explicit_arguments_prepared": [],
        },
        "ac60_direct_call": {
            **call_site,
            "target_rva": "0xbd80",
            "continuation_rva": "0xb2e7",
        },
        "callback_continuation_if_invoked": {
            "through_ac00": "0xac05 (then EAX is cleared and AC00 returns)",
            "directly_from_ac60": "0xb2e7",
            "arguments": "neither call site prepares callback arguments; caller stack contents and callback-side interpretation are unresolved",
        },
        "verified_host_adapter_resolution": {
            "original_image_base": "0x400000",
            "original_bd80_va": "0x40bd80",
            "base_relocation_count": base_relocations,
            "finding": "the pinned original BD80 gate is statically addressable as image_base + 0xBD80 without relying on a module copy; live installation or invocation is unobserved",
        },
        "interchangeability": "same callback target/gate, but not ABI-identical: AC00 inserts a return frame and clears EAX after callback; the callback sees different return-address context",
    }


def main() -> None:
    args = parse_args()
    root = repository_root(args.original_root)

    analysis_deps = Path(os.environ.get("HERCULES_PYTHON_DEPS", r"C:\tools\hercules\python"))
    if not analysis_deps.is_dir():
        raise RuntimeError(f"analysis dependency directory missing: {analysis_deps}")
    os.environ["HERCULES_PYTHON_DEPS"] = str(analysis_deps)
    sys.path[:0] = [str(analysis_deps), str(root / "scripts"),
                    str(root / "renderer_contract" / "tests")]

    import capstone
    from capstone import x86
    from pe import PE
    from oracle_common import EXPECTED_EXE_SHA256, sha256

    if capstone.__version__ != "5.0.7":
        raise RuntimeError(f"expected Capstone 5.0.7, got {capstone.__version__}")
    if EXPECTED_EXE_SHA256 != IMAGE_SHA256["HERCULES.EXE"]:
        raise RuntimeError("public oracle helper and audit EXE pins disagree")

    evidence_dir = root / "renderer_contract" / "evidence"
    public_hashes = {}
    for filename, expected in PUBLIC_EVIDENCE_SHA256.items():
        path = evidence_dir / filename
        actual = sha256(path.read_bytes())
        if actual != expected:
            raise AssertionError(f"public evidence changed: {filename} SHA-256 {actual} != {expected}")
        public_hashes[filename] = actual

    closure = json.loads((evidence_dir / "closure.json").read_text(encoding="utf-8-sig"))
    dispatch_evidence = json.loads((evidence_dir / "native_dispatch.json").read_text(encoding="utf-8-sig"))
    recovery_path = root / "recovery.json"
    recovery_bytes = recovery_path.read_bytes()
    recovery_sha = sha256(recovery_bytes)
    if recovery_sha != RECOVERY_SHA256:
        raise AssertionError(f"recovery.json SHA-256 {recovery_sha} != pinned inventory {RECOVERY_SHA256}")
    recovery = json.loads(recovery_bytes.decode("utf-8-sig"))
    dependencies = []
    for row in closure["dependencies"]:
        start = int(row["rva"], 16)
        dependencies.append({
            "name": row["name"], "module": row.get("module", "HERCULES.EXE"),
            "start": start, "end": start + row["size"],
            "size": row["size"], "sha256": row["span_sha256"],
            "classification": row["classification"],
        })

    overrides = {"HERCULES.EXE": args.exe, "ENG1.DLL": args.eng1,
                 "ENG3.DLL": args.eng3, "TITLE.DLL": args.title}
    suffixes = {
        "HERCULES.EXE": "work/discs/pc_install/HERCULES.EXE",
        "ENG1.DLL": "assets/pc/HERCULES/ENG1.DLL",
        "ENG3.DLL": "assets/pc/HERCULES/ENG3.DLL",
        "TITLE.DLL": "assets/pc/HERCULES/TITLE.DLL",
    }
    images = {}
    paths = {}
    for name, expected in IMAGE_SHA256.items():
        path = find_original(root, suffixes[name], overrides[name])
        paths[name] = path
        images[name] = scan_image(name, path, expected, PE, capstone, x86,
                                  dependencies, recovery["functions"])

    candidate_count_summary = {}
    address_candidate_count_summary = {}
    outside_candidate_sites = {}
    for key in TARGETS:
        candidate_count_summary[key] = {}
        address_candidate_count_summary[key] = {}
        for image in IMAGE_SHA256:
            candidate_count_summary[key][image] = {}
            address_candidate_count_summary[key][image] = {}
            for source in ("section_linear_sweep", "recovery_function_inventory",
                           "bounded_renderer_closure"):
                reference = images[image]["targets"][key][source]["direct_absolute_memory"]
                candidate_count_summary[key][image][source] = {
                    "inside_bounded_closure": sum(row["count"] for row in reference["inside_bounded_closure"]),
                    "outside_bounded_closure": reference["outside_bounded_closure"]["count"],
                    "module_direct": reference["module_direct"]["count"],
                }
                address_candidate_count_summary[key][image][source] = len(
                    images[image]["targets"][key][source]["address_immediate_candidates"])
        exe = images["HERCULES.EXE"]["targets"][key]
        outside_candidate_sites[key] = exe["unique_outside_candidate_rows"]

    result = {
        "schema_version": 1,
        "status": "STATIC_CANDIDATE_INVENTORY",
        "scope": "Candidate inventory from a full Capstone x86 linear sweep of each file-backed executable section with explicit one-byte skipdata recovery, plus bounded sweeps rooted at recovery.json function starts and public renderer-closure spans. This is not proof of actual control-flow instruction alignment; decoded data can resemble instructions.",
        "tool": {"capstone_version": capstone.__version__, "pe_reader": "scripts/pe.py",
                 "oracle_helper": "renderer_contract/tests/oracle_common.py",
                 "skipdata": "Capstone .byte pseudo-ops are counted as skipped bytes and never scanned for operands",
                 "function_inventory": "recovery.json"},
        "public_evidence_sha256": public_hashes,
        "function_inventory_sha256": recovery_sha,
        "originals": images,
        "bounded_dependency_intervals": [
            {"name": row["name"], "module": row["module"],
             "rva": hex(row["start"]), "size": row["size"],
             "sha256": row["sha256"], "classification": row["classification"]}
            for row in dependencies
        ],
        "callback_80_vs_bd80": callback_comparison(
            PE, paths["HERCULES.EXE"], dispatch_evidence["oracle"]["base_relocations"]),
        "summary": {
            "direct_absolute_reference_candidate_counts": candidate_count_summary,
            "address_immediate_candidate_counts": {
                key: address_candidate_count_summary[key] for key in TARGETS
            },
            "page_base_address_immediate_counts": {
                image: {source: len(images[image]["page_base_address_immediates"][source])
                        for source in ("section_linear_sweep", "recovery_function_inventory",
                                       "bounded_renderer_closure")}
                for image in IMAGE_SHA256
            },
            "unique_outside_candidate_sites": {
                key: sites for key, sites in outside_candidate_sites.items() if sites
            },
            "strict_decode_first_stops": {
                image: images[image]["strict_decode_first_stops"] for image in IMAGE_SHA256
            },
        },
        "limits": [
            "Linear-sweep decoding can begin inside embedded data or decode data bytes as plausible instructions; all decoded operands remain candidates unless independently bounded at a real function start.",
            "recovery.json supplies only a partial function inventory; function-rooted spans are useful anchors, not a complete control-flow map.",
            "Every undecodable byte is represented by a Capstone .byte skipdata pseudo-op; exact gaps and unanchored spans are retained per executable section.",
            "An address-immediate candidate is not asserted to be a dereference or pointer escape without local evidence.",
            "Outside means outside the reviewed bounded dependency intervals, not outside every renderer-related path.",
            "The scan does not resolve register-computed aliases, dynamic code, external callbacks, or allocation extents.",
            "Absence of a scanned candidate does not exclude callback-derived aliases; retain observed staged writes until downstream consumers of possible aliases are excluded.",
        ],
    }
    output = args.output if args.output.is_absolute() else root / args.output
    output = output.resolve()
    work_root = (root / "work").resolve()
    try:
        output.relative_to(work_root)
    except ValueError as error:
        raise ValueError(f"--output must stay inside the checkout's work directory: {work_root}") from error
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, separators=(",", ":")) + "\n", encoding="utf-8")
    print(json.dumps({"status": result["status"], "output": str(output),
                      "originals": {name: info["sha256"] for name, info in images.items()}},
                     separators=(",", ":")))


if __name__ == "__main__":
    main()
