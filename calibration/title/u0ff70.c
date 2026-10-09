#include "title_engine.h"
#include "title_screen.h"
#include "title_files.h"

/* Memory-card save record (0x3c bytes; same layout as TitleObj in r10ce0.c, filled by
   title_12070 and applied by title_121d0). Three slots at 0x2aa50, 0x2aa8c, 0x2aac8. */
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
    int rect_20[4];
    int field_30;
    int field_34;
    unsigned char field_38;
    unsigned char field_39;
    unsigned char field_3a;
    unsigned char field_3b;
} TitleObj;
static TitleObj g_2aa50[3];
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

static int g_2aa04;
static int g_2aa24;
static int g_2aa30;
static int g_2aa4c;
static int g_2a9ec;
static int g_2ab04;
static int g_2ab14;
extern char g_29128[];
void title_016e0(const char *format, ...);
void title_01de0(int a, int b, int c, int d, int e);
void title_04b70(int c, int d);
void title_05e90();
void title_05f10(void *target);
void title_07510();
void title_0c330();
void title_0c3f0();
void title_0c450();
void title_0c560(int a, int b);
void title_0c5a0(void);
void title_0c6d0(int a);
void title_0c8b0();
void title_10fa0(void);
void title_10fe0(int a);
void title_1d790(int a);

static int g_2ab10;
static int g_2a9e8;
static int g_2aa20;
void title_1c500();

