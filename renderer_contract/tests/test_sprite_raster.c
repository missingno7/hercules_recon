#include <stdio.h>
#include <string.h>

#include "hercules_sprite_raster.h"
#include "hercules_textures.h"

#define PAGE_BYTES 65536UL
#define SURFACE_BYTES 76800UL

typedef struct Probe {
    renderer_u32 calls;
    renderer_u32 last_offset;
    renderer_u8 value;
} Probe;

typedef struct ObserverProbe {
    renderer_u8 *surface;
    renderer_u32 calls;
    renderer_u32 offset;
    renderer_u8 source;
    renderer_u8 stored;
} ObserverProbe;

static void put_u16(renderer_u8 *bytes, renderer_u32 offset,
                    renderer_u16 value)
{
    bytes[offset] = (renderer_u8)value;
    bytes[offset + 1UL] = (renderer_u8)(value >> 8);
}

static void put_u32(renderer_u8 *bytes, renderer_u32 offset,
                    renderer_u32 value)
{
    bytes[offset] = (renderer_u8)value;
    bytes[offset + 1UL] = (renderer_u8)(value >> 8);
    bytes[offset + 2UL] = (renderer_u8)(value >> 16);
    bytes[offset + 3UL] = (renderer_u8)(value >> 24);
}

static void make_e1(renderer_u8 wire[12], renderer_u32 page_index,
                    renderer_u32 selector)
{
    memset(wire, 0, 12U);
    wire[7] = 0xE1U;
    put_u32(wire, 4UL, 0xE1000000UL | (selector << 5) | page_index);
}

static void make_variable(renderer_u8 wire[0x14], renderer_u8 command,
                          renderer_i16 x, renderer_i16 y,
                          renderer_u8 u, renderer_u8 v,
                          renderer_u16 width, renderer_u16 height)
{
    memset(wire, 0, 0x14U);
    wire[7] = command;
    put_u16(wire, 8UL, (renderer_u16)x);
    put_u16(wire, 10UL, (renderer_u16)y);
    wire[12] = u;
    wire[13] = v;
    put_u16(wire, 16UL, width);
    put_u16(wire, 18UL, height);
}

static void make_fixed(renderer_u8 wire[0x0e], renderer_u8 command,
                       renderer_i16 x, renderer_i16 y,
                       renderer_u8 u, renderer_u8 v)
{
    memset(wire, 0, 0x0eU);
    wire[7] = command;
    put_u16(wire, 8UL, (renderer_u16)x);
    put_u16(wire, 10UL, (renderer_u16)y);
    wire[12] = u;
    wire[13] = v;
}

static void make_views(HerculesRectangleTextureView *texture,
                       HerculesRectangleSurfaceView *surface,
                       renderer_raw32 page_address,
                       const renderer_u8 *texture_bytes,
                       renderer_u8 *surface_bytes)
{
    texture->page_address = page_address;
    texture->bytes = texture_bytes;
    texture->length = PAGE_BYTES;
    texture->width = 256UL;
    texture->height = 256UL;
    texture->row_pitch = 256UL;
    surface->bytes = surface_bytes;
    surface->length = SURFACE_BYTES;
    surface->width = 320UL;
    surface->height = 240UL;
    surface->row_pitch = 320UL;
}

static HerculesSpriteStatus resolve_packet(
    const renderer_u8 *sprite_wire, size_t sprite_size,
    renderer_u32 page_index, renderer_u32 selector,
    RendererAc60StateView *state, RendererDecodedPacket *decoded,
    HerculesRectangleTextureView *texture,
    const HerculesRectangleSurfaceView *surface, HerculesSpritePlan *plan)
{
    renderer_u8 e1_wire[12];
    RendererAc60Event event;

    make_e1(e1_wire, page_index, selector);
    memset(state, 0, sizeof(*state));
    state->clip_bounds.bottom = 240;
    if (renderer_ac60_apply_packet(e1_wire, sizeof(e1_wire), state, &event) !=
        RENDERER_AC60_APPLY_OK)
        return HERCULES_SPRITE_UNSUPPORTED;
    if (renderer_decode_packet(sprite_wire, sprite_size, decoded) !=
        RENDERER_PACKET_OK)
        return HERCULES_SPRITE_TRUNCATED;
    if (renderer_ac60_apply_packet(sprite_wire, sprite_size, state, &event) !=
        RENDERER_AC60_APPLY_OK)
        return HERCULES_SPRITE_UNSUPPORTED;
    texture->page_address = state->workspace.texture_page_address;
    return hercules_sprite_resolve(decoded, sprite_size, state, texture,
                                   surface, plan);
}

