#define HERCULES_RENDERER_LIVE_X86 1
#include "hercules_projection.h"

/* Diagnostic single-record fixture only. This is deliberately not a
 * production capture buffer: it is static, one-deep, and not synchronized. */
struct hercules_projection_diagnostic_capture {
    renderer_u32 camera_x;
    renderer_u32 camera_y;
    renderer_u32 camera_z;
    renderer_u16 projected_x;
    renderer_u16 projected_y;
};

typedef char hercules_projection_diagnostic_capture_size_is_16[
    (sizeof(struct hercules_projection_diagnostic_capture) == 16) ? 1 : -1];

static HerculesProjectionSlot106Callback original_slot106;
static struct hercules_projection_diagnostic_capture diagnostic_capture;

HERCULES_PROJECTION_API void HERCULES_CDECL
hercules_projection_diagnostic_set_original_slot106(
    HerculesProjectionSlot106Callback callback)
{
    original_slot106 = callback;
}

HERCULES_PROJECTION_API renderer_raw32 HERCULES_CDECL
hercules_projection_diagnostic_capture_slot106(void)
{
    renderer_raw32 raw_eax;

    if (original_slot106 == (HerculesProjectionSlot106Callback)0)
        return 0xffffffffUL;
    raw_eax = (*original_slot106)();
    diagnostic_capture.camera_x =
        *((volatile renderer_u32 *)HERCULES_PROJECTION_CAMERA_X_ADDRESS);
    diagnostic_capture.camera_y =
        *((volatile renderer_u32 *)HERCULES_PROJECTION_CAMERA_Y_ADDRESS);
    diagnostic_capture.camera_z =
        *((volatile renderer_u32 *)HERCULES_PROJECTION_CAMERA_Z_ADDRESS);
    diagnostic_capture.projected_x =
        *((volatile renderer_u16 *)HERCULES_PROJECTION_SCREEN_X_ADDRESS);
    diagnostic_capture.projected_y =
        *((volatile renderer_u16 *)HERCULES_PROJECTION_SCREEN_Y_ADDRESS);
    return raw_eax;
}

HERCULES_PROJECTION_API const
struct hercules_projection_diagnostic_capture * HERCULES_CDECL
hercules_projection_diagnostic_capture_data(void)
{
    return &diagnostic_capture;
}
