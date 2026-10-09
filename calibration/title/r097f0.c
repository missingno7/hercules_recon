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

typedef struct TitleRec16 {         /* movie: file, frame size, start frame, length, flags */
    char *name_000;
    unsigned short width_004;
    unsigned short height_006;
    unsigned short w8;
    unsigned short wa;
    unsigned short flags_00c;
    unsigned short unknown_00e;
} TitleRec16;

/* Movie table (TITLE.DLL .data 0x251e0..0x253a3); the file names are its literals (reverse order). */
TitleRec16 g_251e0[28] = {
    {"\\MV\\M1.;1", 320, 240, 1, 495, 0x8fff, 0},
    {"\\MV\\M1.;1", 320, 240, 1, 104, 0xffff, 0},
    {"\\MV\\M2.;1", 320, 240, 1, 233, 0xffff, 0},
    {"\\MV\\M3.;1", 320, 240, 1, 105, 0xffff, 0},
    {"\\MV\\M4.;1", 320, 240, 1, 165, 0xcfff, 0},
    {"\\MV\\M5.;1", 320, 240, 1, 148, 0xcfff, 0},
    {"\\MV\\M6.;1", 320, 240, 1, 141, 0xcfff, 0},
    {"\\MV\\M7.;1", 320, 240, 1, 148, 0xefff, 0},
    {"\\MV\\M8.;1", 320, 240, 1, 115, 0xcfff, 0},
    {"\\MV\\M9.;1", 320, 240, 1, 295, 0xcfff, 0},
    {"\\MV\\M10.;1", 320, 240, 1, 166, 0xcfff, 0},
    {"\\MV\\MT3.;1", 320, 240, 1, 2193, 0xcfff, 0},
    {"\\MV\\M11.;1", 320, 240, 1, 298, 0xcfff, 0},
    {"\\MV\\M12.;1", 320, 240, 1, 528, 0xcfff, 0},
    {"\\MV\\M13.;1", 320, 240, 1, 418, 0xcfff, 0},
    {"\\MV\\M14.;1", 320, 240, 1, 1846, 0xcfff, 0},
    {"\\MV\\M15.;1", 320, 240, 1, 348, 0xcfff, 0},
    {"\\MV\\M16.;1", 320, 240, 1, 1045, 0xcfff, 0},
    {"\\MV\\M17.;1", 320, 240, 1, 1093, 0xcfff, 0},
    {"\\MV\\M18.;1", 320, 240, 1, 797, 0xcfff, 0},
    {"\\MV\\M19.;1", 320, 240, 1, 174, 0xcfff, 0},
    {"\\MV\\M20.;1", 320, 240, 1, 66, 0xcfff, 0},
    {"\\MV\\M21.;1", 320, 240, 1, 270, 0xcfff, 0},
    {"\\MV\\M22.;1", 320, 240, 1, 60, 0xcfff, 0},
    {"\\MV\\M23.;1", 320, 240, 1, 82, 0xcfff, 0},
    {"\\MV\\M24.;1", 320, 240, 1, 421, 0xcfff, 0},
    {"\\MV\\M25.;1", 320, 240, 1, 280, 0x8fff, 0},
    {"\\MV\\M26.;1", 320, 240, 1, 927, 0xcfff, 0},
};
int g_253a0 = 1;
static int g_2a178;
static int g_2a17c;
static int g_2a180;
static int g_2a184;
static int g_2a190;
static int g_2a194;
static int g_2a19c;
static int g_2a1a0;
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

