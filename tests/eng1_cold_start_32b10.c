#include <string.h>
#include "../calibration/engine_interface.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

extern u8 g_cold_start_state_bytes_70820[0x1c];
extern void __cdecl dispatch_local_32b10(void);

typedef void (__cdecl *SlotPair)(void *, const void *);
typedef union SlotBits {
    unsigned long word;
    SlotPair pair;
} SlotBits;
typedef char fixture_requires_32bit_slots[
    (sizeof(unsigned long) == 4 && sizeof(SlotPair) == 4) ? 1 : -1];

static unsigned char g_pool_bytes[0x21000];
static int g_allocate_writes_slot;
static void *g_allocate_return;
static void *g_allocate_slot_initial;
static void *g_allocate_slot_address;
static void *g_file_destination;
static void *g_upload_source;
static void *g_freed_pointer;
static const void *g_upload_rectangle;
static unsigned long g_allocate_bytes;
static unsigned long g_allocate_flags;
static unsigned long g_file_bytes;
static unsigned long g_notice_value;
static char g_prefix[8];
static char g_suffix[64];
static unsigned short g_rectangle_words[4];
static int g_order[8];
static int g_order_count;
static unsigned char g_state_before[0x1c];
static int g_fixture_case;

static void record(int event)
{
    if (g_order_count < (int)(sizeof(g_order) / sizeof(g_order[0])))
        g_order[g_order_count++] = event;
}

static void *__cdecl fixture_allocate(void **output_slot,
                                      unsigned long bytes,
                                      unsigned long flags)
{
    g_allocate_slot_address = output_slot;
    g_allocate_slot_initial = *output_slot;
    g_allocate_bytes = bytes;
    g_allocate_flags = flags;
    if (g_allocate_writes_slot != 0)
        *output_slot = g_pool_bytes;
    else
        *output_slot = (void *)0;
    record(1);
    return g_allocate_return;
}

static unsigned long __cdecl fixture_load_sync(const char *prefix,
                                                const char *suffix,
                                                void *destination,
                                                unsigned long bytes)
{
    strncpy(g_prefix, prefix, sizeof(g_prefix) - 1);
    g_prefix[sizeof(g_prefix) - 1] = 0;
    strncpy(g_suffix, suffix, sizeof(g_suffix) - 1);
    g_suffix[sizeof(g_suffix) - 1] = 0;
    g_file_destination = destination;
    g_file_bytes = bytes;
    record(2);
    return 0;
}

static void __cdecl fixture_upload(void *rectangle, const void *source)
{
    int i;
    g_upload_rectangle = rectangle;
    g_upload_source = (void *)source;
    for (i = 0; i != 4; ++i)
        g_rectangle_words[i] = ((const unsigned short *)rectangle)[i];
    record(3);
}

static void __cdecl fixture_notify(unsigned long value)
{
    g_notice_value = value;
    record(4);
}

static void __cdecl fixture_free(void *pointer)
{
    g_freed_pointer = pointer;
    record(5);
}

static void reset_fixture(int writes_slot)
{
    int i;
    SlotBits bits;

    memset(&g_engine_interface, 0, sizeof(g_engine_interface));
    memset(g_cold_start_state_bytes_70820, 0xa5,
           sizeof(g_cold_start_state_bytes_70820));
    memcpy(g_state_before, g_cold_start_state_bytes_70820,
           sizeof(g_state_before));
    memset(g_pool_bytes, 0x5a, sizeof(g_pool_bytes));
    g_allocate_writes_slot = writes_slot;
    g_allocate_return = writes_slot ? (void *)0 : (void *)g_pool_bytes;
    g_allocate_slot_initial = (void *)0;
    g_allocate_slot_address = (void *)0;
    g_file_destination = (void *)0;
    g_upload_source = (void *)0;
    g_freed_pointer = (void *)0;
    g_upload_rectangle = (const void *)0;
    g_allocate_bytes = 0;
    g_allocate_flags = 0xffffffffUL;
    g_file_bytes = 0;
    g_notice_value = 0xffffffffUL;
    g_prefix[0] = 0;
    g_suffix[0] = 0;
    g_order_count = 0;
    for (i = 0; i < (int)(sizeof(g_order) / sizeof(g_order[0])); ++i)
        g_order[i] = 0;

    g_engine_interface.load_sync_068 = fixture_load_sync;
    g_engine_interface.allocate_078 = fixture_allocate;
    g_engine_interface.free_allocation_040 = fixture_free;
    g_engine_interface.dispatch_2c630_16c = fixture_notify;
    bits.pair = fixture_upload;
    g_engine_interface.opaque_1fc[2] = bits.word;
}

