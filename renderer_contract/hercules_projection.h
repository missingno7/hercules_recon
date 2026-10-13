#ifndef HERCULES_PROJECTION_H
#define HERCULES_PROJECTION_H

#include <stddef.h>
#include "hercules_host_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && defined(HERCULES_PROJECTION_BUILD_EXPORTS)
#define HERCULES_PROJECTION_API __declspec(dllexport)
#else
#define HERCULES_PROJECTION_API
#endif

/* Slot constants are owned by hercules_host_abi.h. */
#define HERCULES_PROJECTION_SLOT_POINT_INPUT HERCULES_SLOT_POINT_INPUT
#define HERCULES_PROJECTION_SLOT_PROJECT HERCULES_SLOT_PROJECT
#define HERCULES_PROJECTION_SLOT_SCREEN_OUTPUT HERCULES_SLOT_SCREEN_OUTPUT

#define HERCULES_PROJECTION_RVA_POINT_INPUT 0x0000b6f0UL
#define HERCULES_PROJECTION_RVA_SCREEN_OUTPUT 0x0000b720UL
#define HERCULES_PROJECTION_RVA_TRANSFORM 0x0000b740UL
#define HERCULES_PROJECTION_RVA_PROJECT 0x0000b920UL

#define HERCULES_PROJECTION_CAMERA_X_ADDRESS 0x00a8e71cUL
#define HERCULES_PROJECTION_CAMERA_Y_ADDRESS 0x00a8e728UL
#define HERCULES_PROJECTION_CAMERA_Z_ADDRESS 0x00a8e734UL
#define HERCULES_PROJECTION_SCREEN_X_ADDRESS 0x00a8e740UL
#define HERCULES_PROJECTION_SCREEN_Y_ADDRESS 0x00a8e746UL

/* Physical callback bit-pattern signatures; raw EAX is renderer_raw32.
 * Native pointer types exist only in the explicit 32-bit-live opt-in. */
#ifdef HERCULES_RENDERER_LIVE_X86
RENDERER_ASSERT_LIVE_X86_POINTER_WIDTH(hercules_projection_live_callbacks_require_x86);
typedef renderer_raw32 (HERCULES_CDECL *HerculesProjectionSlot101Callback)(
    renderer_u16 *xyz_words);
typedef renderer_raw32 (HERCULES_CDECL *HerculesProjectionSlot106Callback)(void);
typedef renderer_raw32 (HERCULES_CDECL *HerculesProjectionSlot114Callback)(
    renderer_u16 *xy_words);
#endif

/* Joined tool-side values assembled from the matrix/translation and
 * center/focal setter state. This is NOT a contiguous original host struct. */
struct hercules_projection_config_view {
    renderer_i16 matrix_row_major[9];
    renderer_i32 translation_xyz[3];
    renderer_i32 center_x;
    renderer_i32 center_y;
    renderer_u32 focal;
};

/* B6F0 reads three WORDs; B740 later sign-extends each WORD as an input. */
struct hercules_projection_point_view {
    renderer_i16 xyz[3];
};

/* Joined tool-side post-B920 view; not an original host allocation/layout.
 * Camera coordinates are sign-extended low WORDs from B740; camera_z has
 * already been clamped to at least 1. projected_xy are the stored WORDs.
 * raw_eax preserves the complete physical return bit pattern from B920. */
struct hercules_projection_result_view {
    renderer_i32 camera_x;
    renderer_i32 camera_y;
    renderer_i32 camera_z;
    renderer_u16 projected_x;
    renderer_u16 projected_y;
    renderer_raw32 raw_eax;
};

typedef char hercules_projection_point_size_is_6[
    (sizeof(struct hercules_projection_point_view) == 6) ? 1 : -1];
typedef char hercules_projection_config_translation_offset_is_20[
    (offsetof(struct hercules_projection_config_view, translation_xyz) == 20) ? 1 : -1];
typedef char hercules_projection_config_center_offset_is_32[
    (offsetof(struct hercules_projection_config_view, center_x) == 32) ? 1 : -1];
typedef char hercules_projection_config_focal_offset_is_40[
    (offsetof(struct hercules_projection_config_view, focal) == 40) ? 1 : -1];
typedef char hercules_projection_config_view_size_is_44[
    (sizeof(struct hercules_projection_config_view) == 44) ? 1 : -1];
typedef char hercules_projection_result_view_size_is_20[
    (sizeof(struct hercules_projection_result_view) == 20) ? 1 : -1];

HERCULES_PROJECTION_API void hercules_projection_eval(
    const struct hercules_projection_config_view *config,
    const struct hercules_projection_point_view *point,
    struct hercules_projection_result_view *result);

#ifdef __cplusplus
}
#endif

#endif
