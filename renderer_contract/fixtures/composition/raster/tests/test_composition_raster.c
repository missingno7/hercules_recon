#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "composition_raster.h"

#define MEMORY_BASE 0x10000000UL
#define MEMORY_SIZE 0x4000UL
#define OT_BASE (MEMORY_BASE + 0x100UL)
#define FT4_PACKET (MEMORY_BASE + 0x1000UL)
#define TERMINATOR (MEMORY_BASE + 0x1040UL)
#define TEXTURE_TOKEN 0x0096E6E0UL
#define SURFACE_TOKEN 0x34000000UL
#define DIB_TOKEN 0x35000000UL
#define HDC_TOKEN 0x36000000UL
#define SURFACE_LENGTH 0x14000UL
#define TEXTURE_LENGTH 0x10000UL

typedef struct ProviderState {
    renderer_u32 calls[8];
    renderer_u32 call_count;
    renderer_u32 bad_call;
} ProviderState;

typedef struct FixtureMemory {
    renderer_u8 bytes[MEMORY_SIZE];
} FixtureMemory;

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

static int read_region(void *opaque, renderer_raw32 address,
                       renderer_u32 byte_count,
                       const renderer_u8 **resolved_bytes)
{
    FixtureMemory *memory = (FixtureMemory *)opaque;
    renderer_u32 offset;
    if (memory == 0 || resolved_bytes == 0 || address < MEMORY_BASE)
        return 0;
    offset = address - MEMORY_BASE;
    if (offset > MEMORY_SIZE || byte_count > MEMORY_SIZE - offset)
        return 0;
    *resolved_bytes = memory->bytes + offset;
    return 1;
}

static void make_ft4(FixtureMemory *memory, renderer_i16 x, renderer_i16 y,
                     renderer_u16 width, renderer_u16 height,
                     renderer_u8 u, renderer_u8 v,
                     int bad_shape, int asymmetric_uv)
{
    renderer_u32 offset = FT4_PACKET - MEMORY_BASE;
    renderer_u32 end_offset = TERMINATOR - MEMORY_BASE;
    renderer_i16 x0 = (renderer_i16)(x + (bad_shape ? 1 : 0));
    renderer_u8 u2 = (renderer_u8)(u + (asymmetric_uv ? 1U : 0U));
    memset(memory, 0, sizeof(*memory));
    put32(memory->bytes, OT_BASE - MEMORY_BASE, FT4_PACKET);
    memset(memory->bytes + offset, 0xA5, 0x28U);
    put32(memory->bytes, offset, TERMINATOR);
    memory->bytes[offset + 4U] = 0x64U;
    memory->bytes[offset + 5U] = 0x75U;
    memory->bytes[offset + 6U] = 0x86U;
    memory->bytes[offset + 7U] = 0x2CU;
    put16(memory->bytes, offset + 0x08U, (renderer_u16)x0);
    put16(memory->bytes, offset + 0x0AU, (renderer_u16)y);
    memory->bytes[offset + 0x0CU] = u;
    memory->bytes[offset + 0x0DU] = v;
    put16(memory->bytes, offset + 0x0EU, 0U);
    put16(memory->bytes, offset + 0x10U, (renderer_u16)(x + width));
    put16(memory->bytes, offset + 0x12U, (renderer_u16)y);
    memory->bytes[offset + 0x14U] = (renderer_u8)(u + width);
    memory->bytes[offset + 0x15U] = v;
    put16(memory->bytes, offset + 0x16U, 0U);
    put16(memory->bytes, offset + 0x18U, (renderer_u16)x);
    put16(memory->bytes, offset + 0x1AU, (renderer_u16)(y + height));
    memory->bytes[offset + 0x1CU] = u2;
    memory->bytes[offset + 0x1DU] = (renderer_u8)(v + height);
    put16(memory->bytes, offset + 0x1EU, 0U);
    put16(memory->bytes, offset + 0x20U, (renderer_u16)(x + width));
    put16(memory->bytes, offset + 0x22U, (renderer_u16)(y + height));
    memory->bytes[offset + 0x24U] = (renderer_u8)(u + width);
    memory->bytes[offset + 0x25U] = (renderer_u8)(v + height);
    put16(memory->bytes, offset + 0x26U, 0U);
    memset(memory->bytes + end_offset, 0xA5, 8U);
    put32(memory->bytes, end_offset, 0U);
    memory->bytes[end_offset + 7U] = 0x40U;
}

