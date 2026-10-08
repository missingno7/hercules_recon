#include "../calibration/host_callback_protocol_externs.h"
#include <stdio.h>

HostCallbackStoragePrefix g_host_callback_prefix;
static int callback_a_count;
static int callback_b_count;
static int mutate_callback_a;
static EngineNoArgCallback old_from_nested_setter;
static unsigned long nested_disable_result;
static int checks;
static int failures;

static void check(int condition, const char *name)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", name);
    }
}

static void __cdecl callback_b(void);

static void __cdecl callback_a(void)
{
    ++callback_a_count;
    if (mutate_callback_a) {
        mutate_callback_a = 0;
        old_from_nested_setter = host_set_callback_zero(callback_b);
        nested_disable_result = host_disable_callback_gate();
    }
}

static void __cdecl callback_b(void)
{
    ++callback_b_count;
}

int main(void)
{
    EngineNoArgCallback previous;
    int before_a;
    int before_b;

    check(g_host_callback_prefix.callback_zero_000 == 0 &&
          g_host_callback_prefix.callback_one_004 == 0 &&
          g_host_callback_prefix.delivery_gate_008 == 0,
          "loader-zero initial state skips both channels");

    previous = host_set_callback_zero(callback_a);
    check(previous == 0 && g_host_callback_prefix.callback_zero_000 == callback_a,
          "channel zero setter stores and returns its previous pointer");
    previous = host_set_callback_zero(callback_b);
    check(previous == callback_a && g_host_callback_prefix.callback_zero_000 == callback_b,
          "channel zero replacement returns the old pointer");
    previous = host_set_callback_one(callback_a);
    check(previous == 0 && g_host_callback_prefix.callback_one_004 == callback_a,
          "channel one setter is independent");

    host_invoke_callback_zero();
    host_invoke_callback_one();
    check(callback_a_count == 0 && callback_b_count == 0,
          "a nonnull callback is blocked while shared gate is zero");

    g_host_callback_prefix.delivery_gate_008 = 1;
    host_invoke_callback_zero();
    host_invoke_callback_one();
    check(callback_a_count == 1 && callback_b_count == 1,
          "both channels deliver their own current callback");

    previous = host_set_callback_zero(callback_a);
    check(previous == callback_b, "channel zero registration still returns prior callback");
    mutate_callback_a = 1;
    g_host_callback_prefix.delivery_gate_008 = 1;
    host_invoke_callback_zero();
    check(callback_a_count == 2 && callback_b_count == 1,
          "current delivery stays on the callback captured before mutation");
    check(old_from_nested_setter == callback_a &&
          g_host_callback_prefix.callback_zero_000 == callback_b &&
          g_host_callback_prefix.delivery_gate_008 == 0 &&
          nested_disable_result == 0,
          "callback can replace itself and disable only future delivery");
    host_invoke_callback_zero();
    check(callback_b_count == 1, "callback replacement is gated on the next entry");
    g_host_callback_prefix.delivery_gate_008 = 2;
    host_invoke_callback_zero();
    check(callback_b_count == 2, "any nonzero gate value enables channel zero");

    previous = host_set_callback_one(callback_a);
    check(previous == callback_a, "channel one old pointer is preserved independently");
    g_host_callback_prefix.delivery_gate_008 = 0x80000000UL;
    host_invoke_callback_one();
    check(callback_a_count == 3, "high-bit nonzero gate enables channel one");

    before_a = callback_a_count;
    before_b = callback_b_count;
    check(host_disable_callback_gate() == 0 &&
          g_host_callback_prefix.delivery_gate_008 == 0,
          "disable returns zero and clears the shared gate");
    check(g_host_callback_prefix.callback_zero_000 == callback_b &&
          g_host_callback_prefix.callback_one_004 == callback_a,
          "disable preserves both registered callbacks");
    host_invoke_callback_zero();
    host_invoke_callback_one();
    check(callback_a_count == before_a && callback_b_count == before_b,
          "disabled gate suppresses both channels");

    previous = host_set_callback_zero(0);
    check(previous == callback_b && g_host_callback_prefix.callback_zero_000 == 0,
          "null setter returns and clears channel zero pointer");
    previous = host_set_callback_one(0);
    check(previous == callback_a && g_host_callback_prefix.callback_one_004 == 0,
          "null setter returns and clears channel one pointer");
    g_host_callback_prefix.delivery_gate_008 = 1;
    host_invoke_callback_zero();
    host_invoke_callback_one();
    check(callback_a_count == before_a && callback_b_count == before_b,
          "null pointers skip even when the gate is enabled");

    host_set_callback_zero(callback_b);
    host_set_callback_one(callback_a);
    g_host_callback_prefix.delivery_gate_008 = 0;
    check(host_enable_clear_callbacks() == 0 &&
          g_host_callback_prefix.delivery_gate_008 == 1 &&
          g_host_callback_prefix.callback_zero_000 == 0 &&
          g_host_callback_prefix.callback_one_004 == 0,
          "enable sets gate and clears both callback cells");
    host_invoke_callback_zero();
    host_invoke_callback_one();
    check(callback_a_count == before_a && callback_b_count == before_b,
          "reset empty cells do not deliver after enabling");

    host_set_callback_zero(callback_b);
    host_set_callback_one(callback_a);
    g_host_callback_prefix.delivery_gate_008 = 1;
    host_invoke_callback_zero();
    host_invoke_callback_one();
    check(callback_b_count == before_b + 1 && callback_a_count == before_a + 1,
          "callbacks can be registered and delivered again after reset");

    printf("host callback protocol fixture: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
