/* TITLE.DLL lane w12 region: functions that reached MASKED EQUAL, ascending RVA.
 * Shared engine include, struct views, externs and prototypes first. */
#include "title_engine.h"

typedef struct TitleObj {
    unsigned char unknown_000[0x50];
    unsigned long flags_050;
    unsigned long flags_054;
    void *ptr_058;
    struct TitleObj *ptr_05c;
    void *ptr_060;
    unsigned long unknown_064;
    unsigned long unknown_068;
    void *ptr_06c;
    unsigned char unknown_070[0x12c - 0x70];
    unsigned long unknown_12c;
    unsigned long unknown_130;
} TitleObj;

typedef struct TitleCtx {
    unsigned char unknown_000[6];
    unsigned char state_006;
    unsigned char unknown_007[0x30 - 7];
    unsigned long flags_030;
} TitleCtx;

typedef struct TitleRec16 {
    unsigned char unknown_000[8];
    unsigned short w8;
    unsigned short wa;
    unsigned char unknown_00c[4];
} TitleRec16;

extern TitleRec16 g_251e0[];
extern int g_253a0;
extern int g_2a178;
extern int g_2a17c;
extern int g_2a180;
extern int g_2a184;
extern int g_2a190;
extern int g_2a194;
extern int g_2a19c;
extern int g_2a1a0;
extern int g_2a1b8;
extern int g_2a1c4;
extern int g_2a1e0;
extern int g_2a1e8;
extern int g_2a208;
extern int g_2a220;

void title_05a70(TitleObj *p);
void title_09350(TitleObj *p);
void unlink_12c(TitleObj *p);
void title_0ca30(void);
void title_1d770(void);
void title_0c9c0(int a, int b);
int title_0c9e0(int a, int b);
unsigned int title_0c4a0(unsigned int a);
void title_09a40(void);
void title_09c90(void);
void title_0a020(void);
void title_01de0(int a, int b, int c, int d, int e);
void title_02090(int a, int b, int c, int d, int e, int f, int g, int h);

void title_097f0(TitleObj *p)
{
    TitleObj *q = p->ptr_05c;

    if (q && !(p->flags_050 & 0x20000)) {
        if (p->flags_050 & 0x10000)
            p = q;
        p->ptr_05c = 0;
        if (q->flags_050 & 0x10000)
            title_09350(q);
    }
}

void title_09830(TitleObj *p)
{
    void *q = p->ptr_060;

    if (q && !(p->flags_050 & 0x40000000)) {
        p->flags_054 &= 0xff7fffff;
        p->ptr_060 = 0;
        title_09350(q);
    }
}

void title_09870(TitleObj *p)
{
    if (!(p->flags_050 & 0x10000000)) {
        if (p->ptr_06c) {
            p->ptr_06c = 0;
            title_05a70(p);
        }
    }
}

void title_098f0(TitleObj *p)
{
    p->flags_054 &= 0x7fffffff;
    unlink_12c(p);
    p->unknown_12c = 0;
    p->unknown_130 = 0;
}

void title_09920(void)
{
    unsigned short rc[4];

    rc[0] = 0;
    rc[1] = 0;
    rc[2] = 0x1e0;
    rc[3] = 0x200;
    title_0c6d0(0);
    title_0cc70(0);
    title_0cae0(0);
    title_0c670(rc, 0, 0, 0);
    title_0c6d0(0);
}

int title_09980(int a, int b)
{
    TitleCtx *ctx = (TitleCtx *)g_engine_interface.context_004;
    TitleRec16 *rec;
    int r;

    if (ctx->state_006 == 3)
        /* Original falls through with the context still in eax; reproduced explicitly. */
        return (int)ctx;
    g_2a194 = 0;
    g_2a190 = 0;
    if (ctx->flags_030 & 0x10000)
        return -1;
    rec = &g_251e0[a];
    g_253a0 = rec->w8;
    g_2a1a0 = rec->wa - 0xf;
    title_09920();
    title_0ca30();
    g_2a19c = 0;
    g_2a184 = 0;
    g_2a180 = 0;
    g_2a17c = 0;
    g_2a178 = 0;
    title_1d770();
    if (a == 0x15) title_0c9c0(0x1000, 0x4000); else title_0c9c0(8, 0x4000);
    r = title_0c9e0(a, b);
    title_0c9c0(0, 0);
    return r;
}

void title_0a300(void)
{
    switch (g_2a208) {
    case 0:
        if (title_0c4a0(0x12c) < 2) {
            if (g_2a1e8 == 0 || g_2a1e8 == 8)
                g_2a1e8 = 1;
        }
        if (title_0c4a0(0x12c) < 2) {
            if (g_2a1e0 == 0 || g_2a1e0 == 5)
                g_2a1e0 = 1;
        }
        break;
    case 1:
        if (title_0c4a0(0x12c) < 2) {
            if (g_2a1e8 == 0 || g_2a1e8 == 8)
                g_2a1e8 = 1;
        }
        if (title_0c4a0(0xc8) < 2) {
            if (g_2a1c4 == 0 || g_2a1c4 == 3)
                g_2a1c4 = 4;
        }
        if (g_2a1e0 == 0)
            g_2a1e0 = 3;
        break;
    case 2:
        if (title_0c4a0(0x12c) < 2) {
            if (g_2a1e0 == 0 || g_2a1e0 == 5)
                g_2a1e0 = 1;
        }
        if (title_0c4a0(0x12c) < 2) {
            if (g_2a1c4 == 0 || g_2a1c4 == 3)
                g_2a1c4 = 4;
        }
        break;
    }
    title_09a40();
    title_09c90();
    title_0a020();
}

void title_0c140(void)
{
    title_01de0(0x140, 0, g_2a1b8, 0, 0);
    title_02090(0x140, 0x100, g_2a1b8, 3, g_2a220, 0, 0, 0x4ec);
    title_02090(0x140, 0x100, g_2a1b8, 3, g_2a220 + 0x13f, 0, 1, 0x4ec);
    title_01de0(0x280, 0x100, g_2a1b8, 0, 0x4f1);
    g_2a220--;
    if (g_2a220 < -320)
        g_2a220 += 320;
}

