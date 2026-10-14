#include <stdio.h>
#include <string.h>

#include "hercules_rectangle.h"

#define RECTANGLE_TEXTURE_ADDRESS 0x0096E6E0UL
#define RECTANGLE_TEXTURE_LENGTH 0x10000UL
#define RECTANGLE_SURFACE_WIDTH 320UL
#define RECTANGLE_SURFACE_HEIGHT 240UL
#define RECTANGLE_SURFACE_LENGTH 76800UL

typedef struct ObserverState {
    renderer_u32 count;
    renderer_u32 offsets[64];
    renderer_u8 texels[64];
} ObserverState;

typedef struct PixelCallbackTrace {
    char events[32];
    renderer_u32 event_count;
    renderer_u32 shade_offset;
    renderer_u32 blend_offset;
    renderer_u32 observer_count;
    renderer_u32 observer_offset;
    renderer_u8 observer_source;
    renderer_u8 observer_value;
    renderer_u8 *surface_bytes;
} PixelCallbackTrace;

typedef struct TableProbe {
    PixelCallbackTrace *trace;
    char event;
    renderer_u8 value;
} TableProbe;

static void write_u16(renderer_u8 *bytes, renderer_u32 offset,
                      renderer_u16 value)
{
    bytes[offset] = (renderer_u8)value;
    bytes[offset + 1UL] = (renderer_u8)(value >> 8);
}

static void make_ft4(renderer_u8 wire[0x28], renderer_i16 x,
                     renderer_i16 y, renderer_u16 width,
                     renderer_u16 height, renderer_u8 u, renderer_u8 v,
                     int asymmetric_uv)
{
    memset(wire, 0, 0x28U);
    wire[4] = 0x64U;
    wire[5] = 0x75U;
    wire[6] = 0x86U;
    wire[7] = 0x2CU;
    write_u16(wire, 0x08U, (renderer_u16)x);
    write_u16(wire, 0x0AU, (renderer_u16)y);
    wire[0x0CU] = u;
    wire[0x0DU] = v;
    write_u16(wire, 0x0EU, 0U);
    write_u16(wire, 0x10U, (renderer_u16)(x + (renderer_i16)width));
    write_u16(wire, 0x12U, (renderer_u16)y);
    wire[0x14U] = (renderer_u8)(u + (renderer_u8)width);
    wire[0x15U] = v;
    write_u16(wire, 0x16U, 0U);
    write_u16(wire, 0x18U, (renderer_u16)x);
    write_u16(wire, 0x1AU, (renderer_u16)(y + (renderer_i16)height));
    wire[0x1CU] = (renderer_u8)(u + (asymmetric_uv ? 1U : 0U));
    wire[0x1DU] = (renderer_u8)(v + (renderer_u8)height);
    write_u16(wire, 0x1EU, 0U);
    write_u16(wire, 0x20U,
              (renderer_u16)(x + (renderer_i16)width));
    write_u16(wire, 0x22U,
              (renderer_u16)(y + (renderer_i16)height));
    wire[0x24U] = (renderer_u8)(u + (renderer_u8)width);
    wire[0x25U] = (renderer_u8)(v + (renderer_u8)height);
    write_u16(wire, 0x26U, 0U);
}

static void make_blended_ft4(renderer_u8 wire[0x28], renderer_i16 x,
                             renderer_i16 y, renderer_u16 width,
                             renderer_u16 height, renderer_u8 u,
                             renderer_u8 v, renderer_u8 red,
                             renderer_u16 tpage)
{
    make_ft4(wire, x, y, width, height, u, v, 0);
    wire[4] = red;
    wire[7] = 0x2EU;
    write_u16(wire, 0x16U, tpage);
}

static void trace_event(PixelCallbackTrace *trace, char event)
{
    if (trace != 0 && trace->event_count <
        (renderer_u32)sizeof(trace->events))
        trace->events[trace->event_count++] = event;
}

static int read_table_probe(void *opaque, renderer_u32 offset,
                            renderer_u8 *value)
{
    TableProbe *probe = (TableProbe *)opaque;
    if (probe == 0 || probe->trace == 0 || value == 0)
        return 0;
    trace_event(probe->trace, probe->event);
    if (probe->event == 'S')
        probe->trace->shade_offset = offset;
    else
        probe->trace->blend_offset = offset;
    *value = probe->value;
    return 1;
}

