/* ENG1.DLL 0x27410/0x27450. Original unchecked paths are recovered;
 * current source reports the unproved kind-info domain explicitly.
 * See evidence/eng1_actor_resource_helpers_2026_10_25.json.
 */
#include "actor_region.h"
#include "incomplete.h"

/* The native helpers use an unchecked 8-byte kind-info row index. Current
 * ENG1 source owns only row 0; other indices report incomplete data before
 * the read instead of borrowing adjacent storage. */
void __cdecl mark_actor_resource_for_release(Actor *actor)
{
    unsigned char *resource;
    unsigned short index;
    unsigned short flags;

    if (actor->kind != 0 && actor->record_offset != 0) {
        index = actor->profile;
        if (index >= 1U)
            eng1_incomplete_behavior(0x6fee8UL,
                "27450 kind-info index exceeds the measured row-0 prefix");
        resource = g_kind_info[index].owner + actor->record_offset;
        flags = *(unsigned short *)(resource + 0x0c);
        flags &= 0xdfffU;
        flags |= 0x4000U;
        *(unsigned short *)(resource + 0x0c) = flags;
    }
}

void __cdecl remove_actor_preserving_resource(Actor *actor)
{
    unsigned char *resource;
    unsigned short index;
    unsigned short flags;

    if (actor->kind != 0 && actor->record_offset != 0) {
        index = actor->profile;
        if (index >= 1U)
            eng1_incomplete_behavior(0x6fee8UL,
                "27410 kind-info index exceeds the measured row-0 prefix");
        resource = g_kind_info[index].owner + actor->record_offset;
        flags = *(unsigned short *)(resource + 0x0c);
        flags &= 0xdfffU;
        flags |= 0x4000U;
        *(unsigned short *)(resource + 0x0c) = flags;
    }
    remove_actor(actor);
}