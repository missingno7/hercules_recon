#include "composition_orchestration.h"

#include <string.h>

typedef struct CompositionVisitContext {
    const CompositionOrchestrationConfig *config;
    CompositionTrace *trace;
    renderer_u32 callback_seen;
    renderer_u32 callback_mutated_link;
} CompositionVisitContext;

static void init_event(CompositionEvent *event, CompositionTrace *trace,
                       composition_u16 kind, composition_u8 origin,
                       composition_u8 domain, composition_u32 source_rva,
                       composition_u32 raw_token)
{
    memset(event, 0, sizeof(*event));
    event->scope_id = 1U;
    event->local_order = trace->count;
    /* In a reconstructed stream this is a per-operation sequence number, not
     * a native instruction count. The flag and origin make that distinction. */
    event->source_order = trace->count + 1U;
    event->kind = kind;
    event->origin = origin;
    event->domain = domain;
    event->semantic_owner = COMPOSITION_OWNER_UNCLASSIFIED;
    event->source_rva = source_rva;
    event->raw_token = raw_token;
    event->flags = COMPOSITION_EVENT_FLAG_C_RECONSTRUCTION;
}

static CompositionTraceStatus emit_event(CompositionTrace *trace,
                                         CompositionEvent *event)
{
    if (trace == 0 || event == 0)
        return COMPOSITION_TRACE_INVALID;
    event->local_order = trace->count;
    event->source_order = trace->count + 1U;
    return composition_trace_append(trace, event);
}

static CompositionTraceStatus fail_unknown(CompositionTrace *trace,
                                           renderer_u32 source_rva,
                                           renderer_u32 raw_token,
                                           renderer_u32 tag)
{
    CompositionEvent event;
    CompositionTraceStatus status;

    init_event(&event, trace, COMPOSITION_EVENT_UNKNOWN_WRITER,
               COMPOSITION_ORIGIN_UNRESOLVED,
               COMPOSITION_DOMAIN_EXTERNAL, source_rva, raw_token);
    event.argument[0] = tag;
    status = emit_event(trace, &event);
    if (status == COMPOSITION_TRACE_OK)
        return COMPOSITION_TRACE_INCOMPLETE_UNKNOWN;
    return status;
}

static int read_packet(FrameRaw32Resolver resolver, void *resolver_context,
                       renderer_raw32 address, renderer_u32 length,
                       const renderer_u8 **bytes)
{
    if (resolver == 0 || bytes == 0 || address == 0 || length < 4U)
        return 0;
    return resolver(resolver_context, address, length, bytes) && *bytes != 0;
}

static renderer_u32 read_u32_le(const renderer_u8 *bytes)
{
    return (renderer_u32)bytes[0] |
           ((renderer_u32)bytes[1] << 8) |
           ((renderer_u32)bytes[2] << 16) |
           ((renderer_u32)bytes[3] << 24);
}

static int read_live_link(const CompositionOrchestrationConfig *config,
                          renderer_raw32 packet, renderer_u32 *next)
{
    const renderer_u8 *bytes;
    if (next == 0 || !read_packet(config->resolver, config->resolver_context,
                                  packet, 4U, &bytes))
        return 0;
    *next = read_u32_le(bytes);
    return 1;
}

static int call_provider(const CompositionOrchestrationConfig *config,
                         CompositionControlledService service,
                         const CompositionEvent *event,
                         const renderer_u32 *arguments,
                         renderer_u32 argument_count)
{
    CompositionProviderCall call;
    renderer_u32 index;

    if (config == 0 || config->provider == 0 || event == 0)
        return 0;
    if (argument_count > 13U ||
        (argument_count != 0 && arguments == 0))
        return 0;
    memset(&call, 0, sizeof(call));
    call.service = service;
    call.event = *event;
    call.argument_count = argument_count;
    for (index = 0; index < argument_count; ++index)
        call.argument[index] = arguments[index];
    return config->provider(config->provider_context, &call) != 0;
}

