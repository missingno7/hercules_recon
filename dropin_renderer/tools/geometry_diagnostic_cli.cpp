#include "geometry_diagnostic.h"

#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>

namespace {

bool no_extra_fields(std::istringstream &row)
{
    std::string extra;
    return !(row >> extra);
}

bool read_config(std::istringstream &row,
                 hercules_projection_config_view *config)
{
    if (config == nullptr)
        return false;
    for (std::size_t index = 0; index < 9; ++index) {
        int value = 0;
        if (!(row >> value) || value < -32768 || value > 32767)
            return false;
        config->matrix_row_major[index] = static_cast<renderer_i16>(value);
    }
    for (std::size_t index = 0; index < 3; ++index) {
        long long value = 0;
        if (!(row >> value) || value < -2147483648LL || value > 2147483647LL)
            return false;
        config->translation_xyz[index] = static_cast<renderer_i32>(value);
    }
    long long center_x = 0;
    long long center_y = 0;
    unsigned long long focal = 0;
    if (!(row >> center_x >> center_y >> focal) ||
        center_x < -2147483648LL || center_x > 2147483647LL ||
        center_y < -2147483648LL || center_y > 2147483647LL ||
        focal > 0xffffffffULL)
        return false;
    config->center_x = static_cast<renderer_i32>(center_x);
    config->center_y = static_cast<renderer_i32>(center_y);
    config->focal = static_cast<renderer_u32>(focal);
    return true;
}

bool read_input(const std::string &path,
                hercules::dropin::CapturedMeshInput *input,
                std::string *error)
{
    if (input == nullptr || error == nullptr) {
        if (error != nullptr)
            *error = "invalid parser output pointer";
        return false;
    }
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        *error = "cannot open geometry input";
        return false;
    }
    stream.imbue(std::locale::classic());
    std::string line;
    if (!std::getline(stream, line) || line != "HERCULES_GEOMETRY_DIAGNOSTIC_INPUT_V1") {
        *error = "geometry input header mismatch";
        return false;
    }

