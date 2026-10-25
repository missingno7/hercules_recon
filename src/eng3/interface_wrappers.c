#include "../shared/engine_interface.h"

int eng3_slot_020(const char *a, const char *b, int c, void *d, unsigned long e)
{
    return g_engine_interface.load_async_020(a, b, c, d, e);
}

void eng3_slot_024(void)
{
    g_engine_interface.cancel_load_024();
}

void eng3_slot_02c(void)
{
    g_engine_interface.dispatch_2c270_02c();
}

void eng3_slot_030(void)
{
    g_engine_interface.dispatch_2c280_030();
}

unsigned long eng3_slot_034(int a)
{
    return g_engine_interface.compact_pool_034(a);
}

unsigned long eng3_slot_048(const char *a, const char *b)
{
    return g_engine_interface.file_length_048(a, b);
}

void eng3_slot_05c(void)
{
    g_engine_interface.dispatch_2c350_05c();
}

void eng3_slot_060(void)
{
    g_engine_interface.dispatch_2c360_060();
}

unsigned long eng3_slot_068(const char *a, const char *b, void *c, unsigned long d)
{
    return g_engine_interface.load_sync_068(a, b, c, d);
}

void *eng3_slot_078(void **a, unsigned long b, unsigned long c)
{
    return g_engine_interface.allocate_078(a, b, c);
}

void eng3_slot_084(void)
{
    g_engine_interface.dispatch_2c420_084();
}

void __cdecl eng3_slot_040(void *value)
{
    g_engine_interface.free_allocation_040(value);
}

void __cdecl eng3_slot_044(unsigned long value)
{
    g_engine_interface.bulk_release_044(value);
}

void __cdecl eng3_slot_050(void *arena, unsigned long bytes)
{
    g_engine_interface.save_arena_050(arena, bytes);
}

void __cdecl eng3_slot_06c(unsigned long value)
{
    g_engine_interface.dispatch_2c3b0_06c(value);
}

void __cdecl eng3_slot_08c(void)
{
    g_engine_interface.dispatch_2c440_08c();
}

void __cdecl eng3_slot_140(const void *first, unsigned long second,
                           unsigned long third, unsigned long fourth)
{
    g_engine_interface.dispatch_2c5e0_140(first, second, third, fourth);
}

int __cdecl eng3_slot_144(void *first, unsigned long second)
{
    return g_engine_interface.frame_chain_write_144(first, second);
}

void __cdecl eng3_slot_164(void *value)
{
    g_engine_interface.dispatch_2c620_164(value);
}

void __cdecl eng3_slot_16c(unsigned long value)
{
    g_engine_interface.dispatch_2c630_16c(value);
}

EngineNoArgCallback __cdecl eng3_slot_170(EngineNoArgCallback callback)
{
    return g_engine_interface.dispatch_2c640_170(callback);
}
