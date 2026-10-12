#include "../src/eng1/behavioral/resource_runtime_data.h"
#include <stdio.h>

typedef char pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char long_is_32[(sizeof(long) == 4) ? 1 : -1];

typedef struct NormalView NormalView;
struct NormalView {
    unsigned char bytes[0x134];
};
typedef struct MiddleView MiddleView;
struct MiddleView {
    unsigned char bytes[0xec];
};
typedef struct AlternateView AlternateView;
struct AlternateView {
    unsigned char bytes[0x94];
};

/* Counts copied from the initialized configuration row with ID 0 in
 * frame_data.c: normal +0x02, middle +0x04 then +5, alternate +0x06 then +5,
 * and action-group prefix +0x16. These are fixture observations, not array
 * capacity claims for the runtime. */
enum {
    NORMAL_COUNT = 0x14,
    MIDDLE_COUNT = 0x3c + 5,
    ALTERNATE_COUNT = 0x28 + 5,
    ACTION_GROUP_COUNT = 4
};

extern void __cdecl eng1_call_274e0(void);
extern void __cdecl eng1_call_279f0(void);
extern void __cdecl eng1_call_27fa0(void);

void *g_normal_actor_pool;
void *g_middle_actor_pool;
void *g_alternate_actor_pool;
unsigned short g_configuration_word_713b4;
unsigned short g_configuration_word_71432;
unsigned short g_configuration_word_713ba;
unsigned short g_configuration_word_71380;
PendingActionGroup *pending_action_group_table[20];

/* The reused initializer TU also contains allocator/reset providers that this
 * fixture does not exercise. These fixture cells and spies satisfy those
 * separate sections; only calls from the startup candidate are expected. */
unsigned short g_normal_cursor_primary, g_normal_limit_primary;
unsigned short g_normal_limit_wrap, g_normal_cursor_alternate;
unsigned short g_normal_limit_wrap_start;
unsigned short g_alternate_cursor_primary, g_alternate_limit_primary;
unsigned short g_alternate_limit_wrap, g_alternate_cursor_alternate;
unsigned short g_alternate_limit_wrap_start;
unsigned short g_middle_cursor_primary, g_middle_limit_primary;
unsigned short g_middle_wrap_start_primary, g_middle_cursor_alternate;
unsigned short g_middle_wrap_start_alternate;
static unsigned long unexpected_reset_calls;

int __cdecl remove_actor(void *actor)
{
    (void)actor;
    ++unexpected_reset_calls;
    return 0;
}

int __cdecl remove_middle_actor(void *actor)
{
    (void)actor;
    ++unexpected_reset_calls;
    return 0;
}

int __cdecl release_actor(void *actor)
{
    (void)actor;
    ++unexpected_reset_calls;
    return 0;
}

void __cdecl clear_actor(void *actor)
{
    (void)actor;
    ++unexpected_reset_calls;
}

static NormalView normal_pool[NORMAL_COUNT];
static MiddleView middle_pool[MIDDLE_COUNT];
static AlternateView alternate_pool[ALTERNATE_COUNT];
static long group_counts[ACTION_GROUP_COUNT];

static void fill(void *p, unsigned long bytes, unsigned char value)
{
    unsigned char *cursor = (unsigned char *)p;
    unsigned long i;
    for (i = 0; i < bytes; ++i)
        cursor[i] = value;
}

static unsigned short load16(const void *p)
{
    return *(const unsigned short *)p;
}

static unsigned long load32(const void *p)
{
    return *(const unsigned long *)p;
}

int main(void)
{
    unsigned long i;

    g_normal_actor_pool = normal_pool;
    g_middle_actor_pool = middle_pool;
    g_alternate_actor_pool = alternate_pool;
    g_configuration_word_713b4 = NORMAL_COUNT;
    g_configuration_word_71432 = MIDDLE_COUNT;
    g_configuration_word_713ba = ALTERNATE_COUNT;
    g_configuration_word_71380 = ACTION_GROUP_COUNT;
    for (i = 0; i < ACTION_GROUP_COUNT; ++i) {
        pending_action_group_table[i] =
            (PendingActionGroup *)&group_counts[i];
        group_counts[i] = (long)(i + 7);
    }

    fill(normal_pool, sizeof(normal_pool), 0xa5);
    fill(middle_pool, sizeof(middle_pool), 0xa5);
    fill(alternate_pool, sizeof(alternate_pool), 0xa5);

    eng1_call_274e0();
    eng1_call_279f0();
    eng1_call_27fa0();

    for (i = 0; i < ACTION_GROUP_COUNT; ++i) {
        if (group_counts[i] != -1)
            return 1;
    }
    if (unexpected_reset_calls != 0)
        return 5;

    for (i = 0; i < NORMAL_COUNT; ++i) {
        unsigned char *normal = normal_pool[i].bytes;

        if (load16(normal + 0x2e) != 0 || load32(normal + 0x50) != 2 ||
            load32(normal + 0x54) != 0x80000000UL ||
            normal[0x22] != 1 || normal[0xec] != 0 ||
            normal[0xed] != 0 || load16(normal + 0xee) != 1 ||
            load32(normal + 0xf0) != 0 || load32(normal + 0x110) != 0 ||
            load16(normal + 0x114) != 0xffffU ||
            load16(normal + 0x11c) != 0xffffU || normal[0x80] != 0xa5)
            return 2;
    }

    for (i = 0; i < MIDDLE_COUNT; ++i) {
        unsigned char *middle = middle_pool[i].bytes;

        if (load16(middle + 0x2e) != 0 || load32(middle + 0x50) != 2 ||
            load32(middle + 0x54) != 0x80000000UL ||
            middle[0x22] != 1 || load32(middle + 0x9c) != 0 ||
            load32(middle + 0xe8) != 0 || middle[0x80] != 0xa5)
            return 3;
    }

    for (i = 0; i < ALTERNATE_COUNT; ++i) {
        unsigned char *alternate = alternate_pool[i].bytes;

        if (load16(alternate + 0x2e) != 0 ||
            load32(alternate + 0x50) != 2 ||
            load32(alternate + 0x54) != 0x80000000UL ||
            alternate[0x22] != 1 || load32(alternate + 0x64) != 0 ||
            load16(alternate + 0x32) != 0xffffU ||
            alternate[0x80] != 0xa5)
            return 4;
    }

    printf("eng1_pool_startup_initializer_cases=3 profile=ID0 normal=20 middle=65 alternate=45 groups=4 failures=0\n");
    return 0;
}
