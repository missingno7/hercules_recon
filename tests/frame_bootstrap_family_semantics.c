#include <stdio.h>
#include <string.h>
#include "../calibration/engine_interface.h"
#include "../calibration/engine_dispatch_externs.h"

extern void __cdecl dispatch_local_1f220(void);
extern void __cdecl dispatch_local_1f320(void);
extern void __cdecl dispatch_engine_2c3c0(void **, u32, u32);
extern void __cdecl dispatch_engine_2c7e0(void);
extern EngineNoArgCallback __cdecl dispatch_engine_2c640(EngineNoArgCallback);
extern void __cdecl dispatch_engine_2c260(void);
extern void __cdecl dispatch_engine_2c280(void);
extern void __cdecl dispatch_engine_2c2d0(void *);
extern void __cdecl dispatch_engine_2c2e0(u32);
extern void __cdecl dispatch_engine_2c5e0(const void *, u32, u32, u32);
extern void __cdecl dispatch_engine_2c500(void *, void *);
extern void __cdecl dispatch_engine_2c980(void *, u32, u32, u32, u32);
extern void __cdecl dispatch_engine_2c8f0(void *);
extern void __cdecl dispatch_engine_2c8e0(void *);
extern void __cdecl dispatch_local_02f30(void);
extern void __cdecl dispatch_local_03270(void *, void *);
extern void __cdecl dispatch_local_15090(void);
extern void __cdecl dispatch_local_05dc0(u32);
extern void __cdecl dispatch_local_25620(void);
extern void __cdecl resource_rebuild_blob(void);
extern void __cdecl resource_rebuild_group_rows(void);
extern int __cdecl engine_frame_chain_write(void *, unsigned long);
extern int __cdecl frame_chain_write(void *, unsigned long);

EngineInterface g_engine_interface;
u32 g_dispatch_callback_flag_71ec0;
u8 g_dispatch_key_7286c;
u8 *g_dispatch_frame_current_71ec4;
u8 *g_dispatch_frame_base_71ec8;

u16 g_frame_count_713b4;
u16 g_frame_count_71432;
u16 g_frame_count_713ba;
void *g_frame_owner_72860;
void *g_frame_owner_73494;
void *g_frame_owner_734e0;
void *g_frame_owner_70c80;
void *g_frame_owner_7348c;
void *g_frame_owner_72870;
void *g_frame_owner_734e8;
u32 g_frame_global_70c54;
u16 g_frame_word_6ee84;
u16 g_frame_word_70d12;
u8 g_frame_byte_70d01;
u32 g_frame_global_70d24;
u32 g_frame_global_70ce8;
u32 g_frame_global_70cec;
u32 g_frame_global_70cf4;
u32 g_frame_global_73484;
u32 g_frame_global_734f4;
u8 g_frame_global_734f0;
u8 g_frame_global_734f1;
u8 g_frame_global_734f2;
u8 g_frame_global_73498;
u8 g_frame_config_5df08[28 * 8];
u8 g_frame_descriptor_links_5a69c[24 * 4];
const u8 g_frame_setup_callback_58a60[4] = { 0x10, 0x20, 0x30, 0x40 };

