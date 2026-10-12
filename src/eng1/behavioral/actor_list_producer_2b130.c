/* Ordinary-C behavioral reconstruction of ENG1 RVA 0x2b130.
 * The actor list at +0x118 and the acquired count-prefixed group at +0x120
 * are pointer views only; their dynamic capacities are not established here. */
#include "interpreters.h"
#include "secondary_records.h"

/* PendingActionGroup is the existing resource_runtime_data.h shared view. */
typedef PendingActionGroup Eng1PendingActionGroup;

typedef void (__cdecl *Eng1ActionCallback)(void *actor);

extern Eng1PendingActionGroup *__cdecl acquire_pending_action_group(void);
extern void __cdecl release_pending_action(Eng1PendingActionGroup *group);
extern void *__cdecl spawn_normal(long x, long y, long z,
                                  unsigned short kind);
extern void *__cdecl spawn_middle_277e0(long x, long y, long z,
                                         unsigned short kind);
extern void *__cdecl spawn_kind(long x, long y, long z,
                                unsigned short kind);
extern void eng1_guard_worklist_table_read(const void *base,
                                           unsigned long index,
                                           unsigned long stride,
                                           unsigned long width,
                                           const char *site);
extern unsigned short g_configuration_word_71394;
extern unsigned long g_configuration_dword_71370;
extern unsigned long g_configuration_dword_7137c;
extern unsigned long g_configuration_dword_7139c;

static volatile Eng1SpawnRecord *load_spawn_records(void *actor)
{
    unsigned long address = *(volatile unsigned long *)
        (void *)((unsigned char *)actor + 0x118);
    return (volatile Eng1SpawnRecord *)(void *)address;
}

static unsigned long coordinate_with_record_word(long coordinate,
                                                  short record_word)
{
    return (unsigned long)coordinate +
           ((unsigned long)(long)record_word << 16);
}

static long read_group_count(Eng1PendingActionGroup *group)
{
    return *(volatile long *)(void *)&group->count;
}

static void write_group_count(Eng1PendingActionGroup *group, long count)
{
    *(volatile long *)(void *)&group->count = count;
}

static void call_record_action(unsigned long table_cell,
                               unsigned short kind,
                               unsigned long stride,
                               const char *site,
                               void *child)
{
    unsigned char *table = (unsigned char *)(void *)table_cell;
    unsigned long index = (unsigned long)(kind & 0x01ffU);
    Eng1ActionCallback callback;

    eng1_guard_worklist_table_read(table, index, stride, 4UL, site);
    callback = *(Eng1ActionCallback *)(void *)(table + index * stride);
    callback(child);
}

int __cdecl eng1_native_code_target_002b130(Actor *actor)
{
    unsigned char *actor_bytes = (unsigned char *)actor;
    Eng1PendingActionGroup *group;
    volatile Eng1SpawnRecord *records;
    unsigned long record_offset;
    unsigned short kind;

    /* The two early exits do not modify actor+0x120. */
    if (load_spawn_records(actor) == 0)
        return 0;

    group = acquire_pending_action_group();
    if (group == 0)
        return 0;

    /* Native reloads +0x118 after acquisition, before installing +0x120. */
    records = load_spawn_records(actor);
    *(volatile unsigned long *)(void *)(actor_bytes + 0x120) =
        (unsigned long)(void *)group;
    kind = records[0].kind;
    if (kind == 0U)
        return 1;

    record_offset = 0UL;
    for (;;) {
        unsigned short capacity = g_configuration_word_71394;
        unsigned long x_bits;
        unsigned long y_bits;
        unsigned long z_bits;
        unsigned short sprite;
        unsigned short class_bits;
        void *child = 0;
        unsigned long new_count_bits;
        long new_count;

        /* Native performs a signed JGE against the zero-extended WORD. */
        if (read_group_count(group) >= (long)(unsigned short)capacity)
            goto rollback;

        /* These fields are captured before the factory or callback runs. */
        {
            short record_x = records[record_offset / sizeof(Eng1SpawnRecord)].x;
            long actor_x;
            class_bits = (unsigned short)(kind & 0x7000U);
            actor_x = *(volatile long *)(void *)(actor_bytes + 0x00);
            x_bits = coordinate_with_record_word(actor_x, record_x);
        }
        {
            short record_y = records[record_offset / sizeof(Eng1SpawnRecord)].y;
            long actor_y = *(volatile long *)(void *)(actor_bytes + 0x04);
            y_bits = coordinate_with_record_word(actor_y, record_y);
        }
        {
            short record_z = records[record_offset / sizeof(Eng1SpawnRecord)].z;
            long actor_z = *(volatile long *)(void *)(actor_bytes + 0x08);
            z_bits = coordinate_with_record_word(actor_z, record_z);
        }
        sprite = records[record_offset / sizeof(Eng1SpawnRecord)].sprite;

        if (class_bits == 0U) {
            child = spawn_normal((long)x_bits, (long)y_bits, (long)z_bits,
                                 kind);
            if (child != 0) {
                unsigned short state;
                records = load_spawn_records(actor);
                state = records[record_offset / sizeof(Eng1SpawnRecord)].state;
                *(unsigned short *)(void *)((unsigned char *)child + 0x2a) =
                    state;
                call_record_action(g_configuration_dword_71370, kind,
                                   12UL, "0x2b130 normal callback", child);
            }
        } else if (class_bits == 0x4000U) {
            child = spawn_middle_277e0((long)x_bits, (long)y_bits,
                                       (long)z_bits, kind);
            if (child != 0) {
                unsigned short state;
                records = load_spawn_records(actor);
                state = records[record_offset / sizeof(Eng1SpawnRecord)].state;
                *(unsigned short *)(void *)((unsigned char *)child + 0x2a) =
                    state;
                call_record_action(g_configuration_dword_7137c, kind,
                                   8UL, "0x2b130 middle callback", child);
            }
        } else if (class_bits == 0x2000U) {
            child = spawn_kind((long)x_bits, (long)y_bits, (long)z_bits,
                               kind);
            if (child != 0) {
                unsigned short state;
                records = load_spawn_records(actor);
                state = records[record_offset / sizeof(Eng1SpawnRecord)].state;
                *(unsigned short *)(void *)((unsigned char *)child + 0x2a) =
                    state;
                call_record_action(g_configuration_dword_7139c, kind,
                                   8UL, "0x2b130 alternate callback", child);
            }
        }

        if (child == 0)
            goto rollback;

        /* Reload count after callback, then store count and pointer before the
         * optional cached sprite write, matching the native order. */
        new_count_bits = (unsigned long)read_group_count(group) + 1UL;
        new_count = (long)new_count_bits;
        write_group_count(group, new_count);
        group->entries[new_count_bits] = child;
        if (sprite != 0U)
            *(unsigned short *)(void *)((unsigned char *)child + 0x34) = sprite;

        record_offset += (unsigned long)sizeof(Eng1SpawnRecord);
        records = load_spawn_records(actor);
        kind = records[record_offset / sizeof(Eng1SpawnRecord)].kind;
        if (kind == 0U)
            return 1;
    }

rollback:
    release_pending_action(group);
    *(volatile unsigned long *)(void *)(actor_bytes + 0x120) = 0UL;
    return 0;
}
