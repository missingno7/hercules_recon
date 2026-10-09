/* TITLE unit 0x197f0..0x1d380 (one C object; 0x1c500, the tally screen, is its last function and owns
   the literal that ends its .data). Its functions share one private .bss
   block (scripts/unit_bounds.py). Declarations unified from six region files; title_0c4a0 is
   unsigned here because callers compare its result unsigned. */
#include "title_engine.h"
#include "title_screen.h"

struct title_pos {
    short x;
    short y;
};

struct DigitSprite;

static int g_2aea8[48];
static int g_2ad00[48];
static int g_2adc8[48];
static int g_2b1e8[20];
static TitleObject *g_2b240[48];
static int g_2b1d0;
static int g_2adc4;
static int g_2afd4;
static int g_2afdc;
static int g_2afe0;
static int g_2afe4;
static int g_2afe8;
static int g_2aff0;
static int g_2b008;
static int g_2b00c;
static int g_2b090;
static int g_2b08c;
static int g_2b0ec;
static int g_2b318;
static int g_2b324;
static int g_2b328;
static int g_2b350;
static int g_2b354;
extern int g_2cc64;
static int g_2b098[20];
static int g_2b180[20];
static TitleObject *g_2af70[20];
static TitleObject *g_2afc8;
static TitleObject *g_2b014;
static TitleObject *g_2b0e8;
static TitleObject *g_2b110;
static TitleObject *g_2b118[7];
extern TitleObject *g_29e08;
static struct DigitSprite *g_2b120;
static struct DigitSprite *g_2b124;
static struct DigitSprite *g_2b128;
static struct DigitSprite *g_2b12c;
static TitleObject *g_2b060;
static TitleObject *g_2b238;
static TitleObject *g_2b134;
static TitleObject *g_2b300;
static TitleObject *g_2b10c;
static TitleObject *g_2b304;
static TitleObject *g_2b1d8[4];
static int g_2b0f0[4];
static int g_2afc4;
static int g_2afec;
static int g_2acf8;
static int g_2acfc;
static int g_2b308;
static int g_2b35c;
static int g_2b360;
extern int g_2cc60;
extern int g_2cc68;
extern signed char g_23270[];
extern signed char g_23290[];
static int g_2aff8[4];
static unsigned int g_2b178;
static TitleObject *g_2b30c;
static TitleObject *g_2adc0;
static int g_2b100;
static TitleObject *g_2b104[2];
static TitleObject *g_2b058[2];
static TitleObject *g_2b108;
static TitleObject *g_2b05c;
static int g_2af68;
static int g_2aff4;
static TitleObject *g_2afd0;
static TitleObject *g_2b330[8];
static TitleObject *g_2b31c;
static int g_2afcc;
static int g_2affc;
static int g_2b000;
static int g_2b004;

static int g_2b010;
static int g_2b358;
void title_09350(void *p);
unsigned int title_0c4a0(int a);
int title_0c4d0(int a);
void title_054f0(int a, int b);
void title_05ad0(int a, void *b);
void title_05cc0(int a1, signed char *buf, int reset, int a4, int a5);
TitleObject *title_17ad0(int a0, int a1, int a2, int size, int a4);
void title_18600(TitleObject *o, struct title_pos *pos);
void title_19820(void);
void title_19a40(int a1, int a2, int a3, int a4, int a5, int a6, int a7);
void set_decimal_digits(struct DigitSprite *thousands, struct DigitSprite *hundreds,
                        struct DigitSprite *tens, struct DigitSprite *units, int value);

/* Initialized data of this unit (TITLE.DLL .data 0x264f0..0x26717), in address order. Arrays of 8+ bytes
   are 8-aligned, so 0x265b4 and 0x26634 are not arrays of their own (0x26634 is g_26630[i + 1]). */
