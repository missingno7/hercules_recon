#include <stdio.h>
#include "../calibration/frame_lifecycle_bridges.h"

EngineInterface g_engine_interface;

static unsigned long checks;
static unsigned long failures;
static unsigned long slot12_observed;
static unsigned long slot17_calls;
static unsigned long slot17_value;
static unsigned long slot80_calls;
static const void *slot80_first;
static unsigned long slot80_values[3];
static unsigned long slot50_a_calls;
static unsigned long slot50_b_calls;
static void *slot50_first;
static void *slot50_second;
static void *slot171_first;
static unsigned long slot171_values[4];
static void *slot159_value;
static void *slot160_value;
static unsigned long slot126_calls;
static void *slot16_value;
static EngineNoArgCallback registered_callback;

static void verify(int condition)
{
    ++checks;
    if (!condition)
        ++failures;
}

static void __cdecl slot12_a(void)
{
    slot12_observed = 1;
}

static void __cdecl slot12_b(void)
{
    slot12_observed = 2;
}

static void __cdecl slot16_callback(void *value)
{
    slot16_value = value;
}

static void __cdecl slot17_callback(unsigned long value)
{
    ++slot17_calls;
    slot17_value = value;
}

static void __cdecl slot50_a(void *first, void *second)
{
    ++slot50_a_calls;
    slot50_first = first;
    slot50_second = second;
}

static void __cdecl slot50_b(void *first, void *second)
{
    ++slot50_b_calls;
    slot50_first = first;
    slot50_second = second;
}

static void __cdecl slot80_callback(const void *first, unsigned long second,
                                    unsigned long third, unsigned long fourth)
{
    ++slot80_calls;
    slot80_first = first;
    slot80_values[0] = second;
    slot80_values[1] = third;
    slot80_values[2] = fourth;
}

static void __cdecl slot171_callback(void *first, unsigned long second,
                                     unsigned long third, unsigned long fourth,
                                     unsigned long fifth)
{
    slot171_first = first;
    slot171_values[0] = second;
    slot171_values[1] = third;
    slot171_values[2] = fourth;
    slot171_values[3] = fifth;
}

static void __cdecl slot159_callback(void *value)
{
    slot159_value = value;
}

static void __cdecl slot160_callback(void *value)
{
    slot160_value = value;
}

static void __cdecl slot126_callback(void)
{
    ++slot126_calls;
}

static EngineNoArgCallback __cdecl slot92_setter(EngineNoArgCallback callback)
{
    EngineNoArgCallback previous = registered_callback;
    registered_callback = callback;
    return previous;
}

int main(void)
{
    unsigned long marker = 0;
    void *first = &marker;
    void *second = (void *)((unsigned char *)&marker + 1);
    EngineNoArgCallback previous;

    g_engine_interface.dispatch_2c280_030 = slot12_a;
    dispatch_engine_2c280();
    verify(slot12_observed == 1);
    g_engine_interface.dispatch_2c280_030 = slot12_b;
    dispatch_engine_2c280();
    verify(slot12_observed == 2);

    g_engine_interface.free_allocation_040 = slot16_callback;
    dispatch_engine_2c2d0(first);
    verify(slot16_value == first);

    g_engine_interface.bulk_release_044 = slot17_callback;
    dispatch_engine_2c2e0(0x1234abcdUL);
    verify(slot17_calls == 1 && slot17_value == 0x1234abcdUL);

    g_engine_interface.dispatch_2c5e0_140 = slot80_callback;
    dispatch_engine_2c5e0(first, 0, 1, 0x9cUL);
    verify(slot80_calls == 1 && slot80_first == first &&
           slot80_values[0] == 0 && slot80_values[1] == 1 &&
           slot80_values[2] == 0x9cUL);

    g_engine_interface.dispatch_2c500_0c8 = slot50_a;
    dispatch_engine_2c500(first, second);
    verify(slot50_a_calls == 1 && slot50_first == first &&
           slot50_second == second);
    g_engine_interface.dispatch_2c500_0c8 = slot50_b;
    dispatch_engine_2c500(second, first);
    verify(slot50_b_calls == 1 && slot50_first == second &&
           slot50_second == first);

    g_engine_interface.dispatch_2c980_2ac = slot171_callback;
    dispatch_engine_2c980(first, 0, 1, 0x9cUL, 0);
    verify(slot171_first == first && slot171_values[0] == 0 &&
           slot171_values[1] == 1 && slot171_values[2] == 0x9cUL &&
           slot171_values[3] == 0);

    g_engine_interface.dispatch_2c8f0_280 = slot160_callback;
    dispatch_engine_2c8f0(first);
    verify(slot160_value == first);
    g_engine_interface.dispatch_2c8e0_27c = slot159_callback;
    dispatch_engine_2c8e0(second);
    verify(slot159_value == second);

    g_engine_interface.dispatch_2c7e0_1f8 = slot126_callback;
    dispatch_engine_2c7e0();
    verify(slot126_calls == 1);

    g_engine_interface.dispatch_2c640_170 = slot92_setter;
    registered_callback = slot12_a;
    previous = dispatch_engine_2c640(slot12_b);
    verify(previous == slot12_a && registered_callback == slot12_b);
    previous = dispatch_engine_2c640(slot12_a);
    verify(previous == slot12_b && registered_callback == slot12_a);

    printf("frame lifecycle bridge fixture: %lu checks, %lu failures\n",
           checks, failures);
    return failures != 0;
}
