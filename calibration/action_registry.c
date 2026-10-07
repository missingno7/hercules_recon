/* PC-derived pending-action registry helpers; descriptive names only. */
typedef union PendingActionGroup PendingActionGroup;

union PendingActionGroup {
    long count;
    void *entries[1];
};

extern unsigned short pending_action_group_count;
extern PendingActionGroup *pending_action_group_table[];
extern int remove_actor_variant(void *actor);

PendingActionGroup *acquire_pending_action_group(void)
{
    unsigned short index;
    unsigned short count = pending_action_group_count;

    for (index = 0; index < count; ++index) {
        if (pending_action_group_table[index]->count == -1) {
            pending_action_group_table[index]->count = 0;
            return pending_action_group_table[index];
        }
    }
    return 0;
}

void release_pending_action(PendingActionGroup *group)
{
    unsigned short group_index;

    for (group_index = 0; group_index < pending_action_group_count; ++group_index) {
        PendingActionGroup **slot = &pending_action_group_table[group_index];
        if (*slot == group) {
            long initial_count = group->count;
            if (initial_count != -1) {
                unsigned short item_index = 0;
                if (initial_count > 0) {
                    do {
                        ++item_index;
                        remove_actor_variant(group->entries[item_index]);
                    } while (item_index < group->count);
                }
                (*slot)->count = -1;
            }
        }
    }
}
