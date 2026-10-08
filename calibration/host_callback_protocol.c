#include "host_callback_protocol_externs.h"

void __cdecl host_invoke_callback_zero(void)
{
    EngineNoArgCallback callback;

    callback = g_host_callback_prefix.callback_zero_000;
    if (callback != 0 && g_host_callback_prefix.delivery_gate_008 != 0)
        callback();
}

EngineNoArgCallback __cdecl host_set_callback_zero(EngineNoArgCallback callback)
{
    EngineNoArgCallback previous;

    previous = g_host_callback_prefix.callback_zero_000;
    g_host_callback_prefix.callback_zero_000 = callback;
    return previous;
}

void __cdecl host_invoke_callback_one(void)
{
    EngineNoArgCallback callback;

    callback = g_host_callback_prefix.callback_one_004;
    if (callback != 0 && g_host_callback_prefix.delivery_gate_008 != 0)
        callback();
}

EngineNoArgCallback __cdecl host_set_callback_one(EngineNoArgCallback callback)
{
    EngineNoArgCallback previous;

    previous = g_host_callback_prefix.callback_one_004;
    g_host_callback_prefix.callback_one_004 = callback;
    return previous;
}

unsigned long __cdecl host_disable_callback_gate(void)
{
    g_host_callback_prefix.delivery_gate_008 = 0;
    return 0;
}

unsigned long __cdecl host_enable_clear_callbacks(void)
{
    g_host_callback_prefix.delivery_gate_008 = 1;
    g_host_callback_prefix.callback_zero_000 = 0;
    g_host_callback_prefix.callback_one_004 = 0;
    return 0;
}
