#include "title_engine.h"

typedef struct TitleObj {
    unsigned long dword_000;
    unsigned long dword_004;
    unsigned char unknown_008[0x23 - 0x08];
    unsigned char byte_023;
    unsigned char unknown_024[0x34 - 0x24];
    unsigned short word_034;
    unsigned char unknown_036[0x3e - 0x36];
    unsigned short word_03e;
    unsigned char unknown_040[0x4a - 0x40];
    unsigned short word_04a;
    unsigned char unknown_04c[0x54 - 0x4c];
    unsigned long dword_054;
    unsigned char unknown_058[0x74 - 0x58];
    unsigned short word_074;
} TitleObj;

typedef struct TitleCtxView {
    unsigned char unknown_000[0x12];
    unsigned char byte_012;
} TitleCtxView;

typedef struct TitleEntry {
    int key0;
    int key1;
    int key2;
    int key3;
    int code_010;
    unsigned char flags_014;
    unsigned char unknown_015[3];
} TitleEntry;

extern TitleObj *g_2a38c;
extern TitleObj *g_2a3a4;
extern TitleObj *g_2a264;
extern TitleObj *g_2a278;
extern TitleObj *g_2a27c;
extern TitleObj *g_2a280;
extern TitleObj *g_2a274;
extern int g_2a254;
extern int g_25a28;
extern int g_2a288[64];
extern int g_2a9b8;
extern TitleEntry g_25a30[43];

void *title_17ad0(int a, int b, int c, int d, int e);
int title_0c4a0(int a);
int title_0c4c0(void);
int title_1d7b0(int a);

void title_0c9b0(int a) { g_engine_interface.slot_248(a); }

void title_0c9c0(int a, int b) { g_engine_interface.slot_24c(a, b); }

int title_0c9e0(int a, int b) { return g_engine_interface.slot_250(a, b); }

void title_0ca00(void) { g_engine_interface.slot_258(); }

void title_0ca10(int a) { g_engine_interface.slot_27c(a); }

void title_0ca20(int a) { g_engine_interface.slot_280(a); }

void title_0ca30(void) { g_engine_interface.slot_284(); }

int title_0ca40(int a, int b) { return g_engine_interface.slot_290(a, b); }

int title_0ca60(int a, int b) { return g_engine_interface.slot_294(a, b); }

int title_0ca80(int a, int b, int c, int d, int e) { return g_engine_interface.slot_298(a, b, c, d, e); }

int title_0cab0(int a, int b, int c, int d, int e) { return g_engine_interface.slot_29c(a, b, c, d, e); }

int title_0cae0(int a) { return g_engine_interface.slot_2a0(a); }

int title_0caf0(int a, int b, int c, int d) { return g_engine_interface.slot_2b4(a, b, c, d); }

int title_0cb10(int a, int b) { return g_engine_interface.slot_2b8(a, b); }

void title_0cb30(int a) { g_engine_interface.slot_2bc(a); }

void title_0cb40(int a) { g_engine_interface.slot_2c8(a); }

void title_0cb50(int a) { g_engine_interface.slot_2e0(a); }

void title_0cb60(int a) { g_engine_interface.slot_2ec(a); }

void title_0cb70(int a) { g_engine_interface.slot_300(a); }

int title_0cb80(int a, int b) { return g_engine_interface.slot_310(a, b); }

void title_0cba0(void) { g_engine_interface.slot_330(); }

int title_0cbb0(int a, int b) { return g_engine_interface.slot_35c(a, b); }

void title_0cbd0(void) { g_engine_interface.slot_370(); }

int title_0cbe0(int a, int b, int c) { return g_engine_interface.slot_374(a, b, c); }

int title_0cc00(int a) { return g_engine_interface.slot_384(a); }

int title_0cc10(int a, int b, int c, int d, int e, int f, int g) { return g_engine_interface.slot_388(a, b, c, d, e, f, g); }

int title_0cc40(int a, int b, int c) { return g_engine_interface.slot_390(a, b, c); }

int title_0cc60(int a) { return g_engine_interface.slot_3d8(a); }

int title_0cc70(int a) { return g_engine_interface.slot_3e0(a); }

int title_0cc80(int a) { return g_engine_interface.slot_3e4(a); }

int title_0cc90(void) {
    int d = title_0c4c0();
    g_2a9b8 = g_2a9b8 + ((d - g_2a9b8) >> 1);
    if (g_2a9b8 > 30) return 5;
    if (g_2a9b8 > 24) return 4;
    if (g_2a9b8 > 18) return 2;
    if (g_2a9b8 > 6) return 3;
    return 1;
}

