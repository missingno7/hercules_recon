#include "hercules_rectangle.h"

#include "hercules_quad_dispatch.h"

#define HERCULES_RECTANGLE_MAX_SIDE 8UL
#define HERCULES_RECTANGLE_TEXTURE_SIZE 256UL
#define HERCULES_RECTANGLE_SURFACE_WIDTH 320UL
#define HERCULES_RECTANGLE_SURFACE_HEIGHT 240UL
#define HERCULES_RECTANGLE_SURFACE_PITCH 320UL

static int rectangle_add(renderer_u32 left, renderer_u32 right,
                         renderer_u32 *result)
{
    if (result == 0 || left > (renderer_u32)0xffffffffUL - right)
        return 0;
    *result = left + right;
    return 1;
}

static int rectangle_multiply(renderer_u32 left, renderer_u32 right,
                              renderer_u32 *result)
{
    if (result == 0 || (right != 0UL &&
        left > (renderer_u32)0xffffffffUL / right))
        return 0;
    *result = left * right;
    return 1;
}

static void rectangle_state_lanes(const RendererAc60WorkspaceView *workspace,
                                  renderer_u32 x[4], renderer_u32 y[4],
                                  renderer_u32 u[4], renderer_u32 v[4])
{
    x[0] = workspace->native_x_slot0;
    x[1] = workspace->native_x_slot1;
    x[2] = workspace->native_x_slot2;
    x[3] = workspace->native_x_slot3;
    y[0] = workspace->native_y_slot0;
    y[1] = workspace->native_y_slot1;
    y[2] = workspace->native_y_slot2;
    y[3] = workspace->native_y_slot3;
    u[0] = workspace->native_u_slot0;
    u[1] = workspace->native_u_slot1;
    u[2] = workspace->native_u_slot2;
    u[3] = workspace->native_u_slot3;
    v[0] = workspace->native_v_slot0;
    v[1] = workspace->native_v_slot1;
    v[2] = workspace->native_v_slot2;
    v[3] = workspace->native_v_slot3;
}

static int rectangle_state_matches_packet(
    const RendererDecodedPacket *packet,
    const RendererAc60WorkspaceView *workspace)
{
    static const renderer_u8 packet_lane[4] = { 0U, 1U, 3U, 2U };
    renderer_u32 index;

    for (index = 0; index < 4UL; ++index) {
        renderer_u32 source_lane = (renderer_u32)packet_lane[index];
        renderer_u32 expected_x =
            (renderer_u32)(renderer_i32)packet->x[source_lane];
        renderer_u32 expected_y =
            (renderer_u32)(renderer_i32)packet->y[source_lane];
        renderer_u32 expected_u = (renderer_u32)packet->u[source_lane] << 16;
        renderer_u32 expected_v = (renderer_u32)packet->v[source_lane] << 16;
        renderer_u32 actual_x;
        renderer_u32 actual_y;
        renderer_u32 actual_u;
        renderer_u32 actual_v;

        if (index == 0UL) {
            actual_x = workspace->native_x_slot0;
            actual_y = workspace->native_y_slot0;
            actual_u = workspace->native_u_slot0;
            actual_v = workspace->native_v_slot0;
        } else if (index == 1UL) {
            actual_x = workspace->native_x_slot1;
            actual_y = workspace->native_y_slot1;
            actual_u = workspace->native_u_slot1;
            actual_v = workspace->native_v_slot1;
        } else if (index == 2UL) {
            actual_x = workspace->native_x_slot2;
            actual_y = workspace->native_y_slot2;
            actual_u = workspace->native_u_slot2;
            actual_v = workspace->native_v_slot2;
        } else {
            actual_x = workspace->native_x_slot3;
            actual_y = workspace->native_y_slot3;
            actual_u = workspace->native_u_slot3;
            actual_v = workspace->native_v_slot3;
        }
        if (actual_x != expected_x || actual_y != expected_y ||
            actual_u != expected_u || actual_v != expected_v)
            return 0;
    }
    return 1;
}

