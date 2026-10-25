/* ENG1.DLL RVA0x27d40: source child creation/link wrapper.
 * Existing factory, allocator, initializer and detach providers are reused. */
#include "action_spawn_27d40.h"

typedef unsigned char eng1_u8;
typedef unsigned short eng1_u16;
typedef unsigned long eng1_u32;
typedef struct ActorDetachView ActorDetachView;

extern void __cdecl detach_child(ActorDetachView *actor);
extern Actor *__cdecl spawn_kind(long x, long y, long z, eng1_u16 kind);

Actor *__cdecl action_spawn_27d40(Actor *actor, unsigned long action)
{
    volatile eng1_u8 *parent = (volatile eng1_u8 *)actor;
    volatile eng1_u8 *child_bytes;
    Actor *child;
    eng1_u32 flags;
    long x;
    long y;
    long z;
    eng1_u16 word;
    eng1_u8 byte;

    if (*(volatile eng1_u32 *)(parent + 0x50) & 0x40000000UL)
        return 0;

    if (*(volatile eng1_u32 *)(parent + 0x60) != 0)
        detach_child((ActorDetachView *)actor);

    /* These are live reads after detachment, in native z/y/x load order. */
    z = *(volatile long *)(parent + 0x08);
    y = *(volatile long *)(parent + 0x04);
    x = *(volatile long *)(parent + 0x00);
    child = spawn_kind(x, y, z, (eng1_u16)action);
    if (child == 0)
        return 0;

    child_bytes = (volatile eng1_u8 *)child;
    *(volatile eng1_u32 *)(child_bytes + 0x50) = 0x10000000UL;
    flags = *(volatile eng1_u32 *)(parent + 0x54);
    flags |= 0x00800000UL;
    *(volatile eng1_u32 *)(parent + 0x60) = (eng1_u32)(unsigned long)child;
    *(volatile eng1_u32 *)(parent + 0x54) = flags;
    word = *(volatile eng1_u16 *)(parent + 0x38);
    *(volatile eng1_u16 *)(child_bytes + 0x38) = word;
    byte = *(volatile eng1_u8 *)(parent + 0x22);
    *(volatile eng1_u8 *)(child_bytes + 0x22) = byte;
    return child;
}
