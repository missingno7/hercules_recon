#ifndef HERCULES_DROPIN_GEOMETRY_DIAGNOSTIC_H
#define HERCULES_DROPIN_GEOMETRY_DIAGNOSTIC_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "hercules_reprojection.h"

namespace hercules {
namespace dropin {

/* Pointer-free records copied from one freshly executed ENG1 geometry
 * manifest. The Python public runner validates archive/report hashes and the
 * original read -> B6F0 -> B720 -> face/packet chain before producing this
 * small input format. */
struct CapturedMeshVertex {
    std::uint32_t source_point_index = 0;
    std::uint32_t source_file_offset = 0;
    std::array<renderer_i16, 3> source_xyz{{0, 0, 0}};
    std::array<renderer_i16, 3> b6f0_input_xyz{{0, 0, 0}};
    std::array<renderer_i32, 3> captured_camera_xyz{{0, 0, 0}};
    std::array<renderer_i16, 2> captured_compatibility_xy{{0, 0}};
    std::uint32_t input_scratch_slot = 0;
    std::uint32_t output_scratch_slot = 0;
    std::uint32_t source_read_sequence = 0;
    std::uint32_t b6f0_callback_sequence = 0;
    std::uint32_t b720_callback_sequence = 0;
    hercules_projection_config_view config{};
};

struct CapturedMeshFace {
    std::uint32_t face_record_index = 0;
    std::uint32_t face_file_offset = 0;
    std::uint32_t packed_index_word = 0;
    std::array<std::uint32_t, 4> source_point_indices{{0, 0, 0, 0}};
    std::array<std::array<renderer_i16, 2>, 4> packet_xy_words{};
    std::uint32_t ot_bucket_index = 0;
    std::uint32_t packet_tag_previous_link = 0;
};

struct CapturedMeshInput {
    std::string native_report_sha256;
    std::string manifest_sha256;
    std::string archive_sha256;
    std::string asset_sha256;
    std::uint32_t mesh_index = 0;
    std::uint32_t map_cell_file_offset = 0;
    std::uint32_t map_byte_length = 0;
    std::uint8_t effective_winding = 0;
    std::uint32_t resolution_scale = 6;
    std::vector<CapturedMeshVertex> vertices;
    std::vector<CapturedMeshFace> faces;
};

struct GeometryDiagnosticVertex {
    std::uint32_t source_point_index = 0;
    std::uint32_t source_file_offset = 0;
    std::array<renderer_i16, 3> source_xyz{{0, 0, 0}};
    std::array<renderer_i16, 3> b6f0_input_xyz{{0, 0, 0}};
    std::array<renderer_i32, 3> camera_xyz{{0, 0, 0}};
    std::array<renderer_i16, 2> compatibility_xy{{0, 0}};
    std::array<double, 2> private_xy{{0.0, 0.0}};
    std::uint32_t input_scratch_slot = 0;
    std::uint32_t output_scratch_slot = 0;
    std::uint32_t source_read_sequence = 0;
    std::uint32_t b6f0_callback_sequence = 0;
    std::uint32_t b720_callback_sequence = 0;
};

struct GeometryDiagnosticCorner {
    std::uint32_t corner = 0;
    std::uint32_t source_point_index = 0;
    std::uint32_t source_file_offset = 0;
    std::array<renderer_i16, 2> compatibility_xy{{0, 0}};
    std::array<renderer_i16, 2> native_packet_xy{{0, 0}};
    std::array<double, 2> private_xy{{0.0, 0.0}};
};

struct GeometryDiagnosticFace {
    std::uint32_t face_record_index = 0;
    std::uint32_t face_file_offset = 0;
    std::uint32_t packed_index_word = 0;
    std::array<std::uint32_t, 4> source_point_indices{{0, 0, 0, 0}};
    std::array<std::array<renderer_i16, 2>, 4> native_packet_xy{};
    std::uint32_t ot_bucket_index = 0;
    std::uint32_t packet_tag_previous_link = 0;
    std::array<GeometryDiagnosticCorner, 4> corners{};
};

struct GeometryDiagnosticResult {
    std::string native_report_sha256;
    std::string manifest_sha256;
    std::string archive_sha256;
    std::string asset_sha256;
    std::uint32_t mesh_index = 0;
    std::uint32_t map_cell_file_offset = 0;
    std::uint32_t resolution_scale = 0;
    std::uint32_t output_width = 0;
    std::uint32_t output_height = 0;
    std::vector<GeometryDiagnosticVertex> vertices;
    std::vector<GeometryDiagnosticFace> faces;
};

/* Computes the finite captured geometry view. It reruns the shared native
 * compatibility projection from each recorded B6F0 input, requires exact
 * equality with captured B740/B920 camera and WORD outputs, then computes a
 * separate floating-point view from camera XYZ. It does not clip or rasterize. */
bool build_geometry_diagnostic(const CapturedMeshInput &input,
                               GeometryDiagnosticResult *result,
                               std::string *error_message);

/* Stable JSON with fixed field order and 17 significant digits for private
 * coordinates. All input hashes and face/packet associations are retained. */
std::string geometry_diagnostic_json(const GeometryDiagnosticResult &result);

}  // namespace dropin
}  // namespace hercules

#endif