static int observe_traced_pixel(void *opaque, renderer_u8 source_texel,
                                renderer_u32 destination_offset)
{
    PixelCallbackTrace *trace = (PixelCallbackTrace *)opaque;
    if (trace == 0 || trace->surface_bytes == 0 ||
        destination_offset >= RECTANGLE_SURFACE_LENGTH)
        return 0;
    trace_event(trace, 'O');
    ++trace->observer_count;
    trace->observer_offset = destination_offset;
    trace->observer_source = source_texel;
    trace->observer_value = trace->surface_bytes[destination_offset];
    return 1;
}

static int observe_pixel(void *opaque, renderer_u8 source_texel,
                         renderer_u32 destination_offset)
{
    ObserverState *state = (ObserverState *)opaque;
    if (state == 0 || state->count >= 64UL)
        return 0;
    state->offsets[state->count] = destination_offset;
    state->texels[state->count] = source_texel;
    ++state->count;
    return 1;
}

static HerculesRectangleStatus prepare(
    const renderer_u8 *wire, size_t wire_size,
    RendererAc60StateView *state,
    const HerculesRectangleTextureView *texture,
    const HerculesRectangleSurfaceView *surface,
    RendererDecodedPacket *decoded, HerculesRectanglePlan *plan)
{
    RendererPacketStatus packet_status;
    RendererAc60Event event;

    packet_status = renderer_decode_packet(wire, wire_size, decoded);
    if (packet_status != RENDERER_PACKET_OK)
        return HERCULES_RECTANGLE_TRUNCATED;
    if (renderer_ac60_apply_packet(wire, wire_size, state, &event) !=
        RENDERER_AC60_APPLY_OK)
        return HERCULES_RECTANGLE_UNSUPPORTED;
    return hercules_rectangle_resolve(decoded, wire_size, state, texture,
                                      surface, plan);
}

static int test_rectangle_render(void)
{
    renderer_u8 wire[0x28];
    renderer_u8 texture_bytes[RECTANGLE_TEXTURE_LENGTH];
    renderer_u8 surface_bytes[RECTANGLE_SURFACE_LENGTH];
    RendererAc60StateView state;
    RendererDecodedPacket decoded;
    HerculesRectangleTextureView texture;
    HerculesRectangleSurfaceView surface;
    HerculesRectanglePlan plan;
    HerculesRectangleEffects effects;
    ObserverState observer;
    HerculesRectangleStatus status;

    memset(&state, 0, sizeof(state));
    memset(&observer, 0, sizeof(observer));
    memset(texture_bytes, 0, sizeof(texture_bytes));
    memset(surface_bytes, 0xA5, sizeof(surface_bytes));
    texture_bytes[0x102U] = 0x11U;
    texture_bytes[0x103U] = 0x22U;
    texture_bytes[0x104U] = 0x33U;
    texture_bytes[0x202U] = 0x44U;
    texture_bytes[0x203U] = 0x55U;
    texture_bytes[0x204U] = 0x66U;
    texture.page_address = RECTANGLE_TEXTURE_ADDRESS;
    texture.bytes = texture_bytes;
    texture.length = RECTANGLE_TEXTURE_LENGTH;
    texture.width = 256UL;
    texture.height = 256UL;
    texture.row_pitch = 256UL;
    surface.bytes = surface_bytes;
    surface.length = RECTANGLE_SURFACE_LENGTH;
    surface.width = RECTANGLE_SURFACE_WIDTH;
    surface.height = RECTANGLE_SURFACE_HEIGHT;
    surface.row_pitch = RECTANGLE_SURFACE_WIDTH;
    make_ft4(wire, 4, 5, 3U, 2U, 2U, 1U, 0);
    status = prepare(wire, sizeof(wire), &state, &texture, &surface,
                     &decoded, &plan);
    if (status != HERCULES_RECTANGLE_OK || plan.page_address != texture.page_address ||
        plan.x != 4UL || plan.y != 5UL || plan.width != 3UL ||
        plan.height != 2UL || plan.u != 2UL || plan.v != 1UL ||
        plan.source_start != 0x102UL || plan.source_pitch != 256UL ||
        plan.destination_start != 1604UL ||
        plan.destination_pitch != 320UL ||
        plan.pixel_mode != HERCULES_RECTANGLE_COPY ||
        plan.blend_selector != 0UL || plan.shade_row != 0UL)
        return 0;
    status = hercules_rectangle_render(&plan, &texture, &surface,
                                      observe_pixel, &observer, &effects);
    if (status != HERCULES_RECTANGLE_OK || effects.accepted != 1U ||
        effects.presentation_requested != 1U ||
        effects.pixel_store_count != 6UL || observer.count != 6UL)
        return 0;
    if (surface_bytes[1604U] != 0x11U || surface_bytes[1605U] != 0x22U ||
        surface_bytes[1606U] != 0x33U || surface_bytes[1924U] != 0x44U ||
        surface_bytes[1925U] != 0x55U || surface_bytes[1926U] != 0x66U ||
        observer.offsets[0] != 1604UL || observer.offsets[5] != 1926UL ||
        observer.texels[0] != 0x11U || observer.texels[5] != 0x66U)
        return 0;
    return 1;
}

