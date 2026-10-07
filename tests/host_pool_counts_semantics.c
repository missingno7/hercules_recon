#include <stdio.h>
#include "../calibration/host_pool_counts.c"
#define POOL_PINNED 0x40000000UL

HostPoolState g_host_pool;
static PoolWord words[64];
static U32 checks, failures;

static void check(int ok, const char *label)
{
    ++checks;
    if (!ok) { ++failures; printf("FAIL: %s\n", label); }
}

static void clear_words(void)
{
    U32 i;
    for (i = 0; i < 64; ++i) words[i].descriptor = 0;
}

int main(void)
{
    clear_words();
    g_host_pool.starts[0] = words;
    g_host_pool.ends[0] = words;
    check(count_used(0) == 0, "equal start/end is empty");

    clear_words();
    g_host_pool.starts[0] = words;
    g_host_pool.ends[0] = words + 5;
    words[0].descriptor = POOL_FREE | 2;
    words[2].descriptor = POOL_FREE | 3;
    check(count_used(0) == 0, "free headers are excluded while the cursor advances by word count");

    clear_words();
    g_host_pool.starts[0] = words;
    g_host_pool.ends[0] = words + 6;
    words[0].descriptor = 2;
    words[2].descriptor = POOL_PINNED | 2;
    words[4].descriptor = POOL_FREE | 2;
    words[6].descriptor = 2;
    check(count_used(0) == 2, "allocated and pinned headers count; free and end marker do not");

    clear_words();
    g_host_pool.starts[0] = words + 8;
    g_host_pool.ends[0] = words + 11;
    words[8].descriptor = POOL_FREE | 1;
    words[9].descriptor = 2;
    words[10].descriptor = 0xdeadbeefUL;
    check(count_used(0) == 1, "one-word free block advances without reading a second word");

    clear_words();
    g_host_pool.starts[1] = words + 20;
    g_host_pool.ends[1] = words + 23;
    words[20].descriptor = 1;
    words[21].descriptor = POOL_FREE | 2;
    g_host_pool.starts[0] = words;
    g_host_pool.ends[0] = words;
    check(count_used(1) == 1 && count_used(0) == 0, "pool_id selects matching start/end pair");

    g_host_pool.starts[2] = words + 30;
    g_host_pool.ends[2] = words + 29;
    check(count_used(2) == 0, "start at or above end returns zero without scanning");

    printf("%lu checks; %lu failures\n", checks, failures);
    return failures ? 1 : 0;
}
