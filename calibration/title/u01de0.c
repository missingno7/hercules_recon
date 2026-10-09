/* TITLE unit 0x1de0..0x2a80 (C): overlay sprites, flat quads and the wave grid (PSX SPRT, POLY_F4,
   POLY_GT4 and DR_TPAGE records); private .bss 0x29148..0x29584.
   TU-context hypothesis (owner-approved 2026-10-09): <windows.h> supplies the large prior symbol
   count this unit evidently had; with fewer than about 400 prior declarations VC5 emits
   [index + global] and flips the AddPrim add destination (blocker B1). Not proven original text. */
#include <windows.h>

#include "title_gpu.h"

typedef struct TitleSprt {            /* PSX SPRT, 0x14 bytes */
    unsigned long tag;
    unsigned char r0, g0, b0, code;
    short x0, y0;
    unsigned char u0, v0;
    unsigned short clut;
    short w, h;
} TitleSprt;

typedef struct TitlePolyF4 {          /* PSX POLY_F4, 0x18 bytes */
    unsigned long tag;
    unsigned char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
    short x2, y2;
    short x3, y3;
} TitlePolyF4;

typedef struct TitleTPage {           /* DR_TPAGE as laid out by the PC port, 0x0c bytes */
    unsigned long tag;
    unsigned long code[2];
} TitleTPage;

typedef struct TitlePolyGT4 {         /* PSX POLY_GT4, 0x34 bytes */
    unsigned long tag;
    unsigned char r0, g0, b0, code;
    short x0, y0;
    unsigned char u0, v0;
    unsigned short clut;
    unsigned char r1, g1, b1, p1;
    short x1, y1;
    unsigned char u1, v1;
    unsigned short tpage;
    unsigned char r2, g2, b2, p2;
    short x2, y2;
    unsigned char u2, v2;
    unsigned short pad2;
    unsigned char r3, g3, b3, p3;
    short x3, y3;
    unsigned char u3, v3;
    unsigned short pad3;
} TitlePolyGT4;

/* Private .bss of the unit (extents from the next known address; layout order not reproduced).
   Each table is indexed by slot + display buffer (g_2cc04 == second buffer). */
static TitleTPage g_29148[4];         /* 0x1de0 texture page, first sprite */
static TitleTPage g_29178[8];         /* 0x2090 texture page, second sprite */
static TitleTPage g_291d8[8];         /* 0x28c0 texture page (abr) */
static TitleSprt g_29238[4];          /* 0x1de0 second sprite (x 0x100/0x80) */
static TitleTPage g_29288[8];         /* 0x2090 texture page, first sprite */
static TitlePolyF4 g_292e8[8];        /* 0x28c0 flat quad */
static TitleTPage g_293a8[4];         /* 0x1de0 texture page, second sprite */
static TitleSprt g_293d8[8];          /* 0x2090 second sprite */
static TitleSprt g_29480[8];          /* 0x2090 first sprite */
static TitleSprt g_29520[4];          /* 0x1de0 first sprite */

static int g_29478;                   /* 0x23c0 clears it on grid setup (0x29478; 0x2947c unseen) */

/* 25 x 25 grid of 0x10.0x10 fixed-point vertices (allocated by 0x23c0, released by 0x2350). */
static int (*g_29570)[25];                 /* x << 20 */
static int (*g_29574)[25];                 /* y (accumulates the wave) */
static int (*g_29578)[25];                 /* amplitude */
static int (*g_2957c)[25];                 /* shade delta */
static int (*g_29580)[25];                 /* phase */
static TitlePolyGT4 *g_29584;          /* 24 x 24 textured quads */

extern short *g_2bb54;                /* sine table, 0x1000 entries */

