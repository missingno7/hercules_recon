/* ENG1 2b030/2b070/2b0d0: retire and append pending actions.
 * Existing native-backed union view: slot zero overlays the signed count.
 * Callback roots are the actual DWORD configuration address owners. Reload
 * the count after the callback and wrap its increment in32 bits explicitly.
 * entries[1] is a prefix view; no group/table allocation capacity is inferred.
 * Retirement retains its existing indexed registry dependency, not accepted
 * as a complete native owner by the appender proofs.
 */
typedef char action_count_is_32[sizeof(long)==4?1:-1];
typedef struct Actor Actor;
typedef union PendingActionGroup PendingActionGroup;

union PendingActionGroup {
    long count;
    Actor *entries[1];
};

typedef struct Callback12 Callback12;
typedef struct Callback8 Callback8;

struct Callback12 {
    void (*initialize)(Actor *);
    long unknown[2];
};

struct Callback8 {
    void (*initialize)(Actor *);
    long unknown;
};

extern unsigned short g_configuration_word_71380;
extern PendingActionGroup *pending_action_group_table[];
extern unsigned short g_configuration_word_71394;
extern unsigned long g_configuration_dword_71370;
#define NORMAL_ACTION_CALLBACKS ((Callback12 *)g_configuration_dword_71370)
extern unsigned long g_configuration_dword_7139c;
#define ALTERNATE_ACTION_CALLBACKS ((Callback8 *)g_configuration_dword_7139c)
extern Actor *spawn_normal(long, long, long, int);
extern Actor *spawn_kind(long, long, long, int);

PendingActionGroup *retire_pending_action_group(PendingActionGroup *group)
{
    unsigned short index;
    PendingActionGroup *entry;
    PendingActionGroup *result = group;

    for (index = 0; index < g_configuration_word_71380; ++index) {
        entry = pending_action_group_table[index];
        if (entry == group) {
            entry->count = -1;
            result = 0;
        }
    }
    return result;
}

Actor *append_normal_action(PendingActionGroup *group,
                            long x, long y, long z, int kind)
{
    Actor *actor;

    if (group->count < g_configuration_word_71394) {
        actor = spawn_normal(x, y, z, kind);
        if (actor) {
            NORMAL_ACTION_CALLBACKS[kind & 0x1ff].initialize(actor);
            group->entries[group->count = (long)((unsigned long)group->count + 1)] = actor;
            return actor;
        }
    }
    return 0;
}

Actor *append_kind_action(PendingActionGroup *group,
                          long x, long y, long z, int kind)
{
    Actor *actor;

    if (group->count < g_configuration_word_71394) {
        actor = spawn_kind(x, y, z, kind);
        if (actor) {
            ALTERNATE_ACTION_CALLBACKS[kind & 0x1ff].initialize(actor);
            group->entries[group->count = (long)((unsigned long)group->count + 1)] = actor;
            return actor;
        }
    }
    return 0;
}

/* Unit-local prefix view from native offsets, avoiding unrelated interpreter APIs.
 * The cached position sums select actor-field carriers with this real prefix
 * context. Measured VC5 /O2 then consumes/reloads the actor register for Z and
 * schedules the callback root before its index, matching the native function.
 * Declaration placement deliberately preserves the three writer peers above;
 * this is not a claim to have recovered an original header or unit identity. */
#include "secondary_records.h"
struct Actor {
    long x, y, z;
    unsigned char pad00c[0x2a-0x0c];
    unsigned short unknown_02a;
    unsigned char pad02c[0x34-0x2c];
    short active_command;
    unsigned char pad036[0x118-0x36];
    void *initial_steps;
    unsigned char pad11c[4];
    PendingActionGroup *action_group;
};
extern PendingActionGroup *acquire_pending_action_group(void);
extern void release_pending_action(PendingActionGroup *group);
extern Actor *spawn_middle_277e0(long x, long y, long z, unsigned short kind);
typedef Actor *(__cdecl *Eng1ShortFactory)(long x, long y, long z, unsigned short kind);
extern unsigned long g_configuration_dword_7137c;
int eng1_native_code_target_002b130(Actor *actor)
{
    unsigned short kind;
    unsigned long record_index;
    Eng1SpawnRecord *records;
    PendingActionGroup *group;
    long z;
    long y;
    long x;
    unsigned short class_bits;
    unsigned short sprite = 0;
    Actor *child;

    records = (Eng1SpawnRecord *)actor->initial_steps;
    if (records != 0) {
        group = acquire_pending_action_group();
        if (group != 0) {

            records = (Eng1SpawnRecord *)actor->initial_steps;
            actor->action_group = group;
            for (record_index = 0; (kind = records[record_index].kind) != 0; ++record_index) {

                child = 0;
                if (group->count < (long)g_configuration_word_71394) {
                    class_bits = (unsigned short)(kind & 0x7000U);
                    x = (long)((unsigned long)actor->x +
                        ((unsigned long)(long)records[record_index].x << 16));
                    y = (long)((unsigned long)actor->y +
                        ((unsigned long)(long)records[record_index].y << 16));
                    z = (long)((unsigned long)actor->z +
                        ((unsigned long)(long)records[record_index].z << 16));
                    sprite = records[record_index].sprite;

                    if (class_bits == 0) {
                        child = ((Eng1ShortFactory)spawn_normal)(x, y, z, kind);
                        if (child != 0) {
                            records = (Eng1SpawnRecord *)actor->initial_steps;
                            child->unknown_02a = records[record_index].state;
                            NORMAL_ACTION_CALLBACKS[kind & 0x01ffU].initialize(child);
                        }
                    }

                    if (class_bits == 0x4000U) {
                        child = spawn_middle_277e0(x, y, z, kind);
                        if (child != 0) {
                            records = (Eng1SpawnRecord *)actor->initial_steps;
                            child->unknown_02a = records[record_index].state;
                            ((Callback8 *)g_configuration_dword_7137c)
                                [kind & 0x01ffU].initialize(child);
                        }
                    }

                    if (class_bits == 0x2000U) {
                        child = ((Eng1ShortFactory)spawn_kind)(x, y, z, kind);
                        if (child != 0) {
                            records = (Eng1SpawnRecord *)actor->initial_steps;
                            child->unknown_02a = records[record_index].state;
                            ALTERNATE_ACTION_CALLBACKS[kind & 0x01ffU]
                                .initialize(child);
                        }
                    }

                }
                if (child == 0) {
                    release_pending_action(group);
                    actor->action_group = 0;
                    return 0;
                }

                group->entries[group->count =
                    (long)((unsigned long)group->count + 1UL)] = child;
                if (sprite != 0)
                    child->active_command = (short)sprite;

                records = (Eng1SpawnRecord *)actor->initial_steps;
            }
            return 1;

        }
    }
    return 0;
}
