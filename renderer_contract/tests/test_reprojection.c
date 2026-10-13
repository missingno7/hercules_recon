#include "hercules_reprojection.h"
#include <stdio.h>

#define TEST_SCALE_1920 6UL
#define CAMERA_CANARY 0xa5a5a5a5UL
#define SCREEN_CANARY 0xddddU
#define RAW_CANARY 0xdeadbeefUL

static void identity_config(struct hercules_projection_config_view *config)
{
    unsigned int index;

    for (index = 0U; index != 9U; ++index)
        config->matrix_row_major[index] =
            (index == 0U || index == 4U || index == 8U) ?
                (renderer_i16)4096 : (renderer_i16)0;
    config->translation_xyz[0] = 0;
    config->translation_xyz[1] = 0;
    config->translation_xyz[2] = 0;
    config->center_x = 160;
    config->center_y = 120;
    config->focal = 256UL;
}

static int near_double(double actual, double expected)
{
    double difference;

    difference = actual - expected;
    if (difference < 0.0)
        difference = -difference;
    return difference <= 0.000000001;
}

static int test_precision_collision(void)
{
    struct hercules_projection_config_view config;
    struct hercules_projection_point_view point_a;
    struct hercules_projection_point_view point_b;
    struct hercules_projection_result_view native_a;
    struct hercules_projection_result_view native_b;
    struct hercules_reprojection_private_vertex_view private_a;
    struct hercules_reprojection_private_vertex_view private_b;
    renderer_i32 status_a;
    renderer_i32 status_b;

    identity_config(&config);
    point_a.xyz[0] = 1;
    point_a.xyz[1] = 0;
    point_a.xyz[2] = 1000;
    point_b.xyz[0] = 2;
    point_b.xyz[1] = 0;
    point_b.xyz[2] = 1000;

    status_a = hercules_reprojection_eval_single(
        &config, &point_a, TEST_SCALE_1920, &native_a, &private_a);
    status_b = hercules_reprojection_eval_single(
        &config, &point_b, TEST_SCALE_1920, &native_b, &private_b);
    if (status_a != HERCULES_REPROJECTION_OK ||
        status_b != HERCULES_REPROJECTION_OK)
        return 1;
    if (native_a.camera_x != 1L || native_b.camera_x != 2L ||
        native_a.camera_z != 1000L || native_b.camera_z != 1000L)
        return 2;
    if (native_a.projected_x != 160U || native_b.projected_x != 160U ||
        native_a.projected_y != 120U || native_b.projected_y != 120U)
        return 4;
    if (!near_double(private_a.x, 961.536) ||
        !near_double(private_b.x, 963.072) ||
        !near_double(private_a.y, 720.0) ||
        !near_double(private_b.y, 720.0))
        return 8;
    if (private_a.x == private_b.x ||
        private_a.camera_x_input == private_b.camera_x_input)
        return 16;
    if (private_a.camera_provenance !=
            HERCULES_REPROJECTION_CAMERA_DERIVED_B320_REFERENCE ||
        private_b.camera_provenance !=
            HERCULES_REPROJECTION_CAMERA_DERIVED_B320_REFERENCE)
        return 32;
    return 0;
}

static int test_captured_camera_and_focal_wrap(void)
{
    struct hercules_projection_config_view config;
    struct hercules_reprojection_private_vertex_view result;
    renderer_i32 status;

    identity_config(&config);
    config.focal = 0x0001ffffUL;
    status = hercules_reprojection_from_camera(
        &config, 1L, 0L, 1L, TEST_SCALE_1920, &result);
    if (status != HERCULES_REPROJECTION_OK ||
        result.camera_x_input != 1L || result.camera_y_input != 0L ||
        result.camera_z_input != 1L ||
        result.camera_provenance !=
            HERCULES_REPROJECTION_CAMERA_CALLER_SUPPLIED ||
        !near_double(result.x, 954.0) || !near_double(result.y, 720.0)) {
        fprintf(stderr, "focal wrap status=%ld camera=(%ld,%ld,%ld) xy=%.9f,%.9f provenance=%ld\n",
                (long)status, (long)result.camera_x_input,
                (long)result.camera_y_input, (long)result.camera_z_input,
                result.x, result.y, (long)result.camera_provenance);
        return 1;
    }
    return 0;
}

