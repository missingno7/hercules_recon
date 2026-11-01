/* PC-derived pending-action registry helpers; descriptive names only. */
typedef union PendingActionGroup PendingActionGroup;

union PendingActionGroup {
    long count;
    void *entries[1];
};

#if defined(HERCULES_ENGINE3)
extern unsigned short g_2bf38;
#else
extern unsigned short g_configuration_word_71380;
#endif
#if defined(HERCULES_ENGINE3)
extern PendingActionGroup *g_2bfa0[];
#else
extern PendingActionGroup *pending_action_group_table[];
#endif
extern int remove_actor_variant(void *actor);

PendingActionGroup *acquire_pending_action_group(void)
{
    unsigned short index;
#if defined(HERCULES_ENGINE3)
    unsigned short count = g_2bf38;
#else
    unsigned short count = g_configuration_word_71380;
#endif

    for (index = 0; index < count; ++index) {
#if defined(HERCULES_ENGINE3)
        if (g_2bfa0[index]->count == -1) {
#else
        if (pending_action_group_table[index]->count == -1) {
#endif
#if defined(HERCULES_ENGINE3)
            g_2bfa0[index]->count = 0;
#else
            pending_action_group_table[index]->count = 0;
#endif
#if defined(HERCULES_ENGINE3)
            return g_2bfa0[index];
#else
            return pending_action_group_table[index];
#endif
        }
    }
    return 0;
}

void release_pending_action(PendingActionGroup *group)
{
    unsigned short group_index;

#if defined(HERCULES_ENGINE3)
    for (group_index = 0; group_index < g_2bf38; ++group_index) {
#else
    for (group_index = 0; group_index < g_configuration_word_71380; ++group_index) {
#endif
#if defined(HERCULES_ENGINE3)
        PendingActionGroup **slot = &g_2bfa0[group_index];
#else
        PendingActionGroup **slot = &pending_action_group_table[group_index];
#endif
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