static int test_transparent_rectangle_requests_presentation(void)
{
    renderer_u8 wire[0x28];
    renderer_u8 texture_bytes[RECTANGLE_TEXTURE_LENGTH];
    renderer_u8 surface_bytes[RECTANGLE_SURFACE_LENGTH];
    RendererAc60StateView state;
    RendererDecodedPacket decoded;
    HerculesRectangleTextureView texture;
    HerculesRectangleSurfaceView surface;
    HerculesRectanglePlan plan;
    HerculesRectangleEffects effects;
    HerculesRectangleStatus status;

    memset(&state, 0, sizeof(state));
    memset(texture_bytes, 0, sizeof(texture_bytes));
    memset(surface_bytes, 0xA5, sizeof(surface_bytes));
    texture.page_address = RECTANGLE_TEXTURE_ADDRESS;
    texture.bytes = texture_bytes;
    texture.length = RECTANGLE_TEXTURE_LENGTH;
    texture.width = 256UL;
    texture.height = 256UL;
    texture.row_pitch = 256UL;
    surface.bytes = surface_bytes;
    surface.length = RECTANGLE_SURFACE_LENGTH;
    surface.width = RECTANGLE_SURFACE_WIDTH;
    surface.height = RECTANGLE_SURFACE_HEIGHT;
    surface.row_pitch = RECTANGLE_SURFACE_WIDTH;
    make_ft4(wire, 1, 1, 2U, 2U, 0U, 0U, 0);
    status = prepare(wire, sizeof(wire), &state, &texture, &surface,
                     &decoded, &plan);
    if (status != HERCULES_RECTANGLE_OK)
        return 0;
    status = hercules_rectangle_render(&plan, &texture, &surface, 0, 0,
                                      &effects);
    return status == HERCULES_RECTANGLE_OK && effects.accepted == 1U &&
           effects.presentation_requested == 1U &&
           effects.pixel_store_count == 0UL &&
           surface_bytes[321U] == 0xA5U && surface_bytes[642U] == 0xA5U;
}

