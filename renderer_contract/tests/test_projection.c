#include "hercules_projection.h"

struct projection_oracle_vector {
    renderer_i16 matrix[9];
    renderer_i32 translation[3];
    renderer_i32 center_x;
    renderer_i32 center_y;
    renderer_u32 focal;
    renderer_i16 point[3];
    renderer_i32 camera[3];
    renderer_u16 screen[2];
    renderer_raw32 raw_eax;
};

typedef char projection_test_point_xyz_offset_is_zero[
    (offsetof(struct hercules_projection_point_view, xyz) == 0) ? 1 : -1];
typedef char projection_test_result_projected_x_offset_is_12[
    (offsetof(struct hercules_projection_result_view, projected_x) == 12) ? 1 : -1];
typedef char projection_test_result_raw_eax_offset_is_16[
    (offsetof(struct hercules_projection_result_view, raw_eax) == 16) ? 1 : -1];
typedef char projection_test_batch_screen_offset_is_36[
    (offsetof(struct hercules_projection_batch_result_view, projected_xy) == 36) ? 1 : -1];
typedef char projection_test_batch_size_is_48[
    (sizeof(struct hercules_projection_batch_result_view) == 48) ? 1 : -1];

#define I32_MIN_VALUE (-2147483647L - 1L)

static const struct projection_oracle_vector oracle_vectors[12] = {
    {{4096,0,0, 0,4096,0, 0,0,4096}, {0,0,0}, 160,120, 0x100UL,
     {3,-4,64}, {3,-4,64}, {172,104}, 0x00000068UL},
    {{4096,0,0, 0,4096,0, 0,0,4096}, {0,0,0}, 100,100, 0x100UL,
     {-3,5,7}, {-3,5,7}, {65526,282}, 0x0000011aUL},
    {{4096,0,0, 0,4096,0, 0,0,4096}, {0,0,0}, 160,120, 0x100UL,
     {10,-10,0}, {10,-10,1}, {2720,63096}, 0xfffff678UL},
    {{4096,0,0, 0,4096,0, 0,0,4096}, {0,0,0}, 0,0, 0x100UL,
     {-7,9,-32768}, {-7,9,1}, {63744,2304}, 0x00000900UL},
    {{4096,2048,-1024, 0,4096,0, 1024,0,4096}, {0,0,0}, 80,60, 0x80UL,
     {8,-6,4}, {4,-6,6}, {165,65468}, 0xffffffbcUL},
    {{32767,-32768,32767, -32768,32767,-32768, 32767,32767,32767},
     {2147483647L,I32_MIN_VALUE,2147483647L},
     2147483647L,I32_MIN_VALUE,0x0001ffffUL,
     {32767,-32768,32767}, {-33,24,1}, {32,65512}, 0x7fffffe8UL},
    {{4096,0,0, 0,4096,0, 0,0,4096}, {0,0,0}, 12,-13, 0x12348000UL,
     {2,-3,1}, {2,-3,1}, {12,32755}, 0xffff7ff3UL},
    {{4096,0,0, 0,4096,0, 0,0,4096}, {0,0,0}, 0,0, 0x00008001UL,
     {7,-5,3}, {7,-5,3}, {54615,54611}, 0xffffd553UL},
    {{4096,0,0, 0,4096,0, 0,0,4096}, {0,0,0},
     2147483647L,I32_MIN_VALUE,0x1000UL,
     {32767,-32768,1}, {32767,-32768,1}, {61439,0}, 0x80000000UL},
    {{2048,0,0, 0,4096,0, 0,0,4096}, {-1,7,3}, -17,31,0x20UL,
     {-32768,32767,1}, {-16385,-32762,4}, {65511,79}, 0x0000004fUL},
    {{0,-4096,0, 4096,0,0, 0,0,4096}, {23,-14,2}, 64,48,0x60UL,
     {11,5,32}, {18,-3,34}, {114,39}, 0x00000027UL},
    {{4096,0,0, 0,4096,0, 0,0,4096}, {0,0,0}, 65535,65536,0x80UL,
     {200,-150,3}, {200,-150,3}, {8532,59136}, 0x0000e700UL}
};

