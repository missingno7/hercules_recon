/* Native palette refresh, SDK physical views; original ownership/type spelling unproved. */
#include <windows.h>
#include <ddraw.h>
#include <stddef.h>

typedef char palette_entry_offset_4[(offsetof(LOGPALETTE,palPalEntry)==4)?1:-1];
typedef char palette_entry_size_4[(sizeof(PALETTEENTRY)==4)?1:-1];
extern "C" {
extern int g_exe_91c46c;
extern unsigned char g_fatal_suppress_dialog;
extern IDirectDraw *g_exe_96e3e4;
extern IDirectDrawPalette *g_exe_96e3f0;
extern LOGPALETTE *g_exe_96e3f8;
extern HPALETTE g_exe_96e3fc;
extern void __cdecl graphics_native_07140(void);
extern void __cdecl graphics_native_06ee0(int, int, int);
extern void __cdecl graphics_native_06620(void);
extern void __cdecl graphics_native_06700(int);
}

extern "C" void __cdecl graphics_native_069a0(unsigned char wait)
{
    g_exe_91c46c = 1;
    graphics_native_07140();
    graphics_native_06ee0(0, 0, 0);
    graphics_native_06620();
    if (g_fatal_suppress_dialog != 0) {
        g_exe_96e3e4->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
        if (g_exe_96e3f0 != 0)
            g_exe_96e3f0->SetEntries(0, 0, 256, g_exe_96e3f8->palPalEntry);
        if (wait != 0)
            g_exe_96e3e4->WaitForVerticalBlank(DDWAITVB_BLOCKEND, 0);
        if (wait != 0)
            g_exe_96e3e4->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
    } else {
        SetPaletteEntries(g_exe_96e3fc, 0, 256, g_exe_96e3f8->palPalEntry);
        graphics_native_06700(0);
    }
}