HerculesRectangleStatus hercules_rectangle_resolve(
    const RendererDecodedPacket *packet, size_t packet_size,
    const RendererAc60StateView *state,
    const HerculesRectangleTextureView *texture,
    const HerculesRectangleSurfaceView *surface,
    HerculesRectanglePlan *plan)
{
    const RendererAc60WorkspaceView *workspace;
    renderer_u32 x[4];
    renderer_u32 y[4];
    renderer_u32 u[4];
    renderer_u32 v[4];
    renderer_u32 x0;
    renderer_u32 y0;
    renderer_u32 width;
    renderer_u32 height;
    renderer_u32 source_u;
    renderer_u32 source_v;
    renderer_u32 right;
    renderer_u32 bottom;
    renderer_u32 row_offset;
    renderer_u32 source_last;
    renderer_u32 destination_last;
    renderer_u32 source_start;
    renderer_u32 destination_start;
    renderer_u32 pixel_mode;
    renderer_u32 blend_selector;
    renderer_u32 shade_row;

    if (packet == 0 || state == 0 || texture == 0 || surface == 0 ||
        plan == 0)
        return HERCULES_RECTANGLE_INVALID_ARGUMENT;
    if (packet_size < 0x26U || packet_size < (size_t)packet->minimum_size)
        return HERCULES_RECTANGLE_TRUNCATED;
    if (packet->kind != RENDERER_PACKET_FT4 ||
        packet->minimum_size < 0x26U)
        return HERCULES_RECTANGLE_UNSUPPORTED;
    pixel_mode = HERCULES_RECTANGLE_COPY;
    blend_selector = 0UL;
    shade_row = 0UL;
    if (packet->command == 0x2CU) {
        if (packet->semitransparent != 0U || packet->tpage != 0U ||
            packet->clut != 0U || packet->color[0][0] != 0x64U)
            return HERCULES_RECTANGLE_UNSUPPORTED;
    } else if (packet->command == 0x2EU) {
        if (packet->semitransparent != 1U || packet->clut != 0U ||
            (packet->tpage & 0xff9fU) != 0U)
            return HERCULES_RECTANGLE_UNSUPPORTED;
        blend_selector = (renderer_u32)(packet->tpage >> 5) & 3UL;
    } else {
        return HERCULES_RECTANGLE_UNSUPPORTED;
    }
    if (state->raster_suppression_flag != 0U ||
        texture->page_address == 0UL ||
        state->workspace.texture_page_address != texture->page_address ||
        state->workspace.vertex_count != 4UL ||
        state->workspace.color0_red != (renderer_u32)packet->color[0][0] ||
        state->workspace.clut_word != (renderer_u32)packet->clut ||
        !rectangle_state_matches_packet(packet, &state->workspace))
        return HERCULES_RECTANGLE_UNSUPPORTED;

    if (texture->width != HERCULES_RECTANGLE_TEXTURE_SIZE ||
        texture->height != HERCULES_RECTANGLE_TEXTURE_SIZE ||
        texture->row_pitch != HERCULES_RECTANGLE_TEXTURE_SIZE ||
        surface->width != HERCULES_RECTANGLE_SURFACE_WIDTH ||
        surface->height != HERCULES_RECTANGLE_SURFACE_HEIGHT ||
        surface->row_pitch != HERCULES_RECTANGLE_SURFACE_PITCH)
        return HERCULES_RECTANGLE_UNSUPPORTED;

    workspace = &state->workspace;
    if (packet->command == 0x2EU) {
        if (state->page.page_selector != 0UL ||
            state->page.page_mode != blend_selector ||
            state->page.alternate_bank != 0UL)
            return HERCULES_RECTANGLE_UNSUPPORTED;
        /* A90A00 is the staged red byte used by the original dispatcher.
         * A8E6F8 is only the selected blend table, checked above for staging
         * consistency; it does not choose raw versus shaded blending. */
        if (workspace->color0_red == 0x80UL) {
            pixel_mode = HERCULES_RECTANGLE_RAW_BLEND;
        } else {
            pixel_mode = HERCULES_RECTANGLE_SHADED_BLEND;
            shade_row = workspace->color0_red;
        }
    }
    rectangle_state_lanes(workspace, x, y, u, v);
    if (pixel_mode == HERCULES_RECTANGLE_COPY) {
        if (!renderer_d420_cab0_candidate(1U, 1U, workspace->vertex_count,
                                          x, y, u, v) ||
            renderer_e680_c8f0_candidate(1U, 1U, workspace->color0_red,
                                         workspace->vertex_count, x, y, u, v))
            return HERCULES_RECTANGLE_UNSUPPORTED;
    } else if (pixel_mode == HERCULES_RECTANGLE_RAW_BLEND) {
        if (!renderer_e680_c8f0_candidate(1U, 1U, workspace->color0_red,
                                          workspace->vertex_count,
                                          x, y, u, v))
            return HERCULES_RECTANGLE_UNSUPPORTED;
    } else if (!renderer_d420_cab0_candidate(1U, 1U,
                                             workspace->vertex_count,
                                             x, y, u, v)) {
        return HERCULES_RECTANGLE_UNSUPPORTED;
    }

    x0 = x[0];
    y0 = y[0];
    if ((x0 & 0x80000000UL) != 0UL ||
        (y0 & 0x80000000UL) != 0UL)
        return HERCULES_RECTANGLE_UNSUPPORTED;
    width = x[1] - x0;
    height = y[3] - y0;
    source_u = u[0] >> 16;
    source_v = v[0] >> 16;
    if (width == 0UL || height == 0UL ||
        width > HERCULES_RECTANGLE_MAX_SIDE ||
        height > HERCULES_RECTANGLE_MAX_SIDE)
        return HERCULES_RECTANGLE_UNSUPPORTED;

    if (!rectangle_add(x0, width, &right) ||
        !rectangle_add(y0, height, &bottom) ||
        !rectangle_add(source_u, width, &source_last) ||
        !rectangle_add(source_v, height, &destination_last))
        return HERCULES_RECTANGLE_RANGE_ERROR;
    if (right > surface->width || bottom > surface->height ||
        source_last > texture->width ||
        destination_last > texture->height)
        return HERCULES_RECTANGLE_RANGE_ERROR;

    if (!rectangle_multiply(source_v, texture->row_pitch, &row_offset) ||
        !rectangle_add(row_offset, source_u, &source_start) ||
        !rectangle_multiply(y0, surface->row_pitch, &row_offset) ||
        !rectangle_add(row_offset, x0, &destination_start) ||
        !rectangle_multiply(height - 1UL, texture->row_pitch, &row_offset) ||
        !rectangle_add(source_start, row_offset, &source_last) ||
        !rectangle_add(source_last, width - 1UL, &source_last) ||
        !rectangle_multiply(height - 1UL, surface->row_pitch, &row_offset) ||
        !rectangle_add(destination_start, row_offset, &destination_last) ||
        !rectangle_add(destination_last, width - 1UL, &destination_last))
        return HERCULES_RECTANGLE_RANGE_ERROR;
    if (source_last >= texture->length ||
        destination_last >= surface->length)
        return HERCULES_RECTANGLE_RANGE_ERROR;

    plan->page_address = texture->page_address;
    plan->x = x0;
    plan->y = y0;
    plan->width = width;
    plan->height = height;
    plan->u = source_u;
    plan->v = source_v;
    plan->source_start = source_start;
    plan->source_pitch = texture->row_pitch;
    plan->destination_start = destination_start;
    plan->destination_pitch = surface->row_pitch;
    plan->pixel_mode = pixel_mode;
    plan->blend_selector = blend_selector;
    plan->shade_row = shade_row;
    return HERCULES_RECTANGLE_OK;
}