static int failures;
static int checks;
static int events[256];
static int event_count;
static void *alloc_slots[8];
static u32 alloc_bytes[8];
static u32 alloc_flags[8];
static int alloc_count;
static u8 alloc_storage[8][0x8000];
static u8 late_zero_72870[0x8000];
static u8 late_zero_734e8[0x300];
static u8 context_config[0x1b34];
static u8 context_late[0x1b34];
static u8 context_cleanup_initial[0x1b34];
static u8 context_cleanup_after_cancel[0x1b34];
static u8 context_cleanup_after_bulk[0x1b34];
static u8 context_setup_initial[0x1b34];
static u8 context_setup_live[0x1b34];
static u8 frame_old[0x2910];
static u8 frame_a[0x2910];
static u8 frame_b[0x2910];
static u8 frame_current_mutated[0x2910];
static u8 nested_table[24 * 4];
static u8 descriptor_object;
static u8 nested_object;
static int free_count;
static void *free_values[8];
static void *callback_args[4];
static u32 callback_words[4][4];
static int callback_count;
static void *chain_pair[2];
static int chain_pair_count;
static void *descriptor_args[2];
static void *callback0_value;
static int setup_callback_count;
static int cancel_callback_count;
static int config_count;
static int setup_calls_2f30;
static int setup_calls_15090;
static int setup_calls_5dc0;
static int setup_calls_25620;
static int setup_calls_058b0;
static int setup_calls_2f840;
static int setup_calls_2fb70;
static int setup_calls_2da90;
static int setup_calls_262a0;
static int setup_calls_050d0;
static int setup_calls_1f870;
static void *last_2c8f0;
static void *last_2c8e0;
static int current_callback_count;
static void *current_callback_values[2];

#define CHECK(c) do { ++checks; if (!(c)) { ++failures; \
    printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

static void record(int event)
{
    if (event_count < (int)(sizeof(events) / sizeof(events[0])))
        events[event_count++] = event;
}

static void reset_events(void)
{
    event_count = 0;
    memset(events, 0, sizeof(events));
}

static void expect_events(const int *expected, int count)
{
    int i;
    CHECK(event_count == count);
    for (i = 0; i < count && i < event_count; ++i)
        CHECK(events[i] == expected[i]);
}

static void __cdecl cleanup_failed_current(void)
{
    ++cancel_callback_count;
    record(503);
}

void __cdecl dispatch_local_280f0(void)
{
    ++config_count;
    record(10);
}

void __cdecl dispatch_engine_2c3c0(void **owner, u32 bytes, u32 flags)
{
    int index;
    index = alloc_count;
    CHECK(index < 8);
    if (index >= 8)
        return;
    alloc_slots[index] = owner;
    alloc_bytes[index] = bytes;
    alloc_flags[index] = flags;
    memset(alloc_storage[index], 0x5a, sizeof(alloc_storage[index]));
    *owner = alloc_storage[index];
    record(20 + index);
    ++alloc_count;

    if (index == 0)
        g_frame_count_71432 = 3;
    if (index == 1)
        g_frame_count_713ba = 4;
    if (index == 3)
        g_engine_interface.context_004 = (ResourceCallbackContext *)context_late;
    if (index == 7) {
        g_frame_owner_72870 = late_zero_72870;
        g_frame_owner_734e8 = late_zero_734e8;
    }
}

static void __cdecl observe_dispatch_engine_2c7e0(void)
{
    record(30);
}

void __cdecl dispatch_local_05710(void)
{
    record(40);
}

void __cdecl dispatch_local_15600(void)
{
    record(41);
}

static EngineNoArgCallback __cdecl observe_dispatch_engine_2c640(EngineNoArgCallback callback)
{
    (void)callback;
    record(500);
    return 0;
}

static EngineNoArgCallback __cdecl observe_dispatch_engine_2cc40(EngineNoArgCallback callback)
{
    record(501);
    return callback;
}

void __cdecl dispatch_engine_2c260(void)
{
    record(502);
    g_engine_interface.context_004 =
        (ResourceCallbackContext *)context_cleanup_after_cancel;
}

static void __cdecl observe_dispatch_engine_2c280(void)
{
    record(506);
}

static void __cdecl observe_dispatch_engine_2c2d0(void *owner)
{
    if (free_count < 8)
        free_values[free_count] = owner;
    ++free_count;
    record(510 + free_count - 1);
}

static u32 __cdecl observe_dispatch_engine_2cc30(u32 value)
{
    (void)value;
    record(505);
    return 0;
}

static void __cdecl observe_dispatch_engine_2c630(u32 value)
{
    (void)value;
    record(504);
}

