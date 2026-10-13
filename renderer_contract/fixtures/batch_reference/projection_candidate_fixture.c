#include "../../hercules_projection.c"

#if defined(_MSC_VER)
#define HERCULES_BATCH_FIXTURE_EXPORT __declspec(dllexport)
#else
#define HERCULES_BATCH_FIXTURE_EXPORT
#endif

#if defined(_MSC_VER)
#define HERCULES_BATCH_FIXTURE_CDECL __cdecl
#else
#define HERCULES_BATCH_FIXTURE_CDECL __attribute__((cdecl))
#endif

HERCULES_BATCH_FIXTURE_EXPORT renderer_i32 HERCULES_BATCH_FIXTURE_CDECL
hercules_batch_reference_run(
    const struct hercules_projection_config_view *config,
    const struct hercules_projection_point_view points[
        HERCULES_PROJECTION_BATCH_POINT_COUNT],
    struct hercules_projection_batch_result_view *result,
    renderer_u32 *fault_point,
    renderer_raw32 *raw_eax)
{
    return hercules_projection_batch_eval(
        config, points, result, fault_point, raw_eax);
}
