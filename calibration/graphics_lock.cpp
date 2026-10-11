#include <windows.h>
#include <ddraw.h>
#include <stddef.h>
#include <string.h>
typedef char graphics_tail_ddsurfacedesc_is_108[(sizeof(DDSURFACEDESC)==0x6c)?1:-1];
extern "C" {
extern unsigned char g_exe_ba01f0;
extern unsigned char g_fatal_suppress_dialog;
extern BYTE *g_exe_ba0210;
extern DWORD g_exe_ba0214;
extern BYTE *g_exe_96e3d8;
extern IDirectDrawSurface *g_exe_96e3e8;
extern IDirectDrawSurface *g_exe_96e3ec;
extern void __cdecl cleanup_native_6bb0(void);
extern unsigned short __cdecl cleanup_native_6ec0(int value);
void __cdecl graphics_native_06ff0(void)
{
    if (g_exe_ba01f0 != 0)
        return;
    if (g_fatal_suppress_dialog != 0) {
        if (g_exe_96e3e8 != 0 &&
            g_exe_96e3e8->IsLost() == DDERR_SURFACELOST)
            g_exe_96e3e8->Restore();
        {
            DDSURFACEDESC surface_desc;
            memset(&surface_desc, 0, sizeof surface_desc);
            IDirectDrawSurface *surface = g_exe_96e3ec;
            surface_desc.dwSize = sizeof(surface_desc);
            if (surface != 0 &&
                surface->Lock(0, &surface_desc, DDLOCK_WAIT, 0) != DD_OK) {
                cleanup_native_6ec0(0);
                cleanup_native_6bb0();
                return;
            }
            g_exe_ba0214 = surface_desc.lPitch;
            g_exe_ba0210 = (BYTE *)surface_desc.lpSurface;
            g_exe_ba01f0 = 1;
        }
    } else {
        g_exe_ba0214 = 0x140;
        g_exe_ba0210 = g_exe_96e3d8;
        g_exe_ba01f0 = 1;
    }
}
}