void title_0c330(void *p);
void title_0c450(void *out, int size, int flags);
void title_0cb50(TitlePolyGT4 *p);
void title_0c6c0(TitlePolyGT4 *p);
void title_0c6d0(int a);
void title_0cc70(int a);
void title_0c670(void *p, int a, int b, int c);
int title_0c860(int tp, int abr, int x, int y);
void title_0caf0(TitleTPage *p, int dfe, int dtd, int tpage);
void title_0cb40(TitlePolyF4 *p);
void title_0c650(unsigned long *ot, void *p);

void title_01de0(int x, int y, unsigned int shade, int unused, int depth)
{
    int i;

    i = 0;
    if (shade > 0xff)
        shade = 0;
    if (g_2cc04 == g_2cc08 + 1)
        i = 1;
    if (depth)
        i += 2;

    title_0caf0(&g_29148[i], 0, 0, (unsigned short)title_0c860(2, 0, x, y));
    g_29520[i].code = 0x64;
    g_29520[i].u0 = 0;
    g_29520[i].v0 = 0;
    g_29520[i].x0 = 0;
    g_29520[i].y0 = 0;
    g_29520[i].w = 0x100;
    g_29520[i].h = 0x100;
    g_29520[i].r0 = shade;
    g_29520[i].g0 = shade;
    g_29520[i].b0 = shade;
    g_29520[i].code &= 0xfd;
    if (title_0c860(2, 0, x, y) & 1) {
        g_29520[i].u0 = 0x80;
        g_29520[i].v0 = 0;
        g_29520[i].w = 0x80;
        g_29520[i].h = 0x100;
    } else {
        g_29520[i].w = 0x100;
        g_29520[i].h = 0x100;
    }
    title_0c650(&g_2cc04->ot[depth], &g_29520[i]);
    title_0c650(&g_2cc04->ot[depth], &g_29148[i]);

    title_0caf0(&g_293a8[i], 0, 0, (unsigned short)title_0c860(2, 0, x + 0x100, y));
    g_29238[i].code = 0x64;
    g_29238[i].u0 = 0;
    g_29238[i].v0 = 0;
    g_29238[i].x0 = 0x100;
    g_29238[i].y0 = 0;
    g_29238[i].w = 0x40;
    g_29238[i].h = 0x100;
    g_29238[i].r0 = shade;
    g_29238[i].g0 = shade;
    g_29238[i].b0 = shade;
    g_29238[i].code &= 0xfd;
    if (title_0c860(2, 0, x + 0x80, y) & 1) {
        title_0caf0(&g_293a8[i], 0, 0, (unsigned short)title_0c860(2, 0, x + 0x80, y));
        g_29238[i].w = 0xc0;
        g_29238[i].h = 0x100;
        g_29238[i].x0 = 0x80;
    } else {
        title_0caf0(&g_293a8[i], 0, 0, (unsigned short)title_0c860(2, 0, x + 0x80, y));
        g_29238[i].x0 = 0x100;
    }
    g_29238[i].y0 = 0;
    title_0c650(&g_2cc04->ot[depth], &g_29238[i]);
    title_0c650(&g_2cc04->ot[depth], &g_293a8[i]);
}

