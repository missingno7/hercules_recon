#include "title_engine.h"
#include "title_screen.h"

/* Initialized data of this unit (TITLE.DLL .data 0x254e8..0x257cb), in address order. */
typedef struct TitlePadMap {         /* eight button masks, copied whole by 0xa450 and 0x164b0 */
    short button[8];
} TitlePadMap;
TitlePadMap g_254e8[4] = {
    {{0x4000, 0x2000, 0x1000, 0x8000, 0x0400, 0x0800, 0x0400, 0x0800}},
    {{0x8000, 0x4000, 0x2000, 0x1000, 0x0400, 0x0800, 0x0400, 0x0800}},
    {{0x1000, 0x8000, 0x4000, 0x2000, 0x0400, 0x0800, 0x0400, 0x0800}},
    {{0x2000, 0x1000, 0x8000, 0x4000, 0x0400, 0x0800, 0x0400, 0x0800}},
};
/* Frame tables ending in -1; 999 entries carry a sound request in the next entry. */
short g_25528[32] = {  /* read by 0x9a40 */
    1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 4, 4,
    3, 3, 2, 2, 1, 1, -1, 0,
};
short g_25568[52] = {  /* read by 0x9a40 */
    1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 999, 783,
    6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
    12, 12, 13, 13, 999, 782, 999, 772, 14, 14, 15, 15,
    16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21,
    -1, 0, 0, 0,
};
short g_255d0[24] = {  /* read by 0x9a40 */
    1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 999, 774,
    6, 6, 7, 7, 8, 8, 999, 773, -1, 0, 0, 0,
};
short g_25600[28] = {  /* read by 0x9c90 */
    1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    6, 6, 6, 6, 6, 5, 5, 4, 4, 3, 3, 2,
    2, 1, 1, -1,
};
short g_25638[36] = {  /* read by 0x9c90 */
    1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 999, 774,
    6, 6, 7, 7, 8, 8, 9, 9, 999, 773, 10, 10,
    11, 11, 12, 12, 13, 13, 14, 14, 15, 15, -1, 0,
};
short g_25680[36] = {  /* read by 0x9c90 */
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18,
    999, 783, 999, 784, 19, 19, 21, 21, 22, 22, 23, 23,
    24, 24, 25, 25, 999, 775, 26, 26, 27, 27, -1, 0,
};
short g_256c8[36] = {  /* read by 0x9c90 */
    9, 9, 10, 10, 11, 11, 12, 12, 999, 783, 999, 781,
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 999, 775,
    18, 18, 19, 19, 20, 20, 21, 21, -1, 0, 0, 0,
};
short g_25710[28] = {  /* read by 0xa020 */
    1, 1, 2, 2, 3, 3, 3, 4, 999, 774, 5, 6,
    7, 999, 773, 8, 9, 9, 10, 10, 11, 11, 12, 12,
    -1, 0, 0, 0,
};
short g_25748[24] = {  /* read by 0xa020 */
    1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 9, 9, 8, -1,
};
short g_25778[12] = {  /* read by 0xa020 */
    11, 11, 12, 13, 14, 15, 15, 16, 17, -1, 0, 0,
};
short g_25790[30] = {  /* read by 0xa020 */
    16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 999, 781,
    999, 783, 21, 21, 22, 22, 23, 23, 24, 24, 999, 775,
    25, 25, 26, 26, -1, 0,
};
static int g_2a1b8;
static int g_2a1c4;
static int g_2a1e0;
static int g_2a1e8;
static int g_2a208;
static int g_2a220;
void title_05a70(TitleObject *p);
void title_09350(TitleObject *p);
void unlink_12c(TitleObject *p);
void title_0ca30(void);
void title_1d770(void);
void title_0c9c0(int a, int b);
int title_0c9e0(int a, int b);
unsigned int title_0c4a0(unsigned int a);
void title_09a40(void);
void title_09c90(void);
void title_0a020(void);
void title_01de0(int a, int b, int c, int d, int e);
void title_02090(int a, int b, int c, int d, int e, int f, int g, int h);

/* Options-screen view of g_engine_interface.context_004 (offsets observed in 0xa450). */
typedef struct TitleOptionsView {
    unsigned char unknown_000[0x0b];
    signed char field_00b;
    unsigned char unknown_00c[0x0e - 0x0c];
    signed char field_00e;
    unsigned char unknown_00f[0x6c - 0x0f];
    TitlePadMap pad_06c;
    unsigned char unknown_07c[0x1b0c - 0x7c];
    unsigned long volume_1b0c;
    unsigned long volume_1b10;
    unsigned char unknown_1b14[0x1b24 - 0x1b14];
    unsigned long stereo_1b24;
} TitleOptionsView;

/* Resource slot records (same layout as title_slots.h, which this unit does not include). */
typedef struct TitleRecord {
    char *path_00;
    unsigned short count_04;
    unsigned short unknown_06;
    void *unknown_08;
    unsigned char *table_0c;
    unsigned short state_10;
    unsigned short state_12;
} TitleRecord;
extern TitleRecord g_26110[18];
extern int g_29f98;
extern char g_29128[];
extern char *g_screen_files[];

static int g_2a1b4;
static int g_2a1bc;
static int g_2a1c0;
static TitleObject *g_2a1c8;
static TitleObject *g_2a1cc;
static TitleObject *g_2a1d0;
static TitleObject *g_2a1d4;
static int g_2a1d8;
static TitleObject *g_2a1dc;
static int g_2a1e4;
static int g_2a1ec;
static int g_2a1f0;
static TitleObject *g_2a1f4;
static TitleObject *g_2a1f8;
static TitleObject *g_2a1fc;
static TitleObject *g_2a200;
static TitleObject *g_2a204;
static int g_2a20c;
static int g_2a210;
static int g_2a214;
static int g_2a218;
static TitleObject *g_2a21c;
static int g_2a224;
static TitleObject *g_2a228;
static TitleObject *g_2a22c;
static TitleObject *g_2a230;
static TitleObject *g_2a234;
static int g_2a240;