static CompositionTraceStatus emit_service(
    const CompositionOrchestrationConfig *config, CompositionTrace *trace,
    CompositionControlledService service, CompositionEvent *event,
    const renderer_u32 *arguments, renderer_u32 argument_count)
{
    if (!call_provider(config, service, event, arguments, argument_count))
        return fail_unknown(trace, event->source_rva, event->raw_token,
                            (renderer_u32)service);
    return emit_event(trace, event);
}

static CompositionTraceStatus emit_source(
    const CompositionOrchestrationConfig *config, CompositionTrace *trace,
    CompositionControlledService provider_service, CompositionEvent *event,
    const renderer_u32 *arguments, renderer_u32 argument_count)
{
    if (!call_provider(config, provider_service, event, arguments,
                       argument_count))
        return fail_unknown(trace, event->source_rva, event->raw_token,
                            (renderer_u32)provider_service);
    return emit_event(trace, event);
}

static int emit_callback_entry(CompositionVisitContext *visit,
                               renderer_u32 return_rva)
{
    CompositionEvent event;
    init_event(&event, visit->trace, COMPOSITION_EVENT_CALLBACK_ENTRY,
               COMPOSITION_ORIGIN_STATIC_ORDER,
               COMPOSITION_DOMAIN_EXTERNAL, 0xBD80U,
               visit->config->callback_endpoint);
    event.argument[0] = return_rva;
    return emit_event(visit->trace, &event) == COMPOSITION_TRACE_OK;
}

static int service_callback(CompositionVisitContext *visit,
                           renderer_u32 return_rva,
                           renderer_u32 mutation_address,
                           renderer_u32 old_link,
                           renderer_u32 new_link,
                           int has_mutation)
{
    CompositionEvent event;
    renderer_u32 call_arguments[4];
    CompositionTraceStatus status;

    init_event(&event, visit->trace, COMPOSITION_EVENT_CALLBACK_SERVICE,
               COMPOSITION_ORIGIN_CONTROLLED_ENDPOINT,
               COMPOSITION_DOMAIN_EXTERNAL, 0,
               visit->config->callback_endpoint);
    event.argument[0] = return_rva;
    if (has_mutation) {
        event.argument[1] = mutation_address;
        event.argument[2] = old_link;
        event.argument[3] = new_link;
        call_arguments[0] = 0x40ABE9U;
        call_arguments[1] = 0U;
        call_arguments[2] = mutation_address;
        call_arguments[3] = visit->config->order_table_base + 4U;
    } else {
        call_arguments[0] = 0x40B2E7U;
        call_arguments[1] = 0U;
        call_arguments[2] = 0U;
        call_arguments[3] = 0U;
    }
    status = emit_service(visit->config, visit->trace,
                          COMPOSITION_SERVICE_CALLBACK, &event,
                          call_arguments, 4U);
    if (status != COMPOSITION_TRACE_OK)
        return 0;
    return 1;
}

static int emit_state_stores(CompositionVisitContext *visit,
                             const RendererAc60StateView *state)
{
    CompositionEvent event;
    static const renderer_u32 rvas[3] = { 0xB1B0U, 0xB1BCU, 0xB1CAU };
    static const renderer_u32 addresses[3] = {
        0x00A8E6FCU, 0x00A8E6F4U, 0x00A8E6F8U
    };
    renderer_u32 values[3];
    renderer_u32 i;

    values[0] = state->page.alternate_bank;
    values[1] = state->page.page_selector;
    values[2] = state->page.page_mode;
    for (i = 0; i < 3U; ++i) {
        init_event(&event, visit->trace, COMPOSITION_EVENT_STATE_COMMAND,
                   COMPOSITION_ORIGIN_STATIC_ORDER,
                   COMPOSITION_DOMAIN_NONE, rvas[i], addresses[i]);
        event.argument[0] = values[i];
        event.argument[1] = 4U;
        if (emit_event(visit->trace, &event) != COMPOSITION_TRACE_OK)
            return 0;
    }
    return 1;
}

