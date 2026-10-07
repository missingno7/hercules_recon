#include <stdio.h>
#include <string.h>
#include "../calibration/actor_pool_reset_repaired.c"

ActorNormal *g_normal_actor_pool;
ActorCore *g_alternate_actor_pool;
ActorMiddle *g_middle_actor_pool;
unsigned short g_normal_cursor_primary, g_normal_limit_primary, g_normal_limit_wrap;
unsigned short g_normal_cursor_alternate, g_normal_limit_wrap_start;
unsigned short g_alternate_cursor_primary, g_alternate_limit_primary, g_alternate_limit_wrap;
unsigned short g_alternate_cursor_alternate, g_alternate_limit_wrap_start;
unsigned short g_middle_cursor_primary, g_middle_limit_primary;
unsigned short g_middle_wrap_start_primary, g_middle_cursor_alternate;
unsigned short g_middle_wrap_start_alternate;

static unsigned int normal_calls, middle_calls, alternate_calls, clear_calls;
static int normal_result[8], middle_result[8], alternate_result[8];
static void *normal_seen[8], *middle_seen[8], *alternate_seen[8], *clear_seen[24];

int remove_actor(void *actor)
{
    unsigned int n = normal_calls++;
    normal_seen[n] = actor;
    return normal_result[n];
}
int remove_middle_actor(void *actor)
{
    unsigned int n = middle_calls++;
    middle_seen[n] = actor;
    return middle_result[n];
}
int release_actor(void *actor)
{
    unsigned int n = alternate_calls++;
    alternate_seen[n] = actor;
    return alternate_result[n];
}
void clear_actor(void *actor)
{
    clear_seen[clear_calls++] = actor;
    ((unsigned char *)actor)[0] = 0xc1;
}

static unsigned long checks;
#define CHECK(x) do { ++checks; if (!(x)) return __LINE__; } while (0)

int main(void)
{
    ActorMiddle middle[6];
    ActorNormal normal[3];
    ActorCore alternate[3];
    unsigned int i;

    memset(middle, 0, sizeof(middle));
    g_middle_actor_pool = middle;
    middle[3].base.kind = 1;
    middle[4].base.kind = 1;
    g_middle_cursor_primary = 3;
    g_middle_limit_primary = 5;
    g_middle_wrap_start_primary = 0;
    CHECK(allocate_middle_actor(0x8000) == &middle[0]);
    CHECK(g_middle_cursor_primary == 0);
    CHECK(middle[0].base.kind == 0);
    CHECK(allocate_middle_actor(0x8000) == &middle[0]);
    CHECK(g_middle_cursor_primary == 0);

    memset(middle, 0, sizeof(middle));
    middle[2].base.kind = 1;
    middle[3].base.kind = 1;
    g_middle_cursor_alternate = 2;
    g_middle_wrap_start_primary = 4;
    g_middle_wrap_start_alternate = 1;
    CHECK(allocate_middle_actor(1) == &middle[1]);
    CHECK(g_middle_cursor_alternate == 1);

    memset(middle, 0, sizeof(middle));
    for (i = 0; i < 5; ++i) middle[i].base.kind = 1;
    g_middle_cursor_primary = 1;
    g_middle_limit_primary = 5;
    g_middle_wrap_start_primary = 0;
    CHECK(allocate_middle_actor(0x8000) == 0);
    CHECK(g_middle_cursor_primary == 1);

    memset(normal, 0x66, sizeof(normal));
    g_normal_actor_pool = normal;
    g_normal_limit_primary = 2;
    normal_result[0] = 1;
    normal_result[1] = 0;
    normal_calls = clear_calls = 0;
    reset_normal_actor_pool();
    CHECK(normal_calls == 2 && clear_calls == 1);
    CHECK(normal_seen[0] == &normal[0] && normal_seen[1] == &normal[1]);
    CHECK(clear_seen[0] == &normal[1]);
    CHECK(normal[0].base.base.kind == 0 && normal[0].field_0ee == 1);
    CHECK(((unsigned char *)&normal[1])[0] == 0xc1);
    CHECK(normal[1].base.base.kind == 0x6666);

    memset(middle, 0x55, sizeof(middle));
    g_middle_actor_pool = middle;
    g_middle_limit_primary = 2;
    middle_result[0] = 0;
    middle_result[1] = 1;
    middle_calls = clear_calls = 0;
    reset_middle_actor_pool();
    CHECK(middle_calls == 2 && clear_calls == 1);
    CHECK(middle_seen[0] == &middle[0] && middle_seen[1] == &middle[1]);
    CHECK(clear_seen[0] == &middle[0]);
    CHECK(((unsigned char *)&middle[0])[0] == 0xc1);
    CHECK(middle[1].base.kind == 0);

    memset(alternate, 0x44, sizeof(alternate));
    g_alternate_actor_pool = alternate;
    g_alternate_limit_primary = 2;
    alternate_result[0] = 0;
    alternate_result[1] = 1;
    alternate_calls = clear_calls = 0;
    reset_alternate_actor_pool();
    CHECK(alternate_calls == 2 && clear_calls == 1);
    CHECK(alternate_seen[0] == &alternate[0] && alternate_seen[1] == &alternate[1]);
    CHECK(clear_seen[0] == &alternate[0]);
    CHECK(((unsigned char *)&alternate[0])[0] == 0xc1);
    CHECK(alternate[1].kind == 0);

    g_normal_limit_primary = 0;
    g_middle_limit_primary = 0;
    g_alternate_limit_primary = 0;
    normal_calls = middle_calls = alternate_calls = clear_calls = 0;
    reset_normal_actor_pool();
    reset_middle_actor_pool();
    reset_alternate_actor_pool();
    CHECK(normal_calls == 0 && middle_calls == 0 && alternate_calls == 0 && clear_calls == 0);
    printf("checks=%lu failures=0\n", checks);
    return 0;
}
