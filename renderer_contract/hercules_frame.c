#include "hercules_frame.h"
void frame_visit_guard_init(FrameVisitGuard *guard,
                            renderer_raw32 *visited_addresses,
                            renderer_u32 capacity)
{
    if (guard == 0)
        return;
    guard->visited_addresses = visited_addresses;
    guard->capacity = capacity;
    guard->count = 0;
}

int frame_raw32_region_resolve(void *context,
                               renderer_raw32 address,
                               renderer_u32 byte_count,
                               const renderer_u8 **resolved_bytes)
{
    FrameRaw32Region *region = (FrameRaw32Region *)context;
    renderer_u32 offset;
    if (region == 0 || resolved_bytes == 0 || region->bytes == 0)
        return 0;
    if (region->size > (renderer_u32)0xffffffffu - region->base)
        return 0;
    if (address < region->base)
        return 0;
    offset = address - region->base;
    if (offset > region->size || byte_count > region->size - offset)
        return 0;
    *resolved_bytes = region->bytes + offset;
    return 1;
}

static FrameWalkStatus read_raw32(FrameRaw32Resolver resolver,
                                  void *resolver_context,
                                  renderer_raw32 address,
                                  renderer_raw32 *value)
{
    const renderer_u8 *bytes;
    if (resolver == 0 || value == 0)
        return FRAME_WALK_INVALID_ARGUMENT;
    if (!resolver(resolver_context, address, 4, &bytes) || bytes == 0)
        return FRAME_WALK_UNMAPPED_ADDRESS;
    /* Decode the fixed little-endian raw32 field without depending on host
     * alignment or endianness. */
    *value = (renderer_raw32)bytes[0] |
             ((renderer_raw32)bytes[1] << 8) |
             ((renderer_raw32)bytes[2] << 16) |
             ((renderer_raw32)bytes[3] << 24);
    return FRAME_WALK_OK;
}

static FrameWalkStatus visit_address(FrameVisitGuard *guard,
                                     renderer_raw32 address)
{
    renderer_u32 i;
    if (guard == 0 || address == 0 ||
        (guard->capacity != 0 && guard->visited_addresses == 0))
        return FRAME_WALK_INVALID_ARGUMENT;
    for (i = 0; i < guard->count; ++i)
        if (guard->visited_addresses[i] == address)
            return FRAME_WALK_CYCLE;
    if (guard->count >= guard->capacity)
        return FRAME_WALK_VISIT_LIMIT;
    guard->visited_addresses[guard->count++] = address;
    return FRAME_WALK_OK;
}

FrameWalkStatus frame_walk_raw32_links(FrameRaw32Resolver resolver,
                                       void *resolver_context,
                                       renderer_raw32 head,
                                       FrameVisitGuard *guard,
                                       renderer_raw32 *terminator)
{
    renderer_raw32 cursor = head;
    FrameWalkStatus status;
    renderer_raw32 next;
    if (resolver == 0 || guard == 0 || terminator == 0 ||
        (guard->capacity != 0 && guard->visited_addresses == 0))
        return FRAME_WALK_INVALID_ARGUMENT;
    guard->count = 0;
    *terminator = 0;
    while (cursor != 0) {
        status = read_raw32(resolver, resolver_context, cursor, &next);
        if (status != FRAME_WALK_OK)
            return status;
        status = visit_address(guard, cursor);
        if (status != FRAME_WALK_OK)
            return status;
        cursor = next;
    }
    *terminator = cursor;
    return FRAME_WALK_OK;
}

FrameWalkStatus frame_walk_ac60_order_table(FrameRaw32Resolver resolver,
                                            void *resolver_context,
                                            renderer_raw32 order_table_base,
                                            renderer_u32 slot_count,
                                            FrameVisitGuard *guard)
{
    renderer_raw32 list_cursor = order_table_base;
    renderer_raw32 next_slot;
    renderer_raw32 next;
    renderer_u32 steps = 0;
    renderer_u32 next_slot_index = 1;
    renderer_u32 table_bytes;
    FrameWalkStatus status;
    if (resolver == 0 || guard == 0 || order_table_base == 0 ||
        (guard->capacity != 0 && guard->visited_addresses == 0) ||
        slot_count == 0)
        return FRAME_WALK_INVALID_ARGUMENT;
    if (slot_count > (renderer_u32)0xffffffffu / 4u)
        return FRAME_WALK_ADDRESS_OVERFLOW;
    table_bytes = slot_count * 4u;
    if (table_bytes > (renderer_u32)0xffffffffu - order_table_base)
        return FRAME_WALK_ADDRESS_OVERFLOW;
    guard->count = 0;
    next_slot = order_table_base + 4u;
    for (;;) {
        if (steps >= guard->capacity)
            return FRAME_WALK_VISIT_LIMIT;
        ++steps;
        status = read_raw32(resolver, resolver_context, list_cursor, &next);
        if (status != FRAME_WALK_OK)
            return status;
        list_cursor = next;
        while (list_cursor == next_slot) {
            /* AC60 itself has no slot bound. For this bounded decoder, the
             * one-past address is malformed and is never passed to resolver. */
            if (next_slot_index >= slot_count)
                return FRAME_WALK_RANGE_ERROR;
            if (steps >= guard->capacity)
                return FRAME_WALK_VISIT_LIMIT;
            ++steps;
            status = read_raw32(resolver, resolver_context, list_cursor,
                                &next);
            if (status != FRAME_WALK_OK)
                return status;
            list_cursor = next;
            if (next_slot > (renderer_u32)0xffffffffu - 4u)
                return FRAME_WALK_ADDRESS_OVERFLOW;
            next_slot += 4u;
            ++next_slot_index;
        }
        if (list_cursor == 0)
            return FRAME_WALK_OK;
        status = visit_address(guard, list_cursor);
        if (status != FRAME_WALK_OK)
            return status;
    }
}
