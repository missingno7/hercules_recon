/* PC-backed readiness dependencies; names and complete context extent unproved. */
#include "resource_record.h"
#include "resource_context.h"

extern ActorResource g_actor_resources[];
extern ResourceCallbackContext *g_resource_context;
extern void __cdecl resource_reset(int resource_id);

int __cdecl resource_metadata_scan(unsigned long resource_id, int index)
{
    ResourceCallbackContext *context = g_resource_context;
    long bytes_available =
        ((long)context->total_sectors_0a0 -
         (long)context->remaining_sectors_09e) * 2048L;
    unsigned short entry_count;
    unsigned char *metadata_base;
    long scan_index;
    long descriptor;
    long relative_offset;

    if (bytes_available < 4096L)
        return 0;

    entry_count = g_actor_resources[resource_id].entry_count;
    metadata_base =
        (unsigned char *)g_actor_resources[resource_id].references +
        (unsigned long)entry_count * 4UL + 8UL;

    if (index >= (long)entry_count)
        return 0;

    for (scan_index = (long)index + 1L;
         scan_index <= (long)entry_count;
         ++scan_index) {
        descriptor = *(long *)(metadata_base + scan_index * 4L);
        relative_offset = descriptor >> 8;
        if (relative_offset != 0L)
            return bytes_available >= relative_offset;
    }

    return 0;
}

int __cdecl ensure_actor_resource_ready(unsigned long resource_id, int index)
{
    unsigned short state = g_actor_resources[resource_id].residency_state;

    if (state == 2U)
        return resource_metadata_scan(resource_id, index);

    if (state == 0U) {
        resource_reset((int)resource_id);
        g_actor_resources[resource_id].unknown_012 = 3U;
    }

    return 0;
}
