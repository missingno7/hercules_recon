/* TITLE.DLL lane w26 region: 0x1a630, 0x1a7c0, 0x1ab70, 0x1ac10, 0x1b710, 0x1b900 (ascending).
 * 0x1ab30 (lane w26) is the 14-entry switch table of 0x1a7c0 (its contribution is 944 bytes
 * including the table), so it is not a separate function (see result.json). */

typedef struct TitleObject {
    long unknown_000;
    long unknown_004;
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

extern TitleObject *g_2b060;
extern TitleObject *g_29e08;
extern TitleObject *g_2b238;
extern TitleObject *g_2b134;
extern TitleObject *g_2b300;
extern TitleObject *g_2b10c;
extern TitleObject *g_2b304;
extern TitleObject *g_2b1d8[4];
extern int g_2b0f0[];
extern int g_2afc4;
extern int g_2afe4;
extern int g_2afec;
extern int g_2aff0;
extern int g_2acf8;
extern int g_2acfc;
extern int g_2b308;
extern int g_2b0ec;
extern int g_2b35c;
extern int g_2b360;
extern int g_2b00c;
extern int g_2cc60;
extern int g_2cc64;
extern int g_2cc68;
extern signed char g_23270[];
extern signed char g_23290[];

TitleObject *title_17ad0(int a0, int a1, int a2, int size, int a4);
void title_054f0(int a, int b);
void title_05cc0(int a1, signed char *buf, int reset, int a4, int a5);
unsigned int title_0c4a0(unsigned int a);
void title_19a40(int a, int b, int c, int d, int e, int f, int g);

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

