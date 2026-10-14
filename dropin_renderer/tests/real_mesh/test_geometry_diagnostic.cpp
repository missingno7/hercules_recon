#include "geometry_diagnostic.h"

#include <array>
#include <cmath>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const char *message, int *failures)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++*failures;
    }
}

hercules_projection_config_view synthetic_config()
{
    hercules_projection_config_view config{};
    config.matrix_row_major[0] = 4096;
    config.matrix_row_major[4] = 4096;
    config.matrix_row_major[8] = 4096;
    config.translation_xyz[2] = 2048;
    config.center_x = 160;
    config.center_y = 120;
    config.focal = 256;
    return config;
}

hercules::dropin::CapturedMeshInput synthetic_mesh_input()
{
    using hercules::dropin::CapturedMeshFace;
    using hercules::dropin::CapturedMeshInput;
    using hercules::dropin::CapturedMeshVertex;

    const std::array<std::array<renderer_i16, 3>, 5> source{{
        {{-64, -48, 0}}, {{64, -48, 0}}, {{64, 48, 0}},
        {{-64, 48, 0}}, {{0, 0, 0}},
    }};
    const std::array<std::array<renderer_i32, 3>, 5> camera{{
        {{-64, -48, 2048}}, {{64, -48, 2048}}, {{64, 48, 2048}},
        {{-64, 48, 2048}}, {{0, 0, 2048}},
    }};
    const std::array<std::array<renderer_i16, 2>, 5> compatibility{{
        {{152, 114}}, {{168, 114}}, {{168, 126}}, {{152, 126}}, {{160, 120}},
    }};
    const std::array<std::array<std::uint32_t, 4>, 2> face_points{{
        {{0, 1, 4, 3}}, {{1, 2, 3, 4}},
    }};

    CapturedMeshInput result;
    result.native_report_sha256 = std::string(64, 'a');
    result.manifest_sha256 = std::string(64, 'b');
    result.archive_sha256 = std::string(64, 'c');
    result.asset_sha256 = std::string(64, 'd');
    result.mesh_index = 12;
    result.map_cell_file_offset = 0x200;
    result.map_byte_length = 4096;
    result.effective_winding = 1;
    result.resolution_scale = 6;
    for (std::uint32_t index = 0; index < source.size(); ++index) {
        CapturedMeshVertex vertex;
        vertex.source_point_index = index;
        vertex.source_file_offset = 0x100 + index * 12;
        vertex.source_xyz = source[index];
        vertex.b6f0_input_xyz = source[index];
        vertex.captured_camera_xyz = camera[index];
        vertex.captured_compatibility_xy = compatibility[index];
        vertex.input_scratch_slot = index;
        vertex.output_scratch_slot = index;
        vertex.source_read_sequence = index * 3 + 1;
        vertex.b6f0_callback_sequence = index * 3 + 2;
        vertex.b720_callback_sequence = index * 3 + 3;
        vertex.config = synthetic_config();
        result.vertices.push_back(vertex);
    }
    for (std::uint32_t index = 0; index < face_points.size(); ++index) {
        CapturedMeshFace face;
        face.face_record_index = index;
        face.face_file_offset = 0x300 + index * 40;
        face.source_point_indices = face_points[index];
        for (std::size_t corner = 0; corner < 4; ++corner) {
            face.packed_index_word |= face_points[index][corner] << (6 * corner);
            face.packet_xy_words[corner] = compatibility[face_points[index][corner]];
        }
        face.ot_bucket_index = 0x110 + index;
        result.faces.push_back(face);
    }
    return result;
}

void test_synthetic_mesh(int *failures)
{
    const std::array<std::array<double, 2>, 5> expected_private{{
        {{912.0, 684.0}}, {{1008.0, 684.0}}, {{1008.0, 756.0}},
        {{912.0, 756.0}}, {{960.0, 720.0}},
    }};
    const std::array<std::array<std::uint32_t, 4>, 2> expected_faces{{
        {{0, 1, 4, 3}}, {{1, 2, 3, 4}},
    }};
    const std::array<std::array<renderer_i16, 2>, 5> expected_compatibility{{
        {{152, 114}}, {{168, 114}}, {{168, 126}}, {{152, 126}}, {{160, 120}},
    }};

    const auto input = synthetic_mesh_input();
    hercules::dropin::GeometryDiagnosticResult result;
    std::string error;
    require(hercules::dropin::build_geometry_diagnostic(input, &result, &error),
            "the synthetic mesh should match the shared compatibility projection", failures);
    if (result.vertices.size() != 5 || result.faces.size() != 2) {
        require(false, "synthetic output must contain five points and two faces", failures);
        return;
    }
    require(result.output_width == 1920 && result.output_height == 1440,
            "scale six must produce a 1920x1440 policy view", failures);
    for (std::size_t index = 0; index < result.vertices.size(); ++index) {
        const auto &vertex = result.vertices[index];
        require(vertex.source_point_index == index,
                "vertices must be emitted in source point identity order", failures);
        require(vertex.camera_xyz == input.vertices[index].captured_camera_xyz,
                "captured camera lanes must compare exactly", failures);
        require(vertex.compatibility_xy == expected_compatibility[index],
                "native compatibility XY must match independently calculated values", failures);
        require(vertex.private_xy == expected_private[index],
                "private XY must use the unquantized camera lanes and scale six", failures);
    }
    for (std::size_t face_index = 0; face_index < result.faces.size(); ++face_index) {
        const auto &face = result.faces[face_index];
        require(face.source_point_indices == expected_faces[face_index],
                "face order must preserve the synthetic reorder and point reuse", failures);
        for (std::size_t corner = 0; corner < face.corners.size(); ++corner) {
            const auto point_index = expected_faces[face_index][corner];
            require(face.corners[corner].source_point_index == point_index,
                    "packet corner must retain its indexed source point identity", failures);
            require(face.corners[corner].native_packet_xy ==
                    expected_compatibility[point_index],
                    "packet corner WORDs must match the indexed compatibility point", failures);
            require(face.corners[corner].private_xy == result.vertices[point_index].private_xy,
                    "face private coordinate must follow that same point identity", failures);
        }
    }
    const std::string json_first = hercules::dropin::geometry_diagnostic_json(result);
    const std::string json_second = hercules::dropin::geometry_diagnostic_json(result);
    require(json_first == json_second,
            "geometry JSON serialization must be byte deterministic", failures);
    require(json_first.find("\"native_packet_xy\":[152,114]") != std::string::npos,
            "JSON must preserve the exact synthetic packet coordinates", failures);
}

