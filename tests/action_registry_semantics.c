#include <stdio.h>
#include <string.h>
#include "../calibration/action_registry.c"

unsigned short pending_action_group_count;
PendingActionGroup *pending_action_group_table[16];

typedef union TestGroupStorage TestGroupStorage;
union TestGroupStorage {
    long count;
    void *entries[16];
};

static PendingActionGroup *observed_group;
static void *expected_items[16];
static long expected_counts[16];
static void *callback_items[16];
static int expected_call_limit;
static int callback_count;
static int callback_mutate_after;
static long callback_new_count;
static int callback_replace_slot;
static PendingActionGroup *callback_replacement;
static int callback_registry_count;
static int checks;
static int failures;

#define CHECK(v) do { ++checks; if (!(v)) { ++failures; if (failures < 12) printf("FAIL line %d\n", __LINE__); } } while (0)

static PendingActionGroup *as_group(TestGroupStorage *p)
{
    return (PendingActionGroup *)p;
}

static void clear_world(void)
{
    int i;
    pending_action_group_count = 0;
    for (i = 0; i < 16; ++i) pending_action_group_table[i] = 0;
    observed_group = 0;
    expected_call_limit = 0;
    callback_count = 0;
    callback_mutate_after = -1;
    callback_new_count = 0;
    callback_replace_slot = -1;
    callback_replacement = 0;
    callback_registry_count = -1;
    for (i = 0; i < 16; ++i) {
        expected_items[i] = 0;
        expected_counts[i] = 0;
        callback_items[i] = 0;
    }
}

static void expect_callbacks(PendingActionGroup *group, int count)
{
    observed_group = group;
    expected_call_limit = count;
    callback_count = 0;
}

static void check_callbacks(int expected)
{
    int i;
    CHECK(callback_count == expected);
    for (i = 0; i < expected && i < callback_count; ++i)
        CHECK(callback_items[i] == expected_items[i]);
}

int remove_actor_variant(void *actor)
{
    int n = callback_count;
    CHECK(n < 16);
    if (n < 16) {
        CHECK(n < expected_call_limit);
        CHECK(actor == expected_items[n]);
        if (observed_group) CHECK(observed_group->count == expected_counts[n]);
        callback_items[n] = actor;
        if (n == callback_mutate_after && observed_group)
            observed_group->count = callback_new_count;
        if (n == 0 && callback_replace_slot >= 0)
            pending_action_group_table[callback_replace_slot] = callback_replacement;
        if (n == 0 && callback_registry_count >= 0)
            pending_action_group_count = (unsigned short)callback_registry_count;
    }
    ++callback_count;
    return 7;
}