void title_02090(int x, int y, unsigned int shade, int unused, int sx, int sy, int slot, int depth)
{
    int i;

    i = 0;
    if (shade > 0xff)
        shade = 0;
    if (g_2cc04 == g_2cc08 + 1)
        i = 1;
    i += slot * 2;

    title_0caf0(&g_29288[i], 0, 0, (unsigned short)title_0c860(2, 0, x, y));
    g_29480[i].code = 0x64;
    g_29480[i].u0 = 0;
    g_29480[i].v0 = 0;
    g_29480[i].x0 = sx;
    g_29480[i].y0 = sy;
    g_29480[i].w = 0x100;
    g_29480[i].h = 0x100;
    g_29480[i].r0 = shade;
    g_29480[i].g0 = shade;
    g_29480[i].b0 = shade;
    g_29480[i].code &= 0xfd;
    if (title_0c860(2, 0, x, y) & 1) {
        g_29480[i].u0 = 0x80;
        g_29480[i].v0 = 0;
        g_29480[i].w = 0x80;
        g_29480[i].h = 0x100;
    }
    title_0c650(&g_2cc04->ot[depth], &g_29480[i]);
    title_0c650(&g_2cc04->ot[depth], &g_29288[i]);

    title_0caf0(&g_29178[i], 0, 0, (unsigned short)title_0c860(2, 0, x + 0x100, y));
    g_293d8[i].code = 0x64;
    g_293d8[i].u0 = 0;
    g_293d8[i].v0 = 0;
    g_293d8[i].x0 = sx + 0x100;
    g_293d8[i].y0 = sy;
    g_293d8[i].w = 0x40;
    g_293d8[i].h = 0x100;
    g_293d8[i].r0 = shade;
    g_293d8[i].g0 = shade;
    g_293d8[i].b0 = shade;
    g_293d8[i].code &= 0xfd;
    if (title_0c860(2, 0, x + 0x100, y) & 1) {
        title_0caf0(&g_29178[i], 0, 0, (unsigned short)title_0c860(2, 0, x + 0x80, y));
        g_293d8[i].w = 0xc0;
        g_293d8[i].h = 0x100;
        g_293d8[i].x0 = sx + 0x80;
        g_293d8[i].y0 = sy;
    } else {
        title_0caf0(&g_29178[i], 0, 0, (unsigned short)title_0c860(2, 0, x + 0x80, y));
    }
    title_0c650(&g_2cc04->ot[depth], &g_293d8[i]);
    title_0c650(&g_2cc04->ot[depth], &g_29178[i]);
}

void title_02350(void)
{
    if (g_29570) {
        title_0c330(g_29584);
        title_0c330(g_29574);
        title_0c330(g_29580);
        title_0c330(g_2957c);
        title_0c330(g_29578);
        title_0c330(g_29570);
        g_29570 = 0;
    }
}

void title_023c0(int x, int y)
{
    int i;
    int j;
    int n;
    int u;

    g_29478 = 0;
    if (!g_29570) {
        title_0c450(&g_29570, 0x9c4, 0);
        title_0c450(&g_29578, 0x9c4, 0);
        title_0c450(&g_2957c, 0x9c4, 0);
        title_0c450(&g_29580, 0x9c4, 0);
        title_0c450(&g_29574, 0x9c4, 0);
        title_0c450(&g_29584, 0x7500, 0);
    }

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 25; j++) {
            g_29570[i][j] = j << 20;
            g_29574[i][j] = i * 0xb0000;
            g_29578[i][j] = 0xff - (j - 12) * (j - 12) - (i - 12) * (i - 12);
            g_29580[i][j] = g_29578[i][j] * 50;
        }
    }

    n = 0;
    for (i = 0; i < 240; i += 10) {
        for (j = 0; j < 24; j++) {
            u = j * 16 & 0x3f;
            title_0cb50(&g_29584[n]);
            g_29584[n].code &= 0xfd;
            g_29584[n].tpage = title_0c860(2, 1, x + j * 16, y);
            g_29584[n].u0 = u;
            g_29584[n].v0 = i;
            g_29584[n].u1 = u + 0x10;
            g_29584[n].v1 = i;
            g_29584[n].u2 = u;
            g_29584[n].v2 = i + 10;
            g_29584[n].u3 = u + 0x10;
            g_29584[n].v3 = i + 10;
            n++;
        }
    }
}