static int test_single_z_clamp_and_rejection(void)
{
    struct hercules_projection_config_view config;
    struct hercules_projection_point_view behind;
    struct hercules_projection_point_view zero;
    struct hercules_projection_result_view compatibility;
    struct hercules_reprojection_private_vertex_view private_result;
    renderer_i32 status;

    identity_config(&config);
    behind.xyz[0] = 1;
    behind.xyz[1] = 0;
    behind.xyz[2] = -7;
    status = hercules_reprojection_eval_single(
        &config, &behind, TEST_SCALE_1920, &compatibility, &private_result);
    if (status != HERCULES_REPROJECTION_BEHIND_CAMERA ||
        compatibility.camera_z != 1L || compatibility.projected_x != 416U ||
        private_result.camera_z_input != -7L ||
        private_result.camera_provenance !=
            HERCULES_REPROJECTION_CAMERA_DERIVED_B320_REFERENCE ||
        private_result.status != HERCULES_REPROJECTION_BEHIND_CAMERA ||
        private_result.x != 0.0 || private_result.y != 0.0)
        return 1;

    zero.xyz[0] = 10;
    zero.xyz[1] = -10;
    zero.xyz[2] = 0;
    status = hercules_reprojection_eval_single(
        &config, &zero, TEST_SCALE_1920, &compatibility, &private_result);
    if (status != HERCULES_REPROJECTION_ZERO_DEPTH ||
        compatibility.camera_z != 1L || compatibility.projected_x != 2720U ||
        compatibility.projected_y != 63096U ||
        private_result.camera_z_input != 0L ||
        private_result.status != HERCULES_REPROJECTION_ZERO_DEPTH)
        return 2;
    return 0;
}

static int test_low_word_narrowing_and_overflow(void)
{
    struct hercules_projection_config_view config;
    struct hercules_projection_point_view point;
    struct hercules_projection_result_view compatibility;
    struct hercules_reprojection_private_vertex_view private_result;
    renderer_i32 status;
    unsigned int index;

    config.matrix_row_major[0] = 32767;
    config.matrix_row_major[1] = -32768;
    config.matrix_row_major[2] = 32767;
    config.matrix_row_major[3] = -32768;
    config.matrix_row_major[4] = 32767;
    config.matrix_row_major[5] = -32768;
    config.matrix_row_major[6] = 32767;
    config.matrix_row_major[7] = 32767;
    config.matrix_row_major[8] = 32767;
    config.translation_xyz[0] = 2147483647L;
    config.translation_xyz[1] = (-2147483647L - 1L);
    config.translation_xyz[2] = 2147483647L;
    config.center_x = 2147483647L;
    config.center_y = (-2147483647L - 1L);
    config.focal = 0x0001ffffUL;
    point.xyz[0] = 32767;
    point.xyz[1] = -32768;
    point.xyz[2] = 32767;

    status = hercules_reprojection_eval_single(
        &config, &point, TEST_SCALE_1920, &compatibility, &private_result);
    if (status != HERCULES_REPROJECTION_BEHIND_CAMERA ||
        compatibility.camera_x != -33L || compatibility.camera_y != 24L ||
        compatibility.camera_z != 1L ||
        private_result.camera_x_input != -33L ||
        private_result.camera_y_input != 24L ||
        private_result.camera_z_input != -25L) {
        fprintf(stderr, "narrow status=%ld compat=(%ld,%ld,%ld) private=(%ld,%ld,%ld)\n",
                (long)status, (long)compatibility.camera_x,
                (long)compatibility.camera_y, (long)compatibility.camera_z,
                (long)private_result.camera_x_input,
                (long)private_result.camera_y_input,
                (long)private_result.camera_z_input);
        return 1;
    }

    /* Invalid private policy must not affect a compatibility result. */
    status = hercules_reprojection_eval_single(
        &config, &point, 0UL, &compatibility, &private_result);
    if (status != HERCULES_REPROJECTION_UNSUPPORTED_POLICY ||
        compatibility.camera_x != -33L || compatibility.camera_y != 24L)
        return 2;

    for (index = 0U; index != 9U; ++index)
        config.matrix_row_major[index] = 0;
    return 0;
}