static int test_statuses_and_metadata_only_resolution(void)
{
    renderer_u8 wire[0x28];
    RendererAc60StateView state;
    RendererDecodedPacket decoded;
    RendererAc60Event event;
    HerculesRectangleTextureView texture;
    HerculesRectangleSurfaceView surface;
    HerculesRectanglePlan plan;
    HerculesRectangleStatus status;

    memset(&state, 0, sizeof(state));
    texture.page_address = RECTANGLE_TEXTURE_ADDRESS;
    texture.bytes = 0;
    texture.length = RECTANGLE_TEXTURE_LENGTH;
    texture.width = 256UL;
    texture.height = 256UL;
    texture.row_pitch = 256UL;
    surface.bytes = 0;
    surface.length = RECTANGLE_SURFACE_LENGTH;
    surface.width = RECTANGLE_SURFACE_WIDTH;
    surface.height = RECTANGLE_SURFACE_HEIGHT;
    surface.row_pitch = RECTANGLE_SURFACE_WIDTH;
    make_ft4(wire, 1, 1, 2U, 2U, 0U, 0U, 0);
    if (renderer_decode_packet(wire, sizeof(wire), &decoded) !=
        RENDERER_PACKET_OK || renderer_ac60_apply_packet(
            wire, sizeof(wire), &state, &event) != RENDERER_AC60_APPLY_OK)
        return 0;
    status = hercules_rectangle_resolve(&decoded, 0x25U, &state, &texture,
                                        &surface, &plan);
    if (status != HERCULES_RECTANGLE_TRUNCATED)
        return 0;
    status = hercules_rectangle_resolve(&decoded, sizeof(wire), &state,
                                        &texture, &surface, &plan);
    if (status != HERCULES_RECTANGLE_OK || plan.source_start != 0UL ||
        plan.destination_start != 321UL)
        return 0;
    texture.length = 1UL;
    status = hercules_rectangle_resolve(&decoded, sizeof(wire), &state,
                                        &texture, &surface, &plan);
    if (status != HERCULES_RECTANGLE_RANGE_ERROR)
        return 0;
    texture.length = RECTANGLE_TEXTURE_LENGTH;
    make_ft4(wire, 4, 5, 3U, 2U, 2U, 1U, 0);
    if (renderer_decode_packet(wire, sizeof(wire), &decoded) !=
        RENDERER_PACKET_OK || renderer_ac60_apply_packet(
            wire, sizeof(wire), &state, &event) != RENDERER_AC60_APPLY_OK)
        return 0;
    texture.length = 300UL;
    status = hercules_rectangle_resolve(&decoded, sizeof(wire), &state,
                                        &texture, &surface, &plan);
    if (status != HERCULES_RECTANGLE_RANGE_ERROR)
        return 0;
    texture.length = RECTANGLE_TEXTURE_LENGTH;
    surface.length = 1000UL;
    status = hercules_rectangle_resolve(&decoded, sizeof(wire), &state,
                                        &texture, &surface, &plan);
    if (status != HERCULES_RECTANGLE_RANGE_ERROR)
        return 0;
    surface.length = RECTANGLE_SURFACE_LENGTH;
    make_ft4(wire, 1, 1, 2U, 2U, 0U, 0U, 1);
    if (renderer_decode_packet(wire, sizeof(wire), &decoded) !=
        RENDERER_PACKET_OK || renderer_ac60_apply_packet(
            wire, sizeof(wire), &state, &event) != RENDERER_AC60_APPLY_OK)
        return 0;
    status = hercules_rectangle_resolve(&decoded, sizeof(wire), &state,
                                        &texture, &surface, &plan);
    return status == HERCULES_RECTANGLE_UNSUPPORTED;
}