void title_025c0(int scale, int semi)
{
    int i;
    int j;
    int c0;
    int c1;
    int c2;
    int c3;
    int n;

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 25; j++) {
            g_2957c[i][j] = (g_29574[i][j] >> 13) - (g_29574[i][j + 1] >> 13);
        }
    }

    n = 0;
    for (i = 0; i < 24; i++) {
        for (j = 0; j < 24; j++) {
            c0 = (g_2957c[i][j] + 0x80) * scale >> 7;
            c1 = (g_2957c[i][j + 1] + 0x80) * scale >> 7;
            c2 = (g_2957c[i + 1][j] + 0x80) * scale >> 7;
            c3 = (g_2957c[i + 1][j + 1] + 0x80) * scale >> 7;
            if (c0 < 0)
                c0 = 0;
            if (c1 < 0)
                c1 = 0;
            if (c2 < 0)
                c2 = 0;
            if (c3 < 0)
                c3 = 0;
            if (c0 > 0xff)
                c0 = 0xff;
            if (c1 > 0xff)
                c1 = 0xff;
            if (c2 > 0xff)
                c2 = 0xff;
            if (c3 > 0xff)
                c3 = 0xff;
            if (semi)
                g_29584[n].code |= 2;
            else
                g_29584[n].code &= 0xfd;
            g_29584[n].r0 = c0;
            g_29584[n].g0 = c0;
            g_29584[n].b0 = c0;
            g_29584[n].r1 = c1;
            g_29584[n].g1 = c1;
            g_29584[n].b1 = c1;
            g_29584[n].r2 = c2;
            g_29584[n].g2 = c2;
            g_29584[n].b2 = c2;
            g_29584[n].r3 = c3;
            g_29584[n].g3 = c3;
            g_29584[n].b3 = c3;
            g_29584[n].x0 = g_29570[i][j] >> 16;
            g_29584[n].y0 = g_29574[i][j] >> 16;
            g_29584[n].x1 = g_29570[i][j + 1] >> 16;
            g_29584[n].y1 = g_29574[i][j + 1] >> 16;
            g_29584[n].x2 = g_29570[i + 1][j] >> 16;
            g_29584[n].y2 = g_29574[i + 1][j] >> 16;
            g_29584[n].x3 = g_29570[i + 1][j + 1] >> 16;
            g_29584[n].y3 = g_29574[i + 1][j + 1] >> 16;
            title_0c6c0(&g_29584[n]);
            n++;
        }
    }
}

void title_02860(void)
{
    int i;
    int j;
    int phase;

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 25; j++) {
            phase = g_29580[i][j];
            g_29580[i][j] = g_29578[i][j] + phase;
            g_29574[i][j] += g_29578[i][j] * g_2bb54[phase & 0xfff] >> 4;
        }
    }
}

void title_028c0(int x, int y, int w, int h, unsigned int r, unsigned int g, unsigned int b,
                 unsigned int scale, unsigned int abr, int slot, int depth)
{
    int i;

    i = slot;
    if (abr > 0xff)
        abr = 0;
    if (g_2cc04 == g_2cc08 + 1)
        i += 4;
    r = r * scale >> 7;
    g = g * scale >> 7;
    b = b * scale >> 7;
    title_0caf0(&g_291d8[i], 0, 0, (unsigned short)title_0c860(2, abr, 0, 0));
    title_0cb40(&g_292e8[i]);
    g_292e8[i].x0 = x;
    g_292e8[i].y0 = y;
    g_292e8[i].x1 = x + w;
    g_292e8[i].y1 = y;
    g_292e8[i].x2 = x;
    g_292e8[i].y2 = y + h;
    g_292e8[i].x3 = x + w;
    g_292e8[i].y3 = y + h;
    g_292e8[i].r0 = r;
    g_292e8[i].g0 = g;
    g_292e8[i].b0 = b;
    if (!abr)
        g_292e8[i].code &= 0xfd;
    else
        g_292e8[i].code |= 2;
    title_0c650(&g_2cc04->ot[depth], &g_292e8[i]);
    title_0c650(&g_2cc04->ot[depth], &g_291d8[i]);
}

void title_02a20(void)
{
    unsigned short rc[4];

    rc[0] = 0;
    rc[1] = 0;
    rc[2] = 0x140;
    rc[3] = 0x200;
    title_0c6d0(0);
    title_0cc70(0);
    title_0c670(rc, 0, 0, 0);
    title_0c6d0(0);
}

