#ifndef HERCULES_SPRITE_RASTER_H
#define HERCULES_SPRITE_RASTER_H

#include <stddef.h>
#include "hercules_rectangle.h"

typedef struct HerculesSpritePlan {
    renderer_raw32 page_address;
    renderer_u32 x;
    renderer_u32 y;
    renderer_u32 width;
    renderer_u32 height;
    renderer_u32 u;
    renderer_u32 v;
    renderer_u32 source_start;
    renderer_u32 source_pitch;
    renderer_u32 destination_start;
    renderer_u32 destination_pitch;
    HerculesRectanglePixelMode pixel_mode;
    renderer_u32 blend_selector;
} HerculesSpritePlan;

typedef enum HerculesSpriteStatus {
    HERCULES_SPRITE_OK = 0,
    HERCULES_SPRITE_NO_DRAW = 1,
    HERCULES_SPRITE_INVALID_ARGUMENT = 2,
    HERCULES_SPRITE_UNSUPPORTED = 3,
    HERCULES_SPRITE_TRUNCATED = 4,
    HERCULES_SPRITE_RANGE_ERROR = 5,
    HERCULES_SPRITE_OBSERVER_FAILED = 6
} HerculesSpriteStatus;

#ifdef __cplusplus
extern "C" {
#endif

/* Resolve an already-decoded and AC60-staged sprite to an ordinary aligned
 * 256x256 texture page and 320x240 indexed surface. The caller supplies the
 * current E1/page state and the measured bottom bound of 240; sprite wire
 * records do not carry a CLUT or TPAGE. Other bottom bounds are unsupported.
 * Only post-clip UV spans that stay inside one 256-byte texture row and page
 * are supported. A suppressed or empty sprite returns NO_DRAW. The state view
 * does not expose native BA01F0/BA0210 surface-availability cells; a supplied
 * surface view models the available-target case, so NO_DRAW here describes
 * observed suppression or geometry exits, not that separate native branch. */
HerculesSpriteStatus hercules_sprite_resolve(
    const RendererDecodedPacket *packet, size_t packet_size,
    const RendererAc60StateView *state,
    const HerculesRectangleTextureView *texture,
    const HerculesRectangleSurfaceView *surface,
    HerculesSpritePlan *plan);

/* Render a resolved COPY or RAW_BLEND plan through the existing CAB0/C8F0
 * pixel steps. For RAW_BLEND, selected_blend is the one already-selected
 * 64-KiB table; it may be null when every source texel is zero. Source zero
 * skips table, destination, write, and observer callbacks. The observer is
 * called after a store with the source texel and destination byte offset. An
 * accepted all-transparent plan still requests presentation. */
HerculesSpriteStatus hercules_sprite_render(
    const HerculesSpritePlan *plan,
    const HerculesRectangleTextureView *texture,
    const HerculesRectangleSurfaceView *surface,
    const HercPixelTableReader *selected_blend,
    HerculesRectanglePixelObserver observer, void *observer_context,
    HerculesRectangleEffects *effects);

#ifdef __cplusplus
}
#endif

#endif
