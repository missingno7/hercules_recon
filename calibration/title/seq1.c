/* Sequence screen 1 unit: handler 0x12280 and helpers 0x12710, 0x12810.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"

static int g_2ab2c;
static int g_2ab30;
static int g_2ab38;
static int g_2ab3c;
static int g_2ab40;
static TitleObject *g_2ab1c;
static TitleObject *g_2ab24;
static TitleObject *g_2ab34;
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
static int g_2ab20;
static int g_2ab28;
static int g_2ab44;
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
