#include <stdio.h>
#include "../calibration/engine_file_bridge.c"
#include "../calibration/engine_dependency_bridges.c"
#include "../calibration/dispatcher_bridges.c"

EngineInterface g_engine_interface;
unsigned long g_dispatch_callback_flag_71ec0;
unsigned char *g_resource_root;

static unsigned long checks;
static unsigned long failures;
static unsigned long calls_noarg;
static unsigned long calls_one;
static unsigned long calls_two;
static unsigned long last_first;
static unsigned long last_second;
static void *last_pointer;
static int last_signed_first;
static int last_signed_second;
static unsigned long gate_calls;
static EngineNoArgCallback installed_callback;
static unsigned long ret_a_calls;
static unsigned long ret_b_calls;
static unsigned long ret_a_argument;
static unsigned long ret_b_argument;

static void verify(int condition)
{
    ++checks;
    if (!condition)
        ++failures;
}

static void callback_noarg(void)
{
    ++calls_noarg;
}

static void callback_one(unsigned long value)
{
    ++calls_one;
    last_first = value;
}

static void callback_two(unsigned long first, unsigned long second)
{
    ++calls_two;
    last_first = first;
    last_second = second;
}

static void callback_signed_two(int first, int second)
{
    ++calls_two;
    last_signed_first = first;
    last_signed_second = second;
}

static void callback_pointer(void *value)
{
    last_pointer = value;
}

static unsigned long callback_return_a(unsigned long value)
{
    ++ret_a_calls;
    ret_a_argument = value;
    return value ^ 0x13579bdfUL;
}

static unsigned long callback_return_b(unsigned long value)
{
    ++ret_b_calls;
    ret_b_argument = value;
    return value ^ 0x2468ace0UL;
}

static unsigned long callback_return_host_zero(unsigned long value)
{
    last_first = value;
    return 0x87654321UL ^ value;
}

static EngineNoArgCallback callback_setter(EngineNoArgCallback callback)
{
    EngineNoArgCallback previous = installed_callback;
    installed_callback = callback;
    return previous;
}

static void callback_gate_a(void)
{
    ++gate_calls;
}

static void callback_gate_b(void)
{
    gate_calls += 10;
}

void __cdecl resource_rebuild_blob(void)
{
}

void __cdecl resource_rebuild_group_rows(void)
{
}

int main(void)
{
    unsigned long marker = 0;
    void *pointer_a = &marker;
    void *pointer_b = (void *)((unsigned char *)&marker + 1);
    EngineNoArgCallback previous;

    g_engine_interface.dispatch_2c860_24c = callback_signed_two;
    dispatch_engine_2c860(0x11223344, -7);
    verify(calls_two == 1 && last_signed_first == 0x11223344 &&
           last_signed_second == -7);

    g_engine_interface.dispatch_2c350_05c = callback_noarg;
    g_engine_interface.dispatch_2c360_060 = callback_noarg;
    g_engine_interface.dispatch_2c420_084 = callback_noarg;
    g_engine_interface.dispatch_2c440_08c = callback_noarg;
    g_engine_interface.dispatch_2c270_02c = callback_noarg;
    dispatch_engine_2c350();
    dispatch_engine_2c360();
    dispatch_engine_2c420();
    dispatch_engine_2c440();
    dispatch_engine_2c270();
    verify(calls_noarg == 5);

    g_engine_interface.dispatch_2c3b0_06c = callback_one;
    dispatch_engine_2c3b0(0xdeadbeefUL);
    verify(calls_one == 1 && last_first == 0xdeadbeefUL);

    g_engine_interface.dispatch_2cc30_3e0 = callback_return_a;
    verify(dispatch_engine_2cc30(0x01020304UL) ==
           (0x01020304UL ^ 0x13579bdfUL));
    verify(ret_a_calls == 1 && ret_a_argument == 0x01020304UL);
    g_engine_interface.dispatch_2cc30_3e0 = callback_return_b;
    verify(dispatch_engine_2cc30(0xfedcba98UL) ==
           (0xfedcba98UL ^ 0x2468ace0UL));
    verify(ret_b_calls == 1 && ret_b_argument == 0xfedcba98UL);

    g_engine_interface.dispatch_2c840_244 = callback_signed_two;
    dispatch_engine_2c840(-2, 0x55667788);
    verify(calls_two == 2 && last_signed_first == -2 &&
           last_signed_second == 0x55667788);

    g_engine_interface.dispatch_2c5a0_0ec = callback_one;
    dispatch_engine_2c5a0(4);
    verify(calls_one == 2 && last_first == 4);

    g_engine_interface.dispatch_2c540_0d4 = callback_two;
    dispatch_engine_2c540(0x12345678UL, 0xabcdef01UL);
    verify(calls_two == 3 && last_first == 0x12345678UL &&
           last_second == 0xabcdef01UL);

    g_engine_interface.dispatch_2cc40_3e4 = callback_setter;
    installed_callback = callback_gate_a;
    previous = dispatch_engine_2cc40(callback_gate_b);
    verify(previous == callback_gate_a && installed_callback == callback_gate_b);
    previous = dispatch_engine_2cc40(callback_gate_a);
    verify(previous == callback_gate_b && installed_callback == callback_gate_a);

    g_engine_interface.dispatch_2c4e0_0b8 = callback_one;
    dispatch_engine_2c4e0(0x76543210UL);
    verify(calls_one == 3 && last_first == 0x76543210UL);
    g_engine_interface.dispatch_2c520_0cc = callback_one;
    dispatch_engine_2c520(1);
    verify(calls_one == 4 && last_first == 1);

    g_engine_interface.dispatch_2ca40_2ec = callback_return_host_zero;
    verify(dispatch_engine_2ca40(0x0badc0deUL) ==
           (0x87654321UL ^ 0x0badc0deUL));
    verify(last_first == 0x0badc0deUL);

    g_engine_interface.dispatch_2c630_16c = callback_one;
    dispatch_engine_2c630(0xa1b2c3d4UL);
    verify(calls_one == 5 && last_first == 0xa1b2c3d4UL);
    g_engine_interface.dispatch_2c570_0e0 = callback_pointer;
    dispatch_engine_2c570(pointer_a);
    verify(last_pointer == pointer_a);
    g_engine_interface.dispatch_2c620_164 = callback_pointer;
    dispatch_engine_2c620(pointer_b);
    verify(last_pointer == pointer_b);

    g_engine_interface.dispatch_2caa0_330 = callback_gate_a;
    g_dispatch_callback_flag_71ec0 = 0;
    dispatch_completion_gate();
    verify(gate_calls == 0);
    g_dispatch_callback_flag_71ec0 = 1;
    dispatch_completion_gate();
    verify(gate_calls == 1);
    g_engine_interface.dispatch_2caa0_330 = callback_gate_b;
    dispatch_completion_gate();
    verify(gate_calls == 11);

    printf("dispatcher bridge fixture: %lu checks, %lu failures\n",
           checks, failures);
    return failures != 0;
}
