#ifndef HERCULES_EDGE_STEPS_H
#define HERCULES_EDGE_STEPS_H

#include "hercules_types.h"

typedef char renderer_edge_steps_i32_size[(sizeof(renderer_i32) == 4) ? 1 : -1];
typedef char renderer_edge_steps_u32_size[(sizeof(renderer_u32) == 4) ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

enum renderer_edge_extent_status {
    RENDERER_EDGE_EXTENT_SKIP = 0,
    RENDERER_EDGE_EXTENT_ACCEPT = 1,
    RENDERER_EDGE_EXTENT_REJECT = 2
};

enum renderer_edge_step_status {
    RENDERER_EDGE_STEP_OK = 0,
    RENDERER_EDGE_STEP_SKIP_ZERO_DY = 1,
    RENDERER_EDGE_STEP_REJECT_DOMAIN = 2
};

struct renderer_edge_idiv_result {
    renderer_i32 quotient;
    renderer_i32 remainder;
};

/* Positive extents only: 0 skips, 1..2047 enters, >=2048 exits. */
enum renderer_edge_extent_status renderer_edge_check_extent(renderer_u32 extent);
renderer_u32 renderer_edge_reciprocal_entry(renderer_u32 denominator);

/* q16_delta is already the captured Q16 step input to native IMUL. It is not
 * a raw coordinate delta; callers must not shift it by 16 again. reciprocal
 * is the signed 32-bit table value captured immediately before IMUL. */
renderer_u32 renderer_edge_shrd30_from_q16_delta(
    renderer_i32 q16_delta, renderer_i32 reciprocal,
    renderer_u32 *product_low, renderer_u32 *product_high);

/* Model the observed C2E0 bounded sequence: SHL delta_x,16; CDQ; IDIV delta_y.
 * Only the accepted |delta_x|<2048, 1<=delta_y<2048 domain is modeled. */
enum renderer_edge_step_status renderer_edge_c2_guarded_idiv_step(
    renderer_i32 delta_x, renderer_i32 delta_y,
    struct renderer_edge_idiv_result *result);

#ifdef __cplusplus
}
#endif

#endif
