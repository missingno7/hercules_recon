/* Lane w07 region: MASKED EQUAL candidates in ascending RVA order (scratch). */
#include <string.h>
/* Lane w07 extra declarations (scratch). */
#ifndef TITLE_W07_COMMON2_H
#define TITLE_W07_COMMON2_H

/* Lane w07 shared declarations (scratch). */
#ifndef TITLE_W07_COMMON_H
#define TITLE_W07_COMMON_H

#include "title_engine.h"

typedef struct TitleSlot {
    void *ptr_00;
    unsigned short count_04;
    unsigned short unknown_06;
    void *ptr_08;
    void *ptr_0c;
    unsigned short state_10;
    unsigned short state_12;
} TitleSlot;

typedef struct TitlePair {
    unsigned short lo;
    unsigned short hi;
} TitlePair;

typedef struct TitleCtxView {
    unsigned char pad_000[0x38];
    unsigned long field_38;
    unsigned char pad_3c[0x9c - 0x3c];
    unsigned short field_9c;
    short field_9e;
    short field_a0;
    unsigned char pad_a2[0xb0 - 0xa2];
    unsigned long field_b0;
    unsigned long field_b4;
    unsigned char pad_b8[0xdf - 0xb8];
    unsigned char field_df;
} TitleCtxView;

extern int g_22148;
extern TitleSlot g_26110[];
extern TitleSlot *g_2d324;
extern int g_29d98;
extern int g_29d9c;
extern char g_2214c[];
extern char g_2215c[];

#endif

typedef struct TitleRec64 {
    short id_00;
    unsigned char unknown_02[0x32];
    void *ptr_34;
    unsigned char unknown_38[0x40 - 0x38];
} TitleRec64;

typedef struct TitleObj TitleObj;
struct TitleObj {
    void (*fn_00)(void *);
    unsigned long unknown_04;
    long refs_08;
    unsigned long unknown_0c;
    TitleObj *next_10;
    unsigned long unknown_14;
};

extern int g_29fa4;
extern int g_2cc04;
extern TitleRec64 g_2cca8[];
extern unsigned long g_29e18[];
extern unsigned long g_29f9c;
extern unsigned long g_29e10;
extern TitleObj *g_2cc20[];

typedef struct TitleCell12 {
    unsigned short w0_00;
    unsigned short w2_02;
    unsigned char unknown_04[8];
} TitleCell12;

extern TitleCell12 g_2d340[];
extern unsigned short g_2d400;
extern unsigned short g_2d370;
extern unsigned short g_2d34c;

extern char g_2d2d0[];
extern char g_221c4[];
extern unsigned long g_29dac;
extern unsigned long g_29db8;

extern char g_29f98[];

extern char g_23318[];
extern char g_23300[];
extern char g_232ec[];

extern char g_2d2ae[];
extern char g_221f8[];

typedef struct TitleCursor {
    int pos_00;
    int pos_04;
    unsigned char unknown_08[0x34 - 8];
    short off_34;
    unsigned char unknown_36[0x54 - 0x36];
    unsigned char flags_54;
} TitleCursor;

extern TitleCursor *g_29e08;
extern int g_2cc60;
extern int g_2cc68;

extern char g_221dc[];

#endif

extern TitleRec64 g_2d2a8[];

/* ---- f_4d80 ---- */

void title_04b50(int a);
int title_04dd0(int idx, int start);

int title_04d80(int idx, int arg2)
{
    unsigned int st = g_26110[idx].state_10;

    if (st == 2) {
        return title_04dd0(idx, arg2);
    }
    if (st == 0) {
        title_04b50(idx);
        g_26110[idx].state_12 = 3;
    }
    return 0;
}

/* ---- f_4e40 ---- */

void title_04e40(void)
{
    int i = g_22148;
    if (g_26110[i].state_10 == 2) {
        g_26110[i].state_10 = 3;
    }
}

/* ---- f_4e60 ---- */

void title_04b50(int a);
void title_04ea0(int a);
void title_04f00(int a);

void title_04e60(void)
{
    title_04ea0(g_22148);
    if (g_26110[g_22148].state_10 == 4) {
        title_04f00(g_22148);
        title_04b50(g_22148);
    }
}

/* ---- f_4ea0 ---- */

void title_0c2d0(void);

