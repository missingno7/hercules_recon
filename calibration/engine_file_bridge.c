#include "engine_interface.h"

int __cdecl engine_file_load_async(const char *prefix, const char *suffix,
                                   int sector_offset, void *destination,
                                   unsigned long bytes)
{
    return g_engine_interface.load_async_020(
        prefix, suffix, sector_offset, destination, bytes);
}

int __cdecl engine_file_cancel(void)
{
    return g_engine_interface.cancel_load_024();
}

unsigned long __cdecl engine_pool_compact(int pool_id)
{
    return g_engine_interface.compact_pool_034(pool_id);
}

unsigned long __cdecl engine_file_length(const char *prefix, const char *suffix)
{
    return g_engine_interface.file_length_048(prefix, suffix);
}

unsigned long __cdecl engine_file_load_sync(const char *prefix, const char *suffix,
                                            void *destination,
                                            unsigned long bytes)
{
    return g_engine_interface.load_sync_068(prefix, suffix, destination, bytes);
}

void *__cdecl engine_pool_allocate(void **output_slot, unsigned long bytes,
                                   unsigned long flags)
{
    return g_engine_interface.allocate_078(output_slot, bytes, flags);
}
