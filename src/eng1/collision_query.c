/* ENG1.DLL RVA 0x200e0: wrapped area/actor bounds query.
 * Source is based on the original entry and its callers; the 12-byte local
 * actor bounds and area x-normalization side effect are observable behavior. */
#include "actor_bounds_2a880.h"

typedef struct Actor Actor;

int __cdecl eng1_200e0(Query2a170Bounds *area, Actor *actor)
{
    Query2a170Bounds actor_bounds;
    int delta;
    int separated;

    if (actor_bounds_2a880(actor, &actor_bounds, 0)) {
        delta = area->mn[0] - actor_bounds.mn[0];
        if (delta > 0x800) {
            area->mn[0] -= 4096;
            area->mx[0] -= 4096;
        }
        if (delta < -0x800) {
            actor_bounds.mn[0] -= 4096;
            actor_bounds.mx[0] -= 4096;
        }

        separated = actor_bounds.mx[1] < area->mn[1];
        separated |= actor_bounds.mx[2] < area->mn[2];
        separated |= area->mx[0] < actor_bounds.mn[0];
        separated |= area->mx[2] < actor_bounds.mn[2];
        separated |= area->mx[1] < actor_bounds.mn[1];
        separated |= actor_bounds.mx[0] < area->mn[0];
        return !separated;
    }
    return 0;
}
