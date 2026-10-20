/* HERCULES.EXE 0x5350..0x5a50 (lane h4 candidate): CD-drive probe, full-screen movie
 * playback loop, startup service pair and level palette/CIF selection.
 * Same object as graphics_native_05a50 (src/pc/host_platform_sync.cpp): its initialized
 * cells 0x436638..0x436650 sit between this unit's flag 0x436634 and literal "A:\" 0x436654.
 * C++: 0x5350's function-local static uses the C++ dynamic-initialization guard.
 * Names, prototypes and TU extent unproved. */
#include "display_mode_contract.h"
#include <windows.h>
#include <dsound.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>

extern "C" {
extern HWND g_fatal_owner_window;
extern signed char g_exe_ba0238;
extern BYTE g_exe_ba0228;
extern DWORD g_exe_ba0230;
extern BYTE g_registry_config[72];
extern IDirectSound *g_exe_79e440;
extern int g_exe_91c44c;
extern unsigned char g_exe_ba01f0;
extern BYTE *g_exe_ba0210;
extern DWORD g_exe_ba0214;
extern BYTE g_exe_43a590;

extern void __cdecl cleanup_native_9e10(void);
extern void __cdecl cleanup_native_ff40(int value);
extern void __cdecl host_service_1ff0(void);
extern int __cdecl archive_open(const char *path, const char *ignored_mode);
extern int __cdecl archive_seek(int handle, long offset, int origin);
extern size_t __cdecl archive_read(void *destination, size_t item_size, size_t item_count, int handle);
extern int __cdecl archive_close(int handle);
extern DWORD __cdecl host_03140(const char *path, IDirectSound *sound, BYTE **screen, DWORD *width, DWORD *height);
extern DWORD __cdecl host_03680(void);
extern void __cdecl graphics_native_03770(void);
extern long __cdecl host_037a0(BYTE **palette, int volume);
extern void __cdecl graphics_native_05a50(void);
extern void __cdecl graphics_native_06620(void);
extern void __cdecl graphics_native_06780(short index, BYTE red, BYTE green, BYTE blue);
extern void __cdecl host_palette_get_rgb_6810(short index, BYTE *red, BYTE *green, BYTE *blue);
extern void __cdecl host_068c0(const char *name);
extern void __cdecl graphics_native_069a0(unsigned char wait);
extern void __cdecl cleanup_native_6bb0(void);
extern void __cdecl graphics_native_06ff0(void);
extern void __cdecl graphics_native_07140(void);
extern void __cdecl host_warm_delivery_bcf0(void);
extern int __cdecl host_0fb10(int left, int right);
extern unsigned int __cdecl host_01f60(unsigned long trigger);
}

static char s_cd_drives[26];
static int s_cd_drive_count;
static char s_cd_drive;
static BYTE s_cd_scan = 1;

extern "C" BYTE __cdecl host_05350(char *path)
{
    static DWORD logical_drives = GetLogicalDrives();
    int i;

    if (s_cd_scan) {
        s_cd_scan = 0;
        memset(s_cd_drives, 0, sizeof(s_cd_drives));
        for (i = 0; i < 26; i++) {
            char root[] = "A:\\";
            root[0] += i;
            if ((logical_drives >> i) & 1) {
                if (GetDriveTypeA(root) == DRIVE_CDROM)
                    s_cd_drives[s_cd_drive_count++] = root[0];
            }
        }
    }
    if (s_cd_drive) {
        *path = s_cd_drive;
        if (_access(path, 0) == 0)
            return 1;
    }
    for (i = 0; i < s_cd_drive_count; i++) {
        s_cd_drive = s_cd_drives[i];
        *path = s_cd_drive;
        if (_access(path, 0) == 0)
            return 1;
    }
    return 0;
}

