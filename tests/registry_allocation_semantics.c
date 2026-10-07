#include <stdio.h>
#include "../calibration/registry_allocation.c"

unsigned short pending_action_group_count;
PendingActionGroup *pending_action_group_table[8];
unsigned short pending_action_group_limit;
HostAllocationCallback host_allocation_callback;

static PendingActionGroup groups[8];
static unsigned int callback_calls;
static unsigned int checks;
static unsigned int failures;
static unsigned int expected_bytes[8];
static long expected_group_count[8];
static unsigned int fail_at;
static int allow_failure;
static int grow_on_first;
static int shrink_on_first;
static int change_limits;
static unsigned int return_marker;

static void check(int condition, char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL: %s\n", label);
    }
}

static void *__cdecl test_allocator(void **slot, unsigned int bytes,
                                     unsigned int flags)
{
    unsigned int call_index = callback_calls;
    unsigned int i;

    ++callback_calls;
    check(call_index < 8, "callback call index is within fixture");
    if (call_index >= 8)
        return 0;

    check(slot == (void **)&pending_action_group_table[call_index],
          "callback receives the address of the current pointer slot");
    check(bytes == expected_bytes[call_index],
          "callback receives current append limit shifted left two");
    check(flags == 0, "callback receives zero allocation flags");
    check(groups[call_index].count == expected_group_count[call_index],
          "current group's count is not changed before callback returns");
    for (i = 0; i < call_index; ++i)
        check(groups[i].count == -1,
              "prior successful group is marked before next callback");

    if (allow_failure && call_index == fail_at) {
        *slot = 0;
        return 0;
    }

    *slot = &groups[call_index];
    if (grow_on_first && call_index == 0)
        pending_action_group_count = 3;
    if (shrink_on_first && call_index == 0)
        pending_action_group_count = 1;
    if (change_limits && call_index == 0)
        pending_action_group_limit = 7;
    else if (change_limits && call_index == 1)
        pending_action_group_limit = 2;

    return &return_marker;
}

static void reset_fixture(void)
{
    unsigned int i;

    pending_action_group_count = 0;
    pending_action_group_limit = 0;
    callback_calls = 0;
    fail_at = 0xffffffffUL;
    allow_failure = 0;
    grow_on_first = 0;
    shrink_on_first = 0;
    change_limits = 0;
    return_marker = 0x12345678UL;
    host_allocation_callback = test_allocator;
    for (i = 0; i < 8; ++i) {
        pending_action_group_table[i] = 0;
        groups[i].count = 100 + i;
        expected_bytes[i] = 0;
        expected_group_count[i] = 100 + i;
    }
}

static void test_empty_registry(void)
{
    int result;

    reset_fixture();
    pending_action_group_count = 0;
    pending_action_group_table[0] = &groups[7];
    result = initialize_pending_action_groups();
    check(result == 1, "empty registry returns success");
    check(callback_calls == 0, "empty registry skips allocation callback");
    check(pending_action_group_table[0] == &groups[7],
          "empty registry leaves pointer slots unchanged");
}

static void test_growth_and_live_limit(void)
{
    int result;

    reset_fixture();
    pending_action_group_count = 1;
    pending_action_group_limit = 4;
    expected_bytes[0] = 16;
    expected_bytes[1] = 28;
    expected_bytes[2] = 8;
    grow_on_first = 1;
    change_limits = 1;

    result = initialize_pending_action_groups();
    check(result == 1, "grown registry completes successfully");
    check(callback_calls == 3,
          "callback growth of live count adds two allocation iterations");
    check(pending_action_group_table[0] == &groups[0] &&
          pending_action_group_table[1] == &groups[1] &&
          pending_action_group_table[2] == &groups[2],
          "each success uses the pointer written through its output slot");
    check(groups[0].count == -1 && groups[1].count == -1 &&
          groups[2].count == -1,
          "each returned group is marked -1 after its callback");
    check(return_marker == 0x12345678UL,
          "initializer uses callback EAX only as success and reloads the slot");
}

static void test_live_count_shrink(void)
{
    int result;

    reset_fixture();
    pending_action_group_count = 4;
    pending_action_group_limit = 3;
    expected_bytes[0] = 12;
    shrink_on_first = 1;

    result = initialize_pending_action_groups();
    check(result == 1, "shrunk registry completes successfully");
    check(callback_calls == 1,
          "callback shrink of live count stops the next iteration");
    check(groups[0].count == -1 &&
          pending_action_group_table[0] == &groups[0],
          "shrunk registry retains its initialized first group");
}

static void test_failure_preserves_prior_success(void)
{
    int result;

    reset_fixture();
    pending_action_group_count = 3;
    pending_action_group_limit = 5;
    pending_action_group_table[2] = &groups[7];
    expected_bytes[0] = 20;
    expected_bytes[1] = 20;
    allow_failure = 1;
    fail_at = 1;

    result = initialize_pending_action_groups();
    check(result == 0, "first callback failure returns zero");
    check(callback_calls == 2, "failure stops before later table slots");
    check(pending_action_group_table[0] == &groups[0] &&
          groups[0].count == -1,
          "earlier successful allocation remains initialized");
    check(pending_action_group_table[1] == 0,
          "flags-zero failure's output slot remains null");
    check(pending_action_group_table[2] == &groups[7],
          "failure leaves later slots untouched");
}

int main(void)
{
    test_empty_registry();
    test_growth_and_live_limit();
    test_live_count_shrink();
    test_failure_preserves_prior_success();

    printf("%u checks; %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
