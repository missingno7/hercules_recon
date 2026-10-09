/* TITLE.DLL lane w16 region: functions that reached MASKED EQUAL, ascending RVA. */
#include "title_engine.h"

typedef struct W16Slot {
    int field_00;
    int field_04;
    unsigned char unknown_08[0x34 - 0x08];
    unsigned short field_34;
    unsigned char unknown_36[0x4a - 0x36];
    unsigned short field_4a;
    unsigned char unknown_4c[0x54 - 0x4c];
    unsigned int field_54;
} W16Slot;

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
extern W16Slot *g_2a288[];
extern W16Slot *g_2aa14;
extern W16Slot *g_2aa2c;
extern W16Slot *g_2a9c8;
extern W16Slot *g_2a9cc;
extern W16Slot *g_2a9d0;
extern W16Slot *g_2a9f4;
extern W16Slot *g_2aa48;
extern W16Slot *g_2aa0c;
extern W16Slot *g_2aa08;
extern unsigned int g_2a9f0;

int title_0cce0(void);
int title_0c4a0(int a);
W16Slot *title_17ad0(int a, int b, int c, int d, int e);
int title_0cd30(void);
void title_0c4e0(void);
void title_054f0(int a, int b);
int title_0c4d0(int a);
void title_09350(W16Slot *p);
void title_04ea0(int a);

int title_0d380(int a, int b)
{
    int idx;

    if (++g_2a9c0 < 8) {
        return ++g_2a9c0;
    }
    g_2a9c0 = 0;
    idx = title_0cce0();
    if (idx != -1) {
        g_2a288[idx] = title_17ad0(0, 0, 0, 0x2007, 0);
        if (g_2a288[idx] != 0) {
            g_2a288[idx]->field_00 = a;
            g_2a288[idx]->field_04 = b;
            g_2a288[idx]->field_4a = title_0c4a0(0x80);
            return (unsigned int)g_2a288[idx]->field_4a >> 1;
        }
    }
    return 0;
}

void title_0d440(W16Slot *p)
{
    if (g_29f98 == 0) {
        if (p->field_4a > 0) {
            p->field_4a--;
        }
        if (p->field_4a > 0) {
            p->field_4a--;
        }
        if (p->field_4a > 0) {
            p->field_4a--;
        }
        if (p->field_4a > 0) {
            p->field_4a--;
        }
        if (((unsigned char *)g_engine_interface.context_004)[0x38] & 1) {
            p->field_34++;
            if (p->field_34 == 0xc0) {
                p->field_34 = 0xb6;
            }
        }
    }
}

void title_0d4b0(void)
{
    if ((g_2a250 & 0x8) || (g_2a250 & 0x4000)) {
        g_2bb3c = title_0cd30();
        if (g_2bb3c != -1) {
            if (g_2bb3c < 0x64) {
                g_2a254 = 0x18;
            } else {
                title_054f0(0x302, 0);
            }
        } else {
            g_2a254 = 0x14;
        }
    }
    if (g_2a250 & 0x80) {
        if (g_25a28 > 0) {
            title_0c4e0();
            g_2a270 = 0;
            g_2a3ac = 0;
            g_2a254 = 0xa;
        } else {
            title_0c4e0();
            g_2a270 = 0;
            g_2a3ac = 0;
            g_2a3a0 = 5;
            g_2a254 = 0x1a;
        }
        title_054f0(0x301, 0);
    }
    if (g_2a250 & 0x20) {
        if (g_25a28 < 3) {
            title_0c4e0();
            g_2a270 = 0;
            g_2a3ac = 0;
            g_2a254 = 0xc;
        } else {
            title_0c4e0();
            g_2a270 = 0;
            g_2a3ac = 0;
            g_2a3a0 = 5;
            g_2a254 = 0x1b;
        }
        title_054f0(0x300, 0);
    }
    if (g_2a250 & 0x40) {
        g_2a3a0 = 5;
        g_2a3ac = 0;
        g_2a254 = 8;
        g_2a39c = 0;
        switch (g_25a28) {
        case 0: title_054f0(0x4311, g_2a278); break;
        case 1: title_054f0(0x4311, g_2a27c); break;
        case 2: title_054f0(0x4311, g_2a280); break;
        case 3: title_054f0(0x4311, g_2a274); break;
        }
        title_0c4e0();
        g_2a270 = 0;
    }
    if (g_2a250 & 0x10) {
        g_2a3a0 = 5;
        g_2a3ac = 0;
        g_2a254 = 9;
        g_2a39c = 0;
        switch (g_25a28) {
        case 0: title_054f0(0x4311, g_2a278); break;
        case 1: title_054f0(0x4311, g_2a27c); break;
        case 2: title_054f0(0x4311, g_2a280); break;
        case 3: title_054f0(0x4311, g_2a274); break;
        }
        title_0c4e0();
        g_2a270 = 0;
    }
}

void title_0d6c0(void)
{
    switch (g_2a9bc) {
    case 0: g_2a270 = title_0c4d0(1); break;
    case 1: g_2a270 = title_0c4d0(2); break;
    case 2: g_2a270 = title_0c4d0(3); break;
    case 3: g_2a270 = title_0c4d0(4); break;
    case 4: g_2a270 = title_0c4d0(5); break;
    }
    g_2a9bc++;
    if (g_2a9bc == 5) {
        g_2a9bc = 0;
    }
}