static int test_blended_rectangle_resolution(void)
{
    static const renderer_u16 tpages[4] = {
        0x0000U, 0x0020U, 0x0040U, 0x0060U
    };
    renderer_u8 wire[0x28];
    RendererAc60StateView state;
    RendererDecodedPacket decoded;
    RendererAc60Event event;
    HerculesRectangleTextureView texture;
    HerculesRectangleSurfaceView surface;
    HerculesRectanglePlan plan;
    HerculesRectangleStatus status;
    renderer_u32 selector;

    memset(&state, 0, sizeof(state));
    texture.page_address = RECTANGLE_TEXTURE_ADDRESS;
    texture.bytes = 0;
    texture.length = RECTANGLE_TEXTURE_LENGTH;
    texture.width = 256UL;
    texture.height = 256UL;
    texture.row_pitch = 256UL;
    surface.bytes = 0;
    surface.length = RECTANGLE_SURFACE_LENGTH;
    surface.width = RECTANGLE_SURFACE_WIDTH;
    surface.height = RECTANGLE_SURFACE_HEIGHT;
    surface.row_pitch = RECTANGLE_SURFACE_WIDTH;

    for (selector = 0UL; selector < 4UL; ++selector) {
        memset(&state, 0, sizeof(state));
        make_blended_ft4(wire, 4, 5, 2U, 1U, 2U, 1U, 0x23U,
                         tpages[selector]);
        status = prepare(wire, sizeof(wire), &state, &texture, &surface,
                         &decoded, &plan);
        if (status != HERCULES_RECTANGLE_OK ||
            plan.pixel_mode != HERCULES_RECTANGLE_SHADED_BLEND ||
            plan.blend_selector != selector || plan.shade_row != 0x23UL)
            return 0;

        memset(&state, 0, sizeof(state));
        make_blended_ft4(wire, 4, 5, 2U, 1U, 2U, 1U, 0x80U,
                         tpages[selector]);
        status = prepare(wire, sizeof(wire), &state, &texture, &surface,
                         &decoded, &plan);
        if (status != HERCULES_RECTANGLE_OK ||
            plan.pixel_mode != HERCULES_RECTANGLE_RAW_BLEND ||
            plan.blend_selector != selector || plan.shade_row != 0UL)
            return 0;
    }

    /* The staged page selector must agree with tpage, while mode selection
     * comes from the staged red byte. Internal DWORD 0x180 is not wire red. */
    memset(&state, 0, sizeof(state));
    make_blended_ft4(wire, 4, 5, 2U, 1U, 2U, 1U, 0x80U, 0x0020U);
    if (renderer_decode_packet(wire, sizeof(wire), &decoded) !=
        RENDERER_PACKET_OK || renderer_ac60_apply_packet(
            wire, sizeof(wire), &state, &event) != RENDERER_AC60_APPLY_OK)
        return 0;
    state.page.page_mode = 0UL;
    status = hercules_rectangle_resolve(&decoded, sizeof(wire), &state,
                                        &texture, &surface, &plan);
    if (status != HERCULES_RECTANGLE_UNSUPPORTED)
        return 0;
    state.page.page_mode = 1UL;
    state.workspace.color0_red = 0x180UL;
    status = hercules_rectangle_resolve(&decoded, sizeof(wire), &state,
                                        &texture, &surface, &plan);
    return status == HERCULES_RECTANGLE_UNSUPPORTED;
}

