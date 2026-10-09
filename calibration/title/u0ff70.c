#include "title_engine.h"
#include "title_screen.h"

typedef struct TitleRect {
    int a;
    int b;
    int c;
    int d;
} TitleRect;
typedef struct TitleCtxView {
    signed char kind_00;
    unsigned char unknown_01[0x0b - 0x01];
    signed char field_0b;
    signed char field_0c;
    unsigned char unknown_0d;
    signed char field_0e;
    unsigned char unknown_0f;
    unsigned char field_10;
    unsigned char field_11;
    unsigned char unknown_12[0x30 - 0x12];
    int field_30;
    unsigned char unknown_34[0x6c - 0x34];
    TitleRect rect_6c;
    unsigned char unknown_7c[0x1b0c - 0x7c];
    int field_1b0c;
    int field_1b10;
    unsigned char unknown_1b14[0x1b24 - 0x1b14];
    int field_1b24;
} TitleCtxView;
typedef struct TitleObj {
    unsigned char field_00;
    unsigned char field_01;
    unsigned char field_02;
    unsigned char field_03;
    int field_04;
    int field_08;
    int field_0c;
    int field_10;
    int field_14;
    int field_18;
    int field_1c;
    TitleRect rect_20;
    int field_30;
    int field_34;
    unsigned char field_38;
    unsigned char field_39;
    unsigned char field_3a;
    unsigned char field_3b;
} TitleObj;

extern int g_29f98;
extern int g_2a250;
extern int g_2bb3c;
extern int g_2a254;
extern int g_2a270;
extern int g_2a3ac;
extern int g_2a3a0;
extern int g_2a39c;
extern unsigned int g_25a28;
extern int g_2a274;
extern int g_2a278;
extern int g_2a27c;
extern int g_2a280;
extern int g_2a9bc;
extern int g_2a9c0;
extern TitleObject *g_2a288[];
static TitleObject *g_2aa14;
static TitleObject *g_2aa2c;
static TitleObject *g_2a9c8;
static TitleObject *g_2a9cc;
static TitleObject *g_2a9d0;
static TitleObject *g_2a9f4;
static TitleObject *g_2aa48;
static TitleObject *g_2aa0c;
static TitleObject *g_2aa08;
static unsigned int g_2a9f0;
int title_0cce0(void);
int title_0c4a0(int a);
int title_0cd30(void);
void title_0c4e0(void);
void title_054f0(int a, int b);
int title_0c4d0(int a);
void title_09350(TitleObject *p);
void title_04ea0(int a);
extern TitleObject *g_2ab24;
extern TitleObject *g_2ab1c;
extern TitleObject *g_2ab34;
static int g_2aa54;
static int g_2aa60;
static int g_2aa90;
static int g_2aacc;
static int g_2a9d8;
static int g_2a9dc;
static int g_2a9e4;
static int g_2aa28;
static int g_2aa44;
static int g_2ab08;
extern int g_2ab20;
extern int g_2ab28;
extern int g_2ab38;
extern int g_2ab44;
extern unsigned short g_25f50[];
int title_06670(void);
void title_0c4f0(int a);
void title_0c2e0(void);

void title_0ff70(void)
{
    g_2aa14 = title_17ad0(0, 0, 0, 0x2017, 0);
    g_2aa14->unknown_034 = 2;
    g_2aa2c = title_17ad0(0, 0, 0, 0x2017, 0);
    g_2aa2c->unknown_034 = 0xf;
    g_2a9c8 = title_17ad0(0, 0, 0, 0x2017, 0);
    g_2a9c8->unknown_034 = 0x1a;
    g_2a9cc = title_17ad0(0, 0, 0, 0x2017, 0);
    g_2a9cc->unknown_034 = 0x1a;
    g_2a9d0 = title_17ad0(0, 0, 0, 0x2017, 0);
    g_2a9d0->unknown_034 = 0x1a;
    g_2a9f4 = title_17ad0(0, 0, 0, 0x2017, 0);
    g_2a9f4->unknown_034 = 3;
    g_2aa48 = title_17ad0(0, 0, 0, 0x2017, 0);
    g_2aa48->unknown_034 = 0xc;
    g_2aa48->unknown_054 &= 0x7fffffff;
    g_2aa0c = title_17ad0(0, 0, 0, 0x2017, 0);
    g_2aa0c->unknown_034 = 0xe;
    g_2aa0c->unknown_054 &= 0x7fffffff;
    g_2aa08 = title_17ad0(0, 0, 0, 0x2017, 0);
    g_2aa08->unknown_034 = 8;
    g_2a9c8->unknown_04a = 0x40;
    g_2a9cc->unknown_04a = 0x40;
    g_2a9d0->unknown_04a = 0x40;
}

void title_100e0(void)
{
    g_2a9c8->unknown_034 = 0x1a;
    g_2a9cc->unknown_034 = 0x1a;
    g_2a9d0->unknown_034 = 0x1a;
}

