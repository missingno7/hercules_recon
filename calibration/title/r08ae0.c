#include "title_engine.h"

typedef struct TitleObj {
    unsigned long dw00;
    unsigned long dw04;
    unsigned long dw08;
    unsigned char unknown_0c[16];
    unsigned char b1c;
    unsigned char unknown_1d[13];
    unsigned short w2a;
    unsigned short unknown_2c;
    unsigned short w2e;
    unsigned char unknown_30[8];
    unsigned short w38;
    unsigned char unknown_3a[22];
    unsigned long d50;
    unsigned long d54;
    char *p58;
    unsigned char unknown_5c[196];
    void *p120;
} TitleObj;

typedef struct TitleTab {
    long a;
    long b;
} TitleTab;

typedef struct TitleRec {
    unsigned char unknown_000[0xec];
    unsigned char b0ec;
    unsigned char b0ed;
    unsigned short w0ee;
    unsigned long d0f0;
    unsigned long d0f4;
    unsigned long d0f8;
    unsigned long d0fc;
    unsigned long d100;
    unsigned long d104;
    unsigned long d108;
    unsigned long d10c;
    unsigned long d110;
    short w114;
    unsigned short w116;
    unsigned long d118;
    short w11c;
    unsigned short w11e;
    unsigned long d120;
    unsigned short w124;
    unsigned short w126;
    unsigned long d128;
    unsigned long d12c;
    unsigned long d130;
} TitleRec;

typedef struct TitleSlot {
    unsigned char unknown_00[0x94];
    unsigned char b94;
    unsigned char b95;
    unsigned char b96;
    unsigned char b97;
    unsigned short w98;
    unsigned short w9a;
    unsigned long d9c;
    unsigned long da0;
    unsigned long da4;
    unsigned long da8;
    unsigned long dac;
    unsigned long db0;
    unsigned long db4;
    unsigned long db8;
    unsigned long dbc;
    unsigned long dc0;
    unsigned long dc4;
    unsigned long dc8;
    unsigned long dcc;
    unsigned long dd0;
    unsigned long dd4;
    unsigned long dd8;
    unsigned long ddc;
    unsigned long de0;
    unsigned long de4;
    unsigned long de8;
} TitleSlot;

extern TitleTab g_2a0a0[];
extern signed char g_2cc03;
extern char *g_2d320;
extern char *g_2df4c;
extern unsigned short g_2bf6c;
extern unsigned short g_2bff2;
extern unsigned short g_2bf38;
extern long *g_2bfa0[];

int title_08f50(TitleObj *p);
void title_098f0(TitleObj *p);
void title_097b0(TitleObj *p);
void title_04610(TitleObj *p);
void title_01dd0(void *p);
void title_09770(TitleObj *p);
void title_097f0(TitleObj *p);
void title_09830(TitleObj *p);
void title_09870(TitleObj *p);
void title_098a0(TitleObj *p);
void title_08cd0(void *p);
void title_090f0(void *p);
void title_094c0(TitleSlot *p);

