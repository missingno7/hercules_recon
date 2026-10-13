#ifndef HERCULES_REPROJECTION_H
#define HERCULES_REPROJECTION_H

#include "hercules_projection.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && defined(HERCULES_REPROJECTION_BUILD_EXPORTS)
#define HERCULES_REPROJECTION_API __declspec(dllexport)
#else
#define HERCULES_REPROJECTION_API
#endif

#define HERCULES_REPROJECTION_NATIVE_WIDTH 320UL
#define HERCULES_REPROJECTION_NATIVE_HEIGHT 240UL
#define HERCULES_REPROJECTION_MAX_SCALE 64UL
#define HERCULES_REPROJECTION_BATCH_COUNT HERCULES_PROJECTION_BATCH_POINT_COUNT

/* A private, double-precision point view. These are policy coordinates, not
 * original screen state or a replacement for either projection result view. */
struct hercules_reprojection_private_vertex_view {
    renderer_i32 camera_x_input;
    renderer_i32 camera_y_input;
    renderer_i32 camera_z_input;
    double x;
    double y;
    renderer_i32 status;
    renderer_i32 camera_provenance;
};

typedef char hercules_reprojection_private_view_size_is_40[
    (sizeof(struct hercules_reprojection_private_vertex_view) == 40) ? 1 : -1];
typedef char hercules_reprojection_private_x_offset_is_16[
    (offsetof(struct hercules_reprojection_private_vertex_view, x) == 16) ? 1 : -1];
typedef char hercules_reprojection_double_is_8[(sizeof(double) == 8) ? 1 : -1];

enum hercules_reprojection_status {
    HERCULES_REPROJECTION_OK = 0,
    HERCULES_REPROJECTION_ZERO_DEPTH = 1,
    HERCULES_REPROJECTION_BEHIND_CAMERA = 2,
    HERCULES_REPROJECTION_UNSUPPORTED_POLICY = 3,
    HERCULES_REPROJECTION_CAMERA_UNAVAILABLE = 4,
    HERCULES_REPROJECTION_REFERENCE_DIVERGENCE = 5,
    HERCULES_REPROJECTION_BAD_ARGUMENT = 6
};

enum hercules_reprojection_camera_provenance {
    HERCULES_REPROJECTION_PROVENANCE_UNAVAILABLE = 0,
    HERCULES_REPROJECTION_CAMERA_CALLER_SUPPLIED = 1,
    HERCULES_REPROJECTION_CAMERA_DERIVED_B320_REFERENCE = 2
};

/* Deliberate tool-side rendering policy: scale the configured center and the
 * signed effective focal WORD by an integer 4:3 resolution scale (six gives
 * 1920x1440), then project supplied signed low-WORD camera XYZ with floating-
 * point division. Signed effective focal is sign_extend16(focal & 0xffff),
 * matching the signed value retained by the native focal<<16 DWORD wrap.
 * Native screen WORDs are never inputs. The function rejects unsupported
 * scale/camera widths and nonpositive depth; it does not clip. Values passed
 * here can be captured camera lanes or separately labelled derived lanes.
 * A post-B920 scalar capture has already clamped Z to 1 and cannot reveal an
 * earlier nonpositive depth; this API does not infer that lost information. */
/* The result is initialized before config fields are read. Config and result
 * storage must not overlap; this tool-side API does not model native aliasing. */
HERCULES_REPROJECTION_API renderer_i32 hercules_reprojection_from_camera(
    const struct hercules_projection_config_view *config,
    renderer_i32 camera_x,
    renderer_i32 camera_y,
    renderer_i32 camera_z,
    renderer_u32 resolution_scale,
    struct hercules_reprojection_private_vertex_view *result);

/* Generate scalar-compatible output with the shared B6F0/B740/B920 model.
 * B920's observed post-callback Z lane is clamped. This helper separately
 * derives pre-clamp camera lanes through the B320 reference and labels them
 * HERCULES_REPROJECTION_CAMERA_DERIVED_B320_REFERENCE; those lanes are not a
 * native scalar callback capture. The compatibility result remains available
 * even when the private policy rejects derived Z<=0. Config, point,
 * compatibility and private_result storage must be separate and nonoverlapping. */
HERCULES_REPROJECTION_API renderer_i32 hercules_reprojection_eval_single(
    const struct hercules_projection_config_view *config,
    const struct hercules_projection_point_view *point,
    renderer_u32 resolution_scale,
    struct hercules_projection_result_view *compatibility,
    struct hercules_reprojection_private_vertex_view *private_result);

/* Evaluate the shared B320 model and privately reproject each camera record.
 * `compatibility` is a joined tool view of separate host lanes. On a native
 * batch divide-overflow status, points through fault_point have camera lanes;
 * later private views are marked CAMERA_UNAVAILABLE. B320's zero-Z native
 * screen output is retained even though the private policy rejects zero Z.
 * Config, points, compatibility, private_results and any optional output
 * pointers must refer to nonoverlapping storage. These tool-side values do
 * not declare native alias behavior. */
HERCULES_REPROJECTION_API renderer_i32 hercules_reprojection_eval_batch(
    const struct hercules_projection_config_view *config,
    const struct hercules_projection_point_view points[
        HERCULES_REPROJECTION_BATCH_COUNT],
    renderer_u32 resolution_scale,
    struct hercules_projection_batch_result_view *compatibility,
    struct hercules_reprojection_private_vertex_view private_results[
        HERCULES_REPROJECTION_BATCH_COUNT],
    renderer_u32 *fault_point,
    renderer_raw32 *raw_eax);

#ifdef __cplusplus
}
#endif

#endif