int title_0cce0(void) {
    int i = 0;
    int p = (int)g_2a288;
    while (p < (int)(g_2a288 + 64)) {
        if (*(int *)p == 0) return i;
        p += 4;
        i++;
    }
    return -1;
}

int title_0cd00(int v) {
    int p = (int)g_2a288;
    while (p < (int)(g_2a288 + 64)) {
        if (*(int *)p == v) *(int *)p = 0;
        p += 4;
    }
    return -1;
}

void title_0cf00(void) {
    g_2a38c = title_17ad0(0, 0, 0, 0x2006, 0);
    g_2a38c->word_034 = 1;
    g_2a38c->dword_000 = 0x100000;
    g_2a38c->dword_004 = 0x6c0000;
    g_2a38c->word_04a = 0;
    g_2a38c->dword_054 |= 5;
    g_2a38c->word_03e = 0;
    g_2a38c->byte_023 = 6;

    g_2a3a4 = title_17ad0(0, 0, 0, 0x2006, 0);
    g_2a3a4->word_034 = 1;
    g_2a3a4->dword_000 = 0x110000;
    g_2a3a4->dword_004 = 0x6d0000;
    g_2a3a4->word_04a = 0;
    g_2a3a4->dword_054 |= 6;
    g_2a3a4->word_03e = 0xa;
    g_2a3a4->byte_023 = 6;

    g_2a264 = title_17ad0(0, 0, 0, 0x2005, 0);
    g_2a264->word_034 = 2;
    g_2a264->word_04a = 0;
    g_2a264->dword_000 = 0xff440000;
    g_2a264->dword_004 = 0x5d0000;
    g_2a264->dword_054 |= 0x2000;
    g_2a264->word_03e = 0xf;
    g_2a264->byte_023 = 6;
    g_2a254 = 0;
    g_25a28 = 1;

    g_2a278 = title_17ad0(0, 0, 0, 0x2006, 0);
    g_2a278->word_034 = 0x1fb;
    g_2a278->dword_000 = 0xffab0000;
    g_2a278->dword_004 = 0xfffe0000;
    g_2a278->word_04a = 0;
    g_2a278->dword_054 |= 0x2000;
    g_2a278->word_03e = 0x1e;
    g_2a278->byte_023 = 6;

    g_2a27c = title_17ad0(0, 0, 0, 0x2006, 0);
    g_2a27c->word_034 = 0x139;
    g_2a27c->dword_000 = 0xffee0000;
    g_2a27c->dword_004 = 0xfffe0000;
    g_2a27c->word_04a = 0;
    g_2a27c->dword_054 |= 0x2000;
    g_2a27c->word_03e = 0x1e;
    g_2a27c->byte_023 = 6;

    g_2a280 = title_17ad0(0, 0, 0, 0x2006, 0);
    g_2a280->word_034 = 0x19a;
    g_2a280->dword_000 = 0x2d0000;
    g_2a280->dword_004 = 0xfffe0000;
    g_2a280->word_04a = 0;
    g_2a280->dword_054 |= 0x2000;
    g_2a280->word_03e = 0x1e;
    g_2a280->byte_023 = 6;

    g_2a274 = title_17ad0(0, 0, 0, 0x2006, 0);
    g_2a274->word_034 = 0xd8;
    g_2a274->dword_000 = 0x6d0000;
    g_2a274->dword_004 = 0xfffe0000;
    g_2a274->word_04a = 0;
    g_2a274->dword_054 |= 0x2000;
    g_2a274->word_03e = 0x1e;
    g_2a274->byte_023 = 6;

    {
        int idx;
        if (((TitleCtxView *)g_engine_interface.context_004)->byte_012 == 0xff) return;
        idx = (signed char)((TitleCtxView *)g_engine_interface.context_004)->byte_012;
        g_2a278->word_034 = g_25a30[idx].key0 + 0x1e3;
        idx = (signed char)((TitleCtxView *)g_engine_interface.context_004)->byte_012;
        g_2a27c->word_034 = g_25a30[idx].key1 + 0x121;
        idx = (signed char)((TitleCtxView *)g_engine_interface.context_004)->byte_012;
        g_2a280->word_034 = g_25a30[idx].key2 + 0x182;
        idx = (signed char)((TitleCtxView *)g_engine_interface.context_004)->byte_012;
        g_2a274->word_034 = g_25a30[idx].key3 + 0xc0;
    }
}

void title_0d250(TitleObj *s) {
    s->word_03e = 0x14;
    s->dword_054 |= 5;
    s->byte_023 = 6;
    s->word_034 = title_0c4a0(3) + 0xb6;
    s->word_074 += title_1d7b0(0x400);
}