TitleObject *title_17ad0(int a0, int a1, int a2, int size, int a4);
int title_04d80(int idx, int arg2);
void title_0c140(void);
void title_0c5a0(void);
void title_0c2e0(void);
void title_05bc0(int a);
void title_04ea0(int a);
void title_1d790(int a);
void title_054f0(int a, TitleObject *b);
void title_028c0(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
                 int a9, int a10);
void title_05e90();
void title_05f10(void *target);
void title_07510();
int title_0c580(int a, int b);
int title_0c450();
void title_016e0(const char *format, ...);
void title_0c3f0();
void title_01dd0();
void title_0c8b0();
void title_0c6d0(int a);
void title_0c330();
void title_04b50(int a);

void title_09a40(void)
{
    switch (g_2a1e8) {
    case 0:
        break;
    case 6:
        g_2a1e8 = 7;
        g_2a1f0 = 0;
        break;
    case 7:
        g_2a228->unknown_034 = g_255d0[g_2a1f0] + 0x70;
        while (g_255d0[++g_2a1f0] == 999) {
            title_054f0(g_255d0[g_2a1f0 + 1] | 0x4000, g_2a228);
            g_2a1f0++;
        }
        if (g_255d0[g_2a1f0] == -1)
            g_2a1e8 = 8;
        break;
    case 1:
        g_2a1e8 = 2;
        g_2a1f0 = 0;
        break;
    case 2:
        g_2a228->unknown_034 = g_25528[g_2a1f0] + 3;
        g_2a1f0++;
        if (g_25528[g_2a1f0] == -1) {
            g_2a228->unknown_034 = 0x87;
            g_2a1e8 = 0;
        }
        break;
    case 3:
        g_2a1e8 = 4;
        g_2a1f0 = 0;
        break;
    case 4:
        g_2a228->unknown_034 = g_25568[g_2a1f0] + 0x50;
        g_2a1f0++;
        g_2a1e8 = 5;
        break;
    case 5:
        g_2a228->unknown_034 = g_25568[g_2a1f0] + 0x50;
        while (g_25568[++g_2a1f0] == 999) {
            title_054f0(g_25568[g_2a1f0 + 1] | 0x4000, g_2a228);
            g_2a1f0++;
        }
        if (g_25568[g_2a1f0] == 14)
            g_2a230->unknown_054 &= 0x7fffffff;
        if (g_25568[g_2a1f0] == -1) {
            g_2a230->unknown_054 |= 0x80000000;
            g_2a230->unknown_034 = 0x8a;
            g_2a1e8 = 6;
            g_2a1e0 = 5;
        }
        break;
    }
}

void title_09c90(void)
{
    switch (g_2a1e0) {
    case 0:
    case 5:
        break;
    case 1:
        title_054f0(0x430a, g_2a230);
        g_2a1e0 = 2;
        g_2a1d8 = 0;
        break;
    case 2:
        g_2a230->unknown_034 = g_25600[g_2a1d8] + 0x6a;
        g_2a1d8++;
        if (g_25600[g_2a1d8] == -1) {
            g_2a230->unknown_034 = 0x8a;
            g_2a1e0 = 0;
        }
        break;
    case 3:
        g_2a1e0 = 4;
        g_2a1d8 = 0;
        break;
    case 4:
        g_2a230->unknown_034 = g_25638[g_2a1d8] + 0x36;
        while (g_25638[++g_2a1d8] == 999) {
            title_054f0(g_25638[g_2a1d8 + 1] | 0x4000, g_2a230);
            g_2a1d8++;
        }
        if (g_25638[g_2a1d8] == -1)
            g_2a1e0 = 5;
        break;
    case 6:
        g_2a1e0 = 7;
        g_2a1d8 = 0;
        break;
    case 7:
        g_2a230->unknown_034 = g_25680[g_2a1d8] + 0x1a;
        g_2a1d8++;
        g_2a234->unknown_054 &= 0x7fffffff;
        g_2a1e0 = 8;
        break;
    case 8:
        g_2a230->unknown_034 = g_25680[g_2a1d8] + 0x1a;
        while (g_25680[++g_2a1d8] == 999) {
            title_054f0(g_25680[g_2a1d8 + 1] | 0x4000, g_2a230);
            g_2a1d8++;
        }
        if (g_25680[g_2a1d8] == 0x18) {
            g_2a234->unknown_054 |= 0x80000000;
            g_2a234->unknown_034 = 0x8c;
        }
        if (g_25680[g_2a1d8] == -1) {
            g_2a1e0 = 3;
            g_2a1c4 = 3;
        }
        break;
    case 9:
        g_2a1e0 = 10;
        g_2a1d8 = 0;
        break;
    case 10:
        g_2a230->unknown_034 = g_256c8[g_2a1d8] + 0x70;
        g_2a1d8++;
        g_2a228->unknown_054 &= 0x7fffffff;
        g_2a1e0 = 11;
        break;
    case 11:
        g_2a230->unknown_034 = g_256c8[g_2a1d8] + 0x70;
        while (g_256c8[++g_2a1d8] == 999) {
            title_054f0(g_256c8[g_2a1d8 + 1] | 0x4000, g_2a230);
            g_2a1d8++;
        }
        if (g_256c8[g_2a1d8] == 0x11) {
            g_2a228->unknown_054 |= 0x80000000;
            g_2a228->unknown_034 = 0x87;
        }
        if (g_256c8[g_2a1d8] == -1) {
            g_2a1e0 = 3;
            g_2a1e8 = 8;
        }
        break;
    }
}

