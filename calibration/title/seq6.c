/* Sequence screen 6 unit: handler 0x14970 and helpers 0x14e10, 0x14f20.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"


static TitleObject *g_2ac2c;
static TitleObject *g_2ac34;
static TitleObject *g_2ac48;
static int g_2ac3c;
static int g_2ac40;
static int g_2ac44;
static int g_2ac4c;
static int g_2ac50;
extern int g_29f98;
extern char g_29128[];
void title_01dd0();
void title_02350(void);
void title_04b70(int c, int d);
void title_04ea0(int a);
void title_05e90(void (*fn)(void *), int n);
void title_05f10(TitleProc *req);
void title_09350(void *block);
void title_0c330(void *p);
void title_0c450(void **out, int size, int flags);
int title_0c4f0(int a);
void title_0c6d0(int a);
void title_0c8b0(char *a0, int a1, int a2, int a3, int a4, int a5, int a6);
int title_0c3f0(char *a, char *name, char *c, int d);
void title_139d0(void);
void title_13ad0(void);
void title_14e10(void);
void title_14f20(void);
void title_164b0(void *req);
void title_16300(void);
void title_1d790(int a);
void title_01de0(int a0, int a1, int a2, int a3, int a4);
extern unsigned short g_25f50[];
static int g_2ac30;
static int g_2ac38;
static int g_2ac54;
int title_0c4f0(int a);

void title_14970(TitleProc *self)
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
        g_2ac3c = 0;
        g_2ac50 = 0x0f;
        g_2ac4c = 0;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ6], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(9, 0);
        title_14e10();
        title_1d790(1);
        g_2ac40 = 0x80;
        g_2ac44 = 0;
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        break;
    case 2:
        switch (g_2ac4c) {
        case 0:
            title_01de0(0x140, 0x100, g_2ac44, 0, 0);
            if (g_2ac44 < 0x80) {
                g_2ac44 += 8;
            }
            break;
        case 1:
            title_01de0(0x140, 0, g_2ac40, 0, 0);
            title_01de0(0x140, 0x100, g_2ac44, 1, 1);
            if (g_2ac40 < 0x80) {
                g_2ac40 += 8;
            } else {
                g_2ac40 = 0x80;
            }
            if (g_2ac44 > 0) {
                g_2ac44 -= 8;
            } else {
                g_2ac44 = 0;
            }
            break;
        case 2:
            title_0c4f0(1);
            g_2ac4c = 3;
            /* fall through */
        case 3:
            title_01de0(0x140, 0, g_2ac40, 0, 0);
            if (g_2ac40 > 0) {
                g_2ac40 -= 4;
            }
            break;
        case 11:
            if (g_2ac2c->unknown_04a < 0x80) {
                g_2ac2c->unknown_04a += 8;
            }
            g_2ac48->unknown_04a = g_2ac2c->unknown_04a * 2;
            /* fall through */
        case 10:
            title_01de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_01de0(0x280, 0x100, g_2ac40, 0, 0);
            g_2ac34->unknown_04a = (unsigned short)g_2ac40;
            g_2ac2c->unknown_04a = (unsigned short)g_2ac40;
            g_2ac48->unknown_04a = (unsigned short)(g_2ac40 * 2);
            if (g_2ac40 > 0) {
                g_2ac40 -= 8;
            } else {
                g_2ac34->unknown_054 &= 0x7fffffff;
                g_2ac2c->unknown_054 &= 0x7fffffff;
                g_2ac48->unknown_054 &= 0x7fffffff;
                g_2ac4c = 0;
            }
            break;
        default:
            break;
        }
        if (g_2ac4c == 0x0c) {
            if (g_2ac34 != 0) {
                title_09350(g_2ac34);
                g_2ac34 = 0;
            }
            if (g_2ac2c != 0) {
                title_09350(g_2ac2c);
                g_2ac2c = 0;
            }
            if (g_2ac48 != 0) {
                title_09350(g_2ac48);
                g_2ac48 = 0;
            }
            title_04ea0(9);
            title_16300();
            self->state_04 = 0xfffe;
            self->delay_08 = 2;
            return;
        }
        if (g_29f98 == 0) {
            title_14f20();
        }
        break;
    default:
        return;
    }
    self->state_04 = 2;
    self->delay_08 = 1;
}

void title_14e10(void)
{
    g_2ac4c = 10;
    g_2ac2c = title_17ad0(0, 0, 0, 0x200e, 0);
    g_2ac2c->unknown_034 = 1;
    g_2ac2c->unknown_054 |= 5;
    g_2ac2c->unknown_04a = 0;
    g_2ac48 = title_17ad0(0, 0, 0, 0x200e, 0);
    g_2ac48->unknown_034 = 1;
    g_2ac48->unknown_054 |= 6;
    g_2ac48->unknown_04a = 0;
    g_2ac2c->unknown_023 = 6;
    g_2ac2c->unknown_03e = 10;
    g_2ac48->unknown_023 = 6;
    g_2ac48->unknown_03e = 20;
    g_2ac2c->unknown_000 = 0;
    g_2ac2c->unknown_004 = 0xfff00000;
    g_2ac48->unknown_000 = 0x10000;
    g_2ac48->unknown_004 = 0xfff10000;
    g_2ac34 = title_17ad0(0, 0, 0, 0x200e, 0);
    g_2ac34->unknown_034 = 2;
    g_2ac34->unknown_000 = 0xff9c0000;
    g_2ac34->unknown_004 = 0x640000;
    g_2ac38 = 0;
    g_2ac30 = 0;
}

void title_14f20(void)
{
    unsigned short code;

    g_2ac54 ^= 1;
    if (g_2ac54 != 0) {
        return;
    }
    switch (g_2ac38) {
    case 0:
        break;
    case 1:
        g_2ac34->unknown_034++;
        if (g_2ac34->unknown_034 != 0x10) {
            return;
        }
        g_2ac38 = 2;
        return;
    case 2:
        g_2ac4c = 0xc;
        g_2ac38 = 3;
        return;
    default:
        return;
    }
    g_2ac34->unknown_034 = g_25f50[g_2ac30] + 1;
    g_2ac30++;
    code = g_25f50[g_2ac30];
    if (code == 0xa && g_2ac4c == 0xa) {
        g_2ac4c = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    g_2ac38 = 1;
}
