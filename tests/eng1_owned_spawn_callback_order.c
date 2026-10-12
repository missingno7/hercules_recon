#include <stdio.h>
#include <string.h>
#include "../src/eng1/behavioral/interpreters.h"

static unsigned long frame_calls;
static unsigned long spawn_calls;
static Actor *observed_actor;
static long observed_x, observed_y, observed_z;
static unsigned short observed_kind;
static int ordering_ok;

void __cdecl eng1_guarded_advance_frame(Actor *actor)
{
    ++frame_calls;
    actor->x = 0x12345678L;
    actor->y = -0x1234567L;
    actor->z = 0x76543210L;
}

void __cdecl spawn_actor_with_owner(Actor *owner, long x, long y, long z,
                                    unsigned short kind)
{
    ++spawn_calls;
    observed_actor = owner;
    observed_x = x;
    observed_y = y;
    observed_z = z;
    observed_kind = kind;
    ordering_ok = frame_calls == 1UL;
}

int main(void)
{
    Actor actor;

    memset(&actor, 0, sizeof(actor));
    frame_calls = spawn_calls = 0;
    observed_actor = 0;
    ordering_ok = 0;
    actor.x = 1;
    actor.y = 2;
    actor.z = 3;
    eng1_native_code_target_040e20(&actor);
    if (!ordering_ok || spawn_calls != 1UL || observed_actor != &actor ||
        observed_x != 0x12345678L || observed_y != -0x1234567L ||
        observed_z != 0x76543210L || observed_kind != 0x34U) {
        printf("FAIL source-only mutation order\n");
        return 1;
    }
    printf("source-only mutation-order controls=1 checks=7 failures=0\n");
    return 0;
}
