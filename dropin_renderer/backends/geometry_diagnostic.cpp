#include "geometry_diagnostic.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <unordered_set>

namespace hercules {
namespace dropin {
namespace {

const std::uint32_t kMaximumVertices = 64;
const std::uint32_t kMaximumFaces = 4096;

bool fail(std::string *error_message, const std::string &message)
{
    if (error_message != nullptr)
        *error_message = message;
    return false;
}

bool is_sha256(const std::string &value)
{
    if (value.size() != 64)
        return false;
    for (char ch : value) {
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')))
            return false;
    }
    return true;
}

renderer_i16 signed_word(renderer_u16 value)
{
    const renderer_u32 raw = static_cast<renderer_u32>(value);
    const renderer_i32 signed_value = (raw & 0x8000U) != 0
        ? static_cast<renderer_i32>(raw) - 0x10000
        : static_cast<renderer_i32>(raw);
    return static_cast<renderer_i16>(signed_value);
}

void write_i16_array(std::ostringstream &out,
                     const std::array<renderer_i16, 2> &value)
{
    out << '[' << static_cast<int>(value[0]) << ','
        << static_cast<int>(value[1]) << ']';
}

void write_i16_array3(std::ostringstream &out,
                      const std::array<renderer_i16, 3> &value)
{
    out << '[' << static_cast<int>(value[0]) << ','
        << static_cast<int>(value[1]) << ','
        << static_cast<int>(value[2]) << ']';
}

void write_i32_array3(std::ostringstream &out,
                      const std::array<renderer_i32, 3> &value)
{
    out << '[' << value[0] << ',' << value[1] << ',' << value[2] << ']';
}

void write_double_array(std::ostringstream &out,
                        const std::array<double, 2> &value)
{
    out << '[' << value[0] << ',' << value[1] << ']';
}

const GeometryDiagnosticVertex *find_vertex(
    const std::vector<GeometryDiagnosticVertex> &vertices,
    std::uint32_t source_point_index)
{
    const auto found = std::lower_bound(
        vertices.begin(), vertices.end(), source_point_index,
        [](const GeometryDiagnosticVertex &vertex, std::uint32_t index) {
            return vertex.source_point_index < index;
        });
    if (found == vertices.end() || found->source_point_index != source_point_index)
        return nullptr;
    return &*found;
}

}  // namespace

bool build_geometry_diagnostic(const CapturedMeshInput &input,
                               GeometryDiagnosticResult *result,
                               std::string *error_message)
{
    if (result == nullptr)
        return fail(error_message, "geometry diagnostic result pointer is null");
    *result = GeometryDiagnosticResult{};
    if (error_message != nullptr)
        error_message->clear();

    if (!is_sha256(input.native_report_sha256) ||
        !is_sha256(input.manifest_sha256) ||
        !is_sha256(input.archive_sha256) ||
        !is_sha256(input.asset_sha256))
        return fail(error_message, "input provenance must contain four lowercase SHA-256 values");
    if (input.mesh_index > 0x3ffU)
        return fail(error_message, "mesh index exceeds the encoded ENG1 mesh-index domain");
    if (input.map_byte_length < 4 ||
        input.map_cell_file_offset >= input.map_byte_length)
        return fail(error_message, "mesh cell offset is outside the archived map extent");
    if (input.effective_winding > 1)
        return fail(error_message, "effective winding must be zero or one");
    if (input.resolution_scale == 0 || input.resolution_scale > HERCULES_REPROJECTION_MAX_SCALE)
        return fail(error_message, "resolution scale is outside the reprojection policy domain");
    if (input.vertices.empty() || input.vertices.size() > kMaximumVertices)
        return fail(error_message, "mesh point count must be in the 1..64 packed-index domain");
    if (input.faces.empty() || input.faces.size() > kMaximumFaces)
        return fail(error_message, "face count must be in the 1..4096 diagnostic bound");

    std::vector<const CapturedMeshVertex *> ordered_input;
    ordered_input.reserve(input.vertices.size());
    std::unordered_set<std::uint32_t> point_indices;
    point_indices.reserve(input.vertices.size());
    for (const CapturedMeshVertex &vertex : input.vertices) {
        if (vertex.source_point_index >= kMaximumVertices)
            return fail(error_message, "source point index exceeds the packed six-bit face domain");
        if (!point_indices.insert(vertex.source_point_index).second)
            return fail(error_message, "source point index is duplicated");
        if (vertex.source_file_offset > input.map_byte_length - 4U)
            return fail(error_message, "source point offset is outside the archived map extent");
        if (vertex.input_scratch_slot != vertex.source_point_index ||
            vertex.output_scratch_slot != vertex.source_point_index ||
            vertex.input_scratch_slot != vertex.output_scratch_slot)
            return fail(error_message, "B6F0/B720 scratch slots do not preserve the source point identity");
        if (!(vertex.source_read_sequence <= vertex.b6f0_callback_sequence &&
              vertex.b6f0_callback_sequence < vertex.b720_callback_sequence))
            return fail(error_message, "source-read/B6F0/B720 event order is inconsistent");

        const renderer_i32 expected_x = input.effective_winding != 0
            ? static_cast<renderer_i32>(vertex.source_xyz[0])
            : 64 - static_cast<renderer_i32>(vertex.source_xyz[0]);
        if (vertex.b6f0_input_xyz[0] != expected_x ||
            vertex.b6f0_input_xyz[1] != vertex.source_xyz[1] ||
            vertex.b6f0_input_xyz[2] != vertex.source_xyz[2])
            return fail(error_message, "B6F0 input words disagree with packed point and winding");
        for (renderer_i32 camera_lane : vertex.captured_camera_xyz) {
            if (camera_lane < -32768 || camera_lane > 32767)
                return fail(error_message, "captured camera lane is not a sign-extended WORD");
        }
        ordered_input.push_back(&vertex);
    }
    std::sort(ordered_input.begin(), ordered_input.end(),
              [](const CapturedMeshVertex *left, const CapturedMeshVertex *right) {
                  return left->source_point_index < right->source_point_index;
              });

    result->native_report_sha256 = input.native_report_sha256;
    result->manifest_sha256 = input.manifest_sha256;
    result->archive_sha256 = input.archive_sha256;
    result->asset_sha256 = input.asset_sha256;
    result->mesh_index = input.mesh_index;
    result->map_cell_file_offset = input.map_cell_file_offset;
    result->resolution_scale = input.resolution_scale;
    result->output_width = HERCULES_REPROJECTION_NATIVE_WIDTH * input.resolution_scale;
    result->output_height = HERCULES_REPROJECTION_NATIVE_HEIGHT * input.resolution_scale;
    result->vertices.reserve(input.vertices.size());

    for (const CapturedMeshVertex *captured : ordered_input) {
        struct hercules_projection_point_view projection_point{};
        struct hercules_projection_result_view compatibility{};
        struct hercules_reprojection_private_vertex_view private_view{};
        GeometryDiagnosticVertex vertex{};

        for (std::size_t axis = 0; axis < 3; ++axis)
            projection_point.xyz[axis] = captured->b6f0_input_xyz[axis];
        hercules_projection_eval(&captured->config, &projection_point, &compatibility);

        if (compatibility.camera_x != captured->captured_camera_xyz[0] ||
            compatibility.camera_y != captured->captured_camera_xyz[1] ||
            compatibility.camera_z != captured->captured_camera_xyz[2])
            return fail(error_message, "shared projection camera result differs from the original B740/B920 capture");
        const std::array<renderer_i16, 2> compatibility_xy{{
            signed_word(compatibility.projected_x),
            signed_word(compatibility.projected_y)}};
        if (compatibility_xy != captured->captured_compatibility_xy)
            return fail(error_message, "shared projection WORD output differs from the original B920 capture");

        const renderer_i32 private_status = hercules_reprojection_from_camera(
            &captured->config,
            captured->captured_camera_xyz[0],
            captured->captured_camera_xyz[1],
            captured->captured_camera_xyz[2],
            input.resolution_scale,
            &private_view);
        if (private_status != HERCULES_REPROJECTION_OK)
            return fail(error_message, "private reprojection rejected captured camera depth or policy input");
        if (!std::isfinite(private_view.x) || !std::isfinite(private_view.y))
            return fail(error_message, "private reprojection produced a non-finite coordinate");

        vertex.source_point_index = captured->source_point_index;
        vertex.source_file_offset = captured->source_file_offset;
        vertex.source_xyz = captured->source_xyz;
        vertex.b6f0_input_xyz = captured->b6f0_input_xyz;
        vertex.camera_xyz = captured->captured_camera_xyz;
        vertex.compatibility_xy = compatibility_xy;
        vertex.private_xy = {{private_view.x, private_view.y}};
        vertex.input_scratch_slot = captured->input_scratch_slot;
        vertex.output_scratch_slot = captured->output_scratch_slot;
        vertex.source_read_sequence = captured->source_read_sequence;
        vertex.b6f0_callback_sequence = captured->b6f0_callback_sequence;
        vertex.b720_callback_sequence = captured->b720_callback_sequence;
        result->vertices.push_back(vertex);
    }

    std::unordered_set<std::uint32_t> face_indices;
    face_indices.reserve(input.faces.size());
    result->faces.reserve(input.faces.size());
    for (const CapturedMeshFace &captured : input.faces) {
        if (!face_indices.insert(captured.face_record_index).second)
            return fail(error_message, "face record index is duplicated");
        if (captured.face_file_offset > input.map_byte_length - 4U)
            return fail(error_message, "face record offset is outside the archived map extent");
        for (std::uint32_t corner = 0; corner < 4; ++corner) {
            if (((captured.packed_index_word >> (6U * corner)) & 0x3fU) !=
                captured.source_point_indices[corner])
                return fail(error_message, "packed face index word differs from ordered source point identities");
        }

        GeometryDiagnosticFace face{};
        face.face_record_index = captured.face_record_index;
        face.face_file_offset = captured.face_file_offset;
        face.packed_index_word = captured.packed_index_word;
        face.source_point_indices = captured.source_point_indices;
        face.native_packet_xy = captured.packet_xy_words;
        face.ot_bucket_index = captured.ot_bucket_index;
        face.packet_tag_previous_link = captured.packet_tag_previous_link;
        for (std::uint32_t corner = 0; corner < 4; ++corner) {
            const GeometryDiagnosticVertex *vertex = find_vertex(
                result->vertices, captured.source_point_indices[corner]);
            if (vertex == nullptr)
                return fail(error_message, "face corner references a missing captured source point");
            if (captured.packet_xy_words[corner] != vertex->compatibility_xy)
                return fail(error_message, "native FT4 packet corner differs from its indexed original point");
            face.corners[corner].corner = corner;
            face.corners[corner].source_point_index = vertex->source_point_index;
            face.corners[corner].source_file_offset = vertex->source_file_offset;
            face.corners[corner].compatibility_xy = vertex->compatibility_xy;
            face.corners[corner].native_packet_xy = captured.packet_xy_words[corner];
            face.corners[corner].private_xy = vertex->private_xy;
        }
        result->faces.push_back(face);
    }
    return true;
}

std::string geometry_diagnostic_json(const GeometryDiagnosticResult &result)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(17);
    out << "{\"schema\":\"hercules.dropin.geometry-diagnostic.v1\""
        << ",\"asset\":\"MAP_PSX.BIN\""
        << ",\"mesh_index\":" << result.mesh_index
        << ",\"map_cell_file_offset\":" << result.map_cell_file_offset
        << ",\"resolution\":{\"policy\":\"4:3 integer-scale camera-space reprojection\""
        << ",\"scale\":" << result.resolution_scale
        << ",\"width\":" << result.output_width
        << ",\"height\":" << result.output_height << '}'
        << ",\"provenance\":{\"native_report_sha256\":\""
        << result.native_report_sha256 << "\",\"manifest_sha256\":\""
        << result.manifest_sha256 << "\",\"archive_sha256\":\""
        << result.archive_sha256 << "\",\"asset_sha256\":\""
        << result.asset_sha256 << "\"}"
        << ",\"projection_sources\":{\"compatibility\":\"hercules_projection_eval, exact captured WORD comparison\""
        << ",\"private\":\"hercules_reprojection_from_camera, captured camera XYZ\"}"
        << ",\"vertices\":[";
    for (std::size_t index = 0; index < result.vertices.size(); ++index) {
        const GeometryDiagnosticVertex &vertex = result.vertices[index];
        if (index != 0)
            out << ',';
        out << "{\"source_point_index\":" << vertex.source_point_index
            << ",\"source_file_offset\":" << vertex.source_file_offset
            << ",\"identity\":\"MAP_PSX.BIN+0x" << std::hex
            << vertex.source_file_offset << std::dec << ":mesh" << result.mesh_index
            << ":point" << vertex.source_point_index << '\"'
            << ",\"source_xyz\":";
        write_i16_array3(out, vertex.source_xyz);
        out << ",\"b6f0_input_xyz\":";
        write_i16_array3(out, vertex.b6f0_input_xyz);
        out << ",\"camera_xyz\":";
        write_i32_array3(out, vertex.camera_xyz);
        out << ",\"compatibility_xy\":";
        write_i16_array(out, vertex.compatibility_xy);
        out << ",\"private_xy\":";
        write_double_array(out, vertex.private_xy);
        out << ",\"association\":{\"input_scratch_slot\":"
            << vertex.input_scratch_slot
            << ",\"output_scratch_slot\":" << vertex.output_scratch_slot
            << ",\"source_read_sequence\":" << vertex.source_read_sequence
            << ",\"b6f0_callback_sequence\":" << vertex.b6f0_callback_sequence
            << ",\"b720_callback_sequence\":" << vertex.b720_callback_sequence
            << "}}";
    }
    out << "],\"faces\":[";
    for (std::size_t index = 0; index < result.faces.size(); ++index) {
        const GeometryDiagnosticFace &face = result.faces[index];
        if (index != 0)
            out << ',';
        out << "{\"face_record_index\":" << face.face_record_index
            << ",\"face_file_offset\":" << face.face_file_offset
            << ",\"packed_index_word\":" << face.packed_index_word
            << ",\"source_point_indices\":["
            << face.source_point_indices[0] << ',' << face.source_point_indices[1] << ','
            << face.source_point_indices[2] << ',' << face.source_point_indices[3] << ']'
            << ",\"ot_bucket_index\":" << face.ot_bucket_index
            << ",\"packet_tag_previous_link\":" << face.packet_tag_previous_link
            << ",\"corners\":[";
        for (std::size_t corner_index = 0; corner_index < face.corners.size(); ++corner_index) {
            const GeometryDiagnosticCorner &corner = face.corners[corner_index];
            if (corner_index != 0)
                out << ',';
            out << "{\"corner\":" << corner.corner
                << ",\"source_point_index\":" << corner.source_point_index
                << ",\"source_file_offset\":" << corner.source_file_offset
                << ",\"compatibility_xy\":";
            write_i16_array(out, corner.compatibility_xy);
            out << ",\"native_packet_xy\":";
            write_i16_array(out, corner.native_packet_xy);
            out << ",\"private_xy\":";
            write_double_array(out, corner.private_xy);
            out << '}';
        }
        out << "]}";
    }
    out << "]}\n";
    return out.str();
}

}  // namespace dropin
}  // namespace hercules