static int consume_packet(void *opaque, renderer_raw32 packet_address)
{
    CompositionVisitContext *visit = (CompositionVisitContext *)opaque;
    const CompositionOrchestrationConfig *config;
    const renderer_u8 *wire;
    RendererAc60Event applied;
    RendererAc60ApplyStatus apply_status;
    CompositionEvent event;
    renderer_u32 move_arguments[6];
    renderer_u32 next;
    renderer_u32 tag;
    renderer_u32 length;

    if (visit == 0 || visit->config == 0 || visit->trace == 0)
        return 0;
    config = visit->config;
    if (!read_packet(config->resolver, config->resolver_context,
                     packet_address, 8U, &wire)) {
        (void)fail_unknown(visit->trace, 0xACB9U, packet_address, 0U);
        return 0;
    }
    tag = wire[7];
    if (tag == 0xE1U)
        length = 8U;
    else if (tag == 0x01U)
        length = 0x18U;
    else if (tag == 0x40U)
        length = 8U;
    else {
        (void)fail_unknown(visit->trace, 0xACB9U, packet_address, tag);
        return 0;
    }
    if (!read_packet(config->resolver, config->resolver_context,
                     packet_address, length, &wire)) {
        (void)fail_unknown(visit->trace, 0xACB9U, packet_address, tag);
        return 0;
    }
    if (!read_live_link(config, packet_address, &next)) {
        (void)fail_unknown(visit->trace, 0xACB9U, packet_address, tag);
        return 0;
    }

    init_event(&event, visit->trace, COMPOSITION_EVENT_PACKET_CONSUMED,
               COMPOSITION_ORIGIN_STATIC_ORDER,
               COMPOSITION_DOMAIN_ORDER_TABLE, 0xACB9U, packet_address);
    event.argument[0] = tag;
    event.argument[1] = next;
    if (emit_event(visit->trace, &event) != COMPOSITION_TRACE_OK)
        return 0;

    memset(&applied, 0, sizeof(applied));
    apply_status = renderer_ac60_apply_packet(
        wire, (size_t)length, config->state, &applied);
    if (tag == 0xE1U) {
        if (apply_status != RENDERER_AC60_APPLY_OK ||
            applied.kind != RENDERER_AC60_EVENT_E1_STATE ||
            !emit_state_stores(visit, config->state))
            return 0;
        return 1;
    }
    if (tag == 0x40U)
        return apply_status == RENDERER_AC60_APPLY_OK &&
               applied.kind == RENDERER_AC60_EVENT_OT_GATE;

    if (apply_status != RENDERER_AC60_APPLY_DEFERRED ||
        applied.kind != RENDERER_AC60_EVENT_DEFERRED_COPY)
        return 0;
    init_event(&event, visit->trace, COMPOSITION_EVENT_VRAM_MOVE_REQUEST,
               COMPOSITION_ORIGIN_STATIC_ORDER,
               COMPOSITION_DOMAIN_TEXTURE, 0xA8F0U, packet_address + 0x10U);
    event.argument[0] = (composition_u32)applied.deferred_copy.destination_x;
    event.argument[1] = (composition_u32)applied.deferred_copy.destination_y;
    event.argument[2] = (composition_u32)applied.deferred_copy.width;
    event.argument[3] = (composition_u32)applied.deferred_copy.height;
    move_arguments[0] = (renderer_u32)applied.deferred_copy.destination_x;
    move_arguments[1] = (renderer_u32)applied.deferred_copy.destination_y;
    move_arguments[2] = (renderer_u32)applied.deferred_copy.source_x;
    move_arguments[3] = (renderer_u32)applied.deferred_copy.source_y;
    move_arguments[4] = (renderer_u32)applied.deferred_copy.width;
    move_arguments[5] = (renderer_u32)applied.deferred_copy.height;
    if (emit_source(visit->config, visit->trace,
                    COMPOSITION_SERVICE_MOVE_REQUEST, &event,
                    move_arguments, 6U) != COMPOSITION_TRACE_OK)
        return 0;

    if (visit->callback_seen != 0 ||
        packet_address != config->rewrite_packet_token ||
        !read_live_link(config, packet_address, &next) ||
        !emit_callback_entry(visit, 0xABE9U))
        return 0;
    if (!service_callback(visit, 0xABE9U, packet_address, next,
                          config->replacement_link, 1))
        return 0;
    visit->callback_seen = 1U;
    visit->callback_mutated_link = config->replacement_link;
    if (!read_live_link(config, packet_address, &next) ||
        next != config->replacement_link) {
        (void)fail_unknown(visit->trace, 0xACA2U, packet_address, next);
        return 0;
    }
    init_event(&event, visit->trace, COMPOSITION_EVENT_LIVE_LINK_RELOAD,
               COMPOSITION_ORIGIN_STATIC_ORDER,
               COMPOSITION_DOMAIN_ORDER_TABLE, 0xACA2U, packet_address);
    event.argument[0] = next;
    return emit_event(visit->trace, &event) == COMPOSITION_TRACE_OK;
}