static int read_probe(void *opaque, renderer_u32 offset, renderer_u8 *value)
{
    Probe *probe = (Probe *)opaque;
    if (probe == 0 || value == 0)
        return 0;
    ++probe->calls;
    probe->last_offset = offset;
    *value = probe->value;
    return 1;
}

static int observe_probe(void *opaque, renderer_u8 source,
                         renderer_u32 destination_offset)
{
    ObserverProbe *probe = (ObserverProbe *)opaque;
    if (probe == 0 || probe->surface == 0 ||
        destination_offset >= SURFACE_BYTES)
        return 0;
    ++probe->calls;
    probe->offset = destination_offset;
    probe->source = source;
    probe->stored = probe->surface[destination_offset];
    return 1;
}

static int test_title_strip_plans(void)
{
    renderer_u8 sprite[0x14];
    renderer_u8 texture_bytes[PAGE_BYTES];
    renderer_u8 surface_bytes[SURFACE_BYTES];
    RendererAc60StateView state;
    RendererDecodedPacket decoded;
    HerculesRectangleTextureView texture;
    HerculesRectangleSurfaceView surface;
    HerculesSpritePlan plan;
    renderer_u32 selector;

    memset(texture_bytes, 0, sizeof(texture_bytes));
    memset(surface_bytes, 0, sizeof(surface_bytes));
    make_views(&texture, &surface, HERC_TEX_MAP_BASE + 2UL * PAGE_BYTES,
               texture_bytes, surface_bytes);
    make_variable(sprite, 0x64U, 0, 0, 128U, 0U, 128U, 240U);
    if (resolve_packet(sprite, sizeof(sprite), 4UL, 0UL, &state, &decoded,
                       &texture, &surface, &plan) != HERCULES_SPRITE_OK ||
        plan.page_address != HERC_TEX_MAP_BASE + 2UL * PAGE_BYTES ||
        plan.pixel_mode != HERCULES_RECTANGLE_COPY || plan.blend_selector != 0UL ||
        plan.x != 0UL || plan.y != 0UL || plan.u != 128UL || plan.v != 0UL ||
        plan.width != 128UL || plan.height != 240UL ||
        plan.source_start != 128UL || plan.source_pitch != 256UL ||
        plan.destination_start != 0UL || plan.destination_pitch != 320UL)
        return 0;

    make_views(&texture, &surface, HERC_TEX_MAP_BASE + 3UL * PAGE_BYTES,
               texture_bytes, surface_bytes);
    for (selector = 0UL; selector < HERC_PIXEL_BLEND_SELECTORS; ++selector) {
        make_variable(sprite, 0x66U, 128, 0, 0U, 0U, 192U, 240U);
        if (resolve_packet(sprite, sizeof(sprite), 6UL, selector, &state,
                           &decoded, &texture, &surface, &plan) !=
                HERCULES_SPRITE_OK ||
            plan.page_address != HERC_TEX_MAP_BASE + 3UL * PAGE_BYTES ||
            plan.pixel_mode != HERCULES_RECTANGLE_RAW_BLEND ||
            plan.blend_selector != selector || plan.x != 128UL ||
            plan.y != 0UL || plan.u != 0UL || plan.v != 0UL ||
            plan.width != 192UL || plan.height != 240UL ||
            plan.source_start != 0UL || plan.destination_start != 128UL)
            return 0;
    }
    return 1;
}