int main(void)
{
    TestGroupStorage slots[4];
    PendingActionGroup *result;
    int item_a, item_b, item_c;

    clear_world();
    memset(slots, 0, sizeof(slots));
    pending_action_group_count = 3;
    slots[0].count = 2;
    slots[1].count = -1;
    slots[2].count = -1;
    pending_action_group_table[0] = as_group(&slots[0]);
    pending_action_group_table[1] = as_group(&slots[1]);
    pending_action_group_table[2] = as_group(&slots[2]);
    result = acquire_pending_action_group();
    CHECK(result == as_group(&slots[1]));
    CHECK(slots[1].count == 0);
    CHECK(slots[2].count == -1);

    clear_world();
    memset(slots, 0, sizeof(slots));
    pending_action_group_count = 2;
    slots[0].count = 0;
    slots[1].count = 3;
    pending_action_group_table[0] = as_group(&slots[0]);
    pending_action_group_table[1] = as_group(&slots[1]);
    CHECK(acquire_pending_action_group() == 0);
    CHECK(slots[0].count == 0 && slots[1].count == 3);

    clear_world();
    memset(slots, 0, sizeof(slots));
    pending_action_group_count = 3;
    slots[0].count = 1;
    slots[1].count = -1;
    slots[2].count = -1;
    pending_action_group_table[0] = as_group(&slots[0]);
    pending_action_group_table[1] = as_group(&slots[1]);
    pending_action_group_table[2] = as_group(&slots[2]);
    result = acquire_pending_action_group();
    CHECK(result == as_group(&slots[1]) && slots[1].count == 0);
    slots[1].count = 2;
    slots[1].entries[1] = &item_a;
    slots[1].entries[2] = &item_b;
    pending_action_group_count = 1;
    pending_action_group_table[0] = result;
    expected_items[0] = &item_a;
    expected_items[1] = &item_b;
    expected_counts[0] = 2;
    expected_counts[1] = 2;
    expect_callbacks(result, 2);
    release_pending_action(result);
    check_callbacks(2);
    CHECK(result->count == -1);
    CHECK(acquire_pending_action_group() == result);
    CHECK(result->count == 0);

    clear_world();
    memset(slots, 0, sizeof(slots));
    slots[0].count = 3;
    slots[0].entries[1] = &item_a;
    slots[0].entries[2] = &item_b;
    slots[0].entries[3] = &item_c;
    pending_action_group_count = 4;
    pending_action_group_table[0] = as_group(&slots[1]);
    pending_action_group_table[1] = as_group(&slots[0]);
    pending_action_group_table[2] = as_group(&slots[0]);
    pending_action_group_table[3] = as_group(&slots[2]);
    expected_items[0] = &item_a;
    expected_items[1] = &item_b;
    expected_items[2] = &item_c;
    expected_counts[0] = 3;
    expected_counts[1] = 3;
    expected_counts[2] = 3;
    expect_callbacks(as_group(&slots[0]), 3);
    release_pending_action(as_group(&slots[0]));
    check_callbacks(3);
    CHECK(slots[0].count == -1);
    CHECK(pending_action_group_table[1] == pending_action_group_table[2]);

    clear_world();
    memset(slots, 0, sizeof(slots));
    slots[0].count = 2;
    slots[0].entries[1] = &item_a;
    slots[0].entries[2] = &item_b;
    slots[1].count = 0;
    slots[2].count = 0;
    pending_action_group_count = 2;
    pending_action_group_table[0] = as_group(&slots[1]);
    pending_action_group_table[1] = as_group(&slots[2]);
    expect_callbacks(as_group(&slots[0]), 0);
    release_pending_action(as_group(&slots[0]));
    check_callbacks(0);
    CHECK(slots[0].count == 2);

    clear_world();
    memset(slots, 0, sizeof(slots));
    pending_action_group_count = 1;
    slots[0].count = -1;
    pending_action_group_table[0] = as_group(&slots[0]);
    expect_callbacks(as_group(&slots[0]), 0);
    release_pending_action(as_group(&slots[0]));
    check_callbacks(0);
    CHECK(slots[0].count == -1);

    clear_world();
    memset(slots, 0, sizeof(slots));
    pending_action_group_count = 1;
    slots[0].count = 0;
    pending_action_group_table[0] = as_group(&slots[0]);
    expect_callbacks(as_group(&slots[0]), 0);
    release_pending_action(as_group(&slots[0]));
    check_callbacks(0);
    CHECK(slots[0].count == -1);

    clear_world();
    memset(slots, 0, sizeof(slots));
    pending_action_group_count = 1;
    slots[0].count = -2;
    pending_action_group_table[0] = as_group(&slots[0]);
    expect_callbacks(as_group(&slots[0]), 0);
    release_pending_action(as_group(&slots[0]));
    check_callbacks(0);
    CHECK(slots[0].count == -1);

    clear_world();
    memset(slots, 0, sizeof(slots));
    pending_action_group_count = 1;
    slots[0].count = 3;
    slots[0].entries[1] = &item_a;
    slots[0].entries[2] = &item_b;
    slots[0].entries[3] = &item_c;
    pending_action_group_table[0] = as_group(&slots[0]);
    expected_items[0] = &item_a;
    expected_counts[0] = 3;
    expect_callbacks(as_group(&slots[0]), 1);
    callback_mutate_after = 0;
    callback_new_count = 1;
    release_pending_action(as_group(&slots[0]));
    check_callbacks(1);
    CHECK(slots[0].count == -1);

    clear_world();
    memset(slots, 0, sizeof(slots));
    pending_action_group_count = 1;
    slots[0].count = 1;
    slots[0].entries[1] = &item_a;
    slots[0].entries[2] = &item_b;
    slots[0].entries[3] = &item_c;
    pending_action_group_table[0] = as_group(&slots[0]);
    expected_items[0] = &item_a;
    expected_items[1] = &item_b;
    expected_items[2] = &item_c;
    expected_counts[0] = 1;
    expected_counts[1] = 3;
    expected_counts[2] = 3;
    expect_callbacks(as_group(&slots[0]), 3);
    callback_mutate_after = 0;
    callback_new_count = 3;
    release_pending_action(as_group(&slots[0]));
    check_callbacks(3);
    CHECK(slots[0].count == -1);

    /* The PC reloads the matched table slot after callbacks, then reloads the
     * registry count at the outer-loop test. These fixtures deliberately mutate
     * each location independently; they do not assert production aliasing. */
    clear_world();
    memset(slots, 0, sizeof(slots));
    pending_action_group_count = 1;
    slots[0].count = 1;
    slots[0].entries[1] = &item_a;
    slots[1].count = 7;
    pending_action_group_table[0] = as_group(&slots[0]);
    expected_items[0] = &item_a;
    expected_counts[0] = 1;
    expect_callbacks(as_group(&slots[0]), 1);
    callback_replace_slot = 0;
    callback_replacement = as_group(&slots[1]);
    release_pending_action(as_group(&slots[0]));
    check_callbacks(1);
    CHECK(slots[0].count == 1);
    CHECK(slots[1].count == -1);

    clear_world();
    memset(slots, 0, sizeof(slots));
    pending_action_group_count = 2;
    slots[0].count = 1;
    slots[0].entries[1] = &item_a;
    pending_action_group_table[0] = as_group(&slots[0]);
    pending_action_group_table[1] = as_group(&slots[0]);
    expected_items[0] = &item_a;
    expected_counts[0] = 1;
    expect_callbacks(as_group(&slots[0]), 1);
    callback_registry_count = 0;
    release_pending_action(as_group(&slots[0]));
    check_callbacks(1);
    CHECK(slots[0].count == -1);
    CHECK(pending_action_group_count == 0);

    printf("%d checks; %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
