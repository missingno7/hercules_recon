#include <string.h>
#include <io.h>
#include "title_engine.h"

/* TITLE.DLL lane w09 region: functions in ascending RVA order. */

extern int title_06480(void);
extern void title_064f0(void);
extern void title_065a0(void);
extern void title_066f0(void *buf, int n);
extern void title_0c600(int x);
extern void title_0c610(int x);
extern void title_0c620(int x);
extern void title_01dd0(int a);
extern void title_02090(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
extern void title_0c2a0(char *s, int a, int b, int c, int d);
extern void title_0c8b0(int a1, int a2, int a3, int a4, int a5, int a6, int a7);

extern int g_2c7c0[];
extern char g_2c9c0[];

extern int g_2a044;
extern int g_2a03c;
extern int *g_2a048;
extern int *g_2a050;
extern int *g_2a058;
extern int *g_2a054;
extern int *g_2a04c;
extern int *g_2a060;
extern int *g_2a064;
extern int *g_2a070;
extern int *g_2a05c;
extern int *g_2a06c;
extern int *g_2a000;
extern int *g_2a004;
extern int *g_29ffc;
extern int *g_29ff4;

extern int g_2a018;
extern int g_2a080;
extern int g_2a088;
extern int g_2a078;
extern int g_2239c;
extern int g_22394;
extern int g_2236c;
extern int g_22364;
extern int g_22390;
extern int g_22370;
extern int g_22388;
extern char g_29128[];

int title_06840(char a1, const void *a2, int a3, int a4, int a5)
{
    char path[0x100];
    char rec[0x200];
    int r;
    int fd;
    int i;

    title_0c610(0);
    r = title_06480();
    if (r == 1 || r == 2) {
        return 0;
    }
    if (r == 3) {
        title_065a0();
        title_0c600(0);
        return 2;
    }
    title_064f0();
    title_0c620(0);
    title_06480();

    strcpy(path, "bu00:B-sces-00891");
    path[6] = 'A';
    path[0x11] = ' ';
    path[0x12] = ' ';
    path[0x14] = ' ';
    path[0x16] = 'A';
    path[0x15] = 'S';
    for (i = 0; i < 0x100; i++) {
        g_2c7c0[i] = -1;
    }
    path[0x13] = a1;
    path[0x17] = 'V';
    path[0x18] = 'E';
    path[0x19] = 0;
    title_066f0(g_2c7c0, a5);
    memcpy(g_2c9c0, a2, a3);

    if (a4 == 1) {
        fd = _open(path, 0x10100);
        if (fd == -1) {
            return 1;
        }
        _close(fd);
    }

    fd = _open(path, 1);
    if (fd == -1) {
        return 4;
    }
    if (_write(fd, g_2c7c0, 0x400) != 0x400) {
        return 4;
    }
    _close(fd);

    fd = _open(path, 0);
    if (fd == -1) {
        return 5;
    }
    if (_read(fd, rec, 0x200) != 0x200) {
        return 5;
    }
    _close(fd);
    return 3;
}

void title_06fb0(void)
{
    *g_2a048 = (g_2a044 - 0x7d) << 16;
    g_2a048[1] = (g_2a03c + 0x55) << 16;
    *g_2a050 = (g_2a044 - 0x50) << 16;
    g_2a050[1] = (g_2a03c + 0x69) << 16;
    *g_2a058 = (g_2a044 + 0x50) << 16;
    g_2a058[1] = (g_2a03c + 0x69) << 16;
    *g_2a054 = (g_2a044 + 0x6e) << 16;
    g_2a054[1] = (g_2a03c + 0x5f) << 16;
    *g_2a04c = (g_2a044 + 0x87) << 16;
    g_2a04c[1] = (g_2a03c + 0x4b) << 16;
    *g_2a060 = (g_2a044 - 0x7d) << 16;
    g_2a060[1] = (g_2a03c + 0x49) << 16;
    *g_2a064 = (g_2a044 - 0x50) << 16;
    g_2a064[1] = (g_2a03c + 0x5d) << 16;
    *g_2a070 = (g_2a044 + 0x50) << 16;
    g_2a070[1] = (g_2a03c + 0x5d) << 16;
    *g_2a05c = (g_2a044 + 0x6e) << 16;
    g_2a05c[1] = (g_2a03c + 0x53) << 16;
    *g_2a06c = (g_2a044 + 0x87) << 16;
    g_2a06c[1] = (g_2a03c + 0x3f) << 16;
    *g_2a000 = g_2a044 << 16;
    g_2a000[1] = g_2a03c << 16;
    *g_29ffc = (g_2a044 + 2) << 16;
    g_29ffc[1] = (g_2a03c + 2) << 16;
    *g_2a004 = g_2a044 << 16;
    g_2a004[1] = g_2a03c << 16;
    *g_29ff4 = (g_2a044 + 2) << 16;
    g_29ff4[1] = (g_2a03c + 2) << 16;
}

void title_071e0(int a1)
{
    int tbl[7];
    int x;
    int y;
    int s;

    x = (a1 * g_2a018) >> 7;
    y = ((0x80 - g_2a018) * a1) >> 7;
    tbl[0] = g_2239c;
    tbl[1] = g_22394;
    tbl[2] = g_2236c;
    tbl[3] = g_22364;
    tbl[4] = g_22390;
    tbl[5] = g_22388;
    tbl[6] = g_22370;

    switch (g_2a080) {
    case 0:
        title_0c2a0(g_29128, tbl[g_2a088], 0, g_2a078, 0x14312);
        g_2a088 += 1;
        if (g_2a088 == 6) {
            g_2a088 = 0;
        }
        g_2a080 = 1;
        /* fall through */
    case 1:
        if (*(short *)((char *)g_engine_interface.context_004 + 0x9c) == 0) {
            title_01dd0(g_2a078);
            title_0c8b0(g_2a078 + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
            g_2a080 = 2;
        }
        /* fall through */
    case 2:
        title_02090(0x140, 0, x, 0, g_2a044, g_2a03c, 0, 1);
        return;
    case 3:
        title_02090(0x140, 0, x, 0, g_2a044, g_2a03c, 0, 1);
        title_02090(0x280, 0x100, y, 1, g_2a044, g_2a03c, 1, 2);
        if (g_2a018 > 0) {
            g_2a018 -= 0x10;
            return;
        }
        g_2a018 = 0;
        g_2a080 = 4;
        return;
    case 4:
        title_0c2a0(g_29128, tbl[g_2a088], 0, g_2a078, 0x14312);
        g_2a088 += 1;
        if (g_2a088 == 6) {
            g_2a088 = 0;
        }
        g_2a080 = 5;
        /* fall through */
    case 5:
        if (*(short *)((char *)g_engine_interface.context_004 + 0x9c) == 0) {
            title_01dd0(g_2a078);
            title_0c8b0(g_2a078 + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
            g_2a080 = 6;
        }
        /* fall through */
    case 6:
        title_02090(0x280, 0x100, y, 0, g_2a044, g_2a03c, 1, 1);
        return;
    case 7:
        title_02090(0x140, 0, x, 0, g_2a044, g_2a03c, 0, 1);
        title_02090(0x280, 0x100, y, 1, g_2a044, g_2a03c, 1, 2);
        s = g_2a018;
        if (s < 0x80) {
            g_2a018 = s + 0x10;
            return;
        }
        g_2a018 = 0x80;
        g_2a080 = 0;
        return;
    default:
        return;
    }
}

