#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "host_engine_cells.h"

extern void __cdecl archive_fatal(const char *message);

/* Each read below re-reads the handle from memory in the original (0x405c20, 0x405c34,
 * 0x405c80, 0x405c89), including straight after the LoadLibraryA store.  The qualifier is
 * per access (owner ruling 2026-10-31): the global itself stays non-volatile, and no other
 * function reads it. */
#define HOST_ENGINE_MODULE_RELOAD() (*(volatile HMODULE *)&g_host_engine_module_91c468)

void __cdecl host_engine_resolve(void)
{
    unsigned int index;

    g_host_engine_gate_91c460 = 0;
    if (HOST_ENGINE_MODULE_RELOAD() != 0) {
        FreeLibrary(HOST_ENGINE_MODULE_RELOAD());
        g_host_engine_module_91c468 = 0;
    }

    index = strlen(g_host_engine_path_91bbd8);
    while (g_host_engine_path_91bbd8[index] != '\\')
        --index;

    g_host_engine_module_91c468 = LoadLibraryA(g_host_engine_path_91bbd8 + index + 1);
    if (HOST_ENGINE_MODULE_RELOAD() != 0) {
        g_host_engine_entry_91c45c = GetProcAddress(
            HOST_ENGINE_MODULE_RELOAD(), "PC_DLLEngineMain");
        if (g_host_engine_entry_91c45c != 0)
            return;
    }

    sprintf(g_host_engine_error_91c040, "\"%s\"", g_host_engine_path_91bbd8);
    archive_fatal(g_host_engine_error_91c040);
}
