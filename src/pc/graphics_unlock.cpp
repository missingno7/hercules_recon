#include <windows.h>
#include <ddraw.h>
extern "C" {
extern unsigned char g_exe_ba01f0;
extern unsigned char g_fatal_suppress_dialog;
extern IDirectDrawSurface *g_exe_96e3ec;
void __cdecl graphics_native_07140(void)
{
    if (g_exe_ba01f0 == 0)
        return;
    if (g_fatal_suppress_dialog != 0 && g_exe_96e3ec != 0)
        g_exe_96e3ec->Unlock(0);
    g_exe_ba01f0 = 0;
}
}
