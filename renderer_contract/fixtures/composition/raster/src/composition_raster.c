#include "composition_raster.h"

#include <string.h>

#include "hercules_rectangle.h"

#define RASTER_TEXTURE_BASE 0x0096E6E0UL
#define RASTER_CPU_SURFACE_TOKEN 0x34000000UL
#define RASTER_DIB_TOKEN 0x35000000UL
#define RASTER_HDC_TOKEN 0x36000000UL
#define RASTER_PITCH 320UL
#define RASTER_WIDTH 320UL
#define RASTER_HEIGHT 240UL
#define RASTER_REMAP_ARGUMENT 0xFFUL
#define RASTER_REMAP_SKIP_GUARD 0x80U
#define RASTER_TEXTURE_ROW_BYTES 256UL
#define RASTER_BASE_EVENT_COUNT 10UL

typedef struct RasterVisit {
    CompositionRasterConfig config;
    CompositionTrace *trace;
    renderer_u32 current_packet;
    renderer_u32 ft4_count;
    renderer_u32 terminator_count;
    CompositionRasterStatus status;
} RasterVisit;

static int read_packet(const CompositionRasterConfig *config,
                       renderer_raw32 address, renderer_u32 length,
                       const renderer_u8 **bytes)
{
    if (config == 0 || config->resolver == 0 || bytes == 0 ||
        address == 0 || length < 4U)
        return 0;
    if (!config->resolver(config->resolver_context, address, length, bytes))
        return 0;
    return *bytes != 0;
}

static renderer_u32 raw_u32(const renderer_u8 *bytes)
{
    return (renderer_u32)bytes[0] |
           ((renderer_u32)bytes[1] << 8) |
           ((renderer_u32)bytes[2] << 16) |
           ((renderer_u32)bytes[3] << 24);
}

static void make_event(CompositionEvent *event,
                       const CompositionTrace *trace,
                       CompositionEventKind kind,
                       composition_u8 origin, composition_u8 domain,
                       renderer_u32 source_rva, renderer_u32 raw_token)
{
    memset(event, 0, sizeof(*event));
    event->scope_id = 1U;
    event->local_order = trace->count;
    event->source_order = trace->count + 1U;
    event->source_rva = source_rva;
    event->raw_token = raw_token;
    event->kind = (composition_u16)kind;
    event->origin = origin;
    event->domain = domain;
    event->semantic_owner = COMPOSITION_OWNER_UNCLASSIFIED;
    event->flags = COMPOSITION_EVENT_FLAG_C_RECONSTRUCTION;
}

static CompositionRasterStatus append_event(CompositionTrace *trace,
                                            CompositionEvent *event)
{
    CompositionTraceStatus status;
    if (trace == 0 || event == 0)
        return COMPOSITION_RASTER_INVALID;
    event->local_order = trace->count;
    event->source_order = trace->count + 1U;
    status = composition_trace_append(trace, event);
    if (status == COMPOSITION_TRACE_OK)
        return COMPOSITION_RASTER_OK;
    if (status == COMPOSITION_TRACE_OVERFLOW)
        return COMPOSITION_RASTER_OVERFLOW;
    return COMPOSITION_RASTER_INVALID;
}

static int provider_call(const CompositionRasterConfig *config,
                         CompositionRasterService service,
                         const CompositionEvent *event,
                         const renderer_u32 *arguments,
                         renderer_u32 argument_count)
{
    CompositionRasterProviderCall call;
    renderer_u32 index;

    if (config == 0 || config->provider == 0 || event == 0 ||
        argument_count > 13U ||
        (argument_count != 0U && arguments == 0))
        return 0;
    memset(&call, 0, sizeof(call));
    call.service = service;
    call.event = *event;
    call.argument_count = argument_count;
    for (index = 0; index < argument_count; ++index)
        call.argument[index] = arguments[index];
    return config->provider(config->provider_context, &call) != 0;
}

static CompositionRasterStatus emit_service(
    const CompositionRasterConfig *config, CompositionTrace *trace,
    CompositionRasterService service, CompositionEvent *event,
    const renderer_u32 *arguments, renderer_u32 argument_count)
{
    if (!provider_call(config, service, event, arguments, argument_count))
        return COMPOSITION_RASTER_PROVIDER_FAILED;
    return append_event(trace, event);
}

static void make_rectangle_views(const CompositionRasterConfig *config,
                                 HerculesRectangleTextureView *texture,
                                 HerculesRectangleSurfaceView *surface)
{
    texture->page_address = config->texture_token;
    texture->bytes = config->texture_bytes;
    texture->length = config->texture_length;
    texture->width = RASTER_TEXTURE_ROW_BYTES;
    texture->height = RASTER_TEXTURE_ROW_BYTES;
    texture->row_pitch = RASTER_TEXTURE_ROW_BYTES;
    surface->bytes = config->surface_bytes;
    surface->length = config->surface_length;
    surface->width = RASTER_WIDTH;
    surface->height = RASTER_HEIGHT;
    surface->row_pitch = config->surface_pitch;
}

