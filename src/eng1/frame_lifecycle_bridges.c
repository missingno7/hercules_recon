#include "../shared/frame_lifecycle_bridges.h"

/* Actual copied-table forwarding; original formal prototypes/TU unproved. */
void __cdecl title_0c300(void)
{
    g_engine_interface.dispatch_2c280_030();
}

void __cdecl title_0c330(void *value)
{
    g_engine_interface.free_allocation_040(value);
}

void __cdecl title_0c340(unsigned long value)
{
    g_engine_interface.bulk_release_044(value);
}

void __cdecl title_0c670(const void *first, unsigned long second,
                                   unsigned long third, unsigned long fourth)
{
    g_engine_interface.dispatch_2c5e0_140(first, second, third, fourth);
}

void __cdecl title_0c520(void *first, void *second)
{
    g_engine_interface.dispatch_2c500_0c8(first, second);
}

void __cdecl dispatch_engine_2c980(void *first, unsigned long second,
                                   unsigned long third, unsigned long fourth,
                                   unsigned long fifth)
{
    g_engine_interface.dispatch_2c980_2ac(first, second, third, fourth, fifth);
}

void __cdecl title_0ca20(void *value)
{
    g_engine_interface.dispatch_2c8f0_280(value);
}

void __cdecl title_0ca10(void *value)
{
    g_engine_interface.dispatch_2c8e0_27c(value);
}

void __cdecl title_0c880(void)
{
    g_engine_interface.dispatch_2c7e0_1f8();
}

EngineNoArgCallback __cdecl title_0c6e0(
    EngineNoArgCallback callback)
{
    return g_engine_interface.dispatch_2c640_170(callback);
}
