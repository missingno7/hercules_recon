"""Freshly compare the private reprojection source against the pinned oracle.

The original EXE is executed only as mapped x86 code by the existing bounded
Unicorn runner. This script runs source-built test fixtures, never the game.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import subprocess
import sys


SCRIPT = Path(__file__).resolve()
PACKAGE_DEFAULT = SCRIPT.parent.parent
EXE_DEFAULT = Path(r"D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE")
EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
VC5_DEFAULT = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
VC5_CL_SHA256 = "bf9f9c74f756fed96e13f7f9a4273495c7dde0a1fb968e3ef6d760ad6d73dfeb"
VC5_LINK_SHA256 = "279544809e6353c8e622eb88aea94e86b0258f3c4b8a976373bf4d68a41b63e1"
PYTHON_DEFAULT = Path(r"C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe")
UNICORN_DEPS = Path(r"C:\tools\hercules\renderer-oracle-python")
ANALYSIS_DEPS = Path(r"C:\tools\hercules\python")
GCC_DEFAULT = Path(r"C:\msys64\mingw64\bin")
REAL_RESOURCE_ARCHIVE_SHA256 = "affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4"
REAL_RESOURCE_MAP_SHA256 = "78bc5a60ecce1c99e752fb30a16bc5ace4ee80f686365e1da8136a0decbd69cb"
REAL_RESOURCE_ENG1_SHA256 = "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def discover_root() -> Path:
    for start in (SCRIPT.parent, Path.cwd().resolve()):
        for candidate in (start, *start.parents):
            if (candidate / "scripts" / "pe.py").is_file():
                return candidate.resolve()
    raise RuntimeError("cannot discover checkout root containing scripts/pe.py")


def is_relative_to(path: Path, parent: Path) -> bool:
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
            raise ValueError(f"refusing symlink/junction path component: {current}")


def signed32_expr(value: int) -> str:
    if value == -0x80000000:
        return "(-2147483647L - 1L)"
    if value < 0:
        return f"(-{abs(value)}L)"
    return f"{value}L"


def u32_expr(value: int) -> str:
    return f"0x{value & 0xffffffff:08x}UL"


def config_for_vertex(manifest: dict, vertex: dict) -> dict:
    config = dict(manifest["config"])
    transform = vertex.get("camera_transform", {})
    if "matrix_words_row_major" in transform:
        config["matrix_words_row_major"] = transform["matrix_words_row_major"]
    if "translation_dwords" in transform:
        config["translation_dwords"] = transform["translation_dwords"]
    return config


def resource_source(manifest: dict, native_report: dict) -> str:
    """Build a C fixture for one captured resource mesh, preserving point identity."""
    cfg_default = manifest["config"]
    rows = []
    exact_xy = {}
    private_policy_rows = []
    for vertex in manifest["vertices"]:
        cfg = config_for_vertex(manifest, vertex)
        matrix = ", ".join(str(int(value)) for value in cfg["matrix_words_row_major"])
        translation = ", ".join(signed32_expr(int(value)) for value in cfg["translation_dwords"])
        center_x, center_y = [signed32_expr(int(value)) for value in cfg["center_dwords"]]
        focal = u32_expr(int(cfg["focal_dword"]))
        source = ", ".join(str(int(value))
                            for value in vertex["native_point_input_xyz_signed_words"])
        camera = ", ".join(signed32_expr(int(value))
                             for value in vertex["captured_camera_xyz_dwords"])
        screen = ", ".join(f"{int(value) & 0xffff}U"
                            for value in vertex["captured_screen_xy_words"])
        rows.append(
            "    {%s, {{%s}, {%s}, %s, %s, %s}, {{%s}}, {%s}, {%s}, %s, %s}," %
            (json.dumps(vertex["identity"]), matrix, translation, center_x, center_y,
             focal, source, camera, screen, vertex["private_x_expr"],
             vertex["private_y_expr"]))
        exact_xy[vertex["identity"]] = vertex["captured_screen_xy_words"]
        private_policy_rows.append({"identity": vertex["identity"],
                                   "camera_xyz": vertex["captured_camera_xyz_dwords"],
                                   "native_xy": vertex["captured_screen_xy_words"],
                                   "private_x_rational": vertex["private_x_rational"],
                                   "private_y_rational": vertex["private_y_rational"],
                                   "private_x": vertex["private_x"],
                                   "private_y": vertex["private_y"]})

    selected = native_report["selected_real_resource_case"]
    corner_rows = []
    for face in selected["packets"]:
        for association in face["corner_associations"]:
            point_index = int(association["source_point_index"])
            point_identity = manifest["vertices"][point_index]["identity"]
            packet_xy = [int(value) & 0xffff for value in association["packet_xy_words"]]
            if exact_xy[point_identity] != packet_xy:
                raise RuntimeError("validated packet corner diverged while building C fixture")
            corner_rows.append(
                "    {%dU, %dU, %dU, %dU}," %
                (int(face["face_record_index"]), point_index,
                 packet_xy[0], packet_xy[1]))

    return r'''#include "hercules_reprojection.h"
#include <stdio.h>

struct resource_vertex {
    const char *identity;
    struct hercules_projection_config_view config;
    struct hercules_projection_point_view source;
    renderer_i32 camera_xyz[3];
    renderer_u16 screen_xy[2];
    double private_x;
    double private_y;
};

struct resource_corner {
    unsigned int face;
    unsigned int point;
    renderer_u16 x;
    renderer_u16 y;
};

static const struct resource_vertex vertices[] = {
''' + "\n".join(rows) + r'''
};

static const struct resource_corner corners[] = {
''' + "\n".join(corner_rows) + r'''
};

static renderer_i32 effective_focal(renderer_u32 raw)
{
    renderer_u16 word;
    word = (renderer_u16)raw;
    if (word <= 0x7fffU)
        return (renderer_i32)word;
    return (renderer_i32)(-1L - (renderer_i32)(0xffffU - word));
}

static int near_value(double actual, double expected)
{
    double delta;
    delta = actual - expected;
    if (delta < 0.0)
        delta = -delta;
    return delta <= 0.0000001;
}

int main(void)
{
    struct hercules_projection_result_view compatibility[
        sizeof(vertices) / sizeof(vertices[0])];
    unsigned int i;
    unsigned int native_matches;
    unsigned int private_matches;
    unsigned int corner_matches;

    native_matches = 0U;
    private_matches = 0U;
    corner_matches = 0U;
    for (i = 0U; i != sizeof(vertices) / sizeof(vertices[0]); ++i) {
        struct hercules_reprojection_private_vertex_view private_result;
        renderer_i32 status;
        double expected_x;
        double expected_y;

        hercules_projection_eval(&vertices[i].config, &vertices[i].source,
                                 &compatibility[i]);
        if (compatibility[i].camera_x != vertices[i].camera_xyz[0] ||
            compatibility[i].camera_y != vertices[i].camera_xyz[1] ||
            compatibility[i].camera_z != vertices[i].camera_xyz[2] ||
            compatibility[i].projected_x != vertices[i].screen_xy[0] ||
            compatibility[i].projected_y != vertices[i].screen_xy[1]) {
            fprintf(stderr, "native compatibility mismatch: %s\n", vertices[i].identity);
            return 1;
        }
        ++native_matches;

        status = hercules_reprojection_from_camera(
            &vertices[i].config, vertices[i].camera_xyz[0],
            vertices[i].camera_xyz[1], vertices[i].camera_xyz[2], 6UL,
            &private_result);
        expected_x = (double)vertices[i].config.center_x * 6.0 +
            (double)effective_focal(vertices[i].config.focal) * 6.0 *
            (double)vertices[i].camera_xyz[0] / (double)vertices[i].camera_xyz[2];
        expected_y = (double)vertices[i].config.center_y * 6.0 +
            (double)effective_focal(vertices[i].config.focal) * 6.0 *
            (double)vertices[i].camera_xyz[1] / (double)vertices[i].camera_xyz[2];
        if (status != HERCULES_REPROJECTION_OK ||
            private_result.status != HERCULES_REPROJECTION_OK ||
            private_result.camera_provenance !=
                HERCULES_REPROJECTION_CAMERA_CALLER_SUPPLIED ||
            private_result.camera_x_input != vertices[i].camera_xyz[0] ||
            private_result.camera_y_input != vertices[i].camera_xyz[1] ||
            private_result.camera_z_input != vertices[i].camera_xyz[2] ||
            !near_value(private_result.x, expected_x) ||
            !near_value(private_result.y, expected_y) ||
            !near_value(private_result.x, vertices[i].private_x) ||
            !near_value(private_result.y, vertices[i].private_y)) {
            fprintf(stderr, "private reprojection mismatch: %s\n", vertices[i].identity);
            return 2;
        }
        ++private_matches;
        printf("vertex[%u] %s native=(%u,%u) private=(%.6f,%.6f)\n", i,
               vertices[i].identity, (unsigned int)compatibility[i].projected_x,
               (unsigned int)compatibility[i].projected_y,
               private_result.x, private_result.y);
    }

    for (i = 0U; i != sizeof(corners) / sizeof(corners[0]); ++i) {
        if (corners[i].point >= sizeof(vertices) / sizeof(vertices[0]) ||
            compatibility[corners[i].point].projected_x != corners[i].x ||
            compatibility[corners[i].point].projected_y != corners[i].y) {
            fprintf(stderr, "native packet corner mismatch: face=%u corner=%u\n",
                    corners[i].face, i % 4U);
            return 4;
        }
        ++corner_matches;
    }
    printf("resource_vertices=%u source_compat_exact=%u private_1920x1440=%u faces=%u packet_corners_exact=%u\n",
           (unsigned int)(sizeof(vertices) / sizeof(vertices[0])), native_matches,
           private_matches,
           (unsigned int)(sizeof(corners) / sizeof(corners[0]) / 4U), corner_matches);
    return 0;
}
'''


def validate_real_resource(manifest_path: Path, report_path: Path) -> tuple[dict, dict, dict]:
    """Validate exact raw-read -> B6F0/B740/B920/B720 -> FT4 corner identity."""
    manifest_bytes = manifest_path.read_bytes()
    report_bytes = report_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    report = json.loads(report_bytes)
    manifest_hash = hashlib.sha256(manifest_bytes).hexdigest()
    report_hash = hashlib.sha256(report_bytes).hexdigest()
    provenance = manifest.get("provenance", {})
    route = manifest.get("actual_native_point_callback_route", {})
    if (manifest.get("schema") != "hercules.eng1-real-resource-captured-geometry.v2" or
            provenance.get("fixture_report_sha256") != report_hash or
            provenance.get("exe_hash") != EXE_SHA256 or
            provenance.get("eng1_hash") != REAL_RESOURCE_ENG1_SHA256 or
            provenance.get("asset_hash") != REAL_RESOURCE_MAP_SHA256 or
            provenance.get("archive_hash") != REAL_RESOURCE_ARCHIVE_SHA256 or
            provenance.get("map_mesh_index") != 156 or
            provenance.get("map_call_index") != 2 or
            provenance.get("map_cell_file_offset") != 1102652):
        raise RuntimeError("v2 manifest provenance does not bind its native receipt/resource")
    if (route.get("mesh_producer") != "ENG1+0x2DF20" or
            route.get("point_input_callback") != "ENG1+0x2C690 -> host slot101 HERC+0xB6F0" or
            route.get("point_projection_callback") != "ENG1+0x2C6E0 -> host slot106 HERC+0xB920" or
            route.get("point_output_callback") != "ENG1+0x2C760 -> host slot114 HERC+0xB720" or
            route.get("packet_callback") != "ENG1+0x2C770 FT4" or
            route.get("point_batch_count") != 8 or
            "prefetch the next source record before B720" not in route.get("read_to_output_rule", "")):
        raise RuntimeError("v2 manifest does not name the measured callback/producer route")
    hashes = report.get("hashes", {})
    if (hashes.get("HERCULES.EXE") != EXE_SHA256 or
            hashes.get("ENG1.DLL") != REAL_RESOURCE_ENG1_SHA256 or
            hashes.get("HERCULES.FS") != REAL_RESOURCE_ARCHIVE_SHA256 or
            hashes.get("MAP_PSX.BIN") != REAL_RESOURCE_MAP_SHA256 or
            report.get("faults") != [] or report.get("result") != "EXECUTED" or
            report.get("mesh_call_count") != 87):
        raise RuntimeError("v2 native receipt inputs or bounded execution result changed")

    selected = report.get("selected_real_resource_case", {})
    mesh = selected.get("mesh", {})
    vertices = manifest.get("vertices", [])
    point_outputs = selected.get("point_outputs", [])
    if (selected.get("mesh_call_index") != 2 or mesh.get("mesh_index") != 156 or
            mesh.get("packed_cell") != "0x938" or mesh.get("cell_file_offset") != 1102652 or
            mesh.get("emit") != "ft4" or mesh.get("point_count") != 8 or
            mesh.get("face_count") != 3 or len(vertices) != 8 or len(point_outputs) != 8):
        raise RuntimeError("v2 native receipt selected mesh/counts changed")

    events = report.get("association_events_selected_call", [])
    reads = {int(row["source_point_index"]): row for row in events
             if row.get("kind") == "source_point_read"}
    inputs = {int(row["source_point_index"]): row for row in events
              if row.get("kind") == "b6f0_input"}
    outputs = {int(row["source_point_index_from_b6f0"]): row for row in events
               if row.get("kind") == "b720_output"}
    point_records = {int(row["index"]): row for row in mesh.get("source_points", [])}
    point_outputs_by_index = {int(row["source_point_index"]): row for row in point_outputs}
    if any(len(mapping) != 8 for mapping in (reads, inputs, outputs,
                                              point_records, point_outputs_by_index)):
        raise RuntimeError("v2 report omits source-read, callback, output, or point-record identities")

    cfg_default = manifest["config"]
    from fractions import Fraction
    private_vertices = []
    identities = set()
    for index, vertex in enumerate(vertices):
        identity = vertex.get("identity", "")
        if not identity or identity in identities or vertex.get("source_point_index") != index:
            raise RuntimeError("v2 source point identities are missing, duplicated, or reordered")
        identities.add(identity)
        source_read = vertex.get("source_read", {})
        b6f0 = vertex.get("b6f0_input", {})
        b720 = vertex.get("b720_output", {})
        transform = vertex.get("camera_transform", {})
        read_event, input_event, output_event = reads[index], inputs[index], outputs[index]
        record, captured = point_records[index], point_outputs_by_index[index]

        expected_file_offset = 12372 + 4 * index
        expected_source_address = 0x0d803054 + 4 * index
        expected_input_address = 0x00ba2600 + 8 * index
        source_pc_rva = int(source_read["pc_rva"].split("+")[-1], 16)
        expected_source_pc = 0x10000000 + source_pc_rva
        if (source_read.get("file_offset") != expected_file_offset or
                source_read.get("size") != 4 or
                int(source_read.get("packed_dword", "0"), 16) != int(record["packed_dword"], 16) or
                vertex.get("asset_point_xyz_signed_words") != record.get("source_xyz") or
                int(read_event.get("source_read_pc", "0"), 16) != expected_source_pc or
                int(read_event.get("source_read_address", "0"), 16) != expected_source_address or
                read_event.get("source_read_size") != 4 or
                read_event.get("sequence") != b6f0.get("source_read_sequence")):
            raise RuntimeError(f"packed source read/address/bytes do not identify {identity}")

        native_input = vertex.get("native_point_input_xyz_signed_words")
        expected_native_input = [64 - int(vertex["asset_point_xyz_signed_words"][0]),
                                 int(vertex["asset_point_xyz_signed_words"][1]),
                                 int(vertex["asset_point_xyz_signed_words"][2])]
        input_return_pc = 0x10000000 + int(b6f0["return_pc_rva"].split("+")[-1], 16)
        output_return_pc = 0x10000000 + int(b720["return_pc_rva"].split("+")[-1], 16)
        if ("64-X" not in vertex.get("input_derivation", "") or
                native_input != expected_native_input or
                native_input != captured.get("host_input_signed_words") or
                native_input != captured.get("camera_transform", {}).get("input_signed_words") or
                native_input != transform.get("input_signed_words") or
                native_input != vertex.get("projection", {}).get("camera_input_signed_words")):
            raise RuntimeError(f"native source preparation/B6F0 input derivation failed for {identity}")
        if (b6f0.get("pc_rva") != "HERC+0xB6F0" or
                b6f0.get("input_scratch_slot") != index or
                b6f0.get("input_scratch_offset") != f"+0x{index * 8:x}" or
                b6f0.get("source_read_sequence") != read_event.get("sequence") or
                input_event.get("sequence") != b6f0.get("callback_sequence") or
                int(input_event.get("host_input_address", "0"), 16) != expected_input_address or
                int(input_event.get("host_callback_return_pc", "0"), 16) != input_return_pc or
                int(captured.get("source_address", "0"), 16) != expected_source_address or
                int(captured.get("source_packed_dword", "0"), 16) != int(source_read["packed_dword"], 16) or
                int(captured.get("source_read_pc", "0"), 16) != expected_source_pc or
                int(captured.get("host_input_address", "0"), 16) != expected_input_address):
            raise RuntimeError(f"packed record to slot-101 B6F0 input chain failed for {identity}")
        if (b720.get("pc_rva") != "HERC+0xB720" or
                b720.get("output_scratch_slot") != index or
                b720.get("output_scratch_offset") != f"+0x{index * 8:x}" or
                b720.get("reads_from_b6f0_input_slot") != index or
                output_event.get("source_point_index_from_b6f0") != index or
                output_event.get("sequence") != b720.get("callback_sequence") or
                int(output_event.get("host_input_address_from_b6f0", "0"), 16) != expected_input_address or
                int(output_event.get("scratch_output_address", "0"), 16) != expected_input_address or
                int(output_event.get("host_callback_return_pc", "0"), 16) != output_return_pc or
                int(captured.get("scratch_output_address", "0"), 16) != expected_input_address):
            raise RuntimeError(f"slot-106 projection to same-slot slot-114 output chain failed for {identity}")
        if (b6f0.get("source_read_sequence") >= b6f0.get("callback_sequence") or
                b6f0.get("callback_sequence") >= b720.get("callback_sequence")):
            raise RuntimeError(f"callback order is not read -> B6F0 -> B720 for {identity}")
        if index < 7:
            next_read_sequence = vertices[index + 1]["b6f0_input"]["source_read_sequence"]
            if not (b6f0["callback_sequence"] < next_read_sequence < b720["callback_sequence"]):
                raise RuntimeError("next-point prefetch timing no longer matches the v2 B720 attribution rule")

        if (vertex.get("captured_camera_xyz_dwords") != captured.get("camera_xyz_signed") or
                vertex.get("captured_screen_xy_words") != captured.get("projected_xy_words")):
            raise RuntimeError(f"captured camera/XY differ from native output for {identity}")
        native_transform = captured.get("camera_transform", {})
        if (cfg_default.get("matrix_words_row_major") != transform.get("matrix_words_row_major") or
                cfg_default.get("translation_dwords") != transform.get("translation_dwords") or
                cfg_default.get("matrix_words_row_major") != native_transform.get("matrix_words_row_major") or
                cfg_default.get("translation_dwords") != native_transform.get("translation_dwords")):
            raise RuntimeError(f"per-point B740 matrix/translation differ for {identity}")
        if int(vertex["captured_camera_xyz_dwords"][2]) <= 1:
            raise RuntimeError(f"real captured point {identity} is outside the positive post-clamp test domain")

        cfg = config_for_vertex(manifest, vertex)
        focal_word = int(cfg["focal_dword"]) & 0xffff
        effective_focal_value = focal_word if focal_word <= 0x7fff else focal_word - 0x10000
        camera_x, camera_y, camera_z = [int(value) for value in vertex["captured_camera_xyz_dwords"]]
        x_exact = Fraction(int(cfg["center_dwords"][0]) * 6 * camera_z +
                            effective_focal_value * 6 * camera_x, camera_z)
        y_exact = Fraction(int(cfg["center_dwords"][1]) * 6 * camera_z +
                            effective_focal_value * 6 * camera_y, camera_z)
        vertex["private_x_rational"] = f"{x_exact.numerator}/{x_exact.denominator}"
        vertex["private_y_rational"] = f"{y_exact.numerator}/{y_exact.denominator}"
        vertex["private_x"] = float(x_exact)
        vertex["private_y"] = float(y_exact)
        vertex["private_x_expr"] = format(float(x_exact), ".17g")
        vertex["private_y_expr"] = format(float(y_exact), ".17g")
        private_vertices.append({"identity": identity,
                                 "camera_xyz": [camera_x, camera_y, camera_z],
                                 "native_xy": list(vertex["captured_screen_xy_words"]),
                                 "private_x_rational": vertex["private_x_rational"],
                                 "private_y_rational": vertex["private_y_rational"],
                                 "private_x": vertex["private_x"],
                                 "private_y": vertex["private_y"]})

    report_faces = mesh.get("source_faces", [])
    manifest_faces = manifest.get("faces_and_packets", [])
    report_packets = selected.get("packets", [])
    if not (len(report_faces) == len(manifest_faces) == len(report_packets) == 3):
        raise RuntimeError("native map sample must retain three source faces and FT4 packets")
    corner_assertions = 0
    for face, manifest_face, packet in zip(report_faces, manifest_faces, report_packets):
        point_indices = face.get("point_indices", [])
        if (manifest_face.get("face_record_index") != face.get("index") or
                manifest_face.get("face_file_offset") != face.get("file_offset") or
                manifest_face.get("source_point_indices") != point_indices or
                packet.get("face_file_offset") != face.get("file_offset") or
                packet.get("face_point_indices") != point_indices or
                packet.get("route") != "ft4"):
            raise RuntimeError("native face record, decoded indices, and FT4 packet do not agree")
        captured_corners = packet.get("corner_associations", [])
        manifest_corners = manifest_face.get("corner_associations", [])
        if len(captured_corners) != 4 or len(manifest_corners) != 4:
            raise RuntimeError("FT4 packet must retain four index-addressed corner joins")
        for corner, (native_corner, manifest_corner) in enumerate(zip(captured_corners, manifest_corners)):
            point_index = int(point_indices[corner])
            vertex = vertices[point_index]
            point_address = 0x0d803054 + 4 * point_index
            expected_address = point_address
            expected_xy = vertex["captured_screen_xy_words"]
            if (native_corner.get("corner") != corner or
                    native_corner.get("source_point_index") != point_index or
                    int(native_corner.get("source_address_file_offset", "0"), 16) != expected_address or
                    native_corner.get("projected_xy_words") != expected_xy or
                    native_corner.get("packet_xy_words") != expected_xy or
                    manifest_corner.get("corner") != corner or
                    manifest_corner.get("source_point_index") != point_index or
                    int(manifest_corner.get("source_address_file_offset", "0"), 16) != expected_address or
                    manifest_corner.get("projected_xy_words") != expected_xy or
                    manifest_corner.get("packet_xy_words") != expected_xy):
                raise RuntimeError("FT4 corner failed address/index-to-native-projection join")
            corner_assertions += 1
    if (corner_assertions != 12 or
            manifest.get("association_summary", {}).get("corners_associated") != 12 or
            manifest.get("association_summary", {}).get("packets_emitted") != 3):
        raise RuntimeError("real resource sample did not preserve all 12 corners of its 3 FT4 faces")

    derived = {"manifest_path": str(manifest_path), "manifest_sha256": manifest_hash,
               "native_report_path": str(report_path), "native_report_sha256": report_hash,
               "resource_asset_sha256": REAL_RESOURCE_MAP_SHA256,
               "archive_sha256": REAL_RESOURCE_ARCHIVE_SHA256,
               "eng1_sha256": REAL_RESOURCE_ENG1_SHA256,
               "selected_mesh_index": 156, "selected_cell": "0x938",
               "selected_cell_file_offset": 1102652, "mesh_call_index": 2,
               "native_route": route,
               "point_count": 8, "face_count": 3, "packet_corner_assertions": corner_assertions,
               "capture_config_provenance": cfg_default["config_provenance"],
               "association_summary": manifest.get("association_summary"),
               "source_c_producer_differential": "PENDING; source C producer is not the acceptance claim. The C fixture here reprojects measured post-transform inputs and compares them to native captured output.",
               "private_policy": "1920x1440 uses scale six from the 320x240 compatibility view; center and signed effective focal WORD are scaled while captured camera XYZ are retained. Native screen WORDs are comparison-only.",
               "vertices": private_vertices,
               "faces": [{"face_record_index": int(packet["face_record_index"]),
                          "face_file_offset": int(packet["face_file_offset"]),
                          "point_indices": list(packet["face_point_indices"]),
                          "packet_xy_words_by_corner": list(packet["packet_xy_words_by_corner"])}
                         for packet in report_packets]}
    return manifest, report, derived


def vector_source(cases: list[dict], manifest: dict) -> str:
    rows = []
    for case in cases:
        native = case["native_result"]
        matrix = ", ".join(str(int(value)) for value in case["matrix_words_row_major"])
        translation = ", ".join(signed32_expr(int(value)) for value in case["translation_dwords"])
        center = [signed32_expr(int(value)) for value in case["center_dwords"]]
        points = ", ".join(str(int(value)) for value in case["input_xyz_words"])
        camera = ", ".join(signed32_expr(int(value)) for value in native["camera_xyz"])
        screen = ", ".join(f"{int(value) & 0xffff}U" for value in native["projected_xy_words"])
        raw_eax = u32_expr(int(native["return_eax"]))
        focal = u32_expr(int(case["focal_dword"], 16))
        rows.append(
            "    {\"%s\", {{%s}, {%s}, %s, %s, %s}, {{%s}}, {%s}, {%s}, %s}," %
            (case["name"], matrix, translation, center[0], center[1], focal,
             points, camera, screen, raw_eax))

    cfg = manifest["config"]
    matrix = ", ".join(str(int(value)) for value in cfg["matrix_words_row_major"])
    translation = ", ".join(signed32_expr(int(value)) for value in cfg["translation_dwords"])
    center = [signed32_expr(int(value)) for value in cfg["center_dwords"]]
    focal = u32_expr(int(cfg["focal_dword"]))
    captured_rows = []
    for vertex in manifest["vertices"]:
        source = ", ".join(str(int(value)) for value in vertex["source_xyz_words"])
        camera = ", ".join(signed32_expr(int(value))
                             for value in vertex["captured_camera_xyz_signed_low_words"])
        screen = ", ".join(f"{int(value) & 0xffff}U"
                            for value in vertex["captured_screen_xy_words"])
        captured_rows.append(
            "    {\"%s\", \"%s\", {{%s}, {%s}, %s, %s, %s}, {%s}, {%s}, {%s}}," %
            (vertex["identity"], vertex["oracle_case"], matrix, translation,
             center[0], center[1], focal, source, camera, screen))
    collision_flag = 1 if manifest.get("precision_collision") else 0

    return r'''#include "hercules_reprojection.h"
#include <stdio.h>

struct oracle_case {
    const char *name;
    struct hercules_projection_config_view config;
    struct hercules_projection_point_view point;
    renderer_i32 camera_xyz[3];
    renderer_u16 screen_xy[2];
    renderer_raw32 raw_eax;
};

struct captured_vertex {
    const char *identity;
    const char *oracle_case;
    struct hercules_projection_config_view config;
    renderer_i16 source_xyz[3];
    renderer_i32 camera_xyz[3];
    renderer_u16 screen_xy[2];
};

static const struct oracle_case cases[] = {
''' + "\n".join(rows) + r'''
};

static const struct captured_vertex captured[] = {
''' + "\n".join(captured_rows) + r'''
};
static const int expect_capture_collision = ''' + str(collision_flag) + r''';

static renderer_i32 effective_focal(renderer_u32 raw)
{
    renderer_u16 word;
    word = (renderer_u16)raw;
    if (word <= 0x7fffU)
        return (renderer_i32)word;
    return (renderer_i32)(-1L - (renderer_i32)(0xffffU - word));
}

static int near_value(double actual, double expected)
{
    double delta;
    delta = actual - expected;
    if (delta < 0.0)
        delta = -delta;
    return delta <= 0.0000001;
}

int main(void)
{
    unsigned int index;
    unsigned int compatibility_matches;
    unsigned int private_positive;
    unsigned int private_rejected;
    unsigned int captured_camera_ok;
    unsigned int manifest_camera_ok;
    unsigned int manifest_index;

    compatibility_matches = 0U;
    private_positive = 0U;
    private_rejected = 0U;
    captured_camera_ok = 0U;
    manifest_camera_ok = 0U;
    for (index = 0U; index != sizeof(cases) / sizeof(cases[0]); ++index) {
        struct hercules_projection_result_view compatibility;
        struct hercules_reprojection_private_vertex_view private_result;
        struct hercules_reprojection_private_vertex_view captured_result;
        renderer_i32 status;
        renderer_i32 captured_status;
        renderer_i32 expected_status;
        double expected_x;
        double expected_y;

        status = hercules_reprojection_eval_single(
            &cases[index].config, &cases[index].point, 6UL,
            &compatibility, &private_result);
        if (compatibility.camera_x != cases[index].camera_xyz[0] ||
            compatibility.camera_y != cases[index].camera_xyz[1] ||
            compatibility.camera_z != cases[index].camera_xyz[2] ||
            compatibility.projected_x != cases[index].screen_xy[0] ||
            compatibility.projected_y != cases[index].screen_xy[1] ||
            compatibility.raw_eax != cases[index].raw_eax) {
            fprintf(stderr, "native compatibility mismatch: %s\n", cases[index].name);
            return 1;
        }
        if (private_result.camera_provenance !=
                HERCULES_REPROJECTION_CAMERA_DERIVED_B320_REFERENCE) {
            fprintf(stderr, "camera provenance mismatch: %s\n", cases[index].name);
            return 2;
        }
        if (private_result.camera_z_input == 0L)
            expected_status = HERCULES_REPROJECTION_ZERO_DEPTH;
        else if (private_result.camera_z_input < 0L)
            expected_status = HERCULES_REPROJECTION_BEHIND_CAMERA;
        else
            expected_status = HERCULES_REPROJECTION_OK;
        if (status != expected_status || private_result.status != expected_status) {
            fprintf(stderr, "depth status mismatch: %s status=%ld private=%ld expected=%ld\n",
                    cases[index].name, (long)status,
                    (long)private_result.status, (long)expected_status);
            return 4;
        }
        if (expected_status == HERCULES_REPROJECTION_OK) {
            expected_x = (double)cases[index].config.center_x * 6.0 +
                (double)effective_focal(cases[index].config.focal) * 6.0 *
                (double)private_result.camera_x_input /
                (double)private_result.camera_z_input;
            expected_y = (double)cases[index].config.center_y * 6.0 +
                (double)effective_focal(cases[index].config.focal) * 6.0 *
                (double)private_result.camera_y_input /
                (double)private_result.camera_z_input;
            if (!near_value(private_result.x, expected_x) ||
                !near_value(private_result.y, expected_y)) {
                fprintf(stderr, "private math mismatch: %s\n", cases[index].name);
                return 8;
            }
            ++private_positive;
        } else {
            if (private_result.x != 0.0 || private_result.y != 0.0) {
                fprintf(stderr, "rejected point kept coordinates: %s\n", cases[index].name);
                return 16;
            }
            ++private_rejected;
        }
        captured_status = hercules_reprojection_from_camera(
            &cases[index].config, cases[index].camera_xyz[0],
            cases[index].camera_xyz[1], cases[index].camera_xyz[2],
            6UL, &captured_result);
        if (captured_result.camera_provenance !=
                HERCULES_REPROJECTION_CAMERA_CALLER_SUPPLIED ||
            captured_result.camera_x_input != cases[index].camera_xyz[0] ||
            captured_result.camera_y_input != cases[index].camera_xyz[1] ||
            captured_result.camera_z_input != cases[index].camera_xyz[2] ||
            captured_status != HERCULES_REPROJECTION_OK ||
            captured_result.status != HERCULES_REPROJECTION_OK) {
            fprintf(stderr, "captured camera path mismatch: %s\n", cases[index].name);
            return 32;
        }
        expected_x = (double)cases[index].config.center_x * 6.0 +
            (double)effective_focal(cases[index].config.focal) * 6.0 *
            (double)cases[index].camera_xyz[0] /
            (double)cases[index].camera_xyz[2];
        expected_y = (double)cases[index].config.center_y * 6.0 +
            (double)effective_focal(cases[index].config.focal) * 6.0 *
            (double)cases[index].camera_xyz[1] /
            (double)cases[index].camera_xyz[2];
        if (!near_value(captured_result.x, expected_x) ||
            !near_value(captured_result.y, expected_y)) {
            fprintf(stderr, "captured camera math mismatch: %s\n", cases[index].name);
            return 64;
        }
        ++captured_camera_ok;
        ++compatibility_matches;
    }
    for (manifest_index = 0U;
         manifest_index != sizeof(captured) / sizeof(captured[0]);
         ++manifest_index) {
        struct hercules_reprojection_private_vertex_view captured_result;
        renderer_i32 status;
        renderer_i32 expected_status;
        double expected_x;
        double expected_y;

        status = hercules_reprojection_from_camera(
            &captured[manifest_index].config,
            captured[manifest_index].camera_xyz[0],
            captured[manifest_index].camera_xyz[1],
            captured[manifest_index].camera_xyz[2], 6UL, &captured_result);
        if (captured[manifest_index].camera_xyz[2] == 0L)
            expected_status = HERCULES_REPROJECTION_ZERO_DEPTH;
        else if (captured[manifest_index].camera_xyz[2] < 0L)
            expected_status = HERCULES_REPROJECTION_BEHIND_CAMERA;
        else
            expected_status = HERCULES_REPROJECTION_OK;
        if (status != expected_status ||
            captured_result.status != expected_status ||
            captured_result.camera_provenance !=
                HERCULES_REPROJECTION_CAMERA_CALLER_SUPPLIED ||
            captured_result.camera_x_input != captured[manifest_index].camera_xyz[0] ||
            captured_result.camera_y_input != captured[manifest_index].camera_xyz[1] ||
            captured_result.camera_z_input != captured[manifest_index].camera_xyz[2]) {
            fprintf(stderr, "manifest camera ingestion mismatch: %s\n",
                    captured[manifest_index].identity);
            return 128;
        }
        if (expected_status == HERCULES_REPROJECTION_OK) {
            expected_x = (double)captured[manifest_index].config.center_x * 6.0 +
                (double)effective_focal(captured[manifest_index].config.focal) * 6.0 *
                (double)captured[manifest_index].camera_xyz[0] /
                (double)captured[manifest_index].camera_xyz[2];
            expected_y = (double)captured[manifest_index].config.center_y * 6.0 +
                (double)effective_focal(captured[manifest_index].config.focal) * 6.0 *
                (double)captured[manifest_index].camera_xyz[1] /
                (double)captured[manifest_index].camera_xyz[2];
            if (!near_value(captured_result.x, expected_x) ||
                !near_value(captured_result.y, expected_y)) {
                fprintf(stderr, "manifest private projection mismatch: %s\n",
                        captured[manifest_index].identity);
                return 256;
            }
        } else if (captured_result.x != 0.0 || captured_result.y != 0.0) {
            fprintf(stderr, "manifest rejected depth kept coordinates: %s\n",
                    captured[manifest_index].identity);
            return 1024;
        }
        ++manifest_camera_ok;
    }
    if (expect_capture_collision &&
        sizeof(captured) / sizeof(captured[0]) >= 2U) {
        struct hercules_reprojection_private_vertex_view first;
        struct hercules_reprojection_private_vertex_view second;
        if (captured[0].screen_xy[0] != captured[1].screen_xy[0] ||
            captured[0].camera_xyz[0] == captured[1].camera_xyz[0] ||
            hercules_reprojection_from_camera(
                &captured[0].config, captured[0].camera_xyz[0],
                captured[0].camera_xyz[1], captured[0].camera_xyz[2],
                6UL, &first) != HERCULES_REPROJECTION_OK ||
            hercules_reprojection_from_camera(
                &captured[1].config, captured[1].camera_xyz[0],
                captured[1].camera_xyz[1], captured[1].camera_xyz[2],
                6UL, &second) != HERCULES_REPROJECTION_OK ||
            first.x == second.x) {
            fprintf(stderr, "manifest precision collision did not separate\n");
            return 512;
        }
    }
    printf("fresh original cases=%u compatibility_exact=%u private_positive=%u private_depth_rejections=%u captured_camera_ok=%u manifest_camera_ok=%u\n",
           (unsigned int)(sizeof(cases) / sizeof(cases[0])),
           compatibility_matches, private_positive, private_rejected,
           captured_camera_ok, manifest_camera_ok);
    return 0;
}
'''


def run_command(argv: list[str], cwd: Path, env: dict | None = None) -> dict:
    completed = subprocess.run(argv, cwd=cwd, env=env, capture_output=True,
                               text=True, errors="replace", timeout=120)
    record = {"argv": argv, "returncode": completed.returncode,
              "stdout": completed.stdout, "stderr": completed.stderr}
    if completed.returncode != 0:
        raise RuntimeError(json.dumps(record, indent=2))
    return record


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path,
                        help="checkout containing scripts/pe.py; discovered by default")
    parser.add_argument("--package", type=Path, default=PACKAGE_DEFAULT,
                        help="private source package; must be under renderer_contract/ or work/")
    parser.add_argument("--exe", type=Path, default=EXE_DEFAULT,
                        help="read-only original HERCULES.EXE with pinned SHA-256")
    parser.add_argument("--output", type=Path, required=True,
                        help="fresh nonexistent directory strictly under checkout work/")
    parser.add_argument("--vc5", type=Path, default=VC5_DEFAULT)
    parser.add_argument("--python", type=Path, default=PYTHON_DEFAULT,
                        help="pinned Python used by the existing original callback runner")
    parser.add_argument("--gcc-bin", type=Path, default=GCC_DEFAULT,
                        help="GCC/G++ directory for strict C89/C++98 cross-checks")
    parser.add_argument("--geometry-manifest", type=Path,
                        help="pointer-free captured geometry JSON; use the real-resource manifest to replay an archived map mesh")
    parser.add_argument("--geometry-report", type=Path,
                        help="native producer receipt paired with --geometry-manifest (defaults to its sibling receipt)")
    args = parser.parse_args()
    args.geometry_manifest_explicit = args.geometry_manifest is not None
    args.repo_root = (args.repo_root.resolve() if args.repo_root else discover_root())
    if not (args.repo_root / "scripts" / "pe.py").is_file():
        parser.error("--repo-root must contain scripts/pe.py")
    package_arg = args.package if args.package.is_absolute() else args.repo_root / args.package
    args.package = Path(os.path.abspath(package_arg))
    allowed = (args.repo_root / "renderer_contract", args.repo_root / "work")
    if not any(is_relative_to(args.package, root) for root in allowed):
        parser.error("--package must be under checkout renderer_contract/ or work/")
    try:
        reject_reparse_components(args.package)
    except ValueError as error:
        parser.error(str(error))
    args.exe = args.exe.resolve()
    if not args.exe.is_file() or sha256_file(args.exe) != EXE_SHA256:
        parser.error("--exe is missing or does not match pinned original EXE hash")
    output_arg = args.output if args.output.is_absolute() else args.repo_root / args.output
    args.output = Path(os.path.abspath(output_arg))
    work_root = Path(os.path.abspath(args.repo_root / "work"))
    if not is_relative_to(args.output, work_root) or args.output == work_root:
        parser.error("--output must be strictly inside checkout work/")
    try:
        reject_reparse_components(args.output)
    except ValueError as error:
        parser.error(str(error))
    if args.output.exists():
        parser.error("--output must be a fresh nonexistent directory")
    resolved_output = args.output.resolve(strict=False)
    resolved_work = work_root.resolve(strict=False)
    if not is_relative_to(resolved_output, resolved_work) or resolved_output == resolved_work:
        parser.error("resolved --output must remain strictly under checkout work/")
    protected = [args.exe.parent.resolve(strict=False),
                 (args.repo_root / "assets").resolve(strict=False)]
    if any(args.output == root or is_relative_to(args.output, root) or
           is_relative_to(root, args.output) for root in protected):
        parser.error("--output overlaps the original-install or assets tree")
    args.vc5 = args.vc5.resolve()
    args.python = args.python.resolve()
    args.gcc_bin = args.gcc_bin.resolve()
    manifest_arg = (args.geometry_manifest if args.geometry_manifest is not None else
                    args.package / "evidence" / "synthetic_captured_geometry.json")
    if not manifest_arg.is_absolute():
        manifest_arg = args.repo_root / manifest_arg
    args.geometry_manifest = Path(os.path.abspath(manifest_arg))
    if not args.geometry_manifest.is_file():
        parser.error("--geometry-manifest does not exist")
    if not any(is_relative_to(args.geometry_manifest, root) for root in allowed):
        parser.error("--geometry-manifest must be under checkout renderer_contract/ or work/")
    try:
        reject_reparse_components(args.geometry_manifest)
    except ValueError as error:
        parser.error(str(error))
    if args.geometry_report is not None:
        report_arg = args.geometry_report if args.geometry_report.is_absolute() else args.repo_root / args.geometry_report
        args.geometry_report = Path(os.path.abspath(report_arg))
        if not args.geometry_report.is_file():
            parser.error("--geometry-report does not exist")
        if not any(is_relative_to(args.geometry_report, root) for root in allowed):
            parser.error("--geometry-report must be under checkout renderer_contract/ or work/")
        try:
            reject_reparse_components(args.geometry_report)
        except ValueError as error:
            parser.error(str(error))
    for path, expected in ((args.vc5 / "bin" / "cl.exe", VC5_CL_SHA256),
                           (args.vc5 / "bin" / "link.exe", VC5_LINK_SHA256)):
        if not path.is_file() or sha256_file(path) != expected:
            parser.error(f"pinned VC5 tool missing or hash mismatch: {path}")
    if not args.python.is_file():
        parser.error("pinned Python executable does not exist")
    if not (args.gcc_bin / "gcc.exe").is_file() or not (args.gcc_bin / "g++.exe").is_file():
        parser.error("expected GCC and G++ in --gcc-bin")
    args.oracle_runner = args.repo_root / "renderer_contract" / "tests" / "test_original_projection.py"
    for filename in ("hercules_reprojection.h", "hercules_reprojection.c",
                     "tests/test_reprojection.c", "tests/test_reprojection_cpp.cpp"):
        if not (args.package / filename).is_file():
            parser.error(f"missing package source: {args.package / filename}")
    return args


def main() -> int:
    args = parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    oracle_output = args.output / "native_oracle"
    command = [str(args.python), "-B", str(args.oracle_runner),
               "--repo-root", str(args.repo_root),
               "--package", str(args.repo_root / "renderer_contract"),
               "--exe", str(args.exe), "--output", str(oracle_output)]
    oracle_run = run_command(command, args.repo_root)
    report_path = oracle_output / "report.json"
    report_bytes = report_path.read_bytes()
    report = json.loads(report_bytes)
    if (report.get("status") != "BOUNDED_X86_DIFFERENTIAL_AND_BRIDGE_OBSERVED" or
            report.get("oracle", {}).get("sha256") != EXE_SHA256 or
            len(report.get("cases", [])) != 14 or
            report.get("runtime", {}).get("original_windows_process_started") is not False):
        raise RuntimeError("fresh bounded native oracle report is incomplete or mismatched")

    generated = args.output / "candidate_vectors.c"
    synthetic_manifest_path = args.package / "evidence" / "synthetic_captured_geometry.json"
    manifest_path = synthetic_manifest_path
    manifest = json.loads(synthetic_manifest_path.read_bytes())
    real_manifest = None
    real_report = None
    real_capture = None
    real_manifest_path = None
    real_report_path = None
    if args.geometry_manifest_explicit:
        requested_manifest = json.loads(args.geometry_manifest.read_bytes())
        if requested_manifest.get("kind") == "synthetic_original_projection_cases":
            manifest_path = args.geometry_manifest
            manifest = requested_manifest
        elif requested_manifest.get("schema") == "hercules.eng1-real-resource-captured-geometry.v2":
            real_manifest_path = args.geometry_manifest
            real_report_path = (args.geometry_report if args.geometry_report is not None else
                                args.geometry_manifest.with_name("native_real_map_report.json"))
            real_manifest, real_report, real_capture = validate_real_resource(
                real_manifest_path, real_report_path)
        else:
            raise RuntimeError("--geometry-manifest has an unsupported capture schema")
    if (manifest.get("schema_version") != 1 or
            manifest.get("kind") != "synthetic_original_projection_cases" or
            manifest.get("capture_route") != "direct_B920_result" or
            manifest.get("provenance", {}).get("original_exe_sha256") != EXE_SHA256):
        raise RuntimeError("package synthetic fixture is not the pinned original-case schema")
    config = manifest.get("config", {})
    if (len(config.get("matrix_words_row_major", [])) != 9 or
            len(config.get("translation_dwords", [])) != 3 or
            len(config.get("center_dwords", [])) != 2 or
            not isinstance(config.get("focal_dword"), int)):
        raise RuntimeError("geometry manifest has invalid joined projection config")
    oracle_by_name = {row["name"]: row for row in report["cases"]}
    identities = set()
    for vertex in manifest.get("vertices", []):
        identity = vertex.get("identity", "")
        case_name = vertex.get("oracle_case", "")
        if (not identity or identity in identities or
                not identity.replace("_", "").isalnum() or case_name not in oracle_by_name):
            raise RuntimeError("geometry manifest has an invalid/duplicate identity or oracle case")
        identities.add(identity)
        if (len(vertex.get("source_xyz_words", [])) != 3 or
                len(vertex.get("captured_camera_xyz_signed_low_words", [])) != 3 or
                len(vertex.get("captured_screen_xy_words", [])) != 2):
            raise RuntimeError(f"geometry manifest vertex {identity} has invalid lane counts")
        oracle_case = oracle_by_name[case_name]
        native_case = oracle_case["native_result"]
        if (vertex["source_xyz_words"] != oracle_case["input_xyz_words"] or
                vertex["captured_camera_xyz_signed_low_words"] != native_case["camera_xyz"] or
                vertex["captured_screen_xy_words"] != native_case["projected_xy_words"] or
                config["matrix_words_row_major"] != oracle_case["matrix_words_row_major"] or
                config["translation_dwords"] != oracle_case["translation_dwords"] or
                config["center_dwords"] != oracle_case["center_dwords"] or
                config["focal_dword"] != int(oracle_case["focal_dword"], 16)):
            raise RuntimeError(f"geometry manifest vertex {identity} differs from its fresh oracle case")
    if len(identities) < 2:
        raise RuntimeError("synthetic geometry manifest must retain at least two point identities")
    generated.write_text(vector_source(report["cases"], manifest), encoding="ascii", newline="\n")
    resource_generated = None
    if real_manifest is not None and real_report is not None:
        resource_generated = args.output / "candidate_real_resource.c"
        resource_generated.write_text(resource_source(real_manifest, real_report),
                                      encoding="ascii", newline="\n")
    candidate_output = args.output / "candidate"
    candidate_output.mkdir()
    compiler = args.vc5 / "bin" / "cl.exe"
    linker = args.vc5 / "bin" / "link.exe"
    env = os.environ.copy()
    for key in ("CL", "_CL_", "LINK", "_LINK_"):
        env.pop(key, None)
    env["PATH"] = str(args.vc5 / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(args.vc5 / "include")
    env["LIB"] = str(args.vc5 / "lib")
    env["TEMP"] = str(candidate_output)
    env["TMP"] = str(candidate_output)
    include_dirs = [args.package, args.repo_root / "renderer_contract"]
    c_sources = [args.repo_root / "renderer_contract" / "hercules_projection.c",
                 args.package / "hercules_reprojection.c",
                 args.package / "tests" / "test_reprojection.c",
                 generated]
    vc5_commands = []
    objects: dict[str, Path] = {}
    for source in c_sources:
        key = source.stem + ("_oracle" if source == generated else "")
        obj = candidate_output / (key + ".obj")
        argv = [str(compiler), "/nologo", "/c", "/O2", "/Za", "/W3",
                *["/I" + str(path) for path in include_dirs],
                "/Fo" + str(obj), str(source)]
        vc5_commands.append(run_command(argv, args.output, env))
        objects[key] = obj

    for test_name, keys in (("test_reprojection.exe", ("hercules_projection",
                                                         "hercules_reprojection",
                                                         "test_reprojection")),
                            ("test_original_vectors.exe", ("hercules_projection",
                                                            "hercules_reprojection",
                                                            "candidate_vectors_oracle"))):
        exe = candidate_output / test_name
        argv = [str(linker), "/nologo", "/SUBSYSTEM:CONSOLE", "/INCREMENTAL:NO",
                "/OUT:" + str(exe), *[str(objects[key]) for key in keys]]
        vc5_commands.append(run_command(argv, args.output, env))
        vc5_commands.append(run_command([str(exe)], candidate_output, env))

    vc5_resource_run = None
    if resource_generated is not None:
        resource_obj = candidate_output / "candidate_real_resource.obj"
        argv = [str(compiler), "/nologo", "/c", "/O2", "/Za", "/W3",
                *["/I" + str(path) for path in include_dirs],
                "/Fo" + str(resource_obj), str(resource_generated)]
        vc5_commands.append(run_command(argv, args.output, env))
        resource_exe = candidate_output / "test_real_resource.exe"
        argv = [str(linker), "/nologo", "/SUBSYSTEM:CONSOLE", "/INCREMENTAL:NO",
                "/OUT:" + str(resource_exe), str(objects["hercules_projection"]),
                str(objects["hercules_reprojection"]), str(resource_obj)]
        vc5_commands.append(run_command(argv, args.output, env))
        vc5_resource_run = run_command([str(resource_exe)], candidate_output, env)

    cpp_source = args.package / "tests" / "test_reprojection_cpp.cpp"
    cpp_obj = candidate_output / "test_reprojection_cpp.obj"
    argv = [str(compiler), "/nologo", "/c", "/O2", "/Za", "/W3",
            *["/I" + str(path) for path in include_dirs],
            "/Fo" + str(cpp_obj), str(cpp_source)]
    vc5_commands.append(run_command(argv, args.output, env))
    cpp_exe = candidate_output / "test_reprojection_cpp.exe"
    argv = [str(linker), "/nologo", "/SUBSYSTEM:CONSOLE", "/INCREMENTAL:NO",
            "/OUT:" + str(cpp_exe), str(objects["hercules_projection"]),
            str(objects["hercules_reprojection"]), str(cpp_obj)]
    vc5_commands.append(run_command(argv, args.output, env))
    vc5_commands.append(run_command([str(cpp_exe)], candidate_output, env))

    gcc_output = args.output / "gcc"
    gcc_output.mkdir()
    gcc = args.gcc_bin / "gcc.exe"
    gxx = args.gcc_bin / "g++.exe"
    core = [str(args.repo_root / "renderer_contract" / "hercules_projection.c"),
            str(args.package / "hercules_reprojection.c")]
    gcc_c = run_command([str(gcc), "-std=c89", "-pedantic-errors", "-Wall", "-Wextra",
                         "-Werror", *["-I" + str(path) for path in include_dirs],
                         *core, str(args.package / "tests" / "test_reprojection.c"),
                         "-o", str(gcc_output / "test_reprojection.exe")], args.output)
    gcc_test = run_command([str(gcc_output / "test_reprojection.exe")], gcc_output)
    gcc_vectors = run_command([str(gcc), "-std=c89", "-pedantic-errors", "-Wall", "-Wextra",
                               "-Werror", *["-I" + str(path) for path in include_dirs],
                               *core, str(generated), "-o", str(gcc_output / "test_original_vectors.exe")],
                              args.output)
    gcc_oracle = run_command([str(gcc_output / "test_original_vectors.exe")], gcc_output)
    gcc_cpp = run_command([str(gxx), "-std=c++98", "-pedantic-errors", "-Wall", "-Wextra",
                           "-Werror", *["-I" + str(path) for path in include_dirs],
                           *core, str(cpp_source), "-o", str(gcc_output / "test_reprojection_cpp.exe")],
                          args.output)
    gcc_cpp_test = run_command([str(gcc_output / "test_reprojection_cpp.exe")], gcc_output)
    gcc_resource_build = None
    gcc_resource_run = None
    if resource_generated is not None:
        gcc_resource_build = run_command(
            [str(gcc), "-std=c89", "-pedantic-errors", "-Wall", "-Wextra", "-Werror",
             *["-I" + str(path) for path in include_dirs], *core,
             str(resource_generated), "-o", str(gcc_output / "test_real_resource.exe")],
            args.output)
        gcc_resource_run = run_command([str(gcc_output / "test_real_resource.exe")], gcc_output)

    collision = {row["name"]: row for row in report["cases"]
                 if row["name"] in ("precision_camera_x_one", "precision_camera_x_two")}
    if set(collision) != {"precision_camera_x_one", "precision_camera_x_two"}:
        raise RuntimeError("fresh native oracle report lacks both precision-collision cases")
    native_a = collision["precision_camera_x_one"]["native_result"]
    native_b = collision["precision_camera_x_two"]["native_result"]
    if (native_a["projected_xy_words"] != native_b["projected_xy_words"] or
            native_a["camera_xyz"][0] == native_b["camera_xyz"][0]):
        raise RuntimeError("fresh native oracle did not reproduce distinct camera X / equal screen XY")

    candidate_native_summary = next(
        (record["stdout"] for record in vc5_commands
         if "fresh original cases=" in record["stdout"]), None)
    candidate_gcc_summary = gcc_oracle["stdout"]
    summary_pattern = re.compile(
        r"fresh original cases=(\d+) compatibility_exact=(\d+) "
        r"private_positive=(\d+) private_depth_rejections=(\d+) "
        r"captured_camera_ok=(\d+) manifest_camera_ok=(\d+)")
    native_summary_match = summary_pattern.search(candidate_native_summary or "")
    gcc_summary_match = summary_pattern.search(candidate_gcc_summary)
    if native_summary_match is None or gcc_summary_match is None:
        raise RuntimeError("candidate vector fixtures did not report their native comparison counts")
    native_summary = tuple(int(value) for value in native_summary_match.groups())
    gcc_summary = tuple(int(value) for value in gcc_summary_match.groups())
    if native_summary != (14, 14, 11, 3, 14, len(manifest["vertices"])) or gcc_summary != native_summary:
        raise RuntimeError(f"unexpected candidate vector counts: VC5={native_summary}, GCC={gcc_summary}")

    resource_summary = None
    if resource_generated is not None:
        resource_pattern = re.compile(
            r"resource_vertices=(\d+) source_compat_exact=(\d+) "
            r"private_1920x1440=(\d+) faces=(\d+) packet_corners_exact=(\d+)")
        vc5_resource_match = resource_pattern.search((vc5_resource_run or {}).get("stdout", ""))
        gcc_resource_match = resource_pattern.search((gcc_resource_run or {}).get("stdout", ""))
        if vc5_resource_match is None or gcc_resource_match is None:
            raise RuntimeError("resource candidate fixture did not report native/private/corner counts")
        vc5_resource_summary = tuple(int(value) for value in vc5_resource_match.groups())
        gcc_resource_summary = tuple(int(value) for value in gcc_resource_match.groups())
        if vc5_resource_summary != (8, 8, 8, 3, 12) or gcc_resource_summary != vc5_resource_summary:
            raise RuntimeError(f"unexpected real resource counts: VC5={vc5_resource_summary}, GCC={gcc_resource_summary}")
        real_capture["compiled_fixture_counts"] = {
            "resource_vertices": vc5_resource_summary[0],
            "source_compatibility_exact": vc5_resource_summary[1],
            "private_1920x1440_projected": vc5_resource_summary[2],
            "faces": vc5_resource_summary[3],
            "packet_corners_exact": vc5_resource_summary[4]}
        real_capture["vc5_resource_fixture_stdout"] = vc5_resource_run["stdout"]
        real_capture["gcc_resource_fixture_stdout"] = gcc_resource_run["stdout"]

    inputs = [args.package / "hercules_reprojection.h",
              args.package / "hercules_reprojection.c",
              args.package / "tests" / "test_reprojection.c",
              args.package / "tests" / "test_reprojection_cpp.cpp",
              args.repo_root / "renderer_contract" / "hercules_projection.h",
              args.repo_root / "renderer_contract" / "hercules_projection.c",
              args.repo_root / "renderer_contract" / "hercules_types.h",
              args.repo_root / "renderer_contract" / "hercules_host_abi.h",
              args.package / "evidence" / "reprojection_prediction.json",
              args.package / "reprojection_policy.md",
              manifest_path,
              args.oracle_runner, SCRIPT, generated,
              args.repo_root / "toolchains" / "manifest.json"]
    if real_manifest_path is not None and real_report_path is not None:
        inputs.extend([real_manifest_path, real_report_path, resource_generated])
    geometry_scenarios = [{
        "path": str(manifest_path),
        "sha256": sha256_file(manifest_path),
        "kind": manifest["kind"],
        "capture_route": manifest["capture_route"],
        "vertices": [row["identity"] for row in manifest["vertices"]],
        "native_case_match": True,
        "pointer_free": True}]
    if real_capture is not None:
        geometry_scenarios.append({
            "path": real_capture["manifest_path"],
            "sha256": real_capture["manifest_sha256"],
            "kind": "resource_backed_original_native_trace",
            "native_report": real_capture["native_report_path"],
            "native_report_sha256": real_capture["native_report_sha256"],
            "resource_sha256": real_capture["resource_asset_sha256"],
            "archive_sha256": real_capture["archive_sha256"],
            "selected_mesh_index": real_capture["selected_mesh_index"],
            "selected_cell": real_capture["selected_cell"],
            "vertices": [row["identity"] for row in real_capture["vertices"]],
            "native_compatibility_exact": True,
            "private_policy_scale": 6,
            "packet_corner_assertions": real_capture["packet_corner_assertions"],
            "source_c_producer_differential": real_capture["source_c_producer_differential"],
            "pointer_free": True})
    resource_fixture = None
    if resource_generated is not None and real_capture is not None:
        resource_fixture = {
            "generated_source": str(resource_generated),
            "generated_source_sha256": sha256_file(resource_generated),
            "vc5_build_and_run": vc5_resource_run,
            "vc5_binary_sha256": sha256_file(candidate_output / "test_real_resource.exe"),
            "gcc_build": gcc_resource_build,
            "gcc_run": gcc_resource_run,
            "gcc_binary_sha256": sha256_file(gcc_output / "test_real_resource.exe"),
            "counts": real_capture["compiled_fixture_counts"]}
    report_out = {
        "schema_version": 1,
        "status": ("FRESH_ORIGINAL_COMPARISON_POLICY_AND_RESOURCE_CAPTURE_PASS"
                   if real_capture is not None else
                   "FRESH_ORIGINAL_COMPARISON_AND_POLICY_TESTS_PASS"),
        "scope": ("Fourteen fresh original scalar-projection cases feed a source-built C89 candidate; scalar compatibility fields and raw EAX match exactly. A separate resource-backed original ENG1/PC traversal receipt binds eight archived mesh points and three native FT4 packets; source-built scalar projection matches the captured camera/screen values, and the private 1920x1440 policy uses those captured camera values. The separate emitter-C comparison is outside this projection replay."
                  if real_capture is not None else
                  "Fourteen fresh original scalar-projection cases feed a source-built C89 candidate; scalar compatibility fields and raw EAX match exactly. The same-run direct native camera XYZ/config values are also passed to the caller-supplied reprojection API. Separately, scalar helper pre-clamp lanes are labelled B320-reference-derived, not native scalar captures."),
        "prediction": "For each fresh oracle input, the shared C89 scalar compatibility result equals original B6F0/B740/B920/B720 camera, screen WORD and raw-EAX outputs. The private output uses camera XYZ and signed low-16 effective focal, computes 1920x1440 coordinates without native screen WORDs, retains the x=1/x=2 collision distinction, and rejects nonpositive derived depth.",
        "falsifier": "Any compatibility field or raw-EAX mismatch; any candidate run using stale/edited native report, screen WORD scaling, provenance confusion, high-resolution collision, undefined arithmetic, silent nonpositive depth acceptance, or compiler/link/test failure.",
        "oracle": {"path": str(args.exe), "sha256": sha256_file(args.exe),
                   "native_report": str(report_path),
                   "native_report_sha256": hashlib.sha256(report_bytes).hexdigest(),
                   "native_status": report["status"], "native_cases": len(report["cases"]),
                   "native_assertions": report.get("summary", {}).get("assertion_count"),
                   "native_toolchain": report.get("toolchain"),
                   "native_runtime": report.get("runtime"),
                   "original_windows_process_started": False},
        "resource_capture": real_capture,
        "candidate": {"generated_vectors_source": str(generated),
                      "generated_vectors_sha256": sha256_file(generated),
                      "generated_from_native_report_sha256": hashlib.sha256(report_bytes).hexdigest(),
                      "vc5_candidate_commands": vc5_commands,
                      "vc5_candidate_artifacts": {
                          name: {"path": str(candidate_output / name),
                                "sha256": sha256_file(candidate_output / name)}
                          for name in ("test_reprojection.exe", "test_original_vectors.exe",
                                       "test_reprojection_cpp.exe")},
                      "gcc_c89_source_test": gcc_c,
                      "gcc_c89_source_test_run": gcc_test,
                      "gcc_c89_oracle_vectors_build": gcc_vectors,
                      "gcc_c89_oracle_vectors_run": gcc_oracle,
                      "gcc_cxx98_linkage_build": gcc_cpp,
                      "gcc_cxx98_linkage_run": gcc_cpp_test,
                      "gcc_candidate_artifacts": {
                          name: {"path": str(gcc_output / name),
                                "sha256": sha256_file(gcc_output / name)}
                          for name in ("test_reprojection.exe", "test_original_vectors.exe",
                                       "test_reprojection_cpp.exe")},
                      "native_compatibility_exact_case_count": 14,
                      "vc5_generated_fixture_counts": {
                          "case_count": native_summary[0],
                          "native_compatibility_exact": native_summary[1],
                          "private_positive": native_summary[2],
                          "private_depth_rejections": native_summary[3],
                          "caller_supplied_native_camera_projection": native_summary[4],
                          "pointer_free_manifest_camera_projection": native_summary[5]},
                      "gcc_generated_fixture_counts": {
                          "case_count": gcc_summary[0],
                          "native_compatibility_exact": gcc_summary[1],
                          "private_positive": gcc_summary[2],
                          "private_depth_rejections": gcc_summary[3],
                          "caller_supplied_native_camera_projection": gcc_summary[4],
                          "pointer_free_manifest_camera_projection": gcc_summary[5]},
                      "resource_fixture": resource_fixture,
                      "geometry_scenarios": geometry_scenarios,
                      "precision_collision": {
                          "native_camera_x": [native_a["camera_xyz"][0], native_b["camera_xyz"][0]],
                          "native_screen_xy": [native_a["projected_xy_words"], native_b["projected_xy_words"]],
                          "private_1920x1440_x_rational": ["961.536", "963.072"],
                          "native_screen_scaling_used": False}},
        "toolchain": {"vc5": str(args.vc5),
                       "cl_sha256": sha256_file(args.vc5 / "bin" / "cl.exe"),
                       "link_sha256": sha256_file(args.vc5 / "bin" / "link.exe"),
                       "gcc": str(gcc), "gcc_sha256": sha256_file(gcc),
                       "g++": str(gxx), "g++_sha256": sha256_file(gxx)},
        "source_sha256": {str(path.relative_to(args.repo_root)): sha256_file(path)
                          for path in inputs},
        "limitations": [
            "The high-resolution formula is a deliberate double-precision tool-side policy, not a recovered native viewport equation.",
            "Single-point native scalar output clamps Z to 1 before the wrapper can capture it; this test labels pre-clamp values derived by the separate B320 reference.",
            "A batch zero-Z path uses native undivided focal<<16 for its screen WORD, while private high-resolution output rejects Z=0.",
            "Input source points remain signed WORDs; no precision beyond them is recovered.",
            "No visibility, clipping, wider camera bounds, pixel renderer, or live-scene behavior is established.",
            ("Resource-backed output is limited to one finite archived mesh/camera context with measured preparation state; upstream preparation/traversal C equivalence remains unverified." if real_capture is not None else
             "Real-resource reprojection requires the explicit native receipt/manifest input; no resource geometry is inferred from the synthetic collision fixture.")],
    }
    report_path_out = args.output / "report.json"
    report_path_out.write_text(json.dumps(report_out, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": report_out["status"], "native_cases": 14,
                      "resource_vertices": (8 if real_capture is not None else 0),
                      "oracle_report_sha256": report_out["oracle"]["native_report_sha256"],
                      "report": str(report_path_out),
                      "report_sha256": sha256_file(report_path_out)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
