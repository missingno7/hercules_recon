#include "hercules_staging.h"

#define AC60_TEXTURE_BASE 0x0096E6E0UL

static void clear_event(RendererAc60Event *event)
{
    renderer_u8 *bytes;
    size_t index;

    bytes = (renderer_u8 *)event;
    for (index = 0; index < sizeof(*event); ++index)
        bytes[index] = 0;
}

static renderer_u32 uv_q16(renderer_u8 value)
{
    return ((renderer_u32)value) << 16;
}

static renderer_raw32 texture_address(renderer_u32 selector,
                                      renderer_u32 alternate_bank)
{
    if (alternate_bank != 0)
        return 0x00A6E6E0UL;
    return (renderer_raw32)(AC60_TEXTURE_BASE + ((selector >> 1) << 16));
}

static renderer_u32 polygon_alternate_bank(renderer_u16 tpage)
{
    /* Native FT4/GT4 discriminator vectors are 0,0,1 for 0x80,0x100,0x180;
     * this is separate from the packet command's semitransparency bit. */
    return ((tpage & 0x0180U) == 0x0180U) ? 1UL : 0UL;
}

static void refresh_texture_page_base(RendererAc60StateView *state)
{
    state->workspace.texture_page_address = texture_address(
        state->page.page_selector, state->page.alternate_bank);
}

static void set_polygon_page(RendererAc60StateView *state,
                             const RendererDecodedPacket *packet)
{
    state->page.page_selector = (renderer_u32)(packet->tpage & 0x001fU);
    state->page.page_mode = (renderer_u32)((packet->tpage >> 5) & 3U);
    state->page.alternate_bank = polygon_alternate_bank(packet->tpage);
    state->workspace.texture_page_address = texture_address(
        state->page.page_selector, state->page.alternate_bank);
    state->workspace.color0_red = packet->color[0][0];
    state->workspace.clut_word = packet->clut;
}

static void stage_quad(RendererAc60StateView *state,
                       const RendererDecodedPacket *packet)
{
    RendererAc60WorkspaceView *w;

    w = &state->workspace;
    w->native_x_slot0 = (renderer_u32)(renderer_i32)packet->x[0];
    w->native_x_slot1 = (renderer_u32)(renderer_i32)packet->x[1];
    w->native_x_slot2 = (renderer_u32)(renderer_i32)packet->x[3];
    w->native_x_slot3 = (renderer_u32)(renderer_i32)packet->x[2];
    w->native_y_slot0 = (renderer_u32)(renderer_i32)packet->y[0];
    w->native_y_slot1 = (renderer_u32)(renderer_i32)packet->y[1];
    w->native_y_slot2 = (renderer_u32)(renderer_i32)packet->y[3];
    w->native_y_slot3 = (renderer_u32)(renderer_i32)packet->y[2];
    w->native_u_slot0 = uv_q16(packet->u[0]);
    w->native_u_slot1 = uv_q16(packet->u[1]);
    w->native_u_slot2 = uv_q16(packet->u[3]);
    w->native_u_slot3 = uv_q16(packet->u[2]);
    w->native_v_slot0 = uv_q16(packet->v[0]);
    w->native_v_slot1 = uv_q16(packet->v[1]);
    w->native_v_slot2 = uv_q16(packet->v[3]);
    w->native_v_slot3 = uv_q16(packet->v[2]);
    w->vertex_count = 4UL;
    set_polygon_page(state, packet);
}