static int append_rectangle_pixel_event(void *opaque,
                                        renderer_u8 source_texel,
                                        renderer_u32 destination_offset)
{
    RasterVisit *visit = (RasterVisit *)opaque;
    CompositionEvent event;
    CompositionRasterStatus status;

    if (visit == 0 || visit->trace == 0)
        return 0;
    make_event(&event, visit->trace, COMPOSITION_EVENT_SCREEN_SPACE_GRAPHIC,
               COMPOSITION_ORIGIN_STATIC_ORDER, COMPOSITION_DOMAIN_CPU_SURFACE,
               0xCBFBU, visit->config.surface_token + destination_offset);
    event.argument[0] = destination_offset;
    event.argument[1] = source_texel;
    event.argument[2] = source_texel;
    status = append_event(visit->trace, &event);
    if (status != COMPOSITION_RASTER_OK) {
        visit->status = status;
        return 0;
    }
    return 1;
}

static int visit_packet(void *opaque, renderer_raw32 packet_address)
{
    RasterVisit *visit = (RasterVisit *)opaque;
    const renderer_u8 *wire;
    renderer_u32 tag;
    renderer_u32 next;
    renderer_u32 length;
    RendererDecodedPacket decoded;
    RendererAc60Event applied;
    RendererAc60ApplyStatus apply_status;
    CompositionEvent event;
    HerculesRectanglePlan rect;
    HerculesRectangleTextureView texture_view;
    HerculesRectangleSurfaceView surface_view;
    HerculesRectangleEffects effects;
    HerculesRectangleStatus rectangle_status;

    if (visit == 0 || visit->trace == 0 || visit->status != COMPOSITION_RASTER_OK)
        return 0;
    if (!read_packet(&visit->config, packet_address, 8U, &wire)) {
        visit->status = COMPOSITION_RASTER_FRAME_FAILED;
        return 0;
    }
    tag = wire[7];
    length = (tag == 0x2CU) ? 0x28U : 8U;
    if (!read_packet(&visit->config, packet_address, length, &wire)) {
        visit->status = COMPOSITION_RASTER_FRAME_FAILED;
        return 0;
    }
    next = raw_u32(wire);
    if (renderer_decode_packet(wire, (size_t)length, &decoded) != RENDERER_PACKET_OK &&
        tag != 0x40U) {
        visit->status = COMPOSITION_RASTER_UNSUPPORTED;
        return 0;
    }

    make_event(&event, visit->trace, COMPOSITION_EVENT_PACKET_CONSUMED,
               COMPOSITION_ORIGIN_STATIC_ORDER, COMPOSITION_DOMAIN_ORDER_TABLE,
               0xACB9U, packet_address);
    event.argument[0] = tag;
    event.argument[1] = next;
    if (append_event(visit->trace, &event) != COMPOSITION_RASTER_OK) {
        visit->status = COMPOSITION_RASTER_INVALID;
        return 0;
    }

    if (tag == 0x40U) {
        if (next != 0U || visit->ft4_count != 1U) {
            visit->status = COMPOSITION_RASTER_UNSUPPORTED;
            return 0;
        }
        visit->terminator_count++;
        visit->current_packet = packet_address;
        return 1;
    }
    if (tag != 0x2CU || visit->ft4_count != 0U) {
        visit->status = COMPOSITION_RASTER_UNSUPPORTED;
        return 0;
    }

    apply_status = renderer_ac60_apply_packet(
        wire, (size_t)length, visit->config.state, &applied);
    make_rectangle_views(&visit->config, &texture_view, &surface_view);
    rectangle_status = hercules_rectangle_resolve(
        &decoded, (size_t)length, visit->config.state, &texture_view,
        &surface_view, &rect);
    if (apply_status != RENDERER_AC60_APPLY_OK ||
        applied.kind != RENDERER_AC60_EVENT_FT4 ||
        rectangle_status != HERCULES_RECTANGLE_OK) {
        visit->status = COMPOSITION_RASTER_UNSUPPORTED;
        return 0;
    }

    make_event(&event, visit->trace, COMPOSITION_EVENT_GEOMETRY_COMMAND,
               COMPOSITION_ORIGIN_STATIC_ORDER, COMPOSITION_DOMAIN_GEOMETRY,
               0xD420U, packet_address);
    event.argument[0] = rect.x;
    event.argument[1] = rect.y;
    event.argument[2] = rect.width;
    event.argument[3] = rect.height;
    if (append_event(visit->trace, &event) != COMPOSITION_RASTER_OK) {
        visit->status = COMPOSITION_RASTER_INVALID;
        return 0;
    }

    rectangle_status = hercules_rectangle_render(
        &rect, &texture_view, &surface_view, append_rectangle_pixel_event,
        visit, &effects);
    if (rectangle_status != HERCULES_RECTANGLE_OK) {
        if (rectangle_status != HERCULES_RECTANGLE_OBSERVER_FAILED)
            visit->status = COMPOSITION_RASTER_UNSUPPORTED;
        return 0;
    }
    ++visit->ft4_count;
    visit->current_packet = packet_address;
    return 1;
}