static void fill_texture(renderer_u8 *texture, int offset_case)
{
    memset(texture, 0, TEXTURE_LENGTH);
    if (offset_case) {
        texture[0x102U] = 0x11U; texture[0x103U] = 0x22U;
        texture[0x104U] = 0x33U; texture[0x202U] = 0x44U;
        texture[0x203U] = 0x55U; texture[0x204U] = 0x66U;
    } else {
        texture[1U] = 0x5AU; texture[0x100U] = 0x77U;
        texture[0x101U] = 0x6BU;
    }
}

static void setup_config(CompositionRasterConfig *config,
                         FixtureMemory *memory, RendererAc60StateView *state,
                         renderer_u8 *texture, renderer_u8 *surface,
                         ProviderState *provider)
{
    memset(config, 0, sizeof(*config));
    memset(state, 0, sizeof(*state));
    memset(provider, 0, sizeof(*provider));
    config->resolver = read_region;
    config->resolver_context = memory;
    config->order_table_base = OT_BASE;
    config->order_table_slot_count = 1U;
    config->state = state;
    config->texture_bytes = texture;
    config->texture_length = TEXTURE_LENGTH;
    config->surface_bytes = surface;
    config->surface_length = SURFACE_LENGTH;
    config->texture_token = TEXTURE_TOKEN;
    config->surface_token = SURFACE_TOKEN;
    config->dib_info_token = DIB_TOKEN;
    config->hdc_token = HDC_TOKEN;
    config->surface_pitch = 320U;
    config->dirty_gate = 1U;
    config->remap_guard = 0x80U;
    config->provider = 0; /* installed after the provider declaration */
    config->provider_context = provider;
}

static int raster_provider(void *opaque,
                           const CompositionRasterProviderCall *call)
{
    ProviderState *state = (ProviderState *)opaque;
    static const renderer_u32 expected_order[] = { 1U, 2U, 3U, 4U, 5U, 6U };
    renderer_u32 expected[13];
    renderer_u32 index;
    int valid = 1;

    if (state == 0 || call == 0 || state->call_count >= 8U)
        return 0;
    state->calls[state->call_count] = (renderer_u32)call->service;
    if (state->call_count >= 6U ||
        state->calls[state->call_count] != expected_order[state->call_count])
        valid = 0;
    if (call->service == COMPOSITION_RASTER_SERVICE_LOCK) {
        valid = valid && call->event.kind == COMPOSITION_EVENT_SURFACE_LOCK &&
                call->event.source_rva == 0x6FF0U &&
                call->event.raw_token == SURFACE_TOKEN &&
                call->argument_count == 3U && call->argument[0] == 320U &&
                call->argument[1] == 240U && call->argument[2] == SURFACE_TOKEN;
    } else if (call->service == COMPOSITION_RASTER_SERVICE_REMAP) {
        valid = valid && call->event.kind == COMPOSITION_EVENT_SURFACE_REMAP_REQUEST &&
                call->event.source_rva == 0x70C0U &&
                call->event.raw_token == SURFACE_TOKEN &&
                call->event.argument[0] == 0xFFU &&
                call->event.argument[1] == 0x80U &&
                call->argument_count == 1U && call->argument[0] == 0xFFU;
    } else if (call->service == COMPOSITION_RASTER_SERVICE_UNLOCK) {
        valid = valid && call->event.kind == COMPOSITION_EVENT_SURFACE_UNLOCK &&
                call->event.source_rva == 0x7140U &&
                call->argument_count == 0U;
    } else if (call->service == COMPOSITION_RASTER_SERVICE_PRESENT) {
        valid = valid && call->event.kind == COMPOSITION_EVENT_PRESENT_REQUEST &&
                call->event.source_rva == 0x6620U &&
                call->event.argument[0] == 1U &&
                call->argument_count == 1U && call->argument[0] == 1U;
    } else if (call->service == COMPOSITION_RASTER_SERVICE_STRETCH_DIBITS) {
        expected[0] = HDC_TOKEN; expected[1] = 0U; expected[2] = 0U;
        expected[3] = 320U; expected[4] = 240U; expected[5] = 0U;
        expected[6] = 0U; expected[7] = 320U; expected[8] = 240U;
        expected[9] = SURFACE_TOKEN; expected[10] = DIB_TOKEN;
        expected[11] = 0U; expected[12] = 0x00CC0020UL;
        valid = valid && call->event.kind == COMPOSITION_EVENT_FINAL_OUTPUT &&
                call->event.raw_token == HDC_TOKEN &&
                call->argument_count == 13U && call->argument[0] == HDC_TOKEN;
        for (index = 0; valid && index < 13U; ++index)
            if (call->argument[index] != expected[index])
                valid = 0;
    } else if (call->service == COMPOSITION_RASTER_SERVICE_GDI_FLUSH) {
        valid = valid && call->event.kind == COMPOSITION_EVENT_OUTPUT_FLUSH &&
                call->event.argument[0] == 0x66ACU &&
                call->argument_count == 0U;
    } else {
        valid = 0;
    }
    ++state->call_count;
    if (!valid)
        state->bad_call = (renderer_u32)call->service;
    return valid;
}

