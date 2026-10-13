#include <stdio.h>
#include <string.h>
#include "hercules_staging.h"

static unsigned checks;
static unsigned failures;

static void expect(int condition, const char *name)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL:%s\n", name);
    }
}

static void put16(renderer_u8 *bytes, unsigned offset, renderer_u16 value)
{
    bytes[offset] = (renderer_u8)value;
    bytes[offset + 1U] = (renderer_u8)(value >> 8);
}

static void put32(renderer_u8 *bytes, unsigned offset, renderer_u32 value)
{
    bytes[offset] = (renderer_u8)value;
    bytes[offset + 1U] = (renderer_u8)(value >> 8);
    bytes[offset + 2U] = (renderer_u8)(value >> 16);
    bytes[offset + 3U] = (renderer_u8)(value >> 24);
}

static void header(renderer_u8 *bytes, size_t size, renderer_u32 next,
                   renderer_u8 command)
{
    memset(bytes, 0xa5, size);
    put32(bytes, 0, next);
    bytes[7] = command;
}

static void fill_ft3(renderer_u8 *p)
{
    header(p, 0x20U, 0x10001040UL, 0x26U);
    p[4] = 0x31U; p[5] = 0x42U; p[6] = 0x53U;
    put16(p, 0x08, (renderer_u16)-101); put16(p, 0x0a, 202);
    p[0x0c] = 17; p[0x0d] = 29; put16(p, 0x0e, 0x1234);
    put16(p, 0x10, 303); put16(p, 0x12, (renderer_u16)-404);
    p[0x14] = 43; p[0x15] = 71; put16(p, 0x16, 0x004b);
    put16(p, 0x18, 505); put16(p, 0x1a, 606);
    p[0x1c] = 101; p[0x1d] = 203;
}

static void fill_ft4(renderer_u8 *p)
{
    header(p, 0x28U, 0x10001080UL, 0x2cU);
    p[4] = 0x64U; p[5] = 0x75U; p[6] = 0x86U;
    put16(p, 0x08, (renderer_u16)-111); put16(p, 0x0a, 212);
    p[0x0c] = 19; p[0x0d] = 31; put16(p, 0x0e, 0x5678);
    put16(p, 0x10, 515); put16(p, 0x12, (renderer_u16)-616);
    p[0x14] = 47; p[0x15] = 73; put16(p, 0x16, 0x006d);
    put16(p, 0x18, 717); put16(p, 0x1a, (renderer_u16)-818);
    p[0x1c] = 103; p[0x1d] = 205;
    put16(p, 0x20, (renderer_u16)-919); put16(p, 0x22, 1018);
    p[0x24] = 109; p[0x25] = 211;
}

static void fill_gt4(renderer_u8 *p)
{
    header(p, 0x34U, 0x100010c0UL, 0x3eU);
    p[4] = 0x97U; p[5] = 0xa8U; p[6] = 0xb9U;
    put16(p, 0x08, (renderer_u16)-121); put16(p, 0x0a, 222);
    p[0x0c] = 23; p[0x0d] = 37; put16(p, 0x0e, 0x9abc);
    p[0x10] = 0xc1; p[0x11] = 0xc2; p[0x12] = 0xc3;
    put16(p, 0x14, 525); put16(p, 0x16, (renderer_u16)-626);
    p[0x18] = 53; p[0x19] = 79; put16(p, 0x1a, 0x01bb);
    p[0x1c] = 0xc4; p[0x1d] = 0xc5; p[0x1e] = 0xc6;
    put16(p, 0x20, 737); put16(p, 0x22, (renderer_u16)-838);
    p[0x24] = 107; p[0x25] = 213;
    p[0x28] = 0xc7; p[0x29] = 0xc8; p[0x2a] = 0xc9;
    put16(p, 0x2c, (renderer_u16)-949); put16(p, 0x2e, 1050);
    p[0x30] = 113; p[0x31] = 217;
}

static void fill_e1(renderer_u8 *p)
{
    header(p, 8U, 0x100010d0UL, 0xe1U);
    put32(p, 4, 0xe100017bUL);
}

static void fill_ignored68(renderer_u8 *p)
{
    header(p, 8U, 0x100010e0UL, 0x68U);
}

