#include "hercules_reprojection.h"

static renderer_i32 hercules_reprojection_signed16(renderer_u16 raw)
{
    if (raw <= 0x7fffU)
        return (renderer_i32)raw;
    return (renderer_i32)(-1L - (renderer_i32)(0xffffU - raw));
}

static int hercules_reprojection_word_bits(renderer_u32 raw,
                                            renderer_i32 *value)
{
    renderer_i32 signed_value;

    signed_value = hercules_reprojection_signed16((renderer_u16)raw);
    if (raw != (renderer_u32)signed_value)
        return 0;
    *value = signed_value;
    return 1;
}

static void hercules_reprojection_empty_private(
    struct hercules_reprojection_private_vertex_view *result,
    renderer_i32 status)
{
    result->camera_x_input = 0;
    result->camera_y_input = 0;
    result->camera_z_input = 0;
    result->x = 0.0;
    result->y = 0.0;
    result->status = status;
    result->camera_provenance = HERCULES_REPROJECTION_PROVENANCE_UNAVAILABLE;
}

static renderer_i32 hercules_reprojection_effective_focal(renderer_u32 raw)
{
    renderer_u16 low_word;

    low_word = (renderer_u16)raw;
    if (low_word <= 0x7fffU)
        return (renderer_i32)low_word;
    return (renderer_i32)(-1L - (renderer_i32)(0xffffU - low_word));
}

renderer_i32 hercules_reprojection_from_camera(
    const struct hercules_projection_config_view *config,
    renderer_i32 camera_x,
    renderer_i32 camera_y,
    renderer_i32 camera_z,
    renderer_u32 resolution_scale,
    struct hercules_reprojection_private_vertex_view *result)
{
    double scale;
    double private_focal;
    renderer_i32 effective_focal;

    if (result == 0)
        return HERCULES_REPROJECTION_BAD_ARGUMENT;
    hercules_reprojection_empty_private(result,
                                        HERCULES_REPROJECTION_BAD_ARGUMENT);
    if (config == 0)
        return result->status;

    result->camera_x_input = camera_x;
    result->camera_y_input = camera_y;
    result->camera_z_input = camera_z;
    result->camera_provenance = HERCULES_REPROJECTION_CAMERA_CALLER_SUPPLIED;

    if (resolution_scale == 0UL ||
        resolution_scale > HERCULES_REPROJECTION_MAX_SCALE ||
        camera_x < -32768L || camera_x > 32767L ||
        camera_y < -32768L || camera_y > 32767L ||
        camera_z < -32768L || camera_z > 32767L) {
        result->status = HERCULES_REPROJECTION_UNSUPPORTED_POLICY;
        return result->status;
    }
    if (camera_z == 0L) {
        result->status = HERCULES_REPROJECTION_ZERO_DEPTH;
        return result->status;
    }
    if (camera_z < 0L) {
        result->status = HERCULES_REPROJECTION_BEHIND_CAMERA;
        return result->status;
    }

    scale = (double)resolution_scale;
    effective_focal = hercules_reprojection_effective_focal(config->focal);
    private_focal = (double)effective_focal * scale;
    result->x = (double)config->center_x * scale +
                private_focal * (double)camera_x /
                    (double)camera_z;
    result->y = (double)config->center_y * scale +
                private_focal * (double)camera_y /
                    (double)camera_z;
    result->status = HERCULES_REPROJECTION_OK;
    return result->status;
}

renderer_i32 hercules_reprojection_eval_single(
    const struct hercules_projection_config_view *config,
    const struct hercules_projection_point_view *point,
    renderer_u32 resolution_scale,
    struct hercules_projection_result_view *compatibility,
    struct hercules_reprojection_private_vertex_view *private_result)
{
    struct hercules_projection_point_view repeated_points[
        HERCULES_PROJECTION_BATCH_POINT_COUNT];
    struct hercules_projection_batch_result_view transformed;
    renderer_u32 fault_point;
    renderer_i32 batch_status;
    renderer_i32 camera_x;
    renderer_i32 camera_y;
    renderer_i32 camera_z;
    renderer_i32 status;
    unsigned int index;

    if (config == 0 || point == 0 || compatibility == 0 || private_result == 0) {
        if (private_result != 0)
            hercules_reprojection_empty_private(
                private_result, HERCULES_REPROJECTION_BAD_ARGUMENT);
        return HERCULES_REPROJECTION_BAD_ARGUMENT;
    }

