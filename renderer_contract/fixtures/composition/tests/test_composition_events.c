#include <string.h>
#include "../src/composition_events.h"

static int check(int condition)
{
    return condition ? 0 : 1;
}

int main(void)
{
    CompositionEvent events[2];
    CompositionEvent event;
    CompositionTrace trace;
    int failures = 0;

    memset(&event, 0, sizeof(event));
    event.scope_id = 1;
    event.local_order = 0;
    event.source_order = 1;
    event.source_rva = 0xAC60U;
    event.kind = COMPOSITION_EVENT_GEOMETRY_COMMAND;
    event.origin = COMPOSITION_ORIGIN_ORIGINAL_INSTRUCTION;
    event.domain = COMPOSITION_DOMAIN_GEOMETRY;
    event.semantic_owner = COMPOSITION_OWNER_UNCLASSIFIED;

    composition_trace_init(&trace, events, 2);
    failures += check(composition_trace_append(&trace, &event) == COMPOSITION_TRACE_OK);
    failures += check(trace.count == 1);
    failures += check(trace.events[0].semantic_owner == COMPOSITION_OWNER_UNCLASSIFIED);

    event.local_order = 1;
    event.kind = COMPOSITION_EVENT_UNKNOWN_WRITER;
    event.origin = COMPOSITION_ORIGIN_UNRESOLVED;
    failures += check(composition_trace_append(&trace, &event) == COMPOSITION_TRACE_INCOMPLETE_UNKNOWN);
    failures += check(trace.count == 2);
    failures += check(trace.status == COMPOSITION_TRACE_INCOMPLETE_UNKNOWN);

    composition_trace_init(&trace, events, 1);
    event.kind = COMPOSITION_EVENT_FRAME_ENTRY;
    event.origin = COMPOSITION_ORIGIN_ORIGINAL_INSTRUCTION;
    event.local_order = 0;
    failures += check(composition_trace_append(&trace, &event) == COMPOSITION_TRACE_OK);
    event.local_order = 1;
    failures += check(composition_trace_append(&trace, &event) == COMPOSITION_TRACE_OVERFLOW);
    failures += check(trace.status == COMPOSITION_TRACE_OVERFLOW);

    composition_trace_init(&trace, events, 2);
    event.local_order = 1;
    failures += check(composition_trace_append(&trace, &event) == COMPOSITION_TRACE_ORDER_ERROR);
    failures += check(trace.count == 0);

    return failures == 0 ? 0 : 1;
}
