#include "../calibration/engine_dispatch_externs.h"
#include <stdio.h>
#include <string.h>

void __cdecl engine_dispatch(void);

#define MAX_EVENTS 512
#define MAX_CALLS 16
static unsigned long event_log[MAX_EVENTS];
static unsigned long event_count;
static int checks;
static int failures;
static int mutate_entry_on_first_init_call;
static int switch_context_on_first_restart;
static int continuation_values[MAX_CALLS];
static int continuation_count;
static int continuation_index;
static unsigned long cc30_args[MAX_CALLS];
static unsigned long cc30_results[MAX_CALLS];
static int cc30_count;
static unsigned long ca40_args[MAX_CALLS];
static int ca40_count;
static unsigned long last_descriptor;
static int last_2c840_level;
static int last_2c840_mode;
static void *frame_args[MAX_CALLS];
static int frame_arg_count;
static void *frame88_args[MAX_CALLS];
static int frame88_arg_count;
static void (__cdecl *registered_callback)(void);
static int registered_callback_count;
static int two_arg_left;
static int two_arg_right;
static unsigned char frame_storage[2 * 0x1488 + 32];
static ResourceCallbackContext context_a;
static ResourceCallbackContext context_b;
static ResourceCallbackContext context_c;

EngineInterface g_engine_interface;
u32 g_dispatch_entry_state_6fcec;
u8 g_dispatch_startup_gate_6fce8;
u32 g_dispatch_global_58b00;
u32 g_dispatch_global_58af8;
u32 g_dispatch_descriptors_5a6a4[64];
u32 g_dispatch_callback_flag_71ec0;
u8 g_dispatch_key_7286c;
u16 g_dispatch_setting_4d858;
u16 g_dispatch_word_6ee80;
u8 *g_dispatch_frame_current_71ec4;
u8 *g_dispatch_frame_base_71ec8;
u32 g_dispatch_global_6ff88;
const u8 g_dispatch_diagnostic_58ac8[] = "diagnostic fixture";

static void event(unsigned long rva)
{
    if (event_count < MAX_EVENTS)
        event_log[event_count++] = rva;
}

static int count_event(unsigned long rva)
{
    unsigned long i;
    int n;
    n = 0;
    for (i = 0; i < event_count; ++i)
        if (event_log[i] == rva)
            ++n;
    return n;
}

static void check(int condition, const char *name)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", name);
    }
}

void __cdecl dispatch_engine_2c860(int a, int b)
{
    event(0x2c860);
    two_arg_left = a;
    two_arg_right = b;
    if (mutate_entry_on_first_init_call) {
        g_dispatch_entry_state_6fcec = 9;
        mutate_entry_on_first_init_call = 0;
    }
}

#define MOCK0(name, rva) void __cdecl name(void) { event(rva); }
MOCK0(dispatch_engine_2c350, 0x2c350)
MOCK0(dispatch_local_083d3, 0x083d3)

void __cdecl dispatch_local_082b7(int value)
{
    event(0x082b7);
    check(value == 0, "initial custom helper receives zero");
}

void __cdecl dispatch_engine_2c3b0(u32 value)
{
    event(0x2c3b0);
    last_descriptor = value;
}

u32 __cdecl dispatch_engine_2cc30(u32 value)
{
    int slot;
    event(0x2cc30);
    slot = cc30_count++;
    if (slot < MAX_CALLS) {
        cc30_args[slot] = value;
        cc30_results[slot] = 0x70000000UL + (unsigned long)slot;
    }
    return 0x70000000UL + (unsigned long)slot;
}

void __cdecl dispatch_local_0827f(int value)
{
    event(0x0827f);
    check(value == 1, "initial custom helper receives one");
}

MOCK0(dispatch_local_1f0f0, 0x1f0f0)
MOCK0(dispatch_local_1f750, 0x1f750)
MOCK0(dispatch_local_32b10, 0x32b10)
MOCK0(dispatch_local_255f0, 0x255f0)
MOCK0(dispatch_local_05ea0, 0x05ea0)

