"""Run fresh archived-mesh preparation/emission and private geometry output."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
sys.dont_write_bytecode = True

ROOT_HINT = Path(__file__).resolve().parents[2]
REAL_MESH_FIXTURE = ROOT_HINT / "renderer_contract" / "fixtures" / "eng1_real_mesh"
PREPARATION_FIXTURE = ROOT_HINT / "renderer_contract" / "fixtures" / "eng1_preparation"
sys.path.insert(0, str(REAL_MESH_FIXTURE))
from pathguard import (discover_repo_root, file_below, input_file, owned_output,
                       repo_path, _under_work)

ROOT = discover_repo_root(Path(__file__).resolve())
EXPECTED_HASHES = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "HERCULES.FS": "affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4",
    "MAP_PSX.BIN": "78bc5a60ecce1c99e752fb30a16bc5ace4ee80f686365e1da8136a0decbd69cb",
}
EXPECTED_FACE_INDICES = [[5, 4, 7, 6], [3, 2, 5, 4], [0, 1, 3, 2]]
EXPECTED_PREP_TOOL_HASHES = {
    "tool_cl.exe": "bf9f9c74f756fed96e13f7f9a4273495c7dde0a1fb968e3ef6d760ad6d73dfeb",
    "tool_link.exe": "279544809e6353c8e622eb88aea94e86b0258f3c4b8a976373bf4d68a41b63e1",
}
MAP_LENGTH = 1304576
MAX_OT_BUCKETS = 4096
SOURCE_FILES = (
    "dropin_renderer/CMakeLists.txt",
    "dropin_renderer/backends/geometry_diagnostic.h",
    "dropin_renderer/backends/geometry_diagnostic.cpp",
    "dropin_renderer/tools/geometry_diagnostic_cli.cpp",
    "dropin_renderer/tools/run_real_mesh_geometry.py",
    "renderer_contract/CMakeLists.txt",
    "renderer_contract/hercules_projection.h",
    "renderer_contract/fixtures/eng1_preparation/run_prep_source_compare.py",
    "renderer_contract/fixtures/eng1_preparation/run_native_prep_trace.py",
    "renderer_contract/fixtures/eng1_real_mesh/make_real_manifest.py",
    "src/eng1/mesh_prepare.c",
    "src/eng1/mesh_emit.c",
    "src/eng1/primitive_emitters_a.c",
    "renderer_contract/hercules_projection.c",
    "renderer_contract/hercules_reprojection.h",
    "renderer_contract/hercules_types.h",
    "renderer_contract/hercules_reprojection.c",
)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha_file(path: Path) -> str:
    return sha(path.read_bytes())


def parse_int(value) -> int:
    if isinstance(value, bool):
        raise ValueError("boolean is not an integer capture value")
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        return int(value, 0)
    raise ValueError(f"expected an integer capture value, got {type(value).__name__}")


def verify_original_inputs(exe: Path, eng1: Path, archive: Path) -> dict[str, str]:
    checked = {
        "HERCULES.EXE": input_file(exe, "HERCULES.EXE"),
        "ENG1.DLL": input_file(eng1, "ENG1.DLL"),
        "HERCULES.FS": input_file(archive, "HERCULES.FS"),
    }
    if checked["HERCULES.EXE"].parent != checked["ENG1.DLL"].parent:
        raise ValueError("the bounded original fixture requires EXE and ENG1.DLL in one explicit originals directory")
    actual = {name: sha_file(path) for name, path in checked.items()}
    for name, expected in EXPECTED_HASHES.items():
        if name == "MAP_PSX.BIN":
            continue
        if actual[name] != expected:
            raise ValueError(f"{name} identity mismatch")
    return actual


def load_json_below(path: Path, output: Path, label: str) -> tuple[dict, bytes]:
    checked = file_below(path, output, label)
    raw = checked.read_bytes()
    value = json.loads(raw)
    if not isinstance(value, dict):
        raise ValueError(f"{label} must be a JSON object")
    return value, raw


def validate_report_manifest_link(report_bytes: bytes, manifest_bytes: bytes) -> tuple[dict, dict]:
    """Validate the fresh native report link before reading geometry rows."""
    report = json.loads(report_bytes)
    manifest = json.loads(manifest_bytes)
    if not isinstance(report, dict) or not isinstance(manifest, dict):
        raise ValueError("native report and geometry manifest must be JSON objects")
    if report.get("result") != "EXECUTED":
        raise ValueError("native report is not a completed original producer run")
    if manifest.get("schema") != "hercules.eng1-real-resource-captured-geometry.v2":
        raise ValueError("geometry manifest schema is not the current source-v2 capture")
    provenance = manifest.get("provenance", {})
    if provenance.get("source_fixture_report") != "native_real_map_report.json":
        raise ValueError("manifest does not name the fresh native report")
    if provenance.get("fixture_report_sha256") != sha(report_bytes):
        raise ValueError("fresh native report/manifest SHA link mismatch")
    hashes = report.get("hashes", {})
    if hashes != EXPECTED_HASHES:
        raise ValueError("native report original/archive/map hashes differ from the pinned fixture")
    if (provenance.get("exe_hash") != EXPECTED_HASHES["HERCULES.EXE"] or
            provenance.get("eng1_hash") != EXPECTED_HASHES["ENG1.DLL"] or
            provenance.get("archive_hash") != EXPECTED_HASHES["HERCULES.FS"] or
            provenance.get("asset_hash") != EXPECTED_HASHES["MAP_PSX.BIN"]):
        raise ValueError("manifest provenance does not match the pinned original resource set")
    return report, manifest


def validate_prep_emitter_receipt(receipt: dict, report_bytes: bytes,
                                  output: Path) -> None:
    if receipt.get("result") != "PASS":
        raise ValueError("fresh source-built preparation/emitter comparison did not pass")
    if (receipt.get("compile_exit_code") != 0 or receipt.get("run_exit_code") != 0 or
            receipt.get("prep_only_source_test", {}).get("result") != "PASS"):
        raise ValueError("fresh source-built preparation compile/run did not pass")
    emitter = receipt.get("resource_emitter_integration", {})
    if emitter.get("result") != "PASS":
        raise ValueError("fresh source-built resource emitter comparison did not pass")
    if emitter.get("compile_exit_code") != 0 or emitter.get("run_exit_code") != 0:
        raise ValueError("source-built resource emitter compile/run returned a failure code")
    original_hashes = receipt.get("original_hashes", {})
    if any(original_hashes.get(name) != digest for name, digest in EXPECTED_HASHES.items()):
        raise ValueError("prep/emitter receipt is not bound to the pinned original resource set")

    raw_source_hashes = receipt.get("source_hashes", {})
    if not isinstance(raw_source_hashes, dict):
        raise ValueError("prep/emitter receipt source hashes are malformed")
    source_hashes = {}
    for raw_name, digest in raw_source_hashes.items():
        if not isinstance(raw_name, str):
            raise ValueError("prep/emitter receipt source hash name is not text")
        name = raw_name.replace("\\", "/")
        if name in source_hashes:
            raise ValueError(f"duplicate prep/emitter source hash path after normalization: {name}")
        source_hashes[name] = digest
    report_sha = sha(report_bytes)
    if (source_hashes.get("native_report") != report_sha or
            source_hashes.get("generated_native_source_cases_from_report") != report_sha):
        raise ValueError("prep/emitter receipt does not bind this native resource report")
    for relative in ("renderer_contract/fixtures/eng1_preparation/prep_source_compare.c",
                     "renderer_contract/fixtures/eng1_preparation/prep_emitter_compare.c",
                     "src/eng1/mesh_prepare.c",
                     "src/eng1/mesh_emit.c",
                     "src/eng1/primitive_emitters_a.c",
                     "renderer_contract/hercules_projection.c"):
        if source_hashes.get(relative) != sha_file(ROOT / relative):
            raise ValueError(f"fresh prep/emitter receipt source hash mismatch: {relative}")
    for relative, expected in EXPECTED_PREP_TOOL_HASHES.items():
        if source_hashes.get(relative) != expected:
            raise ValueError(f"prep/emitter receipt tool identity mismatch: {relative}")
    for filename, receipt_field in (("prep_source_compare.exe", "prep_only_source_test"),
                                    ("prep_emitter_compare.exe", "resource_emitter_integration")):
        executable = file_below(output / filename, output, filename)
        if sha_file(executable) != receipt[receipt_field].get("executable_sha256"):
            raise ValueError(f"fresh prep/emitter executable hash mismatch: {filename}")


def _validate_signed_words(values, count: int, label: str) -> list[int]:
    if not isinstance(values, list) or len(values) != count:
        raise ValueError(f"{label} must contain {count} signed WORDs")
    result = [parse_int(value) for value in values]
    if any(value < -32768 or value > 32767 for value in result):
        raise ValueError(f"{label} contains a value outside signed WORD range")
    return result


def _signed_byte(value: int) -> int:
    return value - 256 if value & 0x80 else value


def validate_capture(report: dict, manifest: dict, report_bytes: bytes,
                     manifest_bytes: bytes, map_bytes: bytes) -> dict:
    """Join real native points, face packets, and fresh source emitter proof."""
    report, manifest = validate_report_manifest_link(report_bytes, manifest_bytes)
    if len(map_bytes) != MAP_LENGTH or sha(map_bytes) != EXPECTED_HASHES["MAP_PSX.BIN"]:
        raise ValueError("extracted MAP_PSX.BIN bytes fail length/hash validation")
    native_location = report.get("map_location", {})
    if (parse_int(native_location.get("length", -1)) != MAP_LENGTH or
            parse_int(native_location.get("archive_offset", -1)) < 0):
        raise ValueError("native report map extent does not match the pinned archive member")
    map_base = parse_int(native_location.get("map_base", "-1"))
    if map_base < 0:
        raise ValueError("native report is missing the archived map's emulator base")

    provenance = manifest["provenance"]
    if (parse_int(provenance.get("map_mesh_index", -1)) != 156 or
            parse_int(provenance.get("map_cell_file_offset", -1)) != 0x10D33C):
        raise ValueError("manifest selected resource discriminator changed")
    if parse_int(provenance.get("archive_file_offset", -1)) != parse_int(native_location["archive_offset"]):
        raise ValueError("manifest archive member location differs from the native report")

    selected = report.get("selected_real_resource_case", {})
    mesh = selected.get("mesh", {})
    call_index = parse_int(selected.get("mesh_call_index", -1))
    mesh_calls = report.get("mesh_calls", [])
    if call_index < 0 or call_index >= len(mesh_calls) or mesh_calls[call_index] != mesh:
        raise ValueError("selected mesh call is not the corresponding native traversal record")
    if (parse_int(mesh.get("mesh_index", -1)) != 156 or
            parse_int(mesh.get("cell_file_offset", -1)) != 0x10D33C or
            parse_int(mesh.get("point_count", -1)) != 8 or
            parse_int(mesh.get("face_count", -1)) != 3):
        raise ValueError("native report lacks the pinned 8-point/3-face archived mesh")
    if parse_int(provenance.get("map_call_index", -1)) != call_index:
        raise ValueError("manifest and native report selected different mesh calls")

    effective_winding_by_call = {
        parse_int(row["mesh_call_index"]): parse_int(row["value"])
        for row in report.get("selected_and_auxiliary_winding_writes", [])
    }
    if call_index not in effective_winding_by_call:
        raise ValueError("native point producer lacks its observed effective winding write")
    winding = effective_winding_by_call[call_index]
    if winding not in (0, 1):
        raise ValueError("effective winding is outside the measured 0/1 domain")

    vertices = manifest.get("vertices", [])
    if len(vertices) != 8:
        raise ValueError("manifest must contain all eight native point identities")
    by_index = {}
    for row in vertices:
        index = parse_int(row.get("source_point_index", -1))
        if index in by_index or index < 0 or index >= 8:
            raise ValueError("manifest source point indices are duplicated or out of range")
        by_index[index] = row
    if set(by_index) != set(range(8)):
        raise ValueError("manifest source point identities are incomplete")

    source_points = {parse_int(row["index"]): row for row in mesh.get("source_points", [])}
    source_faces = {parse_int(row["index"]): row for row in mesh.get("source_faces", [])}
    captured_outputs = {
        parse_int(row["source_point_index"]): row
        for row in selected.get("point_outputs", [])
    }
    if (set(source_points) != set(range(8)) or set(captured_outputs) != set(range(8)) or
            len(source_faces) != 3):
        raise ValueError("native point read/output sets do not cover the selected resource")

    config = manifest.get("config", {})
    matrix = _validate_signed_words(config.get("matrix_words_row_major"), 9, "camera matrix")
    translation = config.get("translation_dwords")
    if not isinstance(translation, list) or len(translation) != 3:
        raise ValueError("camera translation must contain three captured DWORDs")
    translation = [parse_int(value) for value in translation]
    if any(value < -0x80000000 or value > 0x7FFFFFFF for value in translation):
        raise ValueError("camera translation contains an out-of-range DWORD")
    center = config.get("center_dwords")
    if not isinstance(center, list) or len(center) != 2:
        raise ValueError("camera center must contain two captured DWORDs")
    center = [parse_int(value) for value in center]
    if any(value < -0x80000000 or value > 0x7FFFFFFF for value in center):
        raise ValueError("camera center contains an out-of-range DWORD")
    focal = parse_int(config.get("focal_dword"))
    if focal < 0 or focal > 0xFFFFFFFF:
        raise ValueError("focal value is outside DWORD range")

    vertex_records = []
    expected_vertex_rows = []
    for index in range(8):
        row = by_index[index]
        source = source_points[index]
        output = captured_outputs[index]
        source_xyz = _validate_signed_words(row.get("asset_point_xyz_signed_words"), 3, f"point {index} archive XYZ")
        input_xyz = _validate_signed_words(row.get("native_point_input_xyz_signed_words"), 3, f"point {index} B6F0 XYZ")
        camera_xyz = row.get("captured_camera_xyz_dwords")
        if not isinstance(camera_xyz, list) or len(camera_xyz) != 3:
            raise ValueError(f"point {index} camera capture must have three lanes")
        camera_xyz = [parse_int(value) for value in camera_xyz]
        if any(value < -32768 or value > 32767 for value in camera_xyz):
            raise ValueError(f"point {index} camera lane is not a sign-extended WORD")
        if camera_xyz[2] <= 0:
            raise ValueError(f"point {index} camera depth is outside the private reprojection domain")
        compatibility_xy = _validate_signed_words(row.get("captured_screen_xy_words"), 2, f"point {index} B920 XY")
        source_offset = parse_int(source.get("file_offset", -1))
        output_offset = parse_int(output.get("source_address", "-1")) - parse_int(report["map_location"]["map_base"])
        if source_offset < 0 or source_offset >= MAP_LENGTH or source_offset != output_offset:
            raise ValueError(f"point {index} source offset does not match the native packed-read identity")
        if source_offset + 4 > len(map_bytes):
            raise ValueError(f"point {index} packed source record escapes the original map extent")
        packed_source = struct.unpack_from("<I", map_bytes, source_offset)[0]
        raw_xyz = map_bytes[source_offset:source_offset + 4]
        map_xyz = [_signed_byte(raw_xyz[0]), _signed_byte(raw_xyz[1]),
                   _signed_byte(raw_xyz[3])]
        if parse_int(source["index"]) != index or parse_int(output["source_point_index"]) != index:
            raise ValueError(f"point {index} native read/output identity changed")
        if (parse_int(source.get("packed_dword", "-1")) != parse_int(output.get("source_packed_dword", "-2")) or
                parse_int(source.get("packed_dword", "-1")) != packed_source or
                _validate_signed_words(source.get("source_xyz"), 3, f"point {index} packed source XYZ") != source_xyz or
                source_xyz != map_xyz):
            raise ValueError(f"point {index} native record and manifest differ from original archived point bytes")
        expected_x = source_xyz[0] if winding else 64 - source_xyz[0]
        if input_xyz != [expected_x, source_xyz[1], source_xyz[2]]:
            raise ValueError(f"point {index} B6F0 input does not follow actual winding and packed XYZ")
        b6f0 = row.get("b6f0_input", {})
        b720 = row.get("b720_output", {})
        source_read_sequence = parse_int(b6f0.get("source_read_sequence", -1))
        input_slot = parse_int(b6f0.get("input_scratch_slot", -1))
        b6f0_sequence = parse_int(b6f0.get("callback_sequence", -1))
        output_slot = parse_int(b720.get("output_scratch_slot", -1))
        b720_sequence = parse_int(b720.get("callback_sequence", -1))
        if (input_slot != index or output_slot != index or
                parse_int(b720.get("reads_from_b6f0_input_slot", -1)) != index or
                not (source_read_sequence <= b6f0_sequence < b720_sequence)):
            raise ValueError(f"point {index} lost its B6F0/B720 scratch or event association")
        if output.get("host_input_signed_words") != input_xyz or output.get("camera_xyz_signed") != camera_xyz or output.get("scratch_xy_after_return") != compatibility_xy:
            raise ValueError(f"point {index} native callback values disagree with the manifest")
        transform = row.get("camera_transform", {})
        point_matrix = _validate_signed_words(transform.get("matrix_words_row_major"), 9, f"point {index} matrix")
        point_translation = transform.get("translation_dwords")
        if (point_matrix != matrix or point_translation != translation):
            raise ValueError(f"point {index} transform differs from the manifest's joined config")

        vertex_records.append({
            "index": index,
            "offset": source_offset,
            "source_xyz": source_xyz,
            "input_xyz": input_xyz,
            "camera_xyz": camera_xyz,
            "compatibility_xy": compatibility_xy,
            "input_slot": input_slot,
            "output_slot": output_slot,
            "source_read_sequence": source_read_sequence,
            "b6f0_sequence": b6f0_sequence,
            "b720_sequence": b720_sequence,
            "matrix": matrix,
            "translation": translation,
            "center": center,
            "focal": focal,
        })
        expected_vertex_rows.append(row)

    faces = manifest.get("faces_and_packets", [])
    if len(faces) != 3:
        raise ValueError("manifest must contain all three native FT4 faces")
    face_indices = []
    face_records = []
    for face in faces:
        face_index = parse_int(face.get("face_record_index", -1))
        face_file_offset = parse_int(face.get("face_file_offset", -1))
        point_indices = [parse_int(value) for value in face.get("source_point_indices", [])]
        packet_xy = face.get("packet_xy_words_by_corner", [])
        associations = face.get("corner_associations", [])
        if len(point_indices) != 4 or len(packet_xy) != 4 or len(associations) != 4:
            raise ValueError(f"face {face_index} does not carry four native FT4 corners")
        source_face = source_faces.get(face_index)
        if source_face is None or parse_int(source_face.get("file_offset", -1)) != face_file_offset:
            raise ValueError(f"face {face_index} does not match its original archived face record")
        if face_file_offset < 0 or face_file_offset + 4 > len(map_bytes):
            raise ValueError(f"face {face_index} packed-index word escapes the original map extent")
        packed_word = parse_int(face.get("packed_index_word", -1))
        original_word = struct.unpack_from("<I", map_bytes, face_file_offset)[0]
        source_word = parse_int(source_face.get("packed_index_word", -1))
        if packed_word != original_word or source_word != original_word:
            raise ValueError(f"face {face_index} packed-index word differs from original archived bytes")
        decoded_indices = [(original_word >> (corner * 6)) & 0x3F for corner in range(4)]
        if (point_indices != decoded_indices or
                source_face.get("point_indices") != point_indices):
            raise ValueError(f"face {face_index} corner indices do not decode from the original packed word")
        if any(point_index not in by_index for point_index in point_indices):
            raise ValueError(f"face {face_index} references a missing native source point")
        raw_hex = face.get("packet_raw_40_bytes", "")
        try:
            raw = bytes.fromhex(raw_hex)
        except ValueError as error:
            raise ValueError(f"face {face_index} packet bytes are malformed") from error
        if len(raw) != 40:
            raise ValueError(f"face {face_index} native FT4 packet must be 40 bytes")
        parsed_packet_xy = [list(struct.unpack_from("<hh", raw, 8 + corner * 8)) for corner in range(4)]
        if parsed_packet_xy != packet_xy:
            raise ValueError(f"face {face_index} packet XY words do not match captured FT4 bytes")
        for corner, point_index in enumerate(point_indices):
            association = associations[corner]
            vertex = by_index[point_index]
            if (parse_int(association.get("corner", -1)) != corner or
                    parse_int(association.get("source_point_index", -1)) != point_index or
                    parse_int(association.get("source_address_file_offset", "-1")) - map_base != parse_int(vertex["source_read"]["file_offset"]) or
                    association.get("projected_xy_words") != vertex["captured_screen_xy_words"] or
                    association.get("packet_xy_words") != packet_xy[corner] or
                    packet_xy[corner] != vertex["captured_screen_xy_words"]):
                raise ValueError(f"face {face_index} corner {corner} lost its source-point/native-packet association")
        bucket = parse_int(face.get("ot_insert", {}).get("bucket_index_from_fixture_base", -1))
        if bucket < 0 or bucket >= MAX_OT_BUCKETS:
            raise ValueError(f"face {face_index} OT bucket is outside bounded original fixture backing")
        face_indices.append(point_indices)
        face_records.append({
            "face_index": face_index,
            "offset": face_file_offset,
            "packed_word": parse_int(face.get("packed_index_word")),
            "point_indices": point_indices,
            "packet_xy": packet_xy,
            "ot_bucket": bucket,
            "previous_link": parse_int(face.get("packet_tag_previous_link")),
        })
    if face_indices != EXPECTED_FACE_INDICES:
        raise ValueError("native FT4 face order no longer preserves the reorder/reuse discriminator")
    if [record["offset"] for record in face_records] != [
            parse_int(value) for value in provenance.get("face_file_offsets", [])]:
        raise ValueError("manifest face offsets differ from the pinned native resource summary")
    if manifest.get("association_summary", {}).get("corners_associated") != 12:
        raise ValueError("manifest corner association count is not twelve")

    return {
        "mesh_index": 156,
        "cell_offset": 0x10D33C,
        "map_length": MAP_LENGTH,
        "winding": winding,
        "vertices": vertex_records,
        "faces": face_records,
    }


def write_cli_input(path: Path, capture: dict, report_bytes: bytes,
                    manifest_bytes: bytes) -> None:
    if path.exists():
        raise ValueError(f"refusing to overwrite geometry input: {path}")
    lines = ["HERCULES_GEOMETRY_DIAGNOSTIC_INPUT_V1"]
    for key, value in (
            ("native_report_sha256", sha(report_bytes)),
            ("manifest_sha256", sha(manifest_bytes)),
            ("archive_sha256", EXPECTED_HASHES["HERCULES.FS"]),
            ("asset_sha256", EXPECTED_HASHES["MAP_PSX.BIN"])):
        lines.append(f"meta {key} {value}")
    lines.append("mesh {} {} {} {} 6".format(
        capture["mesh_index"], capture["cell_offset"], capture["map_length"], capture["winding"]))
    for vertex in capture["vertices"]:
        values = ["vertex", vertex["index"], vertex["offset"],
                  *vertex["source_xyz"], *vertex["input_xyz"], *vertex["camera_xyz"],
                  *vertex["compatibility_xy"], vertex["input_slot"], vertex["output_slot"],
                  vertex["source_read_sequence"], vertex["b6f0_sequence"], vertex["b720_sequence"],
                  *vertex["matrix"], *vertex["translation"], *vertex["center"], vertex["focal"]]
        lines.append(" ".join(str(value) for value in values))
    for face in capture["faces"]:
        values = ["face", face["face_index"], face["offset"], face["packed_word"],
                  *face["point_indices"], *(lane for xy in face["packet_xy"] for lane in xy),
                  face["ot_bucket"], face["previous_link"]]
        lines.append(" ".join(str(value) for value in values))
    with path.open("x", encoding="ascii", newline="\n") as stream:
        stream.write("\n".join(lines) + "\n")


def verify_geometry_output(output: dict, capture: dict, manifest: dict,
                           report_bytes: bytes, manifest_bytes: bytes) -> None:
    if output.get("schema") != "hercules.dropin.geometry-diagnostic.v1":
        raise ValueError("geometry CLI emitted an unknown schema")
    if (output.get("asset") != "MAP_PSX.BIN" or
            parse_int(output.get("mesh_index", -1)) != capture["mesh_index"] or
            parse_int(output.get("map_cell_file_offset", -1)) != capture["cell_offset"]):
        raise ValueError("geometry CLI emitted a different archived mesh identity")
    resolution = output.get("resolution", {})
    if (parse_int(resolution.get("scale", -1)) != 6 or
            parse_int(resolution.get("width", -1)) != 1920 or
            parse_int(resolution.get("height", -1)) != 1440):
        raise ValueError("geometry CLI output does not use the explicit 4:3 scale-six policy")
    provenance = output.get("provenance", {})
    if (provenance.get("native_report_sha256") != sha(report_bytes) or
            provenance.get("manifest_sha256") != sha(manifest_bytes) or
            provenance.get("archive_sha256") != EXPECTED_HASHES["HERCULES.FS"] or
            provenance.get("asset_sha256") != EXPECTED_HASHES["MAP_PSX.BIN"]):
        raise ValueError("geometry CLI provenance differs from the freshly validated capture")
    output_vertices = output.get("vertices", [])
    if len(output_vertices) != len(capture["vertices"]):
        raise ValueError("geometry CLI output point count differs from the captured resource")
    expected_by_index = {row["index"]: row for row in capture["vertices"]}
    emitted_indices = []
    for row in output_vertices:
        index = parse_int(row.get("source_point_index", -1))
        emitted_indices.append(index)
        expected = expected_by_index.get(index)
        if expected is None:
            raise ValueError("geometry CLI emitted an unknown source point identity")
        if (parse_int(row.get("source_file_offset", -1)) != expected["offset"] or
                row.get("source_xyz") != expected["source_xyz"] or
                row.get("b6f0_input_xyz") != expected["input_xyz"] or
                row.get("camera_xyz") != expected["camera_xyz"] or
                row.get("compatibility_xy") != expected["compatibility_xy"]):
            raise ValueError(f"geometry CLI point {index} differs from original captured values")
        private_xy = row.get("private_xy", [])
        if len(private_xy) != 2 or not all(math.isfinite(float(value)) for value in private_xy):
            raise ValueError(f"geometry CLI point {index} has a non-finite private coordinate")
        focal = expected["focal"] & 0xFFFF
        if focal & 0x8000:
            focal -= 0x10000
        expected_private = [
            expected["center"][0] * 6 + (focal * 6) * expected["camera_xyz"][0] / expected["camera_xyz"][2],
            expected["center"][1] * 6 + (focal * 6) * expected["camera_xyz"][1] / expected["camera_xyz"][2],
        ]
        if any(abs(float(private_xy[axis]) - expected_private[axis]) > 1e-9
               for axis in range(2)):
            raise ValueError(f"geometry CLI point {index} private coordinates differ from captured camera projection")
        association = row.get("association", {})
        if (parse_int(association.get("input_scratch_slot", -1)) != index or
                parse_int(association.get("output_scratch_slot", -1)) != index or
                parse_int(association.get("source_read_sequence", -1)) != expected["source_read_sequence"] or
                parse_int(association.get("b6f0_callback_sequence", -1)) != expected["b6f0_sequence"] or
                parse_int(association.get("b720_callback_sequence", -1)) != expected["b720_sequence"]):
            raise ValueError(f"geometry CLI point {index} lost its original scratch-slot identity")
    if emitted_indices != list(range(len(capture["vertices"]))):
        raise ValueError("geometry CLI point rows are duplicated or out of source identity order")

    output_faces = output.get("faces", [])
    if len(output_faces) != len(capture["faces"]):
        raise ValueError("geometry CLI output face count differs from the captured FT4 packets")
    for row, expected in zip(output_faces, capture["faces"]):
        if (parse_int(row.get("face_record_index", -1)) != expected["face_index"] or
                parse_int(row.get("face_file_offset", -1)) != expected["offset"] or
                parse_int(row.get("packed_index_word", -1)) != expected["packed_word"] or
                row.get("source_point_indices") != expected["point_indices"] or
                parse_int(row.get("ot_bucket_index", -1)) != expected["ot_bucket"] or
                parse_int(row.get("packet_tag_previous_link", -1)) != expected["previous_link"]):
            raise ValueError("geometry CLI face identity/order differs from the original packet")
        corners = row.get("corners", [])
        if len(corners) != 4:
            raise ValueError("geometry CLI face does not contain four packet corners")
        for expected_corner, (corner, point_index) in enumerate(zip(corners, expected["point_indices"])):
            expected_vertex = expected_by_index[point_index]
            private_xy = corner.get("private_xy", [])
            expected_focal = expected_vertex["focal"] & 0xFFFF
            if expected_focal & 0x8000:
                expected_focal -= 0x10000
            expected_private = [
                expected_vertex["center"][0] * 6 + (expected_focal * 6) * expected_vertex["camera_xyz"][0] / expected_vertex["camera_xyz"][2],
                expected_vertex["center"][1] * 6 + (expected_focal * 6) * expected_vertex["camera_xyz"][1] / expected_vertex["camera_xyz"][2],
            ]
            if (parse_int(corner.get("corner", -1)) != expected_corner or
                    parse_int(corner.get("source_point_index", -1)) != point_index or
                    parse_int(corner.get("source_file_offset", -1)) != expected_vertex["offset"] or
                    corner.get("native_packet_xy") != expected["packet_xy"][expected_corner] or
                    len(private_xy) != 2 or
                    any(not math.isfinite(float(private_xy[axis])) or
                        abs(float(private_xy[axis]) - expected_private[axis]) > 1e-9
                        for axis in range(2))):
                raise ValueError("geometry CLI corner no longer resolves to the original point and FT4 WORDs")


def run(args) -> int:
    root = repo_path(args.repo_root)
    if root != ROOT:
        raise ValueError(f"runner location and --repo-root disagree: {ROOT} vs {root}")
    exe = input_file(args.exe, "HERCULES.EXE")
    eng1 = input_file(args.eng1, "ENG1.DLL")
    archive = input_file(args.archive, "HERCULES.FS")
    original_hashes = verify_original_inputs(exe, eng1, archive)
    cli = input_file(args.geometry_cli, "geometry_diagnostic_cli")
    output = _under_work(root, args.output)
    if output.exists():
        raise ValueError(f"refusing to reuse an existing replay output: {output}")

    python_exe = Path(sys.executable).resolve(strict=True)
    prep_runner = PREPARATION_FIXTURE / "run_prep_source_compare.py"
    prep_command = [str(python_exe), str(prep_runner), "--repo-root", str(root),
                    "--exe", str(exe), "--eng1", str(eng1), "--archive", str(archive),
                    "--output", str(output)]
    prep = subprocess.run(prep_command, cwd=root, capture_output=True, text=True, timeout=900)
    if prep.returncode != 0:
        print(prep.stdout, end="")
        print(prep.stderr, end="", file=sys.stderr)
        return prep.returncode

    output = owned_output(root, output)
    prep_receipt_path = file_below(output / "prep_source_compare_report.json", output,
                                   "fresh prep/emitter report")
    prep_receipt = json.loads(prep_receipt_path.read_bytes())
    native_report_path = file_below(output / "native_real_map_report.json", output,
                                    "fresh native resource report")
    native_report_bytes = native_report_path.read_bytes()
    validate_prep_emitter_receipt(prep_receipt, native_report_bytes, output)

    manifest_runner = REAL_MESH_FIXTURE / "make_real_manifest.py"
    manifest_command = [str(python_exe), str(manifest_runner), "--repo-root", str(root),
                        "--output", str(output)]
    manifest_run = subprocess.run(manifest_command, cwd=root, capture_output=True,
                                  text=True, timeout=120)
    if manifest_run.returncode != 0:
        print(manifest_run.stdout, end="")
        print(manifest_run.stderr, end="", file=sys.stderr)
        return manifest_run.returncode

    manifest_path = file_below(output / "real_resource_projection_manifest.json", output,
                               "fresh geometry manifest")
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    extracted_map = file_below(output / "map_psx.bin", output, "extracted original map slice")
    native_report, _ = validate_report_manifest_link(native_report_bytes, manifest_bytes)
    capture = validate_capture(native_report, manifest,
                               native_report_bytes, manifest_bytes, extracted_map.read_bytes())

    input_path = output / "geometry_diagnostic_input.tsv"
    write_cli_input(input_path, capture, native_report_bytes, manifest_bytes)
    cli_command = [str(cli), "--input", str(input_path)]
    native = subprocess.run(cli_command, cwd=root, capture_output=True, text=True, timeout=60)
    if native.returncode != 0:
        raise RuntimeError("geometry_diagnostic_cli failed: " + native.stderr.strip())
    if native.stderr.strip():
        raise RuntimeError("geometry_diagnostic_cli wrote unexpected diagnostics on stderr")
    geometry = json.loads(native.stdout)
    verify_geometry_output(geometry, capture, manifest, native_report_bytes, manifest_bytes)

    gpu_result = {"result": "NOT_REQUESTED"}
    gpu_probe_path = None
    if args.gpu_probe is not None:
        gpu_probe_path = input_file(args.gpu_probe, "sdl_gpu_probe")
        first_face = geometry["faces"][0]
        first_face_points = [corner["private_xy"] for corner in first_face["corners"]]
        clipped_corners = [
            index for index, (x, y) in enumerate(first_face_points)
            if x < 0.0 or x >= 1920.0 or y < 0.0 or y >= 1440.0
        ]
        quad_values = [format(float(lane), ".17g")
                       for corner in first_face["corners"]
                       for lane in corner["private_xy"]]
        gpu_command = [str(gpu_probe_path), "--diagnostic", "--quad", *quad_values]
        probe = subprocess.run(gpu_command, cwd=root, capture_output=True,
                               text=True, timeout=60)
        if probe.returncode == 77:
            gpu_result = {"result": "SKIPPED", "command": gpu_command,
                          "stderr": probe.stderr.strip(),
                          "reason": "offscreen SDL GPU backend unavailable",
                          "clipped_input_corners": clipped_corners,
                          "pixel_equivalence_claim": False}
        elif probe.returncode != 0:
            raise RuntimeError("optional SDL GPU diagnostic failed: " + probe.stderr.strip())
        else:
            gpu_result = {"result": "PASS", "command": gpu_command,
                          "stdout": probe.stdout.strip(), "stderr": probe.stderr.strip(),
                          "probe_sha256": sha_file(gpu_probe_path),
                          "input_bounds": [1920, 1440],
                          "clipped_input_corners": clipped_corners,
                          "interpretation": "offscreen clear/readback diagnostic only; clipped corners do not establish visible mesh output",
                          "pixel_equivalence_claim": False}

    geometry_path = output / "geometry_diagnostic.json"
    receipt_path = output / "dropin_geometry_replay_report.json"
    if geometry_path.exists() or receipt_path.exists():
        raise ValueError("refusing to overwrite geometry output or replay receipt")
    geometry_path.write_text(native.stdout, encoding="utf-8", newline="\n")
    input_hashes = {name: sha_file(root / name) for name in SOURCE_FILES}
    input_hashes["geometry_diagnostic_cli"] = sha_file(cli)
    receipt = {
        "result": "PASS",
        "scope": "Fresh hash-pinned original ENG1 resource traversal and source-built preparation/emitter feeding a deterministic C++17 camera-space geometry diagnostic for archived MAP_PSX.BIN mesh 156.",
        "original_hashes": {**original_hashes, "MAP_PSX.BIN": EXPECTED_HASHES["MAP_PSX.BIN"]},
        "native_report_sha256": sha(native_report_bytes),
        "manifest_sha256": sha(manifest_bytes),
        "input_sha256": sha(input_path.read_bytes()),
        "geometry_json_sha256": sha(geometry_path.read_bytes()),
        "geometry_cli_sha256": sha_file(cli),
        "source_hashes": input_hashes,
        "source_preparation_emitter": {
            "result": prep_receipt["result"],
            "emitter_result": prep_receipt["resource_emitter_integration"]["result"],
            "source_receipt_sha256": sha(prep_receipt_path.read_bytes()),
            "comparison_stdout": prep_receipt["resource_emitter_integration"].get("stdout", "").strip(),
        },
        "gpu_diagnostic": gpu_result,
        "native_capture": {
            "mesh_index": capture["mesh_index"],
            "map_cell_file_offset": capture["cell_offset"],
            "vertices": len(capture["vertices"]),
            "faces": len(capture["faces"]),
            "associated_packet_corners": sum(len(face["point_indices"]) for face in capture["faces"]),
            "effective_winding": capture["winding"],
        },
        "private_view": {"policy": "hercules_reprojection_from_camera",
                         "resolution": [1920, 1440], "scale": 6,
                         "input": "captured post-B740 camera XYZ; native XY WORDs are comparison-only"},
        "limitations": [
            "The mesh and packet bytes are from the pinned archived FOREST map; the finite row/camera and host context are controlled fixture inputs.",
            "The source-built preparation/emitter comparison is finite; it does not establish live gameplay state or whole-map coverage.",
            "Private coordinates are a separate numerical policy and do not clip, rasterize, or prove visible screen coverage.",
            "The optional SDL probe checks its offscreen geometry path and readback only; it does not claim pixel equality with the original game.",
        ],
    }
    receipt_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"result": "PASS", "geometry": str(geometry_path),
                      "receipt": str(receipt_path),
                      "vertices": len(capture["vertices"]), "faces": len(capture["faces"]),
                      "corners": receipt["native_capture"]["associated_packet_corners"],
                      "geometry_sha256": receipt["geometry_json_sha256"]}, indent=2))
    return 0


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="Consume a fresh archived ENG1 mesh trace with the modern geometry diagnostic.")
    parser.add_argument("--repo-root", type=Path, default=ROOT,
                        help="checkout root; defaults to the fixture's current checkout")
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--eng1", required=True, type=Path)
    parser.add_argument("--archive", required=True, type=Path)
    parser.add_argument("--geometry-cli", required=True, type=Path,
                        help="already-built geometry_diagnostic_cli executable")
    parser.add_argument("--gpu-probe", type=Path,
                        help="optional SDL3 offscreen probe receiving the first validated FT4 face")
    parser.add_argument("--output", type=Path, default=Path("work/dropin_mesh/replay"),
                        help="new output path below the checkout work directory")
    parser.add_argument("--scale", type=int, default=6,
                        help="private reprojection integer 4:3 scale (this fixture currently requires 6)")
    args = parser.parse_args(argv)
    if args.scale != 6:
        parser.error("the finite archived-mesh consumer is pinned to scale 6 (1920x1440)")
    try:
        return run(args)
    except (OSError, ValueError, RuntimeError, KeyError, TypeError, json.JSONDecodeError) as error:
        print(json.dumps({"result": "FAIL", "error": str(error)}, indent=2), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
