#include <windows.h>
#include <ddraw.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef char startup_log_palette_size[(sizeof(LOGPALETTE) == 8) ? 1 : -1];
typedef char startup_bitmap_info_size[(sizeof(BITMAPINFO) == 44) ? 1 : -1];
typedef char startup_mode_desc_size[(sizeof(DDSURFACEDESC) == 108) ? 1 : -1];
typedef char startup_mode_bits_offset[(offsetof(DDSURFACEDESC, ddpfPixelFormat) +
    offsetof(DDPIXELFORMAT, dwRGBBitCount) == 0x54) ? 1 : -1];

extern "C" {
extern int g_host_graphics_reciprocal_68e918[2048];
extern HWND g_fatal_owner_window;
extern HDC g_exe_96e400;
extern LOGPALETTE *g_exe_96e3f8;
extern BITMAPINFO *g_exe_96e3f4;
extern BYTE *g_exe_96e3d8;
extern HBITMAP g_exe_96e404;
extern HPALETTE g_exe_96e3fc;
extern HPALETTE g_exe_96e3dc;
extern IDirectDraw *g_exe_96e3e4;
extern BYTE g_exe_92c488;
extern void __cdecl graphics_native_06ff0(void);
extern void __cdecl graphics_native_071a0(void);
extern void __cdecl archive_fatal(const char *);

HRESULT WINAPI host_graphics_mode_callback_7170(LPDDSURFACEDESC mode, LPVOID ignored)
{
    if (mode->ddpfPixelFormat.dwRGBBitCount == 8 && mode->dwWidth == 320 &&
        mode->dwHeight == 240) {
        g_exe_92c488 = 1;
        return DDENUMRET_CANCEL;
    }
    return DDENUMRET_OK;
}

void __cdecl host_service_72a0(void)
{
    int index;
    HRESULT status;
    g_host_graphics_reciprocal_68e918[0] = 0;
    for (index = 1; index < 2048; ++index)
        g_host_graphics_reciprocal_68e918[index] = 0x40000000 / index;
    g_exe_96e400 = GetDC(g_fatal_owner_window);
    if (g_exe_96e400 == 0)
        archive_fatal("PC_Mainhdc");
    g_exe_96e3f8 = (LOGPALETTE *)malloc(sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));
    if (g_exe_96e3f8 == 0)
        archive_fatal("PC_LogPal");
    memset(g_exe_96e3f8, 0, sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));
    g_exe_96e3f8->palNumEntries = 256;
    g_exe_96e3f8->palVersion = 0x300;
    g_exe_96e3f4 = (BITMAPINFO *)malloc(sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD));
    if (g_exe_96e3f4 == 0)
        archive_fatal("PC_BitmapInfo");
    memset(g_exe_96e3f4, 0, sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD));
    g_exe_96e3f4->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_exe_96e3f4->bmiHeader.biPlanes = 1;
    g_exe_96e3f4->bmiHeader.biBitCount = 8;
    g_exe_96e3f4->bmiHeader.biCompression = BI_RGB;
    g_exe_96e3f4->bmiHeader.biWidth = 320;
    g_exe_96e3f4->bmiHeader.biHeight = -240;
    g_exe_96e3d8 = 0;
    g_exe_96e404 = CreateDIBSection(g_exe_96e400, g_exe_96e3f4, DIB_RGB_COLORS,
                                   (void **)&g_exe_96e3d8, 0, 0);
    g_exe_96e3fc = CreatePalette(g_exe_96e3f8);
    if (g_exe_96e3fc == 0)
        archive_fatal("CreatePalette()");
    g_exe_96e3dc = SelectPalette(g_exe_96e400, g_exe_96e3fc, FALSE);
    graphics_native_06ff0();
    graphics_native_071a0();
    status = DirectDrawCreate(0, &g_exe_96e3e4, 0);
    if (status != DD_OK)
        archive_fatal("DirectDrawCreate()");
    status = g_exe_96e3e4->SetCooperativeLevel(g_fatal_owner_window, 0x55);
    if (status != DD_OK)
        archive_fatal("SetCooperativeLevel()");
    g_exe_92c488 = 0;
    status = g_exe_96e3e4->EnumDisplayModes(0, 0, 0, host_graphics_mode_callback_7170);
    if (status != DD_OK)
        archive_fatal("EnumDisplayModes()");
    status = g_exe_96e3e4->SetCooperativeLevel(g_fatal_owner_window, 0x0c);
    if (status != DD_OK)
        archive_fatal("SetCooperativeLevel()");
}
}
