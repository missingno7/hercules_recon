#include "../calibration/resource_readiness.c"

#include <stdio.h>

ActorResource g_actor_resources[81];
ResourceCallbackContext *g_resource_context;

static ResourceCallbackContext fixture_context;
static unsigned long fixture_blob[128];
static int reset_calls;
static int reset_resource_id;
static int check_count;
static int failure_count;

void __cdecl resource_reset(int resource_id)
{
    ++reset_calls;
    reset_resource_id = resource_id;
    if (g_actor_resources[(unsigned long)resource_id].residency_state == 0U)
        g_actor_resources[resource_id].residency_state = 1U;
}

static void check(const char *name, int passed)
{
    ++check_count;
    if (!passed) {
        ++failure_count;
        printf("FAIL: %s\n", name);
    }
}

static void clear_case(unsigned short count)
{
    unsigned long i;
    for (i = 0; i < sizeof(fixture_blob) / sizeof(fixture_blob[0]); ++i)
        fixture_blob[i] = 0UL;
    g_actor_resources[80].entry_count = count;
    g_actor_resources[80].references = (ActorReference *)fixture_blob;
    g_actor_resources[80].residency_state = 2U;
    g_actor_resources[80].unknown_012 = 0U;
    g_resource_context = &fixture_context;
}

static unsigned long *descriptor_slot(int slot)
{
    unsigned char *basis =
        (unsigned char *)g_actor_resources[80].references +
        (unsigned long)g_actor_resources[80].entry_count * 4UL + 8UL;
    return (unsigned long *)(basis + (long)slot * 4L);
}

static long signed_shift_8(unsigned long bits)
{
    unsigned long shifted = bits >> 8;
    if (bits & 0x80000000UL)
        shifted |= 0xff000000UL;
    return (long)shifted;
}

/* Independent byte-oriented reference for the target's scan contract. */
static int expected_scan(int index)
{
    long available = ((long)fixture_context.total_sectors_0a0 -
                      (long)fixture_context.remaining_sectors_09e) * 2048L;
    unsigned short count = g_actor_resources[80].entry_count;
    unsigned char *basis =
        (unsigned char *)g_actor_resources[80].references +
        (unsigned long)count * 4UL + 8UL;
    long slot;

    if (available < 4096L)
        return 0;
    if (index >= (long)count)
        return 0;

    for (slot = (long)index + 1L; slot <= (long)count; ++slot) {
        unsigned long bits = *(unsigned long *)(basis + slot * 4L);
        long offset = signed_shift_8(bits);
        if (offset != 0L)
            return available >= offset;
    }
    return 0;
}

static void compare_scan(const char *name, int index)
{
    check(name, resource_metadata_scan(80UL, index) == expected_scan(index));
}

int main(void)
{
    clear_case(3U);
    fixture_context.remaining_sectors_09e = 1;
    fixture_context.total_sectors_0a0 = 2;
    check("sub-threshold guard returns without an indexed resource record",
          resource_metadata_scan(0x01000000UL, 0) == 0);

    clear_case(3U);
    fixture_context.remaining_sectors_09e = 1;
    fixture_context.total_sectors_0a0 = 3;
    *descriptor_slot(3) = 4096UL << 8;
    check("exact 4096-byte budget and inclusive final slot", resource_metadata_scan(80UL, 2) == 1);
    compare_scan("inclusive final-slot reference", 2);
    compare_scan("signed index at count is rejected", 3);

    fixture_context.total_sectors_0a0 = 2;
    *descriptor_slot(1) = 1UL << 8;
    compare_scan("one sector is below the 4096-byte guard", 0);

    clear_case(3U);
    fixture_context.remaining_sectors_09e = -3;
    fixture_context.total_sectors_0a0 = -1;
    *descriptor_slot(3) = 4096UL << 8;
    compare_scan("negative signed sector fields with positive 4096-byte difference", 2);

    fixture_context.total_sectors_0a0 = -1;
    fixture_context.remaining_sectors_09e = 0;
    compare_scan("negative available-byte budget", 2);

    clear_case(3U);
    fixture_context.total_sectors_0a0 = 3;
    fixture_context.remaining_sectors_09e = 1;
    *descriptor_slot(1) = 4097UL << 8;
    *descriptor_slot(2) = 1024UL << 8;
    compare_scan("first nonzero offset over budget stops before smaller later offset", 0);

    clear_case(2U);
    fixture_context.total_sectors_0a0 = 3;
    fixture_context.remaining_sectors_09e = 1;
    *descriptor_slot(1) = 0x00000080UL;
    *descriptor_slot(2) = 4096UL << 8;
    compare_scan("zero after arithmetic shift skips low-byte-only word", 0);

    clear_case(2U);
    fixture_context.total_sectors_0a0 = 3;
    fixture_context.remaining_sectors_09e = 1;
    compare_scan("all shifted descriptor words zero returns zero", 0);

    clear_case(2U);
    fixture_context.total_sectors_0a0 = 3;
    fixture_context.remaining_sectors_09e = 1;
    *descriptor_slot(1) = 4096UL << 8;
    compare_scan("negative input starts at slot zero", -1);
    compare_scan("large positive raw index is rejected by signed comparison", 0x10000);

    clear_case(2U);
    fixture_context.total_sectors_0a0 = 4;
    fixture_context.remaining_sectors_09e = 0;
    *descriptor_slot(1) = 8192UL << 8;
    compare_scan("offset equal to larger byte budget", 0);
    *descriptor_slot(1) = 8193UL << 8;
    compare_scan("offset one byte over budget", 0);

    clear_case(2U);
    fixture_context.total_sectors_0a0 = 3;
    fixture_context.remaining_sectors_09e = 1;
    *descriptor_slot(1) = 0xffffff00UL;
    compare_scan("arithmetic descriptor shift remains signed", 0);

    clear_case(0U);
    fixture_context.remaining_sectors_09e = 0;
    fixture_context.total_sectors_0a0 = 3;
    compare_scan("zero count rejects zero index", 0);

    clear_case(3U);
    fixture_context.total_sectors_0a0 = 3;
    fixture_context.remaining_sectors_09e = 1;
    *descriptor_slot(3) = 4096UL << 8;
    g_actor_resources[80].residency_state = 2U;
    g_actor_resources[80].unknown_012 = 0U;
    reset_calls = 0;
    check("state two delegates and returns scan EAX",
          ensure_actor_resource_ready(80UL, 2) == 1 && reset_calls == 0 &&
          g_actor_resources[80].unknown_012 == 0U);

    g_actor_resources[80].residency_state = 0U;
    g_actor_resources[80].unknown_012 = 0U;
    check("state zero resets then stores unknown_012=3",
          ensure_actor_resource_ready(80UL, 2) == 0 && reset_calls == 1 &&
          reset_resource_id == 80UL && g_actor_resources[80].residency_state == 1U &&
          g_actor_resources[80].unknown_012 == 3U);

    g_actor_resources[80].residency_state = 1U;
    g_actor_resources[80].unknown_012 = 9U;
    check("state one returns zero without reset or field write",
          ensure_actor_resource_ready(80UL, 2) == 0 && reset_calls == 1 &&
          g_actor_resources[80].unknown_012 == 9U);

    g_actor_resources[80].residency_state = 3U;
    check("state three is not a special bypass here", ensure_actor_resource_ready(80UL, 2) == 0 && reset_calls == 1);

    printf("resource readiness fixture: %d checks, %d failures\n", check_count, failure_count);
    return failure_count ? 1 : 0;
}
