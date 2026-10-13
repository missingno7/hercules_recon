#include "hercules_pixels.h"

#include <stdio.h>
#include <string.h>

#define EVENT_CAPACITY 16UL
#define TEST_BLEND_INDEX ((renderer_u32)0xA63CUL)

typedef struct PixelEventLog {
    renderer_u8 events[EVENT_CAPACITY];
    renderer_u32 count;
} PixelEventLog;

typedef struct PixelReaderContext {
    const renderer_u8 *bytes;
    renderer_u32 length;
    renderer_u32 reads;
    renderer_u32 last_offset;
    renderer_u8 event_id;
    int fail_read;
    PixelEventLog *log;
} PixelReaderContext;

typedef struct PixelDestinationContext {
    renderer_u8 value;
    renderer_u32 reads;
    renderer_u32 writes;
    renderer_u8 last_written;
    int fail_read;
    int fail_write;
    PixelEventLog *log;
} PixelDestinationContext;

static renderer_u8 shade_bytes[256];
static renderer_u8 palette_bytes[256];
static renderer_u8 blend_bytes[4][65536];
static PixelEventLog events;
static PixelReaderContext shade_context;
static PixelReaderContext palette_context;
static PixelReaderContext blend_contexts[4];
static PixelDestinationContext destination_context;
static HercPixelTableReader shade_reader;
static HercPixelTableReader palette_reader;
static HercPixelTableReader blend_readers[4];
static HercPixelDestinationAccess destination_access;
static renderer_u32 check_count;

