/* w06 shared views (scratch). Field names are by offset; unknown bytes kept as pad_XX. */
#include "title_engine.h"
#include "title_gpu.h"
typedef struct TitleSlot { unsigned char pad_00[0x0c]; unsigned int ptr_0c; unsigned short state_10; unsigned short state_12; } TitleSlot;
typedef struct TitleTabEntry { unsigned short a_00; unsigned short b_02; } TitleTabEntry;
typedef struct TitleCtx {
    unsigned char pad_00[0x1d];
    unsigned char b_1d;
    unsigned char b_1e;
    unsigned char pad_1f[3];
    unsigned char b_22;
    unsigned char pad_23[0x34 - 0x23];
    unsigned short w_34;
    unsigned short w_36;
    unsigned char pad_38[0x40 - 0x38];
    unsigned short w_40;
    unsigned short w_42;
    unsigned short w_44;
    unsigned short w_46;
    unsigned short w_48;
    unsigned char pad_4a[0x54 - 0x4a];
    unsigned int d_54;
    unsigned char pad_58[0x60 - 0x58];
    struct TitleCtx *p_60;
} TitleCtx;
typedef struct TitleGame {
    unsigned char pad_00[0x9c];
    unsigned short w_9c;
    unsigned char pad_9e[0xdf - 0x9e];
    unsigned char b_df;
} TitleGame;
extern TitleSlot g_26110[18];
extern TitleTabEntry g_2d2a0[];
extern unsigned char g_2df50;
extern int g_2df44;
extern char g_22100[];
extern char g_22108[];
extern unsigned char g_2dfac;
extern int g_2bb24;
extern int g_2df40;
extern int g_29da0;
unsigned char title_04710(char *p, int b, int c, int d);
unsigned char title_046b0(unsigned short key, unsigned int f);
void title_047d0(int a, int v, int b);
void title_042d0(TitleCtx *p);
TitleTabEntry *title_047a0(TitleCtx *c);
void title_04610(TitleCtx *p);
void title_04b70(int c, int d);
void title_04f00(int i);
void title_0c690(void *p, int n);
void title_0c890(void *p, int n);

/* ---- MASKED EQUAL functions, ascending RVA ---- */
/* f_4610 */
void title_04610(TitleCtx *p)
{
    unsigned int f = p->d_54;
    if (f & 0x10000800)
        return;
    title_042d0(p);
    title_047a0(p);
    if (f & 0x800000) {
        TitleCtx *q = p->p_60;
        if (q) {
            title_04610(q);
            title_047a0(q);
        }
    }
}

/* f_4660 */
void title_04660(void)
{
    title_0c690(g_2cc04->ot, TITLE_OT_SIZE);
    if (g_2cc04 == g_2cc08)
        g_2df40 = g_2bb24 + 0x11800;
    else
        g_2df40 = g_2bb24;
}

/* f_46b0 */
unsigned char title_046b0(unsigned short key, unsigned int f)
{
    unsigned int flag = ((f & 0x80) | 2) >> 1;
    TitleTabEntry *p;
    int i;
    for (p = g_2d2a0, i = 0; i < 0x20; p++, i++) {
        if (p->b_02 == key) {
            p->a_00++;
            return (unsigned char)i;
        }
    }
    for (p = g_2d2a0, i = 0; i < 0x20; p++, i++) {
        if (p->a_00 == 0) {
            g_2d2a0[i].b_02 = key;
            g_2d2a0[i].a_00 = (unsigned short)flag;
            return (unsigned char)(i | 0x80);
        }
    }
    return (unsigned char)(i | 0xff);
}

/* f_4760 */
void title_04760(void)
{
    TitleTabEntry *p = g_2d2a0;
    int i = 0x20;
    do {
        p->b_02 = 0xffff;
        p->a_00 = 0;
        p++;
    } while (--i != 0);
    g_2dfac = title_04710(g_22108, 0x7641, 0x20, 5);
}

/* f_4870 */
void title_04870(void)
{
    if (g_2df50) {
        title_0c890(g_22100, g_2df44);
        g_2df50 = 0;
    }
}

/* f_47a0 (integrator) */
TitleTabEntry *title_047a0(TitleCtx *c)
{
    TitleTabEntry *entry;
    if (c->b_1e != 0xff) {
        entry = &g_2d2a0[c->b_1e];
        entry->a_00--;
        c->b_1e = 0xff;
        c->b_1d = 0xff;
        return entry;
    }
}

/* f_47d0 */
void title_047d0(unsigned short *src, int row, int n)
{
    unsigned short *dst;
    int i;
    dst = (unsigned short *)(g_2df44 + (row << 9));
    if (n != 0x100) {
        *dst++ = 0;
        for (i = 1; i < n; i++)
            *dst++ = src[i] | 0x8000;
    } else {
        for (i = 0x100; i != 0; i--)
            *dst++ = *src++;
    }
    if (n != 0x100 && n < 0x100) {
        i = 0x100 - n;
        do {
            *dst = dst[-n];
            dst++;
        } while (--i != 0);
    }
    g_2df50 = 1;
}

/* f_4a40 */
void title_04a40(const unsigned char *s, int x)
{
    unsigned int c;
    if (s == 0)
        return;
    c = *s++;
    if (c == 0xff)
        return;
    do {
        if (c == (unsigned int)x && g_26110[c].ptr_0c == 0) {
            title_04b70(c, 0);
            g_26110[c].state_12 = 2;
        }
        c = *s++;
    } while (c != 0xff);
}

/* f_4b00 */
void title_04b00(void)
{
    int i;
    for (i = 1; i < 18; i++) {
        int s = g_26110[i].state_10;
        if (s == 4)
            title_04f00(i);
        if (s == 7) {
            title_04f00(i);
            g_26110[i].state_10 = 6;
        }
    }
}

/* f_4b50 */
void title_04b50(int idx)
{
    if (g_26110[idx].state_10 == 0)
        g_26110[idx].state_10 = 1;
}

