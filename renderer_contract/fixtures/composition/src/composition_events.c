#include "composition_events.h"

void composition_trace_init(CompositionTrace *trace,
                            CompositionEvent *storage,
                            composition_u32 capacity)
{
    if (trace == 0)
        return;
    trace->events = storage;
    trace->capacity = capacity;
    trace->count = 0;
    trace->status = (storage == 0 && capacity != 0)
        ? COMPOSITION_TRACE_INVALID : COMPOSITION_TRACE_OK;
}

CompositionTraceStatus composition_trace_append(
    CompositionTrace *trace, const CompositionEvent *event)
{
    composition_u32 index;
    const CompositionEvent *previous;

    if (trace == 0 || event == 0)
        return COMPOSITION_TRACE_INVALID;
    if (trace->status != COMPOSITION_TRACE_OK)
        return trace->status;
    if (trace->events == 0 || trace->count >= trace->capacity) {
        trace->status = COMPOSITION_TRACE_OVERFLOW;
        return trace->status;
    }
    if (event->kind == 0 || event->scope_id == 0 ||
        (event->kind > COMPOSITION_EVENT_OUTPUT_FLUSH &&
         event->kind != COMPOSITION_EVENT_UNKNOWN_WRITER) ||
        event->origin == 0 ||
        (event->origin > COMPOSITION_ORIGIN_TOOL_BOOKKEEPING &&
         event->origin != COMPOSITION_ORIGIN_UNRESOLVED) ||
        event->domain > COMPOSITION_DOMAIN_EXTERNAL ||
        event->semantic_owner > COMPOSITION_OWNER_SOURCE_ASSOCIATED) {
        trace->status = COMPOSITION_TRACE_INVALID;
        return trace->status;
    }

    previous = 0;
    for (index = trace->count; index > 0; --index) {
        if (trace->events[index - 1].scope_id == event->scope_id) {
            previous = &trace->events[index - 1];
            break;
        }
    }
    if ((previous == 0 && event->local_order != 0) ||
        (previous != 0 && event->local_order <= previous->local_order)) {
        trace->status = COMPOSITION_TRACE_ORDER_ERROR;
        return trace->status;
    }

    trace->events[trace->count++] = *event;
    if (event->kind == COMPOSITION_EVENT_UNKNOWN_WRITER ||
        event->origin == COMPOSITION_ORIGIN_UNRESOLVED) {
        trace->status = COMPOSITION_TRACE_INCOMPLETE_UNKNOWN;
    }
    return trace->status;
}