unsigned char g_264f0[4] = {  /* read by 0x3da0 (element type pending) */
    1, 3, 5, 15,
};
unsigned char g_264f8[16] = {  /* read by 0x3100..0x3da0 and 0x17f50..0x186e0 (element type pending) */
    0, 0, 1, 0, 2, 0, 0, 0, 3, 1, 0, 0, 2, 0, 0, 0,
};
unsigned char g_26508[172] = {  /* read by 0x3da0 (element type pending) */
    16, 17, 18, 19, 20, 21, 22, 32, 33, 34, 35, 36, 37, 38, 48, 49,
    50, 51, 52, 53, 54, 64, 65, 66, 67, 68, 69, 70, 80, 81, 82, 83,
    84, 85, 86, 96, 97, 98, 99, 100, 101, 102, 144, 145, 146, 147, 148, 149,
    150, 160, 161, 162, 163, 164, 165, 166, 176, 177, 178, 179, 180, 181, 182, 192,
    193, 194, 195, 196, 197, 198, 208, 209, 210, 211, 212, 213, 214, 224, 225, 226,
    227, 228, 229, 230, 240, 241, 242, 243, 244, 245, 246, 134, 133, 132, 131, 130,
    129, 128, 112, 113, 114, 115, 116, 117, 118, 6, 5, 4, 3, 2, 1, 0,
    4, 12, 20, 28, 6, 14, 22, 30, 7, 15, 23, 31, 8, 16, 24, 32,
    9, 17, 25, 33, 10, 18, 26, 34, 12, 20, 28, 36, 12, 28, 44, 60,
    14, 30, 46, 62, 15, 31, 47, 63, 17, 33, 49, 65, 18, 34, 50, 66,
    20, 36, 52, 68, 31, 63, 95, 127, 33, 65, 97, 129,
};
int g_265b8[7] = {
    32, 96, 104, 116, 128, 140, 156,
};
int g_265d8[8] = {  /* read by 0x1b150 */
    597, 621, 645, 669, 693, 717, 621, 741,
};
int g_265f8[8] = {  /* read by 0x1b150 */
    24, 24, 24, 24, 24, 24, 24, 24,
};
int g_26618[6] = {  /* frame list ending in -1 */
    821, 821, 822, 822, -1, 0,
};
int g_26630[58] = {  /* frame/delay pairs ending in -1, -1 */
    797, 5, 797, 5, 798, 5, 798, 5,
    799, 5, 799, 5, 800, 5, 800, 5,
    801, 5, 801, 5, 802, 5, 802, 5,
    803, 100, 803, 100, 804, 100, 804, 100,
    805, 100, 805, 100, 806, 100, 806, 100,
    807, 100, 807, 100, 808, 100, 808, 100,
    809, 100, 809, 100, 821, 5, 821, 5,
    -1, -1,
};

void title_197f0(void)
{
    int i;

    for (i = 0; i < 0x14; i++) {
        g_2b1e8[i] = -1;
    }
    for (i = 0; i < 0x30; i++) {
        g_2aea8[i] = -1;
    }
    for (i = 0; i < 0x30; i++) {
        g_2b240[i] = 0;
    }
}

void title_19820(void)
{
    int i;

    for (i = 0; i < 0x30; i++) {
        if (g_2aea8[i] == 0) {
            if (g_2b240[i] != 0) {
                title_09350(g_2b240[i]);
                g_2b240[i] = 0;
            }
            g_2aea8[i] = 2;
        }
    }
}

void title_19870(int limit)
{
    int tab[16];
    int i;

    tab[0] = 0;
    tab[1] = 0;
    tab[2] = 2;
    tab[3] = 0;
    tab[4] = 1;
    tab[5] = 0;
    tab[6] = 0;
    tab[7] = 0;
    tab[8] = 0;
    tab[9] = 0;
    tab[10] = -2;
    tab[11] = 0;
    tab[12] = -1;
    tab[13] = 0;
    tab[14] = 0;
    tab[15] = 0;
    for (i = 0; i < 0x30; i++) {
        if (g_2aea8[i] == 0) {
            if (g_2b240[i]->unknown_04a > 0) {
                g_2b240[i]->unknown_04a -= 4;
                if ((int)g_2b240[i]->unknown_034 > limit) {
                    g_2b240[i]->unknown_034--;
                }
                g_2b240[i]->unknown_000 += tab[(g_2b240[i]->unknown_004 >> 16) & 0xf] << 16;
                g_2b240[i]->unknown_004 += g_2adc8[i];
                g_2b240[i]->unknown_074 += title_0c4a0(7);
                g_2ad00[i] -= g_2ad00[i] >> 2;
                if (g_2b240[i]->unknown_004 > 0x1000000) {
                    if (g_2b240[i] != 0) {
                        title_09350(g_2b240[i]);
                        g_2b240[i] = 0;
                    }
                    g_2aea8[i] = -1;
                }
            } else {
                if (g_2b240[i] != 0) {
                    title_09350(g_2b240[i]);
                    g_2b240[i] = 0;
                }
                g_2aea8[i] = -1;
            }
        }
    }
}

