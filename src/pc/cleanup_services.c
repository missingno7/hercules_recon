#include "display_mode_contract.h"
#include <stdlib.h>
#include <windows.h>
#include <ddraw.h>
#include <mmsystem.h>
#include <dsound.h>

#define RELEASE_COM(slot) \
    do { \
        if ((slot) != 0) { \
            (slot)->lpVtbl->Release(slot); \
            (slot) = 0; \
        } \
    } while (0)

extern void __cdecl cleanup_native_6bb0(void);
extern void __cdecl cleanup_native_9e10(void);
extern void __cdecl cleanup_native_ff40(int value);
extern HWND g_fatal_owner_window;

extern IDirectDrawSurface *g_exe_96e3ec;
extern IDirectDrawSurface *g_exe_96e3e8;
extern IDirectDrawPalette *g_exe_96e3f0;
extern IDirectDraw *g_exe_96e3e4;
extern HPALETTE g_exe_96e3dc;
extern HPALETTE g_exe_96e3fc;
extern HDC g_exe_96e400;
extern LOGPALETTE *g_exe_96e3f8;
extern BITMAPINFO *g_exe_96e3f4;
extern HBITMAP g_exe_96e404;
extern int g_exe_96e630;
extern IDirectSoundBuffer *g_exe_79e444;
extern IDirectSound *g_exe_79e440;

void __cdecl cleanup_native_74c0(void)
{
    cleanup_native_6ec0(0);
    cleanup_native_6bb0();

    RELEASE_COM(g_exe_96e3ec);
    RELEASE_COM(g_exe_96e3e8);
    RELEASE_COM(g_exe_96e3f0);
    RELEASE_COM(g_exe_96e3e4);

    if (g_exe_96e3fc != 0 && g_exe_96e400 != 0) {
        SelectPalette(g_exe_96e400, g_exe_96e3dc, 0);
        DeleteObject(g_exe_96e3fc);
        g_exe_96e3fc = 0;
    }

    if (g_exe_96e3f8 != 0) {
        free(g_exe_96e3f8);
        g_exe_96e3f8 = 0;
    }
    if (g_exe_96e3f4 != 0) {
        free(g_exe_96e3f4);
        g_exe_96e3f4 = 0;
    }
    if (g_exe_96e400 != 0) {
        ReleaseDC(g_fatal_owner_window, g_exe_96e400);
        g_exe_96e400 = 0;
    }
    if (g_exe_96e404 != 0) {
        DeleteObject(g_exe_96e404);
        g_exe_96e404 = 0;
    }

    InvalidateRect(0, 0, 1);
}

void __cdecl cleanup_native_a120(void)
{
    cleanup_native_9e10();
    if (g_exe_96e630 != 0)
        mciSendStringA("close cdaudio wait", 0, 0, 0);
}

void __cdecl cleanup_native_ff70(void)
{
    cleanup_native_ff40(1);
    RELEASE_COM(g_exe_79e444);
    RELEASE_COM(g_exe_79e440);
}
