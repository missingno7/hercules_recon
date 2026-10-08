/* TITLE.DLL lane w20 region: 10 functions, ascending RVA. Shared types, externs and
 * prototypes first. Field names are offsets, not recovered types. */
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

extern TitleObject *g_2abd0;
extern int g_2abd4;
extern TitleObject *g_2abd8;
extern int g_2abdc;
extern TitleObject *g_2abf0;
extern int g_2abf4;
extern int g_2abfc;
extern unsigned short g_25f50[];
extern TitleObject *g_2ac0c;
extern int g_2ac08;
extern int g_2ac10;
extern int g_2ac1c;
extern int g_2ac28;
extern TitleObject *g_2ac04;
extern TitleObject *g_2ac18;
extern TitleObject *g_2ac2c;
extern TitleObject *g_2ac34;
extern TitleObject *g_2ac48;
extern int g_2ac30;
extern int g_2ac38;
extern int g_2ac4c;
extern int g_2ac54;
extern TitleObject *g_2ac58;
extern TitleObject *g_2ac64;
extern TitleObject *g_2ac70;
extern int g_2ac5c;
extern int g_2ac68;
extern int g_2ac74;
extern int g_2ac80;
extern TitleObject *g_2ac84;
extern TitleObject *g_2ac8c;
extern TitleObject *g_2aca0;
extern int g_2ac88;
extern int g_2ac90;
extern int g_2aca4;
extern int g_2acac;

TitleObject *title_17ad0(int a0, int a1, int a2, int size, int a4);
int title_0c4f0(int a);

void title_14120(void)
{
    g_2abf4 = 10;
    g_2abd0 = title_17ad0(0, 0, 0, 0x200c, 0);
    g_2abd0->unknown_034 = 1;
    g_2abd0->unknown_054 |= 5;
    g_2abd0->unknown_04a = 0;
    g_2abf0 = title_17ad0(0, 0, 0, 0x200c, 0);
    g_2abf0->unknown_034 = 1;
    g_2abf0->unknown_054 |= 6;
    g_2abf0->unknown_04a = 0;
    g_2abd0->unknown_023 = 6;
    g_2abd0->unknown_03e = 10;
    g_2abf0->unknown_023 = 6;
    g_2abf0->unknown_03e = 20;
    g_2abf0->unknown_000 = 0x10000;
    g_2abf0->unknown_004 = 0x10000;
    g_2abd8 = title_17ad0(0, 0, 0, 0x200c, 0);
    g_2abd8->unknown_034 = 2;
    g_2abd8->unknown_054 |= 0x10;
    g_2abd8->unknown_000 = 0x640000;
    g_2abd8->unknown_004 = 0x280000;
    g_2abdc = 0;
    g_2abd4 = 0;
}

