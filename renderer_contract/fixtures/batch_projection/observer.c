#define HERCULES_RENDERER_LIVE_X86 1
#include "hercules_projection.h"

#if !defined(HERCULES_RENDERER_LIVE_X86)
#error This fixture requires the explicit live-x86 ABI opt-in.
#endif
#if !defined(_M_IX86)
#error This fixture requires the 32-bit x86 compiler target.
#endif

#define BATCH_PROJECTION_API __declspec(dllexport)
#define CAMERA_X_ADDRESS ((renderer_raw32)0x00a8e71cUL)
#define CAMERA_Y_ADDRESS ((renderer_raw32)0x00a8e728UL)
#define CAMERA_Z_ADDRESS ((renderer_raw32)0x00a8e734UL)
#define SCREEN_X_ADDRESS ((renderer_raw32)0x00a8e740UL)
#define SCREEN_Y_ADDRESS ((renderer_raw32)0x00a8e746UL)

/* The saved target's return lane is captured as raw ABI bits only. */
typedef HerculesProjectionSlot107Callback SavedSlot107RawEax;

struct batch_diagnostic_state {
    SavedSlot107RawEax saved_slot107;
    struct hercules_projection_batch_result_view capture;
};

typedef char batch_capture_size_is_48[
    (sizeof(struct hercules_projection_batch_result_view) == 48) ? 1 : -1];
typedef char batch_state_size_is_52[
    (sizeof(struct batch_diagnostic_state) == 52) ? 1 : -1];

static struct batch_diagnostic_state diagnostic_state;

BATCH_PROJECTION_API void HERCULES_CDECL
batch_projection_set_saved_slot107(SavedSlot107RawEax callback)
{
    diagnostic_state.saved_slot107 = callback;
}

/* This is a true zero-argument diagnostic callback around one saved B320 call. */
BATCH_PROJECTION_API renderer_raw32 HERCULES_CDECL
batch_projection_observe_slot107(void)
{
    renderer_raw32 raw_eax;
    volatile renderer_u32 *camera_x;
    volatile renderer_u32 *camera_y;
    volatile renderer_u32 *camera_z;
    volatile renderer_u16 *screen_x;
    volatile renderer_u16 *screen_y;
    int i;

    raw_eax = diagnostic_state.saved_slot107();
    camera_x = (volatile renderer_u32 *)CAMERA_X_ADDRESS;
    camera_y = (volatile renderer_u32 *)CAMERA_Y_ADDRESS;
    camera_z = (volatile renderer_u32 *)CAMERA_Z_ADDRESS;
    screen_x = (volatile renderer_u16 *)SCREEN_X_ADDRESS;
    screen_y = (volatile renderer_u16 *)SCREEN_Y_ADDRESS;
    for (i = 0; i != 3; ++i) {
        diagnostic_state.capture.camera_xyz_bits[i * 3 + 0] = camera_x[i];
        diagnostic_state.capture.camera_xyz_bits[i * 3 + 1] = camera_y[i];
        diagnostic_state.capture.camera_xyz_bits[i * 3 + 2] = camera_z[i];
        diagnostic_state.capture.projected_xy[i * 2 + 0] = screen_x[i];
        diagnostic_state.capture.projected_xy[i * 2 + 1] = screen_y[i];
    }
    return raw_eax;
}

BATCH_PROJECTION_API const struct batch_diagnostic_state * HERCULES_CDECL
batch_projection_diagnostic_state(void)
{
    return &diagnostic_state;
}