static void fill_f3(renderer_u8 *p)
{
    header(p, 12U, 0x10001110UL, 0xf3U);
    put16(p, 4, (renderer_u16)-7); p[6] = 0xf5U;
    put16(p, 8, 20); put16(p, 10, 300);
}

static void fill_move(renderer_u8 *p)
{
    header(p, 0x18U, 0x10001130UL, 0x01U);
    put16(p, 0x0c, (renderer_u16)-15); put16(p, 0x0e, 240);
    put16(p, 0x10, 300); put16(p, 0x12, 244);
    put16(p, 0x14, 17); put16(p, 0x16, 9);
}

static void fill_sprite(renderer_u8 *p)
{
    header(p, 0x18U, 0x10001170UL, 0x64U);
    p[4] = 0xc1; p[5] = 0xd2; p[6] = 0xe3;
    put16(p, 8, (renderer_u16)-131); put16(p, 10, 232);
    p[0x0c] = 127; p[0x0d] = 229;
    put16(p, 0x0e, 13); put16(p, 0x10, 13); put16(p, 0x12, 27);
}

static void fill_gate(renderer_u8 *p)
{
    header(p, 8U, 0, 0x40U);
}

static void fill_fixed_sprite(renderer_u8 *p, renderer_u8 command)
{
    header(p, 0x10U, 0, command);
    put16(p, 8U, (renderer_u16)-321);
    put16(p, 10U, 432U);
    p[0x0c] = 0x35U;
    p[0x0d] = 0xe7U;
}

static void print_state(const char *name, RendererAc60ApplyStatus status,
                        const RendererAc60Event *event,
                        const RendererAc60StateView *state)
{
    const RendererAc60WorkspaceView *w;
    const RendererAc60PageCells *page;
    const HerculesCommand01Move *move;

    w = &state->workspace;
    page = &state->page;
    move = &event->deferred_copy;
    printf("ROW;%s;%d;%d;%d;%u;%u;%u;",
           name, (int)status, (int)event->kind, (int)event->packet_kind,
           (unsigned)event->command, (unsigned)event->semitransparent,
           (unsigned)state->dispatch_sequence);
    printf("%u;%u;%u;%u;%u;%u;%u;%u;%u;%u;",
           w->clip_left, w->clip_top, w->native_x_slot0, w->native_x_slot1,
           w->native_x_slot2, w->native_x_slot3, w->native_y_slot0,
           w->native_y_slot1, w->native_y_slot2, w->native_y_slot3);
    printf("%u;%u;%u;%u;%u;%u;%u;%u;%u;%u;",
           w->native_u_slot0, w->native_u_slot1, w->native_u_slot2,
           w->native_u_slot3, w->native_v_slot0, w->native_v_slot1,
           w->native_v_slot2, w->native_v_slot3, w->vertex_count,
           w->texture_page_address);
    printf("%u;%u;%u;%u;%u;%u;%u;%u;%u;%u;%u;%u;",
           w->color0_red, w->clut_word, (unsigned)w->sprite_width,
           (unsigned)w->sprite_height, page->page_selector, page->page_mode,
           page->alternate_bank, (unsigned)state->clip_bounds.left,
           (unsigned)state->clip_bounds.top, (unsigned)state->clip_bounds.right,
           (unsigned)state->clip_bounds.bottom, (unsigned)move->destination_x);
    printf("%u;%u;%u;%u;%u\n", (unsigned)move->destination_y,
           (unsigned)move->source_x, (unsigned)move->source_y,
           (unsigned)move->width, (unsigned)move->height);
}

static void apply_row(const char *name, const renderer_u8 *bytes, size_t size,
                      RendererAc60StateView *state,
                      RendererAc60EventKind expected_kind,
                      RendererAc60ApplyStatus expected_status)
{
    RendererAc60Event event;
    RendererAc60ApplyStatus status;

    memset(&event, 0, sizeof(event));
    status = renderer_ac60_apply_packet(bytes, size, state, &event);
    expect(status == expected_status, "event status");
    expect(event.kind == expected_kind, "event kind");
    print_state(name, status, &event, state);
}

