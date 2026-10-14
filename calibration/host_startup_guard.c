#include <windows.h>

/* Measured DWORD setter; original source name and parameter signedness unknown. */
extern DWORD g_startup_apply_guard_rva_51c450;

void __cdecl host_set_config_apply_guard_5750(DWORD value)
{
    g_startup_apply_guard_rva_51c450 = value;
}
