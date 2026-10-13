from __future__ import annotations

import hashlib
import argparse
import json
from pathlib import Path
import sys
sys.dont_write_bytecode = True
from pathguard import file_below, owned_output, repo_path

LANE = Path(__file__).resolve().parent


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main(argv=None) -> None:
    parser = argparse.ArgumentParser(description="Create a pointer-free geometry manifest from a fresh native ENG1 trace.")
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path,
                        help="existing fresh output directory created by run_native_real_map.py")
    parser.add_argument("--report", type=Path,
                        help="native trace report; defaults to <output>/native_real_map_report.json")
    args = parser.parse_args(argv)
    root = repo_path(args.repo_root)
    output_dir = owned_output(root, args.output)
    report_path = file_below(args.report or output_dir / "native_real_map_report.json",
                             output_dir, "native report")
    if report_path.name != "native_real_map_report.json":
        raise ValueError("manifest generation requires native_real_map_report.json from this fresh run")
    report_bytes = report_path.read_bytes()
    report = json.loads(report_bytes)
    if report.get("result") != "EXECUTED":
        raise RuntimeError("native trace report is not an executed original producer run")
    mesh_rows = report["mesh_calls"]
    mesh_call_index = report["selected_real_resource_case"]["mesh_call_index"]
    mesh = mesh_rows[mesh_call_index]
    outputs = [row for row in report["captured_real_point_outputs"]
               if row["mesh_call_index"] == mesh_call_index]
    if (mesh["mesh_index"], mesh["cell_file_offset"], mesh["point_count"], mesh["face_count"]) != (156, 0x10D33C, 8, 3):
        raise RuntimeError("selected resource discriminator changed")
    by_point = {row["source_point_index"]: row for row in outputs}
    if set(by_point) != set(range(8)):
        raise RuntimeError("native output set does not cover all eight source point indices")

    events = report["association_events_selected_call"]
    reads = {event["source_point_index"]: event for event in events if event["kind"] == "source_point_read"}
    inputs = {event["source_point_index"]: event for event in events if event["kind"] == "b6f0_input"}
    outputs_by_source = {event["source_point_index_from_b6f0"]: event for event in events if event["kind"] == "b720_output"}
    if not (set(reads) == set(inputs) == set(outputs_by_source) == set(range(8))):
        raise RuntimeError("native source-read/B6F0/B720 provenance is incomplete")

    vertices = []
    for point in mesh["source_points"]:
        index = point["index"]
        native_output = by_point[index]
        read = reads[index]
        host_input = inputs[index]
        host_output = outputs_by_source[index]
        if (read["source_read_address"] != host_input["source_read_address"] or
                host_input["host_input_address"] != host_output["host_input_address_from_b6f0"] or
                host_input["host_input_address"] != host_output["scratch_output_address"] or
                host_output["source_read_address"] != read["source_read_address"]):
            raise RuntimeError(f"point {index} read/input/output pointer chain does not agree")
        stage_xyz = native_output["host_input_signed_words"]
        raw_xyz = point["source_xyz"]
        expected_x = raw_xyz[0] if mesh["winding_024"] else 64 - raw_xyz[0]
        if stage_xyz != [expected_x, raw_xyz[1], raw_xyz[2]]:
            raise RuntimeError(f"point {index}: native input transform disagrees with actual winding and packed point")
        vertices.append({
            "identity": f"MAP_PSX.BIN+0x{point['file_offset']:x}:mesh156:point{index}",
            "source_point_index": index,
            "source_point_address_label": f"map+0x{point['file_offset']:x}",
            "source_read": {
                "pc_rva": f"ENG1+0x{int(read['source_read_pc'], 16) - 0x10000000:x}",
                "size": read["source_read_size"],
                "packed_dword": point["packed_dword"],
                "file_offset": point["file_offset"],
            },
            "asset_point_xyz_signed_words": raw_xyz,
            "native_point_input_xyz_signed_words": stage_xyz,
            "input_derivation": "2E600 native producer winding0 maps X to 64-X; Y/Z pass through; B6F0 input at this point is indexed by the actual preceding packed point read",
            "b6f0_input": {
                "pc_rva": "HERC+0xB6F0",
                "return_pc_rva": f"ENG1+0x{int(host_input['host_callback_return_pc'], 16) - 0x10000000:x}",
                "source_read_sequence": read["sequence"],
                "callback_sequence": host_input["sequence"],
                "input_scratch_slot": index,
                "input_scratch_offset": f"+0x{int(host_input['host_input_address'], 16) - int(inputs[0]['host_input_address'], 16):x}",
            },
            "camera_transform": native_output["camera_transform"],
            "captured_camera_xyz_dwords": native_output["camera_xyz_signed"],
            "projection": native_output["projection"],
            "captured_screen_xy_words": native_output["scratch_xy_after_return"],
            "b720_output": {
                "pc_rva": "HERC+0xB720",
                "return_pc_rva": f"ENG1+0x{int(host_output['host_callback_return_pc'], 16) - 0x10000000:x}",
                "callback_sequence": host_output["sequence"],
                "output_scratch_slot": index,
                "output_scratch_offset": f"+0x{int(host_output['scratch_output_address'], 16) - int(outputs_by_source[0]['scratch_output_address'], 16):x}",
                "reads_from_b6f0_input_slot": index,
            },
            "captured_camera_xyz_stage": "post-B740 signed low WORD lanes with camera Z clamp already applied; this sample has Z > 1",
        })

    faces = []
    for packet in report["selected_real_resource_case"]["packets"]:
        faces.append({
            "face_record_index": packet["face_record_index"],
            "face_file_offset": packet["face_file_offset"],
            "packed_index_word": packet["face_packed_index_word"],
            "source_point_indices": packet["face_point_indices"],
            "packet_wrapper_rva": f"ENG1+0x{int(packet['wrapper_rva'], 16):x}",
            "packet_address_label": f"fixture-packet+0x{int(packet['packet_address'], 16) - int(report['selected_real_resource_case']['packets'][0]['packet_address'], 16):x}",
            "packet_tag_previous_link": packet["packet_tag_previous_link"],
            "packet_raw_40_bytes": packet["packet_raw_40_bytes"],
            "packet_xy_words_by_corner": packet["packet_xy_words_by_corner"],
            "corner_associations": packet["corner_associations"],
            "ot_insert": packet["ot_insert"],
        })

    manifest = {
        "schema": "hercules.eng1-real-resource-captured-geometry.v2",
        "provenance": {
            "fixture_report_sha256": sha(report_bytes),
            "source_fixture_report": "native_real_map_report.json",
            "asset_hash": report["hashes"]["MAP_PSX.BIN"],
            "archive_hash": report["hashes"]["HERCULES.FS"],
            "archive_file_offset": report["map_location"]["archive_offset"],
            "exe_hash": report["hashes"]["HERCULES.EXE"],
            "eng1_hash": report["hashes"]["ENG1.DLL"],
            "map_mesh_index": mesh["mesh_index"],
            "map_call_index": mesh_call_index,
            "map_cell_file_offset": mesh["cell_file_offset"],
            "source_stream_file_offset": mesh["source_points"][0]["file_offset"] - 4,
            "face_file_offsets": [row["file_offset"] for row in mesh["source_faces"]],
            "native_resource_route": "original ENG1 2DA90 traverses a finite synthetic camera cell; original 2DF20 selects and emits this archived resource FT4 mesh",
            "controlled_camera_context": "synthetic finite input; resource bytes and resource consumer are original; not a shipped live frame/camera state",
        },
        "actual_native_point_callback_route": {
            "mesh_producer": "ENG1+0x2DF20",
            "primitive_emitter": "ENG1+0x2E600 with its explicit branch tail at +0x2E669 through RET +0x2E6B4",
            "point_batch_count": mesh["point_count"],
            "batch_term_definition": "eight point callbacks for one native 2DF20 mesh call; does not describe a separate callback mode",
            "point_input_callback": "ENG1+0x2C690 -> host slot101 HERC+0xB6F0",
            "point_projection_callback": "ENG1+0x2C6E0 -> host slot106 HERC+0xB920",
            "point_output_callback": "ENG1+0x2C760 -> host slot114 HERC+0xB720",
            "packet_callback": "ENG1+0x2C770 FT4",
            "read_to_output_rule": "Each B720 result is attributed to the source point captured at the preceding B6F0 callback by the retained point index; 2E600 may prefetch the next source record before B720 returns, so latest-read-at-B720 is not a valid join.",
        },
        "config": {
            "matrix_words_row_major": vertices[0]["camera_transform"]["matrix_words_row_major"],
            "translation_dwords": vertices[0]["camera_transform"]["translation_dwords"],
            "center_dwords": report["camera_context"]["center"],
            "focal_dword": report["camera_context"]["focal"],
            "config_provenance": "matrix/translation observed at each original B740 callback; center/focal set by synthetic HERC B970/B990 calls before original traversal; no setter calls occurred during traversal",
        },
        "vertices": vertices,
        "faces_and_packets": faces,
        "association_summary": {
            "point_read_pc_first": "ENG1+0x2E61B",
            "point_read_pc_subsequent": "ENG1+0x2E695 (tail after explicit branch at +0x2E622)",
            "native_camera_projection_entry": "HERC+0xB740 then HERC+0xB920",
            "xy_scratch_copy": "HERC+0xB720 into the same +0x8-stride scratch slot used as B6F0 input, proven by pointer identity and callback order",
            "face_index_decode": "2DF20 uses packed face word four six-bit indices at bits 0, 6, 12 and 18; actual records reuse/reorder indices across three faces",
            "packets_emitted": len(faces),
            "corners_associated": sum(len(face["corner_associations"]) for face in faces),
            "ot_order_in_execution": [face["ot_insert"]["bucket_index_from_fixture_base"] for face in faces],
        },
        "limitations": [
            "All resource mesh/point/face/UV/material bytes are from the hash-pinned archived FOREST MAP_PSX.BIN; the finite row/camera/context and frame/OT/packet backing are controlled fixture state.",
            "HERC B6F0/B920/B720 and ENG1 resource/traversal/producer code execute from pinned original PE images in Unicorn; this is not a live game run.",
            "Camera values are those captured at B740; camera Z is already clamped and these samples do not exercise the clamp boundary.",
            "Packet backing and OT range are bounded emulator memory; they do not establish a general frame capacity or packet allocation owner.",
            "The packet/index association is demonstrated for this one native FT4 mesh route; no dynamic actor identity or general route coverage is claimed.",
        ],
    }
    OUTPUT = output_dir / "real_resource_projection_manifest.json"
    if OUTPUT.exists():
        raise ValueError(f"refusing to overwrite an existing manifest: {OUTPUT}")
    OUTPUT.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"manifest": str(OUTPUT), "manifest_sha256": sha(OUTPUT.read_bytes()),
                      "report_sha256": sha(report_bytes), "vertices": len(vertices),
                      "faces": len(faces), "corners": manifest["association_summary"]["corners_associated"]}, indent=2))


if __name__ == "__main__":
    main()