static void stage_triangle(RendererAc60StateView *state,
                           const RendererDecodedPacket *packet)
{
    RendererAc60WorkspaceView *w;

    w = &state->workspace;
    w->native_x_slot0 = (renderer_u32)(renderer_i32)packet->x[0];
    w->native_x_slot1 = (renderer_u32)(renderer_i32)packet->x[1];
    w->native_x_slot2 = (renderer_u32)(renderer_i32)packet->x[2];
    w->native_x_slot3 = (renderer_u32)(renderer_i32)packet->x[2];
    w->native_y_slot0 = (renderer_u32)(renderer_i32)packet->y[0];
    w->native_y_slot1 = (renderer_u32)(renderer_i32)packet->y[1];
    w->native_y_slot2 = (renderer_u32)(renderer_i32)packet->y[2];
    w->native_y_slot3 = (renderer_u32)(renderer_i32)packet->y[2];
    w->native_u_slot0 = uv_q16(packet->u[0]);
    w->native_u_slot1 = uv_q16(packet->u[1]);
    w->native_u_slot2 = uv_q16(packet->u[2]);
    w->native_v_slot0 = uv_q16(packet->v[0]);
    w->native_v_slot1 = uv_q16(packet->v[1]);
    w->native_v_slot3 = uv_q16(packet->u[2]);
    w->native_v_slot2 = uv_q16(packet->v[2]);
    w->vertex_count = 3UL;
    set_polygon_page(state, packet);
}

static void stage_sprite(RendererAc60StateView *state,
                         const RendererDecodedPacket *packet)
{
    RendererAc60WorkspaceView *w;

    w = &state->workspace;
    w->native_x_slot0 = (renderer_u32)(renderer_i32)packet->x[0];
    w->native_y_slot0 = (renderer_u32)(renderer_i32)packet->y[0];
    w->native_u_slot0 = packet->u[0];
    w->native_v_slot0 = packet->v[0];
    w->texture_page_address = texture_address(
        state->page.page_selector, state->page.alternate_bank);
    w->sprite_width = packet->width;
    w->sprite_height = packet->height;
}

static RendererAc60ApplyStatus decode_status(RendererPacketStatus status)
{
    if (status == RENDERER_PACKET_INVALID_ARGUMENT)
        return RENDERER_AC60_APPLY_INVALID_ARGUMENT;
    if (status == RENDERER_PACKET_TRUNCATED ||
        status == RENDERER_PACKET_SHORT_HEADER)
        return RENDERER_AC60_APPLY_TRUNCATED;
    if (status == RENDERER_PACKET_RANGE_ERROR ||
        status == RENDERER_PACKET_ADDRESS_OVERFLOW)
        return RENDERER_AC60_APPLY_RANGE_ERROR;
    if (status == RENDERER_PACKET_UNSUPPORTED)
        return RENDERER_AC60_APPLY_UNSUPPORTED;
    return RENDERER_AC60_APPLY_UNRECOGNIZED;
}

