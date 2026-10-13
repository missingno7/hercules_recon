#include "hercules_sprite_geometry.h"

#define SPRITE_RIGHT_LIMIT 320L
#define SPRITE_BLEND_FAMILY 0x92E3D0UL

static renderer_i32 renderer_signed_word(renderer_u16 word)
{
    if ((word & 0x8000U) != 0U) {
        return (renderer_i32)word - 0x10000L;
    }
    return (renderer_i32)word;
}

static renderer_u16 renderer_add_word(renderer_u16 word, renderer_i32 delta)
{
    renderer_u32 result;
    result = (renderer_u32)word + (renderer_u32)delta;
    return (renderer_u16)result;
}

static renderer_u32 renderer_add_u32(renderer_u32 left, renderer_u32 right)
{
    return left + right;
}

renderer_i32 renderer_sprite_geometry_prepare(
    renderer_sprite_geometry *geometry,
    renderer_u8 draw_gate,
    renderer_u8 path,
    renderer_u32 blend_selector)
{
    renderer_i32 signed_width;
    renderer_i32 signed_height;
    renderer_i32 right;
    renderer_i32 bottom;
    renderer_u32 row_product;
    renderer_u32 blend_offset;

    if (draw_gate == 0U || geometry->destination == 0UL) {
        return RENDERER_SPRITE_NO_DRAW;
    }
    if ((renderer_i32)geometry->x >= SPRITE_RIGHT_LIMIT ||
        (renderer_i32)geometry->y >= geometry->bottom_limit) {
        return RENDERER_SPRITE_NO_DRAW;
    }

    signed_width = renderer_signed_word(geometry->width_word);
    signed_height = renderer_signed_word(geometry->height_word);
    right = (renderer_i32)geometry->x + signed_width;
    bottom = (renderer_i32)geometry->y + signed_height;
    if (right < 0L || bottom < 0L) {
        return RENDERER_SPRITE_NO_DRAW;
    }

    if (geometry->x < 0) {
        geometry->width_word = renderer_add_word(geometry->width_word, geometry->x);
        geometry->u_offset -= (renderer_u32)(renderer_i32)geometry->x;
        geometry->x = 0;
    }
    if (geometry->y < 0) {
        geometry->height_word = renderer_add_word(geometry->height_word, geometry->y);
        geometry->v_offset -= (renderer_u32)(renderer_i32)geometry->y;
        geometry->y = 0;
    }

    if (right >= SPRITE_RIGHT_LIMIT) {
        geometry->width_word = renderer_add_word(
            geometry->width_word, SPRITE_RIGHT_LIMIT - right);
    }
    if (bottom >= geometry->bottom_limit) {
        geometry->height_word = renderer_add_word(
            geometry->height_word, geometry->bottom_limit - bottom);
    }

    signed_width = renderer_signed_word(geometry->width_word);
    signed_height = renderer_signed_word(geometry->height_word);
    if (signed_width <= 0L || signed_height <= 0L) {
        return RENDERER_SPRITE_NO_DRAW;
    }

    row_product = (renderer_u32)(renderer_i32)geometry->y * geometry->stride;
    geometry->destination_start = renderer_add_u32(
        renderer_add_u32(geometry->destination, row_product),
        (renderer_u32)(renderer_i32)geometry->x);
    geometry->page_u_base = renderer_add_u32(
        geometry->selected_page_base, geometry->u_offset);
    geometry->first_row_v_offset = geometry->v_offset << 8;

    if (path == RENDERER_SPRITE_BLEND_PATH) {
        blend_offset = (blend_selector & 3UL) << 16;
        geometry->blend_table = SPRITE_BLEND_FAMILY + blend_offset;
    }
    return RENDERER_SPRITE_PREPARED;
}
