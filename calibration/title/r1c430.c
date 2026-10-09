/* TITLE.DLL lane w28 region: 0x1c430, 0x1d380, 0x1d4e0, 0x1d590, 0x1d770, 0x1d790. */
#include "title_engine.h"

typedef struct TitleObject TitleObject;

typedef struct TitleHost {
    unsigned char unknown_000[0x38];
    unsigned long dword_038;
    unsigned char unknown_03c[0x5c - 0x3c];
    unsigned short word_05c;
    unsigned short word_05e;
    unsigned char unknown_060[0x62 - 0x60];
    unsigned short word_062;
    unsigned short word_064;
    unsigned char unknown_066[0x9c - 0x66];
    unsigned short word_09c;
    unsigned char unknown_09e[0xb4 - 0x9e];
    int (*handler_0b4)(int);
} TitleHost;

typedef struct TitleBlock {
    unsigned char unknown_000[0x16];
    unsigned char byte_016;
    unsigned char unknown_017;
    unsigned char byte_018;
    unsigned char byte_019;
    unsigned char byte_01a;
    unsigned char byte_01b;
    unsigned char unknown_01c[0x148e - 0x1c];
    unsigned char byte_148e;
    unsigned char unknown_148f;
    unsigned char byte_1490;
    unsigned char byte_1491;
    unsigned char byte_1492;
    unsigned char byte_1493;
} TitleBlock;

extern TitleObject *g_2b058[2];
extern TitleObject *g_2b104[2];
extern TitleObject *g_2b1d8[4];
extern TitleBlock *g_2cc04;
extern TitleBlock *g_2cc08;
extern unsigned short g_2bf6c;
extern unsigned short g_2bff2;
extern unsigned short g_2bf72;
extern void *g_2d320;
extern void *g_2df4c;
extern void *g_2dfa0;
extern void *g_2df44;
extern void *g_2d32c;
extern void *g_2dfa8;
extern char *g_2bb24;
extern char *g_2dfb0;
extern char *g_2df40;
extern unsigned char g_2df50;
extern int g_2b370;
extern int g_2674c;
extern int g_2cbe0[3];
extern char g_29128[];

void title_09350(void *block);
void title_19820(void);
void title_0c2d0(void);
void title_0c300(void);
void title_0c330(void *p);
int title_0c340(int a);
int title_0c6e0(int a);
void title_0c6d0(int a);
int title_0c310(int a);
int title_0c450(void *out, int size, int flags);
int title_0c3f0(char *a, int b, int c, int d);
int title_0c880(void);
void title_0c890(unsigned short *rect, int value);
void title_0c3c0(void);
void title_02a20(void);
void title_04760(void);
int title_048a0(int a, int b);
void title_01dd0(void);
void title_0c690(void *p, int n);
void title_0c520(void *a, void *b);
void title_04870(void);
void title_0cb10(int a, int b);
void title_0cb30(int a);
void title_0c5b0(void *p);
void title_041d0(void);
void title_0ca20(void *p);
void title_0ca10(void *p);
void title_1d770(void);

void title_1c430(void)
{
    if (g_2b104[0]) {
        title_09350(g_2b104[0]);
        g_2b104[0] = 0;
    }
    if (g_2b058[0]) {
        title_09350(g_2b058[0]);
        g_2b058[0] = 0;
    }
    if (g_2b104[1]) {
        title_09350(g_2b104[1]);
        g_2b104[1] = 0;
    }
    if (g_2b058[1]) {
        title_09350(g_2b058[1]);
        g_2b058[1] = 0;
    }
    if (g_2b1d8[0]) {
        title_09350(g_2b1d8[0]);
        g_2b1d8[0] = 0;
    }
    if (g_2b1d8[1]) {
        title_09350(g_2b1d8[1]);
        g_2b1d8[1] = 0;
    }
    if (g_2b1d8[2]) {
        title_09350(g_2b1d8[2]);
        g_2b1d8[2] = 0;
    }
    if (g_2b1d8[3]) {
        title_09350(g_2b1d8[3]);
        g_2b1d8[3] = 0;
    }
    title_19820();
}

