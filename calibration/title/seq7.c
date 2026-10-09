/* Sequence screen 7 unit: handler 0x14ff0 and helpers 0x15480, 0x15580.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"


extern TitleObject *g_2abd0;
extern int g_2abd4;
extern TitleObject *g_2abd8;
extern int g_2abdc;
extern TitleObject *g_2abf0;
extern int g_2abf4;
extern int g_2abfc;
extern unsigned short g_25f50[];
extern TitleObject *g_2ac0c;
extern int g_2ac08;
extern int g_2ac10;
extern int g_2ac1c;
extern int g_2ac28;
extern TitleObject *g_2ac04;
extern TitleObject *g_2ac18;
extern TitleObject *g_2ac2c;
extern TitleObject *g_2ac34;
extern TitleObject *g_2ac48;
extern int g_2ac30;
extern int g_2ac38;
extern int g_2ac4c;
extern int g_2ac54;
extern TitleObject *g_2ac58;
extern TitleObject *g_2ac64;
extern TitleObject *g_2ac70;
extern int g_2ac5c;
extern int g_2ac68;
extern int g_2ac74;
extern int g_2ac80;
extern TitleObject *g_2ac84;
extern TitleObject *g_2ac8c;
extern TitleObject *g_2aca0;
extern int g_2ac88;
extern int g_2ac90;
extern int g_2aca4;
extern int g_2acac;
int title_0c4f0(int a);
extern int g_2ac60;
extern int g_2ac6c;
extern int g_2ac78;
extern int g_2ac7c;
extern int g_2ac94;
extern int g_2ac98;
extern int g_2ac9c;
extern int g_2aca8;
extern int g_29f98;
extern char g_29128[];
void title_01dd0();
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
void title_15480(void);
void title_15580(void);
void title_15ae0(void);
void title_15be0(void);
void title_164b0(void *req);
void title_16300(void);
void title_1d790(int a);
void title_01de0(int a0, int a1, int a2, int a3, int a4);

void title_14ff0(TitleProc *self)
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
        g_2ac7c = 0;
        g_2ac60 = 0x0f;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ7], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(0x0a, 0);
        title_15480();
        title_1d790(1);
        g_2ac78 = 0x80;
        g_2ac6c = 0;
        title_05e90(title_164b0, 0);
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        break;
    case 2:
        switch (g_2ac74) {
        case 0:
            title_01de0(0x140, 0x100, g_2ac6c, 0, 0);
            if (g_2ac6c < 0x80) {
                g_2ac6c += 2;
            }
            break;
        case 1:
            title_01de0(0x140, 0, g_2ac78, 0, 0);
            title_01de0(0x140, 0x100, g_2ac6c, 1, 1);
            if (g_2ac78 < 0x80) {
                g_2ac78 += 8;
            } else {
                g_2ac78 = 0x80;
            }
            if (g_2ac6c > 0) {
                g_2ac6c -= 8;
            } else {
                g_2ac6c = 0;
            }
            break;
        case 2:
            title_01de0(0x140, 0, g_2ac78, 0, 0);
            if (g_2ac78 > 0) {
                g_2ac78 -= 4;
            }
            break;
        case 11:
            if (g_2ac58->unknown_04a < 0x80) {
                g_2ac58->unknown_04a += 8;
                g_2ac70->unknown_04a = g_2ac58->unknown_04a;
            }
            /* fall through */
        case 10:
            title_01de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_01de0(0x280, 0x100, g_2ac78, 0, 0);
            g_2ac64->unknown_04a = (unsigned short)g_2ac78;
            g_2ac58->unknown_04a = (unsigned short)g_2ac78;
            g_2ac70->unknown_04a = (unsigned short)g_2ac78;
            if (g_2ac78 > 0) {
                g_2ac78 -= 8;
            } else {
                g_2ac64->unknown_054 &= 0x7fffffff;
                g_2ac58->unknown_054 &= 0x7fffffff;
                g_2ac70->unknown_054 &= 0x7fffffff;
                g_2ac74 = 0;
            }
            break;
        default:
            break;
        }
        if (g_2ac74 == 0x0c) {
            if (g_2ac64 != 0) {
                title_09350(g_2ac64);
                g_2ac64 = 0;
            }
            if (g_2ac58 != 0) {
                title_09350(g_2ac58);
                g_2ac58 = 0;
            }
            if (g_2ac70 != 0) {
                title_09350(g_2ac70);
                g_2ac70 = 0;
            }
            title_04ea0(0x0a);
            title_16300();
            self->state_04 = 0xfffe;
            self->delay_08 = 2;
            return;
        }
        if (g_29f98 == 0) {
            title_15580();
        }
        break;
    default:
        return;
    }
    self->state_04 = 2;
    self->delay_08 = 1;
}

void title_15480(void)
{
    g_2ac74 = 10;
    g_2ac58 = title_17ad0(0, 0, 0, 0x200f, 0);
    g_2ac58->unknown_034 = 1;
    g_2ac58->unknown_054 |= 5;
    g_2ac58->unknown_04a = 0;
    g_2ac70 = title_17ad0(0, 0, 0, 0x200f, 0);
    g_2ac70->unknown_034 = 1;
    g_2ac70->unknown_054 |= 6;
    g_2ac70->unknown_04a = 0;
    g_2ac58->unknown_023 = 6;
    g_2ac58->unknown_03e = 10;
    g_2ac70->unknown_023 = 6;
    g_2ac70->unknown_03e = 20;
    g_2ac70->unknown_000 = 0x10000;
    g_2ac70->unknown_004 = 0x10000;
    g_2ac64 = title_17ad0(0, 0, 0, 0x200f, 0);
    g_2ac64->unknown_034 = 2;
    g_2ac64->unknown_000 = 0xff9c0000;
    g_2ac64->unknown_004 = 0x640000;
    g_2ac68 = 0;
    g_2ac5c = 0;
}

void title_15580(void)
{
    unsigned short code;

    g_2ac80 ^= 1;
    if (g_2ac80 != 0) {
        return;
    }
    switch (g_2ac68) {
    case 0:
        break;
    case 1:
        g_2ac64->unknown_034++;
        if (g_2ac64->unknown_034 != 0x10) {
            return;
        }
        g_2ac68 = 2;
        return;
    case 2:
        g_2ac74 = 0xc;
        g_2ac68 = 3;
        return;
    default:
        return;
    }
    g_2ac64->unknown_034 = g_25f50[g_2ac5c] + 1;
    g_2ac5c++;
    code = g_25f50[g_2ac5c];
    if (code == 0xa && g_2ac74 == 0xa) {
        g_2ac74 = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    g_2ac68 = 1;
}