RendererAc60ApplyStatus renderer_ac60_apply_packet(
    const renderer_u8 *wire, size_t length,
    RendererAc60StateView *state, RendererAc60Event *event)
{
    RendererDecodedPacket packet;
    RendererAc60Event result_event;
    RendererAc60ApplyStatus result;
    RendererPacketStatus packet_status;
    HerculesE1StateUpdate e1;
    HerculesF3ClipBounds f3;
    HerculesCommand01Move move;
    HerculesStateStatus state_status;

    if (wire == 0 || state == 0 || event == 0)
        return RENDERER_AC60_APPLY_INVALID_ARGUMENT;
    if (length < 8U)
        return RENDERER_AC60_APPLY_TRUNCATED;

    clear_event(&result_event);
    result_event.command = wire[7];
    packet_status = renderer_decode_packet(wire, length, &packet);
    result_event.packet_kind = packet.kind;
    result_event.semitransparent = packet.semitransparent;

    if ((result_event.command & 0xfdU) == 0x68U) {
        /* AC60's group-0x60 preamble refreshes A909FC before testing for the
         * 0x64/0x74/0x7c draw forms. The 0x68 consumer path has no draw call,
         * but keeps that page write. Payload bytes beyond the 8-byte header
         * remain opaque. BA01FC nonzero takes the original early-return path. */
        if (state->raster_suppression_flag == 0)
            refresh_texture_page_base(state);
        result_event.kind = RENDERER_AC60_EVENT_IGNORED_RASTER;
        result = RENDERER_AC60_APPLY_OK;
    } else if (result_event.command == 0x40U) {
        result_event.kind = RENDERER_AC60_EVENT_OT_GATE;
        result = RENDERER_AC60_APPLY_OK;
    } else if (packet_status == RENDERER_PACKET_OK &&
               (packet.kind == RENDERER_PACKET_FT3 ||
                packet.kind == RENDERER_PACKET_FT4 ||
                packet.kind == RENDERER_PACKET_GT4)) {
        if (state->raster_suppression_flag != 0) {
            result_event.kind = RENDERER_AC60_EVENT_RASTER_SUPPRESSED;
            result = RENDERER_AC60_APPLY_OK;
        } else if (packet.kind == RENDERER_PACKET_FT3) {
            stage_triangle(state, &packet);
            result_event.kind = RENDERER_AC60_EVENT_FT3;
        } else {
            stage_quad(state, &packet);
            result_event.kind = (packet.kind == RENDERER_PACKET_FT4)
                ? RENDERER_AC60_EVENT_FT4 : RENDERER_AC60_EVENT_GT4;
        }
        result = RENDERER_AC60_APPLY_OK;
    } else if (packet_status == RENDERER_PACKET_OK &&
               (packet.kind == RENDERER_PACKET_SPRITE_VARIABLE ||
                packet.kind == RENDERER_PACKET_SPRITE_8 ||
                packet.kind == RENDERER_PACKET_SPRITE_16)) {
        if (state->raster_suppression_flag != 0) {
            result_event.kind = RENDERER_AC60_EVENT_RASTER_SUPPRESSED;
        } else {
            stage_sprite(state, &packet);
            if (packet.kind == RENDERER_PACKET_SPRITE_VARIABLE)
                result_event.kind = RENDERER_AC60_EVENT_VARIABLE_SPRITE;
            else if (packet.kind == RENDERER_PACKET_SPRITE_8)
                result_event.kind = RENDERER_AC60_EVENT_SPRITE_8;
            else
                result_event.kind = RENDERER_AC60_EVENT_SPRITE_16;
        }
        result = RENDERER_AC60_APPLY_OK;
    } else if (packet_status == RENDERER_PACKET_OK &&
               packet.kind == RENDERER_PACKET_E1_MODE) {
        state_status = hercules_state_decode_e1(wire, length, &e1);
        if (state_status != HERCULES_STATE_OK)
            return RENDERER_AC60_APPLY_UNSUPPORTED;
        state->page.page_selector = e1.page_index;
        state->page.page_mode = e1.table_selector;
        state->page.alternate_bank = e1.alternate_bank;
        result_event.kind = RENDERER_AC60_EVENT_E1_STATE;
        result = RENDERER_AC60_APPLY_OK;
    } else if (result_event.command == 0xf3U) {
        state_status = hercules_state_decode_f3(wire, length, &f3);
        if (state_status != HERCULES_STATE_OK) {
            return (state_status == HERCULES_STATE_TRUNCATED)
                ? RENDERER_AC60_APPLY_TRUNCATED : RENDERER_AC60_APPLY_UNSUPPORTED;
        }
        state->clip_bounds = f3;
        state->workspace.clip_left = (renderer_u32)f3.left;
        state->workspace.clip_top = (renderer_u32)f3.top;
        result_event.kind = RENDERER_AC60_EVENT_F3_CLIP;
        result = RENDERER_AC60_APPLY_OK;
    } else if (result_event.command == 0x01U) {
        state_status = hercules_state_decode_command01(wire, length, &move);
        if (state_status != HERCULES_STATE_OK) {
            return (state_status == HERCULES_STATE_TRUNCATED)
                ? RENDERER_AC60_APPLY_TRUNCATED : RENDERER_AC60_APPLY_UNSUPPORTED;
        }
        result_event.deferred_copy = move;
        result_event.kind = RENDERER_AC60_EVENT_DEFERRED_COPY;
        result = RENDERER_AC60_APPLY_DEFERRED;
    } else {
        return decode_status(packet_status);
    }

    ++state->dispatch_sequence;
    *event = result_event;
    return result;
}
