#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/composition_events.h"

#define MAX_EVENTS 128U

static int read_event(FILE *stream, CompositionEvent *event, int *at_end)
{
    unsigned long value[14];
    int matched;

    matched = fscanf(stream,
        "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
        &value[0], &value[1], &value[2], &value[3], &value[4], &value[5],
        &value[6], &value[7], &value[8], &value[9], &value[10], &value[11],
        &value[12], &value[13]);
    if (matched == EOF) {
        *at_end = 1;
        return 1;
    }
    if (matched != 14)
        return 0;
    *at_end = 0;
    if (value[0] > 0xffffffffUL || value[1] > 0xffffffffUL ||
        value[2] > 0xffffffffUL || value[3] > 0xffffUL ||
        value[4] > 0xffUL || value[5] > 0xffUL || value[6] > 0xffUL ||
        value[7] > 0xffffffffUL || value[8] > 0xffffffffUL ||
        value[9] > 0xffffffffUL || value[10] > 0xffffffffUL ||
        value[11] > 0xffffffffUL || value[12] > 0xffffffffUL ||
        value[13] > 0xffUL)
        return 0;

    memset(event, 0, sizeof(*event));
    event->scope_id = (composition_u32)value[0];
    event->local_order = (composition_u32)value[1];
    event->source_order = (composition_u32)value[2];
    event->kind = (composition_u16)value[3];
    event->origin = (composition_u8)value[4];
    event->domain = (composition_u8)value[5];
    event->semantic_owner = (composition_u8)value[6];
    event->source_rva = (composition_u32)value[7];
    event->raw_token = (composition_u32)value[8];
    event->argument[0] = (composition_u32)value[9];
    event->argument[1] = (composition_u32)value[10];
    event->argument[2] = (composition_u32)value[11];
    event->argument[3] = (composition_u32)value[12];
    event->flags = (composition_u8)value[13];
    return 1;
}

int main(int argc, char **argv)
{
    CompositionEvent storage[MAX_EVENTS];
    CompositionEvent event;
    CompositionTrace trace;
    CompositionTraceStatus status;
    FILE *stream;
    int at_end;
    int count = 0;

    if (argc != 2) {
        fprintf(stderr, "usage: validate_composition_stream <events.tsv>\n");
        return 2;
    }
    stream = fopen(argv[1], "r");
    if (stream == 0) {
        fprintf(stderr, "cannot open event stream\n");
        return 2;
    }
    composition_trace_init(&trace, storage, MAX_EVENTS);
    for (;;) {
        if (!read_event(stream, &event, &at_end)) {
            fclose(stream);
            fprintf(stderr, "malformed event row\n");
            return 2;
        }
        if (at_end)
            break;
        status = composition_trace_append(&trace, &event);
        if (status != COMPOSITION_TRACE_OK) {
            fclose(stream);
            fprintf(stderr, "event append failed: %u\n", (unsigned int)status);
            return 1;
        }
        ++count;
    }
    fclose(stream);
    status = composition_validate_connected_frame(storage, trace.count, 1U);
    if (status != COMPOSITION_TRACE_OK) {
        fprintf(stderr, "connected AC60 frame order failed: %u\n", (unsigned int)status);
        return 1;
    }
    printf("status=connected_subset_pass events=%d scopes=1 semantic_owner=unclassified\n", count);
    return 0;
}