static int test_shaded_blend_render_zero_and_lookup_order(void)
{
    renderer_u8 wire[0x28];
    renderer_u8 texture_bytes[RECTANGLE_TEXTURE_LENGTH];
    renderer_u8 surface_bytes[RECTANGLE_SURFACE_LENGTH];
    RendererAc60StateView state;
    RendererDecodedPacket decoded;
    HerculesRectangleTextureView texture;
    HerculesRectangleSurfaceView surface;
    HerculesRectanglePlan plan;
    HerculesRectangleEffects effects;
    HerculesRectangleStatus status;
    HercPixelTableReader shade_row;
    HercPixelTableReader blend_tables[HERC_PIXEL_BLEND_SELECTORS];
    TableProbe shade_probe;
    TableProbe blend_probes[HERC_PIXEL_BLEND_SELECTORS];
    PixelCallbackTrace trace;
    renderer_u32 selector;

    memset(&state, 0, sizeof(state));
    memset(texture_bytes, 0, sizeof(texture_bytes));
    memset(surface_bytes, 0xA5, sizeof(surface_bytes));
    memset(&trace, 0, sizeof(trace));
    texture.page_address = RECTANGLE_TEXTURE_ADDRESS;
    texture.bytes = texture_bytes;
    texture.length = RECTANGLE_TEXTURE_LENGTH;
    texture.width = 256UL;
    texture.height = 256UL;
    texture.row_pitch = 256UL;
    surface.bytes = surface_bytes;
    surface.length = RECTANGLE_SURFACE_LENGTH;
    surface.width = RECTANGLE_SURFACE_WIDTH;
    surface.height = RECTANGLE_SURFACE_HEIGHT;
    surface.row_pitch = RECTANGLE_SURFACE_WIDTH;
    trace.surface_bytes = surface_bytes;

    make_blended_ft4(wire, 4, 5, 2U, 1U, 2U, 1U, 0x7FU, 0x0040U);
    status = prepare(wire, sizeof(wire), &state, &texture, &surface,
                     &decoded, &plan);
    if (status != HERCULES_RECTANGLE_OK ||
        plan.pixel_mode != HERCULES_RECTANGLE_SHADED_BLEND ||
        plan.shade_row != 0x7FUL || plan.blend_selector != 2UL)
        return 0;

    /* The first zero texel must not touch any callback. The second maps to
     * zero but still blends with its old destination byte. */
    texture_bytes[0x102U] = 0U;
    texture_bytes[0x103U] = 7U;
    surface_bytes[1604U] = 0x55U;
    surface_bytes[1605U] = 0x3CU;
    shade_probe.trace = &trace;
    shade_probe.event = 'S';
    shade_probe.value = 0U;
    shade_row.length = HERC_PIXEL_COLOR_ROW_BYTES;
    shade_row.read_byte = read_table_probe;
    shade_row.context = &shade_probe;
    for (selector = 0UL; selector < HERC_PIXEL_BLEND_SELECTORS; ++selector) {
        blend_probes[selector].trace = &trace;
        blend_probes[selector].event = 'B';
        blend_probes[selector].value = 0U;
        blend_tables[selector].length = HERC_PIXEL_BLEND_BYTES;
        blend_tables[selector].read_byte = read_table_probe;
        blend_tables[selector].context = &blend_probes[selector];
    }

    status = hercules_rectangle_render_tables(
        &plan, &texture, &surface, &shade_row, blend_tables,
        HERC_PIXEL_BLEND_SELECTORS, observe_traced_pixel, &trace, &effects);
    if (status != HERCULES_RECTANGLE_OK || effects.accepted != 1U ||
        effects.presentation_requested != 1U ||
        effects.pixel_store_count != 1UL || trace.event_count != 3UL ||
        trace.events[0] != 'S' || trace.events[1] != 'B' ||
        trace.events[2] != 'O' || trace.shade_offset != 7UL ||
        trace.blend_offset != 0x003CUL || trace.observer_count != 1UL ||
        trace.observer_offset != 1605UL || trace.observer_source != 7U ||
        trace.observer_value != 0U || surface_bytes[1604U] != 0x55U ||
        surface_bytes[1605U] != 0U)
        return 0;

    /* Table adapters may be absent for a fully transparent blended rectangle;
     * geometry and plan metadata are still checked. */
    memset(texture_bytes + 0x102U, 0, 2U);
    memset(&trace, 0, sizeof(trace));
    trace.surface_bytes = surface_bytes;
    status = hercules_rectangle_render_tables(
        &plan, &texture, &surface, 0, 0, 0UL, observe_traced_pixel, &trace,
        &effects);
    if (status != HERCULES_RECTANGLE_OK || effects.accepted != 1U ||
        effects.presentation_requested != 1U ||
        effects.pixel_store_count != 0UL || trace.event_count != 0UL ||
        trace.observer_count != 0UL)
        return 0;

    plan.blend_selector = HERC_PIXEL_BLEND_SELECTORS;
    status = hercules_rectangle_render_tables(
        &plan, &texture, &surface, 0, 0, 0UL, 0, 0, &effects);
    if (status != HERCULES_RECTANGLE_UNSUPPORTED)
        return 0;
    plan.blend_selector = 2UL;
    plan.shade_row = 0x100UL;
    status = hercules_rectangle_render_tables(
        &plan, &texture, &surface, 0, 0, 0UL, 0, 0, &effects);
    if (status != HERCULES_RECTANGLE_UNSUPPORTED)
        return 0;
    plan.shade_row = 0x80UL;
    status = hercules_rectangle_render_tables(
        &plan, &texture, &surface, 0, 0, 0UL, 0, 0, &effects);
    if (status != HERCULES_RECTANGLE_UNSUPPORTED)
        return 0;
    plan.shade_row = 0x7FUL;
    return hercules_rectangle_render(&plan, &texture, &surface, 0, 0,
                                     &effects) ==
           HERCULES_RECTANGLE_UNSUPPORTED;
}