void title_04ea0(int idx)
{
    switch (g_26110[idx].state_10) {
    case 1:
        g_26110[idx].state_10 = 4;
        break;
    case 2:
        ((TitleCtxView *)g_engine_interface.context_004)->field_b0 = 0;
        ((TitleCtxView *)g_engine_interface.context_004)->field_b4 = 0;
        title_0c2d0();
        g_26110[idx].state_10 = 4;
        break;
    case 3:
        g_26110[idx].state_10 = 4;
        break;
    }
}

/* ---- f_4f00 ---- */

int title_0c330(int a);

void title_04f00(int idx)
{
    if (g_26110[idx].ptr_0c != 0) {
        title_0c330((int)g_26110[idx].ptr_0c);
        g_26110[idx].ptr_0c = 0;
    }
    g_26110[idx].state_10 = 0;
    g_26110[idx].state_12 = 0;
}

/* ---- f_4f40 ---- */

void title_04b00(void);
void title_04ea0(int idx);

void title_04f40(const unsigned char *a, const unsigned char *b)
{
    int i;
    unsigned char c;
    const unsigned char *p;

    for (i = 0; i < 18; i++) {
        if (g_26110[i].state_10 != 0) {
            if (a != 0) {
                c = *a;
                p = a + 1;
                while (c != 0xff) {
                    if (i == c) {
                        goto next;
                    }
                    c = *p;
                    p++;
                }
            }
            if (b != 0) {
                c = *b;
                p = b + 1;
                while (c != 0xff) {
                    if (i == c) {
                        goto next;
                    }
                    c = *p;
                    p++;
                }
            }
            title_04ea0(i);
        }
    next:
        ;
    }
    title_04b00();
}

/* ---- f_5080 ---- */

extern TitleRec64 g_2d2a8[];

int title_05080(int id)
{
    int count = 0;
    TitleRec64 *p;

    p = g_2cca8;
    do {
        if (p->ptr_34 != 0) {
            if (p->id_00 == id) {
                count++;
            }
        }
        p++;
    } while ((int)p < (int)g_2d2a8);
    return count;
}

/* ---- f_5a70 ---- */

void title_0c6f0(void);
void title_0c700(void);
void title_0cc00(int a);
int title_016e0(const char *s, int a);

void title_05a70(int arg)
{
    int i;
    unsigned long *p;
    unsigned long mask;

    if (arg == 0) {
        return;
    }
    mask = 0x8000;
    title_0c6f0();
    title_016e0(g_221c4, arg);
    i = 0;
    p = (unsigned long *)((char *)g_2cca8 + 0x28);
    do {
        if (p[0] == (unsigned long)arg) {
            if ((p[1] & mask) == 0) {
                title_0cc00(i);
            }
            p[0] = 0;
        }
        p += 16;
        i++;
    } while ((int)p < (int)g_2d2d0);
    title_0c700();
}

/* ---- f_5c60 ---- */

void title_0c5e0(void);
void title_0c550(int a);
void title_050b0(void);

void title_05c60(int a)
{
    char *p;

    title_0c5e0();
    g_29dac = 0;
    g_29db8 = 0;
    if (a == 0) {
        title_0c550((int)title_050b0);
    } else {
        title_0c550(0);
    }
    p = (char *)g_2cca8 + 0x28;
    do {
        *(unsigned short *)(p - 0x24) = 0xff;
        *(unsigned long *)p = 0;
        p += 0x40;
    } while ((int)p < (int)g_2d2d0);
}

/* ---- f_5d60 ---- */

int title_0c5b0(int a);

void title_05d60(void)
{
    if (g_29fa4 == 1) {
        title_0c5b0(g_2cc04);
        g_29fa4 = 0;
    }
}

/* ---- f_5d90 ---- */

void title_05d90(void)
{
    memset(g_2cc20, 0, 0x40);
    memset(g_29e18, 0, 0x180);
}

/* ---- f_5e40 ---- */

void title_05e40(void)
{
    TitleObj **pp;
    TitleObj *o;

    g_29f9c++;
    for (pp = g_2cc20; pp < &g_2cc20[16]; pp++) {
        o = *pp;
        while (o != 0) {
            o->refs_08--;
            if (o->refs_08 == 0) {
                g_29e10 = (unsigned long)o->fn_00;
                o->fn_00(o);
            }
            o = o->next_10;
        }
    }
}

