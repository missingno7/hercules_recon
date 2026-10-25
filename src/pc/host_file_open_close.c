/* Original PC open/close physical return views; original prototypes/TU unproved. */
#include <stdio.h>
#include "../shared/engine_interface.h"
#include "host_file_callbacks.h"
extern EngineInterface g_host_engine_interface;
extern void __cdecl host_diagnostic(const char *format, ...);
extern int __cdecl virtual_file_open(const char *path, int mode, int flags);
extern int __cdecl virtual_file_close(int handle);

/* Valid C physical return view; original formal return declaration is unproved. */
int __cdecl host_file_open_only(const char *prefix, const char *suffix)
{
    int opened;
    if (g_host_engine_interface.context_004->mode_09c == 1)
        return 0;
    host_file_prepare();
    sprintf(g_resource_filename, g_resource_filename_format, prefix, suffix);
    opened = virtual_file_open(g_resource_filename, 0, 0);
    g_host_engine_interface.context_004->virtual_handle_0b8 = opened;
    if (g_host_engine_interface.context_004->virtual_handle_0b8 == -1) {
        host_diagnostic(g_resource_sync_open_failure_format, g_resource_filename);
        return 0;
    }
    host_diagnostic(g_resource_opening_format, g_resource_filename);
    return opened;
}

int __cdecl host_frame_close_state(void)
{
    if (virtual_file_close(g_host_engine_interface.context_004->virtual_handle_0b8) != 0) {
        host_diagnostic(g_resource_close_failure_format);
        return 0;
    }
    g_host_engine_interface.context_004->unknown_0ac = 0;
    g_host_engine_interface.context_004->mode_09c = 0;
    host_diagnostic(g_resource_close_success_format);
    return 1;
}
