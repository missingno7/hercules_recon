#include <windows.h>
#include <ddraw.h>
#include <stddef.h>
#include <string.h>

typedef char palette_fill_fx_size_is_100[(sizeof(DDBLTFX) == 0x64) ? 1 : -1];
typedef char palette_fill_fx_offset_is_50[(offsetof(DDBLTFX, dwFillColor) == 0x50) ? 1 : -1];

extern "C" {
extern HDC g_exe_96e400;
extern HPALETTE g_exe_96e3fc;
extern unsigned char g_fatal_suppress_dialog;
extern unsigned char g_exe_43a590;

extern unsigned char g_exe_ba01f0;
extern unsigned char g_exe_96e418;
extern unsigned char g_exe_96e41c;
extern unsigned char g_exe_96e420;
extern unsigned char g_exe_96e424;
extern IDirectDrawSurface *g_exe_96e3ec;
extern unsigned char *g_exe_96e3d8;
extern DWORD g_exe_ba0214;
extern unsigned char *g_exe_ba0210;

extern unsigned char __cdecl graphics_native_06850(int red, int green, int blue);
extern void __cdecl graphics_native_06620(void);

UINT __cdecl graphics_native_06700(int palette_mode)
{
    UINT realized;

    if (g_exe_96e400 == 0 || g_exe_96e3fc == 0 ||
        g_fatal_suppress_dialog != 0)
        return 0;

    if (palette_mode == 0x311)
        UnrealizeObject(g_exe_96e3fc);

    SelectPalette(g_exe_96e400, g_exe_96e3fc, palette_mode == 0x311);
    realized = RealizePalette(g_exe_96e400);
    g_exe_43a590 = 1;
    graphics_native_06620();
    return realized;
}
}
