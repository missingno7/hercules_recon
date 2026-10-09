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
int rand();  /* LIBCMT; called with an argument, so unprototyped */

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