void title_199b0(int limit)
{
    int i;

    for (i = 0; i < 0x30; i++) {
        if (g_2aea8[i] == 0) {
            if (g_2b240[i]->unknown_04a > 0) {
                g_2b240[i]->unknown_04a -= 8;
                if ((int)g_2b240[i]->unknown_034 > limit) {
                    g_2b240[i]->unknown_034--;
                }
                g_2b240[i]->unknown_074 += title_0c4a0(0x2f);
            } else {
                if (g_2b240[i] != 0) {
                    title_09350(g_2b240[i]);
                    g_2b240[i] = 0;
                }
                g_2aea8[i] = -1;
            }
        }
    }
}

void title_19a40(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    int i;

    if (g_2b1d0 == 4) {
        return;
    }
    for (i = 0; i < 0x30; i++) {
        if (g_2aea8[i] == -1) {
            g_2b240[i] = title_17ad0(0, 0, 0, a2, 0);
            if (g_2b240[i] == 0) {
                return;
            }
            g_2b240[i]->unknown_034 = (short)(a6 + a1);
            g_2b240[i]->unknown_000 = a3;
            g_2b240[i]->unknown_004 = a4;
            g_2b240[i]->unknown_008 = a5;
            g_2b240[i]->unknown_04a = (unsigned short)a7;
            g_2b240[i]->unknown_023 = 6;
            g_2b240[i]->unknown_03e = 0xa;
            g_2b240[i]->unknown_074 += title_0c4a0(0x1000);
            g_2b240[i]->unknown_054 |= 5;
            g_2ad00[i] = 0;
            g_2adc8[i] = 0x10000;
            g_2aea8[i] = 0;
            return;
        }
    }
}

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

