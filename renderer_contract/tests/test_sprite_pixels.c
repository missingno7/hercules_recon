/* Callback-order fixture for private C8F0/CAB0 one-pixel helper proposals. */
#include "hercules_pixels.h"
#include <stdio.h>
#include <string.h>

#define EVENT_CAPACITY 8UL

typedef struct PixelProposalProbe {
    renderer_u8 events[EVENT_CAPACITY];
    renderer_u32 event_count;
    renderer_u32 table_reads;
    renderer_u32 table_offset;
    renderer_u32 destination_reads;
    renderer_u32 destination_writes;
    renderer_u8 destination_value;
    renderer_u8 table_value;
    renderer_u8 last_written;
} PixelProposalProbe;

static renderer_u32 checks;

static int check_value(int condition, const char *message)
{
    ++checks;
    if (!condition) {
        printf("FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

#define CHECK(condition, message) \
    do { if (!check_value((condition), (message))) return 1; } while (0)

static void reset_probe(PixelProposalProbe *probe, renderer_u8 destination)
{
    memset(probe, 0, sizeof(*probe));
    probe->destination_value = destination;
}

static int append_event(PixelProposalProbe *probe, renderer_u8 event)
{
    if (probe == 0 || probe->event_count >= EVENT_CAPACITY)
        return 0;
    probe->events[probe->event_count++] = event;
    return 1;
}

static int read_blend_byte(void *opaque, renderer_u32 offset,
                           renderer_u8 *value)
{
    PixelProposalProbe *probe;

    probe = (PixelProposalProbe *)opaque;
    if (probe == 0 || value == 0 || offset >= HERC_PIXEL_BLEND_BYTES)
        return 0;
    ++probe->table_reads;
    probe->table_offset = offset;
    if (!append_event(probe, 2U))
        return 0;
    *value = probe->table_value;
    return 1;
}

static int read_destination(void *opaque, renderer_u8 *value)
{
    PixelProposalProbe *probe;

    probe = (PixelProposalProbe *)opaque;
    if (probe == 0 || value == 0)
        return 0;
    ++probe->destination_reads;
    if (!append_event(probe, 1U))
        return 0;
    *value = probe->destination_value;
    return 1;
}

static int write_destination(void *opaque, renderer_u8 value)
{
    PixelProposalProbe *probe;

    probe = (PixelProposalProbe *)opaque;
    if (probe == 0)
        return 0;
    ++probe->destination_writes;
    probe->last_written = value;
    if (!append_event(probe, 3U))
        return 0;
    probe->destination_value = value;
    return 1;
}

static int test_c8f0(void)
{
    PixelProposalProbe probe;
    HercPixelTableReader blend_table;
    HercPixelDestinationAccess destination;
    HercPixelStepResult result;
    renderer_u32 selector;

    blend_table.length = HERC_PIXEL_BLEND_BYTES;
    blend_table.read_byte = read_blend_byte;
    blend_table.context = &probe;
    destination.read_byte = read_destination;
    destination.write_byte = write_destination;
    destination.context = &probe;

    reset_probe(&probe, 0x35U);
    probe.table_value = 0xD2U;
    result = herc_pixel_c8f0_step(0U, &blend_table, &destination);
    CHECK(result == HERC_PIXEL_NO_WRITE, "C8F0 zero source result");
    CHECK(probe.event_count == 0UL && probe.destination_value == 0x35U,
          "C8F0 zero source bypasses destination and table callbacks");

    for (selector = 0; selector < HERC_PIXEL_BLEND_SELECTORS; ++selector) {
        reset_probe(&probe, 0x3CU);
        probe.table_value = (renderer_u8)(0xD0U + selector);
        result = herc_pixel_c8f0_step(7U, &blend_table, &destination);
        CHECK(result == HERC_PIXEL_WROTE, "C8F0 nonzero source result");
        CHECK(probe.table_offset == 0x073CUL,
              "C8F0 uses source-high/destination-low blend index");
        CHECK(probe.destination_value == (renderer_u8)(0xD0U + selector),
              "C8F0 selected blend value is written");
        CHECK(probe.event_count == 3UL && probe.events[0] == 1U &&
              probe.events[1] == 2U && probe.events[2] == 3U,
              "C8F0 callback order is destination read, blend read, write");
    }

    reset_probe(&probe, 0xA5U);
    probe.table_value = 0U;
    result = herc_pixel_c8f0_step(7U, &blend_table, &destination);
    CHECK(result == HERC_PIXEL_WROTE && probe.destination_value == 0U,
          "C8F0 writes a zero blend result for nonzero source");
    CHECK(probe.event_count == 3UL && probe.events[2] == 3U,
          "C8F0 zero blend result still reaches the destination writer");
    return 0;
}

static int test_cab0(void)
{
    PixelProposalProbe probe;
    HercPixelDestinationAccess destination;
    HercPixelStepResult result;

    reset_probe(&probe, 0x35U);
    destination.read_byte = 0;
    destination.write_byte = write_destination;
    destination.context = &probe;
    result = herc_pixel_cab0_step(0U, &destination);
    CHECK(result == HERC_PIXEL_NO_WRITE, "CAB0 zero source result");
    CHECK(probe.event_count == 0UL && probe.destination_value == 0x35U,
          "CAB0 zero source skips destination write");

    result = herc_pixel_cab0_step(7U, &destination);
    CHECK(result == HERC_PIXEL_WROTE && probe.destination_value == 7U,
          "CAB0 copies a nonzero source byte");
    CHECK(probe.event_count == 1UL && probe.events[0] == 3U &&
          probe.destination_reads == 0UL && probe.destination_writes == 1UL,
          "CAB0 writes once without reading the prior destination");
    return 0;
}

int main(void)
{
    if (test_c8f0() != 0 || test_cab0() != 0)
        return 1;
    printf("PASS: %lu private C8F0/CAB0 proposal checks\n",
           (unsigned long)checks);
    return 0;
}
