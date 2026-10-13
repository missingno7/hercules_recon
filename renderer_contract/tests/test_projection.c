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

#ifndef HERCULES_PROJECTION_BUILD_EXPORTS
#include <stdio.h>
int main(void)
{
    int failures = hercules_projection_test_all();
    if (failures != 0) {
        fprintf(stderr, "projection oracle vector failure mask: %d\n", failures);
        return 1;
    }
    puts("projection oracle vectors passed: 12");
    return 0;
}
#endif
