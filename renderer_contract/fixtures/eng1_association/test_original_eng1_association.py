"""Replay the bounded original-x86 mesh association fixture with explicit inputs."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import stat
import subprocess
import sys


SCRIPT = Path(__file__).resolve()
FIXTURE_DIR = SCRIPT.parent
EXPECTED_ORIGINALS = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "ENG3.DLL": "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
    "TITLE.DLL": "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
}
# LF-normalized publication of the pre-experiment prediction; the historical
# CRLF receipt hash was 8adee7152c7f2e8a9abf8f14fb083ae0593a90dfe93886c43cb2d8baf1db849d.
PREDICTION_SHA256 = "b0b71f105687e8f3dd9adf7c211adf77db7cd8b3cd5807380ab064afe9ea8262"
PUBLIC_BRIDGE_BASE = 0x11000000


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def inside(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def reject_reparse_components(path: Path) -> None:
    current = Path(path.anchor)
    reparse_flag = getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400)
    for part in path.parts[1:]:
        current = current / part
        try:
            info = current.lstat()
        except FileNotFoundError:
            continue
        if current.is_symlink() or (getattr(info, "st_file_attributes", 0) & reparse_flag):
            raise ValueError(f"reparse/symlink component is not allowed: {current}")


def safe_repo_relative(root: Path, raw: str) -> Path:
    normalized = raw.replace("\\", "/")
    candidate = Path(normalized)
    if candidate.is_absolute() or ".." in candidate.parts:
        raise ValueError(f"receipt path is not repository-relative: {raw}")
    path = (root / candidate).resolve(strict=False)
    if not inside(path, root):
        raise ValueError(f"receipt path escapes repository: {raw}")
    return path


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True,
                        help="explicit checkout containing renderer_contract/, scripts/, and work/")
    parser.add_argument("--originals", type=Path, required=True,
                        help="explicit read-only matching PC install; exact PE hashes are checked")
    parser.add_argument("--projection-report", type=Path, required=True,
                        help="explicit current public projection report; sources and artifacts are revalidated")
    parser.add_argument("--output", type=Path, required=True,
                        help="new, unused output directory strictly below this checkout's work/")
    args = parser.parse_args()
    raw_paths = (args.repo_root, args.originals, args.projection_report, args.output)
    if any(not path.is_absolute() for path in raw_paths):
        parser.error("all paths must be absolute")
    try:
        for path in raw_paths:
            reject_reparse_components(path)
    except ValueError as error:
        parser.error(str(error))

    args.repo_root = args.repo_root.resolve(strict=False)
    if not args.repo_root.is_dir():
        parser.error("--repo-root must be an existing directory")
    expected_script = (args.repo_root / "renderer_contract" / "fixtures" / "eng1_association"
                       / "test_original_eng1_association.py").resolve(strict=False)
    if SCRIPT != expected_script:
        parser.error("test entry must be the shipped renderer_contract fixture in --repo-root")
    if not (args.repo_root / "scripts" / "pe.py").is_file():
        parser.error("--repo-root must contain scripts/pe.py")
    args.originals = args.originals.resolve(strict=False)
    if not args.originals.is_dir():
        parser.error("--originals must be a directory")
    args.projection_report = args.projection_report.resolve(strict=False)
    if not args.projection_report.is_file():
        parser.error("--projection-report must be a readable report file")
    work_root = (args.repo_root / "work").resolve(strict=False)
    if not work_root.is_dir():
        parser.error("--repo-root/work must exist")
    args.output = Path(os.path.abspath(args.output))
    resolved_output = args.output.resolve(strict=False)
    if not inside(resolved_output, work_root) or resolved_output == work_root:
        parser.error("--output must be strictly below --repo-root/work")
    if resolved_output.exists():
        parser.error("--output must be a new, unused directory")
    protected = ((args.repo_root / "assets").resolve(strict=False), args.originals)
    if any(resolved_output == item or inside(resolved_output, item) or inside(item, resolved_output)
           for item in protected):
        parser.error("--output overlaps assets or the original install tree")
    try:
        for path in (args.repo_root, args.originals, args.projection_report, resolved_output):
            reject_reparse_components(path)
    except ValueError as error:
        parser.error(str(error))
    if Path(sys.executable).resolve() != Path(
            r"C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"):
        parser.error("run with the pinned Python runtime recorded by the public projection receipt")
    return args


def extract_prefixed_path(argv: list[str], prefix: str) -> Path | None:
    lowered = prefix.lower()
    for arg in argv:
        if arg.lower().startswith(lowered):
            return Path(arg[len(prefix):]).resolve(strict=False)
    return None


def verify_public_projection(repo_root: Path, report_path: Path) -> dict:
    raw_report = report_path.read_bytes()
    report = json.loads(raw_report)
    summary = report.get("summary", {})
    if (report.get("status") != "BOUNDED_X86_DIFFERENTIAL_AND_BRIDGE_OBSERVED"
            or summary.get("case_count") != 14 or summary.get("assertion_count", 0) < 1
            or summary.get("original_oracle_vector_table_passed") is not True
            or summary.get("wrapper_game_write_sequences_match_direct_B920") is not True
            or len(report.get("cases", [])) != summary.get("case_count")):
        raise RuntimeError("projection report does not satisfy its finite 14-case acceptance gates")
    toolchain = report.get("toolchain", {})
    source_receipts = toolchain.get("sources", {})
    if not source_receipts:
        raise RuntimeError("projection report has no source-hash receipt")
    verified_sources = {}
    for relative, expected_hash in source_receipts.items():
        path = safe_repo_relative(repo_root, relative)
        if not path.is_file() or sha256_file(path) != expected_hash:
            raise RuntimeError(f"projection source differs from report receipt: {relative}")
        verified_sources[str(path)] = expected_hash

    manifest_path = Path(toolchain["manifest_path"]).resolve(strict=False)
    if (not manifest_path.is_file() or not inside(manifest_path, repo_root)
            or sha256_file(manifest_path) != toolchain["manifest_sha256"]):
        raise RuntimeError("toolchain manifest differs from projection report receipt")
    compiler = Path(toolchain["compiler"]).resolve(strict=False)
    linker = Path(toolchain["linker"]).resolve(strict=False)
    if (not compiler.is_file() or not linker.is_file()
            or sha256_file(compiler) != toolchain["compiler_sha256"]
            or sha256_file(linker) != toolchain["linker_sha256"]):
        raise RuntimeError("projection compiler/linker differs from report receipt")

    runtime = report.get("runtime", {})
    runtime_python = Path(runtime["python_executable"]).resolve(strict=False)
    if (runtime_python != Path(sys.executable).resolve()
            or sha256_file(runtime_python) != runtime.get("python_executable_sha256")):
        raise RuntimeError("Python runtime differs from public projection receipt")
    runtime_module = Path(runtime["unicorn_module_path"]).resolve(strict=False)
    runtime_dll = runtime_module.parent / "lib" / "unicorn.dll"
    if (not runtime_module.is_file() or not runtime_dll.is_file()
            or sha256_file(runtime_module) != runtime.get("unicorn_python_sha256")
            or sha256_file(runtime_dll) != runtime.get("native_dll_sha256")):
        raise RuntimeError("Unicorn runtime files differ from public projection receipt")

    harness = report.get("harness", {})
    pe_helper = Path(harness["pe_helper_path"]).resolve(strict=False)
    if (not pe_helper.is_file() or not inside(pe_helper, repo_root)
            or sha256_file(pe_helper) != harness["pe_helper_sha256"]
            or pe_helper != (repo_root / "scripts" / "pe.py").resolve()):
        raise RuntimeError("repository PE helper differs from projection report receipt")

    commands = toolchain.get("commands", [])
    for command in commands:
        if command.get("returncode") != 0:
            raise RuntimeError("projection build receipt contains a failed command")
    link_commands = [row for row in commands
                     if any(arg.lower().startswith("/out:") for arg in row.get("argv", []))]
    if not link_commands:
        raise RuntimeError("projection report has no successful output-link command")
    link_argv = link_commands[-1]["argv"]
    public_dll = extract_prefixed_path(link_argv, "/OUT:")
    public_map = extract_prefixed_path(link_argv, "/MAP:")
    if public_dll is None or public_map is None:
        raise RuntimeError("projection link receipt omits its DLL or map output")
    work_root = (repo_root / "work").resolve(strict=False)
    if not work_root.is_dir():
        raise RuntimeError("repository work directory is missing")
    if (not inside(public_dll, work_root) or not inside(public_map, work_root)
            or not public_dll.is_file() or not public_map.is_file()
            or sha256_file(public_dll) != toolchain.get("candidate_dll_sha256")
            or sha256_file(public_map) != toolchain.get("map_file_sha256")):
        raise RuntimeError("projection DLL or map differs from report output receipt")

    expected_sources = {
        (repo_root / "renderer_contract" / "hercules_projection.c").resolve(): "hercules_projection.obj",
        (repo_root / "renderer_contract" / "fixtures" / "projection"
         / "hercules_projection_bridge_diagnostic.c").resolve(): "hercules_projection_bridge_diagnostic.obj",
    }
    objects = {}
    for command in commands:
        argv = command.get("argv", [])
        if not argv:
            continue
        source_arg = Path(argv[-1]).resolve(strict=False)
        if source_arg not in expected_sources:
            continue
        object_path = extract_prefixed_path(argv, "/Fo")
        if object_path is None or not inside(object_path, work_root) or not object_path.is_file():
            raise RuntimeError(f"projection object output missing/outside work/: {source_arg}")
        objects[expected_sources[source_arg]] = object_path
    if set(objects) != set(expected_sources.values()):
        raise RuntimeError("projection report does not identify both bridge input objects")
    return {
        "report": report,
        "report_path": str(report_path),
        "report_sha256": sha256_bytes(raw_report),
        "public_dll": public_dll,
        "public_dll_sha256": toolchain["candidate_dll_sha256"],
        "public_map": public_map,
        "public_map_sha256": toolchain["map_file_sha256"],
        "source_hashes": verified_sources,
        "objects": objects,
        "object_hashes": {name: sha256_file(path) for name, path in objects.items()},
        "compiler": compiler,
        "linker": linker,
        "compiler_sha256": toolchain["compiler_sha256"],
        "linker_sha256": toolchain["linker_sha256"],
        "runtime": runtime,
        "manifest_path": str(manifest_path),
        "manifest_sha256": toolchain["manifest_sha256"],
    }


def make_public_bridge_builder(repo_root: Path, output_root: Path, public: dict):
    def build_bridge(output: Path) -> dict:
        output = output.resolve(strict=False)
        if not inside(output, output_root) or output == output_root:
            raise RuntimeError("bridge build output must stay in the selected redirected work output")
        reject_reparse_components(output)
        output.mkdir(parents=True, exist_ok=True)
        temp_dir = output / "tmp"
        temp_dir.mkdir(parents=True, exist_ok=True)
        dll = output / "projection_bridge_from_public_objects_at_11000000.dll"
        map_path = output / "projection_bridge_from_public_objects_at_11000000.map"
        obj_core = public["objects"]["hercules_projection.obj"]
        obj_bridge = public["objects"]["hercules_projection_bridge_diagnostic.obj"]
        command = [str(public["linker"]), "/nologo", "/DLL", "/NOENTRY", "/NODEFAULTLIB",
                   "/INCREMENTAL:NO", "/OPT:NOREF", "/FIXED", f"/BASE:0x{PUBLIC_BRIDGE_BASE:08x}",
                   "/MACHINE:X86", f"/OUT:{dll}", f"/MAP:{map_path}", str(obj_core), str(obj_bridge)]
        env = os.environ.copy()
        for key in ("LINK", "_LINK_"):
            env.pop(key, None)
        env["PATH"] = str(public["linker"].parent) + os.pathsep + env.get("PATH", "")
        env["TEMP"] = str(temp_dir)
        env["TMP"] = str(temp_dir)
        process = subprocess.run(command, cwd=str(output), env=env, text=True,
                                 errors="replace", capture_output=True, timeout=60)
        if process.returncode != 0:
            raise RuntimeError(json.dumps({"command": command, "returncode": process.returncode,
                                           "stdout": process.stdout, "stderr": process.stderr}, indent=2))
        return {
            "dll": str(dll), "dll_sha256": sha256_file(dll),
            "map": str(map_path), "map_sha256": sha256_file(map_path),
            "public_projection_report": {"path": public["report_path"], "sha256": public["report_sha256"],
                                          "status": public["report"]["status"],
                                          "case_count": public["report"]["summary"]["case_count"],
                                          "assertion_count": public["report"]["summary"]["assertion_count"]},
            "public_projection_dll": {"path": str(public["public_dll"]), "sha256": public["public_dll_sha256"]},
            "public_projection_map": {"path": str(public["public_map"]), "sha256": public["public_map_sha256"]},
            "public_source_hashes": public["source_hashes"],
            "public_object_hashes": public["object_hashes"],
            "toolchain": {"compiler_sha256": public["compiler_sha256"],
                           "linker_sha256": public["linker_sha256"],
                           "compiler_version": public["report"]["toolchain"]["compiler_version"],
                           "linker_version": public["report"]["toolchain"]["linker_version"],
                           "compiler_path": str(public["compiler"]), "linker_path": str(public["linker"]),
                           "manifest_path": public["manifest_path"],
                           "manifest_sha256": public["manifest_sha256"]},
            "link_command": command,
            "link_flags": ["/DLL", "/NOENTRY", "/NODEFAULTLIB", "/INCREMENTAL:NO", "/OPT:NOREF",
                           "/FIXED", f"/BASE:0x{PUBLIC_BRIDGE_BASE:08x}", "/MACHINE:X86"],
            "link_stdout": process.stdout,
        }

    return build_bridge


def packet_map(case: dict) -> list[dict]:
    result = []
    for packet in case["packets"]:
        vertices = []
        for vertex in packet["vertices"]:
            provenance = vertex["copy_provenance"]
            if vertex["slot"] < 3:
                copy = {
                    "face_read_pc": provenance["face_record_read_pc"],
                    "face_slot": provenance["face_slot"],
                    "area_argument_point_index": provenance["area_callback_argument_point_index"],
                    "copy_callback": provenance["packet_copy_callback"],
                    "packet_copy_global_reads": provenance["packet_copy_global_reads"],
                    "packet_destination_writes": provenance["packet_copy_destination_writes"],
                }
            else:
                copy = {
                    "source_read_pc": provenance["source_read_pc"],
                    "source_stage_address": provenance["source_stage_address"],
                    "point_index_from_stage_address": provenance["source_point_index_from_address"],
                    "packet_store_pc": provenance["original_store_pc"],
                    "packet_store_address": provenance["packet_destination_address"],
                    "packet_store_value": provenance["packet_store_value"],
                }
            vertices.append({"slot": vertex["slot"], "point_index": vertex["point_index"],
                             "source_stream_address": vertex["source_point_stream_address"],
                             "stage_record_address": vertex["source_stage_record_address"],
                             "packet_xy_address": vertex["packet_xy_address"],
                             "xy": vertex["xy"], "copy": copy})
        result.append({"packet_address": packet["packet_address"],
                       "face_record_address": packet["face_record_address"],
                       "face_record_index": packet["face_record_index"],
                       "face_point_indices": packet["face_point_indices"],
                       "tag": packet["tag_word"], "stride_bytes": packet["stride"],
                       "link_previous_head": packet["link_previous_head"], "vertices": vertices})
    return result


def validate_point_face_packet_provenance(report: dict) -> None:
    """Reject value-only joins: every packet corner must resolve by original address/index."""
    layout = report["synthetic_fixture_layout"]
    point_stream = int(layout["point_stream"], 16)
    stage_base = int(layout["point_stage_record_base"], 16)
    for case in report["cases"]:
        points = {row["source_index"]: row for row in case["point_input_records"]}
        if len(points) != len(layout["points_xyz"]):
            raise RuntimeError(f"{case['case']}: point capture count differs from fixture points")
        for index, point in points.items():
            if int(point["source_address"], 16) != point_stream + index * 4:
                raise RuntimeError(f"{case['case']}: source address does not encode its point index")
            if int(point["scratch_address"], 16) != stage_base + index * 8:
                raise RuntimeError(f"{case['case']}: stage address does not encode its point index")
            if (point["source_read"]["source_index"] != index
                    or point["source_read"]["source_address"] != point["source_address"]):
                raise RuntimeError(f"{case['case']}: original source read lost address/index provenance")
            if (point["B6F0_input"]["input_address"] != point["scratch_address"]
                    or point["B6F0_input"]["input_words"] != point["source_xyz"]):
                raise RuntimeError(f"{case['case']}: B6F0 input pointer/data differ from the selected point")
            if point["B920_projection_return"]["projected_xy"] != point["scratch_xy"]:
                raise RuntimeError(f"{case['case']}: projected XY differs from observed stage XY")
            if point["B720_output"] is not None:
                if point["B720_output"]["source_point"]["source_index"] != index:
                    raise RuntimeError(f"{case['case']}: B720 output lost original point index")
                if (case["case"].startswith("gt4_")
                        and point["B720_output"]["output_address"] != point["scratch_address"]):
                    raise RuntimeError(f"{case['case']}: GT4 output pointer differs from point stage address")
        for association in case["face_to_packet_associations"]:
            packet = next((row for row in case["packets"]
                           if row["packet_address"] == association["packet_address"]), None)
            if packet is None:
                raise RuntimeError(f"{case['case']}: face association has no committed packet")
            indices = association["point_indices"]
            if len(indices) != 4 or len(packet["vertices"]) != 4:
                raise RuntimeError(f"{case['case']}: expected four indexed face corners")
            first = packet["vertices"][0]["copy_provenance"]
            word = int(first["face_record_word"], 16)
            if [(word >> (6 * slot)) & 0x3f for slot in range(4)] != indices:
                raise RuntimeError(f"{case['case']}: six-bit face decode differs from producer association")
            if first["face_record_address"] != association["face_record_address"] or first["face_slot"] != 0:
                raise RuntimeError(f"{case['case']}: packet face address/first slot mismatch")
            for slot, (index, vertex) in enumerate(zip(indices, packet["vertices"])):
                provenance = vertex["copy_provenance"]
                point = points.get(index)
                if point is None or vertex["point_index"] != index:
                    raise RuntimeError(f"{case['case']}: packet corner does not retain the decoded point ID")
                if slot < 3:
                    if (provenance["face_slot"] != slot
                            or provenance["face_record_address"] != association["face_record_address"]
                            or provenance["area_callback_argument_point_index"] != index):
                        raise RuntimeError(f"{case['case']}: callback corner lost face/index provenance")
                else:
                    source_face = provenance["source_face_record"]
                    if (source_face["address"] != association["face_record_address"]
                            or source_face["point_indices"] != indices
                            or provenance["source_point_index_from_address"] != index):
                        raise RuntimeError(f"{case['case']}: fourth-corner indexed load lost face provenance")
                if (vertex["source_stage_record_address"] != point["scratch_address"]
                        or vertex["source_point_stream_address"] != point["source_address"]
                        or vertex["source_scratch_xy"] != point["scratch_xy"]
                        or vertex["xy"] != point["scratch_xy"]):
                    raise RuntimeError(f"{case['case']}: packet corner does not copy the indexed point's stage data")
    direct = next(row for row in report["cases"] if row["case"] == "gt4_direct")
    direct_points = {row["source_index"]: row for row in direct["point_input_records"]}
    if (direct_points[0]["scratch_xy"] != direct_points[3]["scratch_xy"]
            or direct_points[0]["scratch_address"] == direct_points[3]["scratch_address"]
            or [row["point_index"] for row in direct["packets"][0]["vertices"][:2]] != [0, 3]):
        raise RuntimeError("equal-XY discriminator no longer proves distinct address/index association")


def create_compact_evidence(repo_root: Path, report: dict, report_path: Path, public: dict) -> dict:
    by_name = {row["case"]: row for row in report["cases"]}
    direct = by_name["gt4_direct"]
    point_inputs = [{
        "index": row["source_index"], "source_address": row["source_address"],
        "source_read_pc": row["source_read"]["pc"], "source_xyz": row["source_xyz"],
        "stage_address": row["scratch_address"], "b6f0_input": row["B6F0_input"]["input_words"],
        "camera_xyz_after_b920": row["B920_projection_return"]["camera_xyz"],
        "projected_xy": row["B920_projection_return"]["projected_xy"],
        "stage_xy_after_b720": row["scratch_xy"],
    } for row in direct["point_input_records"]]
    cases = []
    for name in ("gt4_direct", "gt4_bridge", "ft4_tail_direct", "gt4_culled_prefix_direct"):
        row = by_name[name]
        cases.append({
            "name": name, "packet_count": row["packet_count"],
            "executed_span_names": row["executed_span_names"],
            "executed_byte_ranges_by_span": row["executed_byte_ranges_by_span"],
            "face_to_packet": row["face_to_packet_associations"], "packets": packet_map(row),
            "ot_writes": row["OT_bucket_observations"],
            "bridge_capture_matches_native_B920": (row["bridge_capture"]["matches_native_B920_events"]
                                                    if row["bridge_capture"] else None),
            "bridge_capture_sample_count": len(row["bridge_capture_samples"]),
        })
    projection = public["report"]
    return {
        "schema_version": 1,
        "status": "ORIGINAL_X86_SYNTHETIC_MESH_POINT_TO_PACKET_ASSOCIATION_OBSERVED",
        "scope": "Four bounded synthetic producer cases over hash-pinned original HERCULES.EXE and ENG1.DLL x86 code; no game process/assets.",
        "replay": {
            "repo_root": str(repo_root), "originals": report["input_paths"]["originals"],
            "projection_report": {"path": public["report_path"], "sha256": public["report_sha256"],
                                  "status": projection["status"], "case_count": projection["summary"]["case_count"],
                                  "assertion_count": projection["summary"]["assertion_count"]},
            "output": report["input_paths"]["output"],
            "test_entry": str(SCRIPT), "test_entry_sha256": sha256_file(SCRIPT),
            "fixture_runner": {"path": report["runner"]["path"], "sha256": report["runner"]["sha256"]},
            "machine_report": {"path": str(report_path), "sha256": sha256_file(report_path)},
            "observer_contract_header": {"path": str(repo_root / "renderer_contract" / "eng1_geometry_association_observer.h"),
                                          "sha256": sha256_file(repo_root / "renderer_contract" / "eng1_geometry_association_observer.h")},
            "prediction": report["prediction"],
        },
        "oracle_hashes": report["oracle_hashes"], "mapped_image_bases": report["mapped_image_bases"],
        "callback_transport": report["callback_transport"],
        "dependencies": report["dependencies"],
        "synthetic_inputs": {
            "object_identity": "synthetic mesh/header only; no live scene object ID",
            "point_stream_address": report["synthetic_fixture_layout"]["point_stream"],
            "point_source_record_stride": 4, "points": report["synthetic_fixture_layout"]["points_xyz"],
            "point_stage_pointer_from_original_slot4": report["synthetic_fixture_layout"]["point_stage_pointer_from_native_slot4"],
            "point_stage_record_base": report["synthetic_fixture_layout"]["point_stage_record_base"],
            "point_stage_record_stride": report["synthetic_fixture_layout"]["point_stage_record_stride"],
            "face_record_start": report["synthetic_fixture_layout"]["face_record_start"],
            "face_record_stride": report["synthetic_fixture_layout"]["face_record_stride"],
            "face_slot_decode": {"count": 4, "bits_per_index": 6, "mask": "0x3f",
                                  "shift_by_slot": [0, 6, 12, 18],
                                  "checked_against_original_indexed_reads_and_packet_copies": True},
            "faces_per_case": report["synthetic_fixture_layout"]["faces"],
            "material_mode_byte_offset": report["synthetic_fixture_layout"]["material_mode_byte_offset"],
            "material_mode_byte_value": report["synthetic_fixture_layout"]["material_mode_byte_value"],
            "ft4_dispatch_flag_word": 8,
            "visit_winding_word": report["synthetic_fixture_layout"]["visit_winding_word"],
            "visit_depth_input_word": report["synthetic_fixture_layout"]["visit_depth_input_word"],
            "synthetic_ot_base": report["synthetic_fixture_layout"]["synthetic_ot_base"],
            "synthetic_packet_arena": report["synthetic_fixture_layout"]["synthetic_packet_arena"],
            "synthetic_packet_arena_mapping_bytes": report["synthetic_fixture_layout"]["synthetic_packet_arena_mapping_bytes"],
            "packet_arena_capacity_claim": False,
        },
        "route_pins": {
            "gt4_first_ret": report["historical_geometry_extent_preserved"],
            "ft4_branch_to_separate_tail": report["separate_ft4_tail_extent"],
            "observed_face_word_boundaries": report["producer_observation_contract"],
            "proved_function_extents_are_not_promoted": True,
        },
        "source_point_to_projection": point_inputs,
        "equal_xy_distinct_point_discriminator": {
            "point_indices": [0, 3], "projected_xy": [163, 121],
            "camera_xyz": [[31, 20, 320], [34, 12, 352]],
        },
        "cases": cases,
        "public_projection_bridge": {
            "projection_report_sha256": public["report_sha256"],
            "projection_dll": {"path": str(public["public_dll"]), "sha256": public["public_dll_sha256"]},
            "projection_map": {"path": str(public["public_map"]), "sha256": public["public_map_sha256"]},
            "projection_source_hashes": public["source_hashes"],
            "projection_object_hashes": public["object_hashes"],
            "relinked_bridge_build": report["passive_slot106_bridge_build"],
            "original_callback_slot106_retained": report["callback_transport"]["slot106_original"],
            "only_transport_word_substituted": report["callback_transport"]["bridge_substitution"]["only_word_changed"],
            "direct_and_bridge_packet_summaries_equal": True,
            "direct_and_bridge_ot_head_writes_equal": True,
            "bridge_samples_match_native_b920": by_name["gt4_bridge"]["bridge_capture"]["matches_native_B920_events"],
            "bridge_sample_count": len(by_name["gt4_bridge"]["bridge_capture_samples"]),
        },
        "limitations": [
            "Association relies on emulator instruction-PC/memory-read provenance; a slot-106-only callback cannot recover point IDs from XY values.",
            "A future observer must snapshot face context at ENG1+0x2e0f1 (GT4) or +0x2e27b (FT4), then confirm packet commit at +0x2c790/+0x2c770.",
            "The observed face selector is table context, not dynamic actor/object identity.",
            "Within-run scratch overwrite is observed; cross-frame lifetime and reuse are not proved.",
            "OT bucket index is from these executed synthetic stores only; no global depth formula or frame capacity is claimed.",
            "No production callback ABI, detour, renderer backend, full module recovery, or whole-binary equality is claimed.",
        ],
    }


def main() -> int:
    args = parse_args()
    for name, expected in EXPECTED_ORIGINALS.items():
        path = args.originals / name
        if not path.is_file() or sha256_file(path) != expected:
            raise RuntimeError(f"original hash mismatch: {path}")
    prediction = FIXTURE_DIR / "prediction.md"
    if not prediction.is_file() or sha256_file(prediction) != PREDICTION_SHA256:
        raise RuntimeError("shipped pre-experiment prediction differs from its receipt")
    public = verify_public_projection(args.repo_root, args.projection_report)

    args.output.mkdir(parents=True, exist_ok=False)
    runner_path = FIXTURE_DIR / "run_fixture.py"
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location("shipped_eng1_association_fixture", runner_path)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load shipped original-x86 fixture runner")
    runner = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(runner)
    command = [sys.executable, str(SCRIPT), *sys.argv[1:]]
    runner.configure(args.repo_root, args.originals, args.output, command)
    runner.build_bridge = make_public_bridge_builder(args.repo_root, args.output, public)
    status = runner.main()
    if status != 0:
        raise RuntimeError(f"shipped bounded producer runner returned {status}")

    report_path = args.output / "association_report.json"
    report = json.loads(report_path.read_text(encoding="utf-8"))
    report["input_paths"] = {"repo_root": str(args.repo_root), "originals": str(args.originals),
                             "projection_report": str(args.projection_report), "output": str(args.output)}
    validate_point_face_packet_provenance(report)
    by_name = {row["case"]: row for row in report["cases"]}
    expected_counts = {"gt4_direct": 3, "gt4_bridge": 3, "ft4_tail_direct": 1,
                       "gt4_culled_prefix_direct": 2}
    if {name: by_name[name]["packet_count"] for name in expected_counts} != expected_counts:
        raise RuntimeError("producer packet counts differ from finite association gates")
    if by_name["gt4_bridge"]["bridge_capture"]["matches_native_B920_events"] is not True:
        raise RuntimeError("public slot-106 bridge samples do not match original B920 events")
    if by_name["gt4_direct"]["packets"] != by_name["gt4_bridge"]["packets"]:
        raise RuntimeError("direct and public-bridge packet copies differ")
    if by_name["gt4_direct"]["OT_bucket_writes"] != by_name["gt4_bridge"]["OT_bucket_writes"]:
        raise RuntimeError("direct and public-bridge OT head writes differ")
    if [row["face_record_index"] for row in by_name["gt4_culled_prefix_direct"]["face_to_packet_associations"]] != [1, 2]:
        raise RuntimeError("culled prefix did not preserve later face-to-packet identity")
    if report["historical_geometry_extent_preserved"] != {
            "entry_rva": "0x2df20", "size": "0x334", "ret_rva": "0x2e253"}:
        raise RuntimeError("historical first-RET geometry extent changed")
    if report["separate_ft4_tail_extent"] != {
            "entry_rva": "0x2e254", "size": "0x156", "ret_rva": "0x2e3a9", "branch_rva": "0x2e0da"}:
        raise RuntimeError("separate FT4 tail pin changed")
    ft4 = by_name["ft4_tail_direct"]
    ft4_executed = set(ft4["original_executed_rvas"])
    if ft4["bridge_on"] or not {"0x2e0da", "0x2e254", "0x2e27b", "0x2e3a9"}.issubset(ft4_executed):
        raise RuntimeError("FT4 branch, separate tail, face read, or own RET was not executed")
    tail_ranges = ft4["executed_byte_ranges_by_span"].get("ENG1+0x2e254", [])
    if (not tail_ranges or tail_ranges[0]["start_rva"] != "0x2e254"
            or tail_ranges[-1]["end_rva_exclusive"] != "0x2e3aa"):
        raise RuntimeError("FT4 tail byte trace does not reach its own RET")

    report["public_replay_receipt"] = {
        "test_path": str(SCRIPT), "test_sha256": sha256_file(SCRIPT),
        "fixture_path": str(runner_path), "fixture_sha256": sha256_file(runner_path),
        "projection_report_sha256": public["report_sha256"],
        "output_is_strictly_under_work": True,
    }
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
    evidence = create_compact_evidence(args.repo_root, report, report_path, public)
    evidence_path = args.output / "eng1_geometry_association.json"
    evidence_path.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({"status": report["status"], "cases": expected_counts,
                      "projection_report_sha256": public["report_sha256"],
                      "relinked_public_bridge_sha256": report["passive_slot106_bridge_build"]["dll_sha256"],
                      "machine_report": str(report_path), "machine_report_sha256": sha256_file(report_path),
                      "evidence": str(evidence_path), "evidence_sha256": sha256_file(evidence_path)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
