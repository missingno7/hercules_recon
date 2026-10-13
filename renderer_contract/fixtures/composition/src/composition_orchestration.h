#ifndef HERCULES_COMPOSITION_ORCHESTRATION_H
#define HERCULES_COMPOSITION_ORCHESTRATION_H

#include "composition_events.h"
#include "hercules_frame.h"
#include "hercules_staging.h"

typedef enum CompositionControlledService {
    COMPOSITION_SERVICE_LOCK = 1,
    COMPOSITION_SERVICE_CALLBACK = 2,
    COMPOSITION_SERVICE_REMAP = 3,
    COMPOSITION_SERVICE_UNLOCK = 4,
    COMPOSITION_SERVICE_PRESENT_REQUEST = 5,
    COMPOSITION_SERVICE_STRETCH_DIBITS = 6,
    COMPOSITION_SERVICE_GDI_FLUSH = 7,
    COMPOSITION_SERVICE_MOVE_REQUEST = 8
} CompositionControlledService;

/* A provider is an explicitly controlled fixture endpoint. A missing,
 * rejected, or unknown endpoint is an incomplete result, never a no-op. */
typedef struct CompositionProviderCall {
    CompositionControlledService service;
    CompositionEvent event;
    renderer_u32 argument_count;
    renderer_u32 argument[13];
} CompositionProviderCall;

typedef int (*CompositionControlledProvider)(
    void *context, const CompositionProviderCall *call);

typedef struct CompositionOrchestrationConfig {
    FrameRaw32Resolver resolver;
    void *resolver_context;
    renderer_raw32 order_table_base;
    renderer_u32 order_table_slot_count;
    RendererAc60StateView *state;
    renderer_raw32 callback_endpoint;
    renderer_raw32 cpu_pixel_token;
    renderer_raw32 dib_info_token;
    renderer_raw32 window_hdc_token;
    renderer_u32 cpu_pitch_bytes;
    renderer_u8 dirty_present_gate;
    renderer_raw32 rewrite_packet_token;
    renderer_raw32 replacement_link;
    CompositionControlledProvider provider;
    void *provider_context;
} CompositionOrchestrationConfig;

/* Executes only the bounded state-only command subset: E1 state, command-01
 * deferred-copy notification, and 0x40 terminator. Raster/unknown packets,
 * missing providers, malformed links, and capacity failures are explicit
 * non-success. Surface and display providers are synthetic test services; no
 * pixel bytes are rendered or claimed. */
CompositionTraceStatus composition_run_state_only_frame(
    const CompositionOrchestrationConfig *config,
    CompositionTrace *trace);

#endif
