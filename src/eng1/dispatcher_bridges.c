/* Private dispatcher wrapper experiment. The shared EngineInterface view is
 * parent-owned and comes from calibration/engine_interface.h. */
#include "../shared/engine_interface.h"

extern unsigned long g_dispatch_callback_flag_71ec0;

void __cdecl title_0c9c0(int first, int second)
{
    g_engine_interface.dispatch_2c860_24c(first, second);
}

void __cdecl title_0c3b0(void)
{
    g_engine_interface.dispatch_2c350_05c();
}

void __cdecl dispatch_engine_2c3b0(unsigned long value)
{
    g_engine_interface.dispatch_2c3b0_06c(value);
}

unsigned long __cdecl dispatch_engine_2cc30(unsigned long value)
{
    return g_engine_interface.dispatch_2cc30_3e0(value);
}

void __cdecl title_0c990(int first, int second)
{
    g_engine_interface.dispatch_2c840_244(first, second);
}

void __cdecl title_0c5d0(unsigned long value)
{
    g_engine_interface.dispatch_2c5a0_0ec(value);
}

void __cdecl title_0c3c0(void)
{
    g_engine_interface.dispatch_2c360_060();
}

void __cdecl title_0c560(unsigned long first,
                                   unsigned long second)
{
    g_engine_interface.dispatch_2c540_0d4(first, second);
}

EngineNoArgCallback __cdecl dispatch_engine_2cc40(
    EngineNoArgCallback callback)
{
    return g_engine_interface.dispatch_2cc40_3e4(callback);
}

void __cdecl title_0c4f0(unsigned long value)
{
    g_engine_interface.dispatch_2c4e0_0b8(value);
}

void __cdecl title_0c540(unsigned long value)
{
    g_engine_interface.dispatch_2c520_0cc(value);
}

void __cdecl title_0c2f0(void)
{
    g_engine_interface.dispatch_2c270_02c();
}

unsigned long __cdecl dispatch_engine_2ca40(unsigned long value)
{
    return g_engine_interface.dispatch_2ca40_2ec(value);
}

void __cdecl title_0c470(void)
{
    g_engine_interface.dispatch_2c420_084();
}

void __cdecl title_0c490(void)
{
    g_engine_interface.dispatch_2c440_08c();
}

void __cdecl title_0c6d0(unsigned long value)
{
    g_engine_interface.dispatch_2c630_16c(value);
}

void __cdecl title_0c5b0(void *value)
{
    g_engine_interface.dispatch_2c570_0e0(value);
}

void __cdecl title_0c6b0(void *value)
{
    g_engine_interface.dispatch_2c620_164(value);
}

void __cdecl dispatch_engine_2caa0(void)
{
    g_engine_interface.dispatch_2caa0_330();
}

void __cdecl dispatch_completion_gate(void)
{
    if (g_dispatch_callback_flag_71ec0 != 0)
        dispatch_engine_2caa0();
}