int title_08ae0(TitleObj *p)
{
    char *frame;
    unsigned short index;
    unsigned short flags;

    if (p->w2e & 0x6000)
        return title_08f50(p);
    if (p->w2e == 0)
        return 1;
    if (p->d54 & 0x800) {
        title_098f0(p);
        return 0;
    }
    if (p->d50 & 0x10000000) {
        title_097b0(p);
        p->w2e = 0;
        title_04610(p);
        return 1;
    }
    if (p->p120)
        title_01dd0(p->p120);
    frame = p->p58;
    if (p->p58 == 0) {
        title_09770(p);
        title_097f0(p);
        title_09830(p);
        title_09870(p);
        title_097b0(p);
        title_098a0(p);
        /* Original stores the low word of the (null) frame register. */
        p->w2e = (unsigned short)(unsigned long)frame;
        title_04610(p);
        return 1;
    }
    index = p->w38;
    flags = *(unsigned short *)(frame + g_2a0a0[index].a + 0xc);
    frame += g_2a0a0[index].a;
    if (flags & 0x2000)
        return 0;
    if (flags & 0x100) {
        if (p->b1c != 0xff)
            p->w2a |= (unsigned short)(p->b1c << 8);
        if ((short)index == (short)g_2cc03) {
            *(unsigned short *)(frame + 0xe) = p->w2a;
            *(unsigned long *)(frame + 0) = p->dw00;
            *(unsigned long *)(frame + 4) = p->dw04;
            *(unsigned long *)(frame + 8) = p->dw08;
        }
    }
    if (*(unsigned char *)(frame + 0xc) & 0x80) {
        if (p->b1c != 0xff)
            p->w2a |= (unsigned short)(p->b1c << 8);
        if ((short)p->w38 == (short)g_2cc03)
            *(unsigned short *)(frame + 0xe) = p->w2a;
    }
    title_09770(p);
    title_09830(p);
    title_09870(p);
    title_097f0(p);
    title_097b0(p);
    title_098a0(p);
    p->w2e = 0;
    *(unsigned short *)(frame + 0xc) &= 0x7fff;
    title_04610(p);
    return 1;
}

void title_08c90(void)
{
    int i;
    char *base;

    i = 0;
    if (g_2bf6c > i) {
        int off;
        int n;

        off = 0;
        do {
            base = g_2d320;
            title_08cd0(base + off);
            n = g_2bf6c;
            i++;
            off += 0x134;
        } while (i < n);
    }
}

void title_08cd0(void *pv)
{
    TitleRec *p;
    unsigned long z;
    long m;

    p = pv;

    title_090f0(p);
    z = 0;
    m = -1;
    p->b0ed = (unsigned char)z;
    p->b0ec = (unsigned char)z;
    p->w0ee = 1;
    p->d0f8 = z;
    p->d0f4 = z;
    p->d0f0 = z;
    p->d104 = z;
    p->d100 = z;
    p->d0fc = z;
    p->d10c = z;
    p->d108 = z;
    p->d110 = z;
    p->w114 = m;
    p->w116 = (unsigned short)z;
    p->d118 = z;
    p->w11c = m;
    p->w11e = (unsigned short)z;
    p->d120 = z;
    p->w124 = (unsigned short)z;
    p->w126 = (unsigned short)z;
    p->d130 = z;
    p->d12c = z;
    p->d128 = z;
}

void title_09090(void)
{
    char *base;
    int i;

    i = 0;
    if (g_2bff2 > i) {
        int off;
        int n;

        off = 0;
        do {
            base = g_2df4c;
            title_090f0(base + off);
            n = g_2bff2;
            i++;
            off += 0xec;
        } while (i < n);
    }
    i = 0;
    if (g_2bf38 > i) {
        do {
            *g_2bfa0[i] = -1;
            i++;
        } while (i < g_2bf38);
    }
}

void title_090f0(void *pv)
{
    TitleSlot *p;
    unsigned long z;

    p = pv;

    title_094c0(p);
    z = 0;
    p->b94 = (unsigned char)z;
    p->b96 = (unsigned char)z;
    p->b95 = (unsigned char)z;
    p->b97 = (unsigned char)z;
    p->w9a = (unsigned short)z;
    p->w98 = (unsigned short)z;
    p->da4 = z;
    p->da0 = z;
    p->d9c = z;
    p->db4 = z;
    p->db0 = z;
    p->dac = z;
    p->da8 = z;
    p->dc4 = z;
    p->dc0 = z;
    p->dbc = z;
    p->db8 = z;
    p->dd4 = z;
    p->dd0 = z;
    p->dcc = z;
    p->dc8 = z;
    p->de0 = z;
    p->ddc = z;
    p->dd8 = z;
    p->de8 = z;
    p->de4 = z;
}