void title_1a180(void)
{
    g_2b358 ^= 1;
    if (g_2b358 != 0) {
        return;
    }
    switch (g_2b010) {
    case 0:
        g_2b31c = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b31c->unknown_034 = 0x1eb;
        g_2b31c->unknown_000 = 0x6e0000;
        g_2b31c->unknown_004 = 0x420000;
        g_2b31c->unknown_023 = 6;
        g_2b31c->unknown_03e = 0x32;
        g_2b31c->unknown_054 |= 0x10;
        g_2b010 = 1;
        break;
    case 1:
        if (title_0c4a0(100) <= 10) {
            g_2b010 = 2;
        }
        break;
    case 2:
        g_2b31c->unknown_034++;
        if (g_2b31c->unknown_034 == 0x1f7) {
            g_2b31c->unknown_034 = 0x1eb;
            g_2b010 = 1;
            if (title_0c4a0(100) < 25 && g_2b0ec == 0) {
                title_054f0(0x4320, (int)g_2b31c);
            }
        }
        break;
    }
    g_2b31c->unknown_04a = g_2afe4;
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

void title_1a630(void)
{
    switch (g_2afc4) {
    case 0:
        g_2b060 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b060->unknown_034 = 0x21c;
        g_2b060->unknown_023 = 6;
        g_2b060->unknown_03e = 0x32;
        g_2b060->unknown_000 = 0xff830000;
        g_2b060->unknown_004 = 0xfea20000;
        g_2afc4 = 2;
        break;
    case 1:
        break;
    case 2:
        if (g_2b060->unknown_004 < 0x6a0000) {
            g_2b060->unknown_004 += 0x80000;
        } else {
            if (g_2b0ec == 0) {
                title_054f0(0x431f, (int)g_2b060);
            }
            g_2afc4 = 3;
            g_2b060->unknown_004 = 0x6a0000;
        }
        break;
    case 3:
        g_2b35c ^= 1;
        if (g_2b35c == 0) {
            g_2b060->unknown_034++;
            if (g_2b060->unknown_034 == 0x229) {
                g_2b060->unknown_034 = 0x20b;
                g_2afc4 = 4;
            }
        }
        break;
    case 4:
        g_2b35c ^= 1;
        if (g_2b35c == 0) {
            g_2b060->unknown_034++;
            if (g_2b060->unknown_034 == 0x21c) {
                g_2b060->unknown_034 = 0x20b;
            }
        }
        break;
    }
    g_2b060->unknown_04a = g_2afe4;
}

void title_1a7c0(void)
{
    g_2b360 ^= 1;
    switch (g_2cc64) {
    case 0:
        g_29e08 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_29e08->unknown_034 = 0x229;
        g_29e08->unknown_023 = 6;
        g_29e08->unknown_03e = 0x32;
        g_29e08->unknown_000 = 0xffab0000;
        g_29e08->unknown_004 = 0x210000;
        g_2cc64 = 1;
        g_2b238 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b238->unknown_034 = 0x17;
        g_2b238->unknown_023 = 6;
        g_2b238->unknown_03e = 0x32;
        g_2b238->unknown_000 = 0xffab0000;
        g_2b238->unknown_004 = 0x210000;
        g_2cc60 = 0;
        g_2cc68 = 0;
        g_29e08->unknown_034 = 0x229;
        break;
    case 2:
        g_2cc64 = 3;
        break;
    case 3:
        if (g_2b360 != 0) {
            g_2b238->unknown_034++;
            g_29e08->unknown_034++;
            if (g_29e08->unknown_034 == 8) {
                if (g_2b0ec == 0) {
                    title_054f0(0x4321, (int)g_29e08);
                }
            }
            if (g_29e08->unknown_034 == 0xf) {
                if (g_2b0ec == 0) {
                    title_054f0(0x430f, (int)g_29e08);
                }
            }
            if (g_29e08->unknown_034 == 0x16) {
                g_29e08->unknown_034 = 3;
                g_2b238->unknown_034 = 0x17;
                g_2cc64 = 1;
                g_2afec = 1;
                g_29e08->unknown_034 = 0x229;
            }
        }
        break;
    case 4:
        break;
    case 5:
        g_2cc60 = 0;
        g_2cc68 = 0;
        g_29e08->unknown_034 = 0x229;
        g_2b00c = 0;
        g_2cc64 = 6;
        break;
    case 6:
        title_05cc0(0x229, g_23270, 0, 0, 0);
        g_2b00c = g_2b00c + 1;
        if (g_2b00c > 5) {
            g_2cc64 = 7;
            g_2cc60 = 0;
            g_2cc68 = 0;
            g_29e08->unknown_034 = 0x231;
        }
        break;
    case 7:
        title_05cc0(0x231, g_23290, 4, 6, 0);
        if (g_29e08->unknown_000 > 0xc80000) {
            g_29e08->unknown_054 |= 0x10;
            g_2cc64 = 8;
        }
        break;
    case 8:
        title_05cc0(0x231, g_23290, 4, 6, 0);
        if (g_29e08->unknown_000 < 0) {
            g_2cc60 = 0;
            g_2cc68 = 0;
            g_29e08->unknown_034 = 0x229;
            if (g_2b0ec == 0) {
                title_054f0(0x4323, (int)g_2b238);
            }
            g_2cc64 = 9;
        }
        break;
    case 1:
    case 9:
        title_05cc0(0x229, g_23270, 0, 0, 0);
        break;
    }
    switch (g_2afec) {
    case 0:
        break;
    case 1:
        g_2b238->unknown_034 = 0x302;
        g_2afec = 2;
        break;
    case 2:
        if (g_2b360 != 0) {
            g_2b238->unknown_034++;
            if (g_2b238->unknown_034 == 0x30d) {
                g_2b238->unknown_034 = 0x30d;
                g_2afec = 3;
            }
        }
        break;
    case 3:
        if (g_2b360 != 0) {
            if (title_0c4a0(0x64) < 0x32) {
                g_2b238->unknown_034++;
            }
            if (g_2b238->unknown_034 == 0x31c) {
                g_2b238->unknown_034 = 0x30d;
            }
        }
        break;
    }
    g_29e08->unknown_04a = g_2afe4;
    g_2b238->unknown_04a = g_2afe4;
}

void title_1ab70(int which)
{
    switch (which) {
    case 0:
        g_2b1d8[0]->unknown_034++;
        if (g_2b1d8[0]->unknown_034 >= 0x1e6) {
            g_2b1d8[0]->unknown_034 = 0x18e;
        }
        break;
    case 1:
        g_2b1d8[1]->unknown_034++;
        if (g_2b1d8[1]->unknown_034 >= 0x124) {
            g_2b1d8[1]->unknown_034 = 0xcc;
        }
        break;
    case 2:
        g_2b1d8[2]->unknown_034++;
        if (g_2b1d8[2]->unknown_034 >= 0x185) {
            g_2b1d8[2]->unknown_034 = 0x12d;
        }
        break;
    case 3:
        g_2b1d8[3]->unknown_034++;
        if (g_2b1d8[3]->unknown_034 >= 0xc3) {
            g_2b1d8[3]->unknown_034 = 0x6b;
        }
        break;
    }
}

void title_1ac10(int which)
{
    int n;

    if (g_2b0f0[which] == 4) {
        return;
    }
    for (n = 5; n != 0; n--) {
        title_19a40(0x23f, 0x2012,
            ((title_0c4a0(0x20) - 0x10) << 16) + g_2b1d8[which]->unknown_000,
            ((title_0c4a0(0x20) - 0x30) << 16) + g_2b1d8[which]->unknown_004,
            0, title_0c4a0(3), g_2afe4);
    }
}

void title_1ac90(void)
{
    int i;

    for (i = 0; i < 4; i++) {
        switch (g_2aff8[i]) {
        case 0:
            g_2b1d8[i] = title_17ad0(0, 0, 0, 0x2012, 0);
            switch (i) {
            case 0:
                g_2b1d8[0]->unknown_034 = g_2b0f0[0] + 0x14a;
                if (g_2b1d8[0]->unknown_034 < 0x18e)
                    g_2b1d8[0]->unknown_034 += 0x5c;
                break;
            case 1:
                g_2b1d8[1]->unknown_034 = g_2b0f0[1] + 0x88;
                if (g_2b1d8[1]->unknown_034 < 0xcc)
                    g_2b1d8[1]->unknown_034 += 0x5c;
                break;
            case 2:
                g_2b1d8[2]->unknown_034 = g_2b0f0[2] + 0xe9;
                if (g_2b1d8[2]->unknown_034 < 0x12d)
                    g_2b1d8[2]->unknown_034 += 0x5c;
                break;
            case 3:
                g_2b1d8[3]->unknown_034 = g_2b0f0[3] + 0x27;
                if (g_2b1d8[3]->unknown_034 < 0x6b)
                    g_2b1d8[3]->unknown_034 += 0x5c;
                break;
            }
            g_2b1d8[i]->unknown_000 = (i * 15 << 18) - 0x5a0000;
            g_2b1d8[i]->unknown_004 = 0x80000;
            g_2b1d8[i]->unknown_008 = 0x3e80000;
            g_2b1d8[i]->unknown_054 &= 0x7fffffff;
            g_2aff8[i] = 1;
            break;
        case 1:
            break;
        case 2:
            g_2b1d8[i]->unknown_054 |= 0x80000000;
            if (g_2b1d8[i]->unknown_008 > 0) {
                g_2b1d8[i]->unknown_008 -= 0x640000;
                title_1ab70(i);
            } else {
                if (g_2b0ec == 0)
                    title_054f0(0x4319, (int)g_2b1d8[i]);
                g_2b1d8[i]->unknown_008 = 0;
                g_2aff8[i] = 4;
                title_1ab70(i);
            }
            break;
        case 3:
            break;
        case 4:
            title_1ab70(i);
            switch (i) {
            case 0:
                if (g_2b0f0[0] != -1 && g_2b0f0[0] == (int)g_2b1d8[0]->unknown_034 - 0x18a) {
                    title_05ad0(0x319, g_2b1d8[0]);
                    if (g_2b0ec == 0)
                        title_054f0(0x31a, 0);
                    g_2aff8[0] = 5;
                    g_2b1d8[0]->unknown_04a = 0xff;
                    title_1ac10(0);
                }
                break;
            case 1:
                if (g_2aff8[0] > 4 && g_2b0f0[1] != -1 && g_2b0f0[1] == (int)g_2b1d8[1]->unknown_034 - 0xc8) {
                    title_05ad0(0x319, g_2b1d8[1]);
                    if (g_2b0ec == 0)
                        title_054f0(0x31a, 0);
                    g_2aff8[1] = 5;
                    g_2b1d8[1]->unknown_04a = 0xff;
                    title_1ac10(1);
                }
                break;
            case 2:
                if (g_2aff8[1] > 4 && g_2b0f0[2] != -1 && g_2b0f0[2] == (int)g_2b1d8[2]->unknown_034 - 0x129) {
                    title_05ad0(0x319, g_2b1d8[2]);
                    if (g_2b0ec == 0)
                        title_054f0(0x31a, 0);
                    g_2aff8[2] = 5;
                    g_2b1d8[2]->unknown_04a = 0xff;
                    title_1ac10(2);
                }
                break;
            case 3:
                if (g_2aff8[2] > 4 && g_2b0f0[3] != -1 && g_2b0f0[3] == (int)g_2b1d8[3]->unknown_034 - 0x67) {
                    title_05ad0(0x319, g_2b1d8[3]);
                    if (g_2b0ec == 0) {
                        title_054f0(0x31a, 0);
                        if (((ScreenContext *)g_engine_interface.context_004)->unknown_00[0x11] == 0xf)
                            title_054f0(0x31e, 0);
                    }
                    g_2aff8[3] = 5;
                    g_2b1d8[3]->unknown_04a = 0xff;
                    title_1ac10(3);
                }
                break;
            }
            break;
        case 5:
            if (g_2b1d8[i]->unknown_04a > 0x80)
                g_2b1d8[i]->unknown_04a -= 4;
            else
                g_2aff8[i] = 6;
            if (g_2b1d0 != 4 && title_0c4a0(0x14) == 0) {
                title_19a40(0x23f, 0x2012,
                            ((title_0c4a0(0x20) - 0x10) << 16) + g_2b1d8[i]->unknown_000,
                            ((title_0c4a0(4) - 8) << 16) + g_2b1d8[i]->unknown_004,
                            0, title_0c4a0(3), g_2afe4);
            }
            break;
        default:
            break;
        }
        if (g_2aff8[i] != 5)
            g_2b1d8[i]->unknown_04a = (unsigned short)g_2afe4;
    }
}

void title_1b710(void)
{
    switch (g_2acfc) {
    case 0:
        g_2b134 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b300 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b134->unknown_034 = 0x31c;
        g_2b300->unknown_034 = 0x31c;
        g_2b300->unknown_023 = 6;
        g_2b300->unknown_03e = 0x1e;
        g_2b300->unknown_04a = 0;
        g_2b134->unknown_023 = 6;
        g_2b134->unknown_03e = 0x14;
        g_2b134->unknown_04a = 0;
        g_2b300->unknown_054 |= 6;
        g_2b300->unknown_000 = 0x20000;
        g_2b300->unknown_004 = 0x20000;
        g_2b300->unknown_04a = 0;
        g_2b134->unknown_04a = 0;
        g_2b134->unknown_054 &= 0x7fffffff;
        g_2b300->unknown_054 &= 0x7fffffff;
        g_2acfc = 0x63;
        break;
    case 1:
        g_2b134->unknown_054 |= 0x80000000;
        g_2b300->unknown_054 |= 0x80000000;
        if (g_2b134->unknown_04a < 0x80) {
            g_2b134->unknown_04a += 0x10;
        } else {
            g_2acfc = 2;
        }
        break;
    case 2:
        if (g_2b308 == 8) {
            g_2b134->unknown_004 += 0xfffc0000;
            g_2b300->unknown_004 += 0xfffc0000;
            if (g_2b134->unknown_004 < 0x1000000) {
                g_2acfc = 3;
            }
        }
        break;
    case 3:
        g_2b134->unknown_054 &= 0x7fffffff;
        g_2b300->unknown_054 &= 0x7fffffff;
        break;
    case 4:
        g_2b134->unknown_04a = g_2afe4;
        break;
    }
    if (g_2acfc != 0) {
        g_2b300->unknown_04a = g_2b134->unknown_04a << 1;
    }
}

void title_1b900(void)
{
    switch (g_2acf8) {
    case 3:
        g_2b10c->unknown_04a = g_2afe4;
        break;
    case 2:
        if (g_2b10c->unknown_04a < 0x80) {
            g_2b10c->unknown_04a += 8;
        } else {
            g_2acf8 = 3;
        }
        break;
    case 1:
        g_2b10c = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b304 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b10c->unknown_034 = 0x2fd;
        if (g_2aff0 > 0x50) {
            g_2b10c->unknown_034 = 0x301;
        } else if (g_2aff0 > 0x3c) {
            g_2b10c->unknown_034 = 0x300;
        } else if (g_2aff0 > 0x28) {
            g_2b10c->unknown_034 = 0x2ff;
        } else if (g_2aff0 > 0x14) {
            g_2b10c->unknown_034 = 0x2fe;
        }
        g_2b304->unknown_034 = g_2b10c->unknown_034;
        g_2b304->unknown_023 = 6;
        g_2b304->unknown_03e = 0x14;
        g_2b304->unknown_04a = 0;
        g_2b10c->unknown_023 = 6;
        g_2b10c->unknown_03e = 0xa;
        g_2b10c->unknown_04a = 0;
        g_2b304->unknown_054 |= 6;
        g_2b10c->unknown_004 = 0xff900000;
        g_2b304->unknown_000 = g_2b10c->unknown_000 + 0x20000;
        g_2b304->unknown_004 = g_2b10c->unknown_004 + 0x20000;
        g_2acf8 = 2;
        break;
    }
    if (g_2acf8 != 0) {
        g_2b304->unknown_04a = g_2b10c->unknown_04a << 1;
    }
}

void title_1bab0(void)
{
    int value;

    if (g_2b100 == 0) {
        value = 6;
        g_2b104[0] = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b104[0]->unknown_034 = 0x57;
        g_2b104[0]->unknown_023 = value;
        g_2b104[0]->unknown_03e = 0x14;
        g_2b104[0]->unknown_04a = 0x80;
        g_2b058[0] = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b058[0]->unknown_034 = 0x57;
        g_2b058[0]->unknown_023 = value;
        g_2b058[0]->unknown_03e = 0x1e;
        g_2b058[0]->unknown_04a = 0xff;
        g_2b058[0]->unknown_054 |= value;
        g_2b058[0]->unknown_000 = 0x20000;
        g_2b058[0]->unknown_004 = 0x20000;
        g_2b108 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b108->unknown_023 = value;
        g_2b108->unknown_03e = 0x14;
        g_2b108->unknown_04a = 0x80;
        g_2b05c = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b05c->unknown_023 = value;
        g_2b05c->unknown_03e = 0x1e;
        g_2b05c->unknown_04a = 0xff;
        g_2b05c->unknown_054 |= value;
        g_2b05c->unknown_000 = 0x20000;
        g_2b05c->unknown_004 = 0x20000;
        g_2b108->unknown_034 = 0x59;
        g_2b05c->unknown_034 = 0x59;
        g_2b100 = 1;
    }
    g_2b104[0]->unknown_04a = (unsigned short)g_2afe4;
    g_2b058[0]->unknown_04a = (unsigned short)(g_2afe4 * 2);
    g_2b108->unknown_04a = (unsigned short)g_2afe4;
    g_2b05c->unknown_04a = (unsigned short)(g_2afe4 * 2);
}

void title_1bc60(void)
{
    TitleObject **p;

    if (g_2b308 == 8) {
        g_2b178++;
    }
    if (g_2b178 > 0 && g_2afe0 == 2) {
        for (p = g_2b118; (int)p < (int)(g_2b118 + 7); p++) {
            (*p)->unknown_054 |= 0x80000000;
        }
        g_2b014->unknown_054 |= 0x80000000;
        g_2afe0 = 1;
        g_2adc4 = 1;
    }
    if (g_2b178 > 0x78 && g_2acf8 == 0) {
        g_2acf8 = 1;
    }
}

void title_1bcf0(void)
{
    if (++g_2b178 != 0 && g_2aff8[0] == 1) {
        if (g_2b0ec == 0) {
            title_054f0(0x318, 0);
        }
        g_2aff8[0] = 2;
    }
    if (g_2b178 > 0xa && g_2affc == 1) {
        if (g_2b0ec == 0) {
            title_054f0(0x318, 0);
        }
        g_2affc = 2;
    }
    if (g_2b178 > 0x14 && g_2b000 == 1) {
        if (g_2b0ec == 0) {
            title_054f0(0x318, 0);
        }
        g_2b000 = 2;
    }
    if (g_2b178 > 0x1e && g_2b004 == 1) {
        if (g_2b0ec == 0) {
            title_054f0(0x318, 0);
        }
        g_2b004 = 2;
    }
}

void title_1bdc0(void)
{
    switch (g_2afcc) {
    case 0:
        g_2b30c = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2b30c->unknown_034 = 2;
        g_2adc0 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2adc0->unknown_034 = 2;
        g_2adc0->unknown_023 = 6;
        g_2b30c->unknown_023 = 6;
        g_2b30c->unknown_03e = 0x14;
        g_2adc0->unknown_03e = 0x1e;
        g_2adc0->unknown_004 = 0x20000;
        g_2adc0->unknown_000 = 0x20000;
        g_2adc0->unknown_054 |= 6;
        g_2adc0->unknown_04a = 0;
        g_2b30c->unknown_04a = 0;
        g_2afcc = 1;
        break;
    case 1:
        g_2b30c->unknown_04a = (unsigned short)g_2afe4;
        g_2adc0->unknown_04a = (unsigned short)(g_2afe4 + g_2afe4);
        break;
    }
}

void title_1beb0(void)
{
    struct title_pos pos;

    switch (g_2af68) {
    case 0:
        g_2afd0 = title_17ad0(0, 0, 0, 0x2012, 0);
        g_2afd0->unknown_034 = 0x335;
        g_2adc0->unknown_023 = 6;
        g_2afd0->unknown_023 = 6;
        g_2afd0->unknown_03e = 5;
        g_2aff4 = 0;
        g_2af68 = 1;
        g_2afd0->unknown_000 = 0xff380000;
        g_2afd0->unknown_004 = 0xffe00000;
        break;
    case 1:
        break;
    case 2:
        g_2afd0->unknown_034 = g_26618[g_2aff4];
        g_2aff4++;
        if (g_26618[g_2aff4] == -1) {
            g_2aff4 = 0;
        }
        title_18600(g_2afd0, &pos);
        title_19a40(0x23f, 0x2012,
                    ((title_0c4a0(0x10) + pos.x - 8) << 16) + g_2afd0->unknown_000,
                    ((title_0c4a0(0x10) + pos.y - 8) << 16) + g_2afd0->unknown_004,
                    0, title_0c4a0(1), g_2afe4);
        g_2afd0->unknown_000 += 0x30000;
        if (g_2afd0->unknown_000 > -0x800000) {
            g_2af68 = 3;
            g_2aff4 = 0;
        }
        break;
    case 3:
        title_18600(g_2afd0, &pos);
        title_19a40(0x23f, 0x2012,
                    ((title_0c4a0(0x10) + pos.x - 8) << 16) + g_2afd0->unknown_000,
                    ((title_0c4a0(0x10) + pos.y - 8) << 16) + g_2afd0->unknown_004,
                    0, title_0c4a0(1), g_2afe4);
        g_2afd0->unknown_034 = g_26630[g_2aff4];
        g_2afd0->unknown_03e = g_26630[g_2aff4 + 1];
        g_2aff4 += 2;
        if (g_26630[g_2aff4] == -1) {
            g_2aff4 = 0;
        }
        g_2afd0->unknown_000 += 0x30000;
        if (g_2afd0->unknown_000 > 0x400000 && g_2afd0->unknown_034 == 0x335) {
            g_2af68 = 4;
            g_2aff4 = 0;
        }
        break;
    case 4:
        if (g_2afd0->unknown_000 < 0xc80000) {
            g_2afd0->unknown_034 = g_26618[g_2aff4];
            g_2aff4++;
            if (g_26618[g_2aff4] == -1) {
                g_2aff4 = 0;
            }
            title_18600(g_2afd0, &pos);
            title_19a40(0x23f, 0x2012,
                        ((title_0c4a0(0x10) + pos.x - 8) << 16) + g_2afd0->unknown_000,
                        ((title_0c4a0(0x10) + pos.y - 8) << 16) + g_2afd0->unknown_004,
                        0, title_0c4a0(1), g_2afe4);
            g_2afd0->unknown_000 += 0x30000;
        }
        break;
    }
    g_2afd0->unknown_04a = (unsigned short)g_2afe4;
}

void title_1c240(void)
{
    TitleObject **p;
    int off;

    if (g_2afd0 != 0) {
        title_09350(g_2afd0);
        g_2afd0 = 0;
    }
    if (g_2b134 != 0) {
        title_09350(g_2b134);
        g_2b134 = 0;
    }
    if (g_2b300 != 0) {
        title_09350(g_2b300);
        g_2b300 = 0;
    }
    if (g_29e08 != 0) {
        title_09350(g_29e08);
        g_29e08 = 0;
    }
    if (g_2b238 != 0) {
        title_09350(g_2b238);
        g_2b238 = 0;
    }
    if (g_2b31c != 0) {
        title_09350(g_2b31c);
        g_2b31c = 0;
    }
    if (g_2b060 != 0) {
        title_09350(g_2b060);
        g_2b060 = 0;
    }
    if (g_2afc8 != 0) {
        title_09350(g_2afc8);
        g_2afc8 = 0;
    }
    if (g_2b110 != 0) {
        title_09350(g_2b110);
        g_2b110 = 0;
    }
    if (g_2b30c != 0) {
        title_09350(g_2b30c);
        g_2b30c = 0;
    }
    if (g_2adc0 != 0) {
        title_09350(g_2adc0);
        g_2adc0 = 0;
    }
    for (off = 0; off < 0x50; off += 4) {
        if (*(int *)((char *)g_2b1e8 + off) != -1 && *(void **)((char *)g_2af70 + off) != 0) {
            title_09350(*(void **)((char *)g_2af70 + off));
            *(void **)((char *)g_2af70 + off) = 0;
        }
    }
    for (p = g_2b118; (int)p < (int)(g_2b118 + 7); p++) {
        if (*p != 0) {
            title_09350(*p);
            *p = 0;
        }
    }
    for (p = g_2b330; (int)p < (int)(g_2b330 + 8); p++) {
        if (*p != 0) {
            title_09350(*p);
            *p = 0;
        }
    }
    if (g_2b014 != 0) {
        title_09350(g_2b014);
        g_2b014 = 0;
    }
    if (g_2b0e8 != 0) {
        title_09350(g_2b0e8);
        g_2b0e8 = 0;
    }
    if (g_2b10c != 0) {
        title_09350(g_2b10c);
        g_2b10c = 0;
    }
    if (g_2b304 != 0) {
        title_09350(g_2b304);
        g_2b304 = 0;
    }
}

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