static void __cdecl observe_dispatch_engine_2c2e0(u32 value)
{
    u8 *context;
    (void)value;
    record(520);
    context = context_cleanup_after_bulk;
    *(u32 *)(context + 0x7c) = 0x11223344UL;
    *(u32 *)(context + 0x80) = 0x55667788UL;
    *(u32 *)(context + 0x84) = 0x99aabbccUL;
    *(u32 *)(context + 0x88) = 0xddeeff00UL;
    context[0x0f] = 0x7d;
    g_engine_interface.context_004 = (ResourceCallbackContext *)context;
}

static void __cdecl observe_dispatch_engine_2c5e0(const void *descriptor,
                                    u32 a, u32 b, u32 c)
{
    (void)a;
    (void)b;
    (void)c;
    ++setup_callback_count;
    callback0_value = (void *)descriptor;
    record(600);
    g_engine_interface.context_004 = (ResourceCallbackContext *)context_setup_live;
    g_dispatch_frame_base_71ec8 = frame_a;
}

void __cdecl resource_rebuild_blob(void)
{
    record(601);
    g_frame_word_70d12 = 3;
}

void __cdecl dispatch_local_02f30(void)
{
    ++setup_calls_2f30;
    record(602);
}

void __cdecl dispatch_local_03270(void *item, void *child)
{
    descriptor_args[0] = item;
    descriptor_args[1] = child;
    record(603);
}

void __cdecl dispatch_local_15090(void)
{
    ++setup_calls_15090;
    record(604);
}

void __cdecl dispatch_local_1f870(int value)
{
    CHECK(value == 0);
    ++setup_calls_1f870;
    record(605);
}

void __cdecl resource_rebuild_group_rows(void)
{
    record(606);
}

void __cdecl dispatch_local_05dc0(u32 value)
{
    CHECK(value == 0);
    ++setup_calls_5dc0;
    record(607);
}

void __cdecl dispatch_local_25620(void)
{
    ++setup_calls_25620;
    record(608);
}

static void __cdecl observe_dispatch_engine_2c500(void *first, void *second)
{
    chain_pair[0] = first;
    chain_pair[1] = second;
    ++chain_pair_count;
    record(610);
}

static void __cdecl observe_dispatch_engine_2c980(void *pointer, u32 a, u32 b, u32 c, u32 d)
{
    CHECK(callback_count < 4);
    if (callback_count < 4) {
        callback_args[callback_count] = pointer;
        callback_words[callback_count][0] = a;
        callback_words[callback_count][1] = b;
        callback_words[callback_count][2] = c;
        callback_words[callback_count][3] = d;
    }
    ++callback_count;
    record(620 + callback_count - 1);
    if (callback_count == 1)
        g_dispatch_frame_base_71ec8 = frame_b;
}

void __cdecl dispatch_local_058b0(void)
{
    ++setup_calls_058b0;
    record(630);
}

static void __cdecl observe_dispatch_engine_2c570(void *pointer)
{
    if (current_callback_count < 2)
        current_callback_values[current_callback_count] = pointer;
    ++current_callback_count;
    record(631 + current_callback_count - 1);
    if (current_callback_count == 1)
        g_dispatch_frame_current_71ec4 = frame_current_mutated;
}

void __cdecl dispatch_local_2f840(void)
{
    ++setup_calls_2f840;
    record(633);
}

void __cdecl dispatch_local_2fb70(void)
{
    ++setup_calls_2fb70;
    record(634);
}

void __cdecl dispatch_local_2da90(void)
{
    ++setup_calls_2da90;
    record(635);
}

void __cdecl dispatch_local_262a0(void)
{
    ++setup_calls_262a0;
    record(636);
}

void __cdecl dispatch_local_050d0(void)
{
    ++setup_calls_050d0;
    record(637);
}

static void __cdecl observe_dispatch_engine_2c8f0(void *pointer)
{
    last_2c8f0 = pointer;
    record(638);
    g_dispatch_frame_current_71ec4 = frame_b;
}

