#include <string.h>
#include <io.h>
#include "title_engine.h"
#include "title_files.h"

typedef struct TitleObject {
    long unknown_000;
    long unknown_004;
    unsigned char unknown_008[0x23 - 0x08];
    unsigned char unknown_023;
    unsigned char unknown_024[0x34 - 0x24];
    unsigned short unknown_034;
    unsigned char unknown_036[0x3e - 0x36];
    unsigned short unknown_03e;
    unsigned char unknown_040[0x4a - 0x40];
    unsigned short unknown_04a;
    unsigned char unknown_04c[0x54 - 0x4c];
    unsigned long unknown_054;
    unsigned char unknown_058[0x74 - 0x58];
    unsigned short unknown_074;
} TitleObject;

extern TitleObject *g_29e08;
extern TitleObject *g_29ff4;
extern TitleObject *g_29ffc;
extern TitleObject *g_2a000;
extern TitleObject *g_2a004;
extern TitleObject *g_2a048;
extern TitleObject *g_2a04c;
extern TitleObject *g_2a050;
extern TitleObject *g_2a054;
extern TitleObject *g_2a058;
extern TitleObject *g_2a05c;
extern TitleObject *g_2a060;
extern TitleObject *g_2a064;
extern TitleObject *g_2a06c;
extern TitleObject *g_2a070;
extern int g_2a008;
extern int g_2a010;
extern int g_2a024;
extern int g_2a02c;
extern int g_2a034;
extern int g_2cc64;
extern int title_06480(void);
extern void title_064f0(void);
extern void title_065a0(void);
extern void title_066f0(void *buf, int n);
extern void title_0c600(int x);
extern void title_0c610(int x);
extern void title_0c620(int x);
extern void title_01dd0(int a);
extern void title_02090(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
extern void title_0c2a0(char *s, char *name, int b, int c, int d);
extern void title_0c8b0(int a1, int a2, int a3, int a4, int a5, int a6, int a7);
extern int g_2c7c0[];
extern char g_2c9c0[];
extern int g_2a044;
extern int g_2a03c;
extern int g_2a018;
extern int g_2a080;
extern int g_2a088;
extern int g_2a078;
extern char g_29128[];

void title_06bb0(void)
{
    g_2a004 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a004->unknown_054 |= 5;
    g_29ff4 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_29ff4->unknown_054 |= 6;
    g_2a000 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a000->unknown_054 |= 5;
    g_29ffc = title_17ad0(0, 0, 0, 0x2002, 0);
    g_29ffc->unknown_054 |= 6;
    g_2a000->unknown_034 = 1;
    g_29ffc->unknown_034 = 1;
    g_29e08 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_29e08->unknown_034 = 0x16;
    g_29e08->unknown_000 = 0xfea20000;
    g_29e08->unknown_004 = 0x500000;
    g_2cc64 = -1;

    g_2a050 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a050->unknown_034 = 0x3d;
    g_2a054 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a054->unknown_034 = 0x5c;
    g_2a048 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a048->unknown_034 = 0x7d;
    g_2a04c = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a04c->unknown_034 = 0xb0;
    g_2a058 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a058->unknown_034 = 0xcf;
    g_2a064 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a064->unknown_034 = 0x3d;
    g_2a05c = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a05c->unknown_034 = 0x5c;
    g_2a060 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a060->unknown_034 = 0x7d;
    g_2a06c = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a06c->unknown_034 = 0xb0;
    g_2a070 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a070->unknown_034 = 0xcf;

    g_29e08->unknown_023 = 6;
    g_29e08->unknown_03e = 0x1e;
    g_2a048->unknown_023 = 6;
    g_2a048->unknown_03e = 0x18;
    g_2a050->unknown_023 = 6;
    g_2a050->unknown_03e = 0x17;
    g_2a058->unknown_023 = 6;
    g_2a058->unknown_03e = 0x14;
    g_2a054->unknown_023 = 6;
    g_2a054->unknown_03e = 0x15;
    g_2a04c->unknown_023 = 6;
    g_2a04c->unknown_03e = 0x16;
    g_2a064->unknown_023 = 6;
    g_2a064->unknown_03e = 0x1e;
    g_2a05c->unknown_023 = 6;
    g_2a05c->unknown_03e = 0x1e;
    g_2a060->unknown_023 = 6;
    g_2a060->unknown_03e = 0x1e;
    g_2a06c->unknown_023 = 6;
    g_2a06c->unknown_03e = 0x1e;
    g_2a070->unknown_023 = 6;
    g_2a070->unknown_03e = 0x1e;

    g_2a064->unknown_054 |= 4;
    g_2a05c->unknown_054 |= 4;
    g_2a060->unknown_054 |= 4;
    g_2a06c->unknown_054 |= 4;
    g_2a070->unknown_054 |= 4;

    g_2a060->unknown_074 = 0x80;
    g_2a064->unknown_074 = 0x70;
    g_2a070->unknown_074 = 0xff80;
    g_2a05c->unknown_074 = 0xff90;
    g_2a06c->unknown_074 = 0xffa0;

    g_2a070->unknown_04a = 0;
    g_2a06c->unknown_04a = 0;
    g_2a060->unknown_04a = 0;
    g_2a05c->unknown_04a = 0;
    g_2a064->unknown_04a = 0;

    g_2a004->unknown_023 = 6;
    g_2a004->unknown_03e = 10;
    g_29ff4->unknown_023 = 6;
    g_29ff4->unknown_03e = 0xf;
    g_2a000->unknown_023 = 6;
    g_2a000->unknown_03e = 10;
    g_29ffc->unknown_023 = 6;
    g_29ffc->unknown_03e = 0xf;

    g_2a010 = 0;
    g_2a008 = 0;
    g_2a024 = 0;
    g_2a02c = 0;
    g_2a034 = 0;
}

void title_06fb0(void)
{
    g_2a048->unknown_000 = (g_2a044 - 0x7d) << 16;
    g_2a048->unknown_004 = (g_2a03c + 0x55) << 16;
    g_2a050->unknown_000 = (g_2a044 - 0x50) << 16;
    g_2a050->unknown_004 = (g_2a03c + 0x69) << 16;
    g_2a058->unknown_000 = (g_2a044 + 0x50) << 16;
    g_2a058->unknown_004 = (g_2a03c + 0x69) << 16;
    g_2a054->unknown_000 = (g_2a044 + 0x6e) << 16;
    g_2a054->unknown_004 = (g_2a03c + 0x5f) << 16;
    g_2a04c->unknown_000 = (g_2a044 + 0x87) << 16;
    g_2a04c->unknown_004 = (g_2a03c + 0x4b) << 16;
    g_2a060->unknown_000 = (g_2a044 - 0x7d) << 16;
    g_2a060->unknown_004 = (g_2a03c + 0x49) << 16;
    g_2a064->unknown_000 = (g_2a044 - 0x50) << 16;
    g_2a064->unknown_004 = (g_2a03c + 0x5d) << 16;
    g_2a070->unknown_000 = (g_2a044 + 0x50) << 16;
    g_2a070->unknown_004 = (g_2a03c + 0x5d) << 16;
    g_2a05c->unknown_000 = (g_2a044 + 0x6e) << 16;
    g_2a05c->unknown_004 = (g_2a03c + 0x53) << 16;
    g_2a06c->unknown_000 = (g_2a044 + 0x87) << 16;
    g_2a06c->unknown_004 = (g_2a03c + 0x3f) << 16;
    g_2a000->unknown_000 = g_2a044 << 16;
    g_2a000->unknown_004 = g_2a03c << 16;
    g_29ffc->unknown_000 = (g_2a044 + 2) << 16;
    g_29ffc->unknown_004 = (g_2a03c + 2) << 16;
    g_2a004->unknown_000 = g_2a044 << 16;
    g_2a004->unknown_004 = g_2a03c << 16;
    g_29ff4->unknown_000 = (g_2a044 + 2) << 16;
    g_29ff4->unknown_004 = (g_2a03c + 2) << 16;
}

void title_071e0(int a1)
{
    char *tbl[7];
    int x;
    int y;
    int s;

    x = (a1 * g_2a018) >> 7;
    y = ((0x80 - g_2a018) * a1) >> 7;
    tbl[0] = g_sequence_files[TITLE_SEQ_T015];
    tbl[1] = g_sequence_files[TITLE_SEQ_T013];
    tbl[2] = g_sequence_files[TITLE_SEQ_T003];
    tbl[3] = g_sequence_files[TITLE_SEQ_T001];
    tbl[4] = g_sequence_files[TITLE_SEQ_T012];
    tbl[5] = g_sequence_files[TITLE_SEQ_T010];
    tbl[6] = g_sequence_files[TITLE_SEQ_T004];

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