static int test_raw_blend_render(void)
{
    renderer_u8 wire[0x28];
    renderer_u8 texture_bytes[RECTANGLE_TEXTURE_LENGTH];
    renderer_u8 surface_bytes[RECTANGLE_SURFACE_LENGTH];
    RendererAc60StateView state;
    RendererDecodedPacket decoded;
    HerculesRectangleTextureView texture;
    HerculesRectangleSurfaceView surface;
    HerculesRectanglePlan plan;
    HerculesRectangleEffects effects;
    HerculesRectangleStatus status;
    HercPixelTableReader blend_tables[HERC_PIXEL_BLEND_SELECTORS];
    TableProbe blend_probes[HERC_PIXEL_BLEND_SELECTORS];
    PixelCallbackTrace trace;
    renderer_u32 selector;

    memset(&state, 0, sizeof(state));
    memset(texture_bytes, 0, sizeof(texture_bytes));
    memset(surface_bytes, 0xA5, sizeof(surface_bytes));
    memset(&trace, 0, sizeof(trace));
    texture.page_address = RECTANGLE_TEXTURE_ADDRESS;
    texture.bytes = texture_bytes;
    texture.length = RECTANGLE_TEXTURE_LENGTH;
    texture.width = 256UL;
    texture.height = 256UL;
    texture.row_pitch = 256UL;
    surface.bytes = surface_bytes;
    surface.length = RECTANGLE_SURFACE_LENGTH;
    surface.width = RECTANGLE_SURFACE_WIDTH;
    surface.height = RECTANGLE_SURFACE_HEIGHT;
    surface.row_pitch = RECTANGLE_SURFACE_WIDTH;
    trace.surface_bytes = surface_bytes;
    make_blended_ft4(wire, 4, 5, 1U, 1U, 2U, 1U, 0x80U, 0x0060U);
    status = prepare(wire, sizeof(wire), &state, &texture, &surface,
                     &decoded, &plan);
    if (status != HERCULES_RECTANGLE_OK ||
        plan.pixel_mode != HERCULES_RECTANGLE_RAW_BLEND ||
        plan.blend_selector != 3UL || plan.shade_row != 0UL)
        return 0;

    texture_bytes[0x102U] = 7U;
    surface_bytes[1604U] = 0x3CU;
    for (selector = 0UL; selector < HERC_PIXEL_BLEND_SELECTORS; ++selector) {
        blend_probes[selector].trace = &trace;
        blend_probes[selector].event = 'B';
        blend_probes[selector].value = (renderer_u8)(0xD0U + selector);
        blend_tables[selector].length = HERC_PIXEL_BLEND_BYTES;
        blend_tables[selector].read_byte = read_table_probe;
        blend_tables[selector].context = &blend_probes[selector];
    }
    status = hercules_rectangle_render_tables(
        &plan, &texture, &surface, 0, blend_tables,
        HERC_PIXEL_BLEND_SELECTORS, observe_traced_pixel, &trace, &effects);
    if (status != HERCULES_RECTANGLE_OK || effects.pixel_store_count != 1UL ||
        trace.event_count != 2UL || trace.events[0] != 'B' ||
        trace.events[1] != 'O' || trace.blend_offset != 0x073CUL ||
        surface_bytes[1604U] != 0xD3U || trace.observer_value != 0xD3U)
        return 0;

    plan.shade_row = 1UL;
    status = hercules_rectangle_render_tables(
        &plan, &texture, &surface, 0, blend_tables,
        HERC_PIXEL_BLEND_SELECTORS, 0, 0, &effects);
    return status == HERCULES_RECTANGLE_UNSUPPORTED;
}

int main(void)
{
    if (!test_rectangle_render()) {
        fprintf(stderr, "rectangle direct-copy plan/render failed\n");
        return 1;
    }
    if (!test_transparent_rectangle_requests_presentation()) {
        fprintf(stderr, "transparent rectangle effects failed\n");
        return 1;
    }
    if (!test_statuses_and_metadata_only_resolution()) {
        fprintf(stderr, "rectangle status/range handling failed\n");
        return 1;
    }
    if (!test_blended_rectangle_resolution()) {
        fprintf(stderr, "rectangle blend mode resolution failed\n");
        return 1;
    }
    if (!test_shaded_blend_render_zero_and_lookup_order()) {
        fprintf(stderr, "rectangle shaded blend/zero callback behavior failed\n");
        return 1;
    }
    if (!test_raw_blend_render()) {
        fprintf(stderr, "rectangle raw blend render failed\n");
        return 1;
    }
    puts("rectangle contract: copy, raw/shaded blend, transparent skip, and bounded metadata pass");
    return 0;
}