static void test_explicit_errors_and_immutability(void)
{
    renderer_u8 unknown[8];
    renderer_u8 short_ft3[8];
    RendererAc60StateView state;
    RendererAc60StateView before;
    RendererAc60Event event;

    memset(&state, 0x5a, sizeof(state));
    before = state;
    header(unknown, sizeof(unknown), 0, 0x55U);
    memset(&event, 0x3c, sizeof(event));
    expect(renderer_ac60_apply_packet(unknown, sizeof(unknown), &state, &event)
           == RENDERER_AC60_APPLY_UNRECOGNIZED,
           "unknown packet status explicit");
    expect(memcmp(&state, &before, sizeof(state)) == 0,
           "unknown packet leaves state unchanged");
    header(short_ft3, sizeof(short_ft3), 0, 0x24U);
    expect(renderer_ac60_apply_packet(short_ft3, sizeof(short_ft3), &state, &event)
           == RENDERER_AC60_APPLY_TRUNCATED,
           "known truncated packet status explicit");
    expect(memcmp(&state, &before, sizeof(state)) == 0,
           "truncated packet leaves state unchanged");
}

static void test_ignored_raster_refresh(void)
{
    renderer_u8 packet[8];
    RendererAc60StateView state;
    RendererAc60StateView before;
    RendererAc60Event event;
    RendererAc60ApplyStatus status;

    memset(&state, 0, sizeof(state));
    state.page.page_selector = 9U;
    state.page.page_mode = 2U;
    state.page.alternate_bank = 0U;
    state.workspace.texture_page_address = 0x11111111UL;
    state.workspace.native_x_slot0 = 0x22222222UL;
    state.workspace.clut_word = 0x33333333UL;
    state.dispatch_sequence = 4U;
    header(packet, sizeof(packet), 0, 0x68U);
    before = state;
    status = renderer_ac60_apply_packet(packet, sizeof(packet), &state, &event);
    expect(status == RENDERER_AC60_APPLY_OK, "0x68 ignored-raster status");
    expect(event.kind == RENDERER_AC60_EVENT_IGNORED_RASTER,
           "0x68 has explicit ignored-raster operation");
    expect(event.packet_kind == RENDERER_PACKET_OBSERVED_UNDRAWN,
           "0x68 payload remains unclassified and opaque");
    expect(state.workspace.texture_page_address == 0x009AE6E0UL,
           "0x68 refreshes A909FC from current A8E6F4/F8/FC page cells");
    expect(state.workspace.native_x_slot0 == before.workspace.native_x_slot0 &&
           state.workspace.clut_word == before.workspace.clut_word &&
           state.page.page_selector == before.page.page_selector &&
           state.page.page_mode == before.page.page_mode &&
           state.page.alternate_bank == before.page.alternate_bank,
           "0x68 preserves fields outside its page-base refresh");
    expect(state.dispatch_sequence == before.dispatch_sequence + 1U,
           "0x68 advances adapter dispatch sequence");

    state.raster_suppression_flag = 1U;
    state.workspace.texture_page_address = 0x44444444UL;
    before = state;
    status = renderer_ac60_apply_packet(packet, sizeof(packet), &state, &event);
    expect(status == RENDERER_AC60_APPLY_OK &&
           event.kind == RENDERER_AC60_EVENT_IGNORED_RASTER,
           "gated 0x68 remains an ignored-raster operation");
    expect(state.workspace.texture_page_address == before.workspace.texture_page_address,
           "native BA01FC gate suppresses 0x68 page refresh");
}

