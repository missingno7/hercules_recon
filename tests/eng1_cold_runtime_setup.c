#include <string.h>
#include "../calibration/engine_interface.h"
#include "../calibration/engine_fx_record.h"

extern int g_context_sequence_phase;
extern int g_context_sequence_index;
extern unsigned short *g_context_sequence_words;
extern char g_cold_file_prefix_byte_6eef4;
extern void __cdecl dispatch_local_05ea0(void);
extern void __cdecl engine_init(void);

EngineInterface g_engine_interface;
ResourceCallbackContext g_test_context;

static unsigned char g_allocated_bytes[0x1800];
static int g_allocate_calls;
static int g_file_calls;
static unsigned long g_allocate_size;
static unsigned long g_allocate_flags;
static unsigned long g_file_size;
static void *g_file_destination;
static char g_file_prefix_seen[8];
static char g_file_suffix_seen[32];
static char g_debug_message_seen[32];
static int g_debug_calls;
static int g_order[32];
static int g_order_count;
static unsigned long g_stopped_channels[24];
static int g_stopped_count;

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (0)

void *__cdecl engine_pool_allocate(void **output_slot, unsigned long bytes,
                                   unsigned long flags)
{
    ++g_allocate_calls;
    g_allocate_size = bytes;
    g_allocate_flags = flags;
    *output_slot = g_allocated_bytes;
    return g_allocated_bytes;
}

unsigned long __cdecl engine_file_load_sync(const char *prefix,
                                             const char *suffix,
                                             void *destination,
                                             unsigned long bytes)
{
    ++g_file_calls;
    strncpy(g_file_prefix_seen, prefix, sizeof(g_file_prefix_seen) - 1);
    g_file_prefix_seen[sizeof(g_file_prefix_seen) - 1] = 0;
    strncpy(g_file_suffix_seen, suffix, sizeof(g_file_suffix_seen) - 1);
    g_file_suffix_seen[sizeof(g_file_suffix_seen) - 1] = 0;
    g_file_destination = destination;
    g_file_size = bytes;
    ((unsigned char *)destination)[0] = 0x34;
    ((unsigned char *)destination)[1] = 0x12;
    ((unsigned char *)destination)[2] = 0x80;
    ((unsigned char *)destination)[3] = 0x00;
    ((unsigned char *)destination)[4] = 0x56;
    return 0;
}

/* Instrument the known no-op 0x1000 entry so this fixture can verify that
 * engine_init makes the call with the native message at the correct point. */
void __cdecl dispatch_local_01000(const void *message)
{
    ++g_debug_calls;
    strncpy(g_debug_message_seen, (const char *)message,
            sizeof(g_debug_message_seen) - 1);
    g_debug_message_seen[sizeof(g_debug_message_seen) - 1] = 0;
    g_order[g_order_count++] = 4;
}

static void __cdecl mark_begin(void)
{
    g_order[g_order_count++] = 1;
}

static void __cdecl mark_end(void)
{
    g_order[g_order_count++] = 3;
}

static void __cdecl mark_stop(unsigned long channel)
{
    g_order[g_order_count++] = 2;
    g_stopped_channels[g_stopped_count++] = channel;
}

static void reset_cold_fixture(void)
{
    memset(&g_engine_interface, 0, sizeof(g_engine_interface));
    memset(&g_test_context, 0, sizeof(g_test_context));
    memset(g_allocated_bytes, 0, sizeof(g_allocated_bytes));
    g_engine_interface.context_004 = &g_test_context;
    g_context_sequence_words = 0;
    g_context_sequence_phase = 91;
    g_context_sequence_index = 92;
    g_cold_file_prefix_byte_6eef4 = 0;
    g_allocate_calls = 0;
    g_file_calls = 0;
    g_allocate_size = 0;
    g_allocate_flags = 0xffffffffUL;
    g_file_size = 0;
    g_file_destination = 0;
    g_file_prefix_seen[0] = 0;
    g_file_suffix_seen[0] = 0;
    g_debug_message_seen[0] = 0;
    g_debug_calls = 0;
}

static int test_cold_state_3(void)
{
    reset_cold_fixture();
    g_test_context.unknown_000[6] = 1;
    *(unsigned long *)(void *)((unsigned char *)&g_test_context + 0x30) =
        0x01000000UL;
    dispatch_local_05ea0();
    CHECK(g_test_context.unknown_000[6] == 3);
    CHECK(g_context_sequence_index == 0x1518);
    CHECK(g_context_sequence_phase == 91);
    CHECK(g_allocate_calls == 0 && g_file_calls == 0);
    return 0;
}

