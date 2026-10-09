/* Sequence screen 2 unit: handler 0x12e90 and helpers 0x13310, 0x13430.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"


extern int g_2ab2c;
extern int g_2ab30;
extern int g_2ab38;
extern int g_2ab3c;
extern int g_2ab40;
extern TitleObject *g_2ab1c;
extern TitleObject *g_2ab24;
extern TitleObject *g_2ab34;
static int g_2ab88;
static int g_2ab8c;
static int g_2ab90;
static int g_2ab94;
static int g_2ab9c;
static TitleObject *g_2ab78;
static TitleObject *g_2ab80;
static TitleObject *g_2ab98;
extern int g_29f98;
extern char g_29128[];
extern void title_164b0(void);
void title_01dd0();
void title_01de0();
void title_04b70(int c, int d);
void title_04ea0(int a);
void title_05e90();
void title_05f10(void *target);
void title_09350(void *block);
int title_0c3f0(char *a, char *name, char *c, int d);
void title_0c450(void **out, int size, int flags);
void title_0c4f0(int a);
void title_0c330(void *value);
void title_0c6d0(int a);
void title_0c8b0(char *a1, int a2, int a3, int a4, int a5, int a6, int a7);
void title_12710(void);
void title_12810(void);
void title_13310(void);
void title_13430(void);
void title_16300(void);
void title_1d790(int a);
extern TitleObject *g_2ab48;
extern TitleObject *g_2ab54;
extern TitleObject *g_2ab64;
extern TitleObject *g_2aba4;
extern TitleObject *g_2abac;
extern TitleObject *g_2abc0;
extern int g_2ab4c;
extern int g_2ab58;
extern int g_2ab68;
extern int g_2ab74;
static int g_2ab7c;
static int g_2ab84;
static int g_2aba0;
extern int g_2aba8;
extern int g_2abb0;
extern int g_2abc4;
extern int g_2abcc;
extern unsigned short g_25f50[];

void title_12e90(TitleProc *self)
{
    void *handle;
    short cursor_y = ((ScreenContext *)g_engine_interface.context_004)->cursor_5e;

    switch (self->state_04) {
    case 0xfffd:
        title_05e90(title_164b0, 0);
        title_05f10(self);
        return;
    case 0xfffe:
        title_01dd0();
        self->state_04 = 0xfffd;
        self->delay_08 = 2;
        return;
    case 0xffff:
        g_2ab88 = 0;
        g_2ab90 = 0xf;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ2], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(5, 0);
        title_13310();
        title_1d790(1);
        g_2ab8c = 0x80;
        g_2ab94 = 0;
        g_2ab9c = 0xa;
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        self->state_04 = 2;
        self->delay_08 = 1;
        return;
    case 2:
        break;
    default:
        return;
    }

    switch (g_2ab9c) {
    case 0:
        title_01de0(0x140, 0x100, g_2ab94, 0);
        if (g_2ab94 < 0x80) {
            g_2ab94 += 8;
        }
        break;
    case 1:
        title_01de0(0x140, 0, g_2ab8c, 0, 0);
        title_01de0(0x140, 0x100, g_2ab94, 1, 1);
        if (g_2ab8c < 0x80) {
            g_2ab8c += 8;
        }
        if (g_2ab94 > 0) {
            g_2ab94 -= 8;
        }
        break;
    case 2:
        title_0c4f0(1);
        g_2ab9c = 3;
        /* fall through */
    case 3:
        title_01de0(0x140, 0, g_2ab8c, 0, 0);
        if (g_2ab8c > 0) {
            g_2ab8c -= 4;
        }
        break;
    case 11:
        if (g_2ab78->unknown_04a < 0x80) {
            g_2ab78->unknown_04a += 8;
            g_2ab98->unknown_04a = g_2ab78->unknown_04a * 2;
        }
        /* fall through */
    case 10:
        title_01de0(0x280, 0x100, 0x80, 0, 0);
        break;
    case 12:
        title_01de0(0x280, 0x100, g_2ab8c, 0, 0);
        g_2ab80->unknown_04a = g_2ab8c;
        g_2ab78->unknown_04a = g_2ab8c;
        g_2ab98->unknown_04a = g_2ab8c * 2;
        if (g_2ab8c > 0) {
            g_2ab8c -= 8;
        } else {
            g_2ab80->unknown_054 &= 0x7fffffff;
            g_2ab78->unknown_054 &= 0x7fffffff;
            g_2ab9c = 0;
        }
        break;
    }

    if (g_2ab9c == 0xc) {
        if (g_2ab80 != 0) {
            title_09350(g_2ab80);
            g_2ab80 = 0;
        }
        if (g_2ab78 != 0) {
            title_09350(g_2ab78);
            g_2ab78 = 0;
        }
        if (g_2ab98 != 0) {
            title_09350(g_2ab98);
            g_2ab98 = 0;
        }
        title_04ea0(5);
        title_16300();
        self->state_04 = 0xfffe;
        self->delay_08 = 2;
        return;
    }
    if (g_29f98 == 0) {
        title_13430();
    }
    self->state_04 = 2;
    self->delay_08 = 1;
}

void title_13310(void)
{
    g_2ab78 = title_17ad0(0, 0, 0, 0x200a, 0);
    g_2ab78->unknown_034 = 0x10;
    g_2ab78->unknown_054 |= 5;
    g_2ab78->unknown_04a = 0;
    g_2ab98 = title_17ad0(0, 0, 0, 0x200a, 0);
    g_2ab98->unknown_034 = 0x10;
    g_2ab98->unknown_054 |= 6;
    g_2ab98->unknown_04a = 0;
    g_2ab78->unknown_023 = 6;
    g_2ab78->unknown_03e = 0x0a;
    g_2ab98->unknown_023 = 6;
    g_2ab98->unknown_03e = 0x14;
    g_2ab78->unknown_000 = 0xfff60000;
    g_2ab78->unknown_004 = 0xffe80000;
    g_2ab98->unknown_000 = 0xfff70000;
    g_2ab98->unknown_004 = 0xffe90000;
    g_2ab80 = title_17ad0(0, 0, 0, 0x200a, 0);
    g_2ab80->unknown_034 = 1;
    g_2ab80->unknown_054 |= 0x10;
    g_2ab80->unknown_000 = 0x640000;
    g_2ab80->unknown_004 = 0x640000;
    g_2ab84 = 0;
    g_2ab7c = 0;
}

void title_13430(void)
{
    g_2aba0 ^= 1;
    if (g_2aba0 != 0) {
        return;
    }
    switch (g_2ab84) {
    case 0:
        g_2ab80->unknown_034 = g_25f50[g_2ab7c];
        g_2ab7c++;
        if (g_25f50[g_2ab7c] == 0x0a && g_2ab9c == 0x0a) {
            g_2ab9c = 0x0b;
        }
        if (g_25f50[g_2ab7c] == 0xffff) {
            title_0c4f0(1);
            g_engine_interface.context_004->unknown_1b30 = 4;
            g_2ab84 = 1;
        }
        return;
    case 1:
        g_2ab80->unknown_034++;
        if (g_2ab80->unknown_034 == 0x0f) {
            g_2ab84 = 2;
        }
        return;
    case 2:
        g_2ab9c = 0x0c;
        g_2ab84 = 3;
        return;
    }
}
