#include "../calibration/host_pool_lifecycle.c"

#include <stdio.h>

HostPoolState g_host_pool;
const char g_fmt_compact[] = "CompactMalloc Heap%d @ 0x%x,Size %d\n";

static PoolWord test_pool[64];
static unsigned long checks;
static unsigned long failures;
static unsigned long frees;
static unsigned long copies;
static unsigned long diagnostics;

#define CHECK(x) do { ++checks; if (!(x)) ++failures; } while (0)

void __cdecl reverse_dword_copy(void *destination, const void *source,
                                U32 bytes)
{
    U32 count = (bytes + 3) >> 2;
    U32 *to = (U32 *)destination;
    const U32 *from = (const U32 *)source;
    ++copies;
    while (count != 0) {
        --count;
        to[count] = from[count];
    }
}

void __cdecl coalesce_pool(U32 pool_id)
{
    PoolWord *cursor = g_host_pool.starts[pool_id];
    PoolWord *end = g_host_pool.ends[pool_id];
    while (cursor < end) {
        U32 descriptor = cursor->descriptor;
        if (descriptor & POOL_FREE) {
            U32 next = cursor[descriptor & POOL_SIZE].descriptor;
            if (next & POOL_FREE) {
                cursor->descriptor = descriptor + (next & POOL_SIZE);
                continue;
            }
        }
        cursor += descriptor & POOL_SIZE;
    }
}

U32 __cdecl count_used(U32 pool_id)
{
    PoolWord *cursor = g_host_pool.starts[pool_id];
    PoolWord *end = g_host_pool.ends[pool_id];
    U32 count = 0;
    while (cursor < end) {
        U32 descriptor = cursor->descriptor;
        if ((descriptor & POOL_FREE) == 0)
            ++count;
        cursor += descriptor & POOL_SIZE;
    }
    return count;
}

void __cdecl host_free(void *payload)
{
    PoolWord *header = (PoolWord *)((U8 *)payload - 8);
    PoolWord *owner_word = (PoolWord *)((U8 *)payload - 4);
    ++frees;
    header->descriptor |= POOL_FREE;
    coalesce_pool(0);
    if (owner_word->owner_slot != 0)
        *owner_word->owner_slot = 0;
}

void __cdecl host_diagnostic(const char *format, ...)
{
    (void)format;
    ++diagnostics;
}

static void reset_pool(U32 end_index)
{
    U32 i;
    for (i = 0; i < 64; ++i)
        test_pool[i].descriptor = 0;
    g_host_pool.starts[0] = test_pool;
    g_host_pool.ends[0] = test_pool + end_index;
    g_host_pool.bulk_first_header = 0;
    g_host_pool.bulk_first_words = 0;
    frees = 0;
    copies = 0;
    diagnostics = 0;
}

static void test_overlapping_rightward_move(void)
{
    void *owner_a;
    void *owner_b;
    U32 result;

    reset_pool(12);
    owner_a = &test_pool[2];
    owner_b = &test_pool[9];
    test_pool[0].descriptor = 4;
    test_pool[1].owner_slot = &owner_a;
    test_pool[2].descriptor = 0xa1a1a1a1UL;
    test_pool[3].descriptor = 0xa2a2a2a2UL;
    test_pool[4].descriptor = POOL_FREE | 3;
    test_pool[7].descriptor = 5;
    test_pool[8].owner_slot = &owner_b;
    test_pool[9].descriptor = 0xb1b1b1b1UL;
    test_pool[10].descriptor = 0xb2b2b2b2UL;
    test_pool[11].descriptor = 0xb3b3b3b3UL;
    test_pool[12].descriptor = 1;

    result = compact_pool(0, 0);
    CHECK(result == 1);
    CHECK(copies == 1);
    CHECK(owner_a == (void *)&test_pool[5]);
    CHECK(owner_b == (void *)&test_pool[9]);
    CHECK(test_pool[0].descriptor == (POOL_FREE | 3));
    CHECK(test_pool[3].descriptor == 4);
    CHECK(test_pool[4].owner_slot == &owner_a);
    CHECK(test_pool[5].descriptor == 0xa1a1a1a1UL);
    CHECK(test_pool[6].descriptor == 0xa2a2a2a2UL);
    CHECK(test_pool[7].descriptor == 5);
    CHECK(test_pool[9].descriptor == 0xb1b1b1b1UL);
    CHECK(test_pool[10].descriptor == 0xb2b2b2b2UL);
    CHECK(test_pool[11].descriptor == 0xb3b3b3b3UL);
    CHECK(diagnostics == 1);
}

static void test_pinned_block_is_not_moved(void)
{
    void *owner;
    U32 result;

    reset_pool(7);
    owner = &test_pool[2];
    test_pool[0].descriptor = POOL_PINNED | 4;
    test_pool[1].owner_slot = &owner;
    test_pool[2].descriptor = 0xc1c1c1c1UL;
    test_pool[3].descriptor = 0xc2c2c2c2UL;
    test_pool[4].descriptor = POOL_FREE | 3;
    test_pool[7].descriptor = 1;

    result = compact_pool(0, 0);
    CHECK(result == 1);
    CHECK(copies == 0);
    CHECK(owner == (void *)&test_pool[2]);
    CHECK(test_pool[0].descriptor == (POOL_PINNED | 4));
    CHECK(test_pool[1].owner_slot == &owner);
    CHECK(test_pool[2].descriptor == 0xc1c1c1c1UL);
}

static void test_bulk_free_skips_and_repeats_safely(void)
{
    void *empty_owner;
    void *pinned_owner;
    void *owned_owner;

    reset_pool(12);
    empty_owner = 0;
    pinned_owner = &test_pool[7];
    owned_owner = &test_pool[10];
    test_pool[0].descriptor = POOL_FREE | 1;
    test_pool[1].descriptor = 4;
    test_pool[2].owner_slot = &empty_owner;
    test_pool[3].descriptor = 0xd1d1d1d1UL;
    test_pool[4].descriptor = 0xd2d2d2d2UL;
    test_pool[5].descriptor = POOL_PINNED | 3;
    test_pool[6].owner_slot = &pinned_owner;
    test_pool[7].descriptor = 0xe1e1e1e1UL;
    test_pool[8].descriptor = 4;
    test_pool[9].owner_slot = &owned_owner;
    test_pool[10].descriptor = 0xf1f1f1f1UL;
    test_pool[11].descriptor = 0xf2f2f2f2UL;
    test_pool[12].descriptor = 1;

    bulk_free(0);
    CHECK(frees == 1);
    CHECK(empty_owner == 0);
    CHECK(pinned_owner == (void *)&test_pool[7]);
    CHECK(owned_owner == 0);
    CHECK(test_pool[0].descriptor == (POOL_FREE | 1));
    CHECK(test_pool[1].descriptor == 4);
    CHECK(test_pool[5].descriptor == (POOL_PINNED | 3));
    CHECK(test_pool[8].descriptor == (POOL_FREE | 4));
    CHECK(g_host_pool.bulk_first_header == test_pool);
    CHECK(g_host_pool.bulk_first_words == 1);

    bulk_free(0);
    CHECK(frees == 1);
    CHECK(test_pool[0].descriptor == (POOL_FREE | 1));
    CHECK(test_pool[5].descriptor == (POOL_PINNED | 3));
    CHECK(test_pool[8].descriptor == (POOL_FREE | 4));
}

int main(void)
{
    test_overlapping_rightward_move();
    test_pinned_block_is_not_moved();
    test_bulk_free_skips_and_repeats_safely();
    printf("checks=%lu failures=%lu\n", checks, failures);
    return failures != 0;
}
