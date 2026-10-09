#include "title_engine.h"

/* TITLE.DLL lane w25 region: 0x19b40, 0x19c90, 0x19fa0, 0x1a3a0 (ascending). */

typedef struct TitleObject {
    long unknown_000;
    long unknown_004;
    long unknown_008;
    unsigned char unknown_00c[0x23 - 0x0c];
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

struct DigitSprite;

extern int g_2adc4;
extern int g_2afd4;
extern int g_2afdc;
extern int g_2afe0;
extern int g_2afe4;
extern int g_2afe8;
extern int g_2aff0;
extern int g_2b008;
extern int g_2b00c;
extern int g_2b090;
extern int g_2b08c;
extern int g_2b0ec;
extern int g_2b318;
extern int g_2b324;
extern int g_2b328;
extern int g_2b350;
extern int g_2b354;
extern int g_2cc64;
extern int g_265b8[7];
extern int g_2b098[20];
extern int g_2b180[20];
extern int g_2b1e8[20];
extern TitleObject *g_2af70[20];
extern TitleObject *g_2afc8;
extern TitleObject *g_2b014;
extern TitleObject *g_2b0e8;
extern TitleObject *g_2b110;
extern TitleObject *g_2b118[7];
extern TitleObject *g_29e08;
extern struct DigitSprite *g_2b120;
extern struct DigitSprite *g_2b124;
extern struct DigitSprite *g_2b128;
extern struct DigitSprite *g_2b12c;

TitleObject *title_17ad0(int a0, int a1, int a2, int size, int a4);
unsigned int title_0c4a0(unsigned int a);
int title_0c4d0(int a);
void title_054f0(int a, int b);
void title_19a40(int a, int b, int c, int d, int e, int f, int g);
void set_decimal_digits(struct DigitSprite *thousands, struct DigitSprite *hundreds,
                        struct DigitSprite *tens, struct DigitSprite *units, int value);

void title_19b40(void)
{
    switch (g_2b090) {
    case 0:
        g_2afc8 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2afc8->unknown_034 = 1;
        g_2b110 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b110->unknown_034 = 1;
        g_2afc8->unknown_023 = 6;
        g_2afc8->unknown_03e = 10;
        g_2b110->unknown_023 = 6;
        g_2b110->unknown_03e = 20;
        g_2afc8->unknown_04a = 0x80;
        g_2b110->unknown_04a = 0xff;
        g_2b110->unknown_000 = 0x20000;
        g_2b110->unknown_004 = 0x20000;
        g_2afc8->unknown_008 = 0x3e80000;
        g_2b110->unknown_008 = 0x3e80000;
        g_2b110->unknown_054 |= 6;
        g_2b090 = 1;
        /* fall through */
    case 1:
        if (g_2afc8->unknown_008 > 0) {
            g_2afc8->unknown_008 -= 0x4b0000;
            g_2b110->unknown_008 -= 0x4b0000;
        } else {
            g_2afc8->unknown_008 = 0;
            g_2b110->unknown_008 = 0;
            g_2b090 = 2;
            g_2afe8 = 0x80;
        }
        break;
    }
    g_2afc8->unknown_04a = (unsigned short)g_2afe4;
    g_2b110->unknown_04a = (unsigned short)(g_2afe4 * 2);
}

void title_19c90(void)
{
    int i;

    g_2b350 ^= 1;
    for (i = 0; i < 20; i++) {
        switch (g_2b1e8[i]) {
        case 0:
            g_2af70[i] = title_17ad0(0, 0, 0, 0x2012, 0);
            g_2af70[i]->unknown_034 = title_0c4a0(8) + 0x24d;
            g_2af70[i]->unknown_000 = 0x500000;
            g_2af70[i]->unknown_004 = 0x3c0000;
            g_2af70[i]->unknown_023 = 6;
            g_2af70[i]->unknown_03e = 20;
            g_2b180[i] = -0xa0000;
            g_2b098[i] = (title_0c4a0(8) - 5) << 16;
            g_2b1e8[i] = 1;
            break;
        case 1:
            g_2af70[i]->unknown_000 += g_2b098[i];
            g_2b098[i] -= g_2b098[i] >> 4;
            g_2af70[i]->unknown_004 += g_2b180[i];
            if (title_0c4a0(100) < 50) {
                title_19a40(0x23f, 0x2012,
                            ((title_0c4a0(16) - 8) << 16) + g_2af70[i]->unknown_000,
                            ((title_0c4a0(16) - 8) << 16) + g_2af70[i]->unknown_004,
                            0, title_0c4a0(3), g_2afe4);
            }
            if (g_2af70[i]->unknown_004 < 0x460000) {
                g_2b00c = 0;
            }
            if (g_2af70[i]->unknown_004 > 0x460000) {
                g_2af70[i]->unknown_004 = 0x460000;
                g_2b180[i] = -(g_2b180[i] >> 2);
            }
            if (g_29e08->unknown_000 > g_2af70[i]->unknown_000) {
                if (g_2b0ec == 0) {
                    title_054f0(0x4322, (int)g_29e08);
                }
                g_2b1e8[i] = 2;
                title_19a40(0x23f, 0x2012,
                            ((title_0c4a0(16) - 8) << 16) + g_2af70[i]->unknown_000,
                            ((title_0c4a0(16) - 8) << 16) + g_2af70[i]->unknown_004,
                            0, 9, g_2afe4);
            }
            g_2b180[i] += 0x8000;
            g_2af70[i]->unknown_034++;
            if (g_2af70[i]->unknown_034 == 0x255) {
                g_2af70[i]->unknown_034 = 0x24d;
            }
            if (g_2b1e8[i] != -1) {
                g_2af70[i]->unknown_04a = g_2afe4;
            }
            break;
        case 2:
            g_2af70[i]->unknown_000 += g_2b098[i];
            g_2b098[i] -= g_2b098[i] >> 4;
            g_2af70[i]->unknown_054 |= 5;
            if (g_2af70[i]->unknown_04a > 0) {
                g_2af70[i]->unknown_04a -= 4;
            } else {
                g_2b1e8[i] = 3;
            }
            if (g_2af70[i]->unknown_004 > -0xc80000) {
                g_2af70[i]->unknown_004 -= 0x30000;
            }
            g_2af70[i]->unknown_034++;
            if (g_2af70[i]->unknown_034 == 0x255) {
                g_2af70[i]->unknown_034 = 0x24d;
            }
            break;
        }
    }
}

void title_19fa0(void)
{
    g_2b354 ^= 1;
    switch (g_2adc4) {
    case 0:
        g_2b014 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b014->unknown_034 = 0x2b;
        g_2b014->unknown_000 = 0xffc00000;
        g_2b014->unknown_004 = 0xffcc0000;
        g_2b014->unknown_008 = g_2afd4 << 16;
        g_2b014->unknown_054 &= 0x7fffffff;
        g_2b014->unknown_023 = 6;
        g_2b014->unknown_03e = 0x1e;
        g_2b0e8 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b0e8->unknown_034 = 0x41;
        g_2b0e8->unknown_000 = 0;
        g_2b0e8->unknown_004 = 0xffcc0000;
        g_2b0e8->unknown_008 = g_2afd4 << 16;
        g_2b0e8->unknown_054 &= 0x7fffffff;
        g_2b0e8->unknown_023 = 6;
        g_2b0e8->unknown_03e = 0x1e;
        g_2adc4 = 2;
        break;
    case 1:
        g_2b014->unknown_054 |= 0x80000000;
        g_2b014->unknown_008 = g_2afd4;
        if (g_2b354) {
            g_2b014->unknown_034++;
            if (g_2b014->unknown_034 == 0x41) {
                g_2b014->unknown_034 = 0x2b;
            }
        }
        g_2b0e8->unknown_054 |= 0x80000000;
        g_2b0e8->unknown_008 = g_2afd4;
        if (g_2b354) {
            g_2b0e8->unknown_034++;
            if (g_2b0e8->unknown_034 == 0x57) {
                g_2b0e8->unknown_034 = 0x41;
            }
        }
        if (g_2afd4 > 0) {
            g_2afd4 -= 0x4b0000;
        } else {
            g_2afd4 = 0;
        }
        g_2b014->unknown_04a = g_2afe4;
        g_2b0e8->unknown_04a = g_2afe4;
        break;
    }
}

void title_1a3a0(void)
{
    int j;
    int k;
    int r;

    switch (g_2afe0) {
    case 0:
        for (j = 0; j < 7; j++) {
            g_2b118[j] = title_17ad0(0, 0, 0, 0x2012, 0);
            g_2b118[j]->unknown_034 = 0x5a;
            g_2b118[j]->unknown_023 = 6;
            g_2b118[j]->unknown_03e = 0x1e;
            g_2b118[j]->unknown_000 = (g_265b8[j] - 0x40) << 16;
            g_2b118[j]->unknown_004 = 0xffcc0000;
            g_2b118[j]->unknown_008 = g_2afd4;
            g_2b118[j]->unknown_054 &= 0x7fffffff;
        }
        g_2b118[0]->unknown_034 = 0x64;
        g_2b118[1]->unknown_034 = 0x65;
        g_2b118[6]->unknown_034 = 0x66;
        g_2afe0 = 2;
        break;
    case 1:
        if (g_2afd4 <= 0 && g_2afdc < g_2b318) {
            if (g_2b0ec == 0) {
                r = g_2afdc % 10;
                if (r == 0) {
                    title_054f0(0x31b, r);
                }
            }
            g_2afdc++;
        }
        if (g_2afdc == g_2b318 && g_2afd4 == 0 && g_2cc64 != 2) {
            g_29e08->unknown_034 = 3;
            g_2afe0 = 3;
            g_2cc64 = 2;
            g_2b008 = 10;
            if (g_2aff0 < 10) {
                title_0c4d0(0x16);
            } else if (g_2aff0 < 0x14) {
                title_0c4d0(0x1a);
            } else if (g_2aff0 < 0x28) {
                title_0c4d0(0x1c);
            } else if (g_2aff0 < 0x3c) {
                title_0c4d0(0x22);
            } else if (g_2aff0 < 0x50) {
                title_0c4d0(0x21);
            } else if (g_2aff0 < 0x64) {
                title_0c4d0(0x1b);
            } else {
                title_0c4d0(0x1f);
            }
        }
        set_decimal_digits(g_2b120, g_2b124, g_2b128, g_2b12c, g_2afdc);
        break;
    case 2:
        break;
    case 3:
        if (g_2aff0 >= g_2b328 && g_2b08c != 0 && g_2b324 < g_2b328) {
            if (g_2b0ec == 0) {
                title_054f0(0x31d, 0);
            }
            g_2b1e8[g_2b324] = 0;
            g_2b324++;
        }
        g_2afe0 = 4;
        /* fall through */
    case 4:
        if (g_2b08c != 0 && g_2cc64 == 1) {
            g_2cc64 = 5;
        }
        break;
    }
    for (k = 0; k < 7; k++) {
        g_2b118[k]->unknown_04a = g_2afe4;
        g_2b118[k]->unknown_008 = g_2afd4;
    }
}
