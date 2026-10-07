#include <stdio.h>
#include <string.h>
#include "../calibration/actor_pool_reset_repaired.c"

static ActorNormal normal_old_pool[3];
static ActorNormal normal_new_pool[3];
static ActorMiddle middle_old_pool[3];
static ActorMiddle middle_new_pool[3];
static ActorCore alternate_old_pool[3];
static ActorCore alternate_new_pool[3];

ActorNormal *g_normal_actor_pool;
ActorCore *g_alternate_actor_pool;
ActorMiddle *g_middle_actor_pool;
unsigned short g_normal_cursor_primary;
unsigned short g_normal_limit_primary;
unsigned short g_normal_limit_wrap;
unsigned short g_normal_cursor_alternate;
unsigned short g_normal_limit_wrap_start;
unsigned short g_alternate_cursor_primary;
unsigned short g_alternate_limit_primary;
unsigned short g_alternate_limit_wrap;
unsigned short g_alternate_cursor_alternate;
unsigned short g_alternate_limit_wrap_start;
unsigned short g_middle_cursor_primary;
unsigned short g_middle_limit_primary;
unsigned short g_middle_wrap_start_primary;
unsigned short g_middle_cursor_alternate;
unsigned short g_middle_wrap_start_alternate;

static int result_value;
static int normal_calls;
static int middle_calls;
static int alternate_calls;
static int clear_calls;
static void *normal_seen[3];
static void *middle_seen[3];
static void *alternate_seen[3];
static void *clear_seen[3];
static int checks;
static int failures;

static void expect(int ok, const char *label)
{
    ++checks;
    if (!ok) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

int remove_actor(void *actor)
{
    normal_seen[normal_calls] = actor;
    ++normal_calls;
    if (normal_calls == 1) {
        g_normal_actor_pool = normal_new_pool;
        g_normal_limit_primary = 2;
    }
    return result_value;
}

int remove_middle_actor(void *actor)
{
    middle_seen[middle_calls] = actor;
    ++middle_calls;
    if (middle_calls == 1) {
        g_middle_actor_pool = middle_new_pool;
        g_middle_limit_primary = 2;
    }
    return result_value;
}

int release_actor(void *actor)
{
    alternate_seen[alternate_calls] = actor;
    ++alternate_calls;
    if (alternate_calls == 1) {
        g_alternate_actor_pool = alternate_new_pool;
        g_alternate_limit_primary = 2;
    }
    return result_value;
}

void clear_actor(void *actor)
{
    clear_seen[clear_calls] = actor;
    ++clear_calls;
}

static void prepare(void)
{
    memset(normal_old_pool, 0x5a, sizeof(normal_old_pool));
    memset(normal_new_pool, 0x5a, sizeof(normal_new_pool));
    memset(middle_old_pool, 0x5a, sizeof(middle_old_pool));
    memset(middle_new_pool, 0x5a, sizeof(middle_new_pool));
    memset(alternate_old_pool, 0x5a, sizeof(alternate_old_pool));
    memset(alternate_new_pool, 0x5a, sizeof(alternate_new_pool));
    memset(normal_seen, 0, sizeof(normal_seen));
    memset(middle_seen, 0, sizeof(middle_seen));
    memset(alternate_seen, 0, sizeof(alternate_seen));
    memset(clear_seen, 0, sizeof(clear_seen));
    normal_calls = middle_calls = alternate_calls = clear_calls = 0;
}

static void test_normal(int accepted)
{
    prepare();
    result_value = accepted;
    g_normal_actor_pool = normal_old_pool;
    g_normal_limit_primary = 1;
    reset_normal_actor_pool();
    expect(normal_calls == 2, "normal count reloaded after callback");
    expect(normal_seen[0] == &normal_old_pool[0], "normal first actor captured before callback");
    expect(normal_seen[1] == &normal_new_pool[1], "normal next actor uses reloaded base plus stride");
    if (accepted) {
        expect(normal_old_pool[0].base.base.category == 1, "normal initializer keeps first captured actor");
        expect(normal_new_pool[1].base.base.category == 1, "normal initializer processes reloaded actor");
        expect(normal_new_pool[0].base.base.category == 0x5a, "normal reloaded base offset remains accumulated");
        expect(clear_calls == 0, "normal accepted path skips clear");
    } else {
        expect(clear_calls == 2, "normal clear called for both captured actors");
        expect(clear_seen[0] == &normal_old_pool[0], "normal clear uses original selected actor");
        expect(clear_seen[1] == &normal_new_pool[1], "normal clear uses reloaded selected actor");
    }
}

static void test_middle(int accepted)
{
    prepare();
    result_value = accepted;
    g_middle_actor_pool = middle_old_pool;
    g_middle_limit_primary = 1;
    reset_middle_actor_pool();
    expect(middle_calls == 2, "middle count reloaded after callback");
    expect(middle_seen[0] == &middle_old_pool[0], "middle first actor captured before callback");
    expect(middle_seen[1] == &middle_new_pool[1], "middle next actor uses reloaded base plus stride");
    if (accepted) {
        expect(middle_old_pool[0].field_094 == 0, "middle initializer keeps first captured actor");
        expect(middle_new_pool[1].field_094 == 0, "middle initializer processes reloaded actor");
        expect(middle_new_pool[0].field_094 == 0x5a, "middle reloaded base offset remains accumulated");
        expect(clear_calls == 0, "middle accepted path skips clear");
    } else {
        expect(clear_calls == 2, "middle clear called for both captured actors");
        expect(clear_seen[0] == &middle_old_pool[0], "middle clear uses original selected actor");
        expect(clear_seen[1] == &middle_new_pool[1], "middle clear uses reloaded selected actor");
    }
}

static void test_alternate(int accepted)
{
    prepare();
    result_value = accepted;
    g_alternate_actor_pool = alternate_old_pool;
    g_alternate_limit_primary = 1;
    reset_alternate_actor_pool();
    expect(alternate_calls == 2, "alternate count reloaded after callback");
    expect(alternate_seen[0] == &alternate_old_pool[0], "alternate first actor captured before callback");
    expect(alternate_seen[1] == &alternate_new_pool[1], "alternate next actor uses reloaded base plus stride");
    if (accepted) {
        expect(alternate_old_pool[0].category == 1, "alternate initializer keeps first captured actor");
        expect(alternate_new_pool[1].category == 1, "alternate initializer processes reloaded actor");
        expect(alternate_new_pool[0].category == 0x5a, "alternate reloaded base offset remains accumulated");
        expect(clear_calls == 0, "alternate accepted path skips clear");
    } else {
        expect(clear_calls == 2, "alternate clear called for both captured actors");
        expect(clear_seen[0] == &alternate_old_pool[0], "alternate clear uses original selected actor");
        expect(clear_seen[1] == &alternate_new_pool[1], "alternate clear uses reloaded selected actor");
    }
}

int main(void)
{
    test_normal(1);
    test_normal(0);
    test_middle(1);
    test_middle(0);
    test_alternate(1);
    test_alternate(0);
    printf("checks=%d failures=%d\n", checks, failures);
    return failures != 0;
}
