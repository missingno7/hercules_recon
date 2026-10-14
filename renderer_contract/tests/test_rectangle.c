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
        plan.destination_pitch != 320UL)
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
    puts("rectangle contract: plan, CAB0 stores, transparent presentation, and range statuses pass");
    return 0;
}