static int test_fixed_and_clipped_geometry(void)
{
    static const renderer_u8 commands[4] = { 0x74U, 0x76U, 0x7cU, 0x7eU };
    renderer_u8 sprite[0x14];
    renderer_u8 texture_bytes[PAGE_BYTES];
    renderer_u8 surface_bytes[SURFACE_BYTES];
    RendererAc60StateView state;
    RendererDecodedPacket decoded;
    HerculesRectangleTextureView texture;
    HerculesRectangleSurfaceView surface;
    HerculesSpritePlan plan;
    renderer_u32 index;

    memset(texture_bytes, 0, sizeof(texture_bytes));
    memset(surface_bytes, 0, sizeof(surface_bytes));
    make_views(&texture, &surface, HERC_TEX_MAP_BASE + 2UL * PAGE_BYTES,
               texture_bytes, surface_bytes);
    for (index = 0UL; index < 4UL; ++index) {
        make_fixed(sprite, commands[index], 3, 4, 8U, 9U);
        if (resolve_packet(sprite, 0x0eU, 4UL, 2UL, &state, &decoded,
                           &texture, &surface, &plan) != HERCULES_SPRITE_OK ||
            plan.width != ((index < 2UL) ? 8UL : 16UL) ||
            plan.height != plan.width ||
            plan.pixel_mode != ((commands[index] & 2U) != 0U
                ? HERCULES_RECTANGLE_RAW_BLEND : HERCULES_RECTANGLE_COPY))
            return 0;
        if (plan.pixel_mode == HERCULES_RECTANGLE_RAW_BLEND &&
            plan.blend_selector != 2UL)
            return 0;
        if (plan.pixel_mode == HERCULES_RECTANGLE_COPY &&
            plan.blend_selector != 0UL)
            return 0;
    }

    make_variable(sprite, 0x64U, -3, -5, 4U, 5U, 7U, 8U);
    if (resolve_packet(sprite, sizeof(sprite), 4UL, 0UL, &state, &decoded,
                       &texture, &surface, &plan) != HERCULES_SPRITE_OK ||
        plan.x != 0UL || plan.y != 0UL || plan.width != 4UL ||
        plan.height != 3UL || plan.u != 7UL || plan.v != 10UL ||
        plan.source_start != 2567UL || plan.destination_start != 0UL)
        return 0;

    make_variable(sprite, 0x64U, 318, 238, 0U, 0U, 8U, 8U);
    if (resolve_packet(sprite, sizeof(sprite), 4UL, 0UL, &state, &decoded,
                       &texture, &surface, &plan) != HERCULES_SPRITE_OK ||
        plan.width != 2UL || plan.height != 2UL ||
        plan.destination_start != 318UL + 238UL * 320UL)
        return 0;
    return 1;
}

