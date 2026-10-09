/* TITLE.DLL lane w21 region: 7 functions that reached MASKED EQUAL, ascending RVA.
 * Shared types, externs and prototypes first. Field names are offsets, not recovered types. */
#include "title_engine.h"
#include "title_screen.h"

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
#include "title_gpu.h"
extern char g_29128[];
extern char *g_engine_paths[];  /* title_files.c */
/* Initialized data of this unit (TITLE.DLL .data 0x25f50..0x2609f), in address order. The sequence
   screens read g_25f50; the engine path pairs share their strings with the 0x6350 unit (folded literals). */
short g_25f50[90] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, -1, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, -1,
};
struct TitleEnginePath { char *dir; char *name; };
struct TitleEnginePath g_26008[12] = {
    {"\\SRC1\\ENGINE\\", "ENGINE1"},
    {"\\SRC1\\ENGINE\\", "ENGINE1"},
    {"\\SRC3\\ENGINE\\", "ENGINE3"},
    {"\\SRC1\\ENGINE\\", "ENGINE1"},
    {"\\SRC1\\ENGINE\\", "ENGINE1"},
    {"\\SRC1\\ENGINE\\", "ENGINE1"},
    {"\\SRC1\\ENGINE\\", "ENGINE1"},
    {"\\SRC1\\ENGINE\\", "ENGINE1"},
    {"\\SRC3\\ENGINE\\", "ENGINE3"},
    {"\\SRC1\\ENGINE\\", "ENGINE1"},
    {"\\SRC3\\ENGINE\\", "ENGINE3"},
    {"\\SRC1\\ENGINE\\", "ENGINE1"},
};
int g_26068[14] = {
    1, 1, 1, 2, 1, 3, 2, 4, 3, 5, 5, 6, 5, 7,
};

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
        title_0c3d0(g_29128, g_engine_paths[(char)g_engine_interface.context_004->unknown_000[0]]);
    }
}

void title_16470(void)
{
    if (g_2acf0 != 0) {
        title_016e0("\n No Need to reload music !");
        return;
    }
    g_2acf0 = 1;
    title_0c410(0xc, 1);
    title_0c430(0x19, 2);
}

