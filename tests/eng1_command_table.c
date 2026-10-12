#include <stdio.h>
#include <string.h>
#include "../calibration/engine_interface.h"

typedef unsigned long u32;

#define COMMAND_SLOT_COUNT 20
#define COMMAND_RECORD_WORDS 10

extern unsigned short *g_command_slot_storage_71320[COMMAND_SLOT_COUNT];
extern void __cdecl reset_command_table(void);
extern int __cdecl initialize_command_table(u32 bytes, unsigned int opcode,
                                            u32 count);
extern unsigned short **__cdecl allocate_command(unsigned int opcode);

static unsigned short allocation_records[COMMAND_SLOT_COUNT][COMMAND_RECORD_WORDS];
static void **observed_slots[COMMAND_SLOT_COUNT + 4];
static u32 observed_bytes[COMMAND_SLOT_COUNT + 4];
static u32 observed_flags[COMMAND_SLOT_COUNT + 4];
static int allocation_calls;
static int successful_allocations;
static int fail_on_call;

#define CHECK(condition) do { \
    if (!(condition)) { \
        printf("FAIL line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static void *__cdecl allocation_spy(void **output_slot, u32 bytes, u32 flags)
{
    int call_number;
    unsigned short *record;

    ++allocation_calls;
    call_number = allocation_calls;
    if (call_number <= COMMAND_SLOT_COUNT + 4) {
        observed_slots[call_number - 1] = output_slot;
        observed_bytes[call_number - 1] = bytes;
        observed_flags[call_number - 1] = flags;
    }
    if (fail_on_call == call_number)
        return 0;
    if (successful_allocations >= COMMAND_SLOT_COUNT)
        return 0;

    record = allocation_records[successful_allocations++];
    *output_slot = record;
    return record;
}

static void reset_fixture(int fail_at)
{
    unsigned long i, j;

    reset_command_table();
    memset(&g_engine_interface, 0, sizeof(g_engine_interface));
    g_engine_interface.allocate_078 = allocation_spy;
    memset(observed_slots, 0, sizeof(observed_slots));
    memset(observed_bytes, 0, sizeof(observed_bytes));
    memset(observed_flags, 0, sizeof(observed_flags));
    for (i = 0; i < COMMAND_SLOT_COUNT; ++i)
        for (j = 0; j < COMMAND_RECORD_WORDS; ++j)
            allocation_records[i][j] = 0xa5a5;
    allocation_calls = 0;
    successful_allocations = 0;
    fail_on_call = fail_at;
}

static int test_startup_pool_and_macro_consumer(void)
{
    unsigned long i, j;
    unsigned short **handle;
    unsigned short *record;

    reset_fixture(0);
    CHECK(initialize_command_table(0x14, 0x3781, 0x0e) == 1);
    CHECK(allocation_calls == 14 && successful_allocations == 14);
    for (i = 0; i < 14; ++i) {
        CHECK(observed_slots[i] == (void **)&g_command_slot_storage_71320[i]);
        CHECK(observed_bytes[i] == 0x14 && observed_flags[i] == 0);
        CHECK(g_command_slot_storage_71320[i] == allocation_records[i]);
        CHECK(allocation_records[i][0] == 0x3781);
        for (j = 1; j < COMMAND_RECORD_WORDS; ++j)
            CHECK(allocation_records[i][j] == 0xa5a5);
    }
    for (i = 14; i < COMMAND_SLOT_COUNT; ++i)
        CHECK(g_command_slot_storage_71320[i] == 0);
    CHECK(allocate_command(0x3781) == &g_command_slot_storage_71320[0]);
    CHECK(allocate_command(0x3782) == 0);
    CHECK(allocate_command(0x10003781U) == 0);

    /* calibration/macro_actor.c create_command stores this returned handle,
     * sets record[0]'s 0x8000 active bit, then zeroes words 1 through 9. */
    for (i = 0; i < 14; ++i) {
        handle = allocate_command(0x3781);
        CHECK(handle == &g_command_slot_storage_71320[i]);
        record = *handle;
        CHECK(record == allocation_records[i]);
        record[0] |= 0x8000;
        for (j = 1; j < COMMAND_RECORD_WORDS; ++j)
            record[j] = 0;
        CHECK(record[0] == 0xb781);
        for (j = 1; j < COMMAND_RECORD_WORDS; ++j)
            CHECK(record[j] == 0);
    }
    CHECK(allocate_command(0x3781) == 0);
    CHECK(allocation_calls == 14);

    reset_command_table();
    CHECK(allocation_calls == 14 && successful_allocations == 14);
    for (i = 0; i < COMMAND_SLOT_COUNT; ++i)
        CHECK(g_command_slot_storage_71320[i] == 0);
    return 0;
}

static int test_partial_allocation_is_retained(void)
{
    reset_fixture(3);
    CHECK(initialize_command_table(20, 0x3781, 14) == 0);
    CHECK(allocation_calls == 3 && successful_allocations == 2);
    CHECK(observed_slots[0] == (void **)&g_command_slot_storage_71320[0]);
    CHECK(observed_slots[1] == (void **)&g_command_slot_storage_71320[1]);
    CHECK(observed_slots[2] == (void **)&g_command_slot_storage_71320[2]);
    CHECK(g_command_slot_storage_71320[0] == allocation_records[0]);
    CHECK(g_command_slot_storage_71320[1] == allocation_records[1]);
    CHECK(g_command_slot_storage_71320[2] == 0);
    CHECK(g_command_slot_storage_71320[0][0] == 0x3781);
    CHECK(g_command_slot_storage_71320[1][0] == 0x3781);
    CHECK(allocate_command(0x3781) == &g_command_slot_storage_71320[0]);
    return 0;
}

static int test_capacity_and_repeated_request(void)
{
    unsigned long i;

    reset_fixture(0);
    CHECK(initialize_command_table(20, 0x3781, 14) == 1);
    CHECK(initialize_command_table(20, 0x3781, 14) == 0);
    CHECK(allocation_calls == 20 && successful_allocations == 20);
    for (i = 0; i < COMMAND_SLOT_COUNT; ++i) {
        CHECK(observed_slots[i] == (void **)&g_command_slot_storage_71320[i]);
        CHECK(observed_bytes[i] == 20 && observed_flags[i] == 0);
        CHECK(g_command_slot_storage_71320[i] == allocation_records[i]);
        CHECK(g_command_slot_storage_71320[i][0] == 0x3781);
    }
    CHECK(allocate_command(0x3781) == &g_command_slot_storage_71320[0]);
    return 0;
}

int main(void)
{
    if (test_startup_pool_and_macro_consumer()) return 1;
    if (test_partial_allocation_is_retained()) return 1;
    if (test_capacity_and_repeated_request()) return 1;
    puts("PASS: twenty command slots, measured host allocations, lookup handles, failure ownership");
    return 0;
}
