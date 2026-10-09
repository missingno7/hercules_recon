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
    unsigned long unknown_00;
    unsigned long unknown_04;
    short id_08;
    short unknown_0a;
    unsigned short word_0c;
    unsigned char unknown_0e[0x30 - 0x0e];
    unsigned long owner_30;
    unsigned long flags_34;
    unsigned long unknown_38;
    void *ptr_3c;
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
extern TitleRec64 g_2cca0[24];
extern unsigned long g_29e18[];
extern unsigned long g_29f9c;
extern unsigned long g_29e10;
extern TitleObj *g_2cc20[];


extern unsigned long g_29dac;
extern unsigned long g_29db8;

extern char g_29f98[];

extern char g_23318[];
extern char g_23300[];
extern char g_232ec[];

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


int title_05080(int id)
{
    int count = 0;
    int i;

    for (i = 0; i < 24; i++) {
        if (g_2cca0[i].ptr_3c != 0) {
            if (g_2cca0[i].id_08 == id) {
                count++;
            }
        }
    }
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
    unsigned long mask;

    if (arg == 0) {
        return;
    }
    mask = 0x8000;
    title_0c6f0();
    title_016e0("\n RemoveFxLinks(0x%x)", arg);
    for (i = 0; i < 24; i++) {
        if (g_2cca0[i].owner_30 == (unsigned long)arg) {
            if ((g_2cca0[i].flags_34 & mask) == 0) {
                title_0cc00(i);
            }
            g_2cca0[i].owner_30 = 0;
        }
    }
    title_0c700();
}

/* ---- f_5c60 ---- */

void title_0c5e0(void);
void title_0c550(int a);
void title_050b0(void);

void title_05c60(int a)
{
    int i;

    title_0c5e0();
    g_29dac = 0;
    g_29db8 = 0;
    if (a == 0) {
        title_0c550((int)title_050b0);
    } else {
        title_0c550(0);
    }
    for (i = 0; i < 24; i++) {
        g_2cca0[i].word_0c = 0xff;
        g_2cca0[i].owner_30 = 0;
    }
}
