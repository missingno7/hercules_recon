#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "host_engine_cells.h"

extern void __cdecl archive_fatal(const char *message);

void __cdecl host_engine_resolve(void)
{
    unsigned int index;

    g_host_engine_gate_91c460 = 0;
    if (g_host_engine_module_91c468 != 0) {
        FreeLibrary(g_host_engine_module_91c468);
        g_host_engine_module_91c468 = 0;
    }

    index = strlen(g_host_engine_path_91bbd8);
    while (g_host_engine_path_91bbd8[index] != '\\')
        --index;

    g_host_engine_module_91c468 = LoadLibraryA(g_host_engine_path_91bbd8 + index + 1);
    if (g_host_engine_module_91c468 != 0) {
        g_host_engine_entry_91c45c = GetProcAddress(
            g_host_engine_module_91c468, "PC_DLLEngineMain");
        if (g_host_engine_entry_91c45c != 0)
            return;
    }

    sprintf(g_host_engine_error_91c040, "\"%s\"", g_host_engine_path_91bbd8);
    archive_fatal(g_host_engine_error_91c040);
}
