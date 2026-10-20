/* HERCULES.EXE graphics unit continuations (lane h4 candidate):
 * 0x6440 numbered debug text output, 0x68c0 CIF palette/table loader,
 * 0x70c0 full-screen shade remap.  The shade byte 0x436a0c (initialized 0x80)
 * directly precedes 0x6440's literal "(%c) %s " (0x436a10) in .data, consistent with
 * one object holding both.  Names, prototypes and TU extent unproved. */
#include <windows.h>
#include <ddraw.h>
#include <stdio.h>
#include <string.h>
#include "host_graphics_table_storage.h"

extern "C" {
extern HWND g_fatal_owner_window;
extern BYTE g_exe_ba020c;
extern IDirectDrawSurface *g_exe_96e3ec;
extern int g_exe_96e3e0;
extern unsigned char g_exe_ba01f0;
extern BYTE *g_exe_ba0210;
extern DWORD g_exe_ba0214;

extern int __cdecl archive_open(const char *path, const char *ignored_mode);
extern size_t __cdecl archive_read(void *destination, size_t item_size, size_t item_count, int handle);
extern int __cdecl archive_close(int handle);
extern void __cdecl graphics_native_06780(short index, BYTE red, BYTE green, BYTE blue);
}

static BYTE s_text_sequence;
static BYTE s_shade_level = 0x80;

extern "C" void __cdecl host_06440(int x, int y, const char *text)
{
    HDC dc;
    char line[256];

    sprintf(line, "(%c) %s ", (++s_text_sequence & 7) + '0', text);
    if (g_exe_ba020c) {
        if (g_exe_96e3ec)
            g_exe_96e3ec->GetDC(&dc);
        TextOutA(dc, x, y, line, strlen(line));
        if (g_exe_96e3ec)
            g_exe_96e3ec->ReleaseDC(dc);
    } else {
        dc = GetDC(NULL);
        TextOutA(dc, x, y, line, strlen(line));
        ReleaseDC(g_fatal_owner_window, dc);
    }
}

extern "C" void __cdecl host_068c0(const char *name)
{
    int handle;
    int i;
    long color;
    int row;

    handle = archive_open(name, "rb");
    for (i = 0; i < 256; i++) {
        archive_read(&color, 3, 1, handle);
        graphics_native_06780(i, (BYTE)color, (BYTE)(color >> 8), (BYTE)(color >> 16));
    }
    archive_read(g_host_graphics_tables_52c490.average, 0x40000, 1, handle);
    archive_read(g_host_graphics_shade_51c488, 0x10000, 1, handle);
    archive_read(g_host_graphics_tables_52c490.remap, 0x1f00, 1, handle);
    archive_close(handle);
    g_exe_96e3e0 = 0;
    for (row = 1; row < 32; row++) {
        for (i = 0; i < 256; i++) {
            if (g_host_graphics_tables_52c490.remap[row][i] != g_host_graphics_tables_52c490.remap[row - 1][i]) {
                g_exe_96e3e0 = 1;
                break;
            }
        }
    }
}

extern "C" void __cdecl host_070c0(BYTE shade)
{
    DWORD offset;
    int y;
    unsigned int x;
    BYTE *pixel;

    if (shade != 0xff) {
        s_shade_level = shade;
        return;
    }
    if (s_shade_level != 0x80 && g_exe_ba0210 && g_exe_ba01f0) {
        offset = 0;
        for (y = 0; y < 240; y++) {
            for (x = 0; x < 320; x++) {
                pixel = &g_exe_ba0210[offset + x];
                *pixel = g_host_graphics_shade_51c488[s_shade_level][*pixel];
            }
            offset += g_exe_ba0214;
        }
    }
}
