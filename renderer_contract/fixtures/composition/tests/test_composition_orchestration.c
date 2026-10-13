#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "composition_orchestration.h"

#define MEMORY_BASE 0x10000000UL
#define MEMORY_SIZE 0x4000U
#define OT_BASE (MEMORY_BASE + 0x100U)
#define E1_CURRENT (MEMORY_BASE + 0x1000U)
#define CMD01 (MEMORY_BASE + 0x1020U)
#define E1_FUTURE (MEMORY_BASE + 0x1040U)
#define OT_GATE (MEMORY_BASE + 0x1060U)
#define E1_ALT (MEMORY_BASE + 0x1080U)
#define CALLBACK_TOKEN 0x31000000UL
#define PIXEL_TOKEN 0x34000000UL
#define DIB_INFO_TOKEN 0x35000000UL
#define HDC_TOKEN 0x36000000UL

typedef struct ProviderState {
    renderer_u8 *memory;
    renderer_u32 calls[16];
    renderer_u32 call_count;
    renderer_u32 callback_count;
    renderer_u32 packet_mutation_count;
    renderer_u32 bad_call;
} ProviderState;

static void put16(renderer_u8 *bytes, renderer_u32 offset, renderer_u16 value)
{
    bytes[offset] = (renderer_u8)value;
    bytes[offset + 1U] = (renderer_u8)(value >> 8);
}

static void put32(renderer_u8 *bytes, renderer_u32 offset, renderer_u32 value)
{
    bytes[offset] = (renderer_u8)value;
    bytes[offset + 1U] = (renderer_u8)(value >> 8);
    bytes[offset + 2U] = (renderer_u8)(value >> 16);
    bytes[offset + 3U] = (renderer_u8)(value >> 24);
}

static renderer_u32 get32(const renderer_u8 *bytes, renderer_u32 offset)
{
    return (renderer_u32)bytes[offset] |
           ((renderer_u32)bytes[offset + 1U] << 8) |
           ((renderer_u32)bytes[offset + 2U] << 16) |
           ((renderer_u32)bytes[offset + 3U] << 24);
}

static void packet_header(renderer_u8 *memory, renderer_u32 address,
                          renderer_u32 next, renderer_u8 tag)
{
    renderer_u32 offset = address - MEMORY_BASE;
    memset(memory + offset, 0, 0x18U);
    put32(memory, offset, next);
    memory[offset + 7U] = tag;
}

