#include "title_engine.h"
#include "title_screen.h"

/* Initialized data of this unit (TITLE.DLL .data 0x254e8..0x257cb), in address order. */
typedef struct TitlePadMap {         /* eight button masks, copied whole by 0xa450 and 0x164b0 */
    unsigned short button[8];
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