static void __cdecl observe_dispatch_engine_2c8e0(void *pointer)
{
    last_2c8e0 = pointer;
    record(639);
}

static void clear_bootstrap_state(void)
{
    memset(alloc_slots, 0, sizeof(alloc_slots));
    memset(alloc_bytes, 0, sizeof(alloc_bytes));
    memset(alloc_flags, 0, sizeof(alloc_flags));
    memset(alloc_storage, 0, sizeof(alloc_storage));
    memset(late_zero_72870, 0x66, sizeof(late_zero_72870));
    memset(late_zero_734e8, 0x66, sizeof(late_zero_734e8));
    alloc_count = 0;
    config_count = 0;
    g_frame_count_713b4 = 2;
    g_frame_count_71432 = 9;
    g_frame_count_713ba = 9;
    g_engine_interface.context_004 = (ResourceCallbackContext *)context_config;
    context_late[0] = 0;
    *(u16 *)(g_frame_config_5df08 + 6) = 2;
    g_frame_owner_72860 = 0;
    g_frame_owner_73494 = 0;
    g_frame_owner_734e0 = 0;
    g_frame_owner_70c80 = 0;
    g_frame_owner_7348c = 0;
    g_frame_owner_72870 = 0;
    g_frame_owner_734e8 = 0;
    reset_events();
}

static void test_bootstrap(void)
{
    static const int expected[] = {
        10, 20, 21, 22, 30, 23, 24, 25, 26, 27, 40, 41
    };
    static const u32 sizes[] = {
        616, 1016, 900, 0x2910, 160, 0x4000, 0x8000, 0x300
    };
    static const u32 flags[] = { 16, 16, 16, 16, 16, 0, 0, 0 };
    void **slots[8];
    int i;

    clear_bootstrap_state();
    dispatch_local_1f0f0();
    CHECK(config_count == 1);
    CHECK(alloc_count == 8);
    slots[0] = &g_frame_owner_72860;
    slots[1] = &g_frame_owner_73494;
    slots[2] = &g_frame_owner_734e0;
    slots[3] = (void **)&g_dispatch_frame_base_71ec8;
    slots[4] = &g_frame_owner_70c80;
    slots[5] = &g_frame_owner_7348c;
    slots[6] = &g_frame_owner_72870;
    slots[7] = &g_frame_owner_734e8;
    for (i = 0; i < 8; ++i) {
        CHECK(alloc_slots[i] == slots[i]);
        CHECK(alloc_bytes[i] == sizes[i]);
        CHECK(alloc_flags[i] == flags[i]);
    }
    CHECK(g_frame_owner_72870 == late_zero_72870);
    CHECK(g_frame_owner_734e8 == late_zero_734e8);
    CHECK(late_zero_72870[0] == 0 && late_zero_72870[0x7fff] == 0);
    CHECK(late_zero_734e8[0] == 0 && late_zero_734e8[0x2ff] == 0);
    CHECK(alloc_storage[6][0] == 0x5a);
    CHECK(alloc_storage[7][0] == 0x5a);
    expect_events(expected, (int)(sizeof(expected) / sizeof(expected[0])));
}