HERCULES_PROJECTION_API int hercules_projection_test_all(void)
{
    struct hercules_projection_config_view config;
    struct hercules_projection_point_view point;
    struct hercules_projection_result_view result;
    unsigned int vector_index;
    unsigned int item_index;
    unsigned int mismatch_mask;
    int failures;

    failures = 0;
    for (vector_index = 0U; vector_index != 12U; ++vector_index) {
        for (item_index = 0U; item_index != 9U; ++item_index)
            config.matrix_row_major[item_index] =
                oracle_vectors[vector_index].matrix[item_index];
        for (item_index = 0U; item_index != 3U; ++item_index) {
            config.translation_xyz[item_index] =
                oracle_vectors[vector_index].translation[item_index];
            point.xyz[item_index] = oracle_vectors[vector_index].point[item_index];
        }
        config.center_x = oracle_vectors[vector_index].center_x;
        config.center_y = oracle_vectors[vector_index].center_y;
        config.focal = oracle_vectors[vector_index].focal;
        hercules_projection_eval(&config, &point, &result);
        mismatch_mask = 0U;
        if (result.camera_x != oracle_vectors[vector_index].camera[0])
            mismatch_mask |= 1U;
        if (result.camera_y != oracle_vectors[vector_index].camera[1])
            mismatch_mask |= 2U;
        if (result.camera_z != oracle_vectors[vector_index].camera[2])
            mismatch_mask |= 4U;
        if (result.projected_x != oracle_vectors[vector_index].screen[0])
            mismatch_mask |= 8U;
        if (result.projected_y != oracle_vectors[vector_index].screen[1])
            mismatch_mask |= 16U;
        if (result.raw_eax != oracle_vectors[vector_index].raw_eax)
            mismatch_mask |= 32U;
        if (mismatch_mask != 0U)
            failures |= (int)(1U << vector_index);
    }
    return failures;
}

struct projection_batch_oracle_vector {
    renderer_i16 matrix[9];
    renderer_i32 translation[3];
    renderer_i32 center[2];
    renderer_u32 focal;
    renderer_i16 points[9];
    renderer_u32 camera_bits[9];
    renderer_u16 screen[6];
    renderer_raw32 raw_eax;
};

/* Expected outputs are copied from the pinned original B320 fixture receipt.
 * These vectors exercise sum-before-SAR, signed Z and zero Z, low-WORD
 * sign-extension, negative SAR rounding, and a low-32 row-sum wrap. */
static const struct projection_batch_oracle_vector batch_oracle_vectors[5] = {
    {{2048,2048,0, 0,4096,0, 0,0,4096}, {0,0,0}, {50,70}, 256UL,
     {1,1,100, 1,1,-100, 1,1,0},
     {1UL,1UL,100UL, 1UL,1UL,0xffffff9cUL, 1UL,1UL,0UL},
     {52,72, 47,67, 306,326}, 0x00000146UL},
    {{4096,0,0, 0,4096,0, 0,0,4096}, {1,0,0}, {50,70}, 0UL,
     {32767,2,1000, 0,3,1000, 1,4,1000},
     {0xffff8000UL,2UL,1000UL, 1UL,3UL,1000UL, 2UL,4UL,1000UL},
     {50,70, 50,70, 50,70}, 0x00000046UL},
    {{4096,0,0, 0,4096,0, 0,0,4096}, {-1,0,0}, {50,70}, 0UL,
     {-32768,2,1000, 0,3,1000, 1,4,1000},
     {0x00007fffUL,2UL,1000UL, 0xffffffffUL,3UL,1000UL, 0UL,4UL,1000UL},
     {50,70, 50,70, 50,70}, 0x00000046UL},
    {{1,0,0, 0,4096,0, 0,0,4096}, {0,0,0}, {50,70}, 0UL,
     {-1,10,1000, 0,0,1000, 0,0,1000},
     {0xffffffffUL,10UL,1000UL, 0UL,0UL,1000UL, 0UL,0UL,1000UL},
     {50,70, 50,70, 50,70}, 0x00000046UL},
    {{32767,32767,32767, 0,4096,0, 0,0,4096}, {0,0,0}, {50,70}, 0UL,
     {32767,32767,32767, 1,2,3, -1,-2,-3},
     {0xffffffd0UL,32767UL,32767UL, 47UL,2UL,3UL,
      0xffffffd0UL,0xfffffffeUL,0xfffffffdUL},
     {50,70, 50,70, 50,70}, 0x00000046UL}
};

