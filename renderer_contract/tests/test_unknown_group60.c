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

static void put32(renderer_u8 *p, renderer_u32 value)
{
    p[0] = (renderer_u8)value;
    p[1] = (renderer_u8)(value >> 8);
    p[2] = (renderer_u8)(value >> 16);
    p[3] = (renderer_u8)(value >> 24);
}

static void set_header(renderer_u8 *p, renderer_u8 tag)
{
    memset(p, 0, 8U);
    put32(p, 0x10203040UL);
    p[7] = tag;
}

static void initialize(RendererAc60StateView *state, renderer_u32 gate_word)
{
    memset(state, 0, sizeof(*state));
    state->page.page_selector = 0x0bU;
    state->workspace.texture_page_address = 0x11223344UL;
    state->raster_suppression_flag = (renderer_u8)gate_word;
    state->dispatch_sequence = 7U;
}

static void test_unknown_group60(renderer_u8 tag, renderer_u32 gate_word)
{
    renderer_u8 wire[8];
    RendererAc60StateView state;
    RendererAc60StateView expected;
    RendererAc60Event event;
    RendererAc60ApplyStatus status;
    renderer_u32 expected_base;

    set_header(wire, tag);
    initialize(&state, gate_word);
    expected = state;
    memset(&event, 0, sizeof(event));
    status = renderer_ac60_apply_packet(wire, sizeof(wire), &state, &event);
    expected_base = ((renderer_u8)gate_word == 0U) ? 0x009be6e0UL : 0x11223344UL;
    expected.workspace.texture_page_address = expected_base;
    expected.dispatch_sequence += 1U;

    expect(status == RENDERER_AC60_APPLY_UNRECOGNIZED,
           "unknown group60 does not report draw success");
    expect(event.kind == RENDERER_AC60_EVENT_UNRECOGNIZED_GROUP60,
           "unknown group60 publishes explicit state-effect event");
    expect(event.packet_kind == RENDERER_PACKET_UNKNOWN,
           "unknown group60 packet layout remains unknown");
    expect(event.command == tag && event.semitransparent == ((tag & 2U) != 0),
           "event retains tag and bit-one metadata");
    expect(memcmp(&state, &expected, sizeof(state)) == 0,
           "only the observed conditional page refresh and tool sequence change");
}

static void test_known_and_non_group_routes(void)
{
    renderer_u8 wire[0x14];
    renderer_u8 short_wire[7];
    RendererAc60StateView state;
    RendererAc60StateView before;
    RendererAc60Event event;
    RendererAc60Event before_event;
    RendererAc60ApplyStatus status;

    memset(wire, 0, sizeof(wire));
    set_header(wire, 0x68U);
    initialize(&state, 0U);
    status = renderer_ac60_apply_packet(wire, 8U, &state, &event);
    expect(status == RENDERER_AC60_APPLY_OK &&
           event.kind == RENDERER_AC60_EVENT_IGNORED_RASTER &&
           event.packet_kind == RENDERER_PACKET_OBSERVED_UNDRAWN,
           "observed 0x68 keeps its known ignored-raster route");
    expect(state.workspace.texture_page_address == 0x009be6e0UL,
           "0x68 retains its established page refresh");

    set_header(wire, 0x64U);
    wire[0x0c] = 3U;
    wire[0x0d] = 5U;
    wire[0x10] = 8U;
    wire[0x12] = 9U;
    initialize(&state, 0U);
    status = renderer_ac60_apply_packet(wire, sizeof(wire), &state, &event);
    expect(status == RENDERER_AC60_APPLY_OK &&
           event.kind == RENDERER_AC60_EVENT_VARIABLE_SPRITE,
           "recognized sprite remains on its established route");
    expect(state.workspace.texture_page_address == 0x009be6e0UL,
           "recognized sprite stages its page once without new preamble behavior");

    set_header(wire, 0x55U);
    initialize(&state, 0U);
    before = state;
    memset(&event, 0x6d, sizeof(event));
    before_event = event;
    status = renderer_ac60_apply_packet(wire, 8U, &state, &event);
    expect(status == RENDERER_AC60_APPLY_UNRECOGNIZED &&
           memcmp(&state, &before, sizeof(state)) == 0 &&
           memcmp(&event, &before_event, sizeof(event)) == 0,
           "non-group60 unknown keeps its prior unrecognized/no-event contract");

    set_header(wire, 0x60U);
    initialize(&state, 0U);
    before = state;
    memset(&event, 0x6d, sizeof(event));
    before_event = event;
    memcpy(short_wire, wire, sizeof(short_wire));
    status = renderer_ac60_apply_packet(short_wire, sizeof(short_wire), &state, &event);
    expect(status == RENDERER_AC60_APPLY_TRUNCATED &&
           memcmp(&state, &before, sizeof(state)) == 0 &&
           memcmp(&event, &before_event, sizeof(event)) == 0,
           "short group60 input remains truncated with no published event");
}

int main(void)
{
    static const renderer_u8 tags[3] = { 0x60U, 0x62U, 0x70U };
    static const renderer_u32 gates[3] = { 0U, 1U, 0x100U };
    unsigned i;
    unsigned j;
    for (i = 0; i < 3U; ++i)
        for (j = 0; j < 3U; ++j)
            test_unknown_group60(tags[i], gates[j]);
    test_known_and_non_group_routes();
    printf("CHECKS;%u;%u\n", checks, failures);
    return failures != 0U;
}
