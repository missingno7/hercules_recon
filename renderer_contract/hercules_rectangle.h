#ifndef HERCULES_RECTANGLE_H
#define HERCULES_RECTANGLE_H

#include <stddef.h>
#include "hercules_packets.h"
#include "hercules_pixels.h"
#include "hercules_staging.h"

/* Explicit storage views for the bounded D420 -> CAB0 rectangle route.
 * page_address identifies the original texture page selected by AC60 staging;
 * it is never interpreted as a process pointer. The byte pointers are local
 * adapters and may be null while resolving metadata-only views. */
typedef struct HerculesRectangleTextureView {
    renderer_raw32 page_address;
    const renderer_u8 *bytes;
    renderer_u32 length;
    renderer_u32 width;
    renderer_u32 height;
    renderer_u32 row_pitch;
} HerculesRectangleTextureView;

typedef struct HerculesRectangleSurfaceView {
    renderer_u8 *bytes;
    renderer_u32 length;
    renderer_u32 width;
    renderer_u32 height;
    renderer_u32 row_pitch;
} HerculesRectangleSurfaceView;

typedef struct HerculesRectanglePlan {
    renderer_raw32 page_address;
    renderer_u32 x;
    renderer_u32 y;
    renderer_u32 width;
    renderer_u32 height;
    renderer_u32 u;
    renderer_u32 v;
    /* Byte offsets relative to the supplied storage views. */
    renderer_u32 source_start;
    renderer_u32 source_pitch;
    renderer_u32 destination_start;
    renderer_u32 destination_pitch;
} HerculesRectanglePlan;

typedef enum HerculesRectangleStatus {
    HERCULES_RECTANGLE_OK = 0,
    HERCULES_RECTANGLE_INVALID_ARGUMENT = 1,
    HERCULES_RECTANGLE_UNSUPPORTED = 2,
    HERCULES_RECTANGLE_TRUNCATED = 3,
    HERCULES_RECTANGLE_RANGE_ERROR = 4,
    HERCULES_RECTANGLE_OBSERVER_FAILED = 5
} HerculesRectangleStatus;

typedef struct HerculesRectangleEffects {
    renderer_u32 pixel_store_count;
    renderer_u8 accepted;
    renderer_u8 presentation_requested;
} HerculesRectangleEffects;

/* Called after each nonzero CAB0 source texel is stored. Offsets are relative
 * to the explicit surface view. Returning zero stops rendering and reports
 * OBSERVER_FAILED. A null observer is allowed. */
typedef int (*HerculesRectanglePixelObserver)(
    void *context, renderer_u8 source_texel, renderer_u32 destination_offset);

#ifdef __cplusplus
extern "C" {
#endif

/* Resolve a complete FT4 and already-applied AC60 staging state into the
 * finite direct-rectangle subset. Metadata is checked without reading either
 * byte view; null bytes are allowed here. packet_size is the available wire
 * span and distinguishes a truncated packet from an unsupported one. */
HerculesRectangleStatus hercules_rectangle_resolve(
    const RendererDecodedPacket *packet, size_t packet_size,
    const RendererAc60StateView *state,
    const HerculesRectangleTextureView *texture,
    const HerculesRectangleSurfaceView *surface,
    HerculesRectanglePlan *plan);

/* Render a resolved plan through CAB0's existing zero-skip pixel step. This
 * requires actual texture and surface bytes. A valid accepted rectangle sets
 * presentation_requested even when every source texel is zero; presentation
 * is not inferred from pixel_store_count. */
HerculesRectangleStatus hercules_rectangle_render(
    const HerculesRectanglePlan *plan,
    const HerculesRectangleTextureView *texture,
    const HerculesRectangleSurfaceView *surface,
    HerculesRectanglePixelObserver observer, void *observer_context,
    HerculesRectangleEffects *effects);

#ifdef __cplusplus
}
#endif

#endif