void __cdecl dispatch_engine_2c840(int level, int mode)
{
    event(0x2c840);
    last_2c840_level = level;
    last_2c840_mode = mode;
}

void __cdecl dispatch_local_1f320(void)
{
    event(0x1f320);
    if (switch_context_on_first_restart) {
        g_engine_interface.context_004 = &context_b;
        switch_context_on_first_restart = 0;
    }
}

void __cdecl dispatch_engine_2c5a0(u32 value)
{
    event(0x2c5a0);
    check(value == 4, "restart helper receives four");
}

MOCK0(dispatch_engine_2c360, 0x2c360)

void __cdecl dispatch_engine_2c540(u32 a, u32 b)
{
    event(0x2c540);
    check(a == 0 && b == 0, "startup-gated helper receives two zeros");
}

EngineNoArgCallback __cdecl dispatch_engine_2cc40(EngineNoArgCallback callback)
{
    EngineNoArgCallback previous = registered_callback;
    event(0x2cc40);
    registered_callback = callback;
    ++registered_callback_count;
    return previous;
}

void __cdecl dispatch_engine_2c4e0(u32 value)
{
    event(0x2c4e0);
    check(value == 0, "restart helper receives zero");
}

void __cdecl dispatch_engine_2c520(u32 value)
{
    event(0x2c520);
    check(value == 1, "restart helper receives one");
}

void __cdecl dispatch_local_01000(const void *value)
{
    event(0x01000);
    check(value == g_dispatch_diagnostic_58ac8, "diagnostic callee receives data address");
}

MOCK0(dispatch_engine_2c270, 0x2c270)
MOCK0(dispatch_local_0e670, 0x0e670)

u32 __cdecl dispatch_engine_2ca40(u32 value)
{
    int slot;
    event(0x2ca40);
    slot = ca40_count++;
    if (slot < MAX_CALLS)
        ca40_args[slot] = value;
    return 0x40000000UL + (unsigned long)slot;
}

MOCK0(dispatch_engine_2c420, 0x2c420)
MOCK0(dispatch_local_06005, 0x06005)
MOCK0(dispatch_local_02180, 0x02180)

void __cdecl dispatch_local_1f870(int value)
{
    event(0x1f870);
    check(value == 1, "iteration helper receives one");
}

MOCK0(dispatch_engine_2c440, 0x2c440)
MOCK0(dispatch_local_05600, 0x05600)
MOCK0(dispatch_local_2fb70, 0x2fb70)
MOCK0(dispatch_local_2f840, 0x2f840)
MOCK0(dispatch_local_258e0, 0x258e0)
MOCK0(dispatch_local_034d0, 0x034d0)
MOCK0(dispatch_local_156a0, 0x156a0)
MOCK0(dispatch_local_1fd90, 0x1fd90)
MOCK0(dispatch_local_25d00, 0x25d00)
MOCK0(dispatch_local_256e0, 0x256e0)
MOCK0(dispatch_local_03470, 0x03470)
MOCK0(dispatch_local_05d50, 0x05d50)
MOCK0(dispatch_local_03a50, 0x03a50)

void __cdecl dispatch_local_32bd0(void)
{
    event(0x32bd0);
    g_engine_interface.context_004->unknown_040 = 1;
}

MOCK0(dispatch_local_2da90, 0x2da90)
MOCK0(dispatch_local_1f6f0, 0x1f6f0)

void __cdecl dispatch_engine_2c630(u32 value)
{
    event(0x2c630);
    check(value == 0, "per-iteration callback receives zero");
}

void __cdecl dispatch_engine_2c570(void *value)
{
    event(0x2c570);
    if (frame_arg_count < MAX_CALLS)
        frame_args[frame_arg_count++] = value;
}

MOCK0(dispatch_local_02cb0, 0x02cb0)

void __cdecl dispatch_local_050d0(void)
{
    event(0x050d0);
    g_engine_interface.context_004->unknown_034 = 0xdeadbeefUL;
}

MOCK0(dispatch_local_058b0, 0x058b0)

