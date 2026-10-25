#include "../shared/resource_record.h"

extern ActorResource g_actor_resources[];
extern int ensure_actor_resource_ready(unsigned long resource_id, int metadata_index);

void *resource_metadata_pointer(unsigned long resource_id, int metadata_index)
{
    ActorReference *buffer;
    unsigned char *metadata_base;
    short signed_index;
    long descriptor;

    if (metadata_index == 0)
        return 0;

    if (g_actor_resources[resource_id].residency_state != 3 &&
        !ensure_actor_resource_ready(resource_id, metadata_index))
        return 0;

    buffer = g_actor_resources[resource_id].references;
    metadata_base = (unsigned char *)buffer +
        (unsigned long)g_actor_resources[resource_id].entry_count * 4UL + 8UL;
    signed_index = (short)metadata_index;
    descriptor = *(long *)(metadata_base + signed_index * 4);
    if (descriptor == 0)
        return 0;

    return metadata_base + (descriptor >> 8);
}