static CompositionRasterStatus preflight(
    const CompositionRasterConfig *config, HerculesRectanglePlan *rect)
{
    const renderer_u8 *wire;
    renderer_u32 next;
    RendererDecodedPacket decoded;
    RendererAc60StateView state;
    RendererAc60Event applied;
    HerculesRectangleTextureView texture_view;
    HerculesRectangleSurfaceView surface_view;
    HerculesRectangleStatus rectangle_status;

    if (config == 0 || config->resolver == 0 || config->state == 0 ||
        config->order_table_base == 0 || config->order_table_slot_count != 1U ||
        config->provider == 0 || config->surface_bytes == 0 ||
        config->texture_bytes == 0 || config->surface_pitch != RASTER_PITCH ||
        config->surface_token != RASTER_CPU_SURFACE_TOKEN ||
        config->texture_token != RASTER_TEXTURE_BASE ||
        config->dib_info_token != RASTER_DIB_TOKEN ||
        config->hdc_token != RASTER_HDC_TOKEN || config->dirty_gate != 1U ||
        config->remap_guard != RASTER_REMAP_SKIP_GUARD ||
        config->state->raster_suppression_flag != 0U)
        return COMPOSITION_RASTER_INVALID;
    if (!read_packet(config, config->order_table_base, 4U, &wire))
        return COMPOSITION_RASTER_FRAME_FAILED;
    next = raw_u32(wire);
    if (!read_packet(config, next, 0x28U, &wire))
        return COMPOSITION_RASTER_FRAME_FAILED;
    if (renderer_decode_packet(wire, 0x28U, &decoded) != RENDERER_PACKET_OK ||
        decoded.kind != RENDERER_PACKET_FT4 || decoded.command != 0x2CU)
        return COMPOSITION_RASTER_UNSUPPORTED;
    state = *config->state;
    if (renderer_ac60_apply_packet(wire, 0x28U, &state, &applied) !=
            RENDERER_AC60_APPLY_OK || applied.kind != RENDERER_AC60_EVENT_FT4)
        return COMPOSITION_RASTER_UNSUPPORTED;
    make_rectangle_views(config, &texture_view, &surface_view);
    rectangle_status = hercules_rectangle_resolve(
        &decoded, 0x28U, &state, &texture_view, &surface_view, rect);
    if (rectangle_status != HERCULES_RECTANGLE_OK)
        return COMPOSITION_RASTER_UNSUPPORTED;
    next = raw_u32(wire);
    if (next == 0U || !read_packet(config, next, 8U, &wire) || wire[7] != 0x40U ||
        raw_u32(wire) != 0U)
        return COMPOSITION_RASTER_UNSUPPORTED;
    return COMPOSITION_RASTER_OK;
}