void test_reject_bad_associations(int *failures)
{
    std::string error;
    hercules::dropin::GeometryDiagnosticResult result;
    auto bad_face = synthetic_mesh_input();
    bad_face.faces[0].source_point_indices[0] = 63;
    bad_face.faces[0].packed_index_word =
        (bad_face.faces[0].packed_index_word & ~0x3fU) | 63U;
    require(!hercules::dropin::build_geometry_diagnostic(bad_face, &result, &error) &&
            error.find("missing captured source point") != std::string::npos,
            "face indices outside the captured point set must fail", failures);

    auto bad_packed_index = synthetic_mesh_input();
    bad_packed_index.faces[0].packed_index_word ^= 1U;
    require(!hercules::dropin::build_geometry_diagnostic(bad_packed_index, &result, &error) &&
            error.find("packed face index word") != std::string::npos,
            "packed face words must encode the exact corner point identities", failures);

    auto bad_packet = synthetic_mesh_input();
    ++bad_packet.faces[0].packet_xy_words[0][0];
    require(!hercules::dropin::build_geometry_diagnostic(bad_packet, &result, &error) &&
            error.find("FT4 packet corner") != std::string::npos,
            "packet XY must be checked against its indexed point", failures);

    auto bad_projection = synthetic_mesh_input();
    ++bad_projection.vertices[0].captured_compatibility_xy[0];
    require(!hercules::dropin::build_geometry_diagnostic(bad_projection, &result, &error) &&
            error.find("B920 capture") != std::string::npos,
            "the finite native compatibility comparison must be exact", failures);
}

void test_private_precision_and_depth_domain(int *failures)
{
    hercules_projection_config_view config{};
    config.matrix_row_major[0] = 1;
    config.matrix_row_major[4] = 1;
    config.matrix_row_major[8] = 1;
    config.center_x = 160;
    config.center_y = 120;
    config.focal = 256;

    hercules_projection_point_view point{};
    hercules_projection_result_view compatibility{};
    point.xyz[0] = 1;
    point.xyz[2] = 1000;
    hercules_projection_eval(&config, &point, &compatibility);
    require(compatibility.projected_x == 160 && compatibility.projected_y == 120,
            "first synthetic precision-collision point must round to (160,120)", failures);
    point.xyz[0] = 2;
    hercules_projection_eval(&config, &point, &compatibility);
    require(compatibility.projected_x == 160 && compatibility.projected_y == 120,
            "second synthetic precision-collision point must round to (160,120)", failures);

    hercules_reprojection_private_vertex_view first{};
    hercules_reprojection_private_vertex_view second{};
    const auto first_status = hercules_reprojection_from_camera(&config, 1, 0, 1000, 6, &first);
    const auto second_status = hercules_reprojection_from_camera(&config, 2, 0, 1000, 6, &second);
    require(first_status == HERCULES_REPROJECTION_OK &&
            second_status == HERCULES_REPROJECTION_OK &&
            std::abs(first.x - 961.536) < 1e-9 &&
            std::abs(second.x - 963.072) < 1e-9 && first.x != second.x,
            "private projection must preserve synthetic camera precision lost by compatibility WORDs", failures);

    hercules_reprojection_private_vertex_view rejected{};
    const auto zero_status = hercules_reprojection_from_camera(&config, 0, 0, 0, 6, &rejected);
    require(zero_status == HERCULES_REPROJECTION_ZERO_DEPTH &&
            rejected.x == 0.0 && rejected.y == 0.0,
            "zero camera depth must be rejected with initialized private coordinates", failures);
    const auto behind_status = hercules_reprojection_from_camera(&config, 0, 0, -1, 6, &rejected);
    require(behind_status == HERCULES_REPROJECTION_BEHIND_CAMERA &&
            rejected.x == 0.0 && rejected.y == 0.0,
            "negative camera depth must be rejected with initialized private coordinates", failures);
    const auto bad_scale = hercules_reprojection_from_camera(&config, 0, 0, 1, 65, &rejected);
    require(bad_scale == HERCULES_REPROJECTION_UNSUPPORTED_POLICY,
            "resolution scales beyond the shared policy bound must fail", failures);
}

}  // namespace

int main()
{
    int failures = 0;
    test_synthetic_mesh(&failures);
    test_reject_bad_associations(&failures);
    test_private_precision_and_depth_domain(&failures);
    if (failures != 0)
        return 1;
    std::cout << "PASS synthetic_mesh_points=5 faces=2 precision_and_depth=verified\n";
    return 0;
}