void title_1d380(void)
{
    unsigned short rect[4];

    title_0c450(&g_2d320, 0x134 * g_2bf6c, 0x10);
    title_0c450(&g_2df4c, 0x134 + 0xec * g_2bff2, 0x10);
    title_0c450(&g_2dfa0, 0x134 + 0x94 * g_2bf72, 0x10);
    title_0c880();
    title_0c450(&g_2cc08, 0x28f0, 0);
    title_0c450(&g_2df44, 0x4000, 0);
    title_0c450(&g_2d32c, 0x8000, 0);
    title_0c450(&g_2dfa8, 0x300, 0);
    title_0c450(&g_2bb24, 0x23000, 0);
    rect[0] = 0x390;
    rect[1] = 0;
    rect[2] = 0x50;
    rect[3] = 0xc8;
    title_0c450(&g_2b370, 0x8800, 0x10);
    title_0c3f0(g_29128, g_2674c, g_2b370, 0x8800);
    title_1d770();
    title_0c890(rect, g_2b370);
    title_0c3c0();
    title_02a20();
    title_04760();
}

void title_1d4e0(void)
{
    int (*handler)(int);

    title_0c6e0(0);
    title_0c6d0(0);
    if (((TitleHost *)g_engine_interface.context_004)->word_09c == 1) {
        title_0c2d0();
        handler = ((TitleHost *)g_engine_interface.context_004)->handler_0b4;
        if (handler) {
            handler(0);
        }
    } else if (((TitleHost *)g_engine_interface.context_004)->word_09c) {
        title_0c300();
    }
    if (g_2d320) {
        title_0c330(g_2d320);
    }
    if (g_2df4c) {
        title_0c330(g_2df4c);
    }
    if (g_2dfa0) {
        title_0c330(g_2dfa0);
    }
    if (g_2b370) {
        title_0c330((void *)g_2b370);
    }
    title_0c340(0);
}

void title_1d590(void)
{
    ((TitleHost *)g_engine_interface.context_004)->word_05e = 0;
    ((TitleHost *)g_engine_interface.context_004)->word_05c = 0;
    ((TitleHost *)g_engine_interface.context_004)->word_064 = 0;
    ((TitleHost *)g_engine_interface.context_004)->word_062 = 0;
    ((TitleHost *)g_engine_interface.context_004)->dword_038 = 0;
    g_2cbe0[0] = 0xa0;
    g_2cbe0[1] = 0x78;
    g_2cbe0[2] = 0x100;
    g_2cc04 = g_2cc08;
    g_2dfb0 = g_2d320;
    title_048a0(0, 0);
    title_01dd0();
    title_0c690((char *)g_2cc08 + 0x70, 0x500);
    title_0c690((char *)g_2cc08 + 0x14e8, 0x500);
    if (g_2cc04 == g_2cc08) {
        g_2df40 = g_2bb24 + 0x11800;
    } else {
        g_2df40 = g_2bb24;
    }
    title_0c520(g_2cc08, (char *)g_2cc08 + 0x1478);
    g_2cc08->byte_019 = 0;
    g_2cc08->byte_01a = 0;
    g_2cc08->byte_01b = 0;
    g_2cc08->byte_1491 = 0;
    g_2cc08->byte_1492 = 0;
    g_2cc08->byte_1493 = 0;
    g_2cc08->byte_016 = 1;
    g_2cc08->byte_148e = 1;
    g_2cc08->byte_018 = 0;
    g_2cc08->byte_1490 = 0;
    g_2df50 = 1;
    title_04870();
    title_0cb10(g_2cbe0[0], g_2cbe0[1]);
    title_0cb30(g_2cbe0[2]);
    title_0c5b0(g_2cc04);
    title_0c5b0(g_2cc04);
    title_041d0();
    title_0ca20(g_2cc04);
    title_0ca10((char *)g_2cc04 + 0x5c);
    if (g_2cc04 == g_2cc08) {
        g_2cc04 = (TitleBlock *)((char *)g_2cc08 + 0x1478);
    } else {
        g_2cc04 = g_2cc08;
    }
}

void title_1d770(void)
{
    if (title_0c310(0) == 0) {
        do {
        } while (title_0c310(0) == 0);
    }
}

void title_1d790(int value)
{
    g_2cc08->byte_018 = value;
    g_2cc08->byte_1490 = value;
}