static void test_raster_suppression_byte_gate(void)
{
    renderer_u8 ft3[0x20];
    renderer_u8 ft4[0x28];
    renderer_u8 gt4[0x34];
    renderer_u8 variable[0x18];
    renderer_u8 sprite8[0x10];
    renderer_u8 sprite16[0x10];
    const renderer_u8 *packets[6];
    const size_t lengths[6] = { 0x20U, 0x28U, 0x34U, 0x18U, 0x10U, 0x10U };
    const RendererPacketKind kinds[6] = {
        RENDERER_PACKET_FT3, RENDERER_PACKET_FT4, RENDERER_PACKET_GT4,
        RENDERER_PACKET_SPRITE_VARIABLE, RENDERER_PACKET_SPRITE_8,
        RENDERER_PACKET_SPRITE_16
    };
    RendererAc60StateView state;
    RendererAc60StateView before;
    RendererAc60Event event;
    unsigned i;

    memset(&state, 0x39, sizeof(state));
    state.raster_suppression_flag = 1U;
    expect(sizeof(state.raster_suppression_flag) == 1U,
           "BA01FC gate is a one-byte state field");
    fill_ft3(ft3); fill_ft4(ft4); fill_gt4(gt4);
    fill_sprite(variable);
    fill_fixed_sprite(sprite8, 0x74U);
    fill_fixed_sprite(sprite16, 0x7cU);
    packets[0] = ft3; packets[1] = ft4; packets[2] = gt4;
    packets[3] = variable; packets[4] = sprite8; packets[5] = sprite16;

    for (i = 0; i < 6U; ++i) {
        before = state;
        before.dispatch_sequence += 1U;
        memset(&event, 0, sizeof(event));
        expect(renderer_ac60_apply_packet(packets[i], lengths[i], &state, &event)
               == RENDERER_AC60_APPLY_OK,
               "nonzero BA01FC keeps raster packet recognized");
        expect(event.kind == RENDERER_AC60_EVENT_RASTER_SUPPRESSED,
               "nonzero BA01FC suppresses packet staging/raster event");
        expect(event.packet_kind == kinds[i],
               "suppressed packet retains decoded kind");
        expect(memcmp(&state, &before, sizeof(state)) == 0,
               "suppressed polygon/sprite leaves every state field unchanged");
    }
}

static void test_non_raster_commands_continue_when_gated(void)
{
    renderer_u8 e1[8];
    renderer_u8 f3[12];
    renderer_u8 move[0x18];
    renderer_u8 gate[8];
    RendererAc60StateView state;
    RendererAc60Event event;

    memset(&state, 0, sizeof(state));
    state.raster_suppression_flag = 1U;
    fill_e1(e1); put32(e1, 4U, 0xe10001bbUL);
    fill_f3(f3); fill_move(move); fill_gate(gate);
    expect(renderer_ac60_apply_packet(e1, sizeof(e1), &state, &event)
           == RENDERER_AC60_APPLY_OK && event.kind == RENDERER_AC60_EVENT_E1_STATE,
           "E1 executes while BA01FC is nonzero");
    expect(state.page.page_selector == 0x1bU && state.page.page_mode == 1U &&
           state.page.alternate_bank == 1U,
           "gated E1 retains its three page-cell updates");
    expect(renderer_ac60_apply_packet(f3, sizeof(f3), &state, &event)
           == RENDERER_AC60_APPLY_OK && event.kind == RENDERER_AC60_EVENT_F3_CLIP,
           "F3 executes while BA01FC is nonzero");
    expect(state.clip_bounds.left == 0 && state.clip_bounds.top == 5,
           "gated F3 retains its clip-state update");
    expect(renderer_ac60_apply_packet(move, sizeof(move), &state, &event)
           == RENDERER_AC60_APPLY_DEFERRED &&
           event.kind == RENDERER_AC60_EVENT_DEFERRED_COPY,
           "command 01 remains a deferred operation while BA01FC is nonzero");
    expect(renderer_ac60_apply_packet(gate, sizeof(gate), &state, &event)
           == RENDERER_AC60_APPLY_OK && event.kind == RENDERER_AC60_EVENT_OT_GATE,
           "OT gate continues while BA01FC is nonzero");
}

static void emit_flag(const char *family, renderer_u32 bits,
                      const RendererAc60StateView *state,
                      renderer_u8 semitransparent)
{
    printf("FLAG;%s;%u;%u;%u;%u;%u;%u\n", family, (unsigned)bits,
           state->page.page_selector, state->page.page_mode,
           state->page.alternate_bank, state->workspace.texture_page_address,
           (unsigned)semitransparent);
}