static void test_cleanup(void)
{
    static const int expected[] = {
        500, 501, 502, 503, 504, 505, 510, 511, 512, 513, 514, 520
    };
    static const int mode2_expected[] = {
        500, 501, 506, 504, 505, 520
    };
    u8 dummy[5];
    int i;

    memset(context_cleanup_initial, 0, sizeof(context_cleanup_initial));
    memset(context_cleanup_after_cancel, 0, sizeof(context_cleanup_after_cancel));
    memset(context_cleanup_after_bulk, 0, sizeof(context_cleanup_after_bulk));
    ((ResourceCallbackContext *)context_cleanup_initial)->mode_09c = 1;
    ((ResourceCallbackContext *)context_cleanup_after_cancel)->load_failed =
        cleanup_failed_current;
    g_engine_interface.context_004 =
        (ResourceCallbackContext *)context_cleanup_initial;
    g_frame_owner_72860 = &dummy[0];
    g_frame_owner_73494 = &dummy[1];
    g_frame_owner_734e0 = &dummy[2];
    g_frame_owner_70c80 = &dummy[3];
    g_dispatch_frame_base_71ec8 = &dummy[4];
    free_count = 0;
    cancel_callback_count = 0;
    reset_events();

    dispatch_local_1f220();
    CHECK(g_dispatch_callback_flag_71ec0 == 0);
    CHECK(cancel_callback_count == 1);
    CHECK(free_count == 5);
    CHECK(free_values[0] == &dummy[0]);
    CHECK(free_values[1] == &dummy[1]);
    CHECK(free_values[2] == &dummy[2]);
    CHECK(free_values[3] == &dummy[3]);
    CHECK(free_values[4] == &dummy[4]);
    CHECK(*(u32 *)(context_cleanup_after_bulk + 0x6c) == 0x11223344UL);
    CHECK(*(u32 *)(context_cleanup_after_bulk + 0x70) == 0x55667788UL);
    CHECK(*(u32 *)(context_cleanup_after_bulk + 0x74) == 0x99aabbccUL);
    CHECK(*(u32 *)(context_cleanup_after_bulk + 0x78) == 0xddeeff00UL);
    CHECK(context_cleanup_after_bulk[0x0e] == 0x7d);
    expect_events(expected, (int)(sizeof(expected) / sizeof(expected[0])));

    memset(context_cleanup_initial, 0, sizeof(context_cleanup_initial));
    ((ResourceCallbackContext *)context_cleanup_initial)->mode_09c = 2;
    g_engine_interface.context_004 =
        (ResourceCallbackContext *)context_cleanup_initial;
    g_frame_owner_72860 = 0;
    g_frame_owner_73494 = 0;
    g_frame_owner_734e0 = 0;
    g_frame_owner_70c80 = 0;
    g_dispatch_frame_base_71ec8 = 0;
    free_count = 0;
    reset_events();
    dispatch_local_1f220();
    CHECK(free_count == 0);
    expect_events(mode2_expected,
                  (int)(sizeof(mode2_expected) / sizeof(mode2_expected[0])));
}

