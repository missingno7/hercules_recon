#include "../src/eng1/behavioral/resource_runtime_data.h"
#include <stdio.h>

typedef char pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char long_is_32[(sizeof(long) == 4) ? 1 : -1];

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

static unsigned char normal_storage[4 * 0x134];
static unsigned char middle_storage[4 * 0xec];
static unsigned char alternate_storage[4 * 0x94];
static long group_counts[4];
static void *spy_addresses[16];
static unsigned long spy_tags[16];
static unsigned long spy_count;
static unsigned long fail_count;

static void record_spy(unsigned long tag, void *actor)
{
    if (spy_count >= 16) {
        ++fail_count;
        return;
    }
    spy_tags[spy_count] = tag;
    spy_addresses[spy_count] = actor;
    ++spy_count;
}

void __cdecl initialize_normal_actor(void *actor)
{
    record_spy(0x27520UL, actor);
    if (spy_count == 1) {
        g_normal_actor_pool = normal_storage + 0x134;
        g_configuration_word_713b4 = 2;
    }
}

void __cdecl initialize_middle_actor(void *actor)
{
    record_spy(0x27a50UL, actor);
    if (spy_count == 3)
        g_middle_actor_pool = middle_storage + 0xec;
    if (spy_count == 4)
        g_configuration_word_71432 = 2;
}

void __cdecl initialize_alternate_actor(void *actor)
{
    record_spy(0x27fe0UL, actor);
    if (spy_count == 5)
        g_configuration_word_713ba = 1;
}

static int expect_spy(unsigned long position, unsigned long tag,
                      const void *address)
{
    if (position >= spy_count || spy_tags[position] != tag ||
        spy_addresses[position] != address) {
        ++fail_count;
        return 0;
    }
    return 1;
}

int main(void)
{
    unsigned long before;
    unsigned long i;

    g_normal_actor_pool = normal_storage;
    g_middle_actor_pool = middle_storage;
    g_alternate_actor_pool = alternate_storage;

    /* No configured entries means no initializer and no table write. */
    g_configuration_word_713b4 = 0;
    g_configuration_word_71432 = 0;
    g_configuration_word_713ba = 0;
    g_configuration_word_71380 = 0;
    group_counts[0] = 0x13579bdfL;
    pending_action_group_table[0] = (PendingActionGroup *)&group_counts[0];
    before = spy_count;
    eng1_call_274e0();
    eng1_call_279f0();
    eng1_call_27fa0();
    if (spy_count != before || group_counts[0] != 0x13579bdfL)
        ++fail_count;

    /* Each pool sweep sees a WORD-count change made by its initializer before
     * the next comparison, and reloads the same pool base plus its native
     * cumulative byte stride. */
    g_configuration_word_713b4 = 4;
    eng1_call_274e0();
    if (spy_count != 2 ||
        !expect_spy(0, 0x27520UL, normal_storage) ||
        !expect_spy(1, 0x27520UL, normal_storage + 0x268))
        ++fail_count;

    g_configuration_word_71432 = 4;
    g_configuration_word_71380 = 3;
    group_counts[0] = 0x12345678L;
    group_counts[1] = 0x23456789L;
    group_counts[2] = 0x3456789aL;
    pending_action_group_table[0] = (PendingActionGroup *)&group_counts[0];
    pending_action_group_table[1] = (PendingActionGroup *)&group_counts[1];
    pending_action_group_table[2] = (PendingActionGroup *)&group_counts[2];
    eng1_call_279f0();
    if (spy_count != 4 ||
        !expect_spy(2, 0x27a50UL, middle_storage) ||
        !expect_spy(3, 0x27a50UL, middle_storage + 0x1d8) ||
        group_counts[0] != -1 || group_counts[1] != -1 ||
        group_counts[2] != -1)
        ++fail_count;

    g_configuration_word_713ba = 4;
    eng1_call_27fa0();
    if (spy_count != 5 ||
        !expect_spy(4, 0x27fe0UL, alternate_storage))
        ++fail_count;

    /* 0x279f0 writes through each table entry; it leaves the pointer cells
     * intact. The fixture's pointer prefix is the existing owner shape. */
    for (i = 0; i < 3; ++i) {
        if (pending_action_group_table[i] !=
            (PendingActionGroup *)&group_counts[i])
            ++fail_count;
    }

    if (fail_count != 0)
        return 1;
    printf("eng1_pool_startup_spy_cases=4 calls=%lu failures=0\n", spy_count);
    return 0;
}
