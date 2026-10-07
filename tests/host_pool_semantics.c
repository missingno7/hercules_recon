#include <stdio.h>
#include "../calibration/host_pool.c"

HostPoolState g_host_pool;
const char g_fmt_init_pool[] = "init";
const char g_fmt_alloc_failed[] = "old";
const char g_fmt_alloc[] = "alloc";
const char g_fmt_free[] = "free";

static U32 checks;
static U32 failures;
static U32 diagnostic_calls;

void __cdecl host_diagnostic(const char *format, ...)
{
    (void)format;
    ++diagnostic_calls;
}

static void check(int condition, const char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL: %s\n", label);
    }
}

static void reset_state(void)
{
    U32 i;
    for (i = 0; i < 4; ++i) {
        g_host_pool.starts[i] = 0;
        g_host_pool.ends[i] = 0;
    }
    g_host_pool.saved_arena = 0;
    g_host_pool.saved_bytes = 0;
    g_host_pool.bulk_first_header = 0;
    g_host_pool.bulk_first_words = 0;
    diagnostic_calls = 0;
}

static PoolWord init_arena[16];
static PoolWord allocation_arena[64];
static PoolWord selector_arenas[4][8];
static PoolWord free_arena[12];

static void test_initialize_preserves_header_owner_word(void)
{
    U32 i;
    U8 *bytes = (U8 *)init_arena;

    reset_state();
    for (i = 0; i < sizeof(init_arena); ++i)
        bytes[i] = 0x5a;

    save_arena(bytes, 64);
    check(g_host_pool.saved_arena == bytes && g_host_pool.saved_bytes == 64,
          "save_arena records the arena address and byte length");

    initialize_pool(bytes, 64, 2);
    check(g_host_pool.starts[2] == init_arena &&
          g_host_pool.ends[2] == init_arena + 14,
          "initialize_pool publishes the start and end after reserving two words");
    check(init_arena[0].descriptor == (POOL_FREE | 14),
          "initial free descriptor stores FREE plus total dword count");
    check(init_arena[1].descriptor == 0x5a5a5a5aUL,
          "initializer leaves allocated-owner word one untouched");
    for (i = 2; i <= 14; ++i)
        check(init_arena[i].descriptor == 1,
              "initializer writes one marker per remaining word through end marker");
    check(init_arena[15].descriptor == 0x5a5a5a5aUL,
          "initializer leaves the final reserved word untouched");
}

static void test_selector_bits_cover_four_pools(void)
{
    U32 i;
    reset_state();
    for (i = 0; i < 4; ++i) {
        void *slot = 0;
        selector_arenas[i][0].descriptor = POOL_FREE | 8;
        g_host_pool.starts[i] = selector_arenas[i];
        g_host_pool.ends[i] = selector_arenas[i] + 8;
        check(host_allocate(&slot, 0, i) == &selector_arenas[i][8],
              "low two flag bits select each independent pool");
        check(slot == &selector_arenas[i][8] &&
              selector_arenas[i][6].descriptor == 2,
              "selected pool records the zero-byte request's two-word block");
        check(selector_arenas[i][7].owner_slot == &slot,
              "allocated header stores the caller output-slot address");
    }
}

static void test_last_fit_and_high_end_split(void)
{
    void *slot = 0;
    void *result;

    reset_state();
    allocation_arena[0].descriptor = POOL_FREE | 10;
    allocation_arena[10].descriptor = POOL_FREE | 8;
    g_host_pool.starts[0] = allocation_arena;
    g_host_pool.ends[0] = allocation_arena + 18;

    result = host_allocate(&slot, 1, 0);
    check(result == &allocation_arena[17],
          "allocation uses the last fitting free block in address order");
    check(allocation_arena[0].descriptor == (POOL_FREE | 10),
          "last-fit leaves the earlier fitting free block unchanged");
    check(allocation_arena[10].descriptor == (POOL_FREE | 5),
          "high-end split retains the smaller free remainder at the old base");
    check(allocation_arena[15].descriptor == 3 &&
          allocation_arena[16].owner_slot == &slot,
          "high-end allocated header stores size and owner-slot word");
    check(slot == result && diagnostic_calls == 1,
          "successful allocation publishes the payload and emits one diagnostic");
}

