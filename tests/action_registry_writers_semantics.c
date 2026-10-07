#include <stdio.h>
#include <string.h>
#include "../calibration/action_registry_writers.c"

struct Actor { int id; };

unsigned short pending_action_group_count;
PendingActionGroup *pending_action_group_table[16];
unsigned short pending_action_group_limit;

static Callback12 normal_rows[512];
static Callback8 alternate_rows[512];
Callback12 *pending_action_callbacks = normal_rows;
Callback8 *alternate_action_callbacks = alternate_rows;

typedef union TestGroupStorage TestGroupStorage;
union TestGroupStorage {
    long count;
    Actor *entries[32];
};

static struct Actor actors[8];
static Actor *spawn_result;
static int spawn_calls;
static int kind_spawn_calls;
static long spawn_args[3];
static int spawn_kind_arg;
static int callback_calls;
static Actor *callback_arg;
static PendingActionGroup *observed_group;
static long callback_observed_count;
static Actor *callback_observed_entry;
static long callback_override_count;
static int checks;
static int failures;

#define CHECK(value) do { ++checks; if (!(value)) { \
    ++failures; if (failures < 12) printf("FAIL line %d\n", __LINE__); \
} } while (0)

static PendingActionGroup *as_group(TestGroupStorage *storage)
{
    return (PendingActionGroup *)storage;
}

static void reset_world(void)
{
    int i;
    pending_action_group_count = 0;
    pending_action_group_limit = 0;
    for (i = 0; i < 16; ++i) pending_action_group_table[i] = 0;
    for (i = 0; i < 512; ++i) {
        normal_rows[i].initialize = 0;
        alternate_rows[i].initialize = 0;
    }
    spawn_result = 0;
    spawn_calls = 0;
    kind_spawn_calls = 0;
    spawn_args[0] = spawn_args[1] = spawn_args[2] = 0;
    spawn_kind_arg = 0;
    callback_calls = 0;
    callback_arg = 0;
    observed_group = 0;
    callback_observed_count = 0;
    callback_observed_entry = 0;
    callback_override_count = -1;
}

static void observe_callback(Actor *actor)
{
    ++callback_calls;
    callback_arg = actor;
    if (observed_group) {
        callback_observed_count = observed_group->count;
        callback_observed_entry = observed_group->entries[2];
        if (callback_override_count >= 0)
            observed_group->count = callback_override_count;
    }
}

Actor *spawn_normal(long x, long y, long z, int kind)
{
    ++spawn_calls;
    spawn_args[0] = x;
    spawn_args[1] = y;
    spawn_args[2] = z;
    spawn_kind_arg = kind;
    return spawn_result;
}

Actor *spawn_kind(long x, long y, long z, int kind)
{
    ++kind_spawn_calls;
    spawn_args[0] = x;
    spawn_args[1] = y;
    spawn_args[2] = z;
    spawn_kind_arg = kind;
    return spawn_result;
}

static void test_retirement(void)
{
    TestGroupStorage a, b;
    PendingActionGroup *pa = as_group(&a);
    PendingActionGroup *pb = as_group(&b);
    reset_world();
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    a.count = 4;
    b.count = 2;
    CHECK(retire_pending_action_group(pa) == pa);
    CHECK(a.count == 4);
    pending_action_group_count = 3;
    pending_action_group_table[0] = pb;
    pending_action_group_table[1] = pa;
    pending_action_group_table[2] = pa;
    CHECK(retire_pending_action_group(pa) == 0);
    CHECK(a.count == -1);
    CHECK(b.count == 2);
    pending_action_group_count = 0;
    a.count = 7;
    CHECK(retire_pending_action_group(pa) == pa);
    CHECK(a.count == 7);
}

static void test_append_normal(void)
{
    TestGroupStorage storage;
    PendingActionGroup *group = as_group(&storage);
    Actor *old_entry = &actors[0];
    Actor *created = &actors[1];
    reset_world();
    memset(&storage, 0, sizeof(storage));
    storage.count = 3;
    pending_action_group_limit = 3;
    CHECK(append_normal_action(group, 10, 20, 30, 0x34) == 0);
    CHECK(spawn_calls == 0 && callback_calls == 0 && storage.count == 3);

    storage.count = 1;
    storage.entries[1] = old_entry;
    spawn_result = 0;
    CHECK(append_normal_action(group, 10, 20, 30, 0x34) == 0);
    CHECK(spawn_calls == 1 && callback_calls == 0 && storage.count == 1);
    CHECK(storage.entries[1] == old_entry && storage.entries[2] == 0);
    CHECK(spawn_args[0] == 10 && spawn_args[1] == 20 && spawn_args[2] == 30);
    CHECK(spawn_kind_arg == 0x34);

    spawn_result = created;
    observed_group = group;
    normal_rows[0x34].initialize = observe_callback;
    CHECK(append_normal_action(group, -1, 2, -3, 0x34) == created);
    CHECK(spawn_calls == 2 && callback_calls == 1 && callback_arg == created);
    CHECK(callback_observed_count == 1 && callback_observed_entry == 0);
    CHECK(storage.count == 2 && storage.entries[1] == old_entry && storage.entries[2] == created);
    CHECK(spawn_args[0] == -1 && spawn_args[1] == 2 && spawn_args[2] == -3);

    storage.count = 1;
    storage.entries[2] = 0;
    callback_override_count = 3;
    CHECK(append_normal_action(group, 4, 5, 6, 0x2234) == created);
    CHECK(callback_calls == 2 && callback_observed_count == 1);
    CHECK(storage.count == 4 && storage.entries[4] == created);
    CHECK(spawn_kind_arg == 0x2234);
}

static void test_append_kind(void)
{
    TestGroupStorage storage;
    PendingActionGroup *group = as_group(&storage);
    Actor *created = &actors[2];
    reset_world();
    memset(&storage, 0, sizeof(storage));
    storage.count = 0;
    pending_action_group_limit = 4;
    spawn_result = created;
    observed_group = group;
    alternate_rows[0x32].initialize = observe_callback;
    CHECK(append_kind_action(group, 7, 8, 9, 0x2032) == created);
    CHECK(kind_spawn_calls == 1 && spawn_calls == 0);
    CHECK(spawn_args[0] == 7 && spawn_args[1] == 8 && spawn_args[2] == 9);
    CHECK(spawn_kind_arg == 0x2032);
    CHECK(callback_calls == 1 && callback_arg == created && callback_observed_count == 0);
    CHECK(storage.count == 1 && storage.entries[1] == created);

    storage.count = 4;
    CHECK(append_kind_action(group, 1, 2, 3, 0x2032) == 0);
    CHECK(kind_spawn_calls == 1 && callback_calls == 1 && storage.count == 4);

    storage.count = 0;
    spawn_result = 0;
    CHECK(append_kind_action(group, 1, 2, 3, 0x2032) == 0);
    CHECK(kind_spawn_calls == 2 && callback_calls == 1 && storage.count == 0);
}

int main(void)
{
    test_retirement();
    test_append_normal();
    test_append_kind();
    printf("%d checks; %d failures\n", checks, failures);
    return failures != 0;
}
