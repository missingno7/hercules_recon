/* TITLE.DLL lane w21 region: 7 functions that reached MASKED EQUAL, ascending RVA.
 * Shared types, externs and prototypes first. Field names are offsets, not recovered types. */
#include "title_engine.h"

typedef struct TitleObject {
    unsigned long unknown_000;
    unsigned long unknown_004;
    unsigned char unknown_008[0x00c - 0x008];
    unsigned char unknown_00c[0x01f - 0x00c];
    unsigned char unknown_01f;
    unsigned char unknown_020[0x023 - 0x020];
    unsigned char unknown_023;
    unsigned char unknown_024[0x034 - 0x024];
    unsigned short unknown_034;
    unsigned char unknown_036[0x03e - 0x036];
    unsigned short unknown_03e;
    unsigned char unknown_040[0x04a - 0x040];
    unsigned short unknown_04a;
    unsigned char unknown_04c[0x054 - 0x04c];
    unsigned long unknown_054;
    unsigned char unknown_058[0x070 - 0x058];
    unsigned short unknown_070;
    unsigned short unknown_072;
    unsigned short unknown_074;
    unsigned char unknown_076[0x120 - 0x076];
    int *unknown_120;
} TitleObject;

typedef struct TitleVtable8 {
    void (*fn)(void *obj);
    unsigned long unknown_004;
} TitleVtable8;

typedef struct TitleVtable12 {
    void (*fn)(void *obj);
    unsigned char unknown_004[0x0c - 0x04];
} TitleVtable12;

extern TitleObject *g_2acb0;
extern TitleObject *g_2acc8;
extern TitleObject *g_2acbc;
extern int g_2acc0;
extern int g_2acb8;
extern int g_2acd0;
extern int g_2acd8;
extern unsigned short g_25f50[];
extern int g_2acf0;
extern char g_260a0[];
extern int g_2b370;
extern int g_2cc04;
extern int g_2cc08;
extern char g_29128[];
extern char *g_22288[];
extern TitleVtable8 *g_2bf34;
extern TitleVtable8 *g_2bf54;
extern TitleVtable12 *g_2bf2c;

TitleObject *title_17ad0(int a0, int a1, int a2, int size, int a4);
int title_0c4f0(int a);
void title_016e0(char *text);
void title_0c410(int a, int b);
void title_0c430(int a, int b);
void title_0c890(unsigned short *rect, int value);
void title_0cc70(int a);
void title_0c2f0(void);
void title_0c480(void);
void title_0c5c0(void);
void title_1d4e0(void);
void title_0c8e0(unsigned short *rect, int a, int b);
void title_0c370(int a, int b, int c, int d, int e);
void title_0cc80(void (*callback)(void));
void title_0c320(void);
void title_0c270(void);
void title_0c3d0(char *text, char *name);
void *title_08ed0(int a0, int a1, int a2, int size);
void *title_092e0(int a0, int a1, int a2, int size);
void *title_08a50(int a0, int a1, int a2, int size);
int title_195f0(TitleObject *obj);
int title_19470(void *a, void *b);
void title_04410(void *p);
void title_02dc0(void *p);
void title_0ca40(unsigned short *a, unsigned short *b);
void title_0c7d0(unsigned short *a);
void title_0c790(void);

void title_16110(void)
{
    g_2acb0 = title_17ad0(0, 0, 0, 0x2013, 0);
    g_2acb0->unknown_034 = 1;
    g_2acb0->unknown_054 |= 5;
    g_2acb0->unknown_04a = 0;

    g_2acc8 = title_17ad0(0, 0, 0, 0x2013, 0);
    g_2acc8->unknown_034 = 1;
    g_2acc8->unknown_054 |= 6;
    g_2acc8->unknown_04a = 0;

    g_2acb0->unknown_023 = 6;
    g_2acb0->unknown_03e = 10;
    g_2acc8->unknown_023 = 6;
    g_2acc8->unknown_03e = 20;
    g_2acb0->unknown_000 = 0xfff80000;
    g_2acb0->unknown_004 = 0xffe80000;
    g_2acc8->unknown_000 = 0xfff90000;
    g_2acc8->unknown_004 = 0xffe90000;

    g_2acbc = title_17ad0(0, 0, 0, 0x2013, 0);
    g_2acbc->unknown_034 = 2;
    g_2acbc->unknown_054 |= 0x10;
    g_2acbc->unknown_000 = 0x640000;
    g_2acbc->unknown_004 = 0x640000;
    g_2acc0 = 0;
    g_2acb8 = 0;
}