void __cdecl dispatch_engine_2c620(void *value)
{
    event(0x2c620);
    if (frame88_arg_count < MAX_CALLS)
        frame88_args[frame88_arg_count++] = value;
}

MOCK0(dispatch_local_0666b, 0x0666b)

int __cdecl dispatch_local_060f9(void)
{
    int result;
    event(0x060f9);
    if (continuation_index < continuation_count)
        result = continuation_values[continuation_index++];
    else
        result = 0;
    return result;
}

void __cdecl dispatch_completion_gate(void)
{
    event(0x1f0e0);
}

static void reset_fixture(void)
{
    unsigned long i;
    unsigned char *saved_frame;
    saved_frame = frame_storage;
    memset(&g_engine_interface, 0, sizeof(g_engine_interface));
    memset(&context_a, 0, sizeof(context_a));
    memset(&context_b, 0, sizeof(context_b));
    memset(&context_c, 0, sizeof(context_c));
    for (i = 0; i < MAX_EVENTS; ++i)
        event_log[i] = 0;
    for (i = 0; i < MAX_CALLS; ++i) {
        cc30_args[i] = 0;
        cc30_results[i] = 0;
        ca40_args[i] = 0;
        frame_args[i] = 0;
        frame88_args[i] = 0;
        continuation_values[i] = 0;
    }
    event_count = 0;
    continuation_count = 0;
    continuation_index = 0;
    cc30_count = 0;
    ca40_count = 0;
    last_descriptor = 0;
    last_2c840_level = -1;
    last_2c840_mode = -1;
    frame_arg_count = 0;
    frame88_arg_count = 0;
    registered_callback = 0;
    registered_callback_count = 0;
    two_arg_left = -1;
    two_arg_right = -1;
    mutate_entry_on_first_init_call = 0;
    switch_context_on_first_restart = 0;
    g_dispatch_entry_state_6fcec = 0;
    g_dispatch_startup_gate_6fce8 = 0;
    g_dispatch_global_58b00 = 0x11223344UL;
    g_dispatch_global_58af8 = 0x55667788UL;
    memset(g_dispatch_descriptors_5a6a4, 0, sizeof(g_dispatch_descriptors_5a6a4));
    g_dispatch_callback_flag_71ec0 = 0;
    g_dispatch_key_7286c = 0;
    g_dispatch_setting_4d858 = 0;
    g_dispatch_word_6ee80 = 0;
    g_dispatch_frame_base_71ec8 = saved_frame;
    g_dispatch_frame_current_71ec4 = saved_frame;
    g_dispatch_global_6ff88 = 100;
    g_engine_interface.context_004 = &context_a;
    memset(frame_storage, 0, sizeof(frame_storage));
}

