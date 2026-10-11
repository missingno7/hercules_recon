/* Private genuine SDK declaration transfer; native source/TU identity unproved. */
#include <windows.h>
#include <ddraw.h>
#include <stddef.h>

typedef char sdk_dword_4[(sizeof(DWORD)==4)?1:-1];
typedef char sdk_long_4[(sizeof(LONG)==4)?1:-1];
typedef char sdk_hwnd_4[(sizeof(HWND)==4)?1:-1];
typedef char sdk_menu_4[(sizeof(HMENU)==4)?1:-1];
typedef char sdk_rect_16[(sizeof(RECT)==16)?1:-1];
typedef char sdk_placement_44[(sizeof(WINDOWPLACEMENT)==44)?1:-1];
typedef char sdk_showcmd_offset_8[(offsetof(WINDOWPLACEMENT,showCmd)==8)?1:-1];

extern "C" {
extern int g_exe_96e40c;
extern unsigned char g_exe_96e3d0;
extern unsigned char g_fatal_suppress_dialog;
extern HWND g_fatal_owner_window;
extern int g_exe_91c44c;
extern WINDOWPLACEMENT g_exe_92e390;
extern HMENU g_exe_96e414;
extern LONG g_exe_92c48c;
extern LONG g_exe_91c480;
extern unsigned char g_exe_96e410;
extern IDirectDraw *g_exe_96e3e4;
extern DWORD g_exe_ba0214;
extern unsigned char *g_exe_ba0210;
extern unsigned char *g_exe_96e3d8;
extern int g_exe_96e3d4;
extern unsigned char g_exe_43a590;
extern RECT g_exe_92e3c0;

extern void __cdecl graphics_native_03770(void);
extern void __cdecl graphics_native_05a50(void);
extern void __cdecl graphics_native_06530(unsigned char *, int,
                                          int, int,
                                          unsigned char *, int,
                                          int, int);
extern void __cdecl graphics_native_06620(void);
extern void __cdecl graphics_native_069a0(unsigned char value);
extern void __cdecl graphics_native_06a60(DWORD width,
                                          DWORD height);
extern void __cdecl graphics_native_06ff0(void);
extern void __cdecl graphics_native_07140(void);

}

extern "C" void __cdecl cleanup_native_6bb0(void)
{
    int index;
    DWORD previous_width;
    IDirectDraw *device;

    if (g_exe_96e40c != 0)
        return;
    if (g_exe_96e3d0 == g_fatal_suppress_dialog)
        return;

    g_exe_96e40c = 1;
    if (g_exe_91c44c != 0)
        graphics_native_03770();

    if (g_exe_96e3d0 != 0) {
        graphics_native_07140();
        g_exe_92e390.length = 0x2c;
        GetWindowPlacement(g_fatal_owner_window, &g_exe_92e390);
        if (g_exe_92e390.showCmd == 3)
            ShowWindow(g_fatal_owner_window, 9);
        GetWindowRect(g_fatal_owner_window, &g_exe_92e3c0);
        if (g_exe_96e414 == 0) {
            g_exe_96e414 = GetMenu(g_fatal_owner_window);
            SetMenu(g_fatal_owner_window, 0);
        }

        g_exe_92c48c = SetWindowLongA(g_fatal_owner_window, -20, 8);
        g_exe_91c480 = SetWindowLongA(g_fatal_owner_window, -16, (LONG)0x80000000);
        SetWindowPos(g_fatal_owner_window, 0, 0, 0, GetSystemMetrics(0),
                     GetSystemMetrics(1), 0x44);

        if (g_exe_96e410 == 0) {
            g_exe_96e410 = 1;
            ShowCursor(0);
        }

        g_fatal_suppress_dialog = g_exe_96e3d0;
        g_exe_96e3e4->SetCooperativeLevel(g_fatal_owner_window, 0x55);
        previous_width = g_exe_ba0214;
        graphics_native_06a60(0x140, 0xf0);
        graphics_native_05a50();
        graphics_native_06ff0();
        graphics_native_06530(g_exe_96e3d8, 0x140, 0xf0,
                              previous_width, g_exe_ba0210,
                              0x140, 0xf0, g_exe_ba0214);
        graphics_native_07140();
    } else {
        for (index = 0; index < g_exe_96e3d4; ++index) {
            graphics_native_06ff0();
            graphics_native_07140();
            g_exe_43a590 = 1;
            graphics_native_06620();
        }

        graphics_native_06ff0();
        previous_width = g_exe_ba0214;
        g_exe_ba0214 = 0x140;
        graphics_native_06530(g_exe_ba0210, 0x140, 0xf0,
                              previous_width, g_exe_96e3d8,
                              0x140, 0xf0, 0x140);
        g_exe_ba0210 = g_exe_96e3d8;
        graphics_native_07140();

        device = g_exe_96e3e4;
        g_fatal_suppress_dialog = g_exe_96e3d0;
        device->RestoreDisplayMode();
        g_exe_96e3e4->SetCooperativeLevel(g_fatal_owner_window, 0x0c);

        if (g_exe_96e414 != 0) {
            SetMenu(g_fatal_owner_window, g_exe_96e414);
            g_exe_96e414 = 0;
        }

        SetWindowLongA(g_fatal_owner_window, -20, g_exe_92c48c);
        SetWindowLongA(g_fatal_owner_window, -16, g_exe_91c480);
        SetWindowPos(g_fatal_owner_window, 0,
                     g_exe_92e3c0.left, g_exe_92e3c0.top,
                     g_exe_92e3c0.right - g_exe_92e3c0.left,
                     g_exe_92e3c0.bottom - g_exe_92e3c0.top, 0x14);
        graphics_native_05a50();

        if (g_exe_96e410 != 0) {
            g_exe_96e410 = 0;
            ShowCursor(1);
        }
        InvalidateRect(0, 0, 1);
    }

    g_exe_43a590 = 1;
    graphics_native_06620();
    graphics_native_069a0(1);
    g_exe_96e40c = 0;
}