    hercules_projection_eval(config, point, compatibility);
    for (index = 0U; index != HERCULES_PROJECTION_BATCH_POINT_COUNT; ++index)
        repeated_points[index] = *point;
    fault_point = 0xffffffffUL;
    batch_status = hercules_projection_batch_eval(
        config, repeated_points, &transformed, &fault_point, 0);
    if (batch_status == HERCULES_PROJECTION_BATCH_BAD_ARGUMENT ||
        !hercules_reprojection_word_bits(transformed.camera_xyz_bits[0], &camera_x) ||
        !hercules_reprojection_word_bits(transformed.camera_xyz_bits[1], &camera_y) ||
        !hercules_reprojection_word_bits(transformed.camera_xyz_bits[2], &camera_z)) {
        hercules_reprojection_empty_private(
            private_result, HERCULES_REPROJECTION_REFERENCE_DIVERGENCE);
        return private_result->status;
    }

    if (compatibility->camera_x != camera_x ||
        compatibility->camera_y != camera_y ||
        compatibility->camera_z != (camera_z < 1L ? 1L : camera_z)) {
        hercules_reprojection_empty_private(
            private_result, HERCULES_REPROJECTION_REFERENCE_DIVERGENCE);
        return private_result->status;
    }

    status = hercules_reprojection_from_camera(
        config, camera_x, camera_y, camera_z, resolution_scale, private_result);
    private_result->camera_provenance =
        HERCULES_REPROJECTION_CAMERA_DERIVED_B320_REFERENCE;
    return status;
}

renderer_i32 hercules_reprojection_eval_batch(
    const struct hercules_projection_config_view *config,
    const struct hercules_projection_point_view points[
        HERCULES_REPROJECTION_BATCH_COUNT],
    renderer_u32 resolution_scale,
    struct hercules_projection_batch_result_view *compatibility,
    struct hercules_reprojection_private_vertex_view private_results[
        HERCULES_REPROJECTION_BATCH_COUNT],
    renderer_u32 *fault_point,
    renderer_raw32 *raw_eax)
{
    renderer_u32 local_fault_point;
    renderer_u32 *fault_output;
    renderer_i32 native_status;
    renderer_i32 camera[3];
    unsigned int completed_camera_count;
    unsigned int point_index;
    unsigned int lane;
    int valid_camera;

    if (config == 0 || points == 0 || compatibility == 0 || private_results == 0) {
        if (fault_point != 0)
            *fault_point = 0xffffffffUL;
        if (private_results != 0) {
            for (point_index = 0U;
                 point_index != HERCULES_REPROJECTION_BATCH_COUNT;
                 ++point_index)
                hercules_reprojection_empty_private(
                    &private_results[point_index],
                    HERCULES_REPROJECTION_BAD_ARGUMENT);
        }
        return HERCULES_PROJECTION_BATCH_BAD_ARGUMENT;
    }

    local_fault_point = 0xffffffffUL;
    fault_output = fault_point != 0 ? fault_point : &local_fault_point;
    native_status = hercules_projection_batch_eval(
        config, points, compatibility, fault_output, raw_eax);
    local_fault_point = *fault_output;

    if (native_status == HERCULES_PROJECTION_BATCH_OK)
        completed_camera_count = HERCULES_REPROJECTION_BATCH_COUNT;
    else if (native_status == HERCULES_PROJECTION_BATCH_IDIV_OVERFLOW &&
             local_fault_point < HERCULES_REPROJECTION_BATCH_COUNT)
        completed_camera_count = (unsigned int)local_fault_point + 1U;
    else
        completed_camera_count = 0U;

    for (point_index = 0U;
         point_index != HERCULES_REPROJECTION_BATCH_COUNT;
         ++point_index) {
        if (point_index >= completed_camera_count) {
            hercules_reprojection_empty_private(
                &private_results[point_index],
                HERCULES_REPROJECTION_CAMERA_UNAVAILABLE);
            continue;
        }

        valid_camera = 1;
        for (lane = 0U; lane != 3U; ++lane) {
            if (!hercules_reprojection_word_bits(
                    compatibility->camera_xyz_bits[point_index * 3U + lane],
                    &camera[lane]))
                valid_camera = 0;
        }
        if (!valid_camera) {
            hercules_reprojection_empty_private(
                &private_results[point_index],
                HERCULES_REPROJECTION_REFERENCE_DIVERGENCE);
            continue;
        }
        hercules_reprojection_from_camera(
            config, camera[0], camera[1], camera[2], resolution_scale,
            &private_results[point_index]);
        private_results[point_index].camera_provenance =
            HERCULES_REPROJECTION_CAMERA_DERIVED_B320_REFERENCE;
    }
    return native_status;
}
