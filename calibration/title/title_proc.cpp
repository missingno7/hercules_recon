/* TITLE process scheduler unit (C++).
   Hypothesis: one object from 0x5cc0 up to 0x6350. Its literals follow the file-name lists in
   .data and its .bss block 0x29e08..0x29fb8 follows the sound-effect unit's. Two variables of that
   block (0x29e08, 0x29f98) are read across the whole DLL, so they are ordinary definitions of a C++
   object: a C file would make them communal (shared .bss tail) and C statics cannot be shared. */
#include <string.h>
#include "title_engine.h"

extern "C" {

#include "title_screen.h"
#include "title_gpu.h"

/* .bss of this unit (only variables used by reconstructed code; layout order is not reproduced). */
struct TitleObject *g_29e08;    /* current object, read across the DLL */
unsigned long g_29e10;          /* callback of the process being run */
TitleProc g_29e18[16];          /* process pool */
int g_29f98;                    /* read across the DLL */
unsigned long g_29f9c;          /* scheduler ticks */
int g_29fa4;

/* Initialized data of this unit (TITLE.DLL .data 0x23270..0x232eb), in address order: frame/delay
   tables read by 0x1a7c0 and 0x8320 (terminated by -1). Each array takes its extent to the next one. */
signed char g_23270[32] = {0, 6, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 3, 7, 14, 6, 3, 5, 3, 4, 3, 3, 3, 2, 3, 1, 3, 0, 6, -1, 0};
signed char g_23290[32] = {0, 0, 1, 0, 8, 1, 9, 1, 10, 1, 11, 1, 12, 1, 13, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 1, -1, 0, 0, 0};
short g_232b0[30] = {11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, -1, 0, 0, 0, 15, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 18, -1, 0};
extern TitleProc *g_2cc20[16];
extern int g_2cc60;
extern int g_2cc68;

void title_0c5b0(void *p);
void title_016e0(const char *format, ...);

/* Steps the current object's animation through a (frame, delay) table ending in -1, and moves it
   by (dx, dy) whole pixels, mirrored in x when flag 0x10 is set. Returns 1 when the table wraps. */
int title_05cc0(int base, signed char *table, int restart, int dx, int dy)
{
    int done = 0;

    if (g_2cc68 <= 0) {
        g_29e08->unknown_034 = table[g_2cc60] + base;
        g_2cc60++;
        g_2cc68 = table[g_2cc60++];
        if (table[g_2cc60] == -1) {
            g_2cc60 = restart;
            done = 1;
        }
    } else {
        g_2cc68--;
    }
    if (g_29e08->unknown_054 & 0x10)
        g_29e08->unknown_000 -= dx << 16;
    else
        g_29e08->unknown_000 += dx << 16;
    g_29e08->unknown_004 += dy << 16;
    return done;
}

void title_05d60(void)
{
    if (g_29fa4 == 1) {
        title_0c5b0(g_2cc04);
        g_29fa4 = 0;
    }
}

void title_05d90(void)
{
    memset(g_2cc20, 0, 0x40);
    memset(g_29e18, 0, 0x180);
}

/* Starts a process: takes the first free pool record, fills it, links it at the head of list `list`
   and runs it once. */
TitleProc *title_05db0(void (*fn)(TitleProc *), unsigned long arg, int list, unsigned long param)
{
    unsigned int i;
    TitleProc *head;
    TitleProc *p;

    head = g_2cc20[list];
    for (i = 0; i < 16; i++) {
        if (g_29e18[i].fn_00 == 0)
            goto found;
    }
    return 0;
found:
    p = &g_29e18[i];
    g_29e18[i].fn_00 = fn;
    g_29e18[i].unknown_14 = arg;
    g_29e18[i].delay_08 = 1;
    g_29e18[i].state_04 = 0xffff;
    g_29e18[i].unknown_0c = param;
    g_29e18[i].next_10 = head;
    g_2cc20[list] = p;
    g_29e10 = (unsigned long)p->fn_00;
    p->fn_00(p);
    return p;
}

void title_05e40(void)
{
    TitleProc **pp;
    TitleProc *o;

    g_29f9c++;
    for (pp = g_2cc20; pp < &g_2cc20[16]; pp++) {
        o = *pp;
        while (o != 0) {
            o->delay_08--;
            if (o->delay_08 == 0) {
                g_29e10 = (unsigned long)o->fn_00;
                o->fn_00(o);
            }
            o = o->next_10;
        }
    }
}

/* Wakes the process running fn with argument arg (its next tick calls it); traces the pool if none. */
void title_05e90(void (*fn)(TitleProc *), unsigned long arg)
{
    unsigned int i;
    TitleProc *s;

    for (i = 0; i < 16; i++) {
        if (g_29e18[i].fn_00 == fn && g_29e18[i].unknown_0c == arg)
            goto found;
    }
    title_016e0("\n Error specified process does not exist ! 0x%x 0x%x ...", fn, arg);
    for (s = g_29e18; s < g_29e18 + 16; s++)
        title_016e0("\n proc %d :: 0x%x 0x%x", s->fn_00, s->unknown_0c);
    return;
found:
    g_29e18[i].delay_08 = 1;
    title_016e0("\n Wake Up procedd");
}

void title_05f10(TitleProc *target)
{
    int i;
    unsigned int j;
    TitleProc *s;

    for (i = 0, s = g_29e18; s < g_29e18 + 16; i++, s++) {
        if (s == target) {
            for (j = 0; j < 16; j++) {
                if (g_2cc20[j] == s) {
                    g_2cc20[j] = g_29e18[i].next_10;
                    g_29e18[i].next_10 = 0;
                    g_29e18[i].fn_00 = 0;
                    return;
                }
            }
            for (j = 0; j < 16; j++) {
                if (g_29e18[j].next_10 == s) {
                    g_29e18[j].next_10 = g_29e18[i].next_10;
                    g_29e18[i].next_10 = 0;
                    g_29e18[i].fn_00 = 0;
                    return;
                }
            }
        }
    }
}

static int g_29fa8;
static int g_29fac;
static int g_29fb0;
static int g_29fb4;
static int g_29fb8;
extern int g_2cc0c;
extern volatile int g_2a244;   /* volatile: owner-approved codegen hypothesis (0x5fb0) */

void title_164b0(TitleProc *p);
int title_0c4b0(int a);
void title_0c3b0(void);
void title_01dd0(void);
void title_095d0(void);
void title_1d380(void);
void title_085b0(void);
void title_1d590(void);
int title_0c540(int a);
void title_04b70(int a, int b);
int title_17ad0(int a, int b, int c, int d, int e);
int title_0cb60(int a);
void title_0c470(void);
void title_04660(void);
void title_0c2f0(void);
void title_085d0(void);
void title_04b00(void);
void title_08770(void);
void title_04aa0(void);
void title_028c0(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int k);
void title_02b70(void);
int title_0c6d0(int a);
int title_0cc80(int a);
int title_0cc70(int a);
void title_0c4c0(void);
void title_041d0(void);
void title_04870(void);
int title_0c960(int a);
int title_0c6b0(unsigned long *ot);

#define CTX (g_engine_interface.context_004)

void title_05fb0(void)
{
    TitleBuffer *t;

    switch (g_29fb8) {
    case 0:
        g_29fb4 = 0;
        CTX->unknown_000[1] = 0;
        CTX->unknown_000[2] = 0;
        CTX->unknown_000[10] = 4;
        memcpy(&CTX->unknown_08c, "i2.1", 4);
        memcpy(&CTX->unknown_094, "i1.4", 4);
        title_0c4b0(0);
        title_0c3b0();
        title_01dd0();
        title_095d0();
        title_1d380();
        title_085b0();
        CTX->unknown_03c = g_2a244;
        CTX->unknown_040 = 0;
        title_1d590();
        title_05d90();
        title_05db0(title_164b0, 0, 0, 0);
        title_0c540(1);
        title_04b70(0xe, 0);
        g_29fa8 = title_17ad0(0, 0, 0, 0x2014, 0);
        ((TitleObject *)g_29fa8)->unknown_034 = 1;
        ((TitleObject *)g_29fa8)->unknown_023 = 6;
        ((TitleObject *)g_29fa8)->unknown_03e = 0;
        ((TitleObject *)g_29fa8)->unknown_054 &= 0x7fffffff;
        ((TitleObject *)g_29fa8)->unknown_054 |= 5;
        g_29fac = title_17ad0(0, 0, 0, 0x2014, 0);
        ((TitleObject *)g_29fac)->unknown_034 = 1;
        ((TitleObject *)g_29fac)->unknown_023 = 6;
        ((TitleObject *)g_29fac)->unknown_03e = 1;
        ((TitleObject *)g_29fac)->unknown_054 &= 0x7fffffff;
        ((TitleObject *)g_29fac)->unknown_000 += 0x20000;
        ((TitleObject *)g_29fac)->unknown_004 += 0x20000;
        ((TitleObject *)g_29fac)->unknown_04a = 0;
        ((TitleObject *)g_29fa8)->unknown_004 += 0xffc80000;
        ((TitleObject *)g_29fac)->unknown_004 += 0xffc80000;
        g_29fb8 = 1;
    case 1:
        break;
    default:
        return;
    }
    CTX->unknown_034 = title_0cb60((int)g_engine_interface.data_010 + 0x400);
    title_0c470();
    title_04660();
    title_0c2f0();
    title_085d0();
    title_04b00();
    title_08770();
    title_04aa0();
    title_0cb60(CTX->unknown_034);
    if (*(unsigned short *)&CTX->unknown_058[0x60 - 0x58] != 0) {
        if ((CTX->unknown_128 & 0x0a) == 0) {
            g_29fb0 += 1;
            if (g_29fb0 > 2) {
                ((TitleObject *)g_29fa8)->unknown_054 |= 0x80000000;
                ((TitleObject *)g_29fac)->unknown_054 |= 0x80000000;
            }
        }
    } else {
        g_29fb0 = 0;
        ((TitleObject *)g_29fa8)->unknown_054 &= 0x7fffffff;
        ((TitleObject *)g_29fac)->unknown_054 &= 0x7fffffff;
    }
    title_05e40();
    if (g_29fb0 > 2) {
        title_028c0(0x26, 0x64, 0xe6, 0x5c, 0x80, 0x80, 0x80, 0x40, 2, 3, 0x4fc);
    }
    CTX->unknown_034 = title_0cb60((int)g_engine_interface.data_010 + 0x400);
    title_02b70();
    title_0cb60(CTX->unknown_034);
    title_0c6d0(0);
    g_29fa4 = 1;
    g_2cc0c = title_0cc80((int)title_05d60);
    title_0cc70(0);
    title_0cc80(g_2cc0c);
    title_0c4c0();
    CTX->unknown_03c = g_2a244;
    CTX->unknown_054 = title_0cc70(1);
    CTX->unknown_034 = title_0cb60((int)g_engine_interface.data_010 + 0x400);
    title_041d0();
    title_04870();
    title_0cb60(CTX->unknown_034);
    title_0c960(0x80);
    title_0c6b0(g_2cc04->ot);
    t = g_2cc08;
    if (g_2cc04 == t) {
        t++;
    }
    g_2cc04 = t;
    CTX->tick_038 += 1;
}

} /* extern "C" */
