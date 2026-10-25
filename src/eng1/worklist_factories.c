/* Ordinary-C behavioral recovery of ENG1 0x270c0, 0x27720, and 0x27c40.
 * These are the row-bound worklist factories called by 0x262a0/0x26480.
 * They are distinct from coordinate/kind factories 0x271d0 and 0x27db0.
 *
 * Pool allocation and object initialization remain calls to their existing
 * source providers. The resource owner, configuration-selected asset tables,
 * and callback tables are read through their already-owned cells. Exact
 * registered source prefixes fail visibly when an access leaves the measured
 * bytes. Unknown runtime pointers retain native unchecked indexing; these
 * prefixes are not runtime-capacity claims.
 */
#include "worklist_factory_types.h"
#include "resource_runtime_data.h"
#include "worklist_read_guards.h"

extern ActorNormal *__cdecl allocate_normal_actor(ActorKind kind);
extern ActorMiddle *__cdecl allocate_middle_actor(ActorKind kind);
extern ActorCore *__cdecl allocate_alternate_actor(ActorKind kind);
extern void __cdecl initialize_normal_actor(ActorNormal *actor);
extern void __cdecl initialize_middle_actor(ActorMiddle *actor);
extern void __cdecl initialize_alternate_actor(ActorCore *actor);

/* These are existing pointer-valued configuration cells at ENG1 0x713xx,
 * declared as DWORDs by frame_data.c. Their selected pointees are not owned
 * or enlarged here. */
extern unsigned long g_configuration_dword_71388;
extern unsigned long g_configuration_dword_71370;
extern unsigned long g_configuration_dword_71390;
extern unsigned long g_configuration_dword_7137c;
extern unsigned long g_configuration_dword_713a4;
extern unsigned long g_configuration_dword_7139c;

static unsigned char *actor_bytes(void *actor)
{
    return (unsigned char *)actor;
}

static unsigned long load_u32(const unsigned char *p)
{
    return *(const unsigned long *)p;
}

static void store_u8(unsigned char *p, unsigned char value)
{
    *p = value;
}

static void store_u16(unsigned char *p, unsigned short value)
{
    *(unsigned short *)p = value;
}

static void store_u32(unsigned char *p, unsigned long value)
{
    *(unsigned long *)p = value;
}

static void mark_worklist_record(Eng1ForestRecord20 *record)
{
    /* The native OR is exactly at byte +0x0d, the high byte of flags. */
    ((unsigned char *)record)[0x0d] |= 0x80U;
}

static void *group_owner(unsigned long group_index)
{
    unsigned char *row;
    eng1_guard_worklist_group_owner(group_index);
    row = (unsigned char *)g_kind_info + group_index * 8UL;
    return (void *)(unsigned long)load_u32(row);
}

static unsigned char *configuration_table(unsigned long cell)
{
    return (unsigned char *)(unsigned long)cell;
}

static void call_action(unsigned long callback_cell,
                        unsigned long byte_offset,
                        unsigned long callback_stride,
                        void *actor)
{
    Eng1ActorActionCallback *table =
        (Eng1ActorActionCallback *)configuration_table(callback_cell);
    eng1_guard_worklist_table_read(table, byte_offset / callback_stride,
                                   callback_stride,
                                   4UL, "worklist factory action callback");
    table[byte_offset / 4UL](actor);
}

static unsigned long pointer_delta_from_owner(Eng1ForestRecord20 *record,
                                               unsigned long group_index)
{
    /* Native subtracts raw 32-bit addresses, not C object pointers. */
    return (unsigned long)record - (unsigned long)group_owner(group_index);
}

static unsigned long inherited_actor_state(unsigned char *actor,
                                            const Eng1ForestRecord20 *record)
{
    unsigned long initialized = load_u32(actor + 0x54);
    unsigned long inherited = (unsigned long)(actor[0x95] & 0x20U);
    /* Native negates the low byte containing flag bit 12, then masks the
     * resulting 0xff/0 byte with 0x10 before ORing this DWORD. */
    inherited |= (record->flags & 0x1000U) ? 0x10UL : 0UL;
    return initialized | inherited;
}