static int test_blend_pixel_and_zero_skip(void)
{
    renderer_u8 sprite[0x14];
    renderer_u8 texture_bytes[PAGE_BYTES];
    renderer_u8 surface_bytes[SURFACE_BYTES];
    RendererAc60StateView state;
    RendererDecodedPacket decoded;
    HerculesRectangleTextureView texture;
    HerculesRectangleSurfaceView surface;
    HerculesSpritePlan plan;
    HerculesRectangleEffects effects;
    HercPixelTableReader blend;
    Probe table_probe;
    ObserverProbe observer;
    HerculesSpriteStatus status;

    memset(texture_bytes, 0, sizeof(texture_bytes));
    memset(surface_bytes, 0x3c, sizeof(surface_bytes));
    make_views(&texture, &surface, HERC_TEX_MAP_BASE + 3UL * PAGE_BYTES,
               texture_bytes, surface_bytes);
    make_variable(sprite, 0x66U, 7, 8, 2U, 1U, 1U, 1U);
    status = resolve_packet(sprite, sizeof(sprite), 6UL, 3UL, &state,
                            &decoded, &texture, &surface, &plan);
    if (status != HERCULES_SPRITE_OK)
        return 0;

    table_probe.calls = 0UL;
    table_probe.last_offset = 0UL;
    table_probe.value = 0xD3U;
    blend.length = HERC_PIXEL_BLEND_BYTES;
    blend.read_byte = read_probe;
    blend.context = &table_probe;
    observer.surface = surface_bytes;
    observer.calls = 0UL;
    observer.offset = 0UL;
    observer.source = 0U;
    observer.stored = 0U;

    /* A zero source bypasses even a missing table adapter and requests the
     * frame presentation without touching the destination or observer. */
    status = hercules_sprite_render(&plan, &texture, &surface, 0,
                                    observe_probe, &observer, &effects);
    if (status != HERCULES_SPRITE_OK || effects.accepted != 1U ||
        effects.presentation_requested != 1U ||
        effects.pixel_store_count != 0UL || table_probe.calls != 0UL ||
        observer.calls != 0UL || surface_bytes[7UL + 8UL * 320UL] != 0x3CU)
        return 0;

    texture_bytes[258UL] = 7U;
    status = hercules_sprite_render(&plan, &texture, &surface, &blend,
                                    observe_probe, &observer, &effects);
    if (status != HERCULES_SPRITE_OK || effects.accepted != 1U ||
        effects.presentation_requested != 1U ||
        effects.pixel_store_count != 1UL || table_probe.calls != 1UL ||
        table_probe.last_offset != 0x073cUL || observer.calls != 1UL ||
        observer.offset != 7UL + 8UL * 320UL || observer.source != 7U ||
        observer.stored != 0xD3U || surface_bytes[observer.offset] != 0xD3U)
        return 0;

    plan.blend_selector = HERC_PIXEL_BLEND_SELECTORS;
    status = hercules_sprite_render(&plan, &texture, &surface, 0, 0, 0,
                                    &effects);
    if (status != HERCULES_SPRITE_UNSUPPORTED || effects.accepted != 0U ||
        effects.presentation_requested != 0U)
        return 0;
    plan.blend_selector = 3UL;
    texture.bytes = 0;
    status = hercules_sprite_render(&plan, &texture, &surface, &blend, 0, 0,
                                    &effects);
    if (status != HERCULES_SPRITE_INVALID_ARGUMENT)
        return 0;
    return 1;
}

