/* TITLE.DLL lane w01 region: 0x1000, 0x11f0, 0x16e0 (ascending). Shared types and externs first.
 * Layouts agree with every function in this file; 0x1300 is not part of this region (see result.json). */
#include "title_engine.h"
#include "title_screen.h"
#include "title_files.h"

typedef struct TitleContextView {
    unsigned char byte_000;
    unsigned char unknown_001[0x0b - 0x01];
    unsigned char byte_00b;
    unsigned char byte_00c;
    unsigned char unknown_00d[0x2e - 0x0d];
    unsigned short word_02e;
    unsigned long dword_030;
} TitleContextView;

/* Private .bss of the 0x1000 unit (0x290f0..0x29127) in address order: names chosen for meaning and
   for VC5's identifier-hash layout order (scripts/layout_names.py), not recovered identifiers. */
static TitleObject *item0;
static TitleObject *item1;
static TitleObject *item2;
static TitleObject *item3;
static TitleObject *item4;
static TitleObject *twin0;
static TitleObject *twin1;
static TitleObject *twin2;
static TitleObject *twin3;
static TitleObject *twin4;

static unsigned int unknown_f0;
static unsigned int fade2;
static int selection;
static int step;

TitleObject *title_17ad0(int a0, int a1, int a2, int size, int a4);
void title_09350(void *block);

void title_01000(void)
{
    TitleContextView *ctx;

    item4 = 0;
    item3 = 0;
    item2 = 0;
    item1 = 0;
    item0 = 0;
    ctx = (TitleContextView *)g_engine_interface.context_004;
    if (ctx->word_02e == 0) {
        item0 = title_17ad0(0, 0, 0, 0x2018, 0);
        item0->unknown_034 = 0x11;
        item1 = title_17ad0(0, 0, 0, 0x2018, 0);
        item1->unknown_034 = 7;
        item2 = title_17ad0(0, 0, 0, 0x2018, 0);
        item2->unknown_034 = 1;
        item3 = title_17ad0(0, 0, 0, 0x2018, 0);
        item3->unknown_034 = 2;
        item4 = title_17ad0(0, 0, 0, 0x2018, 0);
        item4->unknown_034 = 3;
    } else {
        item0 = title_17ad0(0, 0, 0, 0x2018, 0);
        item0->unknown_034 = 0x1d;
        item1 = title_17ad0(0, 0, 0, 0x2018, 0);
        item1->unknown_034 = 0x14;
        item2 = title_17ad0(0, 0, 0, 0x2018, 0);
        item2->unknown_034 = 0x13;
    }
    twin4 = 0;
    twin3 = 0;
    twin2 = 0;
    twin1 = 0;
    twin0 = 0;
    twin0 = title_17ad0(0, 0, 0, 0x2018, 0);
    twin0->unknown_034 = item0->unknown_034;
    twin1 = title_17ad0(0, 0, 0, 0x2018, 0);
    twin1->unknown_034 = item1->unknown_034;
    ctx = (TitleContextView *)g_engine_interface.context_004;
    if (ctx->word_02e == 0) {
        twin2 = title_17ad0(0, 0, 0, 0x2018, 0);
        twin2->unknown_034 = 1;
        twin3 = title_17ad0(0, 0, 0, 0x2018, 0);
        twin3->unknown_034 = 2;
        twin4 = title_17ad0(0, 0, 0, 0x2018, 0);
        twin4->unknown_034 = 3;
    } else {
        twin2 = title_17ad0(0, 0, 0, 0x2018, 0);
        twin2->unknown_034 = 0x13;
    }
}

void title_011f0(void)
{
    TitleContextView *ctx;

    if (item0) {
        title_09350(item0);
        item0 = 0;
    }
    if (item1) {
        title_09350(item1);
        item1 = 0;
    }
    if (item2) {
        title_09350(item2);
        item2 = 0;
    }
    ctx = (TitleContextView *)g_engine_interface.context_004;
    if (ctx->word_02e == 0) {
        if (item3) {
            title_09350(item3);
            item3 = 0;
        }
        if (item4) {
            title_09350(item4);
            item4 = 0;
        }
    }
    if (twin0) {
        title_09350(twin0);
        twin0 = 0;
    }
    if (twin1) {
        title_09350(twin1);
        twin1 = 0;
    }
    if (twin2) {
        title_09350(twin2);
        twin2 = 0;
    }
    ctx = (TitleContextView *)g_engine_interface.context_004;
    if (ctx->word_02e == 0) {
        if (twin3) {
            title_09350(twin3);
            twin3 = 0;
        }
        if (twin4) {
            title_09350(twin4);
            twin4 = 0;
        }
    }
}