static int provider_call(void *opaque, const CompositionProviderCall *call)
{
    ProviderState *state = (ProviderState *)opaque;
    renderer_u32 offset;
    renderer_u32 index;
    int valid = 1;

    if (state == 0 || call == 0 || state->call_count >= 16U)
        return 0;
    state->calls[state->call_count++] = (renderer_u32)call->service;
    if (call->service == COMPOSITION_SERVICE_LOCK) {
        valid = call->event.kind == COMPOSITION_EVENT_SURFACE_LOCK &&
                call->event.source_rva == 0x6FF0U &&
                call->event.raw_token == PIXEL_TOKEN &&
                call->event.argument[0] == 320U &&
                call->argument_count == 0U;
    } else if (call->service == COMPOSITION_SERVICE_MOVE_REQUEST) {
        valid = call->event.kind == COMPOSITION_EVENT_VRAM_MOVE_REQUEST &&
                call->event.source_rva == 0xA8F0U &&
                call->event.raw_token == CMD01 + 0x10U &&
                call->argument_count == 6U &&
                call->argument[0] == 15U && call->argument[1] == 240U &&
                call->argument[2] == 5U && call->argument[3] == 224U &&
                call->argument[4] == 8U && call->argument[5] == 0U;
    } else if (call->service == COMPOSITION_SERVICE_CALLBACK) {
        ++state->callback_count;
        valid = call->event.kind == COMPOSITION_EVENT_CALLBACK_SERVICE &&
                call->event.raw_token == CALLBACK_TOKEN &&
                call->argument_count == 4U;
        if (valid && call->argument[0] == 0x40ABE9U) {
            offset = CMD01 - MEMORY_BASE;
            valid = state->callback_count == 1U &&
                    call->argument[1] == 0U &&
                    call->argument[2] == CMD01 &&
                    call->argument[3] == OT_BASE + 4U &&
                    call->event.argument[1] == CMD01 &&
                    call->event.argument[2] == E1_FUTURE &&
                    call->event.argument[3] == E1_ALT &&
                    get32(state->memory, offset) == E1_FUTURE;
            if (valid) {
                put32(state->memory, offset, E1_ALT);
                ++state->packet_mutation_count;
            }
        } else if (valid && call->argument[0] == 0x40B2E7U) {
            valid = state->callback_count == 2U &&
                    call->argument[1] == 0U &&
                    call->argument[2] == 0U &&
                    call->argument[3] == 0U &&
                    call->event.argument[1] == 0U &&
                    call->event.argument[2] == 0U &&
                    call->event.argument[3] == 0U;
        } else {
            valid = 0;
        }
    } else if (call->service == COMPOSITION_SERVICE_REMAP) {
        valid = call->event.kind == COMPOSITION_EVENT_SURFACE_REMAP_REQUEST &&
                call->event.source_rva == 0x70C0U &&
                call->event.raw_token == PIXEL_TOKEN &&
                call->event.argument[0] == 0xFFU &&
                call->argument_count == 1U && call->argument[0] == 0xFFU;
    } else if (call->service == COMPOSITION_SERVICE_UNLOCK) {
        valid = call->event.kind == COMPOSITION_EVENT_SURFACE_UNLOCK &&
                call->event.source_rva == 0x7140U &&
                call->event.raw_token == 0U &&
                call->argument_count == 0U;
    } else if (call->service == COMPOSITION_SERVICE_PRESENT_REQUEST) {
        valid = call->event.kind == COMPOSITION_EVENT_PRESENT_REQUEST &&
                call->event.source_rva == 0x6620U &&
                call->event.argument[0] == 1U &&
                call->argument_count == 0U;
    } else if (call->service == COMPOSITION_SERVICE_STRETCH_DIBITS) {
        static const renderer_u32 expected[13] = {
            HDC_TOKEN, 0U, 0U, 320U, 240U, 0U, 0U,
            320U, 240U, PIXEL_TOKEN, DIB_INFO_TOKEN, 0U, 0x00CC0020U
        };
        valid = call->event.kind == COMPOSITION_EVENT_FINAL_OUTPUT &&
                call->event.raw_token == HDC_TOKEN &&
                call->event.argument[0] == 0x66A6U &&
                call->argument_count == 13U;
        for (index = 0; valid && index < 13U; ++index)
            if (call->argument[index] != expected[index])
                valid = 0;
    } else if (call->service == COMPOSITION_SERVICE_GDI_FLUSH) {
        valid = call->event.kind == COMPOSITION_EVENT_OUTPUT_FLUSH &&
                call->event.argument[0] == 0x66ACU &&
                call->argument_count == 0U;
    } else {
        valid = 0;
    }
    if (!valid)
        state->bad_call = call->service;
    return valid;
}

static void write_events(FILE *stream, const CompositionTrace *trace)
{
    renderer_u32 i;
    for (i = 0; i < trace->count; ++i) {
        const CompositionEvent *e = &trace->events[i];
        fprintf(stream,
            "%u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
            (unsigned)e->scope_id, (unsigned)e->local_order,
            (unsigned)e->source_order, (unsigned)e->kind,
            (unsigned)e->origin, (unsigned)e->domain,
            (unsigned)e->semantic_owner, (unsigned)e->source_rva,
            (unsigned)e->raw_token, (unsigned)e->argument[0],
            (unsigned)e->argument[1], (unsigned)e->argument[2],
            (unsigned)e->argument[3], (unsigned)e->flags);
    }
}