static CompositionTraceStatus surface_event(
    const CompositionOrchestrationConfig *config, CompositionTrace *trace,
    CompositionControlledService service, composition_u16 kind,
    composition_u32 source_rva, renderer_u32 raw_token,
    renderer_u32 argument0)
{
    CompositionEvent event;
    renderer_u32 provider_argument;
    const renderer_u32 *provider_arguments;
    renderer_u32 provider_argument_count;

    init_event(&event, trace, kind, COMPOSITION_ORIGIN_STATIC_ORDER,
               (kind == COMPOSITION_EVENT_PRESENT_REQUEST)
                   ? COMPOSITION_DOMAIN_DISPLAY : COMPOSITION_DOMAIN_CPU_SURFACE,
               source_rva, raw_token);
    event.argument[0] = argument0;
    provider_argument = argument0;
    provider_arguments = 0;
    provider_argument_count = 0;
    if (service == COMPOSITION_SERVICE_REMAP) {
        provider_arguments = &provider_argument;
        provider_argument_count = 1U;
    }
    return emit_source(config, trace, service, &event,
                       provider_arguments, provider_argument_count);
}

CompositionTraceStatus composition_run_state_only_frame(
    const CompositionOrchestrationConfig *config,
    CompositionTrace *trace)
{
    CompositionEvent event;
    CompositionVisitContext visit;
    renderer_raw32 visited[32];
    renderer_u32 dib_arguments[13];
    FrameVisitGuard guard;
    FrameWalkStatus walk_status;
    CompositionTraceStatus status;

    if (config == 0 || trace == 0 || config->resolver == 0 ||
        config->state == 0 || config->order_table_base == 0 ||
        config->order_table_slot_count == 0 || config->provider == 0 ||
        config->callback_endpoint == 0 || config->cpu_pixel_token == 0 ||
        config->dib_info_token == 0 || config->window_hdc_token == 0 ||
        config->cpu_pitch_bytes == 0 || config->dirty_present_gate != 1U ||
        config->state->raster_suppression_flag != 0 ||
        config->rewrite_packet_token == 0 || config->replacement_link == 0)
        return COMPOSITION_TRACE_INVALID;

    init_event(&event, trace, COMPOSITION_EVENT_FRAME_ENTRY,
               COMPOSITION_ORIGIN_STATIC_ORDER,
               COMPOSITION_DOMAIN_ORDER_TABLE, 0xAC60U, 0);
    status = emit_event(trace, &event);
    if (status != COMPOSITION_TRACE_OK)
        return status;
    status = surface_event(config, trace, COMPOSITION_SERVICE_LOCK,
                           COMPOSITION_EVENT_SURFACE_LOCK, 0x6FF0U,
                           config->cpu_pixel_token,
                           config->cpu_pitch_bytes);
    if (status != COMPOSITION_TRACE_OK)
        return status;

    visit.config = config;
    visit.trace = trace;
    visit.callback_seen = 0;
    visit.callback_mutated_link = 0;
    frame_visit_guard_init(&guard, visited,
                           (renderer_u32)(sizeof(visited) / sizeof(visited[0])));
    walk_status = frame_walk_ac60_order_table_visit(
        config->resolver, config->resolver_context,
        config->order_table_base, config->order_table_slot_count,
        &guard, consume_packet, &visit);
    if (walk_status != FRAME_WALK_OK) {
        if (trace->status == COMPOSITION_TRACE_INCOMPLETE_UNKNOWN)
            return trace->status;
        return fail_unknown(trace, 0xAC60U, config->order_table_base,
                            (renderer_u32)walk_status);
    }
    if (visit.callback_seen != 1U ||
        visit.callback_mutated_link != config->replacement_link)
        return fail_unknown(trace, 0xACA2U, config->rewrite_packet_token, 0U);

    status = surface_event(config, trace, COMPOSITION_SERVICE_REMAP,
                           COMPOSITION_EVENT_SURFACE_REMAP_REQUEST, 0x70C0U,
                           config->cpu_pixel_token, 0xFFU);
    if (status != COMPOSITION_TRACE_OK)
        return status;
    status = surface_event(config, trace, COMPOSITION_SERVICE_UNLOCK,
                           COMPOSITION_EVENT_SURFACE_UNLOCK, 0x7140U, 0U, 0U);
    if (status != COMPOSITION_TRACE_OK)
        return status;
    status = surface_event(config, trace,
                           COMPOSITION_SERVICE_PRESENT_REQUEST,
                           COMPOSITION_EVENT_PRESENT_REQUEST, 0x6620U, 0,
                           config->dirty_present_gate);
    if (status != COMPOSITION_TRACE_OK)
        return status;

    init_event(&event, trace, COMPOSITION_EVENT_FINAL_OUTPUT,
               COMPOSITION_ORIGIN_CONTROLLED_ENDPOINT,
               COMPOSITION_DOMAIN_DISPLAY, 0, config->window_hdc_token);
    event.argument[0] = 0x66A6U;
    dib_arguments[0] = config->window_hdc_token;
    dib_arguments[1] = 0U;
    dib_arguments[2] = 0U;
    dib_arguments[3] = 320U;
    dib_arguments[4] = 240U;
    dib_arguments[5] = 0U;
    dib_arguments[6] = 0U;
    dib_arguments[7] = 320U;
    dib_arguments[8] = 240U;
    dib_arguments[9] = config->cpu_pixel_token;
    dib_arguments[10] = config->dib_info_token;
    dib_arguments[11] = 0U;
    dib_arguments[12] = 0x00CC0020U;
    status = emit_service(config, trace,
                          COMPOSITION_SERVICE_STRETCH_DIBITS, &event,
                          dib_arguments, 13U);
    if (status != COMPOSITION_TRACE_OK)
        return status;
    init_event(&event, trace, COMPOSITION_EVENT_OUTPUT_FLUSH,
               COMPOSITION_ORIGIN_CONTROLLED_ENDPOINT,
               COMPOSITION_DOMAIN_DISPLAY, 0, 0);
    event.argument[0] = 0x66ACU;
    status = emit_service(config, trace, COMPOSITION_SERVICE_GDI_FLUSH,
                          &event, 0, 0U);
    if (status != COMPOSITION_TRACE_OK)
        return status;

    init_event(&event, trace, COMPOSITION_EVENT_CALLBACK_ENTRY,
               COMPOSITION_ORIGIN_STATIC_ORDER,
               COMPOSITION_DOMAIN_EXTERNAL, 0xBD80U,
               config->callback_endpoint);
    event.argument[0] = 0xB2E7U;
    status = emit_event(trace, &event);
    if (status != COMPOSITION_TRACE_OK)
        return status;
    init_event(&event, trace, COMPOSITION_EVENT_CALLBACK_SERVICE,
               COMPOSITION_ORIGIN_CONTROLLED_ENDPOINT,
               COMPOSITION_DOMAIN_EXTERNAL, 0,
               config->callback_endpoint);
    event.argument[0] = 0xB2E7U;
    {
        renderer_u32 callback_arguments[4];
        callback_arguments[0] = 0x40B2E7U;
        callback_arguments[1] = 0U;
        callback_arguments[2] = 0U;
        callback_arguments[3] = 0U;
        return emit_service(config, trace, COMPOSITION_SERVICE_CALLBACK,
                            &event, callback_arguments, 4U);
    }
}