void title_10110(void)
{
    if (g_2aa14 != 0) {
        title_09350(g_2aa14);
        g_2aa14 = 0;
    }
    if (g_2aa2c != 0) {
        title_09350(g_2aa2c);
        g_2aa2c = 0;
    }
    if (g_2a9c8 != 0) {
        title_09350(g_2a9c8);
        g_2a9c8 = 0;
    }
    if (g_2a9cc != 0) {
        title_09350(g_2a9cc);
        g_2a9cc = 0;
    }
    if (g_2a9d0 != 0) {
        title_09350(g_2a9d0);
        g_2a9d0 = 0;
    }
    if (g_2a9f4 != 0) {
        title_09350(g_2a9f4);
        g_2a9f4 = 0;
    }
    if (g_2aa48 != 0) {
        title_09350(g_2aa48);
        g_2aa48 = 0;
    }
    if (g_2aa0c != 0) {
        title_09350(g_2aa0c);
        g_2aa0c = 0;
    }
    if (g_2aa08 != 0) {
        title_09350(g_2aa08);
        g_2aa08 = 0;
    }
    title_04ea0(0x10);
}

void title_10200(void)
{
    g_2aa14->unknown_04a = g_2a9f0;
    g_2aa2c->unknown_04a = g_2a9f0;
    g_2a9c8->unknown_04a = g_2a9f0;
    g_2a9cc->unknown_04a = g_2a9f0;
    g_2a9d0->unknown_04a = g_2a9f0;
    g_2a9f4->unknown_04a = g_2a9f0;
    g_2aa48->unknown_04a = g_2a9f0;
    g_2aa0c->unknown_04a = g_2a9f0;
    g_2aa08->unknown_04a = g_2a9f0;
    g_2a9d0->unknown_04a = g_2a9f0 >> 1;
    g_2a9cc->unknown_04a = g_2a9d0->unknown_04a;
    g_2a9c8->unknown_04a = g_2a9cc->unknown_04a;
    g_2a9cc->unknown_054 &= 0x7fffffff;
    g_2a9d0->unknown_054 &= 0x7fffffff;
}

void title_10ce0(void)
{
    switch (g_2aa54) {
    case 0: g_2a9c8->unknown_034 = 0x1a; break;
    case 1: g_2a9c8->unknown_034 = 0x10; break;
    case 2: g_2a9c8->unknown_034 = 0x11; break;
    case 3: g_2a9c8->unknown_034 = 0x12; break;
    case 4: g_2a9c8->unknown_034 = 0x13; break;
    case 5: g_2a9c8->unknown_034 = 0x13; break;
    case 6: g_2a9c8->unknown_034 = 0x14; break;
    case 7: g_2a9c8->unknown_034 = 0x15; break;
    case 8: g_2a9c8->unknown_034 = 0x16; break;
    case 9: g_2a9c8->unknown_034 = 0x17; break;
    case 10:
        if (g_2aa60) g_2a9c8->unknown_034 = 0x18;
        else g_2a9c8->unknown_034 = 0x10;
        break;
    case 11:
        if (g_2aa60) g_2a9c8->unknown_034 = 0x19;
        else g_2a9c8->unknown_034 = 0x10;
        break;
    case 12: /* same shape as cases 10 and 11; both arms happen to be 0x10 */
        if (g_2aa60) g_2a9c8->unknown_034 = 0x10;
        else g_2a9c8->unknown_034 = 0x10;
        break;
    }
    switch (g_2aa90) {
    case 0: g_2a9cc->unknown_034 = 0x1a; break;
    case 1: g_2a9cc->unknown_034 = 0x10; break;
    case 2: g_2a9cc->unknown_034 = 0x11; break;
    case 3: g_2a9cc->unknown_034 = 0x12; break;
    case 4: g_2a9cc->unknown_034 = 0x12; break;
    case 5: g_2a9cc->unknown_034 = 0x13; break;
    case 6: g_2a9cc->unknown_034 = 0x14; break;
    case 7: g_2a9cc->unknown_034 = 0x15; break;
    case 8: g_2a9cc->unknown_034 = 0x16; break;
    case 9: g_2a9cc->unknown_034 = 0x17; break;
    case 10: g_2a9cc->unknown_034 = 0x18; break;
    case 11: g_2a9cc->unknown_034 = 0x19; break;
    case 12: g_2a9cc->unknown_034 = 0x10; break;
    }
    switch (g_2aacc) {
    case 0: g_2a9d0->unknown_034 = 0x1a; break;
    case 1: g_2a9d0->unknown_034 = 0x10; break;
    case 2: g_2a9d0->unknown_034 = 0x11; break;
    case 3: g_2a9d0->unknown_034 = 0x12; break;
    case 4: g_2a9d0->unknown_034 = 0x12; break;
    case 5: g_2a9d0->unknown_034 = 0x13; break;
    case 6: g_2a9d0->unknown_034 = 0x14; break;
    case 7: g_2a9d0->unknown_034 = 0x15; break;
    case 8: g_2a9d0->unknown_034 = 0x16; break;
    case 9: g_2a9d0->unknown_034 = 0x17; break;
    case 10: g_2a9d0->unknown_034 = 0x18; break;
    case 11: g_2a9d0->unknown_034 = 0x19; break;
    case 12: g_2a9d0->unknown_034 = 0x10; break;
    }
}

void title_10fa0(void)
{
    g_2a9e4 = 0;
    title_06670();
    g_2ab08 = 0;
    g_2a9dc = 0;
    g_2aa28 = 0;
    g_2aa44 = 1;
    g_2a9d8 = 1;
}
