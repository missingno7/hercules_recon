#include <stdio.h>
#include <string.h>
#include "../calibration/resource_metadata.c"

typedef char fixture_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];

ActorResource g_actor_resources[81];

#define BLOB_SIZE 0x80000
#define BUFFER_OFFSET 0x22000
static unsigned char blob[BLOB_SIZE];
static int helper_result;
static int helper_calls;
static unsigned long helper_resource_id;
static int helper_metadata_index;
static int checks;
static int failures;

int ensure_actor_resource_ready(unsigned long resource_id, int metadata_index)
{
    ++helper_calls;
    helper_resource_id = resource_id;
    helper_metadata_index = metadata_index;
    return helper_result;
}

static void expect(int condition, const char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

static short signed_low_word(int value)
{
    unsigned short low = (unsigned short)value;
    short result;
    memcpy(&result, &low, sizeof(result));
    return result;
}

static unsigned long *descriptor_cell(unsigned long resource_id, int metadata_index)
{
    unsigned char *buffer = blob + BUFFER_OFFSET;
    unsigned char *basis = buffer + (unsigned long)g_actor_resources[resource_id].entry_count * 4UL + 8UL;
    short index = signed_low_word(metadata_index);
    return (unsigned long *)(basis + index * 4);
}

/* Byte-oriented reference model: decode the low-word index and the signed
 * 24-bit relative displacement independently of the candidate expression. */
static void *expected_result(unsigned long resource_id, int metadata_index, int ready_result)
{
    unsigned char *buffer;
    unsigned char *basis;
    unsigned char *cell;
    unsigned short low_index;
    short signed_index;
    unsigned long descriptor_bits;
    unsigned long shifted_bits;
    long signed_displacement;

    if (metadata_index == 0)
        return 0;
    if (g_actor_resources[resource_id].residency_state != 3 && !ready_result)
        return 0;

    buffer = blob + BUFFER_OFFSET;
    basis = buffer + (unsigned long)g_actor_resources[resource_id].entry_count * 4UL + 8UL;
    low_index = (unsigned short)metadata_index;
    memcpy(&signed_index, &low_index, sizeof(signed_index));
    cell = basis + (long)signed_index * 4L;
    memcpy(&descriptor_bits, cell, sizeof(descriptor_bits));
    if (descriptor_bits == 0)
        return 0;

    shifted_bits = descriptor_bits >> 8;
    if (descriptor_bits & 0x80000000UL)
        shifted_bits |= 0xff000000UL;
    memcpy(&signed_displacement, &shifted_bits, sizeof(signed_displacement));
    return basis + signed_displacement;
}

static void run_case(const char *label, unsigned long resource_id,
                     unsigned short entry_count, int metadata_index,
                     unsigned short state, int ready_result,
                     unsigned long descriptor_bits, long expected_delta)
{
    unsigned char *buffer = blob + BUFFER_OFFSET;
    unsigned char *basis;
    void *actual;
    void *expected;
    int expected_calls;

    memset(blob, 0, sizeof(blob));
    memset(g_actor_resources, 0, sizeof(g_actor_resources));
    helper_result = ready_result;
    helper_calls = 0;
    helper_resource_id = 0xffffffffUL;
    helper_metadata_index = 0x12345678;
    g_actor_resources[resource_id].references = (ActorReference *)buffer;
    g_actor_resources[resource_id].entry_count = entry_count;
    g_actor_resources[resource_id].residency_state = state;
    basis = buffer + (unsigned long)entry_count * 4UL + 8UL;
    if (metadata_index != 0)
        *descriptor_cell(resource_id, metadata_index) = descriptor_bits;

    expected = expected_result(resource_id, metadata_index, ready_result);
    actual = resource_metadata_pointer(resource_id, metadata_index);
    expect(actual == expected, label);
    if (expected)
        expect(actual == basis + expected_delta, "expected target-relative return address");
    expected_calls = metadata_index != 0 && state != 3;
    expect(helper_calls == expected_calls, "state/index helper gate");
    if (expected_calls) {
        expect(helper_resource_id == resource_id, "helper resource argument order/value");
        expect(helper_metadata_index == metadata_index, "helper receives raw dword index");
    }
}

int main(void)
{
    run_case("state 3 bypasses helper; positive shifted descriptor", 0, 4, 1, 3, 0, 0x00001234UL, 0x12);
    run_case("0xffff index sign-extends to -1", 80, 7, 0x0000ffff, 3, 0, 0x00000400UL, 4);
    run_case("0x8000 index reaches signed-short lower boundary", 5, 2, 0x00008000, 3, 0, 0xfffffe00UL, -2);
    run_case("0x7fff index reaches signed-short upper boundary", 80, 3, 0x00007fff, 3, 0, 0x00000100UL, 1);
    run_case("state 2 helper success permits metadata lookup", 9, 3, 2, 2, 1, 0x00001000UL, 16);
    run_case("state 2 helper failure returns null", 2, 3, 2, 2, 0, 0x00001000UL, 0);
    run_case("state 0 helper success with zero descriptor returns null", 1, 3, 2, 0, 1, 0, 0);
    run_case("state 3 zero descriptor returns null", 4, 3, 1, 3, 0, 0, 0);
    run_case("raw zero index exits before helper", 3, 3, 0, 2, 0, 0x00000100UL, 0);
    run_case("raw 0x10000 passes zero guard but low-word index is zero", 6, 3, 0x00010000, 3, 0, 0x00000200UL, 2);
    run_case("zero entry count uses buffer plus eight basis", 0, 0, 1, 3, 0, 0x00000100UL, 1);
    run_case("maximum entry count and negative index stay in the fixture blob", 80, 0xffff, 0x0000ffff, 3, 0, 0x00000300UL, 3);
    run_case("helper receives the full raw dword before low-word addressing", 7, 2, 0x00010000, 2, 1, 0x00000500UL, 5);

    printf("checks=%d failures=%d\n", checks, failures);
    return failures != 0;
}
