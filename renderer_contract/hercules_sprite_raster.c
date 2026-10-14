#include "hercules_sprite_raster.h"

#include "hercules_sprite_geometry.h"
#include "hercules_textures.h"

#define HERCULES_SPRITE_TEXTURE_SIZE 256UL
#define HERCULES_SPRITE_PAGE_SIZE 65536UL
#define HERCULES_SPRITE_SURFACE_WIDTH 320UL
#define HERCULES_SPRITE_SURFACE_HEIGHT 240UL
#define HERCULES_SPRITE_SURFACE_PITCH 320UL
#define HERCULES_SPRITE_DESTINATION_TOKEN 1UL

static int sprite_add(renderer_u32 left, renderer_u32 right,
                      renderer_u32 *result)
{
    if (result == 0 || left > (renderer_u32)0xffffffffUL - right)
        return 0;
    *result = left + right;
    return 1;
}

static int sprite_multiply(renderer_u32 left, renderer_u32 right,
                           renderer_u32 *result)
{
    if (result == 0 || (right != 0UL &&
        left > (renderer_u32)0xffffffffUL / right))
        return 0;
    *result = left * right;
    return 1;
}

static renderer_i32 sprite_signed_word(renderer_u16 word)
{
    if ((word & 0x8000U) != 0U)
        return (renderer_i32)word - 0x10000L;
    return (renderer_i32)word;
}

static int sprite_packet_path(const RendererDecodedPacket *packet,
                              renderer_u8 *pixel_path,
                              renderer_u16 *expected_size)
{
    if (packet == 0 || pixel_path == 0 || expected_size == 0)
        return 0;

    if ((packet->command == 0x64U || packet->command == 0x66U) &&
        packet->kind == RENDERER_PACKET_SPRITE_VARIABLE) {
        *expected_size = 0x14U;
    } else if ((packet->command == 0x74U || packet->command == 0x76U) &&
               packet->kind == RENDERER_PACKET_SPRITE_8) {
        *expected_size = 0x0eU;
        if (packet->width != 8U || packet->height != 8U)
            return 0;
    } else if ((packet->command == 0x7cU || packet->command == 0x7eU) &&
               packet->kind == RENDERER_PACKET_SPRITE_16) {
        *expected_size = 0x0eU;
        if (packet->width != 16U || packet->height != 16U)
            return 0;
    } else {
        return 0;
    }

    if (packet->semitransparent !=
        (renderer_u8)((packet->command & 0x02U) != 0U))
        return 0;
    *pixel_path = packet->semitransparent != 0U
        ? RENDERER_SPRITE_BLEND_PATH : RENDERER_SPRITE_COPY_PATH;
    return 1;
}

static int sprite_page_from_state(const RendererAc60StateView *state,
                                  renderer_raw32 *page_address)
{
    renderer_u32 page_index;
    renderer_u32 page_offset;

    if (state == 0 || page_address == 0 ||
        state->page.page_selector > 0x1fUL ||
        state->page.alternate_bank != 0UL)
        return 0;
    page_index = state->page.page_selector >> 1;
    if (page_index >= 16UL ||
        !sprite_multiply(page_index, HERCULES_SPRITE_PAGE_SIZE,
                         &page_offset) ||
        !sprite_add(HERC_TEX_MAP_BASE, page_offset, page_address))
        return 0;
    return 1;
}

static int sprite_staging_matches(
    const RendererDecodedPacket *packet,
    const RendererAc60WorkspaceView *workspace)
{
    renderer_u32 expected_x;
    renderer_u32 expected_y;

    if (packet == 0 || workspace == 0)
        return 0;
    expected_x = (renderer_u32)(renderer_i32)packet->x[0];
    expected_y = (renderer_u32)(renderer_i32)packet->y[0];
    return workspace->native_x_slot0 == expected_x &&
           workspace->native_y_slot0 == expected_y &&
           workspace->native_u_slot0 == (renderer_u32)packet->u[0] &&
           workspace->native_v_slot0 == (renderer_u32)packet->v[0] &&
           workspace->sprite_width == packet->width &&
           workspace->sprite_height == packet->height;
}

static int sprite_storage_layout_valid(
    const HerculesRectangleTextureView *texture,
    const HerculesRectangleSurfaceView *surface)
{
    return texture != 0 && surface != 0 &&
           texture->width == HERCULES_SPRITE_TEXTURE_SIZE &&
           texture->height == HERCULES_SPRITE_TEXTURE_SIZE &&
           texture->row_pitch == HERCULES_SPRITE_TEXTURE_SIZE &&
           surface->width == HERCULES_SPRITE_SURFACE_WIDTH &&
           surface->height == HERCULES_SPRITE_SURFACE_HEIGHT &&
           surface->row_pitch == HERCULES_SPRITE_SURFACE_PITCH;
}