int main(int argc, char **argv)
{
    renderer_u8 memory[MEMORY_SIZE];
    FrameRaw32Region region;
    RendererAc60StateView state_view;
    CompositionEvent events[64];
    CompositionTrace trace;
    CompositionOrchestrationConfig config;
    ProviderState provider;
    FILE *output;
    CompositionTraceStatus status;
    static const renderer_u32 expected_services[] = {
        COMPOSITION_SERVICE_LOCK,
        COMPOSITION_SERVICE_MOVE_REQUEST,
        COMPOSITION_SERVICE_CALLBACK,
        COMPOSITION_SERVICE_REMAP,
        COMPOSITION_SERVICE_UNLOCK,
        COMPOSITION_SERVICE_PRESENT_REQUEST,
        COMPOSITION_SERVICE_STRETCH_DIBITS,
        COMPOSITION_SERVICE_GDI_FLUSH,
        COMPOSITION_SERVICE_CALLBACK
    };
    renderer_u32 i;

    if (argc != 2) {
        fprintf(stderr, "usage: test_composition_orchestration <events.tsv>\n");
        return 2;
    }
    memset(memory, 0, sizeof(memory));
    put32(memory, OT_BASE - MEMORY_BASE, E1_CURRENT);
    packet_header(memory, E1_CURRENT, CMD01, 0xE1U);
    put32(memory, E1_CURRENT - MEMORY_BASE + 4U, 0xE100017BU);
    packet_header(memory, CMD01, E1_FUTURE, 0x01U);
    put16(memory, CMD01 - MEMORY_BASE + 0x0CU, 15U);
    put16(memory, CMD01 - MEMORY_BASE + 0x0EU, 240U);
    put16(memory, CMD01 - MEMORY_BASE + 0x10U, 5U);
    put16(memory, CMD01 - MEMORY_BASE + 0x12U, 224U);
    put16(memory, CMD01 - MEMORY_BASE + 0x14U, 8U);
    put16(memory, CMD01 - MEMORY_BASE + 0x16U, 0U);
    packet_header(memory, E1_FUTURE, OT_GATE, 0xE1U);
    put32(memory, E1_FUTURE - MEMORY_BASE + 4U, 0xE10001BBUL);
    packet_header(memory, OT_GATE, 0U, 0x40U);
    packet_header(memory, E1_ALT, OT_GATE, 0xE1U);
    put32(memory, E1_ALT - MEMORY_BASE + 4U, 0xE1000052UL);

    region.base = MEMORY_BASE;
    region.size = MEMORY_SIZE;
    region.bytes = memory;
    memset(&state_view, 0, sizeof(state_view));
    memset(&provider, 0, sizeof(provider));
    provider.memory = memory;
    memset(&config, 0, sizeof(config));
    config.resolver = frame_raw32_region_resolve;
    config.resolver_context = &region;
    config.order_table_base = OT_BASE;
    config.order_table_slot_count = 1U;
    config.state = &state_view;
    config.callback_endpoint = CALLBACK_TOKEN;
    config.cpu_pixel_token = PIXEL_TOKEN;
    config.dib_info_token = DIB_INFO_TOKEN;
    config.window_hdc_token = HDC_TOKEN;
    config.cpu_pitch_bytes = 320U;
    config.dirty_present_gate = 1U;
    config.rewrite_packet_token = CMD01;
    config.replacement_link = E1_ALT;
    config.provider = provider_call;
    config.provider_context = &provider;
    composition_trace_init(&trace, events, 64U);
    status = composition_run_state_only_frame(&config, &trace);
    if (status != COMPOSITION_TRACE_OK || trace.count != 23U ||
        provider.bad_call != 0U || provider.call_count != 9U ||
        provider.callback_count != 2U || provider.packet_mutation_count != 1U ||
        get32(memory, CMD01 - MEMORY_BASE) != E1_ALT ||
        state_view.page.page_selector != 0x12U ||
        state_view.page.page_mode != 2U ||
        state_view.page.alternate_bank != 0U) {
        fprintf(stderr, "bounded C orchestration failed status=%u events=%u provider=%u\n",
                (unsigned)status, (unsigned)trace.count,
                (unsigned)provider.bad_call);
        return 1;
    }
    for (i = 0; i < sizeof(expected_services) / sizeof(expected_services[0]); ++i) {
        if (provider.calls[i] != expected_services[i]) {
            fprintf(stderr, "controlled service order mismatch at %u\n",
                    (unsigned)i);
            return 1;
        }
    }
    output = fopen(argv[1], "w");
    if (output == 0) {
        fprintf(stderr, "cannot create event stream\n");
        return 2;
    }
    write_events(output, &trace);
    if (fclose(output) != 0) {
        fprintf(stderr, "cannot finish event stream\n");
        return 2;
    }

    /* A raster packet is deliberately outside this reconstructed subset. It
     * must poison the trace instead of becoming a successful skipped draw. */
    put32(memory, CMD01 - MEMORY_BASE, E1_FUTURE);
    memory[E1_ALT - MEMORY_BASE + 7U] = 0x24U;
    memset(&state_view, 0, sizeof(state_view));
    memset(&provider, 0, sizeof(provider));
    provider.memory = memory;
    config.provider_context = &provider;
    composition_trace_init(&trace, events, 64U);
    status = composition_run_state_only_frame(&config, &trace);
    if (status != COMPOSITION_TRACE_INCOMPLETE_UNKNOWN || trace.count == 0U ||
        events[trace.count - 1U].kind != COMPOSITION_EVENT_UNKNOWN_WRITER) {
        fprintf(stderr, "unsupported raster packet did not fail closed: status=%u\n",
                (unsigned)status);
        return 1;
    }
    config.provider = 0;
    composition_trace_init(&trace, events, 64U);
    status = composition_run_state_only_frame(&config, &trace);
    if (status != COMPOSITION_TRACE_INVALID) {
        fprintf(stderr, "missing controlled providers were not rejected: status=%u\n",
                (unsigned)status);
        return 1;
    }

    printf("status=reconstructed_state_subset_pass events=23 providers=9 callbacks=2 mutation=%08x raster=explicit-incomplete-unknown\n",
           (unsigned)get32(memory, CMD01 - MEMORY_BASE));
    return 0;
}
