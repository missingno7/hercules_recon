/* TITLE engine unit 0xc1f0..0xcc8f: PC_DLLEngineMain (0xc1f0, the export; not reconstructed yet) and the
   engine-interface slot wrappers. 0xc140 of the previous unit ends exactly at 0xc1f0 and the next unit
   (private .bss) starts at 0xcc90. Its link position matches the tail group of g_engine_interface,
   so the interface record is tentatively defined here (a C communal, 1000 bytes). */
#include "title_engine.h"
#include "title_screen.h"

TitleEngineInterface g_engine_interface;  /* copied in by PC_DLLEngineMain */

void title_05a70(TitleObject *p);
void title_09350(TitleObject *p);
void unlink_12c(TitleObject *p);
void title_0ca30(void);
void title_1d770(void);
void title_0c9c0(int a, int b);
int title_0c9e0(int a, int b);
unsigned int title_0c4a0(unsigned int a);
void title_09a40(void);
void title_09c90(void);
void title_0a020(void);
void title_01de0(int a, int b, int c, int d, int e);
void title_02090(int a, int b, int c, int d, int e, int f, int g, int h);
void *title_17ad0(int a, int b, int c, int d, int e);
int title_0c4a0(int a);

void title_0c270(void)
{
    g_engine_interface.slot_018();
}

int title_0c280(int a, int b)
{
    return g_engine_interface.slot_01c(a, b);
}

int title_0c2a0(int a, int b, int c, int d, int e)
{
    return g_engine_interface.slot_020(a, b, c, d, e);
}

void title_0c2d0(void)
{
    g_engine_interface.slot_024();
}

void title_0c2e0(void)
{
    g_engine_interface.slot_028();
}

void title_0c2f0(void)
{
    g_engine_interface.slot_02c();
}

void title_0c300(void)
{
    g_engine_interface.slot_030();
}

int title_0c310(int a) { return g_engine_interface.slot_034(a); }

void title_0c320(void) { g_engine_interface.slot_03c(); }

int title_0c330(int a) { return g_engine_interface.slot_040(a); }

int title_0c340(int a) { return g_engine_interface.slot_044(a); }

int title_0c350(int a, int b) { return g_engine_interface.slot_048(a, b); }

int title_0c370(int a, int b) { return g_engine_interface.slot_04c(a, b); }

int title_0c390(int a, int b) { return g_engine_interface.slot_050(a, b); }

void title_0c3b0(void) { g_engine_interface.slot_05c(); }

void title_0c3c0(void) { g_engine_interface.slot_060(); }

int title_0c3d0(int a, int b) { return g_engine_interface.slot_064(a, b); }

int title_0c3f0(int a, int b, int c, int d) { return g_engine_interface.slot_068(a, b, c, d); }

int title_0c410(int a, int b) { return g_engine_interface.slot_070(a, b); }

int title_0c430(int a, int b) { return g_engine_interface.slot_074(a, b); }

int title_0c450(int a, int b, int c) { return g_engine_interface.slot_078(a, b, c); }

void title_0c470(void) { g_engine_interface.slot_084(); }

void title_0c480(void) { g_engine_interface.slot_088(); }

void title_0c490(void) { g_engine_interface.slot_08c(); }

int title_0c4a0(int a) { return g_engine_interface.slot_094(a); }

int title_0c4b0(int a) { return g_engine_interface.slot_0a4(a); }

void title_0c4c0(void) { g_engine_interface.slot_0ac(); }

int title_0c4d0(int a) { return g_engine_interface.slot_0b0(a); }

void title_0c4e0(void) { g_engine_interface.slot_0b4(); }

int title_0c4f0(int a) { return g_engine_interface.slot_0b8(a); }

int title_0c500(int a, int b) { return g_engine_interface.slot_0c4(a, b); }

int title_0c520(int a, int b) { return g_engine_interface.slot_0c8(a, b); }

int title_0c540(int a) { return g_engine_interface.slot_0cc(a); }

int title_0c550(int a) { return g_engine_interface.slot_0d0(a); }

int title_0c560(int a, int b) { return g_engine_interface.slot_0d4(a, b); }

int title_0c580(int a, int b) { return g_engine_interface.slot_0d8(a, b); }