typedef struct RectanglePixelTarget {
    renderer_u8 *bytes;
    renderer_u32 length;
    renderer_u32 offset;
    int failed;
} RectanglePixelTarget;

static int rectangle_pixel_write(void *opaque, renderer_u8 value)
{
    RectanglePixelTarget *target = (RectanglePixelTarget *)opaque;
    if (target == 0 || target->bytes == 0 || target->offset >= target->length) {
        if (target != 0)
            target->failed = 1;
        return 0;
    }
    target->bytes[target->offset] = value;
    return 1;
}

static int rectangle_pixel_read(void *opaque, renderer_u8 *value)
{
    RectanglePixelTarget *target = (RectanglePixelTarget *)opaque;
    if (target == 0 || value == 0 || target->bytes == 0 ||
        target->offset >= target->length) {
        if (target != 0)
            target->failed = 1;
        return 0;
    }
    *value = target->bytes[target->offset];
    return 1;
}

HerculesRectangleStatus hercules_rectangle_render_tables(
    const HerculesRectanglePlan *plan,
    const HerculesRectangleTextureView *texture,
    const HerculesRectangleSurfaceView *surface,
    const HercPixelTableReader *shade_row,
    const HercPixelTableReader *blend_tables,
    renderer_u32 blend_table_count,
    HerculesRectanglePixelObserver observer, void *observer_context,
    HerculesRectangleEffects *effects)
{
    RectanglePixelTarget target;
    HercPixelDestinationAccess destination;
    renderer_u32 right;
    renderer_u32 bottom;
    renderer_u32 source_row_offset;
    renderer_u32 destination_row_offset;
    renderer_u32 source_last;
    renderer_u32 destination_last;
    renderer_u32 row;
    renderer_u32 column;

    if (effects == 0)
        return HERCULES_RECTANGLE_INVALID_ARGUMENT;
    effects->pixel_store_count = 0UL;
    effects->accepted = 0U;
    effects->presentation_requested = 0U;
    if (plan == 0 || texture == 0 || surface == 0 ||
        texture->bytes == 0 || surface->bytes == 0)
        return HERCULES_RECTANGLE_INVALID_ARGUMENT;
    if (plan->pixel_mode == HERCULES_RECTANGLE_COPY) {
        if (plan->blend_selector != 0UL || plan->shade_row != 0UL)
            return HERCULES_RECTANGLE_UNSUPPORTED;
    } else if (plan->pixel_mode == HERCULES_RECTANGLE_RAW_BLEND) {
        if (plan->blend_selector >= HERC_PIXEL_BLEND_SELECTORS ||
            plan->shade_row != 0UL)
            return HERCULES_RECTANGLE_UNSUPPORTED;
    } else if (plan->pixel_mode == HERCULES_RECTANGLE_SHADED_BLEND) {
        if (plan->blend_selector >= HERC_PIXEL_BLEND_SELECTORS ||
            plan->shade_row > 0xffUL || plan->shade_row == 0x80UL)
            return HERCULES_RECTANGLE_UNSUPPORTED;
    } else {
        return HERCULES_RECTANGLE_UNSUPPORTED;
    }
    if (plan->page_address != texture->page_address ||
        texture->width != HERCULES_RECTANGLE_TEXTURE_SIZE ||
        texture->height != HERCULES_RECTANGLE_TEXTURE_SIZE ||
        texture->row_pitch != HERCULES_RECTANGLE_TEXTURE_SIZE ||
        surface->width != HERCULES_RECTANGLE_SURFACE_WIDTH ||
        surface->height != HERCULES_RECTANGLE_SURFACE_HEIGHT ||
        surface->row_pitch != HERCULES_RECTANGLE_SURFACE_PITCH ||
        plan->width == 0UL ||
        plan->height == 0UL || plan->width > HERCULES_RECTANGLE_MAX_SIDE ||
        plan->height > HERCULES_RECTANGLE_MAX_SIDE ||
        plan->source_pitch != texture->row_pitch ||
        plan->destination_pitch != surface->row_pitch)
        return HERCULES_RECTANGLE_UNSUPPORTED;
    if (!rectangle_add(plan->x, plan->width, &right) ||
        !rectangle_add(plan->y, plan->height, &bottom) ||
        right > surface->width || bottom > surface->height)
        return HERCULES_RECTANGLE_RANGE_ERROR;
    if (!rectangle_add(plan->u, plan->width, &right) ||
        !rectangle_add(plan->v, plan->height, &bottom) ||
        right > texture->width || bottom > texture->height)
        return HERCULES_RECTANGLE_RANGE_ERROR;

    if (!rectangle_multiply(plan->v, texture->row_pitch,
                            &source_row_offset) ||
        !rectangle_add(source_row_offset, plan->u, &source_row_offset) ||
        !rectangle_multiply(plan->y, surface->row_pitch,
                            &destination_row_offset) ||
        !rectangle_add(destination_row_offset, plan->x,
                       &destination_row_offset) ||
        source_row_offset != plan->source_start ||
        destination_row_offset != plan->destination_start ||
        !rectangle_multiply(plan->height - 1UL, texture->row_pitch,
                            &source_row_offset) ||
        !rectangle_add(plan->source_start, source_row_offset,
                       &source_last) ||
        !rectangle_add(source_last, plan->width - 1UL, &source_last) ||
        !rectangle_multiply(plan->height - 1UL, surface->row_pitch,
                            &destination_row_offset) ||
        !rectangle_add(plan->destination_start, destination_row_offset,
                       &destination_last) ||
        !rectangle_add(destination_last, plan->width - 1UL,
                       &destination_last))
        return HERCULES_RECTANGLE_RANGE_ERROR;
    if (source_last >= texture->length ||
        destination_last >= surface->length)
        return HERCULES_RECTANGLE_RANGE_ERROR;

    target.bytes = surface->bytes;
    target.length = surface->length;
    target.offset = 0UL;
    target.failed = 0;
    destination.read_byte = rectangle_pixel_read;
    destination.write_byte = rectangle_pixel_write;
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
            /* The original loops test source zero before touching destination
             * or either LUT. Skip it here so null table adapters remain safe
             * for fully transparent resolved rectangles. */
            if (source == 0U)
                continue;
            if (plan->pixel_mode == HERCULES_RECTANGLE_COPY) {
                step = herc_pixel_cab0_step(source, &destination);
            } else if (plan->pixel_mode == HERCULES_RECTANGLE_RAW_BLEND) {
                if (blend_tables == 0 ||
                    blend_table_count != HERC_PIXEL_BLEND_SELECTORS)
                    return HERCULES_RECTANGLE_INVALID_ARGUMENT;
                step = herc_pixel_c8f0_step(
                    source, &blend_tables[plan->blend_selector],
                    &destination);
            } else {
                if (shade_row == 0 || blend_tables == 0 ||
                    blend_table_count != HERC_PIXEL_BLEND_SELECTORS)
                    return HERCULES_RECTANGLE_INVALID_ARGUMENT;
                step = herc_pixel_c882_step(
                    source, shade_row, blend_tables, blend_table_count,
                    plan->blend_selector, &destination);
            }
            if (step == HERC_PIXEL_INVALID)
                return target.failed ? HERCULES_RECTANGLE_RANGE_ERROR :
                       HERCULES_RECTANGLE_INVALID_ARGUMENT;
            if (target.failed)
                return HERCULES_RECTANGLE_RANGE_ERROR;
            if (step == HERC_PIXEL_WROTE) {
                ++effects->pixel_store_count;
                if (observer != 0 &&
                    !observer(observer_context, source, destination_offset))
                    return HERCULES_RECTANGLE_OBSERVER_FAILED;
            }
        }
    }
    effects->presentation_requested = 1U;
    return HERCULES_RECTANGLE_OK;
}

HerculesRectangleStatus hercules_rectangle_render(
    const HerculesRectanglePlan *plan,
    const HerculesRectangleTextureView *texture,
    const HerculesRectangleSurfaceView *surface,
    HerculesRectanglePixelObserver observer, void *observer_context,
    HerculesRectangleEffects *effects)
{
    if (plan != 0 && plan->pixel_mode != HERCULES_RECTANGLE_COPY) {
        if (effects == 0)
            return HERCULES_RECTANGLE_INVALID_ARGUMENT;
        effects->pixel_store_count = 0UL;
        effects->accepted = 0U;
        effects->presentation_requested = 0U;
        return HERCULES_RECTANGLE_UNSUPPORTED;
    }
    return hercules_rectangle_render_tables(
        plan, texture, surface, 0, 0, 0UL, observer, observer_context,
        effects);
}