void title_14230(void)
{
    unsigned short code;

    g_2abfc ^= 1;
    if (g_2abfc != 0) {
        return;
    }
    switch (g_2abdc) {
    case 0:
        break;
    case 1:
        g_2abd8->unknown_034++;
        if (g_2abd8->unknown_034 != 0x10) {
            return;
        }
        g_2abdc = 2;
        return;
    case 2:
        g_2abf4 = 0xc;
        g_2abdc = 3;
        return;
    default:
        return;
    }
    g_2abd8->unknown_034 = g_25f50[g_2abd4] + 1;
    g_2abd4++;
    code = g_25f50[g_2abd4];
    if (code == 0xa && g_2abf4 == 0xa) {
        g_2abf4 = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    g_2abdc = 1;
}

void title_14790(void)
{
    unsigned short code;

    g_2ac28 ^= 1;
    if (g_2ac28 != 0) {
        return;
    }
    switch (g_2ac10) {
    case 0:
        break;
    case 1:
        g_2ac0c->unknown_034++;
        if (g_2ac0c->unknown_034 != 0x10) {
            return;
        }
        g_2ac10 = 2;
        return;
    case 2:
        g_2ac1c = 0xc;
        g_2ac10 = 3;
        return;
    default:
        return;
    }
    g_2ac0c->unknown_034 = g_25f50[g_2ac08] + 1;
    g_2ac08++;
    code = g_25f50[g_2ac08];
    if (code == 0xa && g_2ac1c == 0xa) {
        g_2ac1c = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    g_2ac10 = 1;
}

void title_14860(void)
{
    g_2ac1c = 10;
    g_2ac04 = title_17ad0(0, 0, 0, 0x200d, 0);
    g_2ac04->unknown_034 = 1;
    g_2ac04->unknown_054 |= 5;
    g_2ac04->unknown_04a = 0;
    g_2ac18 = title_17ad0(0, 0, 0, 0x200d, 0);
    g_2ac18->unknown_034 = 1;
    g_2ac18->unknown_054 |= 6;
    g_2ac18->unknown_04a = 0;
    g_2ac04->unknown_023 = 6;
    g_2ac04->unknown_03e = 10;
    g_2ac18->unknown_023 = 6;
    g_2ac18->unknown_03e = 20;
    g_2ac04->unknown_000 = 0;
    g_2ac04->unknown_004 = 0xfff00000;
    g_2ac18->unknown_000 = 0x10000;
    g_2ac18->unknown_004 = 0xfff10000;
    g_2ac0c = title_17ad0(0, 0, 0, 0x200d, 0);
    g_2ac0c->unknown_034 = 2;
    g_2ac0c->unknown_000 = 0xff9c0000;
    g_2ac0c->unknown_004 = 0x640000;
    g_2ac10 = 0;
    g_2ac08 = 0;
}

void title_14e10(void)
{
    g_2ac4c = 10;
    g_2ac2c = title_17ad0(0, 0, 0, 0x200e, 0);
    g_2ac2c->unknown_034 = 1;
    g_2ac2c->unknown_054 |= 5;
    g_2ac2c->unknown_04a = 0;
    g_2ac48 = title_17ad0(0, 0, 0, 0x200e, 0);
    g_2ac48->unknown_034 = 1;
    g_2ac48->unknown_054 |= 6;
    g_2ac48->unknown_04a = 0;
    g_2ac2c->unknown_023 = 6;
    g_2ac2c->unknown_03e = 10;
    g_2ac48->unknown_023 = 6;
    g_2ac48->unknown_03e = 20;
    g_2ac2c->unknown_000 = 0;
    g_2ac2c->unknown_004 = 0xfff00000;
    g_2ac48->unknown_000 = 0x10000;
    g_2ac48->unknown_004 = 0xfff10000;
    g_2ac34 = title_17ad0(0, 0, 0, 0x200e, 0);
    g_2ac34->unknown_034 = 2;
    g_2ac34->unknown_000 = 0xff9c0000;
    g_2ac34->unknown_004 = 0x640000;
    g_2ac38 = 0;
    g_2ac30 = 0;
}

void title_14f20(void)
{
    unsigned short code;

    g_2ac54 ^= 1;
    if (g_2ac54 != 0) {
        return;
    }
    switch (g_2ac38) {
    case 0:
        break;
    case 1:
        g_2ac34->unknown_034++;
        if (g_2ac34->unknown_034 != 0x10) {
            return;
        }
        g_2ac38 = 2;
        return;
    case 2:
        g_2ac4c = 0xc;
        g_2ac38 = 3;
        return;
    default:
        return;
    }
    g_2ac34->unknown_034 = g_25f50[g_2ac30] + 1;
    g_2ac30++;
    code = g_25f50[g_2ac30];
    if (code == 0xa && g_2ac4c == 0xa) {
        g_2ac4c = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    g_2ac38 = 1;
}

void title_15480(void)
{
    g_2ac74 = 10;
    g_2ac58 = title_17ad0(0, 0, 0, 0x200f, 0);
    g_2ac58->unknown_034 = 1;
    g_2ac58->unknown_054 |= 5;
    g_2ac58->unknown_04a = 0;
    g_2ac70 = title_17ad0(0, 0, 0, 0x200f, 0);
    g_2ac70->unknown_034 = 1;
    g_2ac70->unknown_054 |= 6;
    g_2ac70->unknown_04a = 0;
    g_2ac58->unknown_023 = 6;
    g_2ac58->unknown_03e = 10;
    g_2ac70->unknown_023 = 6;
    g_2ac70->unknown_03e = 20;
    g_2ac70->unknown_000 = 0x10000;
    g_2ac70->unknown_004 = 0x10000;
    g_2ac64 = title_17ad0(0, 0, 0, 0x200f, 0);
    g_2ac64->unknown_034 = 2;
    g_2ac64->unknown_000 = 0xff9c0000;
    g_2ac64->unknown_004 = 0x640000;
    g_2ac68 = 0;
    g_2ac5c = 0;
}

void title_15580(void)
{
    unsigned short code;

    g_2ac80 ^= 1;
    if (g_2ac80 != 0) {
        return;
    }
    switch (g_2ac68) {
    case 0:
        break;
    case 1:
        g_2ac64->unknown_034++;
        if (g_2ac64->unknown_034 != 0x10) {
            return;
        }
        g_2ac68 = 2;
        return;
    case 2:
        g_2ac74 = 0xc;
        g_2ac68 = 3;
        return;
    default:
        return;
    }
    g_2ac64->unknown_034 = g_25f50[g_2ac5c] + 1;
    g_2ac5c++;
    code = g_25f50[g_2ac5c];
    if (code == 0xa && g_2ac74 == 0xa) {
        g_2ac74 = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    g_2ac68 = 1;
}

void title_15ae0(void)
{
    g_2aca4 = 10;
    g_2ac84 = title_17ad0(0, 0, 0, 0x2010, 0);
    g_2ac84->unknown_034 = 1;
    g_2ac84->unknown_054 |= 5;
    g_2ac84->unknown_04a = 0;
    g_2aca0 = title_17ad0(0, 0, 0, 0x2010, 0);
    g_2aca0->unknown_034 = 1;
    g_2aca0->unknown_054 |= 6;
    g_2aca0->unknown_04a = 0;
    g_2ac84->unknown_023 = 6;
    g_2ac84->unknown_03e = 10;
    g_2aca0->unknown_023 = 6;
    g_2aca0->unknown_03e = 20;
    g_2aca0->unknown_000 = 0x10000;
    g_2aca0->unknown_004 = 0x10000;
    g_2ac8c = title_17ad0(0, 0, 0, 0x2010, 0);
    g_2ac8c->unknown_034 = 2;
    g_2ac8c->unknown_000 = 0xff9c0000;
    g_2ac8c->unknown_004 = 0x280000;
    g_2ac90 = 0;
    g_2ac88 = 0;
}

void title_15be0(void)
{
    unsigned short code;

    g_2acac ^= 1;
    if (g_2acac != 0) {
        return;
    }
    switch (g_2ac90) {
    case 0:
        break;
    case 1:
        g_2ac8c->unknown_034++;
        if (g_2ac8c->unknown_034 != 0x10) {
            return;
        }
        g_2ac90 = 2;
        return;
    case 2:
        g_2aca4 = 0xc;
        g_2ac90 = 3;
        return;
    default:
        return;
    }
    g_2ac8c->unknown_034 = g_25f50[g_2ac88] + 1;
    g_2ac88++;
    code = g_25f50[g_2ac88];
    if (code == 0xa && g_2aca4 == 0xa) {
        g_2aca4 = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    g_2ac90 = 1;
}
