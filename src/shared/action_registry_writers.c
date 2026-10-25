/* PC-derived pending-action group lifecycle helpers; descriptive names only. */
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

extern unsigned short pending_action_group_count;
extern PendingActionGroup *pending_action_group_table[];
extern unsigned short pending_action_group_limit;
extern Callback12 *pending_action_callbacks;
extern Callback8 *alternate_action_callbacks;
extern Actor *spawn_normal(long, long, long, int);
extern Actor *spawn_kind(long, long, long, int);

PendingActionGroup *retire_pending_action_group(PendingActionGroup *group)
{
    unsigned short index;
    PendingActionGroup *entry;
    PendingActionGroup *result = group;

    for (index = 0; index < pending_action_group_count; ++index) {
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

    if (group->count < pending_action_group_limit) {
        actor = spawn_normal(x, y, z, kind);
        if (actor) {
            pending_action_callbacks[kind & 0x1ff].initialize(actor);
            group->entries[++group->count] = actor;
            return actor;
        }
    }
    return 0;
}

Actor *append_kind_action(PendingActionGroup *group,
                          long x, long y, long z, int kind)
{
    Actor *actor;

    if (group->count < pending_action_group_limit) {
        actor = spawn_kind(x, y, z, kind);
        if (actor) {
            alternate_action_callbacks[kind & 0x1ff].initialize(actor);
            group->entries[++group->count] = actor;
            return actor;
        }
    }
    return 0;
}