typedef struct TitleMenuEntry {
    int frame;
    int level;
} TitleMenuEntry;
/* Level-select menu tables (.data 0x22030..0x220cf, the first first-party initialized data, so this
   unit links first): frame and level per menu entry, normal and alternate (word_02e) menus. */
TitleMenuEntry g_22030[10] = {{7, 1}, {8, 2}, {9, 3}, {10, 5}, {11, 6}, {12, 7}, {13, 8}, {14, 9}, {15, 10}, {16, 11}};
TitleMenuEntry g_22080[10] = {{20, 1}, {21, 2}, {22, 3}, {23, 5}, {24, 6}, {25, 7}, {26, 8}, {27, 9}, {28, 10}, {18, 11}};

void title_01300(void)
{
    if (((TitleContextView *)g_engine_interface.context_004)->word_02e == 0) {
        item1->unknown_034 = g_22030[selection].frame;
        ((TitleContextView *)g_engine_interface.context_004)->byte_000 = g_22030[selection].level;
    } else {
        item1->unknown_034 = g_22080[selection].frame;
        ((TitleContextView *)g_engine_interface.context_004)->byte_000 = g_22080[selection].level;
    }
    if (((TitleContextView *)g_engine_interface.context_004)->word_02e == 0) {
        if (((TitleContextView *)g_engine_interface.context_004)->dword_030 & 8)
            item2->unknown_034 = 1;
        else
            item2->unknown_034 = 4;
        if (((TitleContextView *)g_engine_interface.context_004)->dword_030 & 0x10)
            item3->unknown_034 = 2;
        else
            item3->unknown_034 = 5;
        if (((TitleContextView *)g_engine_interface.context_004)->dword_030 & 0x800000)
            item4->unknown_034 = 3;
        else
            item4->unknown_034 = 6;
    }
    twin0->unknown_054 |= 6;
    twin1->unknown_054 |= 6;
    twin2->unknown_054 |= 6;
    if (((TitleContextView *)g_engine_interface.context_004)->word_02e == 0) {
        twin3->unknown_054 |= 6;
        twin4->unknown_054 |= 6;
    }
    twin0->unknown_000 = 0x10000;
    twin0->unknown_004 = 0x10000;
    twin1->unknown_000 = 0x10000;
    twin1->unknown_004 = 0x10000;
    twin2->unknown_000 = 0x10000;
    twin2->unknown_004 = 0x10000;
    if (((TitleContextView *)g_engine_interface.context_004)->word_02e == 0) {
        twin3->unknown_000 = 0x10000;
        twin3->unknown_004 = 0x10000;
        twin4->unknown_000 = 0x10000;
        twin4->unknown_004 = 0x10000;
    }
    item2->unknown_023 = 6;
    item1->unknown_023 = 6;
    item0->unknown_023 = 6;
    if (((TitleContextView *)g_engine_interface.context_004)->word_02e == 0) {
        item4->unknown_023 = 6;
        item3->unknown_023 = 6;
    }
    if (((TitleContextView *)g_engine_interface.context_004)->word_02e == 0) {
        item0->unknown_03e = 10;
        item1->unknown_03e = 10;
        item2->unknown_03e = 0;
        item3->unknown_03e = 0;
        item4->unknown_03e = 0;
    } else {
        item0->unknown_03e = 10;
        item1->unknown_03e = 10;
        item2->unknown_03e = 20;
    }
    twin1->unknown_023 = 6;
    twin2->unknown_023 = 6;
    twin0->unknown_023 = 6;
    if (((TitleContextView *)g_engine_interface.context_004)->word_02e == 0) {
        twin0->unknown_03e = 40;
        twin1->unknown_03e = 40;
        twin2->unknown_03e = 40;
        twin3->unknown_03e = 10;
        twin4->unknown_03e = 10;
        twin4->unknown_023 = 6;
        twin3->unknown_023 = 6;
    } else {
        twin0->unknown_03e = 40;
        twin1->unknown_03e = 40;
        twin2->unknown_03e = 40;
    }
    item1->unknown_04a = fade2;
    item2->unknown_04a = fade2;
    item0->unknown_04a = fade2;
    if (((TitleContextView *)g_engine_interface.context_004)->word_02e == 0) {
        item4->unknown_04a = fade2;
        item3->unknown_04a = fade2;
    }
    twin1->unknown_04a = fade2 * 2;
    twin2->unknown_04a = twin1->unknown_04a;
    twin0->unknown_04a = twin2->unknown_04a;
    if (((TitleContextView *)g_engine_interface.context_004)->word_02e == 0) {
        twin4->unknown_04a = fade2 * 2;
        twin3->unknown_04a = twin4->unknown_04a;
    }
    twin0->unknown_034 = item0->unknown_034;
    twin1->unknown_034 = item1->unknown_034;
    twin2->unknown_034 = item2->unknown_034;
    if (((TitleContextView *)g_engine_interface.context_004)->word_02e == 0) {
        twin3->unknown_034 = item3->unknown_034;
        twin4->unknown_034 = item4->unknown_034;
    }
}

