#include "hercules_projection.h"

extern "C" HERCULES_PROJECTION_API int
hercules_projection_cpp_linkage_probe(void)
{
    static const hercules_projection_config_view config = {
        {4096,0,0, 0,4096,0, 0,0,4096}, {0,0,0}, 10,20, 256UL
    };
    static const hercules_projection_point_view point = {{1,2,8}};
    static const hercules_projection_point_view points[3] = {
        {{1,2,8}}, {{3,4,0}}, {{0,0,-8}}
    };
    hercules_projection_result_view result;
    hercules_projection_batch_result_view batch_result;
    renderer_raw32 batch_raw_eax;
    renderer_u32 fault_point;

    hercules_projection_eval(&config, &point, &result);
    if (result.camera_x != 1 || result.camera_y != 2 || result.camera_z != 8)
        return 0;
    if (result.projected_x != 42U || result.projected_y != 84U)
        return 0;
    if (result.raw_eax != 84UL)
        return 0;

    if (hercules_projection_batch_eval(&config, points, &batch_result,
                                      &fault_point, &batch_raw_eax) !=
        HERCULES_PROJECTION_BATCH_OK)
        return 0;
    if (fault_point != HERCULES_PROJECTION_BATCH_POINT_COUNT || batch_raw_eax != 20UL)
        return 0;
    if (batch_result.camera_xyz_bits[0] != 1UL ||
        batch_result.camera_xyz_bits[1] != 2UL ||
        batch_result.camera_xyz_bits[2] != 8UL ||
        batch_result.camera_xyz_bits[3] != 3UL ||
        batch_result.camera_xyz_bits[4] != 4UL ||
        batch_result.camera_xyz_bits[5] != 0UL ||
        batch_result.camera_xyz_bits[6] != 0UL ||
        batch_result.camera_xyz_bits[7] != 0UL ||
        batch_result.camera_xyz_bits[8] != 0xfffffff8UL)
        return 0;
    if (batch_result.projected_xy[0] != 42U ||
        batch_result.projected_xy[1] != 84U ||
        batch_result.projected_xy[2] != 778U ||
        batch_result.projected_xy[3] != 1044U ||
        batch_result.projected_xy[4] != 10U ||
        batch_result.projected_xy[5] != 20U)
        return 0;
    return 1;
}

#ifndef HERCULES_PROJECTION_BUILD_EXPORTS
int main(void)
{
    return hercules_projection_cpp_linkage_probe() == 1 ? 0 : 1;
}
#endif
