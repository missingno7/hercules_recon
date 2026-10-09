#include "title_engine.h"
#include "title_screen.h"

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