/* @1300 */
void title_016e0(const char *format, ...)
{
}

extern char g_29128[];
void title_0c990(int a, int b);
void title_0c9c0(int a, int b);
void title_0c560(int a, int b);
void title_1d790(int a);
void title_0c450();
void title_0c3f0();
void title_0c8b0();
void title_0c6d0(int a);
void title_0c330();
void title_04b70(int c, int d);
void title_01300(void);
void title_054f0(int a, int b);
void title_01de0(int a, int b, int c, int d, int e);
void title_04ea0(int a);
void title_05e90();
void title_05f10(void *target);
void title_07510();

void title_016f0(TitleProc *out)
{
    int local;
    int x = ((ScreenContext *)g_engine_interface.context_004)->cursor_5c;
    unsigned long buttons = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5e;

    switch (out->state_04) {
    case 1:
        title_01de0(0x140, 0, fade2, 0, 0);
        switch (step) {
        case 0:
            if (fade2 < 0x80) {
                fade2 = fade2 + 0x10;
            } else {
                step = 1;
            }
            break;
        case 1:
            if (((TitleContextView *)g_engine_interface.context_004)->word_02e == 0) {
                if (buttons & 0x10) {
                    ((TitleContextView *)g_engine_interface.context_004)->dword_030 ^= 8;
                    title_054f0(0x302, 0);
                }
                if (buttons & 0x80) {
                    ((TitleContextView *)g_engine_interface.context_004)->dword_030 ^= 0x10;
                    title_054f0(0x302, 0);
                }
                if (buttons & 0x40) {
                    ((TitleContextView *)g_engine_interface.context_004)->dword_030 ^= 0x800000;
                    title_054f0(0x302, 0);
                }
            }
            if (buttons & 8) {
                step = 2;
            }
            if (((TitleContextView *)g_engine_interface.context_004)->word_02e != 0) {
                if (buttons & 0x40) {
                    selection = selection + 1;
                    title_054f0(0x301, 0);
                }
                if (buttons & 0x10) {
                    selection = selection - 1;
                    title_054f0(0x300, 0);
                }
            } else {
                if (buttons & 0x20) {
                    selection = selection + 1;
                    title_054f0(0x301, 0);
                }
            }
            if (selection == -1) {
                selection = 9;
            }
            if (selection == 10) {
                selection = 0;
            }
            break;
        case 2:
            if (fade2 > 0) {
                fade2 = fade2 - 0x10;
            } else {
                out->state_04 = 0xfffe;
                out->delay_08 = 1;
                return;
            }
            break;
        }
        title_01300();
        out->state_04 = 1;
        out->delay_08 = 1;
        return;
    case 0xfffe:
        title_0c9c0(0, 0);
        ((TitleContextView *)g_engine_interface.context_004)->byte_00c = ((TitleContextView *)g_engine_interface.context_004)->byte_00b;
        title_011f0();
        title_04ea0(0x11);
        title_05e90(title_07510, 0);
        title_05f10(out);
        return;
    case 0xffff:
        title_0c990(-1, 0);
        title_0c9c0(0, 8);
        selection = 0;
        unknown_f0 = 0xf;
        title_0c560(0, 2);
        title_1d790(1);
        local = 0;
        title_0c450(&local, 0x15000, 0);
        title_0c3f0(g_29128, g_screen_files[18], local, 0x14312);
        title_0c8b0(local + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(local);
        title_04b70(0x11, 0);
        title_01000();
        title_01300();
        fade2 = 0;
        step = 0;
        ((TitleContextView *)g_engine_interface.context_004)->dword_030 |= 0x80000000;
        break;
    default:
        return;
    }
    out->state_04 = 1;
    out->delay_08 = 1;
}
