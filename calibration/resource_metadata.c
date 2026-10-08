#include <stddef.h>

typedef short ActorReference[2];

typedef struct ActorResource ActorResource;
struct ActorResource {
    void *path;
    unsigned short entry_count;
    unsigned short unknown_006;
    unsigned long file_length;
    ActorReference *references;
    unsigned short residency_state;
    unsigned short unknown_012;
};

typedef char actor_reference_size_is_4[(sizeof(ActorReference) == 4) ? 1 : -1];
typedef char actor_resource_size_is_20[(sizeof(ActorResource) == 20) ? 1 : -1];
typedef char actor_resource_member_offsets_match[
    (offsetof(ActorResource, entry_count) == 4 &&
     offsetof(ActorResource, file_length) == 8 &&
     offsetof(ActorResource, references) == 12 &&
     offsetof(ActorResource, residency_state) == 16 &&
     offsetof(ActorResource, unknown_012) == 18) ? 1 : -1];

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