static void test_discriminator_vectors(void)
{
    static const renderer_u16 values[3] = { 0x0080U, 0x0100U, 0x0180U };
    renderer_u8 packet[0x34];
    RendererAc60StateView state;
    RendererAc60Event event;
    RendererAc60ApplyStatus status;
    unsigned i;

    for (i = 0; i < 3U; ++i) {
        memset(&state, 0, sizeof(state));
        memset(packet, 0, sizeof(packet));
        header(packet, 0x28U, 0, 0x2cU);
        put16(packet, 0x16, values[i]);
        status = renderer_ac60_apply_packet(packet, 0x28U, &state, &event);
        expect(status == RENDERER_AC60_APPLY_OK, "FT4 bank-vector status");
        emit_flag("ft4", values[i], &state, event.semitransparent);
    }
    for (i = 0; i < 3U; ++i) {
        memset(&state, 0, sizeof(state));
        memset(packet, 0, sizeof(packet));
        header(packet, 0x34U, 0, 0x3eU);
        put16(packet, 0x1a, values[i]);
        status = renderer_ac60_apply_packet(packet, 0x34U, &state, &event);
        expect(status == RENDERER_AC60_APPLY_OK, "GT4 bank-vector status");
        emit_flag("gt4", values[i], &state, event.semitransparent);
    }
    for (i = 0; i < 3U; ++i) {
        memset(&state, 0, sizeof(state));
        memset(packet, 0, sizeof(packet));
        header(packet, 8U, 0, 0xe1U);
        put32(packet, 4, 0xe1000000UL | values[i]);
        status = renderer_ac60_apply_packet(packet, 8U, &state, &event);
        expect(status == RENDERER_AC60_APPLY_OK, "E1 bank-vector status");
        memset(packet, 0, sizeof(packet));
        header(packet, 0x14U, 0, 0x64U);
        status = renderer_ac60_apply_packet(packet, 0x14U, &state, &event);
        expect(status == RENDERER_AC60_APPLY_OK, "E1 sprite page-address status");
        emit_flag("e1", values[i], &state, event.semitransparent);
    }
}
int main(void)
{
    renderer_u8 ft3[0x20];
    renderer_u8 ft4[0x28];
    renderer_u8 gt4[0x34];
    renderer_u8 e1[8];
    renderer_u8 ignored68[8];
    renderer_u8 f3[12];
    renderer_u8 move[0x18];
    renderer_u8 sprite[0x18];
    renderer_u8 gate[8];
    RendererAc60StateView state;

    memset(&state, 0, sizeof(state));
    fill_ft3(ft3); fill_ft4(ft4); fill_gt4(gt4); fill_e1(e1);
    fill_ignored68(ignored68);
    fill_f3(f3); fill_move(move); fill_sprite(sprite); fill_gate(gate);
    apply_row("ft3", ft3, sizeof(ft3), &state, RENDERER_AC60_EVENT_FT3,
              RENDERER_AC60_APPLY_OK);
    apply_row("ft4", ft4, sizeof(ft4), &state, RENDERER_AC60_EVENT_FT4,
              RENDERER_AC60_APPLY_OK);
    apply_row("gt4", gt4, sizeof(gt4), &state, RENDERER_AC60_EVENT_GT4,
              RENDERER_AC60_APPLY_OK);
    apply_row("e1", e1, sizeof(e1), &state, RENDERER_AC60_EVENT_E1_STATE,
              RENDERER_AC60_APPLY_OK);
    apply_row("ignored68", ignored68, sizeof(ignored68), &state,
              RENDERER_AC60_EVENT_IGNORED_RASTER, RENDERER_AC60_APPLY_OK);
    apply_row("f3", f3, sizeof(f3), &state, RENDERER_AC60_EVENT_F3_CLIP,
              RENDERER_AC60_APPLY_OK);
    apply_row("move", move, sizeof(move), &state,
              RENDERER_AC60_EVENT_DEFERRED_COPY, RENDERER_AC60_APPLY_DEFERRED);
    apply_row("sprite", sprite, sizeof(sprite), &state,
              RENDERER_AC60_EVENT_VARIABLE_SPRITE, RENDERER_AC60_APPLY_OK);
    apply_row("ot_gate", gate, sizeof(gate), &state,
              RENDERER_AC60_EVENT_OT_GATE, RENDERER_AC60_APPLY_OK);
    expect(state.dispatch_sequence == 9U, "bounded recognized event sequence");
    test_explicit_errors_and_immutability();
    test_ignored_raster_refresh();
    test_raster_suppression_byte_gate();
    test_non_raster_commands_continue_when_gated();
    test_discriminator_vectors();
    printf("CHECKS;%u;%u\n", checks, failures);
    return failures != 0;
}
