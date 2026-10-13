#include "hercules_projection.h"

extern "C" HERCULES_PROJECTION_API int
hercules_projection_cpp_linkage_probe(void)
{
    static const hercules_projection_config_view config = {
        {4096,0,0, 0,4096,0, 0,0,4096}, {0,0,0}, 10,20, 256UL
    };
    static const hercules_projection_point_view point = {{1,2,8}};
    hercules_projection_result_view result;

    hercules_projection_eval(&config, &point, &result);
    if (result.camera_x != 1 || result.camera_y != 2 || result.camera_z != 8)
        return 0;
    if (result.projected_x != 42U || result.projected_y != 84U)
        return 0;
    if (result.raw_eax != 84UL)
        return 0;
    return 1;
}

#ifndef HERCULES_PROJECTION_BUILD_EXPORTS
int main(void)
{
    return hercules_projection_cpp_linkage_probe() == 1 ? 0 : 1;
}
#endif