void title_102f0(TitleProc *out)
{
    int local;
    int x = ((ScreenContext *)g_engine_interface.context_004)->cursor_5c;
    unsigned long buttons = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5e;

    title_016e0("\n TICK %d", g_2ab10);
    g_2ab10 = g_2ab10 + 1;
    g_2aa24 = 0;

    switch (out->state_04) {
    case 1:
        title_01de0(0x140, 0, g_2a9f0, 0, 0);
        switch (g_2aa30) {
        case 0:
            if (g_2a9f0 < 0x80) {
                g_2a9f0 = g_2a9f0 + 8;
            } else {
                g_2aa30 = 1;
            }
            title_10200();
            g_2aa2c->unknown_054 &= 0x7fffffff;
            g_2a9c8->unknown_054 &= 0x7fffffff;
            g_2a9cc->unknown_054 &= 0x7fffffff;
            g_2a9d0->unknown_054 &= 0x7fffffff;
            g_2aa08->unknown_034 = 0x1e;
            break;
        case 1:
            title_10fe0(0);
            if (g_2ab04 == 0 && (buttons & 0x1000)) {
                g_2aa30 = 2;
                title_054f0(0x303, 0);
            }
            if (g_2aa04 != 0) {
                g_2aa30 = 2;
                title_054f0(0x303, 0);
            }
            break;
        case 2:
            title_10200();
            if (g_2a9f0 > 0) {
                g_2a9f0 = g_2a9f0 - 8;
            } else {
                out->state_04 = 0xfffe;
                out->delay_08 = 1;
                return;
            }
            break;
        case 3:
            title_10fe0(2);
            if (g_2ab04 == 0 && (buttons & 0x1000)) {
                g_2aa30 = 2;
                title_054f0(0x303, 0);
            }
            if (g_2aa04 != 0) {
                g_2aa30 = 4;
            }
            break;
        case 4:
            g_2aa04 = 0;
            g_2aa30 = 1;
            title_10200();
            title_10fa0();
            g_2aa2c->unknown_054 &= 0x7fffffff;
            g_2a9c8->unknown_054 &= 0x7fffffff;
            g_2aa08->unknown_034 = 0x1e;
            g_2a9e4 = 0;
            g_2a9f4->unknown_054 &= 0x7fffffff;
            break;
        }
        out->state_04 = 1;
        out->delay_08 = 1;
        return;
    case 0xfffe:
        title_0c4e0();
        title_10110();
        title_0c5a0();
        title_0c4e0();
        title_05e90(title_1c500, 0);
        title_05f10(out);
        return;
    case 0xffff:
        title_0c560(0, 4);
        g_2aa04 = 0;
        g_2a9e8 = 0;
        g_2aa20 = 0xf;
        title_0c560(0, 2);
        title_1d790(1);
        local = 0;
        title_0c450(&local, 0x15000, 0);
        title_0c3f0(g_29128, g_screen_files[8], local, 0x14312);
        title_0c8b0(local + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(local);
        title_04b70(0x10, 0);
        g_2a9f0 = 0;
        g_2aa30 = 0;
        title_10fa0();
        title_0ff70();
        title_10200();
        g_2ab04 = 0;
        break;
    default:
        return;
    }
    out->state_04 = 1;
    out->delay_08 = 1;
}

void title_10670(TitleProc *out)
{
    int local;
    int x = ((ScreenContext *)g_engine_interface.context_004)->cursor_5c;
    unsigned long buttons = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5e;

    g_2aa24 = 1;
    title_016e0("\n TICK %d", g_2ab14);
    g_2ab14 = g_2ab14 + 1;

    switch (out->state_04) {
    case 1:
        title_01de0(0x140, 0, g_2a9f0, 0, 0);
        switch (g_2aa30) {
        case 0:
            if (g_2a9f0 < 0x80) {
                g_2a9f0 = g_2a9f0 + 8;
            } else {
                g_2aa30 = 1;
            }
            title_10200();
            g_2aa2c->unknown_054 &= 0x7fffffff;
            g_2a9c8->unknown_054 &= 0x7fffffff;
            g_2a9cc->unknown_054 &= 0x7fffffff;
            g_2a9d0->unknown_054 &= 0x7fffffff;
            g_2aa08->unknown_034 = 0x1e;
            break;
        case 1:
            title_10fe0(1);
            if (buttons & 0x1000) {
                g_2aa30 = 2;
                title_054f0(0x303, 0);
            }
            if (g_2aa04 != 0) {
                g_2aa30 = 2;
                title_054f0(0x303, 0);
            }
            break;
        case 2:
            title_10200();
            if (g_2a9f0 > 0) {
                g_2a9f0 = g_2a9f0 - 8;
            } else {
                out->state_04 = 0xfffe;
                out->delay_08 = 1;
                return;
            }
            break;
        case 3:
            title_10fe0(2);
            if (g_2ab04 == 0 && (buttons & 0x1000)) {
                g_2aa30 = 2;
                title_054f0(0x303, 0);
            }
            if (g_2aa04 != 0) {
                g_2aa30 = 4;
            }
            break;
        case 4:
            g_2aa04 = 0;
            g_2aa30 = 1;
            title_10200();
            g_2aa14->unknown_034 = 1;
            title_10fa0();
            g_2aa2c->unknown_054 &= 0x7fffffff;
            g_2a9c8->unknown_054 &= 0x7fffffff;
            g_2aa08->unknown_034 = 0x1e;
            g_2a9e4 = 0;
            g_2a9f4->unknown_054 &= 0x7fffffff;
            break;
        }
        break;
    case 0xfffe:
        title_0c4e0();
        title_10110();
        title_0c5a0();
        title_0c4e0();
        title_05e90(title_07510, 0);
        title_05f10(out);
        return;
    case 0xffff:
        title_0c560(0, 4);
        ((ScreenContext *)g_engine_interface.context_004)->unknown_00[0] = 0;
        g_2aa04 = 0;
        g_2a9ec = 0;
        g_2aa4c = 0xf;
        title_0c560(0, 2);
        title_1d790(1);
        local = 0;
        title_0c450(&local, 0x15000, 0);
        title_0c3f0(g_29128, g_screen_files[8], local, 0x14312);
        title_0c8b0(local + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(local);
        title_04b70(0x10, 0);
        g_2a9f0 = 0;
        g_2aa30 = 0;
        title_10fa0();
        title_0ff70();
        title_10200();
        g_2aa14->unknown_034 = 1;
        break;
    default:
        return;
    }
    out->state_04 = 1;
    out->delay_08 = 1;
}

static int g_2ab18;
static int g_2aa18;
static unsigned int g_2aa1c;
void title_164b0();

void title_109f0(TitleProc *out)
{
    int local;
    int x = ((ScreenContext *)g_engine_interface.context_004)->cursor_5c;
    unsigned long buttons = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5e;

    g_2aa24 = 2;
    title_016e0("\n TICK %d", g_2ab18);
    g_2ab18 = g_2ab18 + 1;

    switch (out->state_04) {
    case 1:
        title_01de0(0x140, 0, g_2a9f0, 0, 0);
        switch (g_2aa30) {
        case 0:
            if (g_2a9f0 < 0x80) {
                g_2a9f0 = g_2a9f0 + 8;
            } else {
                g_2aa1c = g_2aa1c + 1;
                if (g_2aa1c > 0x78) {
                    g_2aa30 = 1;
                }
            }
            title_10200();
            break;
        case 1:
            title_10fe0(2);
            if (buttons & 0x1000) {
                g_2aa30 = 2;
                title_054f0(0x303, 0);
            }
            if (g_2aa04 != 0) {
                g_2aa30 = 2;
                title_054f0(0x303, 0);
            }
            break;
        case 2:
            title_10200();
            if (g_2a9f0 > 0) {
                g_2a9f0 = g_2a9f0 - 8;
            } else {
                out->state_04 = 0xfffe;
                out->delay_08 = 1;
                return;
            }
            break;
        }
        out->state_04 = 1;
        out->delay_08 = 1;
        return;
    case 0xfffe:
        title_0c4e0();
        title_10110();
        title_0c5a0();
        title_0c4e0();
        title_05e90(title_164b0, 0);
        title_05f10(out);
        return;
    case 0xffff:
        title_0c560(0, 4);
        ((ScreenContext *)g_engine_interface.context_004)->unknown_00[0] = 0;
        g_2aa04 = 0;
        g_2aa18 = 0;
        g_2aa1c = 0xf;
        title_0c560(0, 2);
        title_1d790(1);
        local = 0;
        title_0c450(&local, 0x15000, 0);
        title_0c3f0(g_29128, g_screen_files[8], local, 0x14312);
        title_0c8b0(local + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(local);
        title_04b70(0x10, 0);
        g_2a9f0 = 0;
        g_2aa30 = 0;
        title_10fa0();
        title_0ff70();
        title_10200();
        g_2aa2c->unknown_054 &= 0x7fffffff;
        g_2a9c8->unknown_054 &= 0x7fffffff;
        g_2a9cc->unknown_054 &= 0x7fffffff;
        g_2a9d0->unknown_054 &= 0x7fffffff;
        g_2aa14->unknown_054 &= 0x7fffffff;
        g_2aa08->unknown_034 = 0x1e;
        g_2aa08->unknown_054 &= 0x7fffffff;
        break;
    default:
        return;
    }
    out->state_04 = 1;
    out->delay_08 = 1;
}

void title_10ce0(void)
{
    switch (g_2aa50[0].field_04) {
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
        if (g_2aa50[0].field_10) g_2a9c8->unknown_034 = 0x18;
        else g_2a9c8->unknown_034 = 0x10;
        break;
    case 11:
        if (g_2aa50[0].field_10) g_2a9c8->unknown_034 = 0x19;
        else g_2a9c8->unknown_034 = 0x10;
        break;
    case 12: /* same shape as cases 10 and 11; both arms happen to be 0x10 */
        if (g_2aa50[0].field_10) g_2a9c8->unknown_034 = 0x10;
        else g_2a9c8->unknown_034 = 0x10;
        break;
    }
    switch (g_2aa50[1].field_04) {
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
    switch (g_2aa50[2].field_04) {
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


static int g_2aa34;
static int g_2aa38[3];
static int g_2ab0c;
unsigned int title_06350(int a);
int title_063c0(int a);
void title_066c0(void);
int title_066e0(void);
int title_06840(int a, TitleObj *rec, int size, int b, int c);
int title_06a50(int a, TitleObj *rec, int size);
void title_01dd0(int a);
void title_12070(TitleObj *rec);
void title_121d0(TitleObj *rec);

void title_10fe0(int command)
{
    unsigned long pad = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5c;
    unsigned long buttons = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5e;

    title_016e0("\n LSGameMemoryCardLoop LSStatus %d\n\n", g_2a9e4);
    if (g_2a9e4 != 0) {
        g_2a9d8 = 1;
    }
    if (g_2a9e4 != 2) {
        g_2aa44 = 1;
    }

    switch (g_2a9e4) {
    case 0:
        g_2aa48->unknown_054 &= 0x7fffffff;
        g_2aa0c->unknown_054 &= 0x7fffffff;
        g_2a9c8->unknown_04a = 0x40;
        g_2a9cc->unknown_04a = 0x40;
        g_2a9d0->unknown_04a = 0x40;
        switch (title_06350(0)) {
        case 0:
            g_2aa28 = 0;
            g_2a9f4->unknown_034 = 5;
            g_2a9f4->unknown_054 |= 0x80000000;
            if (g_2a9d8 != 0) {
                if (g_2aa24 != 2) {
                    title_0c4e0();
                    title_0c4d0(0x4d);
                }
                g_2a9d8 = 0;
                g_2aa2c->unknown_054 &= 0x7fffffff;
                g_2a9c8->unknown_054 &= 0x7fffffff;
                g_2aa08->unknown_034 = 0x1e;
            }
            title_016e0("\n No card detected");
            return;
        case 1:
            g_2a9e4 = 0xa;
            if (g_2a9d8 == 0) {
                title_0c4e0();
            }
            g_2a9f4->unknown_034 = 3;
            g_2a9f4->unknown_054 |= 0x80000000;
            title_016e0("\n Check Card Format");
            return;
        case 2:
            if (command == 2) {
                g_2aa04 = 1;
                return;
            }
            g_2a9f4->unknown_034 = 3;
            g_2a9f4->unknown_054 |= 0x80000000;
            title_016e0("\n Detected a newly connected card and marked it");
            return;
        case 3:
            if (command == 2) {
                g_2aa04 = 1;
                return;
            }
            g_2aa28 = 0;
            g_2a9f4->unknown_034 = 3;
            g_2a9f4->unknown_054 |= 0x80000000;
            title_016e0("\n Communication error happened");
            return;
        }
        break;

    case 10:
        switch (title_063c0(0)) {
        case 0:
        case 1:
        case 3:
            g_2a9e4 = 0;
            return;
        case 2:
            title_016e0("\n Detected a formatted card");
            g_2a9f4->unknown_054 &= 0x7fffffff;
            g_2a9e4 = command == 2 ? 0xb : 3;
            return;
        case 4:
            g_2ab0c = 0;
            if (command == 2) {
                g_2aa04 = 1;
                return;
            }
            title_016e0("\n Detected a unformatted card");
            if (command == 1) {
                g_2a9f4->unknown_034 = 0x1d;
                g_2a9f4->unknown_054 |= 0x80000000;
                return;
            }
            g_2a9f4->unknown_034 = 4;
            g_2a9f4->unknown_054 |= 0x80000000;
            g_2ab0c = 0;
            g_2a9e4 = 4;
            return;
        }
        break;

    case 4:
        switch (title_06350(0)) {
        case 0:
            g_2aa48->unknown_054 &= 0x7fffffff;
            g_2aa0c->unknown_054 &= 0x7fffffff;
            g_2a9f4->unknown_034 = 3;
            g_2a9e4 = 0;
            title_016e0("\n No card detected");
            return;
        case 1:
            title_016e0("\n Detected a formatted card");
            break;
        case 2:
            g_2aa48->unknown_054 &= 0x7fffffff;
            g_2aa0c->unknown_054 &= 0x7fffffff;
            g_2a9f4->unknown_034 = 3;
            g_2a9e4 = 0;
            title_016e0("\n Detected a newly connected card and marked it");
            return;
        case 3:
            g_2aa48->unknown_054 &= 0x7fffffff;
            g_2aa0c->unknown_054 &= 0x7fffffff;
            g_2a9f4->unknown_034 = 6;
            g_2a9e4 = 0;
            title_016e0("\n Communication error happened");
            return;
        case 4:
            g_2aa48->unknown_054 &= 0x7fffffff;
            g_2aa0c->unknown_054 &= 0x7fffffff;
            g_2a9f4->unknown_034 = 3;
            g_2a9e4 = 0;
            title_016e0("\n Detected an unformatted card");
            return;
        }
        g_2aa08->unknown_034 = 8;
        g_2aa48->unknown_054 |= 0x80000000;
        g_2aa0c->unknown_054 |= 0x80000000;
        switch (g_2ab0c) {
        case 0:
            g_2aa48->unknown_04a = 0x40;
            g_2aa0c->unknown_04a = 0x80;
            break;
        case 1:
            g_2aa48->unknown_04a = 0x80;
            g_2aa0c->unknown_04a = 0x40;
            break;
        }
        if (pad == 0) {
            g_2a9dc = 0;
        }
        if (g_2a9dc == 0) {
            if (g_2ab0c == 1 && (pad & 0x20)) {
                g_2ab0c = 0;
                g_2a9dc = 0xf;
                title_054f0(0x301, 0);
            }
            if (g_2ab0c == 0 && (pad & 0x80)) {
                g_2ab0c = 1;
                g_2a9dc = 0xf;
                title_054f0(0x300, 0);
            }
        } else {
            g_2a9dc = g_2a9dc - 1;
        }
        if (buttons & 0x4000) {
            title_054f0(0x302, 0);
            if (g_2ab0c == 0) {
                g_2aa48->unknown_054 &= 0x7fffffff;
                g_2aa0c->unknown_054 &= 0x7fffffff;
                g_2aa04 = 1;
                return;
            }
            g_2aa48->unknown_054 &= 0x7fffffff;
            g_2aa0c->unknown_054 &= 0x7fffffff;
            g_2a9f4->unknown_034 = 0x1b;
            g_2a9e4 = 7;
            g_2aa34 = 0;
            return;
        }
        break;

    case 7:
        if (g_2aa34 < 0x3c) {
            g_2aa34 = g_2aa34 + 1;
            return;
        }
        if (title_066e0() != 1) {
            g_2a9f4->unknown_034 = 0xd;
            return;
        }
        g_2a9f4->unknown_034 = 3;
        g_2a9e4 = 0;
        return;

    case 3:
        g_2a9c8->unknown_04a = 0x40;
        g_2a9cc->unknown_04a = 0x40;
        g_2a9d0->unknown_04a = 0x40;
        if (title_06670() == -1) {
            g_2a9f4->unknown_034 = 3;
            g_2a9e4 = 0;
        }
        g_2aa38[0] = title_06a50(0x41, &g_2aa50[0], 0x3c);
        if (g_2aa38[0] == 6) {
            if (g_2aa24 == 1) {
                g_2aa2c->unknown_054 &= 0x7fffffff;
                g_2a9c8->unknown_054 &= 0x7fffffff;
                g_2a9cc->unknown_054 &= 0x7fffffff;
                g_2a9d0->unknown_054 &= 0x7fffffff;
                g_2a9f4->unknown_054 |= 0x80000000;
                g_2aa30 = 3;
                g_2aa08->unknown_034 = 0x1e;
                g_2a9f4->unknown_034 = 0x20;
                title_0c4e0();
                title_0c4d0(0x4c);
                g_2a9e4 = 0xd;
                return;
            }
            g_2aa38[0] = title_06840(0x44, &g_2aa50[0], 0x3c, 1, 1);
            title_01dd0(0x44);
            if (g_2aa38[0] == 3) {
                g_2aa50[0].field_04 = 0;
                g_2aa2c->unknown_054 |= 0x80000000;
                g_2a9c8->unknown_054 |= 0x80000000;
                g_2aa08->unknown_034 = 8;
            } else {
                g_2aa2c->unknown_054 &= 0x7fffffff;
                g_2a9c8->unknown_054 &= 0x7fffffff;
                g_2a9cc->unknown_054 &= 0x7fffffff;
                g_2a9d0->unknown_054 &= 0x7fffffff;
                g_2aa08->unknown_034 = 0x1e;
                g_2aa30 = 3;
                g_2a9e4 = 0xd;
                return;
            }
        }
        title_10ce0();
        g_2a9e4 = 2;
        return;

    case 1:
        g_2a9c8->unknown_04a = 0x40;
        g_2a9cc->unknown_04a = 0x40;
        g_2a9d0->unknown_04a = 0x40;
        return;

    case 2:
        if (command == 1 && g_2aa44 != 0) {
            title_0c4e0();
            title_0c4d0(0x49);
            g_2aa44 = 0;
        }
        switch (title_06350(0)) {
        case 0:
            title_100e0();
            g_2a9f4->unknown_034 = 3;
            g_2a9e4 = 0;
            title_016e0("\n No card detected");
            return;
        case 1:
            title_016e0("\n Detected a formatted card");
            break;
        case 2:
            title_100e0();
            g_2a9f4->unknown_034 = 3;
            g_2a9e4 = 0;
            title_016e0("\n Detected a newly connected card and marked it");
            return;
        case 3:
            title_100e0();
            g_2a9f4->unknown_034 = 3;
            g_2a9e4 = 0;
            title_016e0("\n Communication error happened");
            return;
        case 4:
            title_100e0();
            g_2a9f4->unknown_034 = 3;
            g_2a9e4 = 0;
            title_016e0("\n Detected an unformatted card");
            return;
        }
        title_066c0();
        title_016e0("\n SlotChoice %d", g_2ab08);
        if (pad == 0) {
            g_2a9dc = 0;
        }
        g_2aa2c->unknown_054 |= 0x80000000;
        g_2a9c8->unknown_054 |= 0x80000000;
        g_2aa08->unknown_034 = 8;
        if (command == 0) {
            if (buttons & 0x4000) {
                if (g_2aa38[g_2ab08] == 8) {
                    g_2a9f4->unknown_034 = 0x1c;
                    g_2a9f4->unknown_054 |= 0x80000000;
                    g_2ab0c = 0;
                    g_2a9e4 = 8;
                    g_2aa34 = 0;
                } else {
                    g_2a9f4->unknown_034 = 0xa;
                    g_2a9f4->unknown_054 |= 0x80000000;
                    g_2a9e4 = 6;
                    g_2ab04 = 1;
                    g_2aa34 = 0;
                }
                title_054f0(0x302, 0);
            }
        } else {
            if (buttons & 0x4000) {
                if (g_2aa38[g_2ab08] == 8) {
                    title_121d0(&g_2aa50[g_2ab08]);
                    g_2aa04 = 1;
                    title_054f0(0x302, 0);
                } else {
                    title_0c4e0();
                    title_0c4d0(0x4c);
                    title_054f0(0x303, 0);
                }
            }
        }
        switch (g_2ab08) {
        case 0:
            g_2a9c8->unknown_04a = 0x80;
            g_2a9cc->unknown_04a = 0x40;
            g_2a9d0->unknown_04a = 0x40;
            return;
        case 1:
            g_2a9c8->unknown_04a = 0x40;
            g_2a9cc->unknown_04a = 0x80;
            g_2a9d0->unknown_04a = 0x40;
            return;
        case 2:
            g_2a9c8->unknown_04a = 0x40;
            g_2a9cc->unknown_04a = 0x40;
            g_2a9d0->unknown_04a = 0x80;
            return;
        }
        break;

    case 6:
        if (g_2aa34 < 0x3c) {
            g_2aa34 = g_2aa34 + 1;
            return;
        }
        switch (g_2ab08) {
        case 0:
            title_12070(&g_2aa50[0]);
            g_2aa38[0] = title_06840(0x41, &g_2aa50[0], 0x3c, 1, 0);
            break;
        case 1:
            title_12070(&g_2aa50[1]);
            g_2aa38[1] = title_06840(0x42, &g_2aa50[1], 0x3c, 1, 0);
            break;
        case 2:
            title_12070(&g_2aa50[2]);
            g_2aa38[2] = title_06840(0x43, &g_2aa50[2], 0x3c, 1, 0);
            break;
        }
        g_2aa50[0].field_04 = (char)((ScreenContext *)g_engine_interface.context_004)->unknown_00[0] + 1;
        title_10ce0();
        g_2aa34 = -60;
        g_2a9e4 = 5;
        switch (g_2aa38[g_2ab08]) {
        case 1:
            g_2aa34 = -240;
            g_2a9f4->unknown_034 = 0xb;
            g_2a9f4->unknown_054 |= 0x80000000;
            return;
        case 3:
            g_2aa38[g_2ab08] = 8;
            g_2a9f4->unknown_034 = 9;
            g_2a9f4->unknown_054 |= 0x80000000;
            title_10ce0();
            return;
        case 4:
            g_2aa34 = -240;
            g_2a9f4->unknown_034 = 7;
            g_2a9f4->unknown_054 |= 0x80000000;
            return;
        }
        break;

    case 8:
        switch (title_06350(0)) {
        case 0:
            title_100e0();
            g_2a9f4->unknown_034 = 3;
            g_2a9e4 = 0;
            title_016e0("\n No card detected");
            break;
        case 1:
            title_016e0("\n Detected a formatted card");
            break;
        case 2:
            title_100e0();
            g_2a9f4->unknown_034 = 3;
            g_2a9e4 = 0;
            title_016e0("\n Detected a newly connected card and marked it");
            break;
        case 3:
            title_100e0();
            g_2a9f4->unknown_034 = 3;
            g_2a9e4 = 0;
            title_016e0("\n Communication error happened");
            break;
        case 4:
            title_100e0();
            g_2a9f4->unknown_034 = 3;
            g_2a9e4 = 0;
            title_016e0("\n Detected an unformatted card");
            break;
        }
        g_2aa48->unknown_054 |= 0x80000000;
        g_2aa0c->unknown_054 |= 0x80000000;
        g_2aa08->unknown_034 = 8;
        switch (g_2ab0c) {
        case 0:
            g_2aa48->unknown_04a = 0x40;
            g_2aa0c->unknown_04a = 0x80;
            break;
        case 1:
            g_2aa48->unknown_04a = 0x80;
            g_2aa0c->unknown_04a = 0x40;
            break;
        }
        if (pad == 0) {
            g_2a9dc = 0;
        }
        if (g_2a9dc == 0) {
            if (g_2ab0c == 1 && (pad & 0x20)) {
                g_2ab0c = 0;
                g_2a9dc = 0xf;
                title_054f0(0x301, 0);
            }
            if (g_2ab0c == 0 && (pad & 0x80)) {
                g_2ab0c = 1;
                g_2a9dc = 0xf;
                title_054f0(0x300, 0);
            }
        } else {
            g_2a9dc = g_2a9dc - 1;
        }
        if (buttons & 0x4000) {
            title_054f0(0x302, 0);
            if (g_2ab0c == 0) {
                g_2a9f4->unknown_054 &= 0x7fffffff;
                g_2aa48->unknown_054 &= 0x7fffffff;
                g_2aa0c->unknown_054 &= 0x7fffffff;
                g_2a9e4 = 2;
                return;
            }
            g_2aa48->unknown_054 &= 0x7fffffff;
            g_2aa0c->unknown_054 &= 0x7fffffff;
            g_2a9f4->unknown_034 = 0xa;
            g_2ab04 = 1;
            g_2a9e4 = 9;
            g_2aa34 = 0;
            return;
        }
        break;

    case 9:
        if (g_2aa34 < 0x3c) {
            g_2aa34 = g_2aa34 + 1;
            return;
        }
        switch (g_2ab08) {
        case 0:
            title_12070(&g_2aa50[0]);
            g_2aa38[0] = title_06840(0x41, &g_2aa50[0], 0x3c, 0, 0);
            break;
        case 1:
            title_12070(&g_2aa50[1]);
            g_2aa38[1] = title_06840(0x42, &g_2aa50[1], 0x3c, 0, 0);
            break;
        case 2:
            title_12070(&g_2aa50[2]);
            g_2aa38[2] = title_06840(0x43, &g_2aa50[2], 0x3c, 0, 0);
            break;
        }
        g_2aa50[0].field_04 = (char)((ScreenContext *)g_engine_interface.context_004)->unknown_00[0] + 1;
        title_10ce0();
        g_2aa34 = -60;
        g_2a9e4 = 5;
        switch (g_2aa38[g_2ab08]) {
        case 1:
            g_2aa34 = -240;
            g_2a9f4->unknown_034 = 0xb;
            g_2a9f4->unknown_054 |= 0x80000000;
            return;
        case 3:
            g_2aa38[g_2ab08] = 8;
            g_2a9f4->unknown_034 = 9;
            g_2a9f4->unknown_054 |= 0x80000000;
            title_10ce0();
            return;
        case 4:
            g_2aa34 = -240;
            g_2a9f4->unknown_034 = 7;
            g_2a9f4->unknown_054 |= 0x80000000;
            return;
        }
        break;

    case 5:
        g_2ab04 = 0;
        if (g_2aa34 < 0) {
            g_2aa34 = g_2aa34 + 1;
            return;
        }
        g_2a9f4->unknown_054 &= 0x7fffffff;
        g_2a9e4 = 2;
        return;

    case 11:
        g_2aa38[0] = title_06a50(0x41, &g_2aa50[0], 0x3c);
        if (g_2aa24 == 2 && g_2aa38[0] == 8) {
            title_121d0(&g_2aa50[0]);
            ((ScreenContext *)g_engine_interface.context_004)->unknown_00[0] = 0;
            ((ScreenContext *)g_engine_interface.context_004)->unknown_00[0x10] = 0;
        }
        g_2aa38[0] = title_06840(0x44, &g_2aa50[0], 0x3c, 1, 1);
        title_01dd0(0x44);
        if (g_2aa38[0] == 3) {
            g_2aa04 = 1;
            return;
        }
        if (g_2aa38[0] == 1) {
            g_2aa38[0] = title_06a50(0x41, &g_2aa50[0], 0x3c);
            if (g_2aa38[0] == 8) {
                g_2aa04 = 1;
                return;
            }
            g_2a9e4 = 0xd;
            g_2aa34 = 0;
            return;
        }
        g_2a9e4 = 0;
        return;

    case 13:
        switch (title_06350(0)) {
        default:
            if (g_2aa24 == 1) {
                g_2a9f4->unknown_034 = 0x20;
            } else if (g_2aa24 == 2) {
                g_2a9f4->unknown_034 = 0xb;
            } else if (g_2aa24 == 0) {
                g_2a9f4->unknown_034 = 0x21;
            }
            g_2aa08->unknown_054 |= 0x80000000;
            g_2a9f4->unknown_054 |= 0x80000000;
            return;
        case 0:
        case 2:
        case 3:
        case 4:
            title_100e0();
            g_2a9e4 = 0;
            title_016e0("\n No card detected");
            g_2a9f4->unknown_054 &= 0x7fffffff;
            break;
        }
        break;
    }
}