static void test_setup(void)
{
    static const int required_order[] = {
        600, 601, 602, 603, 604, 605, 606, 607, 608,
        610, 620, 621, 622, 623, 630, 631, 632,
        633, 634, 635, 636, 637, 638, 639
    };
    void *expected_nested;
    int i;

    memset(context_setup_initial, 0x7f, sizeof(context_setup_initial));
    memset(context_setup_live, 0, sizeof(context_setup_live));
    ((ResourceCallbackContext *)context_setup_live)->unknown_000[0] = 1;
    ((ResourceCallbackContext *)context_setup_live)->unknown_000[1] = 2;
    ((ResourceCallbackContext *)context_setup_live)->unknown_128 = 0x40;
    g_frame_config_5df08[28] = 0x11;
    g_frame_config_5df08[29] = 0x22;
    g_frame_config_5df08[30] = 0x33;
    g_frame_config_5df08[32] = 0x03;
    *(u16 *)(g_frame_config_5df08 + 34) = 2;
    *(u32 *)(g_frame_config_5df08 + 40) = 0x12345678UL;
    memset(g_frame_descriptor_links_5a69c, 0,
           sizeof(g_frame_descriptor_links_5a69c));
    memset(nested_table, 0, sizeof(nested_table));
    *(void **)(g_frame_descriptor_links_5a69c + 24 + 4) = &descriptor_object;
    expected_nested = &nested_object;
    *(void **)(nested_table + 2 * 24 + 8) = expected_nested;
    /* The row's observed +12 pointer is the base used by the nested +8 read. */
    *(void **)(g_frame_descriptor_links_5a69c + 24) = nested_table;

    memset(frame_old, 0, sizeof(frame_old));
    memset(frame_a, 0, sizeof(frame_a));
    memset(frame_b, 0, sizeof(frame_b));
    memset(frame_current_mutated, 0, sizeof(frame_current_mutated));
    g_frame_owner_72860 = &descriptor_object;
    g_frame_owner_70c80 = frame_current_mutated;
    g_engine_interface.context_004 =
        (ResourceCallbackContext *)context_setup_initial;
    g_dispatch_frame_base_71ec8 = frame_old;
    g_dispatch_frame_current_71ec4 = frame_b;
    g_dispatch_key_7286c = 0xa5;
    g_frame_global_70c54 = 0xabcdef01UL;
    g_frame_word_6ee84 = 0x1234;
    g_frame_word_70d12 = 9;
    g_frame_byte_70d01 = 0;
    g_frame_global_73484 = 0;
    g_frame_global_734f4 = 0;
    g_frame_global_70d24 = 0;
    g_frame_global_70ce8 = 0;
    g_frame_global_70cec = 0;
    g_frame_global_70cf4 = 0;
    g_frame_global_734f0 = 0;
    g_frame_global_734f1 = 0;
    g_frame_global_734f2 = 0;
    g_frame_global_73498 = 0;
    memset(callback_args, 0, sizeof(callback_args));
    memset(callback_words, 0, sizeof(callback_words));
    memset(current_callback_values, 0, sizeof(current_callback_values));
    callback_count = 0;
    current_callback_count = 0;
    chain_pair_count = 0;
    setup_callback_count = 0;
    setup_calls_2f30 = 0;
    setup_calls_15090 = 0;
    setup_calls_5dc0 = 0;
    setup_calls_25620 = 0;
    setup_calls_058b0 = 0;
    setup_calls_2f840 = 0;
    setup_calls_2fb70 = 0;
    setup_calls_2da90 = 0;
    setup_calls_262a0 = 0;
    setup_calls_050d0 = 0;
    setup_calls_1f870 = 0;
    reset_events();

    /* Exercise real accepted writer + real slot-81 forwarding bridge. */
    g_engine_interface.frame_chain_write_144 = frame_chain_write;
    dispatch_local_1f320();

    CHECK(g_frame_global_70c54 == 0);
    CHECK(g_frame_word_6ee84 == 0);
    CHECK(g_dispatch_key_7286c == 0);
    CHECK(((ResourceCallbackContext *)context_setup_initial)->unknown_000[3] == 0);
    CHECK(((ResourceCallbackContext *)context_setup_initial)->tick_038 == 0);
    CHECK(((ResourceCallbackContext *)context_setup_live)->unknown_128 == 0x44);
    CHECK(setup_callback_count == 1);
    CHECK(callback0_value == g_frame_setup_callback_58a60);
    CHECK(g_frame_global_734f4 == (u32)&descriptor_object);
    CHECK(g_frame_global_70d24 == 0x12345678UL);
    CHECK(g_frame_global_70ce8 == 0xa0UL);
    CHECK(g_frame_global_70cec == 32UL);
    CHECK(descriptor_args[0] == &descriptor_object);
    CHECK(descriptor_args[1] == expected_nested);
    CHECK(g_frame_global_734f0 == 0x11);
    CHECK(g_frame_global_734f1 == 0x22);
    CHECK(g_frame_global_734f2 == 0x33);
    CHECK(g_frame_global_70cf4 == 0xffffffffUL);
    CHECK(g_frame_global_73484 == (u32)frame_current_mutated);
    CHECK(chain_pair_count == 1);
    CHECK(chain_pair[0] == frame_a);
    CHECK(chain_pair[1] == frame_a + 0x1488);
    CHECK(*(void **)(frame_a + 0x88) == frame_a + 0x8c);
    CHECK(*(void **)(frame_a + 0x88 + 0x13fc) == 0);
    CHECK(*(void **)(frame_a + 0x1510) == frame_a + 0x1514);
    CHECK(*(void **)(frame_a + 0x1510 + 0x13fc) == 0);
    CHECK(frame_a[0x149e] == 1 && frame_a[0x16] == 1);
    CHECK(frame_a[0x149f] == 0 && frame_a[0x17] == 0);
    CHECK(frame_a[0x14a0] == 2 && frame_a[0x18] == 2);
    CHECK(callback_count == 4);
    CHECK(callback_args[0] == frame_a + 0x70);
    CHECK(callback_args[1] == frame_b + 0x14f8);
    CHECK(callback_args[2] == frame_b + 0x7c);
    CHECK(callback_args[3] == frame_b + 0x1504);
    for (i = 0; i < 4; ++i) {
        CHECK(callback_words[i][0] == 0);
        CHECK(callback_words[i][1] == 1);
        CHECK(callback_words[i][2] == (i < 2 ? 0x9cUL : 0x9eUL));
        CHECK(callback_words[i][3] == 0);
    }
    CHECK(current_callback_count == 2);
    CHECK(current_callback_values[0] == frame_old);
    CHECK(current_callback_values[1] == frame_current_mutated);
    CHECK(last_2c8f0 == frame_current_mutated);
    CHECK(last_2c8e0 == frame_b + 0x5c);
    CHECK(g_dispatch_frame_current_71ec4 == frame_b + 0x1488);
    CHECK(g_frame_global_73498 == 1);
    CHECK(setup_calls_2f30 == 1 && setup_calls_15090 == 1);
    CHECK(setup_calls_5dc0 == 1 && setup_calls_25620 == 1);
    CHECK(setup_calls_058b0 == 1 && setup_calls_1f870 == 1);
    CHECK(setup_calls_2f840 == 1 && setup_calls_2fb70 == 1);
    CHECK(setup_calls_2da90 == 1 && setup_calls_262a0 == 1);
    CHECK(setup_calls_050d0 == 1);
    expect_events(required_order,
                  (int)(sizeof(required_order) / sizeof(required_order[0])));
}

