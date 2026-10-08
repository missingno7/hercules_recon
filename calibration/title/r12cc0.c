/* TITLE.DLL lane w19 region: 0x12cc0, 0x12dc0, 0x13310, 0x13430, 0x139d0, 0x13ad0 (ascending).
 * Shared types and externs first. 0x128e0 (lane w19) is not in this region (see result.json). */
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

extern TitleObject *g_2ab48;
extern TitleObject *g_2ab54;
extern TitleObject *g_2ab64;
extern TitleObject *g_2ab78;
extern TitleObject *g_2ab80;
extern TitleObject *g_2ab98;
extern TitleObject *g_2aba4;
extern TitleObject *g_2abac;
extern TitleObject *g_2abc0;
extern int g_2ab4c;
extern int g_2ab58;
extern int g_2ab68;
extern int g_2ab74;
extern int g_2ab7c;
extern int g_2ab84;
extern int g_2ab9c;
extern int g_2aba0;
extern int g_2aba8;
extern int g_2abb0;
extern int g_2abc4;
extern int g_2abcc;
extern unsigned short g_25f50[];

TitleObject *title_17ad0(int a0, int a1, int a2, int size, int a4);
void title_0c4f0(int a);


void title_12cc0(void)
{
    g_2ab48 = title_17ad0(0, 0, 0, 0x2016, 0);
    g_2ab48->unknown_034 = 0x10;
    g_2ab48->unknown_054 |= 5;
    g_2ab48->unknown_04a = 0;
    g_2ab64 = title_17ad0(0, 0, 0, 0x2016, 0);
    g_2ab64->unknown_034 = 0x10;
    g_2ab64->unknown_054 |= 6;
    g_2ab64->unknown_04a = 0;
    g_2ab48->unknown_023 = 6;
    g_2ab48->unknown_03e = 0x0a;
    g_2ab64->unknown_023 = 6;
    g_2ab64->unknown_03e = 0x14;
    g_2ab64->unknown_000 = 0x10000;
    g_2ab64->unknown_004 = 0x10000;
    g_2ab54 = title_17ad0(0, 0, 0, 0x2016, 0);
    g_2ab54->unknown_034 = 1;
    g_2ab54->unknown_000 = 0xff9c0000;
    g_2ab54->unknown_004 = 0x280000;
    g_2ab58 = 0;
    g_2ab4c = 0;
}

void title_0c4f0(int a);

void title_12dc0(void)
{
    g_2ab74 ^= 1;
    if (g_2ab74 != 0) {
        return;
    }
    switch (g_2ab58) {
    case 0:
        g_2ab54->unknown_034 = g_25f50[g_2ab4c];
        g_2ab4c++;
        if (g_25f50[g_2ab4c] == 0x0a && g_2ab68 == 0x0a) {
            g_2ab68 = 0x0b;
        }
        if (g_25f50[g_2ab4c] == 0xffff) {
            title_0c4f0(1);
            g_engine_interface.context_004->unknown_1b30 = 4;
            g_2ab58 = 1;
        }
        return;
    case 1:
        g_2ab54->unknown_034++;
        if (g_2ab54->unknown_034 == 0x0f) {
            g_2ab58 = 2;
        }
        return;
    case 2:
        g_2ab68 = 0x0c;
        g_2ab58 = 3;
        return;
    }
}

void title_13310(void)
{
    g_2ab78 = title_17ad0(0, 0, 0, 0x200a, 0);
    g_2ab78->unknown_034 = 0x10;
    g_2ab78->unknown_054 |= 5;
    g_2ab78->unknown_04a = 0;
    g_2ab98 = title_17ad0(0, 0, 0, 0x200a, 0);
    g_2ab98->unknown_034 = 0x10;
    g_2ab98->unknown_054 |= 6;
    g_2ab98->unknown_04a = 0;
    g_2ab78->unknown_023 = 6;
    g_2ab78->unknown_03e = 0x0a;
    g_2ab98->unknown_023 = 6;
    g_2ab98->unknown_03e = 0x14;
    g_2ab78->unknown_000 = 0xfff60000;
    g_2ab78->unknown_004 = 0xffe80000;
    g_2ab98->unknown_000 = 0xfff70000;
    g_2ab98->unknown_004 = 0xffe90000;
    g_2ab80 = title_17ad0(0, 0, 0, 0x200a, 0);
    g_2ab80->unknown_034 = 1;
    g_2ab80->unknown_054 |= 0x10;
    g_2ab80->unknown_000 = 0x640000;
    g_2ab80->unknown_004 = 0x640000;
    g_2ab84 = 0;
    g_2ab7c = 0;
}

void title_0c4f0(int a);

void title_13430(void)
{
    g_2aba0 ^= 1;
    if (g_2aba0 != 0) {
        return;
    }
    switch (g_2ab84) {
    case 0:
        g_2ab80->unknown_034 = g_25f50[g_2ab7c];
        g_2ab7c++;
        if (g_25f50[g_2ab7c] == 0x0a && g_2ab9c == 0x0a) {
            g_2ab9c = 0x0b;
        }
        if (g_25f50[g_2ab7c] == 0xffff) {
            title_0c4f0(1);
            g_engine_interface.context_004->unknown_1b30 = 4;
            g_2ab84 = 1;
        }
        return;
    case 1:
        g_2ab80->unknown_034++;
        if (g_2ab80->unknown_034 == 0x0f) {
            g_2ab84 = 2;
        }
        return;
    case 2:
        g_2ab9c = 0x0c;
        g_2ab84 = 3;
        return;
    }
}

void title_139d0(void)
{
    g_2aba4 = title_17ad0(0, 0, 0, 0x200b, 0);
    g_2aba4->unknown_034 = 1;
    g_2aba4->unknown_054 |= 5;
    g_2aba4->unknown_04a = 0;
    g_2abc0 = title_17ad0(0, 0, 0, 0x200b, 0);
    g_2abc0->unknown_034 = 1;
    g_2abc0->unknown_054 |= 6;
    g_2abc0->unknown_04a = 0;
    g_2aba4->unknown_023 = 6;
    g_2aba4->unknown_03e = 0x0a;
    g_2abc0->unknown_023 = 6;
    g_2abc0->unknown_03e = 0x14;
    g_2abc0->unknown_000 = 0x10000;
    g_2abc0->unknown_004 = 0x10000;
    g_2abac = title_17ad0(0, 0, 0, 0x200b, 0);
    g_2abac->unknown_034 = 2;
    g_2abac->unknown_000 = 0xff9c0000;
    g_2abac->unknown_004 = 0x640000;
    g_2abb0 = 0;
    g_2aba8 = 0;
}

void title_0c4f0(int a);

void title_13ad0(void)
{
    g_2abcc ^= 1;
    if (g_2abcc != 0) {
        return;
    }
    switch (g_2abb0) {
    case 0:
        g_2abac->unknown_034 = g_25f50[g_2aba8] + 1;
        g_2aba8++;
        if (g_25f50[g_2aba8] == 0x0a && g_2abc4 == 0x0a) {
            g_2abc4 = 0x0b;
        }
        if (g_25f50[g_2aba8] == 0xffff) {
            title_0c4f0(1);
            g_engine_interface.context_004->unknown_1b30 = 4;
            g_2abb0 = 1;
        }
        return;
    case 1:
        g_2abac->unknown_034++;
        if (g_2abac->unknown_034 == 0x10) {
            g_2abb0 = 2;
        }
        return;
    case 2:
        g_2abc4 = 0x0c;
        g_2abb0 = 3;
        return;
    }
}
