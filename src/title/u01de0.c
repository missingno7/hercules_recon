/* TITLE unit 0x1de0..0x2a80 (C): overlay sprites, flat quads and the wave grid (PSX SPRT, POLY_F4,
   POLY_GT4 and DR_TPAGE records); private .bss 0x29148..0x29584.
   TU-context hypothesis (owner-approved 2026-10-09): <windows.h> supplies the large prior symbol
   count this unit evidently had; with fewer than about 400 prior declarations VC5 emits
   [index + global] and flips the AddPrim add destination (blocker B1). Not proven original text. */
#include <windows.h>

#include "title_gpu.h"
#include "title_engine.h"

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
/* Static names below are layout-chosen (owner policy 2026-10-09): VC5 orders this block by identifier hash, and
   these role-derived names reproduce the original .bss order 0x29148..0x29588 (17/17); they are not recovered names. */
static TitleTPage s_overlay_tpages[4];         /* 0x1de0 texture page, first sprite */
static TitleTPage s_slot_tpage2s[8];         /* 0x2090 texture page, second sprite */
static TitleTPage s_abr_tpage[8];         /* 0x28c0 texture page (abr) */
static TitleSprt s_big_sprt2[4];          /* 0x1de0 second sprite (x 0x100/0x80) */
static TitleTPage s_slot_tpage1[8];         /* 0x2090 texture page, first sprite */
static TitlePolyF4 s_flat_quad[8];        /* 0x28c0 flat quad */
static TitleTPage overlay_tpage2s[4];         /* 0x1de0 texture page, second sprite */
static TitleSprt s_slot_sprt2_buf[8];          /* 0x2090 second sprite */
static TitleSprt s_pair_sprt1[8];          /* 0x2090 first sprite */
static TitleSprt overlay_sprt1[4];          /* 0x1de0 first sprite */

static int s_grid_states;                   /* 0x23c0 clears it on grid setup (0x29478; 0x2947c unseen) */

