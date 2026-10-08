/* TITLE.DLL lane w01 region: 0x1000, 0x11f0, 0x16e0 (ascending). Shared types and externs first.
 * Layouts agree with every function in this file; 0x1300 is not part of this region (see result.json). */
#include "title_engine.h"

typedef struct TitleObject {
    unsigned long unknown_000;
    unsigned long unknown_004;
    unsigned char unknown_008[0x23 - 0x08];
    unsigned char unknown_023;
    unsigned char unknown_024[0x34 - 0x24];
    unsigned short unknown_034;
    unsigned char unknown_036[0x3e - 0x36];
    unsigned short unknown_03e;
    unsigned char unknown_040[0x4a - 0x40];
    unsigned short unknown_04a;
    unsigned char unknown_04c[0x54 - 0x4c];
    unsigned long unknown_054;
} TitleObject;

typedef struct TitleContextView {
    unsigned char byte_000;
    unsigned char unknown_001[0x2e - 0x01];
    unsigned short word_02e;
} TitleContextView;

extern TitleObject *g_290f8;
extern TitleObject *g_290fc;
extern TitleObject *g_29100;
extern TitleObject *g_29104;
extern TitleObject *g_29108;
extern TitleObject *g_29110;
extern TitleObject *g_29114;
extern TitleObject *g_29118;
extern TitleObject *g_2911c;
extern TitleObject *g_29120;

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
