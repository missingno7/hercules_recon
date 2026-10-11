/* Evidence-based physical views; original source/TU ownership remains unproved. */
#include <windows.h>
#include <ddraw.h>

extern "C" {
extern unsigned char g_exe_43a590;
extern unsigned char g_fatal_suppress_dialog;
extern HWND g_fatal_owner_window;
extern HDC g_exe_96e400;
extern unsigned char *g_exe_96e3d8;
extern IDirectDrawSurface *g_exe_96e3e8;
extern BITMAPINFO *g_exe_96e3f4;
extern int g_exe_ba0220;
extern int g_exe_ba0224;

void __cdecl graphics_native_06620(void)
{
    RECT client_rect;
    HDC dc;

    if (g_exe_43a590 == 0)
        return;
    g_exe_43a590 = 0;
    if (g_exe_96e400 != 0 && g_exe_96e3d8 != 0) {
        if (g_fatal_suppress_dialog != 0) {
            if (g_exe_96e3e8 != 0)
                g_exe_96e3e8->Flip(0, DDFLIP_WAIT);
        } else {
            StretchDIBits(g_exe_96e400, 0, 0, g_exe_ba0220, g_exe_ba0224,
                0, 0, 320, 240, g_exe_96e3d8, g_exe_96e3f4,
                DIB_RGB_COLORS, SRCCOPY);
            GdiFlush();
        }
    } else if (g_fatal_owner_window != 0) {
        dc = GetDC(g_fatal_owner_window);
        GetClientRect(g_fatal_owner_window, &client_rect);
        FillRect(dc, &client_rect, (HBRUSH)(COLOR_WINDOW + 1));
        ReleaseDC(g_fatal_owner_window, dc);
    }
}
}
