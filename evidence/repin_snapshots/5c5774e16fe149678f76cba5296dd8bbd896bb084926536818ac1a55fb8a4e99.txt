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