CompositionTraceStatus composition_validate_connected_frame(
    const CompositionEvent *events, composition_u32 count,
    composition_u32 scope_id)
{
    composition_u32 i;
    composition_u32 source_order = 0;
    composition_u32 frame = 0, lock = 0, command01 = 0, move = 0;
    composition_u32 callback1 = 0, link_reload = 0, rewritten_packet = 0;
    composition_u32 remap = 0, unlock = 0, present = 0;
    composition_u32 output = 0, flush = 0, callback2 = 0;
    composition_u32 callback_entry_count = 0, callback_entry1 = 0;
    composition_u32 callback_entry2 = 0, callback1_new_link = 0;
    composition_u32 callbacks_seen = 0;
    composition_u32 command_token = 0;
    composition_u32 rewritten_token = 0;
    composition_u32 event_order = 0;
    int have_previous = 0;

    if (events == 0 || count == 0 || scope_id == 0)
        return COMPOSITION_TRACE_INVALID;

    for (i = 0; i < count; ++i) {
        const CompositionEvent *event = &events[i];
        if (event->scope_id != scope_id)
            continue;
        if (event->local_order != event_order++)
            return COMPOSITION_TRACE_ORDER_ERROR;
        if (have_previous && event->source_order < source_order)
            return COMPOSITION_TRACE_ORDER_ERROR;
        source_order = event->source_order;
        have_previous = 1;
        if (event->semantic_owner != COMPOSITION_OWNER_UNCLASSIFIED)
            return COMPOSITION_TRACE_INVALID;

        if (event->kind == COMPOSITION_EVENT_FRAME_ENTRY) {
            if (frame != 0 || event->origin != COMPOSITION_ORIGIN_ORIGINAL_INSTRUCTION ||
                event->source_rva != 0xAC60U)
                return COMPOSITION_TRACE_ORDER_ERROR;
            frame = i + 1;
        } else if (event->kind == COMPOSITION_EVENT_SURFACE_LOCK) {
            if (frame == 0 || lock != 0 ||
                event->origin != COMPOSITION_ORIGIN_ORIGINAL_INSTRUCTION ||
                event->source_rva != 0x6FF0U)
                return COMPOSITION_TRACE_ORDER_ERROR;
            lock = i + 1;
        } else if (event->kind == COMPOSITION_EVENT_PACKET_CONSUMED &&
                   event->argument[0] == 0x01U) {
            if (lock == 0 || command01 != 0 ||
                event->origin != COMPOSITION_ORIGIN_ORIGINAL_INSTRUCTION ||
                event->source_rva != 0xACB9U)
                return COMPOSITION_TRACE_ORDER_ERROR;
            command01 = i + 1;
            command_token = event->raw_token;
        } else if (event->kind == COMPOSITION_EVENT_VRAM_MOVE_REQUEST) {
            if (command01 == 0 || move != 0 ||
                event->origin != COMPOSITION_ORIGIN_ORIGINAL_INSTRUCTION ||
                event->source_rva != 0xA8F0U)
                return COMPOSITION_TRACE_ORDER_ERROR;
            move = i + 1;
        } else if (event->kind == COMPOSITION_EVENT_CALLBACK_ENTRY) {
            if (move == 0 || event->origin != COMPOSITION_ORIGIN_ORIGINAL_INSTRUCTION ||
                event->source_rva != 0xBD80U)
                return COMPOSITION_TRACE_ORDER_ERROR;
            ++callback_entry_count;
            if (callback_entry_count == 1) {
                if (callback1 != 0)
                    return COMPOSITION_TRACE_ORDER_ERROR;
                callback_entry1 = i + 1;
            } else if (callback_entry_count == 2) {
                if (flush == 0 || callback2 != 0)
                    return COMPOSITION_TRACE_ORDER_ERROR;
                callback_entry2 = i + 1;
            } else {
                return COMPOSITION_TRACE_ORDER_ERROR;
            }
        } else if (event->kind == COMPOSITION_EVENT_CALLBACK_SERVICE) {
            if (event->origin != COMPOSITION_ORIGIN_CONTROLLED_ENDPOINT ||
                event->domain != COMPOSITION_DOMAIN_EXTERNAL)
                return COMPOSITION_TRACE_INVALID;
            ++callbacks_seen;
            if (callbacks_seen == 1) {
                if (move == 0 || callback_entry1 == 0 ||
                    callback_entry1 >= i + 1 || link_reload != 0 ||
                    event->argument[0] != 0xABE9U ||
                    event->raw_token != 0x31000000U)
                    return COMPOSITION_TRACE_ORDER_ERROR;
                callback1 = i + 1;
                callback1_new_link = event->argument[3];
            } else if (callbacks_seen == 2) {
                if (flush == 0 || callback_entry2 == 0 ||
                    callback_entry2 >= i + 1 || event->argument[0] != 0xB2E7U)
                    return COMPOSITION_TRACE_ORDER_ERROR;
                callback2 = i + 1;
            } else {
                return COMPOSITION_TRACE_ORDER_ERROR;
            }
        } else if (event->kind == COMPOSITION_EVENT_LIVE_LINK_RELOAD) {
            if (callback1 == 0 || link_reload != 0 ||
                event->origin != COMPOSITION_ORIGIN_ORIGINAL_INSTRUCTION ||
                event->source_rva != 0xACA2U ||
                event->raw_token != command_token ||
                event->argument[0] != callback1_new_link)
                return COMPOSITION_TRACE_ORDER_ERROR;
            link_reload = i + 1;
            rewritten_token = event->argument[0];
        } else if (event->kind == COMPOSITION_EVENT_PACKET_CONSUMED &&
                   event->argument[0] == 0xE1U && link_reload != 0 &&
                   rewritten_packet == 0) {
            if (event->raw_token != rewritten_token ||
                event->origin != COMPOSITION_ORIGIN_ORIGINAL_INSTRUCTION ||
                event->source_rva != 0xACB9U)
                return COMPOSITION_TRACE_ORDER_ERROR;
            rewritten_packet = i + 1;
        } else if (event->kind == COMPOSITION_EVENT_SURFACE_REMAP_REQUEST) {
            if (rewritten_packet == 0 || remap != 0 ||
                event->origin != COMPOSITION_ORIGIN_ORIGINAL_INSTRUCTION ||
                event->source_rva != 0x70C0U)
                return COMPOSITION_TRACE_ORDER_ERROR;
            remap = i + 1;
        } else if (event->kind == COMPOSITION_EVENT_SURFACE_UNLOCK) {
            if (remap == 0 || unlock != 0 ||
                event->origin != COMPOSITION_ORIGIN_ORIGINAL_INSTRUCTION ||
                event->source_rva != 0x7140U)
                return COMPOSITION_TRACE_ORDER_ERROR;
            unlock = i + 1;
        } else if (event->kind == COMPOSITION_EVENT_PRESENT_REQUEST) {
            if (unlock == 0 || present != 0 ||
                event->origin != COMPOSITION_ORIGIN_ORIGINAL_INSTRUCTION ||
                event->source_rva != 0x6620U)
                return COMPOSITION_TRACE_ORDER_ERROR;
            present = i + 1;
        } else if (event->kind == COMPOSITION_EVENT_FINAL_OUTPUT) {
            if (present == 0 || output != 0 ||
                event->origin != COMPOSITION_ORIGIN_CONTROLLED_ENDPOINT ||
                event->domain != COMPOSITION_DOMAIN_DISPLAY)
                return COMPOSITION_TRACE_ORDER_ERROR;
            output = i + 1;
        } else if (event->kind == COMPOSITION_EVENT_OUTPUT_FLUSH) {
            if (output == 0 || flush != 0 ||
                event->origin != COMPOSITION_ORIGIN_CONTROLLED_ENDPOINT ||
                event->domain != COMPOSITION_DOMAIN_DISPLAY)
                return COMPOSITION_TRACE_ORDER_ERROR;
            flush = i + 1;
        }
    }

    if (frame == 0 || lock == 0 || command01 == 0 || move == 0 ||
        callback1 == 0 || link_reload == 0 || rewritten_packet == 0 ||
        remap == 0 || unlock == 0 || present == 0 || output == 0 ||
        flush == 0 || callback2 == 0)
        return COMPOSITION_TRACE_ORDER_ERROR;
    if (callback_entry_count != 2 || callback_entry1 >= callback1 ||
        callback_entry2 >= callback2)
        return COMPOSITION_TRACE_ORDER_ERROR;
    if (!(frame < lock && lock < command01 && command01 < move &&
          move < callback1 && callback1 < link_reload &&
          link_reload < rewritten_packet && rewritten_packet < remap &&
          remap < unlock && unlock < present && present < output &&
          output < flush && flush < callback2))
        return COMPOSITION_TRACE_ORDER_ERROR;
    return COMPOSITION_TRACE_OK;
}
