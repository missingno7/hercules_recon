/* Sequence screen 1 unit: handler 0x12280 and helpers 0x12710, 0x12810.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"

typedef struct TitleRect {
    int a;
    int b;
    int c;
    int d;
} TitleRect;
typedef struct TitleCtxView {
    signed char kind_00;
    unsigned char unknown_01[0x0b - 0x01];
    signed char field_0b;
    signed char field_0c;
    unsigned char unknown_0d;
    signed char field_0e;
    unsigned char unknown_0f;
    unsigned char field_10;
    unsigned char field_11;
    unsigned char unknown_12[0x30 - 0x12];
    int field_30;
    unsigned char unknown_34[0x6c - 0x34];
    TitleRect rect_6c;
    unsigned char unknown_7c[0x1b0c - 0x7c];
    int field_1b0c;
    int field_1b10;
    unsigned char unknown_1b14[0x1b24 - 0x1b14];
    int field_1b24;
} TitleCtxView;
typedef struct TitleObj {
    unsigned char field_00;
    unsigned char field_01;
    unsigned char field_02;
    unsigned char field_03;
    int field_04;
    int field_08;
    int field_0c;
    int field_10;
    int field_14;
    int field_18;
    int field_1c;
    TitleRect rect_20;
    int field_30;
    int field_34;
    unsigned char field_38;
    unsigned char field_39;
    unsigned char field_3a;
    unsigned char field_3b;
} TitleObj;

extern int g_2ab2c;
extern int g_2ab30;
extern int g_2ab38;
extern int g_2ab3c;
extern int g_2ab40;
extern TitleObject *g_2ab1c;
extern TitleObject *g_2ab24;
extern TitleObject *g_2ab34;
extern int g_2ab88;
extern int g_2ab8c;
extern int g_2ab90;
extern int g_2ab94;
extern int g_2ab9c;
extern TitleObject *g_2ab78;
extern TitleObject *g_2ab80;
extern TitleObject *g_2ab98;
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
extern TitleObject *g_2a9c8;
extern TitleObject *g_2a9cc;
extern TitleObject *g_2a9d0;
extern int g_2aa54;
extern int g_2aa60;
extern int g_2aa90;
extern int g_2aacc;
extern int g_2a9d8;
extern int g_2a9dc;
extern int g_2a9e4;
extern int g_2aa28;
extern int g_2aa44;
extern int g_2ab08;
extern int g_2ab20;
extern int g_2ab28;
extern int g_2ab44;
extern unsigned short g_25f50[];
int title_06670(void);
void title_0c2e0(void);

void title_12280(TitleProc *self)
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
        g_2ab38 = 0xa;
        g_2ab30 = 0;
        g_2ab2c = 0xf;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ1], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(4, 0);
        title_12710();
        title_1d790(1);
        g_2ab40 = 0x80;
        g_2ab3c = 0;
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

    switch (g_2ab38) {
    case 0:
        title_01de0(0x140, 0x100, g_2ab3c, 0, 0);
        if (g_2ab3c < 0x80) {
            g_2ab3c += 8;
        }
        break;
    case 1:
        g_2ab24->unknown_054 &= 0x7fffffff;
        g_2ab1c->unknown_054 &= 0x7fffffff;
        title_01de0(0x140, 0, g_2ab40, 0, 0);
        title_01de0(0x140, 0x100, g_2ab3c, 1, 1);
        if (g_2ab40 < 0x80) {
            g_2ab40 += 8;
        }
        if (g_2ab3c > 0) {
            g_2ab3c -= 8;
        }
        break;
    case 2:
        title_0c4f0(1);
        g_2ab38 = 3;
        /* fall through */
    case 3:
        title_01de0(0x140, 0, g_2ab40, 0, 0);
        if (g_2ab40 > 0) {
            g_2ab40 -= 4;
        }
        break;
    case 11:
        if (g_2ab1c->unknown_04a < 0x80) {
            g_2ab1c->unknown_04a += 8;
            g_2ab34->unknown_04a = g_2ab1c->unknown_04a * 2;
        }
        /* fall through */
    case 10:
        title_01de0(0x280, 0x100, 0x80, 0, 0);
        break;
    case 12:
        title_01de0(0x280, 0x100, g_2ab40, 0, 0);
        g_2ab24->unknown_04a = g_2ab40;
        g_2ab1c->unknown_04a = g_2ab40;
        g_2ab34->unknown_04a = g_2ab40 * 2;
        if (g_2ab40 > 0) {
            g_2ab40 -= 8;
        } else {
            g_2ab24->unknown_054 &= 0x7fffffff;
            g_2ab1c->unknown_054 &= 0x7fffffff;
            g_2ab38 = 0;
        }
        break;
    }

    if (g_29f98 == 0) {
        title_12810();
    }
    if (g_2ab38 == 0xc) {
        if (g_2ab1c != 0) {
            title_09350(g_2ab1c);
            g_2ab1c = 0;
        }
        if (g_2ab34 != 0) {
            title_09350(g_2ab34);
            g_2ab34 = 0;
        }
        if (g_2ab24 != 0) {
            title_09350(g_2ab24);
            g_2ab24 = 0;
        }
        title_04ea0(4);
        title_16300();
        self->state_04 = 0xfffe;
        self->delay_08 = 2;
        return;
    }
    self->state_04 = 2;
    self->delay_08 = 1;
}

void title_12710(void)
{
    g_2ab1c = title_17ad0(0, 0, 0, 0x2009, 0);
    g_2ab1c->unknown_034 = 1;
    g_2ab34 = title_17ad0(0, 0, 0, 0x2009, 0);
    g_2ab34->unknown_034 = 1;
    g_2ab1c->unknown_054 |= 5;
    g_2ab34->unknown_054 |= 6;
    g_2ab34->unknown_04a = 0;
    g_2ab1c->unknown_04a = 0;
    g_2ab34->unknown_023 = 6;
    g_2ab1c->unknown_023 = 6;
    g_2ab1c->unknown_03e = 10;
    g_2ab34->unknown_03e = 20;
    g_2ab34->unknown_000 = 0x10000;
    g_2ab34->unknown_004 = 0x10000;
    g_2ab24 = title_17ad0(0, 0, 0, 0x2009, 0);
    g_2ab24->unknown_034 = 2;
    g_2ab24->unknown_000 = 0x640000;
    g_2ab24->unknown_004 = 0x640000;
    g_2ab24->unknown_054 |= 0x10;
    g_2ab28 = 0;
    g_2ab20 = 0;
}

void title_12810(void)
{
    g_2ab44 ^= 1;
    if (g_2ab44 != 0) {
        return;
    }
    switch (g_2ab28) {
    case 0:
        g_2ab24->unknown_034 = g_25f50[g_2ab20] + 1;
        g_2ab20++;
        if (g_25f50[g_2ab20] == 0x0a && g_2ab38 == 0x0a) {
            g_2ab38 = 0x0b;
        }
        if (g_25f50[g_2ab20] == 0xffff) {
            title_0c4f0(1);
            g_engine_interface.context_004->unknown_1b30 = 4;
            g_2ab28 = 1;
        }
        break;
    case 1:
        g_2ab24->unknown_034++;
        if (g_2ab24->unknown_034 == 0x10) {
            g_2ab28 = 2;
        }
        break;
    case 2:
        g_2ab38 = 0x0c;
        g_2ab28 = 3;
        break;
    }
}
