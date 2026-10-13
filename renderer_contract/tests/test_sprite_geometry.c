#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "hercules_sprite_geometry.h"

typedef char sprite_x_offset[(offsetof(renderer_sprite_geometry, x) == 0) ? 1 : -1];
typedef char sprite_width_offset[(offsetof(renderer_sprite_geometry, width_word) == 4) ? 1 : -1];
typedef char sprite_uv_offset[(offsetof(renderer_sprite_geometry, u_offset) == 8) ? 1 : -1];
typedef char sprite_pointer_offset[(offsetof(renderer_sprite_geometry, selected_page_base) == 16) ? 1 : -1];
typedef char sprite_output_offset[(offsetof(renderer_sprite_geometry, destination_start) == 32) ? 1 : -1];
typedef char sprite_state_size[(sizeof(renderer_sprite_geometry) == 48) ? 1 : -1];

#define CHECK(name, expression) \
    do { if (!(expression)) { \
        printf("FAIL %s at line %d\n", name, (int)__LINE__); return 1; \
    } } while (0)

static renderer_sprite_geometry make_geometry(renderer_i16 x, renderer_i16 y,
    renderer_u16 width, renderer_u16 height, renderer_u32 u, renderer_u32 v)
{
    renderer_sprite_geometry result;
    result.x = x;
    result.y = y;
    result.width_word = width;
    result.height_word = height;
    result.u_offset = u;
    result.v_offset = v;
    result.selected_page_base = 0x32000000UL;
    result.destination = 0x31000000UL;
    result.stride = 640UL;
    result.bottom_limit = 240L;
    result.destination_start = 0xAAAAAAAAUL;
    result.page_u_base = 0xBBBBBBBBUL;
    result.first_row_v_offset = 0xCCCCCCCCUL;
    result.blend_table = 0xDDDDDDDDUL;
    return result;
}

static int same_outputs(renderer_sprite_geometry *g, renderer_raw32 dest,
    renderer_raw32 page, renderer_raw32 row, renderer_raw32 blend)
{
    return g->destination_start == dest && g->page_u_base == page &&
        g->first_row_v_offset == row && g->blend_table == blend;
}