CompositionRasterStatus composition_run_ft4_direct_copy_frame(
    const CompositionRasterConfig *config, CompositionTrace *trace)
{
    CompositionRasterConfig run_config;
    RendererAc60StateView state_copy;
    RasterVisit visit;
    renderer_raw32 visited[4];
    FrameVisitGuard guard;
    FrameWalkStatus frame_status;
    CompositionEvent event;
    CompositionRasterStatus status;
    renderer_u32 args[13];
    HerculesRectanglePlan rect;
    renderer_u32 required_events;

    if (trace == 0 || config == 0)
        return COMPOSITION_RASTER_INVALID;
    composition_trace_init(trace, trace->events, trace->capacity);
    status = preflight(config, &rect);
    if (status != COMPOSITION_RASTER_OK)
        return status;
    required_events = RASTER_BASE_EVENT_COUNT + rect.width * rect.height;
    if (trace->events == 0 || trace->capacity < required_events) {
        return COMPOSITION_RASTER_OVERFLOW;
    }

    run_config = *config;
    state_copy = *config->state;
    run_config.state = &state_copy;
    memset(&visit, 0, sizeof(visit));
    visit.config = run_config;
    visit.trace = trace;
    visit.status = COMPOSITION_RASTER_OK;

    make_event(&event, trace, COMPOSITION_EVENT_FRAME_ENTRY,
               COMPOSITION_ORIGIN_STATIC_ORDER, COMPOSITION_DOMAIN_ORDER_TABLE,
               0xAC60U, config->order_table_base);
    if (append_event(trace, &event) != COMPOSITION_RASTER_OK)
        return COMPOSITION_RASTER_INVALID;

    make_event(&event, trace, COMPOSITION_EVENT_SURFACE_LOCK,
               COMPOSITION_ORIGIN_STATIC_ORDER, COMPOSITION_DOMAIN_CPU_SURFACE,
               0x6FF0U, config->surface_token);
    event.argument[0] = config->surface_pitch;
    event.argument[1] = RASTER_HEIGHT;
    event.argument[2] = config->surface_token;
    args[0] = config->surface_pitch;
    args[1] = RASTER_HEIGHT;
    args[2] = config->surface_token;
    status = emit_service(config, trace, COMPOSITION_RASTER_SERVICE_LOCK,
                          &event, args, 3U);
    if (status != COMPOSITION_RASTER_OK)
        return status;

    frame_visit_guard_init(&guard, visited, 4U);
    frame_status = frame_walk_ac60_order_table_visit(
        config->resolver, config->resolver_context, config->order_table_base,
        config->order_table_slot_count, &guard, visit_packet, &visit);
    if (frame_status != FRAME_WALK_OK || visit.status != COMPOSITION_RASTER_OK ||
        visit.ft4_count != 1U || visit.terminator_count != 1U)
        return visit.status != COMPOSITION_RASTER_OK
            ? visit.status : COMPOSITION_RASTER_FRAME_FAILED;

    make_event(&event, trace, COMPOSITION_EVENT_SURFACE_REMAP_REQUEST,
               COMPOSITION_ORIGIN_STATIC_ORDER, COMPOSITION_DOMAIN_CPU_SURFACE,
               0x70C0U, config->surface_token);
    event.argument[0] = RASTER_REMAP_ARGUMENT;
    event.argument[1] = config->remap_guard;
    args[0] = RASTER_REMAP_ARGUMENT;
    status = emit_service(config, trace, COMPOSITION_RASTER_SERVICE_REMAP,
                          &event, args, 1U);
    if (status != COMPOSITION_RASTER_OK)
        return status;

    make_event(&event, trace, COMPOSITION_EVENT_SURFACE_UNLOCK,
               COMPOSITION_ORIGIN_STATIC_ORDER, COMPOSITION_DOMAIN_CPU_SURFACE,
               0x7140U, config->surface_token);
    status = emit_service(config, trace, COMPOSITION_RASTER_SERVICE_UNLOCK,
                          &event, 0, 0U);
    if (status != COMPOSITION_RASTER_OK)
        return status;

    make_event(&event, trace, COMPOSITION_EVENT_PRESENT_REQUEST,
               COMPOSITION_ORIGIN_STATIC_ORDER, COMPOSITION_DOMAIN_DISPLAY,
               0x6620U, config->surface_token);
    event.argument[0] = config->dirty_gate;
    event.argument[1] = RASTER_WIDTH;
    event.argument[2] = RASTER_HEIGHT;
    args[0] = config->dirty_gate;
    status = emit_service(config, trace, COMPOSITION_RASTER_SERVICE_PRESENT,
                          &event, args, 1U);
    if (status != COMPOSITION_RASTER_OK)
        return status;

    make_event(&event, trace, COMPOSITION_EVENT_FINAL_OUTPUT,
               COMPOSITION_ORIGIN_CONTROLLED_ENDPOINT, COMPOSITION_DOMAIN_DISPLAY,
               0x66A6U, config->hdc_token);
    args[0] = config->hdc_token; args[1] = 0U; args[2] = 0U;
    args[3] = RASTER_WIDTH; args[4] = RASTER_HEIGHT;
    args[5] = 0U; args[6] = 0U; args[7] = RASTER_WIDTH; args[8] = RASTER_HEIGHT;
    args[9] = config->surface_token; args[10] = config->dib_info_token;
    args[11] = 0U; args[12] = 0x00CC0020UL;
    event.argument[0] = 0x66A6U;
    status = emit_service(config, trace, COMPOSITION_RASTER_SERVICE_STRETCH_DIBITS,
                          &event, args, 13U);
    if (status != COMPOSITION_RASTER_OK)
        return status;

    make_event(&event, trace, COMPOSITION_EVENT_OUTPUT_FLUSH,
               COMPOSITION_ORIGIN_CONTROLLED_ENDPOINT, COMPOSITION_DOMAIN_DISPLAY,
               0x66ACU, 0U);
    event.argument[0] = 0x66ACU;
    status = emit_service(config, trace, COMPOSITION_RASTER_SERVICE_GDI_FLUSH,
                          &event, 0, 0U);
    if (status != COMPOSITION_RASTER_OK)
        return status;
    return COMPOSITION_RASTER_OK;
}