static int write_trace(FILE *stream, const CompositionTrace *trace)
{
    renderer_u32 i;
    if (fprintf(stream, "status %u\n", (unsigned)trace->status) < 0)
        return 0;
    for (i = 0; i < trace->count; ++i) {
        const CompositionEvent *event = &trace->events[i];
        if (fprintf(stream, "%u %u %u %u %u %u %u %u %u %u %u\n",
                    (unsigned)event->local_order, (unsigned)event->kind,
                    (unsigned)event->source_rva, (unsigned)event->raw_token,
                    (unsigned)event->argument[0], (unsigned)event->argument[1],
                    (unsigned)event->argument[2], (unsigned)event->argument[3],
                    (unsigned)event->origin, (unsigned)event->domain,
                    (unsigned)event->semantic_owner) < 0)
            return 0;
    }
    return 1;
}

static renderer_u32 write_final_pixels(FILE *stream,
                                       const renderer_u8 *before,
                                       const renderer_u8 *after)
{
    renderer_u32 index;
    renderer_u32 changed = 0;
    for (index = 0; index < SURFACE_LENGTH; ++index) {
        if (before[index] != after[index]) {
            if (fprintf(stream, "%u %u %u\n", (unsigned)index,
                        (unsigned)before[index], (unsigned)after[index]) < 0)
                return 0xFFFFFFFFUL;
            ++changed;
        }
    }
    return changed;
}

