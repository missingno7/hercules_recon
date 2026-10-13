#ifndef RENDERER_SPRITE_GEOMETRY_H
#define RENDERER_SPRITE_GEOMETRY_H

#include "hercules_types.h"

/* Bounded per-packet view for the original C8F0/CAB0 pre-pixel setup.
 * x/y must be the AC60 sign-extended packet coordinates. Width/height retain
 * their raw words because the native routines sign-extend them at use time. */
typedef struct renderer_sprite_geometry {
    renderer_i16 x;
    renderer_i16 y;
    renderer_u16 width_word;
    renderer_u16 height_word;
    renderer_u32 u_offset;
    renderer_u32 v_offset;
    renderer_raw32 selected_page_base;
    renderer_raw32 destination;
    renderer_u32 stride;
    renderer_i32 bottom_limit;
    renderer_raw32 destination_start;
    renderer_raw32 page_u_base;
    renderer_raw32 first_row_v_offset;
    renderer_raw32 blend_table;
} renderer_sprite_geometry;

#define RENDERER_SPRITE_COPY_PATH 0
#define RENDERER_SPRITE_BLEND_PATH 1
#define RENDERER_SPRITE_NO_DRAW 0
#define RENDERER_SPRITE_PREPARED 1

/* Returns PREPARED only after the original left/top/right/bottom clipping
 * and positive signed extents checks succeed. Bottom is a host screen bound;
 * the supported view uses 0..32767. Right is fixed at 320; F3 left/top/right
 * do not supply these sprite limits. This view is not a native memory layout. On NO_DRAW, native-visible
 * left/top geometry/UV mutations are retained, while pointer setup outputs
 * remain untouched. `geometry` is required to point to writable storage. */
#ifdef __cplusplus
extern "C" {
#endif
renderer_i32 renderer_sprite_geometry_prepare(
    renderer_sprite_geometry *geometry,
    renderer_u8 draw_gate,
    renderer_u8 path,
    renderer_u32 blend_selector);
#ifdef __cplusplus
}
#endif

#endif