/* 25 x 25 grid of 0x10.0x10 fixed-point vertices (allocated by 0x23c0, released by 0x2350). */
static int (*s_wave_x_tab)[25];                 /* x << 20 */
static int (*wave_ys)[25];                 /* y (accumulates the wave) */
static int (*s_wave_amp_buf)[25];                 /* amplitude */
static int (*s_grid_shade)[25];                 /* shade delta */
static int (*s_wave_phase_list)[25];                 /* phase */
static TitlePolyGT4 *s_grid_quads_buf;          /* 24 x 24 textured quads */


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

    title_0caf0(&s_overlay_tpages[i], 0, 0, (unsigned short)title_0c860(2, 0, x, y));
    overlay_sprt1[i].code = 0x64;
    overlay_sprt1[i].u0 = 0;
    overlay_sprt1[i].v0 = 0;
    overlay_sprt1[i].x0 = 0;
    overlay_sprt1[i].y0 = 0;
    overlay_sprt1[i].w = 0x100;
    overlay_sprt1[i].h = 0x100;
    overlay_sprt1[i].r0 = shade;
    overlay_sprt1[i].g0 = shade;
    overlay_sprt1[i].b0 = shade;
    overlay_sprt1[i].code &= 0xfd;
    if (title_0c860(2, 0, x, y) & 1) {
        overlay_sprt1[i].u0 = 0x80;
        overlay_sprt1[i].v0 = 0;
        overlay_sprt1[i].w = 0x80;
        overlay_sprt1[i].h = 0x100;
    } else {
        overlay_sprt1[i].w = 0x100;
        overlay_sprt1[i].h = 0x100;
    }
    title_0c650(&g_2cc04->ot[depth], &overlay_sprt1[i]);
    title_0c650(&g_2cc04->ot[depth], &s_overlay_tpages[i]);

    title_0caf0(&overlay_tpage2s[i], 0, 0, (unsigned short)title_0c860(2, 0, x + 0x100, y));
    s_big_sprt2[i].code = 0x64;
    s_big_sprt2[i].u0 = 0;
    s_big_sprt2[i].v0 = 0;
    s_big_sprt2[i].x0 = 0x100;
    s_big_sprt2[i].y0 = 0;
    s_big_sprt2[i].w = 0x40;
    s_big_sprt2[i].h = 0x100;
    s_big_sprt2[i].r0 = shade;
    s_big_sprt2[i].g0 = shade;
    s_big_sprt2[i].b0 = shade;
    s_big_sprt2[i].code &= 0xfd;
    if (title_0c860(2, 0, x + 0x80, y) & 1) {
        title_0caf0(&overlay_tpage2s[i], 0, 0, (unsigned short)title_0c860(2, 0, x + 0x80, y));
        s_big_sprt2[i].w = 0xc0;
        s_big_sprt2[i].h = 0x100;
        s_big_sprt2[i].x0 = 0x80;
    } else {
        title_0caf0(&overlay_tpage2s[i], 0, 0, (unsigned short)title_0c860(2, 0, x + 0x80, y));
        s_big_sprt2[i].x0 = 0x100;
    }
    s_big_sprt2[i].y0 = 0;
    title_0c650(&g_2cc04->ot[depth], &s_big_sprt2[i]);
    title_0c650(&g_2cc04->ot[depth], &overlay_tpage2s[i]);
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

    title_0caf0(&s_slot_tpage1[i], 0, 0, (unsigned short)title_0c860(2, 0, x, y));
    s_pair_sprt1[i].code = 0x64;
    s_pair_sprt1[i].u0 = 0;
    s_pair_sprt1[i].v0 = 0;
    s_pair_sprt1[i].x0 = sx;
    s_pair_sprt1[i].y0 = sy;
    s_pair_sprt1[i].w = 0x100;
    s_pair_sprt1[i].h = 0x100;
    s_pair_sprt1[i].r0 = shade;
    s_pair_sprt1[i].g0 = shade;
    s_pair_sprt1[i].b0 = shade;
    s_pair_sprt1[i].code &= 0xfd;
    if (title_0c860(2, 0, x, y) & 1) {
        s_pair_sprt1[i].u0 = 0x80;
        s_pair_sprt1[i].v0 = 0;
        s_pair_sprt1[i].w = 0x80;
        s_pair_sprt1[i].h = 0x100;
    }
    title_0c650(&g_2cc04->ot[depth], &s_pair_sprt1[i]);
    title_0c650(&g_2cc04->ot[depth], &s_slot_tpage1[i]);

    title_0caf0(&s_slot_tpage2s[i], 0, 0, (unsigned short)title_0c860(2, 0, x + 0x100, y));
    s_slot_sprt2_buf[i].code = 0x64;
    s_slot_sprt2_buf[i].u0 = 0;
    s_slot_sprt2_buf[i].v0 = 0;
    s_slot_sprt2_buf[i].x0 = sx + 0x100;
    s_slot_sprt2_buf[i].y0 = sy;
    s_slot_sprt2_buf[i].w = 0x40;
    s_slot_sprt2_buf[i].h = 0x100;
    s_slot_sprt2_buf[i].r0 = shade;
    s_slot_sprt2_buf[i].g0 = shade;
    s_slot_sprt2_buf[i].b0 = shade;
    s_slot_sprt2_buf[i].code &= 0xfd;
    if (title_0c860(2, 0, x + 0x100, y) & 1) {
        title_0caf0(&s_slot_tpage2s[i], 0, 0, (unsigned short)title_0c860(2, 0, x + 0x80, y));
        s_slot_sprt2_buf[i].w = 0xc0;
        s_slot_sprt2_buf[i].h = 0x100;
        s_slot_sprt2_buf[i].x0 = sx + 0x80;
        s_slot_sprt2_buf[i].y0 = sy;
    } else {
        title_0caf0(&s_slot_tpage2s[i], 0, 0, (unsigned short)title_0c860(2, 0, x + 0x80, y));
    }
    title_0c650(&g_2cc04->ot[depth], &s_slot_sprt2_buf[i]);
    title_0c650(&g_2cc04->ot[depth], &s_slot_tpage2s[i]);
}

void title_02350(void)
{
    if (s_wave_x_tab) {
        title_0c330(s_grid_quads_buf);
        title_0c330(wave_ys);
        title_0c330(s_wave_phase_list);
        title_0c330(s_grid_shade);
        title_0c330(s_wave_amp_buf);
        title_0c330(s_wave_x_tab);
        s_wave_x_tab = 0;
    }
}

