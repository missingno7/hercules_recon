#ifndef RESOURCE_DESTROY_RESOURCE_RECORD_H
#define RESOURCE_DESTROY_RESOURCE_RECORD_H

#include <stddef.h>

typedef short ActorReference[2];
typedef short ActorTokenReference[2];

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
typedef char actor_resource_fields_match_offsets[
    (offsetof(ActorResource, path) == 0 &&
     offsetof(ActorResource, entry_count) == 4 &&
     offsetof(ActorResource, unknown_006) == 6 &&
     offsetof(ActorResource, file_length) == 8 &&
     offsetof(ActorResource, references) == 12 &&
     offsetof(ActorResource, residency_state) == 16 &&
     offsetof(ActorResource, unknown_012) == 18) ? 1 : -1];

#endif