static int test_rejections_and_no_draw(void)
{
    renderer_u8 sprite[0x14];
    renderer_u8 e1_wire[12];
    renderer_u8 texture_bytes[PAGE_BYTES];
    renderer_u8 surface_bytes[SURFACE_BYTES];
    RendererAc60StateView state;
    RendererDecodedPacket decoded;
    HerculesRectangleTextureView texture;
    HerculesRectangleSurfaceView surface;
    HerculesSpritePlan plan;
    RendererAc60Event event;

    memset(texture_bytes, 0, sizeof(texture_bytes));
    memset(surface_bytes, 0, sizeof(surface_bytes));
    make_views(&texture, &surface, HERC_TEX_MAP_BASE + 2UL * PAGE_BYTES,
               texture_bytes, surface_bytes);
    make_e1(e1_wire, 4UL, 0UL);
    make_variable(sprite, 0x64U, 1, 1, 0U, 0U, 2U, 2U);
    memset(&state, 0, sizeof(state));
    state.clip_bounds.bottom = 240;
    if (renderer_ac60_apply_packet(e1_wire, sizeof(e1_wire), &state, &event) !=
        RENDERER_AC60_APPLY_OK)
        return 0;
    if (renderer_decode_packet(sprite, sizeof(sprite), &decoded) !=
        RENDERER_PACKET_OK || renderer_ac60_apply_packet(
            sprite, sizeof(sprite), &state, &event) != RENDERER_AC60_APPLY_OK)
        return 0;
    if (hercules_sprite_resolve(&decoded, sizeof(sprite) - 1U, &state,
                                &texture, &surface, &plan) !=
        HERCULES_SPRITE_TRUNCATED)
        return 0;
    if (hercules_sprite_resolve(&decoded, sizeof(sprite), &state, &texture,
                                &surface, 0) != HERCULES_SPRITE_INVALID_ARGUMENT)
        return 0;

    state.raster_suppression_flag = 1U;
    if (hercules_sprite_resolve(&decoded, sizeof(sprite), &state, &texture,
                                &surface, &plan) != HERCULES_SPRITE_NO_DRAW)
        return 0;
    state.raster_suppression_flag = 0U;
    make_variable(sprite, 0x64U, 1, 1, 0U, 0U, 0xffffU, 2U);
    if (renderer_decode_packet(sprite, sizeof(sprite), &decoded) !=
        RENDERER_PACKET_OK || renderer_ac60_apply_packet(
            sprite, sizeof(sprite), &state, &event) != RENDERER_AC60_APPLY_OK ||
        hercules_sprite_resolve(&decoded, sizeof(sprite), &state, &texture,
                                &surface, &plan) != HERCULES_SPRITE_NO_DRAW)
        return 0;

    make_variable(sprite, 0x64U, 1, 1, 0U, 0U, 4U, 2U);
    if (renderer_decode_packet(sprite, sizeof(sprite), &decoded) !=
        RENDERER_PACKET_OK || renderer_ac60_apply_packet(
            sprite, sizeof(sprite), &state, &event) != RENDERER_AC60_APPLY_OK)
        return 0;
    state.workspace.native_x_slot0++;
    if (hercules_sprite_resolve(&decoded, sizeof(sprite), &state, &texture,
                                &surface, &plan) != HERCULES_SPRITE_UNSUPPORTED)
        return 0;
    state.workspace.native_x_slot0--;
    state.clip_bounds.bottom = 239;
    if (hercules_sprite_resolve(&decoded, sizeof(sprite), &state, &texture,
                                &surface, &plan) != HERCULES_SPRITE_UNSUPPORTED)
        return 0;
    state.clip_bounds.bottom = 240;
    texture.length = 1UL;
    if (hercules_sprite_resolve(&decoded, sizeof(sprite), &state, &texture,
                                &surface, &plan) != HERCULES_SPRITE_RANGE_ERROR)
        return 0;
    texture.length = PAGE_BYTES;
    surface.length = 1UL;
    if (hercules_sprite_resolve(&decoded, sizeof(sprite), &state, &texture,
                                &surface, &plan) != HERCULES_SPRITE_RANGE_ERROR)
        return 0;
    surface.length = SURFACE_BYTES;
    texture.page_address += PAGE_BYTES;
    if (hercules_sprite_resolve(&decoded, sizeof(sprite), &state, &texture,
                                &surface, &plan) != HERCULES_SPRITE_UNSUPPORTED)
        return 0;
    texture.page_address -= PAGE_BYTES;

    /* The post-clip UV span cannot wrap to another source row/page. */
    make_variable(sprite, 0x64U, -2, 1, 254U, 0U, 4U, 1U);
    if (renderer_decode_packet(sprite, sizeof(sprite), &decoded) !=
        RENDERER_PACKET_OK || renderer_ac60_apply_packet(
            sprite, sizeof(sprite), &state, &event) != RENDERER_AC60_APPLY_OK ||
        hercules_sprite_resolve(&decoded, sizeof(sprite), &state, &texture,
                                &surface, &plan) != HERCULES_SPRITE_UNSUPPORTED)
        return 0;

    make_variable(sprite, 0x66U, 1, 1, 0U, 0U, 1U, 1U);
    /* Explicitly use an invalid table selector in already-applied state. */
    if (renderer_decode_packet(sprite, sizeof(sprite), &decoded) !=
        RENDERER_PACKET_OK || renderer_ac60_apply_packet(
            sprite, sizeof(sprite), &state, &event) != RENDERER_AC60_APPLY_OK)
        return 0;
    state.page.page_mode = HERC_PIXEL_BLEND_SELECTORS;
    return hercules_sprite_resolve(&decoded, sizeof(sprite), &state, &texture,
                                   &surface, &plan) ==
           HERCULES_SPRITE_UNSUPPORTED;
}

int main(void)
{
    if (!test_title_strip_plans()) {
        fprintf(stderr, "TITLE sprite strip resolution failed\n");
        return 1;
    }
    if (!test_fixed_and_clipped_geometry()) {
        fprintf(stderr, "fixed sprite or shared clipping contract failed\n");
        return 1;
    }
    if (!test_blend_pixel_and_zero_skip()) {
        fprintf(stderr, "sprite blend pixel/transparent skip failed\n");
        return 1;
    }
    if (!test_rejections_and_no_draw()) {
        fprintf(stderr, "sprite bounds/status contract failed\n");
        return 1;
    }
    puts("sprite raster contract: TITLE strips, fixed sprites, clipping, copy/blend, and bounded skips passed");
    return 0;
}