void title_0a020(void)
{
    switch (g_2a1c4) {
    case 0:
    case 3:
        break;
    case 1:
        g_2a1c4 = 2;
        g_2a1c0 = 0;
        break;
    case 2:
        g_2a234->unknown_034 = g_25710[g_2a1c0] + 0x1a;
        while (g_25710[++g_2a1c0] == 999) {
            title_054f0(g_25710[g_2a1c0 + 1] | 0x4000, g_2a234);
            g_2a1c0++;
        }
        if (g_25710[g_2a1c0] == -1)
            g_2a1c4 = 3;
        break;
    case 4:
        title_054f0(0x430c, g_2a234);
        g_2a1c4 = 5;
        g_2a1c0 = 0;
        break;
    case 5:
        g_2a22c->unknown_054 |= 0x80000000;
        g_2a22c->unknown_034 = g_25748[g_2a1c0] + 9;
        g_2a1c0++;
        if (g_25748[g_2a1c0] == -1) {
            g_2a1c4 = 6;
            g_2a1c0 = 0;
        }
        break;
    case 6:
        g_2a22c->unknown_054 &= 0x7fffffff;
        g_2a234->unknown_034 = g_25778[g_2a1c0] + 9;
        g_2a1c0++;
        if (g_25778[g_2a1c0] == -1)
            g_2a1c4 = 0;
        break;
    case 7:
        g_2a22c->unknown_054 &= 0x7fffffff;
        g_2a1c4 = 8;
        g_2a1c0 = 0;
        break;
    case 8:
        g_2a234->unknown_034 = g_25790[g_2a1c0] + 0x36;
        g_2a1c0++;
        g_2a230->unknown_054 &= 0x7fffffff;
        g_2a1c4 = 9;
        break;
    case 9:
        g_2a234->unknown_034 = g_25790[g_2a1c0] + 0x36;
        while (g_25790[++g_2a1c0] == 999) {
            title_054f0(g_25790[g_2a1c0 + 1] | 0x4000, g_2a234);
            g_2a1c0++;
        }
        if (g_25790[g_2a1c0] == 0x18) {
            g_2a230->unknown_054 |= 0x80000000;
            g_2a230->unknown_034 = 0x8a;
        }
        if (g_25790[g_2a1c0] == -1) {
            g_2a1c4 = 1;
            g_2a1e0 = 5;
        }
        break;
    }
}

void title_0a300(void)
{
    switch (g_2a208) {
    case 0:
        if (title_0c4a0(0x12c) < 2) {
            if (g_2a1e8 == 0 || g_2a1e8 == 8)
                g_2a1e8 = 1;
        }
        if (title_0c4a0(0x12c) < 2) {
            if (g_2a1e0 == 0 || g_2a1e0 == 5)
                g_2a1e0 = 1;
        }
        break;
    case 1:
        if (title_0c4a0(0x12c) < 2) {
            if (g_2a1e8 == 0 || g_2a1e8 == 8)
                g_2a1e8 = 1;
        }
        if (title_0c4a0(0xc8) < 2) {
            if (g_2a1c4 == 0 || g_2a1c4 == 3)
                g_2a1c4 = 4;
        }
        if (g_2a1e0 == 0)
            g_2a1e0 = 3;
        break;
    case 2:
        if (title_0c4a0(0x12c) < 2) {
            if (g_2a1e0 == 0 || g_2a1e0 == 5)
                g_2a1e0 = 1;
        }
        if (title_0c4a0(0x12c) < 2) {
            if (g_2a1c4 == 0 || g_2a1c4 == 3)
                g_2a1c4 = 4;
        }
        break;
    }
    title_09a40();
    title_09c90();
    title_0a020();
}