HERCULES_PROJECTION_API int hercules_projection_batch_test_all(void)
{
    struct hercules_projection_config_view config;
    struct hercules_projection_point_view points[
        HERCULES_PROJECTION_BATCH_POINT_COUNT];
    struct hercules_projection_batch_result_view result;
    renderer_raw32 raw_eax;
    renderer_u32 fault_point;
    renderer_i32 status;
    unsigned int vector_index;
    unsigned int item_index;
    unsigned int mismatch;
    int failures;

    failures = 0;
    for (vector_index = 0U; vector_index != 5U; ++vector_index) {
        for (item_index = 0U; item_index != 9U; ++item_index) {
            config.matrix_row_major[item_index] =
                batch_oracle_vectors[vector_index].matrix[item_index];
            points[item_index / 3U].xyz[item_index % 3U] =
                batch_oracle_vectors[vector_index].points[item_index];
        }
        for (item_index = 0U; item_index != 3U; ++item_index)
            config.translation_xyz[item_index] =
                batch_oracle_vectors[vector_index].translation[item_index];
        config.center_x = batch_oracle_vectors[vector_index].center[0];
        config.center_y = batch_oracle_vectors[vector_index].center[1];
        config.focal = batch_oracle_vectors[vector_index].focal;

        raw_eax = 0xffffffffUL;
        fault_point = 0xffffffffUL;
        status = hercules_projection_batch_eval(
            &config, points, &result, &fault_point, &raw_eax);
        mismatch = (status != HERCULES_PROJECTION_BATCH_OK ||
                    fault_point != HERCULES_PROJECTION_BATCH_POINT_COUNT ||
                    raw_eax != batch_oracle_vectors[vector_index].raw_eax) ? 1U : 0U;
        for (item_index = 0U; item_index != 9U; ++item_index) {
            if (result.camera_xyz_bits[item_index] !=
                batch_oracle_vectors[vector_index].camera_bits[item_index])
                mismatch = 1U;
        }
        for (item_index = 0U; item_index != 6U; ++item_index) {
            if (result.projected_xy[item_index] !=
                batch_oracle_vectors[vector_index].screen[item_index])
                mismatch = 1U;
        }
        if (mismatch != 0U)
            failures |= (int)(1U << vector_index);
    }

    /* The pinned original traps at B3F1 after completing point 0 and storing
     * point 1's camera triplet. Preserve that exact partial-output boundary. */
    for (item_index = 0U; item_index != 9U; ++item_index) {
        config.matrix_row_major[item_index] =
            (item_index == 0U || item_index == 4U || item_index == 8U) ? 4096 : 0;
        result.camera_xyz_bits[item_index] = 0xccccccccUL;
    }
    for (item_index = 0U; item_index != 6U; ++item_index)
        result.projected_xy[item_index] = 0xddddU;
    config.translation_xyz[0] = 0;
    config.translation_xyz[1] = 0;
    config.translation_xyz[2] = 0;
    config.center_x = 0;
    config.center_y = 0;
    config.focal = 32768UL;
    points[0].xyz[0] = 0;
    points[0].xyz[1] = 0;
    points[0].xyz[2] = 1;
    points[1].xyz[0] = 1;
    points[1].xyz[1] = 2;
    points[1].xyz[2] = -1;
    points[2].xyz[0] = 3;
    points[2].xyz[1] = 4;
    points[2].xyz[2] = 5;
    raw_eax = 0xdeadbeefUL;
    fault_point = 0xffffffffUL;
    status = hercules_projection_batch_eval(
        &config, points, &result, &fault_point, &raw_eax);
    if (status != HERCULES_PROJECTION_BATCH_IDIV_OVERFLOW || fault_point != 1U ||
        raw_eax != 0xdeadbeefUL ||
        result.camera_xyz_bits[0] != 0UL || result.camera_xyz_bits[1] != 0UL ||
        result.camera_xyz_bits[2] != 1UL || result.camera_xyz_bits[3] != 1UL ||
        result.camera_xyz_bits[4] != 2UL || result.camera_xyz_bits[5] != 0xffffffffUL ||
        result.camera_xyz_bits[6] != 0xccccccccUL ||
        result.camera_xyz_bits[7] != 0xccccccccUL ||
        result.camera_xyz_bits[8] != 0xccccccccUL ||
        result.projected_xy[0] != 0U || result.projected_xy[1] != 0U ||
        result.projected_xy[2] != 0xddddU || result.projected_xy[3] != 0xddddU ||
        result.projected_xy[4] != 0xddddU || result.projected_xy[5] != 0xddddU)
        failures |= 1 << 5;

    return failures;
}

#ifndef HERCULES_PROJECTION_BUILD_EXPORTS
#include <stdio.h>
int main(void)
{
    int failures = hercules_projection_test_all();
    int batch_failures = hercules_projection_batch_test_all();
    if (failures != 0 || batch_failures != 0) {
        fprintf(stderr, "projection failures: scalar=%d batch=%d\n", failures, batch_failures);
        return 1;
    }
    puts("projection oracle vectors passed: scalar=12 batch=5 plus IDIV partial-write case");
    return 0;
}
#endif
