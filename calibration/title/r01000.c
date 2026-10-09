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

static TitleObject *g_290f8;
static TitleObject *g_290fc;
static TitleObject *g_29100;
static TitleObject *g_29104;
static TitleObject *g_29108;
static TitleObject *g_29110;
static TitleObject *g_29114;
static TitleObject *g_29118;
static TitleObject *g_2911c;
static TitleObject *g_29120;

TitleObject *title_17ad0(int a0, int a1, int a2, int size, int a4);
void title_09350(void *block);

void title_01000(void)
{
    TitleContextView *ctx;

    g_29108 = 0;
    g_29104 = 0;
    g_29100 = 0;
    g_290fc = 0;
    g_290f8 = 0;
    ctx = (TitleContextView *)g_engine_interface.context_004;
    if (ctx->word_02e == 0) {
        g_290f8 = title_17ad0(0, 0, 0, 0x2018, 0);
        g_290f8->unknown_034 = 0x11;
        g_290fc = title_17ad0(0, 0, 0, 0x2018, 0);
        g_290fc->unknown_034 = 7;
        g_29100 = title_17ad0(0, 0, 0, 0x2018, 0);
        g_29100->unknown_034 = 1;
        g_29104 = title_17ad0(0, 0, 0, 0x2018, 0);
        g_29104->unknown_034 = 2;
        g_29108 = title_17ad0(0, 0, 0, 0x2018, 0);
        g_29108->unknown_034 = 3;
    } else {
        g_290f8 = title_17ad0(0, 0, 0, 0x2018, 0);
        g_290f8->unknown_034 = 0x1d;
        g_290fc = title_17ad0(0, 0, 0, 0x2018, 0);
        g_290fc->unknown_034 = 0x14;
        g_29100 = title_17ad0(0, 0, 0, 0x2018, 0);
        g_29100->unknown_034 = 0x13;
    }
    g_29120 = 0;
    g_2911c = 0;
    g_29118 = 0;
    g_29114 = 0;
    g_29110 = 0;
    g_29110 = title_17ad0(0, 0, 0, 0x2018, 0);
    g_29110->unknown_034 = g_290f8->unknown_034;
    g_29114 = title_17ad0(0, 0, 0, 0x2018, 0);
    g_29114->unknown_034 = g_290fc->unknown_034;
    ctx = (TitleContextView *)g_engine_interface.context_004;
    if (ctx->word_02e == 0) {
        g_29118 = title_17ad0(0, 0, 0, 0x2018, 0);
        g_29118->unknown_034 = 1;
        g_2911c = title_17ad0(0, 0, 0, 0x2018, 0);
        g_2911c->unknown_034 = 2;
        g_29120 = title_17ad0(0, 0, 0, 0x2018, 0);
        g_29120->unknown_034 = 3;
    } else {
        g_29118 = title_17ad0(0, 0, 0, 0x2018, 0);
        g_29118->unknown_034 = 0x13;
    }
}

void title_011f0(void)
{
    TitleContextView *ctx;

    if (g_290f8) {
        title_09350(g_290f8);
        g_290f8 = 0;
    }
    if (g_290fc) {
        title_09350(g_290fc);
        g_290fc = 0;
    }
    if (g_29100) {
        title_09350(g_29100);
        g_29100 = 0;
    }
    ctx = (TitleContextView *)g_engine_interface.context_004;
    if (ctx->word_02e == 0) {
        if (g_29104) {
            title_09350(g_29104);
            g_29104 = 0;
        }
        if (g_29108) {
            title_09350(g_29108);
            g_29108 = 0;
        }
    }
    if (g_29110) {
        title_09350(g_29110);
        g_29110 = 0;
    }
    if (g_29114) {
        title_09350(g_29114);
        g_29114 = 0;
    }
    if (g_29118) {
        title_09350(g_29118);
        g_29118 = 0;
    }
    ctx = (TitleContextView *)g_engine_interface.context_004;
    if (ctx->word_02e == 0) {
        if (g_2911c) {
            title_09350(g_2911c);
            g_2911c = 0;
        }
        if (g_29120) {
            title_09350(g_29120);
            g_29120 = 0;
        }
    }
}

void title_016e0(const char *format, ...)
{
}

static unsigned int g_290f0;
static unsigned int g_290f4;
static int g_2910c;
static int g_29124;
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
        title_01de0(0x140, 0, g_290f4, 0, 0);
        switch (g_29124) {
        case 0:
            if (g_290f4 < 0x80) {
                g_290f4 = g_290f4 + 0x10;
            } else {
                g_29124 = 1;
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
                g_29124 = 2;
            }
            if (((TitleContextView *)g_engine_interface.context_004)->word_02e != 0) {
                if (buttons & 0x40) {
                    g_2910c = g_2910c + 1;
                    title_054f0(0x301, 0);
                }
                if (buttons & 0x10) {
                    g_2910c = g_2910c - 1;
                    title_054f0(0x300, 0);
                }
            } else {
                if (buttons & 0x20) {
                    g_2910c = g_2910c + 1;
                    title_054f0(0x301, 0);
                }
            }
            if (g_2910c == -1) {
                g_2910c = 9;
            }
            if (g_2910c == 10) {
                g_2910c = 0;
            }
            break;
        case 2:
            if (g_290f4 > 0) {
                g_290f4 = g_290f4 - 0x10;
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
        g_2910c = 0;
        g_290f0 = 0xf;
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
        g_290f4 = 0;
        g_29124 = 0;
        ((TitleContextView *)g_engine_interface.context_004)->dword_030 |= 0x80000000;
        break;
    default:
        return;
    }
    out->state_04 = 1;
    out->delay_08 = 1;
}
