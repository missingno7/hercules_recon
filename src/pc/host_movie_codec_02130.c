/* HERCULES.EXE movie block codec 0x2130/0x21e0 (lane h1 candidate); follows the
 * movie_native_020c0 initializer.  Names and TU unproved. */
#include <windows.h>

extern int g_exe_4a3810;
extern int g_exe_4a3814;
extern BYTE g_exe_48f810[81920];
extern int g_exe_47f750[47];
extern BYTE g_exe_47f810[256][256];
/* File-local noise phase: the original reloads it with the static-object dword idiom. */
static BYTE g_exe_4a3818;
extern BYTE g_exe_43f754[];
extern unsigned long __cdecl host_02530(BYTE *, BYTE *);

void __cdecl host_02130(const BYTE *mask)
{
    int a, b;
    for (a = 0; a < 256; a++) {
        for (b = 0; b < 256; b += 8) {
            g_exe_47f810[a][b] = (*mask & 0x01) ? b : a;
            g_exe_47f810[a][b + 1] = (*mask & 0x02) ? b + 1 : a;
            g_exe_47f810[a][b + 2] = (*mask & 0x04) ? b + 2 : a;
            g_exe_47f810[a][b + 3] = (*mask & 0x08) ? b + 3 : a;
            g_exe_47f810[a][b + 4] = (*mask & 0x10) ? b + 4 : a;
            g_exe_47f810[a][b + 5] = (*mask & 0x20) ? b + 5 : a;
            g_exe_47f810[a][b + 6] = (*mask & 0x40) ? b + 6 : a;
            g_exe_47f810[a][b + 7] = (*mask & 0x80) ? b + 7 : a;
            mask++;
        }
    }
}

void __cdecl host_021e0(BYTE *screen, BYTE *packed)
{
    int x, y, i, j, offset, row;
    BYTE *data;
    DWORD value;

    host_02530(packed + 4, g_exe_43f754);
    data = g_exe_43f754 + g_exe_4a3814 * g_exe_4a3810 / 64;
    row = 0;
    for (y = 0; y < g_exe_4a3814; y += 4) {
        for (x = 0; x < g_exe_4a3810; x += 4) {
            switch ((g_exe_43f754[(g_exe_4a3810 >> 4) * (y >> 2) + (x >> 4)] >> ((x >> 1) & 6)) & 3) {
            case 0:
                offset = row + x;
                for (i = 0; i < 4; i++) {
                    value = *(DWORD *)data;
                    data += 4;
                    *(DWORD *)(screen + offset) = value;
                    *(DWORD *)(g_exe_48f810 + offset) = value;
                    offset += g_exe_4a3810;
                }
                break;
            case 2:
                value = *data++;
                value |= value << 8;
                value |= value << 16;
                offset = row + x;
                for (i = 0; i < 4; i++) {
                    *(DWORD *)(screen + offset) = value;
                    *(DWORD *)(g_exe_48f810 + offset) = value;
                    offset += g_exe_4a3810;
                }
                break;
            }
        }
        row += g_exe_4a3810 * 4;
    }
    row = g_exe_4a3810 * 4;
    for (y = 4; y < g_exe_4a3814 - 4; y += 4) {
        for (x = 4; x < g_exe_4a3810 - 4; x += 4) {
            if (((g_exe_43f754[(g_exe_4a3810 >> 4) * (y >> 2) + (x >> 4)] >> ((x >> 1) & 6)) & 3) == 2) {
                offset = row + x;
                for (j = 0; j < 4; j++) {
                    for (i = 0; i < 4; i++) {
                        if (++g_exe_4a3818 >= 47)
                            g_exe_4a3818 = 0;
                        screen[offset + i] = g_exe_47f810[screen[offset + i]]
                            [g_exe_48f810[g_exe_47f750[g_exe_4a3818] + offset + i]];
                    }
                    offset += g_exe_4a3810;
                }
            }
        }
        row += g_exe_4a3810 * 4;
    }
}