void title_0a450(TitleProc *self)
{
    char *buffer;
    unsigned long pressed = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5c;
    unsigned long held = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5e;
    unsigned int sfx;
    unsigned int music;
    int level1;
    int level2;

    switch (self->state_04) {
    case 0xfffe:
        title_05e90(title_07510, 0);
        title_05f10(self);
        return;
    case 0xffff:
        g_2a1c4 = 0;
        g_2a1e0 = 0;
        g_2a1e8 = 0;
        g_2a208 = 1;
        g_2a1e4 = 1;
        g_2a214 = 0;
        g_2a218 = 0;
        g_2a210 = 15;
        title_0c580(0, 0);
        buffer = 0;
        if (title_0c450(&buffer, 0x3d800, 0) == 0)
            title_016e0("\n Unable to malloc memory");
        title_0c3f0(g_29128, g_screen_files[12], buffer, 0x14312);
        title_0c3f0(g_29128, g_screen_files[13], buffer + 0x14312, 0x14312);
        title_0c3f0(g_29128, g_screen_files[14], buffer + 0x28624, 0x14312);
        title_01dd0(buffer);
        title_0c8b0(buffer + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_01dd0(buffer + 0x14312);
        title_0c6d0(0);
        title_0c8b0(buffer + 0x14624, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c8b0(buffer + 0x28936, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(buffer);
        title_04b50(2);
        g_2a20c = 0;
        g_2a1b8 = 0;
        g_2a1bc = 0;
        g_2a224 = 0;
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 1:
        title_0c140();
        if (g_26110[2].state_10 != 3 && title_04d80(2, 3) == 0) {
            self->state_04 = 1;
            self->delay_08 = 1;
            return;
        }
        g_2a21c = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a21c->unknown_034 = 3;
        g_2a21c->unknown_004 = 0x3c0000;
        g_2a21c->unknown_04a = 0;
        self->state_04 = 2;
        self->delay_08 = 1;
        return;
    case 2:
        title_0c140();
        if (g_29f98 == 0 && g_2a1b8 < 0x80)
            g_2a1b8 += 8;
        g_2a21c->unknown_04a = g_2a1b8;
        if (g_26110[2].state_10 != 3 && title_04d80(2, 0xa5) == 0) {
            self->state_04 = 2;
            self->delay_08 = 1;
            return;
        }
        self->state_04 = 3;
        self->delay_08 = 1;
        return;
    case 3:
        title_0c140();
        if (g_2a21c != 0) {
            title_09350(g_2a21c);
            g_2a21c = 0;
        }
        g_2a1cc = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a1cc->unknown_034 = 0x95;
        g_2a1cc->unknown_04a = g_2a1bc;
        g_2a1cc->unknown_054 |= 5;
        g_2a1f4 = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a1f4->unknown_034 = 0x95;
        g_2a1f4->unknown_04a = 0;
        g_2a1f4->unknown_004 = 0x20000;
        g_2a1f4->unknown_000 = 0x20000;
        g_2a1f4->unknown_054 |= 6;
        g_2a1c8 = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a1c8->unknown_034 = 1;
        g_2a1c8->unknown_04a = g_2a1bc;
        g_2a1c8->unknown_054 |= 5;
        g_2a1fc = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a1fc->unknown_04a = 0;
        g_2a1fc->unknown_004 = 0x20000;
        g_2a1fc->unknown_000 = 0x20000;
        g_2a1fc->unknown_054 |= 6;
        g_2a1d4 = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a1d4->unknown_034 = 0x9a;
        g_2a1d4->unknown_04a = g_2a1bc;
        g_2a1d4->unknown_054 |= 5;
        g_2a1f8 = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a1f8->unknown_034 = 0x9a;
        g_2a1f8->unknown_04a = 0;
        g_2a1f8->unknown_004 = 0x20000;
        g_2a1f8->unknown_000 = 0x20000;
        g_2a1f8->unknown_054 |= 6;
        g_2a1d0 = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a1d0->unknown_034 = 0x9c;
        g_2a1d0->unknown_04a = g_2a1bc;
        g_2a1d0->unknown_054 |= 5;
        g_2a204 = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a204->unknown_034 = 0x9a;
        g_2a204->unknown_04a = 0;
        g_2a204->unknown_004 = 0x20000;
        g_2a204->unknown_000 = 0x20000;
        g_2a204->unknown_054 |= 6;
        g_2a1dc = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a1dc->unknown_04a = g_2a1bc;
        g_2a1dc->unknown_004 = 0;
        g_2a1dc->unknown_000 = 0;
        g_2a1dc->unknown_054 |= 5;
        g_2a200 = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a200->unknown_034 = 0xa5;
        g_2a200->unknown_04a = 0;
        g_2a200->unknown_004 = 0x20000;
        g_2a200->unknown_000 = 0x20000;
        g_2a200->unknown_054 |= 6;
        g_2a200->unknown_034 = 0x9e;
        g_2a1dc->unknown_034 = 0x9e;
        g_2a1c8->unknown_054 &= 0x7fffffff;
        g_2a1d4->unknown_054 &= 0x7fffffff;
        g_2a1d0->unknown_054 &= 0x7fffffff;
        g_2a1dc->unknown_054 &= 0x7fffffff;
        g_2a1fc->unknown_054 &= 0x7fffffff;
        g_2a1f8->unknown_054 &= 0x7fffffff;
        g_2a204->unknown_054 &= 0x7fffffff;
        g_2a200->unknown_054 &= 0x7fffffff;
        g_2a1cc->unknown_023 = 6;
        g_2a1cc->unknown_03e = 10;
        g_2a1f4->unknown_023 = 6;
        g_2a1f4->unknown_03e = 20;
        g_2a1c8->unknown_023 = 6;
        g_2a1c8->unknown_03e = 10;
        g_2a1fc->unknown_023 = 6;
        g_2a1fc->unknown_03e = 20;
        g_2a1d4->unknown_023 = 6;
        g_2a1d4->unknown_03e = 10;
        g_2a1f8->unknown_023 = 6;
        g_2a1f8->unknown_03e = 20;
        g_2a1d0->unknown_023 = 6;
        g_2a1d0->unknown_03e = 10;
        g_2a204->unknown_023 = 6;
        g_2a204->unknown_03e = 20;
        g_2a1dc->unknown_023 = 6;
        g_2a1dc->unknown_03e = 10;
        g_2a200->unknown_023 = 6;
        g_2a200->unknown_03e = 20;
        g_2a228 = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a228->unknown_034 = 0x87;
        g_2a228->unknown_004 = 0x3c0000;
        g_2a230 = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a230->unknown_034 = 0x89;
        g_2a230->unknown_004 = 0x3c0000;
        g_2a234 = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a234->unknown_034 = 0x8c;
        g_2a234->unknown_004 = 0x3c0000;
        g_2a228->unknown_023 = 6;
        g_2a228->unknown_03e = 30;
        g_2a230->unknown_023 = 6;
        g_2a230->unknown_03e = 30;
        g_2a234->unknown_023 = 6;
        g_2a234->unknown_03e = 30;
        g_2a22c = title_17ad0(0, 0, 0, 0x2004, 0);
        g_2a22c->unknown_034 = 10;
        g_2a22c->unknown_004 = 0x3c0000;
        g_2a22c->unknown_023 = 6;
        g_2a22c->unknown_03e = 30;
        self->state_04 = 4;
        self->delay_08 = 1;
        return;
    case 4:
        switch (g_2a20c) {
        case 1:
            g_2a1cc->unknown_054 |= 5;
            g_2a1d4->unknown_054 |= 5;
            g_2a1d0->unknown_054 |= 5;
            g_2a1dc->unknown_054 |= 5;
            if (g_2a1bc > 0) {
                g_2a1bc -= 0x10;
            } else {
                g_2a1bc = 0;
                if (g_2a1b8 > 0) {
                    g_2a1b8 -= 0x10;
                } else {
                    g_2a1b8 = 0;
                    title_0c5a0();
                    title_0c2e0();
                    title_05bc0(0x303);
                    if (g_2a228 != 0) {
                        title_09350(g_2a228);
                        g_2a228 = 0;
                    }
                    if (g_2a230 != 0) {
                        title_09350(g_2a230);
                        g_2a230 = 0;
                    }
                    if (g_2a234 != 0) {
                        title_09350(g_2a234);
                        g_2a234 = 0;
                    }
                    if (g_2a22c != 0) {
                        title_09350(g_2a22c);
                        g_2a22c = 0;
                    }
                    if (g_2a1cc != 0) {
                        title_09350(g_2a1cc);
                        g_2a1cc = 0;
                    }
                    if (g_2a1f4 != 0) {
                        title_09350(g_2a1f4);
                        g_2a1f4 = 0;
                    }
                    if (g_2a1d4 != 0) {
                        title_09350(g_2a1d4);
                        g_2a1d4 = 0;
                    }
                    if (g_2a1d0 != 0) {
                        title_09350(g_2a1d0);
                        g_2a1d0 = 0;
                    }
                    if (g_2a1dc != 0) {
                        title_09350(g_2a1dc);
                        g_2a1dc = 0;
                    }
                    if (g_2a1f8 != 0) {
                        title_09350(g_2a1f8);
                        g_2a1f8 = 0;
                    }
                    if (g_2a204 != 0) {
                        title_09350(g_2a204);
                        g_2a204 = 0;
                    }
                    if (g_2a200 != 0) {
                        title_09350(g_2a200);
                        g_2a200 = 0;
                    }
                    if (g_2a1c8 != 0) {
                        title_09350(g_2a1c8);
                        g_2a1c8 = 0;
                    }
                    if (g_2a1fc != 0) {
                        title_09350(g_2a1fc);
                        g_2a1fc = 0;
                    }
                    title_04ea0(2);
                    title_1d790(1);
                    self->state_04 = 0xfffe;
                    self->delay_08 = 2;
                    return;
                }
            }
            g_2a1c8->unknown_04a = g_2a1bc;
            g_2a1fc->unknown_04a = g_2a1bc;
            g_2a1cc->unknown_04a = g_2a1bc;
            g_2a1d4->unknown_04a = g_2a1bc;
            g_2a1d0->unknown_04a = g_2a1bc;
            g_2a1dc->unknown_04a = g_2a1bc;
            g_2a1f4->unknown_04a = g_2a1bc;
            g_2a1f8->unknown_04a = g_2a1bc;
            g_2a204->unknown_04a = g_2a1bc;
            g_2a200->unknown_04a = g_2a1bc;
            g_2a22c->unknown_04a = g_2a1b8;
            g_2a234->unknown_04a = g_2a1b8;
            g_2a230->unknown_04a = g_2a1b8;
            g_2a228->unknown_04a = g_2a1b8;
            title_0c140();
            break;
        case 0:
            title_0c140();
            g_2a22c->unknown_04a = g_2a1b8;
            g_2a234->unknown_04a = g_2a1b8;
            g_2a230->unknown_04a = g_2a1b8;
            g_2a228->unknown_04a = g_2a1b8;
            g_2a1c8->unknown_04a = g_2a1bc;
            g_2a1fc->unknown_04a = g_2a1bc;
            g_2a1cc->unknown_04a = g_2a1bc;
            g_2a1d4->unknown_04a = g_2a1bc;
            g_2a1d0->unknown_04a = g_2a1bc;
            g_2a1dc->unknown_04a = g_2a1bc;
            g_2a1f4->unknown_04a = g_2a1bc;
            g_2a1f8->unknown_04a = g_2a1bc;
            g_2a204->unknown_04a = g_2a1bc;
            g_2a200->unknown_04a = g_2a1bc;
            if (g_2a1bc < 0x80) {
                g_2a1bc += 0x10;
            } else {
                g_2a1cc->unknown_054 &= ~5;
                g_2a1d4->unknown_054 &= ~5;
                g_2a1d0->unknown_054 &= ~5;
                g_2a1dc->unknown_054 &= ~5;
            }
            if (g_29f98 != 0)
                break;
            if (g_2a1b8 < 0x80)
                g_2a1b8 += 0x10;
            title_0a300();
            if (pressed == 0)
                g_2a210 = 0;
            if (g_2a210 == 0) {
                switch (g_2a1e4) {
                case 4:
                    g_2a1cc->unknown_034 = 0x97;
                    if ((held & 0x5000) || (held & 8)) {
                        title_054f0(0x303, 0);
                        g_2a20c = 1;
                    }
                    if (pressed & 0x10) {
                        title_054f0(0x301, 0);
                        switch (g_2a1ec) {
                        case 2:
                            g_2a1cc->unknown_034 = 0x96;
                            g_2a1e4 = 2;
                            break;
                        case 1:
                            g_2a1cc->unknown_034 = 0x95;
                            g_2a1e4 = 1;
                            break;
                        case 0:
                            g_2a1cc->unknown_034 = 0x94;
                            g_2a1e4 = 0;
                            break;
                        }
                        g_2a210 = 15;
                    }
                    break;
                case 2:
                    g_2a1ec = 2;
                    g_2a1cc->unknown_034 = 0x96;
                    g_2a1c8->unknown_034 = 1;
                    g_2a1fc->unknown_034 = 1;
                    g_2a1dc->unknown_054 &= 0x7fffffff;
                    g_2a200->unknown_054 &= 0x7fffffff;
                    if (pressed & 0x40) {
                        title_054f0(0x301, 0);
                        g_2a1cc->unknown_034 = 0x97;
                        g_2a1e4 = 4;
                    } else if (pressed & 0x20) {
                        title_054f0(0x301, 0);
                        g_2a1cc->unknown_034 = 0x95;
                        g_2a1e4 = 1;
                        g_2a210 = 15;
                    } else if (held & 0x4000) {
                        title_054f0(0x302, 0);
                        if (((TitleOptionsView *)g_engine_interface.context_004)->pad_06c.button[0] & 0x4000)
                            g_2a1cc->unknown_034 = 0x9d;
                        if (((TitleOptionsView *)g_engine_interface.context_004)->pad_06c.button[0] & 0x8000)
                            g_2a1cc->unknown_034 = 0x92;
                        if (((TitleOptionsView *)g_engine_interface.context_004)->pad_06c.button[0] & 0x1000)
                            g_2a1cc->unknown_034 = 0x93;
                        if (((TitleOptionsView *)g_engine_interface.context_004)->pad_06c.button[0] & 0x2000)
                            g_2a1cc->unknown_034 = 0x91;
                        g_2a1dc->unknown_054 |= 0x80000000;
                        g_2a200->unknown_054 |= 0x80000000;
                        g_2a1e4 = 6;
                        g_2a210 = 15;
                    }
                    break;
                case 1:
                    g_2a1ec = 1;
                    g_2a1cc->unknown_034 = 0x95;
                    g_2a1c8->unknown_034 = 1;
                    g_2a1fc->unknown_034 = 1;
                    if (pressed & 0x40) {
                        title_054f0(0x301, 0);
                        g_2a1cc->unknown_034 = 0x97;
                        g_2a1e4 = 4;
                    } else if (pressed & 0x20) {
                        title_054f0(0x301, 0);
                        g_2a1cc->unknown_034 = 0x94;
                        g_2a1e4 = 0;
                        g_2a210 = 15;
                    } else if (pressed & 0x80) {
                        title_054f0(0x300, 0);
                        g_2a1cc->unknown_034 = 0x96;
                        g_2a1e4 = 2;
                        g_2a210 = 15;
                    } else if (held & 0x4000) {
                        title_054f0(0x302, 0);
                        g_2a218 = 0;
                        if (((TitleOptionsView *)g_engine_interface.context_004)->stereo_1b24 == 0)
                            g_2a1cc->unknown_034 = 0x9f;
                        else
                            g_2a1cc->unknown_034 = 0xa2;
                        g_2a1b4 = 0;
                        g_2a1e4 = 7;
                        g_2a210 = 15;
                    }
                    break;
                case 0:
                    g_2a1ec = 0;
                    g_2a1cc->unknown_034 = 0x94;
                    g_2a1cc->unknown_054 |= 0x80000000;
                    g_2a1c8->unknown_034 = 1;
                    g_2a1fc->unknown_034 = 1;
                    if (pressed & 0x40) {
                        title_054f0(0x301, 0);
                        g_2a1cc->unknown_034 = 0x97;
                        g_2a1e4 = 4;
                    } else if (pressed & 0x80) {
                        title_054f0(0x301, 0);
                        g_2a1cc->unknown_034 = 0x95;
                        g_2a1e4 = 1;
                        g_2a210 = 15;
                    } else if (held & 0x4000) {
                        title_054f0(0x302, 0);
                        g_2a1cc->unknown_034 = 0x8f;
                        g_2a1e4 = 8;
                        g_2a210 = 15;
                    }
                    break;
                case 6:
                    g_2a1c8->unknown_034 = 2;
                    g_2a1fc->unknown_034 = 2;
                    if (held & 0x1000) {
                        title_054f0(0x303, 0);
                        g_2a1e4 = 2;
                        g_2a210 = 15;
                        g_2a240 = 1;
                    }
                    if (pressed & 0x20) {
                        title_054f0(0x302, 0);
                        switch (g_2a1cc->unknown_034) {
                        case 0x9d:
                            g_2a1cc->unknown_034 = 0x92;
                            break;
                        case 0x92:
                            g_2a1cc->unknown_034 = 0x93;
                            break;
                        case 0x93:
                            g_2a1cc->unknown_034 = 0x91;
                            break;
                        case 0x91:
                            g_2a1cc->unknown_034 = 0x9d;
                            break;
                        }
                        g_2a210 = 15;
                    }
                    if (pressed & 0x80) {
                        title_054f0(0x302, 0);
                        switch (g_2a1cc->unknown_034) {
                        case 0x9d:
                            g_2a1cc->unknown_034 = 0x91;
                            break;
                        case 0x92:
                            g_2a1cc->unknown_034 = 0x9d;
                            break;
                        case 0x93:
                            g_2a1cc->unknown_034 = 0x92;
                            break;
                        case 0x91:
                            g_2a1cc->unknown_034 = 0x93;
                            break;
                        }
                        g_2a210 = 15;
                    }
                    switch (g_2a1cc->unknown_034) {
                    case 0x9d:
                        ((TitleOptionsView *)g_engine_interface.context_004)->pad_06c = g_254e8[0];
                        break;
                    case 0x92:
                        ((TitleOptionsView *)g_engine_interface.context_004)->pad_06c = g_254e8[1];
                        break;
                    case 0x93:
                        ((TitleOptionsView *)g_engine_interface.context_004)->pad_06c = g_254e8[2];
                        break;
                    case 0x91:
                        ((TitleOptionsView *)g_engine_interface.context_004)->pad_06c = g_254e8[3];
                        break;
                    }
                    break;
                case 7:
                    g_2a1c8->unknown_034 = 2;
                    g_2a1fc->unknown_034 = 2;
                    if (held & 0x1000) {
                        title_054f0(0x303, 0);
                        g_2a1e4 = 1;
                        g_2a210 = 15;
                        g_2a240 = 1;
                    }
                    switch (g_2a218) {
                    case 2:
                        if (((TitleOptionsView *)g_engine_interface.context_004)->stereo_1b24 == 0)
                            g_2a1cc->unknown_034 = 0xa3;
                        else
                            g_2a1cc->unknown_034 = 0xa4;
                        if ((pressed & 0x80) && ((TitleOptionsView *)g_engine_interface.context_004)->stereo_1b24 != 0) {
                            title_054f0(0x301, 0);
                            ((TitleOptionsView *)g_engine_interface.context_004)->stereo_1b24 = 0;
                        }
                        if ((pressed & 0x20) && ((TitleOptionsView *)g_engine_interface.context_004)->stereo_1b24 != 1) {
                            title_054f0(0x301, 0);
                            ((TitleOptionsView *)g_engine_interface.context_004)->stereo_1b24 = 1;
                        }
                        if (pressed & 0x10) {
                            title_054f0(0x301, 0);
                            g_2a218 = 1;
                            if (((TitleOptionsView *)g_engine_interface.context_004)->stereo_1b24 == 0) {
                                g_2a1cc->unknown_034 = 0xa0;
                                g_2a210 = 15;
                            } else {
                                g_2a1cc->unknown_034 = 0xa1;
                                g_2a210 = 15;
                            }
                        }
                        break;
                    case 1:
                        if (((TitleOptionsView *)g_engine_interface.context_004)->stereo_1b24 == 0)
                            g_2a1cc->unknown_034 = 0xa0;
                        else
                            g_2a1cc->unknown_034 = 0xa1;
                        if (pressed & 0x10) {
                            title_054f0(0x301, 0);
                            g_2a218 = 0;
                            if (((TitleOptionsView *)g_engine_interface.context_004)->stereo_1b24 == 0)
                                g_2a1cc->unknown_034 = 0x9f;
                            else
                                g_2a1cc->unknown_034 = 0xa2;
                            g_2a210 = 15;
                        }
                        if (pressed & 0x40) {
                            title_054f0(0x300, 0);
                            g_2a218 = 2;
                            if (((TitleOptionsView *)g_engine_interface.context_004)->stereo_1b24 == 0)
                                g_2a1cc->unknown_034 = 0xa3;
                            else
                                g_2a1cc->unknown_034 = 0xa4;
                            g_2a210 = 15;
                        }
                        if ((pressed & 0x80) && ((TitleOptionsView *)g_engine_interface.context_004)->volume_1b10 > 0) {
                            if (++g_2a1b4 > 5) {
                                g_2a1b4 = 0;
                                title_054f0(0x302, 0);
                            }
                            ((TitleOptionsView *)g_engine_interface.context_004)->volume_1b10 -= 8;
                        } else if ((pressed & 0x20) && ((TitleOptionsView *)g_engine_interface.context_004)->volume_1b10 < 0xff) {
                            if (++g_2a1b4 > 5) {
                                g_2a1b4 = 0;
                                title_054f0(0x302, 0);
                            }
                            ((TitleOptionsView *)g_engine_interface.context_004)->volume_1b10 += 8;
                        } else {
                            g_2a1b4 = 0;
                        }
                        break;
                    case 0:
                        if (((TitleOptionsView *)g_engine_interface.context_004)->stereo_1b24 == 0)
                            g_2a1cc->unknown_034 = 0x9f;
                        else
                            g_2a1cc->unknown_034 = 0xa2;
                        if (pressed & 0x40) {
                            title_054f0(0x300, 0);
                            g_2a218 = 1;
                            if (((TitleOptionsView *)g_engine_interface.context_004)->stereo_1b24 == 0)
                                g_2a1cc->unknown_034 = 0xa0;
                            else
                                g_2a1cc->unknown_034 = 0xa1;
                            g_2a210 = 15;
                        }
                        if ((pressed & 0x80) && ((TitleOptionsView *)g_engine_interface.context_004)->volume_1b0c > 0)
                            ((TitleOptionsView *)g_engine_interface.context_004)->volume_1b0c -= 8;
                        if ((pressed & 0x20) && ((TitleOptionsView *)g_engine_interface.context_004)->volume_1b0c < 0xff)
                            ((TitleOptionsView *)g_engine_interface.context_004)->volume_1b0c += 8;
                        break;
                    }
                    break;
                case 8:
                    g_2a1c8->unknown_034 = 2;
                    g_2a1fc->unknown_034 = 2;
                    if (held & 0x1000) {
                        title_054f0(0x303, 0);
                        g_2a1e4 = 0;
                        g_2a210 = 15;
                        g_2a240 = 1;
                    }
                    if (g_2a1cc->unknown_034 == 0x8f) {
                        if (pressed & 0x40) {
                            title_054f0(0x301, 0);
                            g_2a1cc->unknown_034 = 0x90;
                            g_2a210 = 15;
                        }
                        if ((pressed & 0x80) && ((TitleOptionsView *)g_engine_interface.context_004)->field_00b > 2) {
                            ((TitleOptionsView *)g_engine_interface.context_004)->field_00b--;
                            g_2a210 = 15;
                            title_054f0(0x302, 0);
                        }
                        if ((pressed & 0x20) && ((TitleOptionsView *)g_engine_interface.context_004)->field_00b < 4) {
                            ((TitleOptionsView *)g_engine_interface.context_004)->field_00b++;
                            g_2a210 = 15;
                            title_054f0(0x302, 0);
                        }
                    }
                    if (g_2a1cc->unknown_034 == 0x90) {
                        if (pressed & 0x10) {
                            title_054f0(0x300, 0);
                            g_2a1cc->unknown_034 = 0x8f;
                            g_2a210 = 15;
                        }
                        if ((pressed & 0x80) && ((TitleOptionsView *)g_engine_interface.context_004)->field_00e > 0) {
                            ((TitleOptionsView *)g_engine_interface.context_004)->field_00e--;
                            g_2a210 = 15;
                            title_054f0(0x302, 0);
                        }
                        if ((pressed & 0x20) && ((TitleOptionsView *)g_engine_interface.context_004)->field_00e < 2) {
                            ((TitleOptionsView *)g_engine_interface.context_004)->field_00e++;
                            g_2a210 = 15;
                            title_054f0(0x302, 0);
                        }
                    }
                    break;
                }
                if (g_2a1e4 <= 2) {
                    if (g_2a240 == 0) {
                        if (held & 0x1000) {
                            title_054f0(0x303, 0);
                            g_2a20c = 1;
                        }
                    } else if (!(held & 0x1000)) {
                        g_2a240 = 0;
                    }
                }
                switch (g_2a208) {
                case 2:
                    if (g_2a224 == 0x66)
                        g_2a224 = 0;
                    if (g_2a224 == 0) {
                        if (g_2a1e4 == 1 || g_2a1e4 == 7 || g_2a1e4 == 2 || g_2a1e4 == 6)
                            g_2a224 = 0x65;
                    }
                    if (g_2a224 == 0x65) {
                        if (g_2a1e0 == 0 || g_2a1e0 == 5 || g_2a1e0 == 2) {
                            if (g_2a1e8 == 0 || g_2a1e8 == 8) {
                                g_2a224 = 0;
                                g_2a208 = 1;
                                g_2a1e8 = 999;
                                g_2a1e0 = 9;
                                g_2a210 = 15;
                            }
                        }
                    }
                    break;
                case 1:
                    if (g_2a224 == 0) {
                        if (g_2a1e4 == 2 || g_2a1e4 == 6)
                            g_2a224 = 0x7a;
                    }
                    if (g_2a224 == 0x7a) {
                        if (g_2a1c4 == 0 || g_2a1c4 == 3 || g_2a1c4 == 5 || g_2a1c4 == 6) {
                            if (g_2a1e0 == 0 || g_2a1e0 == 5 || g_2a1e0 == 2) {
                                g_2a224 = 0;
                                g_2a208 = 0;
                                g_2a1e0 = 999;
                                g_2a1c4 = 7;
                                g_2a210 = 15;
                            }
                        }
                    }
                    if (g_2a224 == 0) {
                        if (g_2a1e4 == 0 || g_2a1e4 == 8)
                            g_2a224 = 0x6e;
                    }
                    if (g_2a224 == 0x6e) {
                        if (g_2a1e8 == 0 || g_2a1e8 == 8 || g_2a1e8 == 2) {
                            if (g_2a1e0 == 0 || g_2a1e0 == 5 || g_2a1e0 == 2) {
                                g_2a224 = 0;
                                g_2a208 = 2;
                                g_2a1e8 = 3;
                                g_2a1e0 = 999;
                                g_2a210 = 15;
                            }
                        }
                    }
                    break;
                case 0:
                    if (g_2a224 != 0x64)
                        g_2a224 = 0;
                    if (g_2a224 == 0) {
                        if (g_2a1e4 == 1 || g_2a1e4 == 7 || g_2a1e4 == 0 || g_2a1e4 == 8)
                            g_2a224 = 0x65;
                    }
                    if (g_2a224 != 0) {
                        if (g_2a1e0 == 0 || g_2a1e0 == 5 || g_2a1e0 == 2) {
                            if (g_2a1c4 == 0 || g_2a1c4 == 3 || g_2a1c4 == 5 || g_2a1c4 == 6) {
                                g_2a224 = 0;
                                g_2a208 = 1;
                                g_2a1c4 = 999;
                                g_2a22c->unknown_054 &= 0x7fffffff;
                                g_2a1e0 = 6;
                                g_2a210 = 15;
                            }
                        }
                    }
                    break;
                }
            } else {
                g_2a210--;
            }
            break;
        }
        if (g_2a1cc->unknown_034 == 0x9f || g_2a1cc->unknown_034 == 0xa0 || g_2a1cc->unknown_034 == 0xa1
            || g_2a1cc->unknown_034 == 0xa2 || g_2a1cc->unknown_034 == 0xa3 || g_2a1cc->unknown_034 == 0xa4) {
            level1 = 0x40;
            level2 = 0x40;
            music = ((TitleOptionsView *)g_engine_interface.context_004)->volume_1b10 * 60 >> 8;
            sfx = ((TitleOptionsView *)g_engine_interface.context_004)->volume_1b0c * 60 >> 8;
            if (g_2a218 == 0)
                level1 = 0x80;
            if (g_2a218 == 1) {
                level1 = 0x40;
                level2 = 0x80;
            }
            title_028c0(0x7e, 0x51, sfx, 8, 0, 0x80, 0, level1, 1, 0, 0x4fe);
            title_028c0(sfx + 0x7e, 0x51, 0x3c - sfx, 8, 0, 0, 0xff, level1, 1, 1, 0x4fe);
            title_028c0(0x7e, 0x7c, music, 8, 0, 0x80, 0, level2, 1, 2, 0x4fe);
            title_028c0(music + 0x7e, 0x7c, 0x3c - music, 8, 0, 0, 0xff, level2, 1, 3, 0x4fe);
            title_0c2e0();
        }
        if (g_2a1cc->unknown_034 != 0x8f && g_2a1cc->unknown_034 != 0x90) {
            g_2a1d4->unknown_054 &= 0x7fffffff;
            g_2a1d0->unknown_054 &= 0x7fffffff;
            g_2a1f8->unknown_054 &= 0x7fffffff;
            g_2a204->unknown_054 &= 0x7fffffff;
        } else {
            g_2a1d4->unknown_054 |= 0x80000000;
            g_2a1d0->unknown_054 |= 0x80000000;
            g_2a1f8->unknown_054 |= 0x80000000;
            g_2a204->unknown_054 |= 0x80000000;
            switch (((TitleOptionsView *)g_engine_interface.context_004)->field_00b) {
            case 4:
                g_2a1d4->unknown_034 = 0x9b;
                break;
            case 3:
                g_2a1d4->unknown_034 = 0x9c;
                break;
            case 2:
                g_2a1d4->unknown_034 = 0xa5;
                break;
            }
            switch (((TitleOptionsView *)g_engine_interface.context_004)->field_00e) {
            case 2:
                g_2a1d0->unknown_034 = 0x98;
                break;
            case 1:
                g_2a1d0->unknown_034 = 0x99;
                break;
            case 0:
                g_2a1d0->unknown_034 = 0x9a;
                break;
            }
        }
        g_2a1f8->unknown_034 = g_2a1d4->unknown_034;
        g_2a204->unknown_034 = g_2a1d0->unknown_034;
        g_2a200->unknown_034 = g_2a1dc->unknown_034;
        g_2a1f4->unknown_034 = g_2a1cc->unknown_034;
        self->state_04 = 4;
        self->delay_08 = 1;
        return;
    }
}

void title_0c140(void)
{
    title_01de0(0x140, 0, g_2a1b8, 0, 0);
    title_02090(0x140, 0x100, g_2a1b8, 3, g_2a220, 0, 0, 0x4ec);
    title_02090(0x140, 0x100, g_2a1b8, 3, g_2a220 + 0x13f, 0, 1, 0x4ec);
    title_01de0(0x280, 0x100, g_2a1b8, 0, 0x4f1);
    g_2a220--;
    if (g_2a220 < -320)
        g_2a220 += 320;
}