void title_16230(void)
{
    unsigned short code;

    g_2acd8 ^= 1;
    if (g_2acd8 != 0) {
        return;
    }
    switch (g_2acc0) {
    case 0:
        break;
    case 1:
        g_2acbc->unknown_034++;
        if (g_2acbc->unknown_034 != 0x10) {
            return;
        }
        g_2acc0 = 2;
        return;
    case 2:
        g_2acd0 = 0xc;
        g_2acc0 = 3;
        return;
    default:
        return;
    }
    g_2acbc->unknown_034 = g_25f50[g_2acb8] + 1;
    g_2acb8++;
    code = g_25f50[g_2acb8];
    if (code == 0xa && g_2acd0 == 0xa) {
        g_2acd0 = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    g_2acc0 = 1;
}

void title_16300(void)
{
    unsigned short box[8];
    int flag;
    box[4] = 0x390;
    box[5] = 0;
    box[6] = 0x50;
    box[7] = 0xc8;
    title_0c890(&box[4], g_2b370);

    if (g_engine_interface.context_004->unknown_0e4 != 0) {
        title_0cc70(0);
        title_0c2f0();
        title_0c480();
        while (g_engine_interface.context_004->suppress_auto_close_0e0 != 0 &&
               g_engine_interface.context_004->unknown_0e4 != 0) {
            title_0cc70(0);
            title_0c2f0();
            title_0c480();
        }
    }

    title_1d4e0();
    title_0c5c0();

    flag = (g_2cc04 == g_2cc08) << 8;
    box[0] = 0;
    box[2] = 0x140;
    box[3] = 0x100;
    box[1] = flag;
    title_0c8e0(box, 0x140, 0);
    title_0c8e0(box, 0, (g_2cc04 != g_2cc08) << 8);

    g_engine_interface.context_004->unknown_128 = 0x18;
    title_0c370(0x140, 0, 2, 2, 0);
    title_0cc80(title_0c320);

    if (g_engine_interface.context_004->unknown_0e4 != 0) {
        title_0c270();
    } else {
        title_0c3d0(g_29128, g_22288[(char)g_engine_interface.context_004->unknown_000[0]]);
    }
}

void title_16470(void)
{
    if (g_2acf0 != 0) {
        title_016e0(g_260a0);
        return;
    }
    g_2acf0 = 1;
    title_0c410(0xc, 1);
    title_0c430(0x19, 2);
}

TitleObject *title_17ad0(int a0, int a1, int a2, int size, int a4)
{
    TitleObject *obj;

    if (size & 0x6000) {
        if (size & 0x4000) {
            obj = title_08ed0(a0, a1, a2, size);
            if (obj != 0) {
                g_2bf34[size & 0xfff].fn(obj);
            }
        } else {
            obj = title_092e0(a0, a1, a2, size);
            if (obj != 0) {
                g_2bf54[size & 0xfff].fn(obj);
            }
        }
    } else {
        obj = title_08a50(a0, a1, a2, size);
        if (obj != 0) {
            g_2bf2c[size & 0xfff].fn(obj);
        }
    }
    return obj;
}

void title_17b90(TitleObject *obj)
{
    int *list;
    int count;
    TitleObject *item;

    if (title_195f0(obj) == -1) {
        obj->unknown_01f |= 8;
        return;
    }
    list = obj->unknown_120;
    count = *list++;
    while (count--) {
        item = (TitleObject *)*list++;
        title_04410(item);
        title_195f0(item);
        title_02dc0(item);
    }
}

void title_17bf0(TitleObject *obj)
{
    unsigned short buf[4];
    unsigned char out[0x20];
    int r;
    int *list;
    int count;
    TitleObject *item;

    r = title_195f0(obj);
    if (r == -1) {
        obj->unknown_01f |= 8;
        return;
    }
    list = obj->unknown_120;
    count = *list;
    list++;
    buf[0] = obj->unknown_070 << 2;
    buf[1] = r / 2 + (obj->unknown_072 << 2);
    buf[2] = obj->unknown_074 << 2;
    title_0ca40(buf, out);
    title_0c7d0(out);
    title_0c790();

    while (count--) {
        item = (TitleObject *)*list++;
        if (item != 0) {
            if (title_19470(&item->unknown_00c, item) != 0) {
                title_04410(item);
                title_02dc0(item);
            }
        }
    }
}