static int sprite_ordinary_page_address(renderer_raw32 page_address)
{
    renderer_u32 page_offset;
    if (page_address < HERC_TEX_MAP_BASE)
        return 0;
    page_offset = page_address - HERC_TEX_MAP_BASE;
    return page_offset < 16UL * HERCULES_SPRITE_PAGE_SIZE &&
           page_offset % HERCULES_SPRITE_PAGE_SIZE == 0UL;
}

static HerculesSpriteStatus sprite_plan_ranges(
    const HerculesSpritePlan *plan,
    const HerculesRectangleTextureView *texture,
    const HerculesRectangleSurfaceView *surface,
    renderer_u32 *source_last, renderer_u32 *destination_last)
{
    renderer_u32 right;
    renderer_u32 bottom;
    renderer_u32 source_row;
    renderer_u32 destination_row;
    renderer_u32 row_delta;

    if (plan == 0 || texture == 0 || surface == 0 ||
        source_last == 0 || destination_last == 0)
        return HERCULES_SPRITE_INVALID_ARGUMENT;
    if (!sprite_storage_layout_valid(texture, surface))
        return HERCULES_SPRITE_UNSUPPORTED;
    if (!sprite_ordinary_page_address(plan->page_address) ||
        plan->page_address != texture->page_address ||
        plan->source_pitch != texture->row_pitch ||
        plan->destination_pitch != surface->row_pitch ||
        plan->width == 0UL || plan->height == 0UL ||
        plan->width > HERCULES_SPRITE_SURFACE_WIDTH ||
        plan->height > HERCULES_SPRITE_SURFACE_HEIGHT ||
        plan->source_pitch != HERCULES_SPRITE_TEXTURE_SIZE ||
        plan->destination_pitch != HERCULES_SPRITE_SURFACE_PITCH)
        return HERCULES_SPRITE_UNSUPPORTED;

    if (!sprite_add(plan->x, plan->width, &right) ||
        !sprite_add(plan->y, plan->height, &bottom) ||
        right > surface->width || bottom > surface->height)
        return HERCULES_SPRITE_RANGE_ERROR;
    if (!sprite_add(plan->u, plan->width, &right) ||
        !sprite_add(plan->v, plan->height, &bottom) ||
        right > texture->width || bottom > texture->height)
        return HERCULES_SPRITE_UNSUPPORTED;

    if (!sprite_multiply(plan->v, texture->row_pitch, &source_row) ||
        !sprite_add(source_row, plan->u, &source_row) ||
        source_row != plan->source_start ||
        !sprite_multiply(plan->y, surface->row_pitch, &destination_row) ||
        !sprite_add(destination_row, plan->x, &destination_row) ||
        destination_row != plan->destination_start)
        return HERCULES_SPRITE_RANGE_ERROR;

    if (!sprite_multiply(plan->height - 1UL, texture->row_pitch,
                         &row_delta) ||
        !sprite_add(plan->source_start, row_delta, source_last) ||
        !sprite_add(*source_last, plan->width - 1UL, source_last) ||
        !sprite_multiply(plan->height - 1UL, surface->row_pitch,
                         &row_delta) ||
        !sprite_add(plan->destination_start, row_delta, destination_last) ||
        !sprite_add(*destination_last, plan->width - 1UL,
                    destination_last))
        return HERCULES_SPRITE_RANGE_ERROR;
    if (*source_last >= texture->length ||
        *destination_last >= surface->length)
        return HERCULES_SPRITE_RANGE_ERROR;
    return HERCULES_SPRITE_OK;
}

