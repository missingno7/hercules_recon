/* Sequence screen 9 unit: handler 0x15cb0 and helpers 0x16110, 0x16230.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"

typedef struct TitleVtable8 {
    void (*fn)(void *obj);
    unsigned long unknown_004;
} TitleVtable8;
typedef struct TitleVtable12 {
    void (*fn)(void *obj);
    unsigned char unknown_004[0x0c - 0x04];
} TitleVtable12;

static TitleObject *g_2acb0;
static TitleObject *g_2acc8;
static TitleObject *g_2acbc;
static int g_2acc0;
static int g_2acb8;
static int g_2acd0;
static int g_2acd8;
extern unsigned short g_25f50[];
extern int g_2acf0;
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

/* 0x15cb0: not reconstructed yet */

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