static void test_pinned_flag_and_existing_output_diagnostic(void)
{
    void *slot = (void *)0x1357;
    void *result;

    reset_state();
    allocation_arena[0].descriptor = POOL_FREE | 6;
    g_host_pool.starts[0] = allocation_arena;
    g_host_pool.ends[0] = allocation_arena + 6;

    result = host_allocate(&slot, 4, 0x10);
    check(result == &allocation_arena[5] && slot == result,
          "four-byte request returns payload after the two-word header");
    check(allocation_arena[3].descriptor == (POOL_PINNED | 3),
          "allocation flag 0x10 becomes the pinned descriptor bit");
    check(allocation_arena[4].owner_slot == &slot,
          "pinned allocation still stores its owner slot");
    check(diagnostic_calls == 2,
          "nonnull old output and successful allocation both use the diagnostic leaf");
}

static void test_request_rounding_and_unsigned_wrap(void)
{
    static const U32 requests[5] = { 0, 1, 3, 4, 0xffffffffUL };
    static const U32 expected_words[5] = { 2, 3, 3, 3, 2 };
    U32 i;

    for (i = 0; i < 5; ++i) {
        void *slot = 0;
        U32 remaining = 10 - expected_words[i];
        reset_state();
        allocation_arena[0].descriptor = POOL_FREE | 10;
        g_host_pool.starts[0] = allocation_arena;
        g_host_pool.ends[0] = allocation_arena + 10;

        check(host_allocate(&slot, requests[i], 0) ==
                  &allocation_arena[remaining + 2],
              "rounded or unsigned-wrapped request returns the expected payload");
        check(allocation_arena[remaining].descriptor == expected_words[i],
              "request arithmetic encodes the expected total dword count");
    }
}

static void test_failure_output_policy(void)
{
    void *slot;
    reset_state();
    g_host_pool.starts[0] = allocation_arena;
    g_host_pool.ends[0] = allocation_arena;

    slot = (void *)0x1357;
    check(host_allocate(&slot, 4, 0) == 0 && slot == 0,
          "ordinary allocation failure clears the caller output slot");
    slot = (void *)0x2468;
    check(host_allocate(&slot, 4, 0x20) == 0 && slot == (void *)0x2468,
          "failure flag 0x20 preserves the existing output value");
    check(diagnostic_calls == 2, "both nonnull prior output values are diagnosed on failure calls");
}

static void test_free_coalesce_clears_owner_and_handles_one_word_free_block(void)
{
    void *owner = &free_arena[4];
    U32 i;

    reset_state();
    for (i = 0; i < 12; ++i)
        free_arena[i].descriptor = 0;
    g_host_pool.starts[0] = free_arena;
    g_host_pool.ends[0] = free_arena + 6;
    free_arena[0].descriptor = POOL_FREE | 2;
    free_arena[2].descriptor = 3;
    free_arena[3].owner_slot = &owner;
    free_arena[5].descriptor = POOL_FREE | 1;
    free_arena[6].descriptor = 0;

    host_free(&free_arena[4]);
    check(free_arena[0].descriptor == (POOL_FREE | 6),
          "free coalesces the freed allocation with adjacent 2-word and 1-word blocks");
    check(owner == 0,
          "free reads and clears the allocated block's owner slot after coalescing");
    check(diagnostic_calls == 1,
          "free emits one diagnostic before coalescing");
}

static void test_null_and_out_of_pool_free_are_noops(void)
{
    static U8 outside[16];
    U32 i;

    reset_state();
    for (i = 0; i < 12; ++i)
        free_arena[i].descriptor = 0;
    g_host_pool.starts[0] = free_arena;
    g_host_pool.ends[0] = free_arena + 6;
    free_arena[0].descriptor = POOL_FREE | 6;

    host_free(0);
    host_free(outside);
    check(free_arena[0].descriptor == (POOL_FREE | 6),
          "null and outside pointers leave pool headers unchanged");
    check(diagnostic_calls == 0,
          "null and outside pointers produce no diagnostic call");
}

int main(void)
{
    test_initialize_preserves_header_owner_word();
    test_selector_bits_cover_four_pools();
    test_last_fit_and_high_end_split();
    test_pinned_flag_and_existing_output_diagnostic();
    test_request_rounding_and_unsigned_wrap();
    test_failure_output_policy();
    test_free_coalesce_clears_owner_and_handles_one_word_free_block();

    test_null_and_out_of_pool_free_are_noops();
    printf("%lu checks; %lu failures\n", checks, failures);
    return failures ? 1 : 0;
}
