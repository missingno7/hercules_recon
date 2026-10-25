#include <stdio.h>
#include "../shared/engine_interface.h"

extern EngineInterface g_host_engine_interface;
extern long __cdecl virtual_file_seek(int handle, long offset, int origin);
extern size_t __cdecl virtual_file_read(int handle, void *destination, size_t bytes);

long __cdecl host_sector_seek(unsigned long sector)
{
    return virtual_file_seek(g_host_engine_interface.context_004->virtual_handle_0b8,
                             (long)(sector << 11), 0);
}

size_t __cdecl host_sector_read(void *destination, unsigned long sectors)
{
    return virtual_file_read(g_host_engine_interface.context_004->virtual_handle_0b8,
                             destination, sectors << 11);
}