int main(void)
{
    unsigned long before_events;
    int before_tick;

    reset_fixture();
    context_a.unknown_000[0] = 2;
    context_a.unknown_000[4] = 0;
    context_b.unknown_000[0] = 3;
    g_dispatch_descriptors_5a6a4[12] = 0x5566aa55UL;
    g_dispatch_entry_state_6fcec = 0;
    mutate_entry_on_first_init_call = 1;
    switch_context_on_first_restart = 1;
    continuation_values[0] = 1;
    continuation_values[1] = 0;
    continuation_count = 2;
    context_a.tick_038 = 9;
    engine_dispatch();
    check(g_dispatch_entry_state_6fcec == 1, "entry state is sampled before callees mutate it");
    check(two_arg_left == 8 && two_arg_right == 8, "initial engine bridge gets 8,8");
    check(last_descriptor == 0x5566aa55UL, "signed level selects 24-byte descriptor member");
    check(last_2c840_level == 2 && last_2c840_mode == 0, "initial final bridge gets signed level and zero");
    check(context_a.unknown_000[1] == 0 && context_a.unknown_000[2] == 0 && context_a.unknown_000[0x0a] == 1, "initial byte writes stay in sampled context");
    check(context_a.unknown_08c == 0x11223344UL && context_a.unknown_094 == 0x55667788UL, "initial global DWORD snapshots");
    check(context_a.unknown_048 == 0x13546547UL && context_a.unknown_04c == 0xecab9ab8UL, "initial fixed DWORD values");
    check(count_event(0x2c860) == 1 && count_event(0x1f0f0) == 1, "initial path runs once despite continuation restart");
    check(count_event(0x1f320) == 2, "common restart repeats exactly once");
    check(count_event(0x2c540) == 1, "startup gate is independent and only first restart calls wrapper");
    check(count_event(0x2cc40) == 2 && registered_callback_count == 2 && registered_callback == dispatch_completion_gate, "restart forwards live completion callback pointer");
    check(count_event(0x0e670) == 2, "iteration tests live context state byte three");
    check(count_event(0x32bd0) == 2 && count_event(0x2da90) == 0, "second live DWORD test sees mutation by first callee");
    check(cc30_count == 7 && cc30_args[0] == 1 && cc30_args[1] == 1 && cc30_args[2] == 0 && cc30_args[3] == 0, "service call arguments and independent updates");
    check(context_b.unknown_1b30 == 256 && context_b.unknown_03c == 100 && context_b.unknown_054 == cc30_results[6], "restart and iteration write through current context pointer");
    check(context_b.unknown_000[3] == 5 && context_b.unknown_000[4] == 0 && context_b.unknown_000[5] == 1 && context_b.unknown_040 == 1, "restart context state and callback mutation");
    check(context_b.tick_038 == 2 && context_a.tick_038 == 9, "both iterations increment the current live context tick");
    check(g_dispatch_frame_current_71ec4 == g_dispatch_frame_base_71ec8, "paired frame selection toggles twice back to base");
    check(frame_arg_count == 2 && frame_args[0] == frame_storage && frame_args[1] == frame_storage + 0x1488, "frame bridge receives current and alternate frame");
    check(frame88_arg_count == 2 && frame88_args[0] == frame_storage + 0x88 && frame88_args[1] == frame_storage + 0x1488 + 0x88, "frame subview keeps the observed 0x88 offset");
    check(ca40_count == 4 && ca40_args[0] == g_engine_interface.opaque_010[0] + 0x400UL && ca40_args[1] == 0xdeadbeefUL, "interface DWORD argument and live context reload");
    check(count_event(0x060f9) == 2, "continuation test runs once per iteration");

    reset_fixture();
    context_c.unknown_000[0] = 3;
    context_c.unknown_000[4] = 1;
    context_c.unknown_040 = 1;
    g_engine_interface.context_004 = &context_c;
    g_dispatch_entry_state_6fcec = 1;
    g_dispatch_startup_gate_6fce8 = 1;
    continuation_values[0] = 0;
    continuation_count = 1;
    context_c.tick_038 = 4;
    engine_dispatch();
    check(g_dispatch_entry_state_6fcec == 1 && context_c.tick_038 == 5, "state-one enters one iteration without restart");
    check(count_event(0x2c860) == 0 && count_event(0x1f320) == 0 && count_event(0x2cc40) == 0, "state-one skips initial and common restart phases");
    check(count_event(0x0e670) == 1, "state-one still checks the live level gate");
    check(count_event(0x2fb70) == 0 && count_event(0x256e0) == 1, "live byte selects alternate per-iteration branch");
    check(count_event(0x32bd0) == 0 && count_event(0x2da90) == 0, "nonzero context DWORD skips both independent tests");
    check(g_dispatch_frame_current_71ec4 == g_dispatch_frame_base_71ec8 + 0x1488, "single iteration selects second frame");

    reset_fixture();
    g_dispatch_entry_state_6fcec = 7;
    context_a.tick_038 = 17;
    before_events = event_count;
    before_tick = (int)context_a.tick_038;
    engine_dispatch();
    check(event_count == before_events && context_a.tick_038 == (unsigned long)before_tick && g_dispatch_entry_state_6fcec == 7, "other sampled DWORD state returns without work");

    printf("engine dispatch fixture: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
