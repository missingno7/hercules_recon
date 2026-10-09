/* TITLE.DLL lane w24 region: MASKED EQUAL functions only, ascending RVA.
 * Shared includes, types, externs and prototypes first. Field names are offsets. */
#include "title_engine.h"

typedef struct TitleRec {
    unsigned char unknown_000[0x0c];
    unsigned char unknown_00c[0x10 - 0x0c];
    short unknown_010;
    unsigned char unknown_012[0x76 - 0x12];
    unsigned short unknown_076;
    unsigned char unknown_078[0x80 - 0x78];
    int unknown_080;
} TitleRec;

typedef struct TitleShape {
    unsigned char unknown_000[0x0c];
    short unknown_00c;
    short unknown_00e;
    short unknown_010;
    unsigned char unknown_012[0x3a - 0x12];
    unsigned short unknown_03a;
    unsigned short unknown_03c;
    unsigned char unknown_03e[0x54 - 0x3e];
    unsigned char unknown_054;
} TitleShape;

typedef struct TitleSlot {
    int unknown_000;
    int unknown_004;
    int unknown_008;
    unsigned char unknown_00c[0x23 - 0x0c];
    unsigned char unknown_023;
    unsigned char unknown_024[0x34 - 0x24];
    unsigned short unknown_034;
    unsigned char unknown_036[0x3e - 0x36];
    unsigned short unknown_03e;
    unsigned char unknown_040[0x4a - 0x40];
    unsigned short unknown_04a;
    unsigned char unknown_04c[0x54 - 0x4c];
    int unknown_054;
    unsigned char unknown_058[0x74 - 0x58];
    unsigned short unknown_074;
} TitleSlot;


extern int g_2cbe8;
extern int g_2b1e8[];
extern int g_2aea8[];
extern int g_2ad00[];
extern int g_2adc8[];
extern int g_2b1d0;
extern TitleSlot *g_2b240[];

void title_09350(void *p);
short title_0c4a0(int a);
TitleSlot *title_17ad0(int a0, int a1, int a2, int size, int a4);
void title_0c760(void *p);
void title_0c7b0(void);
void title_0c7f0(void *p);
void title_0c810(void *p);
void title_0c850(void *p);
int title_196c0(void *unused);

int title_19470(char *obj, int *vec)
{
    short v[3];

    v[0] = (short)(vec[0] >> 16);
    v[1] = (short)(vec[1] >> 16);
    v[2] = (short)(vec[2] >> 16);
    title_0c760(v);
    title_0c7b0();
    title_0c810(obj);
    obj += 4;
    title_0c850(obj);
    return *(short *)obj > 0x40;
}

int title_194e0(char *obj, int *vec, int k)
{
    short v[3];

    v[0] = (short)((((short *)g_engine_interface.data_014)[(k * 4 - 0x400) & 0xfff] * (short)(vec[0] >> 16)) >> 12);
    v[1] = (short)((((short *)g_engine_interface.data_014)[(k & 0x3ff) * 4] * (short)(vec[0] >> 16)) >> 12);
    v[2] = 0;
    title_0c760(v);
    title_0c7b0();
    title_0c810(obj);
    obj += 4;
    title_0c850(obj);
    return *(short *)obj > 0x40;
}

int title_19570(TitleShape *obj, int k)
{
    int h = g_2cbe8 >> 1;
    int q1 = (obj->unknown_03a * h) / obj->unknown_010;
    int q = q1;
    int m1, m, b;

    if (obj->unknown_054 & 0x80) {
        q = (obj->unknown_03c * h) / obj->unknown_010;
    }
    m1 = (q1 * k) >> 8;
    m = (q * k) >> 8;
    if ((unsigned int)(obj->unknown_00c + m1) > (unsigned int)(m1 * 2 + 0x140)) {
        return 1;
    }
    b = obj->unknown_00e + m;
    return (unsigned int)(m * 2 + 0xf0) < (unsigned int)b;
}

void title_19680(TitleRec *rec)
{
    rec->unknown_076 = (unsigned short)title_196c0(rec);
    title_0c810(rec->unknown_00c);
    title_0c7f0(rec->unknown_078);
    rec->unknown_010 = (short)(rec->unknown_080 / 4);
}

int title_196c0(void *unused)
{
    unsigned short *p = (unsigned short *)g_engine_interface.data_010;

    p[0] = 0;
    p[2] = 0;
    p[1] = 0x80;
    title_0c760(p);
    title_0c7b0();
    return 0;
}

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
