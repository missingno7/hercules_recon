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
static int g_2acf0;
extern int g_2b370;
extern int g_2cc04;
extern int g_2cc08;
extern char g_29128[];
extern TitleVtable8 *g_2bf34;
extern TitleVtable8 *g_2bf54;
extern TitleVtable12 *g_2bf2c;
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
