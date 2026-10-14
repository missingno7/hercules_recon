#ifndef HERCULES_COMPOSITION_RASTER_H
#define HERCULES_COMPOSITION_RASTER_H

#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "composition_events.h"
#include "hercules_frame.h"
#include "hercules_rectangle.h"
#include "hercules_staging.h"
#ifdef __cplusplus
}
#endif

typedef enum CompositionRasterStatus {
    COMPOSITION_RASTER_OK = 0,
    COMPOSITION_RASTER_INVALID = 1,
    COMPOSITION_RASTER_UNSUPPORTED = 2,
    COMPOSITION_RASTER_PROVIDER_FAILED = 3,
    COMPOSITION_RASTER_FRAME_FAILED = 4,
    COMPOSITION_RASTER_OVERFLOW = 5
} CompositionRasterStatus;

typedef enum CompositionRasterService {
    COMPOSITION_RASTER_SERVICE_LOCK = 1,
    COMPOSITION_RASTER_SERVICE_REMAP = 2,
    COMPOSITION_RASTER_SERVICE_UNLOCK = 3,
    COMPOSITION_RASTER_SERVICE_PRESENT = 4,
    COMPOSITION_RASTER_SERVICE_STRETCH_DIBITS = 5,
    COMPOSITION_RASTER_SERVICE_GDI_FLUSH = 6
} CompositionRasterService;

typedef struct CompositionRasterProviderCall {
    CompositionRasterService service;
    CompositionEvent event;
    renderer_u32 argument_count;
    renderer_u32 argument[13];
} CompositionRasterProviderCall;

typedef int (*CompositionRasterProvider)(
    void *context, const CompositionRasterProviderCall *call);

typedef struct CompositionRasterConfig {
    FrameRaw32Resolver resolver;
    void *resolver_context;
    renderer_raw32 order_table_base;
    renderer_u32 order_table_slot_count;
    RendererAc60StateView *state;
    const renderer_u8 *texture_bytes;
    renderer_u32 texture_length;
    renderer_u8 *surface_bytes;
    renderer_u32 surface_length;
    renderer_raw32 texture_token;
    renderer_raw32 surface_token;
    renderer_raw32 dib_info_token;
    renderer_raw32 hdc_token;
    renderer_u32 surface_pitch;
    renderer_u8 dirty_gate;
    renderer_u8 remap_guard;
    CompositionRasterProvider provider;
    void *provider_context;
} CompositionRasterConfig;

/* Bounded source model for an in-bounds, unblended AC60 FT4 accepted by D420's
 * instruction-backed axis-aligned, 1:1-UV rectangle fast-path predicate. It
 * derives rectangle/UV addresses and models CAB0's zero-skip copy loop for up
 * to 8x8 pixels. Other packet/flag/geometry domains return explicit non-success
 * before framebuffer writes. The live-link walker reads each successor after
 * its visitor, preserving AC60's packet-link mutation boundary. The frame tail
 * remains the seeded 70C0 equality-guard skip, then unlock and present request. */
#ifdef __cplusplus
extern "C" {
#endif
CompositionRasterStatus composition_run_ft4_direct_copy_frame(
    const CompositionRasterConfig *config,
    CompositionTrace *trace);
#ifdef __cplusplus
}
#endif

#endif
