/* Private ordinary-C reconstruction of ENG1.DLL RVA 0x40e20. */
#include "interpreters.h"

#include "read_guard.h"
extern void __cdecl spawn_actor_with_owner(void *owner, long x, long y,
                                           long z, unsigned short kind);

void __cdecl eng1_native_code_target_040e20(Actor *actor)
{
    volatile Actor *live;
    long z;
    long y;
    long x;

    live = actor;
    eng1_guarded_advance_frame(actor);

    /* Native 0x40e2b/2e/31 loads these after frame advancement, z/y/x. */
    z = live->z;
    y = live->y;
    x = live->x;
    spawn_actor_with_owner(actor, x, y, z, 0x34U);
}