static int test_batch_zero_and_negative_depth(void)
{
    struct hercules_projection_config_view config;
    struct hercules_projection_point_view points[3];
    struct hercules_projection_batch_result_view compatibility;
    struct hercules_reprojection_private_vertex_view private_results[3];
    renderer_u32 fault_point;
    renderer_raw32 raw_eax;
    renderer_i32 status;

    identity_config(&config);
    points[0].xyz[0] = 10;
    points[0].xyz[1] = -10;
    points[0].xyz[2] = 0;
    points[1].xyz[0] = 1;
    points[1].xyz[1] = 0;
    points[1].xyz[2] = -7;
    points[2].xyz[0] = 2;
    points[2].xyz[1] = 3;
    points[2].xyz[2] = 1000;
    fault_point = 0xffffffffUL;
    raw_eax = RAW_CANARY;
    status = hercules_reprojection_eval_batch(
        &config, points, TEST_SCALE_1920, &compatibility, private_results,
        &fault_point, &raw_eax);
    if (status != HERCULES_PROJECTION_BATCH_OK || fault_point != 3UL ||
        raw_eax == RAW_CANARY) {
        fprintf(stderr, "batch status=%ld fault=%lu raw=%lu\n",
                (long)status, (unsigned long)fault_point,
                (unsigned long)raw_eax);
        return 1;
    }
    if (compatibility.camera_xyz_bits[2] != 0UL ||
        compatibility.projected_xy[0] != 2720U ||
        compatibility.projected_xy[1] != 63096U ||
        private_results[0].status != HERCULES_REPROJECTION_ZERO_DEPTH) {
        fprintf(stderr, "batch zero camZ=%lu xy=(%u,%u) privateStatus=%ld\n",
                (unsigned long)compatibility.camera_xyz_bits[2],
                (unsigned int)compatibility.projected_xy[0],
                (unsigned int)compatibility.projected_xy[1],
                (long)private_results[0].status);
        return 2;
    }
    if (compatibility.camera_xyz_bits[5] != 0xfffffff9UL ||
        private_results[1].status != HERCULES_REPROJECTION_BEHIND_CAMERA) {
        fprintf(stderr, "batch negative camZ=%lu privateStatus=%ld\n",
                (unsigned long)compatibility.camera_xyz_bits[5],
                (long)private_results[1].status);
        return 4;
    }
    if (private_results[2].status != HERCULES_REPROJECTION_OK ||
        !near_double(private_results[2].x, 963.072) ||
        !near_double(private_results[2].y, 724.608)) {
        fprintf(stderr, "batch positive status=%ld xy=%.9f,%.9f\n",
                (long)private_results[2].status,
                private_results[2].x, private_results[2].y);
        return 8;
    }
    return 0;
}

static int test_batch_idiv_partial_prefix(void)
{
    struct hercules_projection_config_view config;
    struct hercules_projection_point_view points[3];
    struct hercules_projection_batch_result_view compatibility;
    struct hercules_reprojection_private_vertex_view private_results[3];
    renderer_u32 fault_point;
    renderer_raw32 raw_eax;
    renderer_i32 status;
    unsigned int index;

    identity_config(&config);
    config.focal = 32768UL;
    for (index = 0U; index != 9U; ++index)
        compatibility.camera_xyz_bits[index] = CAMERA_CANARY;
    for (index = 0U; index != 6U; ++index)
        compatibility.projected_xy[index] = SCREEN_CANARY;
    points[0].xyz[0] = 0;
    points[0].xyz[1] = 0;
    points[0].xyz[2] = 1;
    points[1].xyz[0] = 1;
    points[1].xyz[1] = 2;
    points[1].xyz[2] = -1;
    points[2].xyz[0] = 3;
    points[2].xyz[1] = 4;
    points[2].xyz[2] = 5;
    raw_eax = RAW_CANARY;
    fault_point = 0xffffffffUL;

    status = hercules_reprojection_eval_batch(
        &config, points, TEST_SCALE_1920, &compatibility, private_results,
        &fault_point, &raw_eax);
    if (status != HERCULES_PROJECTION_BATCH_IDIV_OVERFLOW || fault_point != 1UL ||
        raw_eax != RAW_CANARY)
        return 1;
    if (compatibility.camera_xyz_bits[0] != 0UL ||
        compatibility.camera_xyz_bits[1] != 0UL ||
        compatibility.camera_xyz_bits[2] != 1UL ||
        compatibility.camera_xyz_bits[3] != 1UL ||
        compatibility.camera_xyz_bits[4] != 2UL ||
        compatibility.camera_xyz_bits[5] != 0xffffffffUL ||
        compatibility.camera_xyz_bits[6] != CAMERA_CANARY ||
        compatibility.projected_xy[2] != SCREEN_CANARY)
        return 2;
    if (private_results[0].status != HERCULES_REPROJECTION_OK ||
        private_results[1].status != HERCULES_REPROJECTION_BEHIND_CAMERA ||
        private_results[2].status != HERCULES_REPROJECTION_CAMERA_UNAVAILABLE)
        return 4;
    return 0;
}

HERCULES_REPROJECTION_API int hercules_reprojection_test_all(void)
{
    int failures;

    failures = 0;
    if (test_precision_collision() != 0)
        failures |= 1;
    if (test_captured_camera_and_focal_wrap() != 0)
        failures |= 32;
    if (test_single_z_clamp_and_rejection() != 0)
        failures |= 2;
    if (test_low_word_narrowing_and_overflow() != 0)
        failures |= 4;
    if (test_batch_zero_and_negative_depth() != 0)
        failures |= 8;
    if (test_batch_idiv_partial_prefix() != 0)
        failures |= 16;
    return failures;
}

#ifndef HERCULES_REPROJECTION_BUILD_EXPORTS
int main(void)
{
    int failures;

    failures = hercules_reprojection_test_all();
    if (failures != 0) {
        fprintf(stderr, "reprojection source tests failed: mask=%d\n", failures);
        return 1;
    }
    puts("private reprojection passed: compatibility, precision collision, depth rejection and batch prefix");
    return 0;
}
#endif