ActorNormal *__cdecl eng1_call_270c0(Eng1ForestRecord20 *record,
                                    unsigned long group_index)
{
    unsigned short kind = record->kind;
    unsigned short asset_kind;
    unsigned char *assets;
    ActorNormal *allocated = allocate_normal_actor(kind);

    if (allocated != 0) {
        unsigned char *actor;
        unsigned long state;

        initialize_normal_actor(allocated);
        actor = actor_bytes(allocated);
        mark_worklist_record(record);

        store_u16(actor + 0x2e, kind);
        store_u32(actor + 0x00, (unsigned long)record->x);
        store_u32(actor + 0x9c, (unsigned long)record->x);
        store_u32(actor + 0x04, (unsigned long)record->y);
        store_u32(actor + 0xa0, (unsigned long)record->y);
        store_u32(actor + 0x08, (unsigned long)record->z);
        store_u32(actor + 0xa4, (unsigned long)record->z);
        store_u16(actor + 0x2a, record->state);
        store_u8(actor + 0x94,
                 (unsigned char)((record->flags & 0x1000U) ? 0xffU : 0U));
        state = inherited_actor_state(actor, record);
        store_u32(actor + 0x54, state);

        assets = configuration_table(g_configuration_dword_71388);
        asset_kind = record->kind;
        eng1_guard_worklist_table_read(assets, asset_kind, 16UL, 16UL,
                                       "0x270c0 normal asset record");
        store_u8(actor + 0x22, assets[(unsigned long)asset_kind * 16UL + 0x0c]);
        /* Native reloads the record kind and root after each actor store.
         * The callback below still uses the kind captured before allocation. */
        asset_kind = record->kind;
        assets = configuration_table(g_configuration_dword_71388);
        eng1_guard_worklist_table_read(assets, asset_kind, 16UL, 16UL,
                                       "0x270c0 normal asset record");
        store_u32(actor + 0x64, load_u32(assets + (unsigned long)asset_kind * 16UL));
        asset_kind = record->kind;
        assets = configuration_table(g_configuration_dword_71388);
        eng1_guard_worklist_table_read(assets, asset_kind, 16UL, 16UL,
                                       "0x270c0 normal asset record");
        store_u32(actor + 0x110,
                  load_u32(assets + (unsigned long)asset_kind * 16UL + 4));
        asset_kind = record->kind;
        assets = configuration_table(g_configuration_dword_71388);
        eng1_guard_worklist_table_read(assets, asset_kind, 16UL, 16UL,
                                       "0x270c0 normal asset record");
        store_u32(actor + 0x118,
                  load_u32(assets + (unsigned long)asset_kind * 16UL + 8));
        store_u16(actor + 0x38, (unsigned short)group_index);
        store_u32(actor + 0x58,
                  pointer_delta_from_owner(record, group_index));

        call_action(g_configuration_dword_71370,
                    (unsigned long)kind * 12UL, 12UL, allocated);
    }
    return allocated;
}

ActorMiddle *__cdecl eng1_call_27720(Eng1ForestRecord20 *record,
                                    unsigned long group_index)
{
    unsigned short kind = record->kind;
    unsigned short asset_index = kind & 0x0fffU;
    unsigned long asset_offset = (unsigned long)asset_index * 8UL;
    unsigned char *assets;
    ActorMiddle *allocated = allocate_middle_actor(kind);

    if (allocated != 0) {
        unsigned char *actor;
        unsigned long state;

        initialize_middle_actor(allocated);
        actor = actor_bytes(allocated);
        mark_worklist_record(record);

        store_u16(actor + 0x2e, kind);
        store_u32(actor + 0x00, (unsigned long)record->x);
        store_u32(actor + 0x9c, (unsigned long)record->x);
        store_u32(actor + 0x04, (unsigned long)record->y);
        store_u32(actor + 0xa0, (unsigned long)record->y);
        store_u32(actor + 0x08, (unsigned long)record->z);
        store_u32(actor + 0xa4, (unsigned long)record->z);
        store_u8(actor + 0x94,
                 (unsigned char)((record->flags & 0x1000U) ? 0xffU : 0U));
        state = inherited_actor_state(actor, record);
        store_u32(actor + 0x54, state);

        store_u16(actor + 0x38, (unsigned short)group_index);
        store_u32(actor + 0x58,
                  pointer_delta_from_owner(record, group_index));
        assets = configuration_table(g_configuration_dword_71390);
        eng1_guard_worklist_table_read(assets, asset_index, 8UL, 8UL,
                                       "0x27720 middle asset record");
        store_u8(actor + 0x22, assets[asset_offset + 6]);
        assets = configuration_table(g_configuration_dword_71390);
        eng1_guard_worklist_table_read(assets, asset_index, 8UL, 8UL,
                                       "0x27720 middle asset record");
        store_u32(actor + 0x64, load_u32(assets + asset_offset));

        call_action(g_configuration_dword_7137c, asset_offset, 8UL,
                    allocated);
    }
    return allocated;
}

ActorCore *__cdecl eng1_call_27c40(Eng1ForestRecord20 *record,
                                  unsigned long group_index)
{
    unsigned short kind = record->kind;
    unsigned short asset_index = kind & 0x0fffU;
    unsigned long asset_offset = (unsigned long)asset_index * 8UL;
    unsigned char *assets;
    ActorCore *allocated = allocate_alternate_actor(kind);

    if (allocated != 0) {
        unsigned char *actor;

        initialize_alternate_actor(allocated);
        actor = actor_bytes(allocated);
        mark_worklist_record(record);

        store_u16(actor + 0x2e, kind);
        store_u32(actor + 0x00, (unsigned long)record->x);
        store_u32(actor + 0x04, (unsigned long)record->y);
        store_u32(actor + 0x08, (unsigned long)record->z);
        store_u16(actor + 0x38, (unsigned short)group_index);
        store_u32(actor + 0x58,
                  pointer_delta_from_owner(record, group_index));
        assets = configuration_table(g_configuration_dword_713a4);
        eng1_guard_worklist_table_read(assets, asset_index, 8UL, 8UL,
                                       "0x27c40 alternate asset record");
        store_u8(actor + 0x22, assets[asset_offset + 6]);
        assets = configuration_table(g_configuration_dword_713a4);
        eng1_guard_worklist_table_read(assets, asset_index, 8UL, 8UL,
                                       "0x27c40 alternate asset record");
        store_u32(actor + 0x64, load_u32(assets + asset_offset));

        call_action(g_configuration_dword_7139c, asset_offset, 8UL,
                    allocated);
    }
    return allocated;
}