extern "C" unsigned int __cdecl host_05450(int movie, unsigned long trigger)
{
    DWORD height;
    BYTE *screen;
    DWORD width;
    BYTE *palette;
    MSG message;
    char path[256];
    BYTE saved[768];
    unsigned int result = 0;
    long status;
    DWORD row;
    int i;

    cleanup_native_9e10();
    sprintf(path, "e:\\data\\%d.etv", movie);
    if (!host_05350(path))
        return 0;
    for (i = 0; i < 256; i++)
        host_palette_get_rgb_6810(i, &saved[i * 3], &saved[i * 3 + 1], &saved[i * 3 + 2]);
    host_03140(path, g_exe_79e440, &screen, &width, &height);
    g_exe_91c44c = 1;
    for (;;) {
        while (PeekMessageA(&message, g_fatal_owner_window, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        if (g_exe_91c44c == -1) {
            g_exe_91c44c = 0;
            PostMessageA(g_fatal_owner_window, WM_CLOSE, 0, 0);
            break;
        }
        g_exe_ba0230 = 0;
        graphics_native_05a50();
        cleanup_native_6bb0();
        if (g_exe_ba0238 && !g_exe_ba0228) {
            status = host_037a0(&palette, host_0fb10(g_registry_config[14] ? 0x32c8 : 0,
                                                    g_registry_config[14] ? 0x32c8 : 0) << 16 >> 16);
            if (status == 0)
                break;
            if (palette) {
                for (i = 0; i < 256; i++)
                    graphics_native_06780(i, palette[i * 3], palette[i * 3 + 1], palette[i * 3 + 2]);
                graphics_native_069a0(0);
            }
            if (status > 0) {
                graphics_native_06ff0();
                if (g_exe_ba01f0 && g_exe_ba0210) {
                    if (g_exe_ba0214 == width) {
                        memcpy(g_exe_ba0210, screen, height * width);
                    } else {
                        for (row = 0; row < height; row++)
                            memcpy(g_exe_ba0210 + row * g_exe_ba0214, screen + width * row, width);
                    }
                    g_exe_43a590 = 1;
                }
                graphics_native_07140();
                graphics_native_06620();
            }
            if (g_exe_ba0238 == 1)
                g_exe_ba0238 = 0;
            cleanup_native_6bb0();
            graphics_native_05a50();
            host_warm_delivery_bcf0();
            result = host_01f60(trigger);
            if (result)
                break;
        } else {
            graphics_native_03770();
            WaitMessage();
        }
    }
    g_exe_91c44c = 0;
    if (host_03680())
        cleanup_native_6ec0(0);
    for (i = 0; i < 256; i++)
        graphics_native_06780(i, saved[i * 3], saved[i * 3 + 1], saved[i * 3 + 2]);
    graphics_native_069a0(1);
    return result;
}

extern "C" void __cdecl host_05740(void)
{
    cleanup_native_ff40(1);
    host_service_1ff0();
}

extern "C" void __cdecl host_05760(int level, const char *archive_name)
{
    char path[128];
    char name[128];
    char directory[128];
    char extension[128];
    BYTE palette[768];
    int handle;
    int i;

    if (archive_name != NULL) {
        handle = archive_open(archive_name, "rb");
        archive_seek(handle, 0x12, 0);
        archive_read(palette, 0x300, 1, handle);
        archive_close(handle);
        for (i = 0; i < 128; i++)
            graphics_native_06780(i, palette[i * 3 + 2], palette[i * 3 + 1], palette[i * 3]);
        graphics_native_069a0(1);
        return;
    }
    switch (level) {
    case -1: strcpy(path, "o:\\title\\cheat\\map_psx.pal"); break;
    case -2: strcpy(path, "o:\\title\\credits\\map_psx.pal"); break;
    case -3: strcpy(path, "o:\\title\\loadlev\\map_psx.pal"); break;
    case -4: strcpy(path, "o:\\title\\mainmenu\\map_psx.pal"); break;
    case -5: strcpy(path, "o:\\title\\pswd\\map_psx.pal"); break;
    case -6: strcpy(path, "o:\\title\\tally\\map_psx.pal"); break;
    case 1: strcpy(path, "o:\\training\\map_psx.bin"); break;
    case 3: strcpy(path, "o:\\forest\\map_psx.bin"); break;
    case 5: strcpy(path, "o:\\thebes\\map_psx.bin"); break;
    case 6: strcpy(path, "o:\\hydra\\map_psx.bin"); break;
    case 7: strcpy(path, "o:\\medusa\\map_psx.bin"); break;
    case 9: strcpy(path, "o:\\titans\\map_psx.bin"); break;
    case 11: strcpy(path, "o:\\vortex\\map_psx.bin"); break;
    case 10: strcpy(path, "o:\\torment\\map_psx.bin"); break;
    case 2: strcpy(path, "o:\\gauntlet\\map_psx.bin"); break;
    case 8: strcpy(path, "o:\\cyclops\\map_psx.bin"); break;
    default: return;
    }
    _splitpath(path, NULL, directory, name, extension);
    strcat(name, extension);
    if (directory[strlen(directory) - 1] == '\\')
        directory[strlen(directory) - 1] = 0;
    strcpy(name, path);
    name[strlen(name) - 3] = 0;
    strcat(name, "cif");
    host_068c0(name);
    graphics_native_069a0(1);
}
