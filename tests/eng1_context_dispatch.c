#include <stdio.h>
#include "../src/shared/engine_interface.h"

extern void __cdecl dispatch_local_1fd20(void);
extern void __cdecl dispatch_local_1fd90(void);

EngineInterface g_engine_interface;
static ResourceCallbackContext g_context;
static int g_calls;
static int g_target;

static void record_target(int target)
{
    ++g_calls;
    g_target = target;
}

#define TARGET_STUB(name, number) \
    void __cdecl name(void) { record_target(number); }

TARGET_STUB(eng1_native_code_target_003ca10, 1)
TARGET_STUB(eng1_native_code_target_00e3e0, 2)
TARGET_STUB(eng1_native_code_target_0033700, 3)
TARGET_STUB(eng1_native_code_target_001ac10, 4)
TARGET_STUB(eng1_native_code_target_0021050, 5)
TARGET_STUB(eng1_native_code_target_003c170, 6)
TARGET_STUB(eng1_native_code_target_0044120, 7)
TARGET_STUB(eng1_native_code_target_003ca50, 8)
TARGET_STUB(eng1_native_code_target_0033720, 9)
TARGET_STUB(eng1_native_code_target_001ae00, 10)
TARGET_STUB(eng1_native_code_target_0021210, 11)
TARGET_STUB(eng1_native_code_target_0044240, 12)

static int expected_setup(int selector)
{
    switch (selector) {
    case 1: return 1;
    case 3: return 2;
    case 5: return 3;
    case 6: return 4;
    case 7: return 5;
    case 9: return 6;
    case 11: return 7;
    default: return 0;
    }
}

static int expected_tick(int selector)
{
    switch (selector) {
    case 1: return 8;
    case 5: return 9;
    case 6: return 10;
    case 7: return 11;
    case 11: return 12;
    default: return 0;
    }
}

static int check(int selector, int target, int tick)
{
    int expected = tick ? expected_tick(selector) : expected_setup(selector);

    g_context.unknown_000[0] = (unsigned char)(signed char)selector;
    g_calls = 0;
    g_target = 0;
    if (tick)
        dispatch_local_1fd90();
    else
        dispatch_local_1fd20();
    if (g_calls != (expected ? 1 : 0) ||
        g_target != (expected ? expected : 0)) {
        printf("selector %d %s: expected target %d/calls %d, got %d/%d\n",
               selector, tick ? "tick" : "setup", expected,
               expected ? 1 : 0, g_target, g_calls);
        return 0;
    }
    if (target != expected)
        return 0;
    return 1;
}

int main(void)
{
    int selector;

    g_engine_interface.context_004 = &g_context;
    for (selector = -128; selector <= 127; ++selector) {
        if (!check(selector, expected_setup(selector), 0))
            return 1;
        if (!check(selector, expected_tick(selector), 1))
            return 2;
    }
    puts("ENG1 resource category dispatch fixture passed (512 selector checks)");
    return 0;
}