int main(void)
{
    memset(&g_engine_interface, 0, sizeof(g_engine_interface));
    g_engine_interface.dispatch_2c280_030 = observe_dispatch_engine_2c280;
    g_engine_interface.free_allocation_040 = observe_dispatch_engine_2c2d0;
    g_engine_interface.bulk_release_044 = observe_dispatch_engine_2c2e0;
    g_engine_interface.dispatch_2c5e0_140 = observe_dispatch_engine_2c5e0;
    g_engine_interface.dispatch_2c500_0c8 = observe_dispatch_engine_2c500;
    g_engine_interface.dispatch_2c980_2ac = observe_dispatch_engine_2c980;
    g_engine_interface.dispatch_2c8f0_280 = observe_dispatch_engine_2c8f0;
    g_engine_interface.dispatch_2c8e0_27c = observe_dispatch_engine_2c8e0;
    g_engine_interface.dispatch_2c7e0_1f8 = observe_dispatch_engine_2c7e0;
    g_engine_interface.dispatch_2c640_170 = observe_dispatch_engine_2c640;
    g_engine_interface.dispatch_2cc40_3e4 = observe_dispatch_engine_2cc40;
    g_engine_interface.dispatch_2c630_16c = observe_dispatch_engine_2c630;
    g_engine_interface.dispatch_2c570_0e0 = observe_dispatch_engine_2c570;
    g_engine_interface.dispatch_2cc30_3e0 = observe_dispatch_engine_2cc30;
    test_bootstrap();
    test_cleanup();
    test_setup();
    if (failures != 0) {
        printf("frame bootstrap fixture: %d failures\n", failures);
        return 1;
    }
    printf("frame bootstrap fixture: %d checks passed\n", checks);
    return 0;
}
