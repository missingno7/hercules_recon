/* Ordinary C++ reconstruction with the observed post-release descriptor lifetime. */
#include <windows.h>
#include <ddraw.h>
#include <stddef.h>
#include <string.h>

typedef char ddraw_desc_is_108[(sizeof(DDSURFACEDESC) == 0x6c) ? 1 : -1];
typedef char ddraw_caps_is_4[(sizeof(DDSCAPS) == 4) ? 1 : -1];
typedef char palette_entry_is_4[(sizeof(PALETTEENTRY) == 4) ? 1 : -1];

extern "C" {
extern DWORD g_exe_96e3d4;
extern IDirectDraw *g_exe_96e3e4;
extern IDirectDrawSurface *g_exe_96e3e8;
extern IDirectDrawSurface *g_exe_96e3ec;
extern IDirectDrawPalette *g_exe_96e3f0;

void __cdecl graphics_native_06a60(DWORD width, DWORD height)
{
    DDSCAPS surface_caps;
    PALETTEENTRY palette_entries[256];
    DWORD index;

    g_exe_96e3e4->SetDisplayMode(width, height, 8);

    if (g_exe_96e3ec != 0) {
        g_exe_96e3ec->Release();
        g_exe_96e3ec = 0;
    }
    if (g_exe_96e3e8 != 0) {
        g_exe_96e3e8->Release();
        g_exe_96e3e8 = 0;
    }
    if (g_exe_96e3f0 != 0) {
        g_exe_96e3f0->Release();
        g_exe_96e3f0 = 0;
    }

    {
        DDSURFACEDESC surface_desc;
        memset(&surface_desc, 0, sizeof surface_desc);
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        surface_desc.dwBackBufferCount = 2;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY |
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
        g_exe_96e3d4 = 2;
        if (g_exe_96e3e4->CreateSurface(&surface_desc, &g_exe_96e3e8, 0) != 0) {
            surface_desc.dwBackBufferCount = 1;
            g_exe_96e3d4 = 1;
            g_exe_96e3e4->CreateSurface(&surface_desc, &g_exe_96e3e8, 0);
        }
    }

    surface_caps.dwCaps = DDSCAPS_BACKBUFFER;
    if (g_exe_96e3e8 != 0) {
        g_exe_96e3e8->GetAttachedSurface(&surface_caps, &g_exe_96e3ec);
    }

    for (index = 0; index < 256; ++index) {
        palette_entries[index].peBlue = 0;
        palette_entries[index].peGreen = 0;
        palette_entries[index].peRed = 0;
        palette_entries[index].peFlags = 4;
    }
    g_exe_96e3e4->CreatePalette(DDPCAPS_8BIT, palette_entries,
                                &g_exe_96e3f0, 0);
    if (g_exe_96e3e8 != 0)
        g_exe_96e3e8->SetPalette(g_exe_96e3f0);
}
}