HerculesSpriteStatus hercules_sprite_resolve(
    const RendererDecodedPacket *packet, size_t packet_size,
    const RendererAc60StateView *state,
    const HerculesRectangleTextureView *texture,
    const HerculesRectangleSurfaceView *surface,
    HerculesSpritePlan *plan)
{
    renderer_u8 pixel_path;
    renderer_u16 expected_size;
    renderer_raw32 page_address;
    renderer_u32 selector;
    renderer_sprite_geometry geometry;
    renderer_i32 geometry_status;
    renderer_i32 signed_width;
    renderer_i32 signed_height;
    renderer_u32 source_row;
    renderer_u32 source_last;
    renderer_u32 destination_start;
    renderer_u32 destination_last;
    HerculesSpritePlan result;
    HerculesSpriteStatus status;

    if (packet == 0 || state == 0 || texture == 0 || surface == 0 ||
        plan == 0)
        return HERCULES_SPRITE_INVALID_ARGUMENT;
    if (!sprite_packet_path(packet, &pixel_path, &expected_size))
        return HERCULES_SPRITE_UNSUPPORTED;
    if (packet_size < (size_t)expected_size ||
        packet_size < (size_t)packet->minimum_size)
        return HERCULES_SPRITE_TRUNCATED;
    if (packet->minimum_size != expected_size)
        return HERCULES_SPRITE_UNSUPPORTED;

    /* AC60's sprite suppression exits before staging the packet. */
    if (state->raster_suppression_flag != 0U)
        return HERCULES_SPRITE_NO_DRAW;
    if (!sprite_storage_layout_valid(texture, surface))
        return HERCULES_SPRITE_UNSUPPORTED;
    if (!sprite_staging_matches(packet, &state->workspace))
        return HERCULES_SPRITE_UNSUPPORTED;
    if (!sprite_page_from_state(state, &page_address) ||
        state->workspace.texture_page_address != page_address ||
        texture->page_address != page_address)
        return HERCULES_SPRITE_UNSUPPORTED;

    selector = 0UL;
    if (pixel_path == RENDERER_SPRITE_BLEND_PATH) {
        if (state->page.page_mode >= HERC_PIXEL_BLEND_SELECTORS)
            return HERCULES_SPRITE_UNSUPPORTED;
        selector = state->page.page_mode;
    }

    geometry.x = packet->x[0];
    geometry.y = packet->y[0];
    geometry.width_word = packet->width;
    geometry.height_word = packet->height;
    geometry.u_offset = (renderer_u32)packet->u[0];
    geometry.v_offset = (renderer_u32)packet->v[0];
    geometry.selected_page_base = page_address;
    geometry.destination = HERCULES_SPRITE_DESTINATION_TOKEN;
    geometry.stride = surface->row_pitch;
    /* Native CAB0/C8F0 read the staged bottom bound. This slice accepts the
     * measured full-surface bound only; smaller F3 bounds need their own proof. */
    if (state->clip_bounds.bottom != (renderer_i32)surface->height)
        return HERCULES_SPRITE_UNSUPPORTED;
    geometry.bottom_limit = state->clip_bounds.bottom;
    geometry.destination_start = 0UL;
    geometry.page_u_base = 0UL;
    geometry.first_row_v_offset = 0UL;
    geometry.blend_table = 0UL;
    geometry_status = renderer_sprite_geometry_prepare(
        &geometry, 1U, pixel_path, selector);
    if (geometry_status == RENDERER_SPRITE_NO_DRAW)
        return HERCULES_SPRITE_NO_DRAW;
    if (geometry_status != RENDERER_SPRITE_PREPARED)
        return HERCULES_SPRITE_UNSUPPORTED;

    signed_width = sprite_signed_word(geometry.width_word);
    signed_height = sprite_signed_word(geometry.height_word);
    if (signed_width <= 0L || signed_height <= 0L)
        return HERCULES_SPRITE_NO_DRAW;
    if (!sprite_add(geometry.u_offset, (renderer_u32)signed_width,
                    &source_last) ||
        !sprite_add(geometry.v_offset, (renderer_u32)signed_height,
                    &destination_last) ||
        source_last > HERCULES_SPRITE_TEXTURE_SIZE ||
        destination_last > HERCULES_SPRITE_TEXTURE_SIZE)
        return HERCULES_SPRITE_UNSUPPORTED;

    if (!sprite_multiply(geometry.v_offset, texture->row_pitch,
                         &source_row) ||
        !sprite_add(source_row, geometry.u_offset, &source_row) ||
        geometry.destination_start < HERCULES_SPRITE_DESTINATION_TOKEN)
        return HERCULES_SPRITE_RANGE_ERROR;
    destination_start = geometry.destination_start -
                        HERCULES_SPRITE_DESTINATION_TOKEN;

    result.page_address = page_address;
    result.x = (renderer_u32)(renderer_i32)geometry.x;
    result.y = (renderer_u32)(renderer_i32)geometry.y;
    result.width = (renderer_u32)signed_width;
    result.height = (renderer_u32)signed_height;
    result.u = geometry.u_offset;
    result.v = geometry.v_offset;
    result.source_start = source_row;
    result.source_pitch = texture->row_pitch;
    result.destination_start = destination_start;
    result.destination_pitch = surface->row_pitch;
    result.pixel_mode = pixel_path == RENDERER_SPRITE_BLEND_PATH
        ? HERCULES_RECTANGLE_RAW_BLEND : HERCULES_RECTANGLE_COPY;
    result.blend_selector = selector;

    status = sprite_plan_ranges(&result, texture, surface, &source_last,
                                &destination_last);
    if (status != HERCULES_SPRITE_OK)
        return status;
    *plan = result;
    return HERCULES_SPRITE_OK;
}