    bool have_report_hash = false;
    bool have_manifest_hash = false;
    bool have_archive_hash = false;
    bool have_asset_hash = false;
    bool have_mesh = false;
    std::size_t line_number = 1;
    while (std::getline(stream, line)) {
        ++line_number;
        std::istringstream row(line);
        row.imbue(std::locale::classic());
        std::string kind;
        if (!(row >> kind))
            continue;
        if (kind == "meta") {
            std::string key;
            std::string value;
            if (!(row >> key >> value) || !no_extra_fields(row)) {
                *error = "malformed metadata at line " + std::to_string(line_number);
                return false;
            }
            if (key == "native_report_sha256" && !have_report_hash) {
                input->native_report_sha256 = value;
                have_report_hash = true;
            } else if (key == "manifest_sha256" && !have_manifest_hash) {
                input->manifest_sha256 = value;
                have_manifest_hash = true;
            } else if (key == "archive_sha256" && !have_archive_hash) {
                input->archive_sha256 = value;
                have_archive_hash = true;
            } else if (key == "asset_sha256" && !have_asset_hash) {
                input->asset_sha256 = value;
                have_asset_hash = true;
            } else {
                *error = "unknown or duplicate metadata at line " + std::to_string(line_number);
                return false;
            }
            continue;
        }
        if (kind == "mesh") {
            unsigned long long mesh_index = 0;
            unsigned long long cell_offset = 0;
            unsigned long long map_length = 0;
            unsigned int winding = 0;
            unsigned int scale = 0;
            if (have_mesh || !(row >> mesh_index >> cell_offset >> map_length >> winding >> scale) ||
                !no_extra_fields(row) || mesh_index > 0xffffffffULL ||
                cell_offset > 0xffffffffULL || map_length > 0xffffffffULL ||
                winding > 0xffU || scale > 0xffffffffU) {
                *error = "malformed or duplicate mesh row at line " + std::to_string(line_number);
                return false;
            }
            input->mesh_index = static_cast<std::uint32_t>(mesh_index);
            input->map_cell_file_offset = static_cast<std::uint32_t>(cell_offset);
            input->map_byte_length = static_cast<std::uint32_t>(map_length);
            input->effective_winding = static_cast<std::uint8_t>(winding);
            input->resolution_scale = static_cast<std::uint32_t>(scale);
            have_mesh = true;
            continue;
        }
        if (kind == "vertex") {
            hercules::dropin::CapturedMeshVertex vertex;
            if (!(row >> vertex.source_point_index >> vertex.source_file_offset)) {
                *error = "malformed vertex identity at line " + std::to_string(line_number);
                return false;
            }
            for (renderer_i16 &value : vertex.source_xyz) {
                int parsed = 0;
                if (!(row >> parsed) || parsed < -32768 || parsed > 32767) {
                    *error = "invalid source XYZ at line " + std::to_string(line_number);
                    return false;
                }
                value = static_cast<renderer_i16>(parsed);
            }
            for (renderer_i16 &value : vertex.b6f0_input_xyz) {
                int parsed = 0;
                if (!(row >> parsed) || parsed < -32768 || parsed > 32767) {
                    *error = "invalid B6F0 XYZ at line " + std::to_string(line_number);
                    return false;
                }
                value = static_cast<renderer_i16>(parsed);
            }
            for (renderer_i32 &value : vertex.captured_camera_xyz) {
                long long parsed = 0;
                if (!(row >> parsed) || parsed < -2147483648LL || parsed > 2147483647LL) {
                    *error = "invalid camera XYZ at line " + std::to_string(line_number);
                    return false;
                }
                value = static_cast<renderer_i32>(parsed);
            }
            for (renderer_i16 &value : vertex.captured_compatibility_xy) {
                int parsed = 0;
                if (!(row >> parsed) || parsed < -32768 || parsed > 32767) {
                    *error = "invalid compatibility XY at line " + std::to_string(line_number);
                    return false;
                }
                value = static_cast<renderer_i16>(parsed);
            }
            if (!(row >> vertex.input_scratch_slot >> vertex.output_scratch_slot
                       >> vertex.source_read_sequence >> vertex.b6f0_callback_sequence
                       >> vertex.b720_callback_sequence) ||
                !read_config(row, &vertex.config) || !no_extra_fields(row)) {
                *error = "malformed vertex association/config at line " + std::to_string(line_number);
                return false;
            }
            input->vertices.push_back(vertex);
            continue;
        }
        if (kind == "face") {
            hercules::dropin::CapturedMeshFace face;
            if (!(row >> face.face_record_index >> face.face_file_offset >> face.packed_index_word)) {
                *error = "malformed face identity at line " + std::to_string(line_number);
                return false;
            }
            for (std::uint32_t &point_index : face.source_point_indices) {
                if (!(row >> point_index)) {
                    *error = "malformed face point indices at line " + std::to_string(line_number);
                    return false;
                }
            }
            for (auto &xy : face.packet_xy_words) {
                for (renderer_i16 &lane : xy) {
                    int parsed = 0;
                    if (!(row >> parsed) || parsed < -32768 || parsed > 32767) {
                        *error = "invalid packet corner XY at line " + std::to_string(line_number);
                        return false;
                    }
                    lane = static_cast<renderer_i16>(parsed);
                }
            }
            if (!(row >> face.ot_bucket_index >> face.packet_tag_previous_link) ||
                !no_extra_fields(row)) {
                *error = "malformed packet insertion at line " + std::to_string(line_number);
                return false;
            }
            input->faces.push_back(face);
            continue;
        }
        *error = "unknown geometry input record at line " + std::to_string(line_number);
        return false;
    }

    if (stream.bad()) {
        *error = "failed while reading geometry input";
        return false;
    }
    if (!have_report_hash || !have_manifest_hash || !have_archive_hash ||
        !have_asset_hash || !have_mesh) {
        *error = "geometry input is missing provenance or mesh metadata";
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char **argv)
{
    if (argc != 3 || std::string(argv[1]) != "--input") {
        std::cerr << "usage: geometry_diagnostic_cli --input <validated-input.tsv>\n";
        return 2;
    }
    hercules::dropin::CapturedMeshInput input;
    std::string error;
    if (!read_input(argv[2], &input, &error)) {
        std::cerr << "geometry input rejected: " << error << '\n';
        return 2;
    }
    hercules::dropin::GeometryDiagnosticResult result;
    if (!hercules::dropin::build_geometry_diagnostic(input, &result, &error)) {
        std::cerr << "geometry diagnostic rejected: " << error << '\n';
        return 3;
    }
    std::cout << hercules::dropin::geometry_diagnostic_json(result);
    return std::cout.good() ? 0 : 4;
}