void title_023c0(int x, int y)
{
    int i;
    int j;
    int n;
    int u;

    s_grid_states = 0;
    if (!s_wave_x_tab) {
        title_0c450(&s_wave_x_tab, 0x9c4, 0);
        title_0c450(&s_wave_amp_buf, 0x9c4, 0);
        title_0c450(&s_grid_shade, 0x9c4, 0);
        title_0c450(&s_wave_phase_list, 0x9c4, 0);
        title_0c450(&wave_ys, 0x9c4, 0);
        title_0c450(&s_grid_quads_buf, 0x7500, 0);
    }

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 25; j++) {
            s_wave_x_tab[i][j] = j << 20;
            wave_ys[i][j] = i * 0xb0000;
            s_wave_amp_buf[i][j] = 0xff - (j - 12) * (j - 12) - (i - 12) * (i - 12);
            s_wave_phase_list[i][j] = s_wave_amp_buf[i][j] * 50;
        }
    }

    n = 0;
    for (i = 0; i < 240; i += 10) {
        for (j = 0; j < 24; j++) {
            u = j * 16 & 0x3f;
            title_0cb50(&s_grid_quads_buf[n]);
            s_grid_quads_buf[n].code &= 0xfd;
            s_grid_quads_buf[n].tpage = title_0c860(2, 1, x + j * 16, y);
            s_grid_quads_buf[n].u0 = u;
            s_grid_quads_buf[n].v0 = i;
            s_grid_quads_buf[n].u1 = u + 0x10;
            s_grid_quads_buf[n].v1 = i;
            s_grid_quads_buf[n].u2 = u;
            s_grid_quads_buf[n].v2 = i + 10;
            s_grid_quads_buf[n].u3 = u + 0x10;
            s_grid_quads_buf[n].v3 = i + 10;
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
            s_grid_shade[i][j] = (wave_ys[i][j] >> 13) - (wave_ys[i][j + 1] >> 13);
        }
    }

    n = 0;
    for (i = 0; i < 24; i++) {
        for (j = 0; j < 24; j++) {
            c0 = (s_grid_shade[i][j] + 0x80) * scale >> 7;
            c1 = (s_grid_shade[i][j + 1] + 0x80) * scale >> 7;
            c2 = (s_grid_shade[i + 1][j] + 0x80) * scale >> 7;
            c3 = (s_grid_shade[i + 1][j + 1] + 0x80) * scale >> 7;
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
                s_grid_quads_buf[n].code |= 2;
            else
                s_grid_quads_buf[n].code &= 0xfd;
            s_grid_quads_buf[n].r0 = c0;
            s_grid_quads_buf[n].g0 = c0;
            s_grid_quads_buf[n].b0 = c0;
            s_grid_quads_buf[n].r1 = c1;
            s_grid_quads_buf[n].g1 = c1;
            s_grid_quads_buf[n].b1 = c1;
            s_grid_quads_buf[n].r2 = c2;
            s_grid_quads_buf[n].g2 = c2;
            s_grid_quads_buf[n].b2 = c2;
            s_grid_quads_buf[n].r3 = c3;
            s_grid_quads_buf[n].g3 = c3;
            s_grid_quads_buf[n].b3 = c3;
            s_grid_quads_buf[n].x0 = s_wave_x_tab[i][j] >> 16;
            s_grid_quads_buf[n].y0 = wave_ys[i][j] >> 16;
            s_grid_quads_buf[n].x1 = s_wave_x_tab[i][j + 1] >> 16;
            s_grid_quads_buf[n].y1 = wave_ys[i][j + 1] >> 16;
            s_grid_quads_buf[n].x2 = s_wave_x_tab[i + 1][j] >> 16;
            s_grid_quads_buf[n].y2 = wave_ys[i + 1][j] >> 16;
            s_grid_quads_buf[n].x3 = s_wave_x_tab[i + 1][j + 1] >> 16;
            s_grid_quads_buf[n].y3 = wave_ys[i + 1][j + 1] >> 16;
            title_0c6c0(&s_grid_quads_buf[n]);
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
            phase = s_wave_phase_list[i][j];
            s_wave_phase_list[i][j] = s_wave_amp_buf[i][j] + phase;
            wave_ys[i][j] += s_wave_amp_buf[i][j] * ((short *)g_engine_interface.data_014)[phase & 0xfff] >> 4;
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
    title_0caf0(&s_abr_tpage[i], 0, 0, (unsigned short)title_0c860(2, abr, 0, 0));
    title_0cb40(&s_flat_quad[i]);
    s_flat_quad[i].x0 = x;
    s_flat_quad[i].y0 = y;
    s_flat_quad[i].x1 = x + w;
    s_flat_quad[i].y1 = y;
    s_flat_quad[i].x2 = x;
    s_flat_quad[i].y2 = y + h;
    s_flat_quad[i].x3 = x + w;
    s_flat_quad[i].y3 = y + h;
    s_flat_quad[i].r0 = r;
    s_flat_quad[i].g0 = g;
    s_flat_quad[i].b0 = b;
    if (!abr)
        s_flat_quad[i].code &= 0xfd;
    else
        s_flat_quad[i].code |= 2;
    title_0c650(&g_2cc04->ot[depth], &s_flat_quad[i]);
    title_0c650(&g_2cc04->ot[depth], &s_abr_tpage[i]);
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