static int test_cold_gate(void)
{
    reset_cold_fixture();
    g_test_context.unknown_000[6] = 2;
    dispatch_local_05ea0();
    CHECK(g_context_sequence_index == 92 && g_context_sequence_phase == 91);
    CHECK(g_allocate_calls == 0 && g_file_calls == 0);
    return 0;
}

static int test_cold_load(unsigned char state, unsigned long flags,
                          unsigned char selector, const char *expected_path)
{
    unsigned long context_flags;
    unsigned long *masks;

    reset_cold_fixture();
    g_test_context.unknown_000[6] = state;
    g_test_context.unknown_000[8] = selector;
    context_flags = flags | 0x00000005UL;
    *(unsigned long *)(void *)((unsigned char *)&g_test_context + 0x30) =
        context_flags;
    dispatch_local_05ea0();

    CHECK(g_allocate_calls == 1 && g_file_calls == 1);
    CHECK(g_allocate_size == 0x1800UL && g_allocate_flags == 0);
    CHECK(g_file_size == 0x1800UL);
    CHECK(g_file_destination == g_allocated_bytes);
    CHECK(g_context_sequence_words == (unsigned short *)(void *)g_allocated_bytes);
    CHECK(g_file_prefix_seen[0] == 0);
    CHECK(strcmp(g_file_suffix_seen, expected_path) == 0);
    CHECK(g_test_context.unknown_000[1] == 0x34);
    CHECK(g_test_context.unknown_000[2] == 0x34);
    CHECK(*(unsigned long *)(void *)((unsigned char *)&g_test_context + 0x30) ==
          (context_flags | 0x0080UL));
    CHECK(g_test_context.unknown_000[0x0e] == 0x56);
    CHECK(g_test_context.unknown_000[0x0c] == 3);
    masks = (unsigned long *)(void *)((unsigned char *)&g_test_context + 0x6c);
    CHECK(masks[0] == 0x40008000UL);
    CHECK(masks[1] == 0x10002000UL);
    CHECK(masks[2] == 0x08000400UL);
    CHECK(masks[3] == 0x02000100UL);
    CHECK(g_context_sequence_index == 3 && g_context_sequence_phase == 2);
    return 0;
}

static int test_engine_init(void)
{
    int i;
    int result;

    memset(&g_engine_interface, 0, sizeof(g_engine_interface));
    g_engine_interface.opaque_174[1] = (unsigned long)mark_begin;
    g_engine_interface.opaque_174[3] = (unsigned long)mark_end;
    g_engine_interface.opaque_334[20] = (unsigned long)mark_stop;
    for (i = 0; i < 24; ++i) {
        memset(&g_engine_fx_records[i], 0x5a, sizeof(g_engine_fx_records[i]));
        g_engine_fx_records[i].active_03c = 0;
    }
    g_engine_fx_records[1].active_03c = 1;
    g_engine_fx_records[7].active_03c = 2;
    g_engine_fx_records[23].active_03c = 3;
    g_order_count = 0;
    g_stopped_count = 0;
    g_debug_message_seen[0] = 0;
    g_debug_calls = 0;

    engine_init();

    CHECK(g_order_count == 6);
    CHECK(g_order[0] == 1 && g_order[1] == 4 && g_order[2] == 2 &&
          g_order[3] == 2 && g_order[4] == 2 && g_order[5] == 3);
    CHECK(g_debug_calls == 1);
    CHECK(strcmp(g_debug_message_seen, "\n ClearAllSoundFx()") == 0);
    CHECK(g_stopped_count == 3);
    CHECK(g_stopped_channels[0] == 1 && g_stopped_channels[1] == 7 &&
          g_stopped_channels[2] == 23);
    for (i = 0; i < 24; ++i) {
        CHECK(*(unsigned short *)(void *)&g_engine_fx_records[i].unknown_00c[0]
              == 0x00ff);
        CHECK(*(unsigned short *)(void *)&g_engine_fx_records[i].unknown_00c[2]
              == 0);
        CHECK(g_engine_fx_records[i].unknown_030 == 0);
        CHECK(g_engine_fx_records[i].effect_id_008 == 0x5a5a);
    }
    CHECK(g_engine_fx_records[1].active_03c == 1);
    result = 0;
    return result;
}

int main(void)
{
    int result;

    result = test_cold_state_3();
    if (result != 0) return result;
    result = test_cold_gate();
    if (result != 0) return result;
    result = test_cold_load(1, 0x02000000UL, 5,
                            "M:\\DEMO\\DEMO.DAT");
    if (result != 0) return result;
    result = test_cold_load(1, 0, 5,
                            "M:\\DEMO\\DEMO3_1.DAT");
    if (result != 0) return result;
    result = test_cold_load(2, 0x08000000UL, 7,
                            "M:\\DEMO\\DEMO5_2.DAT");
    if (result != 0) return result;
    return test_engine_init();
}
