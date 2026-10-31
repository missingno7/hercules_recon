#include "../shared/engine_interface.h"

int title_0c2a0(const char *a, const char *b, int c, void *d, unsigned long e)
{
    return g_engine_interface.load_async_020(a, b, c, d, e);
}

void title_0c2d0(void)
{
    g_engine_interface.cancel_load_024();
}

void title_0c2f0(void)
{
    g_engine_interface.dispatch_2c270_02c();
}

void title_0c300(void)
{
    g_engine_interface.dispatch_2c280_030();
}

unsigned long title_0c310(int a)
{
    return g_engine_interface.compact_pool_034(a);
}

unsigned long title_0c350(const char *a, const char *b)
{
    return g_engine_interface.file_length_048(a, b);
}

void title_0c3b0(void)
{
    g_engine_interface.dispatch_2c350_05c();
}

void title_0c3c0(void)
{
    g_engine_interface.dispatch_2c360_060();
}

unsigned long title_0c3f0(const char *a, const char *b, void *c, unsigned long d)
{
    return g_engine_interface.load_sync_068(a, b, c, d);
}

void *title_0c450(void **a, unsigned long b, unsigned long c)
{
    return g_engine_interface.allocate_078(a, b, c);
}

void title_0c470(void)
{
    g_engine_interface.dispatch_2c420_084();
}

void __cdecl title_0c330(void *value)
{
    g_engine_interface.free_allocation_040(value);
}

void __cdecl title_0c340(unsigned long value)
{
    g_engine_interface.bulk_release_044(value);
}

void __cdecl title_0c390(void *arena, unsigned long bytes)
{
    g_engine_interface.save_arena_050(arena, bytes);
}

void __cdecl dispatch_engine_2c3b0(unsigned long value)
{
    g_engine_interface.dispatch_2c3b0_06c(value);
}

void __cdecl title_0c490(void)
{
    g_engine_interface.dispatch_2c440_08c();
}

void __cdecl title_0c670(const void *first, unsigned long second,
                           unsigned long third, unsigned long fourth)
{
    g_engine_interface.dispatch_2c5e0_140(first, second, third, fourth);
}

int __cdecl title_0c690(void *first, unsigned long second)
{
    return g_engine_interface.frame_chain_write_144(first, second);
}

void __cdecl title_0c6b0(void *value)
{
    g_engine_interface.dispatch_2c620_164(value);
}

void __cdecl title_0c6d0(unsigned long value)
{
    g_engine_interface.dispatch_2c630_16c(value);
}

EngineNoArgCallback __cdecl title_0c6e0(EngineNoArgCallback callback)
{
    return g_engine_interface.dispatch_2c640_170(callback);
}
