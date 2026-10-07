/* PC-derived registry allocation and generic host-interface forwarding. */
typedef union PendingActionGroup PendingActionGroup;

union PendingActionGroup {
    long count;
    void *entries[1];
};

typedef void *(__cdecl *HostAllocationCallback)(void **, unsigned int,
                                                 unsigned int);

extern unsigned short pending_action_group_count;
extern PendingActionGroup *pending_action_group_table[];
extern unsigned short pending_action_group_limit;
extern HostAllocationCallback host_allocation_callback;

void *forward_host_allocation(void **slot, unsigned int bytes,
                              unsigned int flags)
{
    return host_allocation_callback(slot, bytes, flags);
}

int initialize_pending_action_groups(void)
{
    int index = 0;
    PendingActionGroup **slot = pending_action_group_table;

    while (index < pending_action_group_count) {
        if (!forward_host_allocation(
                (void **)slot,
                (unsigned int)pending_action_group_limit << 2,
                0))
            return 0;

        (*slot)->count = -1;
        ++index;
        ++slot;
    }

    return 1;
}