int main(void)
{
    renderer_sprite_geometry g;
    renderer_i32 status;

    g = make_geometry(10, 20, 4, 5, 250UL, 254UL);
    status = renderer_sprite_geometry_prepare(&g, 1U, RENDERER_SPRITE_COPY_PATH, 6UL);
    CHECK("plain status", status == RENDERER_SPRITE_PREPARED);
    CHECK("plain copy outputs", same_outputs(&g, 0x3100320AUL, 0x320000FAUL,
        0x0000FE00UL, 0xDDDDDDDDUL));

    g = make_geometry(-3, -5, 7, 8, 254UL, 253UL);
    status = renderer_sprite_geometry_prepare(&g, 1U, RENDERER_SPRITE_BLEND_PATH, 7UL);
    CHECK("left top status", status == RENDERER_SPRITE_PREPARED);
    CHECK("left top geometry", g.x == 0 && g.y == 0 && g.width_word == 4 &&
        g.height_word == 3 && g.u_offset == 257UL && g.v_offset == 258UL);
    CHECK("left top setup", same_outputs(&g, 0x31000000UL, 0x32000101UL,
        0x00010200UL, 0x0095E3D0UL));

    g = make_geometry(318, 238, 8, 8, 255UL, 255UL);
    status = renderer_sprite_geometry_prepare(&g, 1U, RENDERER_SPRITE_COPY_PATH, 0UL);
    CHECK("right bottom status", status == RENDERER_SPRITE_PREPARED);
    CHECK("right bottom geometry", g.width_word == 2 && g.height_word == 2 &&
        g.u_offset == 255UL && g.v_offset == 255UL);
    CHECK("right bottom setup", same_outputs(&g, 0x3102543EUL, 0x320000FFUL,
        0x0000FF00UL, 0xDDDDDDDDUL));

    g = make_geometry(-8, 2, 7, 1, 5UL, 9UL);
    status = renderer_sprite_geometry_prepare(&g, 1U, RENDERER_SPRITE_BLEND_PATH, 0UL);
    CHECK("fully left status", status == RENDERER_SPRITE_NO_DRAW);
    CHECK("fully left unchanged", g.x == -8 && g.width_word == 7 && g.u_offset == 5UL &&
        same_outputs(&g, 0xAAAAAAAAUL, 0xBBBBBBBBUL, 0xCCCCCCCCUL, 0xDDDDDDDDUL));

    g = make_geometry(10, 10, 0xFFFFU, 2, 5UL, 9UL);
    status = renderer_sprite_geometry_prepare(&g, 1U, RENDERER_SPRITE_COPY_PATH, 0UL);
    CHECK("negative width status", status == RENDERER_SPRITE_NO_DRAW);
    CHECK("negative width unchanged", g.width_word == 0xFFFFU &&
        same_outputs(&g, 0xAAAAAAAAUL, 0xBBBBBBBBUL, 0xCCCCCCCCUL, 0xDDDDDDDDUL));

    g = make_geometry(10, 10, 0x8000U, 2, 5UL, 9UL);
    status = renderer_sprite_geometry_prepare(&g, 1U, RENDERER_SPRITE_COPY_PATH, 0UL);
    CHECK("most negative width status", status == RENDERER_SPRITE_NO_DRAW);
    CHECK("most negative width unchanged", g.width_word == 0x8000U &&
        same_outputs(&g, 0xAAAAAAAAUL, 0xBBBBBBBBUL, 0xCCCCCCCCUL, 0xDDDDDDDDUL));

    g = make_geometry(-5, 1, 5, 2, 251UL, 9UL);
    status = renderer_sprite_geometry_prepare(&g, 1U, RENDERER_SPRITE_BLEND_PATH, 1UL);
    CHECK("left empty status", status == RENDERER_SPRITE_NO_DRAW);
    CHECK("left empty mutations", g.x == 0 && g.width_word == 0 && g.u_offset == 256UL);
    CHECK("left empty no setup", same_outputs(&g, 0xAAAAAAAAUL, 0xBBBBBBBBUL,
        0xCCCCCCCCUL, 0xDDDDDDDDUL));

    g = make_geometry(7, 9, 0, 2, 1UL, 2UL);
    status = renderer_sprite_geometry_prepare(&g, 1U, RENDERER_SPRITE_COPY_PATH, 0UL);
    CHECK("zero width status", status == RENDERER_SPRITE_NO_DRAW);
    CHECK("zero width no setup", same_outputs(&g, 0xAAAAAAAAUL, 0xBBBBBBBBUL,
        0xCCCCCCCCUL, 0xDDDDDDDDUL));

    g = make_geometry(7, 9, 2, 0, 1UL, 2UL);
    status = renderer_sprite_geometry_prepare(&g, 1U, RENDERER_SPRITE_COPY_PATH, 0UL);
    CHECK("zero height status", status == RENDERER_SPRITE_NO_DRAW);
    CHECK("zero height no setup", same_outputs(&g, 0xAAAAAAAAUL, 0xBBBBBBBBUL,
        0xCCCCCCCCUL, 0xDDDDDDDDUL));

    g = make_geometry(7, 9, 2, 0xFFFEU, 1UL, 2UL);
    status = renderer_sprite_geometry_prepare(&g, 1U, RENDERER_SPRITE_COPY_PATH, 0UL);
    CHECK("negative height status", status == RENDERER_SPRITE_NO_DRAW);
    CHECK("negative height no setup", same_outputs(&g, 0xAAAAAAAAUL, 0xBBBBBBBBUL,
        0xCCCCCCCCUL, 0xDDDDDDDDUL));

    g = make_geometry(319, 239, 2, 2, 5UL, 6UL);
    status = renderer_sprite_geometry_prepare(&g, 1U, RENDERER_SPRITE_BLEND_PATH, 5UL);
    CHECK("bottom-right clamp status", status == RENDERER_SPRITE_PREPARED);
    CHECK("bottom-right clamp dimensions", g.width_word == 1 && g.height_word == 1);
    CHECK("bottom-right clamp blend", g.blend_table == 0x0093E3D0UL);

    g = make_geometry(12, 13, 3, 4, 5UL, 6UL);
    status = renderer_sprite_geometry_prepare(&g, 0U, RENDERER_SPRITE_BLEND_PATH, 3UL);
    CHECK("disabled status", status == RENDERER_SPRITE_NO_DRAW);
    CHECK("disabled no mutation", g.x == 12 && g.y == 13 && g.width_word == 3 &&
        g.height_word == 4 && g.u_offset == 5UL && g.v_offset == 6UL &&
        same_outputs(&g, 0xAAAAAAAAUL, 0xBBBBBBBBUL, 0xCCCCCCCCUL, 0xDDDDDDDDUL));

    printf("PASS renderer sprite geometry bounded contract\n");
    return 0;
}
