/* Ordinary-C reconstruction of ENG1 0x300c0 and 0x30180.
 * The coordinate/kind providers remain existing dependencies.  The distinct
 * 0x277e0 middle-coordinate factory is reconstructed here using the existing middle allocator/initializer. */
#include "worklist_factory_types.h"
#include "worklist_read_guards.h"

extern void *__cdecl spawn_normal(long x, long y, long z, unsigned short kind);
extern void *__cdecl spawn_kind(long x, long y, long z, unsigned short kind);
extern ActorMiddle *__cdecl allocate_middle_actor(ActorKind kind);
extern void __cdecl initialize_middle_actor(ActorMiddle *actor);
extern void __cdecl append_chain(void *owner, void *child);

/* These are existing pointer-valued configuration cells.  The source owner
 * and measured table extents are supplied by existing providers. */
extern unsigned long g_configuration_dword_71370;
extern unsigned long g_configuration_dword_7137c;
extern unsigned long g_configuration_dword_71390;
extern unsigned long g_configuration_dword_7139c;

static void call_action(unsigned long table_cell, unsigned long kind,
                        unsigned long stride, const char *site, void *actor)
{
    unsigned char *table = (unsigned char *)(unsigned long)table_cell;
    unsigned long index = kind & 0x0fffUL;
    Eng1ActorActionCallback callback;

    eng1_guard_worklist_table_read(table, index, stride, 4UL, site);
    callback = *(Eng1ActorActionCallback *)(void *)(table + index * stride);
    callback(actor);
}

static void copy_owner_bytes(void *owner, void *child)
{
    unsigned char *from = (unsigned char *)owner;
    unsigned char *to = (unsigned char *)child;
    to[0x94] = from[0x94];
    to[0x95] = from[0x95];
    to[0x96] = from[0x96];
}

ActorMiddle *__cdecl spawn_middle_277e0(long x, long y, long z,
                                            unsigned short kind)
{
    ActorMiddle *allocated = allocate_middle_actor(kind);

    if (allocated != 0) {
        unsigned char *actor = (unsigned char *)allocated;
        unsigned char *assets;
        unsigned long index = kind & 0x0fffUL;
        unsigned long offset = index * 8UL;

        initialize_middle_actor(allocated);
        *(unsigned short *)(void *)(actor + 0x2e) =
            (unsigned short)(kind | 0x4000U);
        *(unsigned long *)(void *)(actor + 0x00) = (unsigned long)x;
        *(unsigned long *)(void *)(actor + 0x9c) = (unsigned long)x;
        *(unsigned long *)(void *)(actor + 0x04) = (unsigned long)y;
        *(unsigned long *)(void *)(actor + 0xa0) = (unsigned long)y;
        *(unsigned long *)(void *)(actor + 0x08) = (unsigned long)z;
        *(unsigned long *)(void *)(actor + 0xa4) = (unsigned long)z;
        assets = (unsigned char *)(unsigned long)g_configuration_dword_71390;
        eng1_guard_worklist_table_read(assets, index, 8UL, 8UL,
                                       "0x277e0 middle coordinate asset record");
        actor[0x22] = assets[offset + 6UL];
        assets = (unsigned char *)(unsigned long)g_configuration_dword_71390;
        eng1_guard_worklist_table_read(assets, index, 8UL, 8UL,
                                       "0x277e0 middle coordinate image record");
        *(unsigned long *)(void *)(actor + 0x64) =
            *(unsigned long *)(void *)(assets + offset);
    }
    return allocated;
}

static void *create_without_owner(long x, long y, long z,
                                  unsigned short kind)
{
    void *actor;
    unsigned long selector = kind;

    if ((selector & 0x6000UL) == 0UL) {
        actor = spawn_normal(x, y, z, kind);
        if (actor != 0)
            call_action(g_configuration_dword_71370, selector, 12UL,
                        "0x300c0 normal callback", actor);
        return actor;
    }

    if ((selector & 0x4000UL) != 0UL) {
        /* This is the distinct coordinate/middle factory 0x277e0, not
         * worklist factory 0x27720. */
        actor = spawn_middle_277e0(x, y, z, (unsigned short)selector);
        if (actor != 0)
            call_action(g_configuration_dword_7137c, selector, 8UL,
                        "0x300c0 middle callback", actor);
        return actor;
    }

    actor = spawn_kind(x, y, z, kind);
    if (actor != 0)
        call_action(g_configuration_dword_7139c, selector, 8UL,
                    "0x300c0 alternate callback", actor);
    return actor;
}

void *__cdecl spawn_at_group_300c0(long x, long y, long z,
                                     unsigned short kind,
                                     unsigned short ignored_fifth)
{
    (void)ignored_fifth;
    return create_without_owner(x, y, z, kind);
}

/* Known direct callers treat this service as a command.  On a successful
 * callback path native EAX happens to retain the callback's raw return value,
 * that value is not an actor result. Three wrapper paths propagate EAX;
 * their indirect consumers remain outside the verified void-return contract. */
void __cdecl spawn_actor_with_owner(void *owner, long x, long y, long z,
                                    unsigned short kind)
{
    void *actor;
    unsigned long selector = kind;

    if ((selector & 0x6000UL) == 0UL) {
        actor = spawn_normal(x, y, z, kind);
        if (actor != 0) {
            copy_owner_bytes(owner, actor);
            append_chain(owner, actor);
            call_action(g_configuration_dword_71370, selector, 12UL,
                        "0x30180 normal callback", actor);
        }
        return;
    }

    if ((selector & 0x4000UL) != 0UL) {
        actor = spawn_middle_277e0(x, y, z, (unsigned short)selector);
        if (actor != 0) {
            copy_owner_bytes(owner, actor);
            call_action(g_configuration_dword_7137c, selector, 8UL,
                        "0x30180 middle callback", actor);
        }
        return;
    }

    actor = spawn_kind(x, y, z, kind);
    if (actor != 0)
        call_action(g_configuration_dword_7139c, selector, 8UL,
                    "0x30180 alternate callback", actor);
    return;
}