typedef struct SpritePixelTarget {
    renderer_u8 *bytes;
    renderer_u32 length;
    renderer_u32 offset;
    int failed;
} SpritePixelTarget;

static int sprite_pixel_read(void *opaque, renderer_u8 *value)
{
    SpritePixelTarget *target = (SpritePixelTarget *)opaque;
    if (target == 0 || value == 0 || target->bytes == 0 ||
        target->offset >= target->length) {
        if (target != 0)
            target->failed = 1;
        return 0;
    }
    *value = target->bytes[target->offset];
    return 1;
}

static int sprite_pixel_write(void *opaque, renderer_u8 value)
{
    SpritePixelTarget *target = (SpritePixelTarget *)opaque;
    if (target == 0 || target->bytes == 0 || target->offset >= target->length) {
        if (target != 0)
            target->failed = 1;
        return 0;
    }
    target->bytes[target->offset] = value;
    return 1;
}

HerculesSpriteStatus hercules_sprite_render(
    const HerculesSpritePlan *plan,
    const HerculesRectangleTextureView *texture,
    const HerculesRectangleSurfaceView *surface,
    const HercPixelTableReader *selected_blend,
    HerculesRectanglePixelObserver observer, void *observer_context,
    HerculesRectangleEffects *effects)
{
    SpritePixelTarget target;
    HercPixelDestinationAccess destination;
    renderer_u32 source_last;
    renderer_u32 destination_last;
    renderer_u32 row;
    renderer_u32 column;
    HerculesSpriteStatus status;

    if (effects == 0)
        return HERCULES_SPRITE_INVALID_ARGUMENT;
    effects->pixel_store_count = 0UL;
    effects->accepted = 0U;
    effects->presentation_requested = 0U;
    if (plan == 0 || texture == 0 || surface == 0 ||
        texture->bytes == 0 || surface->bytes == 0)
        return HERCULES_SPRITE_INVALID_ARGUMENT;
    if (plan->pixel_mode == HERCULES_RECTANGLE_COPY) {
        if (plan->blend_selector != 0UL)
            return HERCULES_SPRITE_UNSUPPORTED;
    } else if (plan->pixel_mode == HERCULES_RECTANGLE_RAW_BLEND) {
        if (plan->blend_selector >= HERC_PIXEL_BLEND_SELECTORS)
            return HERCULES_SPRITE_UNSUPPORTED;
    } else {
        return HERCULES_SPRITE_UNSUPPORTED;
    }
    status = sprite_plan_ranges(plan, texture, surface, &source_last,
                                &destination_last);
    if (status != HERCULES_SPRITE_OK)
        return status;

    target.bytes = surface->bytes;
    target.length = surface->length;
    target.offset = 0UL;
    target.failed = 0;
    destination.read_byte = sprite_pixel_read;
    destination.write_byte = sprite_pixel_write;
    destination.context = &target;
    effects->accepted = 1U;

    for (row = 0UL; row < plan->height; ++row) {
        for (column = 0UL; column < plan->width; ++column) {
            renderer_u32 source_offset = plan->source_start +
                row * plan->source_pitch + column;
            renderer_u32 destination_offset = plan->destination_start +
                row * plan->destination_pitch + column;
            renderer_u8 source = texture->bytes[source_offset];
            HercPixelStepResult step;

            target.offset = destination_offset;
            /* Match the source-zero branches before any table or surface
             * callback, including a missing selected blend adapter. */
            if (source == 0U)
                continue;
            if (plan->pixel_mode == HERCULES_RECTANGLE_COPY) {
                step = herc_pixel_cab0_step(source, &destination);
            } else {
                if (selected_blend == 0)
                    return HERCULES_SPRITE_INVALID_ARGUMENT;
                step = herc_pixel_c8f0_step(source, selected_blend,
                                            &destination);
            }
            if (step == HERC_PIXEL_INVALID)
                return target.failed ? HERCULES_SPRITE_RANGE_ERROR :
                       HERCULES_SPRITE_INVALID_ARGUMENT;
            if (target.failed)
                return HERCULES_SPRITE_RANGE_ERROR;
            if (step == HERC_PIXEL_WROTE) {
                ++effects->pixel_store_count;
                if (observer != 0 &&
                    !observer(observer_context, source, destination_offset))
                    return HERCULES_SPRITE_OBSERVER_FAILED;
            }
        }
    }

    effects->presentation_requested = 1U;
    return HERCULES_SPRITE_OK;
}
