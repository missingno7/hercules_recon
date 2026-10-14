#include <stdio.h>
#include <string.h>
#include "host_engine_cells.h"

extern void __cdecl host_engine_resolve(void);

void __cdecl host_engine_select(const char *prefix, const char *suffix)
{
    char filename[256];

    if (prefix != 0 && suffix != 0) {
        sprintf(g_host_engine_path_91bbd8, "%s%s", prefix, suffix);
        _splitpath(g_host_engine_path_91bbd8, 0, 0, filename, 0);

        if (_stricmp(filename, "TITLE") == 0)
            strcpy(g_host_engine_path_91bbd8,
                   "m:\\pc\\title\\release\\title.dll");
        if (_stricmp(filename, "ENGINE1") == 0)
            strcpy(g_host_engine_path_91bbd8,
                   "m:\\pc\\eng1\\release\\eng1.dll");
        if (_stricmp(filename, "ENGINE3") == 0)
            strcpy(g_host_engine_path_91bbd8,
                   "m:\\pc\\eng3\\release\\eng3.dll");
    } else {
        g_host_engine_path_91bbd8[0] = '\0';
    }

    if (g_host_engine_selector_91c464 != 0) {
        g_host_engine_gate_91c460 = 1;
        longjmp(g_host_engine_jump_buffer_91bfd8, 0);
    } else {
        host_engine_resolve();
    }
}