static int check_value(int condition, const char *message)
{
    ++check_count;
    if (!condition) {
        printf("FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

#define CHECK(condition, message) \
    do { if (!check_value((condition), (message))) return 1; } while (0)

static void reset_events(void)
{
    events.count = 0;
    memset(events.events, 0, sizeof(events.events));
    shade_context.reads = 0;
    shade_context.last_offset = 0xffffffffUL;
    palette_context.reads = 0;
    palette_context.last_offset = 0xffffffffUL;
    destination_context.reads = 0;
    destination_context.writes = 0;
    destination_context.last_written = 0;
    destination_context.fail_read = 0;
    destination_context.fail_write = 0;
    blend_contexts[0].reads = 0;
    blend_contexts[1].reads = 0;
    blend_contexts[2].reads = 0;
    blend_contexts[3].reads = 0;
    blend_contexts[0].last_offset = 0xffffffffUL;
    blend_contexts[1].last_offset = 0xffffffffUL;
    blend_contexts[2].last_offset = 0xffffffffUL;
    blend_contexts[3].last_offset = 0xffffffffUL;
    blend_contexts[0].fail_read = 0;
    blend_contexts[1].fail_read = 0;
    blend_contexts[2].fail_read = 0;
    blend_contexts[3].fail_read = 0;
}

static int append_event(PixelEventLog *log, renderer_u8 event_id)
{
    if (log == 0 || log->count >= EVENT_CAPACITY)
        return 0;
    log->events[log->count++] = event_id;
    return 1;
}

static int read_byte(void *opaque, renderer_u32 offset, renderer_u8 *value)
{
    PixelReaderContext *context;
    context = (PixelReaderContext *)opaque;
    if (context == 0 || value == 0 || offset >= context->length)
        return 0;
    ++context->reads;
    context->last_offset = offset;
    if (!append_event(context->log, context->event_id))
        return 0;
    if (context->fail_read)
        return 0;
    *value = context->bytes[offset];
    return 1;
}

static int read_destination(void *opaque, renderer_u8 *value)
{
    PixelDestinationContext *context;
    context = (PixelDestinationContext *)opaque;
    if (context == 0 || value == 0)
        return 0;
    ++context->reads;
    if (!append_event(context->log, 2U) || context->fail_read)
        return 0;
    *value = context->value;
    return 1;
}

static int write_destination(void *opaque, renderer_u8 value)
{
    PixelDestinationContext *context;
    context = (PixelDestinationContext *)opaque;
    if (context == 0)
        return 0;
    ++context->writes;
    context->last_written = value;
    if (!append_event(context->log, 4U) || context->fail_write)
        return 0;
    context->value = value;
    return 1;
}

static void setup_readers(void)
{
    renderer_u32 i;

    shade_context.bytes = shade_bytes;
    shade_context.length = 256UL;
    shade_context.event_id = 1U;
    shade_context.fail_read = 0;
    shade_context.log = &events;
    shade_reader.length = 256UL;
    shade_reader.read_byte = read_byte;
    shade_reader.context = &shade_context;

    palette_context.bytes = palette_bytes;
    palette_context.length = 256UL;
    palette_context.event_id = 1U;
    palette_context.fail_read = 0;
    palette_context.log = &events;
    palette_reader.length = 256UL;
    palette_reader.read_byte = read_byte;
    palette_reader.context = &palette_context;

    for (i = 0; i < 4UL; ++i) {
        blend_contexts[i].bytes = blend_bytes[i];
        blend_contexts[i].length = 65536UL;
        blend_contexts[i].event_id = 3U;
        blend_contexts[i].fail_read = 0;
        blend_contexts[i].log = &events;
        blend_readers[i].length = 65536UL;
        blend_readers[i].read_byte = read_byte;
        blend_readers[i].context = &blend_contexts[i];
    }

    destination_context.value = 0;
    destination_context.log = &events;
    destination_access.read_byte = read_destination;
    destination_access.write_byte = write_destination;
    destination_access.context = &destination_context;
}

static int test_c882_selectors(void)
{
    renderer_u32 selector;
    HercPixelStepResult result;

    for (selector = 0; selector < 4UL; ++selector) {
        reset_events();
        destination_context.value = 0x35U;
        result = herc_pixel_c882_step(0U, &shade_reader, blend_readers, 4UL,
                                      selector, &destination_access);
        CHECK(result == HERC_PIXEL_NO_WRITE, "C882 source zero result");
        CHECK(destination_context.value == 0x35U,
              "C882 source zero preserves destination");
        CHECK(events.count == 0UL && shade_context.reads == 0UL &&
              blend_contexts[selector].reads == 0UL &&
              destination_context.reads == 0UL &&
              destination_context.writes == 0UL,
              "C882 source zero bypasses shade, destination, and blend access");

        reset_events();
        destination_context.value = 0x3cU;
        result = herc_pixel_c882_step(7U, &shade_reader, blend_readers, 4UL,
                                      selector, &destination_access);
        CHECK(result == HERC_PIXEL_WROTE, "C882 nonzero source result");
        CHECK(destination_context.value == (renderer_u8)(0xd0U + selector),
              "C882 selector output matches native fixture values");
        CHECK(shade_context.last_offset == 7UL &&
              blend_contexts[selector].last_offset == TEST_BLEND_INDEX,
              "C882 selects shade byte then high-byte/low-byte blend index");
        CHECK(destination_context.reads == 1UL &&
              destination_context.writes == 1UL,
              "C882 reads and writes destination once for nonzero source");
        CHECK(events.count == 4UL && events.events[0] == 1U &&
              events.events[1] == 2U && events.events[2] == 3U &&
              events.events[3] == 4U,
              "C882 access order matches original pixel loop");
    }

    reset_events();
    shade_bytes[8] = 0;
    blend_bytes[2][0x0011UL] = 0x99U;
    destination_context.value = 0x11U;
    result = herc_pixel_c882_step(8U, &shade_reader, blend_readers, 4UL,
                                  2UL, &destination_access);
    CHECK(result == HERC_PIXEL_WROTE && destination_context.value == 0x99U,
          "zero shade result from nonzero source still blends and writes");
    CHECK(blend_contexts[2].last_offset == 0x0011UL,
          "zero shade result indexes blend table using old destination");

    reset_events();
    destination_context.value = 0x3cU;
    result = herc_pixel_c882_step(7U, &shade_reader, blend_readers, 4UL,
                                  4UL, &destination_access);
    CHECK(result == HERC_PIXEL_INVALID && events.count == 0UL &&
          destination_context.value == 0x3cU,
          "C882 rejects invalid selector before side effects");

    reset_events();
    shade_reader.length = 255UL;
    destination_context.value = 0x3cU;
    result = herc_pixel_c882_step(7U, &shade_reader, blend_readers, 4UL,
                                  0UL, &destination_access);
    CHECK(result == HERC_PIXEL_INVALID && events.count == 0UL &&
          destination_context.value == 0x3cU,
          "C882 rejects short shade row before side effects");
    shade_reader.length = 256UL;

    reset_events();
    blend_readers[1].length = 65535UL;
    destination_context.value = 0x3cU;
    result = herc_pixel_c882_step(7U, &shade_reader, blend_readers, 4UL,
                                  1UL, &destination_access);
    CHECK(result == HERC_PIXEL_INVALID && events.count == 0UL &&
          destination_context.value == 0x3cU,
          "C882 rejects short selected blend table before side effects");
    blend_readers[1].length = 65536UL;

    reset_events();
    blend_contexts[0].fail_read = 1;
    destination_context.value = 0x3cU;
    result = herc_pixel_c882_step(7U, &shade_reader, blend_readers, 4UL,
                                  0UL, &destination_access);
    CHECK(result == HERC_PIXEL_INVALID && destination_context.value == 0x3cU &&
          destination_context.writes == 0UL,
          "C882 failed blend read does not publish a pixel");
    return 0;
}

static int test_cc50_and_fill(void)
{
    HercPixelStepResult result;

    reset_events();
    destination_context.value = 0x35U;
    result = herc_pixel_cc50_step(0U, &palette_reader, &destination_access);
    CHECK(result == HERC_PIXEL_NO_WRITE && events.count == 0UL &&
          destination_context.value == 0x35U,
          "CC50 source zero skips palette and destination access");
    CHECK(destination_context.reads == 0UL && destination_context.writes == 0UL,
          "CC50 never reads prior destination");

    reset_events();
    palette_bytes[7] = 0;
    destination_context.value = 0x35U;
    result = herc_pixel_cc50_step(7U, &palette_reader, &destination_access);
    CHECK(result == HERC_PIXEL_WROTE && destination_context.value == 0,
          "CC50 writes a zero palette result for nonzero source");
    CHECK(events.count == 2UL && events.events[0] == 1U &&
          events.events[1] == 4U && destination_context.reads == 0UL,
          "CC50 reads palette then writes without destination read");

    reset_events();
    palette_bytes[9] = 0x42U;
    destination_context.value = 0x35U;
    result = herc_pixel_cc50_step(9U, &palette_reader, &destination_access);
    CHECK(result == HERC_PIXEL_WROTE && destination_context.value == 0x42U,
          "CC50 writes selected palette byte");

    reset_events();
    palette_reader.length = 255UL;
    destination_context.value = 0x35U;
    result = herc_pixel_cc50_step(9U, &palette_reader, &destination_access);
    CHECK(result == HERC_PIXEL_INVALID && events.count == 0UL &&
          destination_context.value == 0x35U,
          "CC50 rejects short palette row before side effects");
    palette_reader.length = 256UL;

    reset_events();
    destination_context.value = 0x35U;
    result = herc_pixel_c2e0_fill_byte(0U, &destination_access);
    CHECK(result == HERC_PIXEL_WROTE && destination_context.value == 0,
          "C2E0 raw fill writes zero as a color byte");
    CHECK(destination_context.reads == 0UL && destination_context.writes == 1UL,
          "C2E0 fill writes without reading old destination");

    reset_events();
    result = herc_pixel_c2e0_fill_byte(0x57U, 0);
    CHECK(result == HERC_PIXEL_INVALID,
          "C2E0 rejects absent destination instead of fake success");
    return 0;
}

int main(void)
{
    renderer_u32 selector;

    memset(shade_bytes, 0, sizeof(shade_bytes));
    memset(palette_bytes, 0, sizeof(palette_bytes));
    memset(blend_bytes, 0, sizeof(blend_bytes));
    shade_bytes[7] = 0xa6U;
    palette_bytes[7] = 0;
    palette_bytes[9] = 0x42U;
    for (selector = 0; selector < 4UL; ++selector)
        blend_bytes[selector][TEST_BLEND_INDEX] =
            (renderer_u8)(0xd0U + selector);
    blend_bytes[2][0x0011UL] = 0x99U;
    setup_readers();

    if (test_c882_selectors() != 0)
        return 1;
    if (test_cc50_and_fill() != 0)
        return 1;

    printf("PASS: %lu pixel-reference checks\n", (unsigned long)check_count);
    return 0;
}