void title_0c5a0(void) { g_engine_interface.slot_0dc(); }

int title_0c5b0(int a) { return g_engine_interface.slot_0e0(a); }

void title_0c5c0(void) { g_engine_interface.slot_0e4(); }

int title_0c5d0(int a) { return g_engine_interface.slot_0ec(a); }

void title_0c5e0(void) { g_engine_interface.slot_0f0(); }

void title_0c5f0(void) { g_engine_interface.slot_0f4(); }

int title_0c600(int a) { return g_engine_interface.slot_0f8(a); }

int title_0c610(int a) { return g_engine_interface.slot_0fc(a); }

int title_0c620(int a) { return g_engine_interface.slot_100(a); }

int title_0c630(int a, int b, int c) { return g_engine_interface.slot_104(a, b, c); }

int title_0c650(int a, int b) { return g_engine_interface.slot_108(a, b); }

int title_0c670(int a, int b, int c, int d) { return g_engine_interface.slot_140(a, b, c, d); }

int title_0c690(int a, int b) { return g_engine_interface.slot_144(a, b); }

int title_0c6b0(int a) { return g_engine_interface.slot_164(a); }

int title_0c6c0(int a) { return g_engine_interface.slot_168(a); }

int title_0c6d0(int a) { return g_engine_interface.slot_16c(a); }

int title_0c6e0(int a) { return g_engine_interface.slot_170(a); }

int title_0c6f0(void) { return g_engine_interface.slot_178(); }

int title_0c700(void) { return g_engine_interface.slot_180(); }

int title_0c710(int a, int b) { return g_engine_interface.slot_184(a, b); }

int title_0c730(int a) { return g_engine_interface.slot_18c(a); }

int title_0c740(int a, int b, int c) { return g_engine_interface.slot_190(a, b, c); }

int title_0c760(int a) { return g_engine_interface.slot_194(a); }

int title_0c770(int a, int b, int c) { return g_engine_interface.slot_198(a, b, c); }

int title_0c790(void) { return g_engine_interface.slot_19c(); }

int title_0c7a0(void) { return g_engine_interface.slot_1a0(); }

int title_0c7b0(void) { return g_engine_interface.slot_1a8(); }

int title_0c7c0(void) { return g_engine_interface.slot_1ac(); }

int title_0c7d0(int a) { return g_engine_interface.slot_1b0(a); }

int title_0c7e0(int a) { return g_engine_interface.slot_1b8(a); }

int title_0c7f0(int a) { return g_engine_interface.slot_1bc(a); }

int title_0c800(int a) { return g_engine_interface.slot_1c0(a); }

int title_0c810(int a) { return g_engine_interface.slot_1c8(a); }

int title_0c820(int a) { return g_engine_interface.slot_1d4(a); }

int title_0c830(int a) { return g_engine_interface.slot_1d8(a); }

int title_0c840(int a) { return g_engine_interface.slot_1e0(a); }

int title_0c850(int a) { return g_engine_interface.slot_1e4(a); }

int title_0c860(int a, int b, int c, int d) { return g_engine_interface.slot_1f4(a, b, c, d); }

int title_0c880(void) { return g_engine_interface.slot_1f8(); }

int title_0c890(int a, int b) { return g_engine_interface.slot_204(a, b); }

int title_0c8b0(int a, int b, int c, int d, int e, int f, int g) { return g_engine_interface.slot_208(a, b, c, d, e, f, g); }

int title_0c8e0(int a, int b, int c) { return g_engine_interface.slot_20c(a, b, c); }

int title_0c900(int a) { return g_engine_interface.slot_210(a); }

int title_0c910(int a) { return g_engine_interface.slot_224(a); }

int title_0c920(int a) { return g_engine_interface.slot_22c(a); }

int title_0c930(int a) { return g_engine_interface.slot_230(a); }

int title_0c940(int a) { return g_engine_interface.slot_234(a); }

int title_0c950(void) { return g_engine_interface.slot_238(); }

int title_0c960(int a) { return g_engine_interface.slot_23c(a); }

int title_0c970(int a, int b) { return g_engine_interface.slot_240(a, b); }

int title_0c990(int a, int b) { return g_engine_interface.slot_244(a, b); }

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
