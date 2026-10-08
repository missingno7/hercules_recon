#include "resource_record.h"

extern ActorResource g_actor_resources[];
extern void __cdecl release_resource_buffer(void *buffer);

void __cdecl resource_destroy(int resource_index)
{
    ActorResource *resource = &g_actor_resources[resource_index];

    if (resource->references != 0)
        release_resource_buffer(resource->references);

    resource->references = 0;
    resource->residency_state = 0;
    resource->unknown_012 = 0;
}