int main(int argc, char **argv)
{
    FixtureMemory memory;
    renderer_u8 texture[TEXTURE_LENGTH];
    renderer_u8 surface[SURFACE_LENGTH];
    renderer_u8 before[SURFACE_LENGTH];
    RendererAc60StateView state;
    CompositionRasterConfig config;
    CompositionEvent event_storage[80];
    CompositionTrace trace;
    ProviderState provider;
    FILE *events_file;
    FILE *pixels_file;
    FILE *raw_file;
    CompositionRasterStatus status;
    renderer_u32 changed;
    renderer_i16 x = 1;
    renderer_i16 y = 1;
    renderer_u16 width = 2U;
    renderer_u16 height = 2U;
    renderer_u8 u = 0U;
    renderer_u8 v = 0U;
    int offset_case = 0;
    renderer_u32 expected_pixels = 0U;
    renderer_u32 row;
    renderer_u32 column;
    renderer_u32 tail_index;

    if (argc != 4 && argc != 5) {
        fprintf(stderr, "usage: test_composition_raster <events.tsv> <pixels.tsv> <raw-inputs.bin> [base|offset]\n");
        return 2;
    }
    if (argc == 5) {
        if (strcmp(argv[4], "offset") != 0) {
            fprintf(stderr, "unknown case: %s\n", argv[4]);
            return 2;
        }
        offset_case = 1;
        x = 4; y = 5; width = 3U; height = 2U; u = 2U; v = 1U;
    }
    make_ft4(&memory, x, y, width, height, u, v, 0, 0);
    fill_texture(texture, offset_case);
    memset(surface, 0xA5, sizeof(surface));
    memcpy(before, surface, sizeof(surface));
    setup_config(&config, &memory, &state, texture, surface, &provider);
    config.provider = raster_provider;
    composition_trace_init(&trace, event_storage, 80U);
    raw_file = fopen(argv[3], "wb");
    if (raw_file == 0) {
        fprintf(stderr, "cannot open raw input output file\n");
        return 1;
    }
    if (fwrite(memory.bytes + (OT_BASE - MEMORY_BASE), 1U, 4U, raw_file) != 4U ||
        fwrite(memory.bytes + (FT4_PACKET - MEMORY_BASE), 1U, 0x28U, raw_file) != 0x28U ||
        fwrite(memory.bytes + (TERMINATOR - MEMORY_BASE), 1U, 8U, raw_file) != 8U) {
        fclose(raw_file);
        fprintf(stderr, "failed to write raw input bytes\n");
        return 1;
    }
    fclose(raw_file);
    status = composition_run_ft4_direct_copy_frame(&config, &trace);
    for (row = 0; row < height; ++row) {
        for (column = 0; column < width; ++column) {
            renderer_u32 source = ((renderer_u32)v + row) * 0x100U +
                                  (renderer_u32)u + column;
            renderer_u32 destination = ((renderer_u32)y + row) * 320U +
                                      (renderer_u32)x + column;
            if (texture[source] != 0U) {
                if (surface[destination] != texture[source])
                    return 1;
                ++expected_pixels;
            }
        }
    }
    tail_index = 4U + expected_pixels;
    if (status != COMPOSITION_RASTER_OK ||
        trace.count != 10U + expected_pixels ||
        provider.bad_call != 0U || provider.call_count != 6U ||
        trace.events[3].argument[0] != (renderer_u32)x ||
        trace.events[3].argument[1] != (renderer_u32)y ||
        trace.events[3].argument[2] != width ||
        trace.events[3].argument[3] != height ||
        trace.events[tail_index].kind != COMPOSITION_EVENT_PACKET_CONSUMED ||
        trace.events[tail_index + 1U].kind != COMPOSITION_EVENT_SURFACE_REMAP_REQUEST ||
        trace.events[tail_index + 2U].kind != COMPOSITION_EVENT_SURFACE_UNLOCK ||
        trace.events[tail_index + 3U].kind != COMPOSITION_EVENT_PRESENT_REQUEST ||
        trace.events[tail_index + 4U].kind != COMPOSITION_EVENT_FINAL_OUTPUT ||
        trace.events[tail_index + 5U].kind != COMPOSITION_EVENT_OUTPUT_FLUSH) {
        fprintf(stderr, "positive connected raster frame failed status=%u count=%u provider=%u\n",
                (unsigned)status, (unsigned)trace.count, (unsigned)provider.bad_call);
        return 1;
    }
    events_file = fopen(argv[1], "w");
    pixels_file = fopen(argv[2], "w");
    if (events_file == 0 || pixels_file == 0) {
        fprintf(stderr, "cannot open output files\n");
        if (events_file != 0) fclose(events_file);
        if (pixels_file != 0) fclose(pixels_file);
        return 1;
    }
    changed = write_final_pixels(pixels_file, before, surface);
    if (changed == 0xFFFFFFFFUL || changed != expected_pixels || !write_trace(events_file, &trace)) {
        fclose(events_file); fclose(pixels_file);
        fprintf(stderr, "failed to write outputs or unexpected pixel count\n");
        return 1;
    }
    fclose(events_file); fclose(pixels_file);

    /* A near-shape FT4 remains outside this finite provider. It is rejected
     * before service calls or framebuffer mutation. */
    make_ft4(&memory, x, y, width, height, u, v, 1, 0);
    memset(surface, 0xA5, sizeof(surface));
    memcpy(before, surface, sizeof(surface));
    setup_config(&config, &memory, &state, texture, surface, &provider);
    config.provider = raster_provider;
    composition_trace_init(&trace, event_storage, 80U);
    status = composition_run_ft4_direct_copy_frame(&config, &trace);
    if (status != COMPOSITION_RASTER_UNSUPPORTED || trace.count != 0U ||
        provider.call_count != 0U || memcmp(surface, before, sizeof(surface)) != 0) {
        fprintf(stderr, "unsupported shape was not safely rejected status=%u count=%u\n",
                (unsigned)status, (unsigned)trace.count);
        return 1;
    }

    make_ft4(&memory, x, y, width, height, u, v, 0, 1);
    memset(surface, 0xA5, sizeof(surface));
    memcpy(before, surface, sizeof(surface));
    setup_config(&config, &memory, &state, texture, surface, &provider);
    config.provider = raster_provider;
    composition_trace_init(&trace, event_storage, 80U);
    status = composition_run_ft4_direct_copy_frame(&config, &trace);
    if (status != COMPOSITION_RASTER_UNSUPPORTED || trace.count != 0U ||
        provider.call_count != 0U || memcmp(surface, before, sizeof(surface)) != 0) {
        fprintf(stderr, "asymmetric-UV shape was not safely rejected status=%u count=%u\n",
                (unsigned)status, (unsigned)trace.count);
        return 1;
    }
    printf("PASS composition raster rectangle subset; case=%s rect=%ux%u pixel_writes=%u nonrectangle=reject asymmetric_uv=reject\n",
           offset_case ? "offset" : "base", (unsigned)width, (unsigned)height,
           (unsigned)expected_pixels);
    return 0;
}
