#include "hercules_reprojection.h"

extern "C" int hercules_reprojection_cpp_linkage_probe(void)
{
    static const hercules_projection_config_view config = {
        {4096,0,0, 0,4096,0, 0,0,4096}, {0,0,0}, 160,120, 256UL
    };
    static const hercules_projection_point_view point = {{2,0,1000}};
    hercules_projection_result_view compatibility;
    hercules_reprojection_private_vertex_view private_result;

    if (hercules_reprojection_eval_single(&config, &point, 6UL,
                                          &compatibility, &private_result) !=
        HERCULES_REPROJECTION_OK)
        return 0;
    if (compatibility.projected_x != 160U || compatibility.projected_y != 120U)
        return 0;
    if (private_result.status != HERCULES_REPROJECTION_OK ||
        private_result.camera_x_input != 2L ||
        private_result.camera_z_input != 1000L ||
        private_result.camera_provenance !=
            HERCULES_REPROJECTION_CAMERA_DERIVED_B320_REFERENCE)
        return 0;
    if (private_result.x < 963.071999999 || private_result.x > 963.072000001)
        return 0;
    return 1;
}

#ifndef HERCULES_REPROJECTION_BUILD_EXPORTS
int main()
{
    return hercules_reprojection_cpp_linkage_probe() == 1 ? 0 : 1;
}
#endif