static int test_output_slot_and_order(void)
{
    int i;
    int written[0x1c];
    unsigned char expected[0x1c];
    const unsigned short rect[4] = { 0, 0x0200, 0x0080, 0x0200 };
    const int expected_order[5] = { 1, 2, 3, 4, 5 };

    reset_fixture(1);
    dispatch_local_32b10();

    for (i = 0; i != 0x1c; ++i) {
        written[i] = 0;
        expected[i] = g_state_before[i];
    }
    expected[0] = expected[1] = expected[2] = expected[3] = 0;
    written[0] = written[1] = written[2] = written[3] = 1;
    expected[4] = expected[5] = 0xff;
    expected[6] = expected[7] = 0xff;
    written[4] = written[5] = written[6] = written[7] = 1;
    expected[8] = 2; expected[9] = 3; expected[10] = 2;
    written[8] = written[9] = written[10] = 1;
    expected[16] = expected[17] = expected[18] = expected[19] = 0;
    expected[20] = 0;
    written[16] = written[17] = written[18] = written[19] = 1;
    written[20] = 1;
    expected[24] = 0xd0; expected[25] = 0;
    expected[26] = 0xd0; expected[27] = 0;
    written[24] = written[25] = written[26] = written[27] = 1;

    for (i = 0; i != 0x1c; ++i) {
        if (g_cold_start_state_bytes_70820[i] != expected[i])
            return __LINE__;
        if (!written[i] && g_cold_start_state_bytes_70820[i] !=
                           g_state_before[i])
            return __LINE__;
    }
    if (g_order_count != 5 || memcmp(g_order, expected_order,
                                     sizeof(expected_order)) != 0)
        return __LINE__;
    if (g_allocate_slot_initial != (void *)0 ||
        g_allocate_slot_address == (void *)0 ||
        g_allocate_bytes != 0x21000UL || g_allocate_flags != 0)
        return __LINE__;
    if (g_file_destination != (void *)g_pool_bytes ||
        g_file_bytes != 0x21000UL || strcmp(g_prefix, "") != 0 ||
        strcmp(g_suffix, "m:\\language\\srclang\\stpanel1.dat") != 0)
        return __LINE__;
    if (g_upload_source != (void *)g_pool_bytes || g_upload_rectangle == 0 ||
        memcmp(g_rectangle_words, rect, sizeof(rect)) != 0)
        return __LINE__;
    if (g_notice_value != 0 || g_freed_pointer != (void *)g_pool_bytes)
        return __LINE__;
    return 0;
}

static int test_return_and_null_output_ignored(void)
{
    reset_fixture(0);
    dispatch_local_32b10();

    if (g_order_count != 5 || g_order[0] != 1 || g_order[1] != 2 ||
        g_order[2] != 3 || g_order[3] != 4 || g_order[4] != 5)
        return __LINE__;
    if (g_file_destination != (void *)0 || g_upload_source != (void *)0 ||
        g_freed_pointer != (void *)0)
        return __LINE__;
    return 0;
}

int main(void)
{
    int result;
    result = test_output_slot_and_order();
    if (result != 0)
        return result;
    result = test_return_and_null_output_ignored();
    if (result != 0)
        return result;
    return 0;
}

/* Test-only unrelated completion-gate cell for uncalled wrapper COMDAT. */
unsigned long g_dispatch_callback_flag_71ec0;
