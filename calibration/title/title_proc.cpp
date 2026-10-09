/* TITLE process scheduler unit (C++).
   Hypothesis: one object from 0x5cc0 up to 0x6350. Its literals follow the file-name lists in
   .data and its .bss block 0x29e08..0x29fb8 follows the sound-effect unit's. Two variables of that
   block (0x29e08, 0x29f98) are read across the whole DLL, so they are ordinary definitions of a C++
   object: a C file would make them communal (shared .bss tail) and C statics cannot be shared. */
#include <string.h>

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

} /* extern "C" */
