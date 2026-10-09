/* TITLE object-system unit (C++), 0x6bb0..0x97ef: menu objects (0x6bb0, 0x6fb0, 0x71e0), the main menu
 * process (0x7510) and the object pools (0x8140..0x97ef). One object: it defines the per-object .bss
 * block 0x29fec..0x2a0b0, whose 0x2a088 is used only by 0x71e0; its .data 0x23668..0x243af follows the
 * level table; code is contiguous. C++ by 0x8a50/0x8ed0 (masked equal only as C++). */

extern "C" {
#include "title_engine.h"
#include "title_screen.h"
#include "title_files.h"
#include "title_slots.h"


/* ---- shared record types ---- */

typedef struct TitleTab {
    long a;
    long b;
} TitleTab;

typedef struct TitleCfg {
    int a_00;
    int b_04;
    int c_08;
    unsigned char d_0c;
    unsigned char unknown_0d[3];
} TitleCfg;

typedef struct Rec8 {
    unsigned long d0;
    unsigned char b4, b5, b6, b7;
} Rec8;

typedef struct TitleCell94 {
    unsigned char unknown_00[0x22];
    unsigned char b22;
    unsigned char unknown_23[0x0b];
    unsigned short w2e;
    unsigned char unknown_30[0x20];
    unsigned long d50;
    unsigned char unknown_54[0x40];
} TitleCell94;

typedef struct TitleRecEC {
    unsigned char unknown_00[0x22];
    unsigned char b22;
    unsigned char unknown_23[0x0b];
    unsigned short w2e;
    unsigned char unknown_30[0x20];
    unsigned long d50;
    unsigned char unknown_54[0x98];
} TitleRecEC;

typedef struct TitleObj {
    int x_00;                      /* signed in 0x8320; copied as dw00 by 0x8ae0/0x8f50 */
    int y_04;                      /* signed in 0x8320; copied as dw04 by 0x8ae0/0x8f50 */
    unsigned long dw08;
    unsigned char unknown_0c[16];
    unsigned char b1c;
    unsigned char unknown_1d[6];
    unsigned char b23;
    unsigned char unknown_24[6];
    unsigned short w2a;
    unsigned short unknown_2c;
    unsigned short w2e;
    unsigned char unknown_30[4];
    unsigned short frame_34;
    unsigned char unknown_36[2];
    unsigned short w38;
    unsigned char unknown_3a[4];
    unsigned short w3e;
    unsigned char unknown_40[10];
    unsigned short w4a;
    unsigned char unknown_4c[4];
    unsigned long d50;
    unsigned long d54;
    char *p58;
    unsigned char unknown_5c[196];
    void *p120;
} TitleObj;

/* 0x134-byte record: 0x8a50/0x8ed0/0x92e0 record views, 0x8cd0 view, and the
 * 0xec-prefix view used by 0x90f0 (b94..de8). Field offsets are the union of all. */
typedef struct TitleRec {
    long d00;
    long d04;
    unsigned long d08;
    unsigned char unknown_0c[0x16];
    unsigned char b22;
    unsigned char b23;
    unsigned char unknown_24[0x0a];
    unsigned short w2e;
    unsigned char unknown_30[4];
    unsigned short field_34;
    unsigned char unknown_36[8];
    unsigned short w3e;
    unsigned char unknown_40[0x0a];
    unsigned short w4a;
    unsigned char unknown_4c[4];
    unsigned long d50;
    unsigned long d54;
    unsigned char unknown_58[0x0c];
    unsigned long d64;
    unsigned char unknown_68[0x0c];
    unsigned short w74;
    unsigned char unknown_76[0x1e];
    unsigned char b94, b95, b96, b97;
    unsigned short w98, w9a;
    unsigned long d9c, da0, da4, da8, dac, db0, db4, db8, dbc, dc0;
    unsigned long dc4, dc8, dcc, dd0, dd4, dd8, ddc, de0, de4, de8;
    unsigned char b0ec, b0ed;
    unsigned short w0ee;
    unsigned long d0f0, d0f4, d0f8, d0fc, d100, d104, d108, d10c, d110;
    short w114;
    unsigned short w116;
    unsigned long d118;
    short w11c;
    unsigned short w11e;
    unsigned long d120;
    unsigned short w124, w126;
    unsigned long d128, d12c, d130;
} TitleRec;

/* ---- globals ---- */

int g_2a008;
int g_2a010;
int g_2a024;
int g_2a02c;
int g_2a034;
int g_2a080;
int g_2a084;
TitleRec *g_2a048;
TitleRec *g_2a04c;
TitleRec *g_2a050;
TitleRec *g_2a054;
TitleRec *g_2a058;
TitleRec *g_2a05c;
TitleRec *g_2a060;
TitleRec *g_2a064;
TitleRec *g_2a06c;
TitleRec *g_2a070;

extern int g_2cc64;
extern int g_2cc60;
int g_2a044;
int g_2a03c;
int g_2a020;
int g_2a014;
int g_2a07c;
extern TitleObj *g_29e08;
extern short g_232b0[];

extern int g_2bffc;
extern int g_2bf7c;
extern TitleCfg *g_2bf40;
TitleTab g_2a0a0[2];

/* Initialized tables of this object (TITLE.DLL .data 0x23668..0x242e7), in address order: five
   0xffff-terminated frame tables and offset tables. Arrays of 8+ bytes are 8-aligned in .data, so
   0x241a4, 0x241e2 and 0x24202 are entries inside them (read as table[i + k]). */
short g_23668[292] = {
    1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 5, 4, 4, 6, 6, 4,
    4, 5, 5, 5, 4, 4, 6, 7, 8, 9, 9, 10, 10, 10, 9, 9,
    11, 11, 9, 10, 10, 10, 9, 9, 11, 11, 12, 12, 4, 4, 5, 5,
    5, 4, 4, 6, 6, 4, 4, 5, 5, 5, 4, 4, 6, 7, 8, 9,
    9, 10, 10, 10, 9, 9, 11, 11, 9, 9, 10, 10, 10, 9, 9, 11,
    12, 12, 4, 4, 5, 5, 5, 4, 4, 6, 6, 4, 4, 5, 5, 5,
    4, 4, 6, 7, 8, 9, 9, 10, 10, 10, 9, 9, 11, 11, 9, 9,
    13, 13, 13, 14, 14, 15, 16, 17, 18, 18, 19, 19, 19, 18, 18, 20,
    20, 18, 18, 19, 19, 19, 18, 18, 20, 21, 22, 23, 23, 24, 24, 24,
    23, 23, 25, 25, 23, 23, 24, 24, 24, 23, 23, 25, 26, 27, 18, 18,
    19, 19, 19, 18, 18, 20, 20, 18, 18, 19, 19, 19, 18, 18, 20, 21,
    22, 23, 23, 24, 24, 24, 23, 23, 25, 25, 23, 23, 24, 24, 24, 23,
    23, 25, 26, 27, 18, 18, 19, 19, 19, 18, 18, 20, 20, 18, 18, 19,
    19, 19, 18, 18, 20, 28, 29, 30, 30, 10, 10, 10, 9, 9, 11, 11,
    9, 9, 10, 10, 10, 9, 9, 11, 12, 12, 4, 4, 5, 5, 5, 4,
    4, 6, 6, 4, 4, 5, 5, 5, 4, 4, 6, 7, 8, 9, 9, 10,
    10, 10, 9, 9, 11, 11, 9, 9, 10, 10, 10, 9, 9, 11, 12, 12,
    4, 4, 5, 5, 5, 4, 4, 6, 6, 4, 4, 5, 5, 5, 4, 6,
    -1, 0, 0, 0,
};
short g_238b0[292] = {
    1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 5, 6, 6, 7, 7, 8,
    8, 9, 9, 9, 6, 6, 7, 10, 11, 12, 12, 13, 13, 13, 14, 14,
    15, 15, 16, 16, 13, 13, 13, 14, 14, 15, 17, 11, 8, 8, 9, 9,
    9, 6, 6, 7, 7, 8, 8, 9, 9, 9, 6, 6, 7, 10, 11, 12,
    12, 13, 13, 13, 14, 14, 15, 15, 16, 16, 13, 13, 13, 14, 14, 15,
    17, 11, 8, 8, 9, 9, 9, 6, 6, 7, 7, 8, 8, 9, 9, 9,
    6, 6, 7, 10, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    18, 18, 19, 19, 20, 20, 21, 22, 23, 23, 24, 24, 24, 25, 25, 26,
    26, 27, 27, 24, 24, 24, 25, 25, 26, 28, 29, 30, 31, 32, 32, 33,
    34, 35, 36, 37, 38, 38, 39, 39, 39, 40, 40, 41, 41, 42, 27, 27,
    24, 24, 24, 25, 25, 26, 26, 27, 27, 24, 24, 24, 25, 25, 26, 28,
    42, 38, 38, 39, 39, 39, 40, 40, 41, 41, 38, 38, 39, 39, 39, 40,
    40, 41, 41, 42, 27, 27, 24, 24, 24, 25, 25, 26, 26, 27, 27, 24,
    24, 24, 25, 43, 44, 45, 46, 47, 48, 13, 13, 13, 14, 14, 15, 15,
    16, 16, 13, 13, 13, 14, 14, 15, 17, 11, 8, 8, 9, 9, 9, 6,
    6, 7, 7, 8, 8, 9, 9, 9, 6, 6, 7, 10, 11, 12, 12, 13,
    13, 13, 14, 14, 15, 15, 16, 16, 13, 13, 13, 14, 14, 15, 11, 11,
    8, 8, 9, 9, 9, 6, 6, 7, 7, 8, 8, 9, 9, 9, 6, 6,
    -1, 0, 0, 0,
};
short g_23af8[292] = {
    1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 5, 4, 4, 6, 6, 4,
    4, 5, 5, 5, 4, 4, 6, 7, 8, 9, 9, 10, 10, 10, 9, 9,
    11, 11, 9, 9, 10, 10, 10, 9, 9, 11, 12, 13, 4, 4, 5, 5,
    5, 4, 4, 6, 6, 4, 4, 5, 5, 5, 4, 4, 6, 7, 8, 9,
    9, 10, 10, 10, 9, 9, 11, 11, 9, 9, 10, 10, 10, 9, 9, 11,
    12, 13, 4, 4, 5, 5, 5, 4, 4, 6, 6, 4, 4, 5, 5, 5,
    4, 4, 6, 7, 8, 9, 9, 10, 10, 10, 9, 9, 11, 11, 9, 9,
    14, 14, 15, 15, 16, 17, 18, 19, 19, 19, 20, 20, 20, 21, 21, 22,
    21, 21, 20, 20, 20, 21, 21, 22, 23, 24, 25, 26, 26, 27, 27, 27,
    26, 26, 28, 28, 26, 26, 27, 27, 27, 26, 26, 28, 25, 23, 22, 22,
    20, 20, 20, 21, 21, 22, 21, 21, 20, 20, 20, 21, 21, 22, 23, 24,
    25, 26, 26, 27, 27, 27, 26, 26, 28, 28, 26, 26, 27, 27, 27, 26,
    26, 28, 25, 24, 21, 21, 20, 20, 20, 21, 21, 23, 23, 21, 21, 20,
    20, 20, 21, 21, 22, 29, 30, 31, 31, 10, 10, 10, 9, 9, 11, 11,
    9, 9, 10, 10, 10, 9, 9, 11, 12, 13, 4, 4, 5, 5, 5, 4,
    4, 6, 6, 4, 4, 5, 5, 5, 4, 4, 6, 7, 8, 9, 9, 10,
    10, 10, 9, 9, 11, 11, 9, 9, 10, 10, 10, 9, 9, 11, 12, 13,
    4, 4, 5, 5, 5, 4, 4, 6, 6, 4, 4, 5, 5, 5, 4, 6,
    -1, 0, 0, 0,
};
short g_23d40[292] = {
    1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 5, 4, 4, 5, 5, 5,
    4, 4, 6, 6, 7, 7, 8, 7, 8, 9, 9, 10, 10, 10, 9, 9,
    11, 11, 9, 9, 10, 10, 10, 9, 9, 11, 12, 8, 4, 4, 5, 5,
    5, 4, 4, 6, 6, 4, 4, 5, 5, 5, 4, 4, 6, 7, 8, 9,
    9, 10, 10, 10, 9, 9, 11, 11, 9, 9, 10, 10, 10, 9, 9, 11,
    12, 8, 4, 4, 5, 5, 5, 4, 4, 6, 6, 4, 4, 5, 5, 5,
    5, 4, 4, 7, 8, 9, 9, 10, 10, 10, 9, 9, 11, 11, 9, 9,
    13, 13, 14, 14, 15, 16, 17, 17, 18, 18, 19, 19, 19, 20, 20, 21,
    21, 20, 20, 19, 19, 19, 20, 20, 21, 22, 23, 24, 24, 25, 25, 25,
    24, 24, 26, 26, 24, 24, 25, 25, 25, 24, 24, 26, 27, 23, 20, 20,
    19, 19, 19, 20, 20, 21, 21, 20, 20, 19, 19, 19, 20, 20, 21, 22,
    23, 24, 24, 25, 25, 25, 24, 24, 26, 26, 24, 24, 25, 25, 25, 24,
    24, 26, 27, 23, 20, 20, 19, 19, 19, 20, 20, 21, 21, 20, 20, 19,
    19, 19, 20, 20, 21, 22, 28, 29, 29, 30, 30, 30, 9, 9, 11, 11,
    9, 9, 10, 10, 10, 9, 9, 11, 12, 8, 4, 4, 5, 5, 5, 4,
    4, 6, 6, 4, 4, 5, 5, 5, 4, 4, 6, 7, 8, 9, 9, 10,
    10, 10, 9, 9, 11, 11, 9, 9, 10, 10, 10, 9, 9, 11, 12, 8,
    4, 4, 5, 5, 5, 4, 4, 6, 6, 4, 4, 5, 5, 5, 4, 4,
    -1, 0, 0, 0,
};
short g_23f88[292] = {
    1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 7, 8, 9, 9, 9, 11, 11, 11,
    9, 9, 9, 11, 11, 11, 11, 11, 11, 12, 13, 14, 15, 15, 15, 16,
    16, 16, 15, 15, 15, 16, 16, 16, 16, 16, 16, 5, 7, 17, 9, 9,
    9, 11, 11, 11, 9, 9, 9, 11, 11, 11, 11, 11, 11, 11, 12, 13,
    14, 15, 15, 15, 16, 16, 16, 15, 15, 15, 16, 16, 16, 16, 16, 16,
    16, 5, 7, 17, 9, 9, 9, 11, 11, 11, 9, 9, 9, 11, 11, 11,
    11, 11, 11, 18, 19, 20, 21, 21, 21, 22, 22, 22, 23, 23, 23, 22,
    22, 22, 23, 23, 23, 23, 23, 24, 25, 26, 27, 27, 27, 28, 28, 28,
    27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 29, 30, 31, 32, 32, 32,
    23, 23, 23, 32, 32, 32, 33, 33, 33, 33, 33, 33, 33, 24, 25, 26,
    27, 27, 27, 28, 28, 28, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28,
    29, 30, 31, 32, 32, 32, 23, 23, 23, 32, 32, 32, 23, 23, 23, 23,
    23, 23, 23, 34, 35, 36, 37, 37, 37, 11, 11, 11, 37, 37, 37, 11,
    11, 11, 11, 11, 11, 11, 12, 13, 14, 15, 15, 15, 16, 16, 16, 38,
    38, 38, 16, 16, 16, 16, 16, 16, 16, 5, 7, 17, 9, 9, 9, 11,
    11, 11, 9, 9, 9, 11, 11, 11, 11, 11, 11, 11, 39, 13, 1, 15,
    15, 16, 16, 16, 16, 16, 16, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    -1, 0, 0, 0,
};
short g_241d0[16] = {
    0, 2, 10, 8, 6, 4, 5, 10, 11, 10, 9, 6, 3, 5, 10, 11,
};
short g_241f0[32] = {
    0, 0, 1, 2, 2, 2, 4, 8, 16, 32, 40, 46, 42, 44, 46, 46,
    0, 3, 5, 3, 2, 1, 0, -1, -9, -20, -32, -38, -46, -50, -52, -53,
};
short g_24230[36] = {
    0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
    8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15,
    -1, 0, 0, 0,
};
short g_24278[32] = {
    0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
    8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, -1, 0,
};
short g_242b8[24] = {
    0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
    8, 8, 9, 9, 10, 10, -1, 0,
};
/* Also in this object's .bss block (0x29fec..0x2a0b0), used by the C unit at 0x6bb0. */
TitleRec *g_29ff4;
TitleRec *g_29ffc;
TitleRec *g_2a000;
TitleRec *g_2a004;
int g_2a018;
int g_2a078;
int g_2a088;
extern signed char g_2cc03;
extern char *g_2d320;
extern char *g_2df4c;
extern unsigned short g_2bf6c;
extern unsigned short g_2bff2;
extern unsigned short g_2bf38;
extern long *g_2bfa0[];
extern Rec8 *g_2bf48;
extern Rec8 *g_2bf5c;
extern TitleCell94 *g_2dfa0;
extern unsigned short g_2bf72;
extern unsigned short g_2bf28;
extern unsigned short g_2bf60;
extern unsigned short g_2bf80;
extern unsigned short g_2bf32;
extern unsigned short g_2bf64;
extern unsigned short g_2bf58;
extern unsigned short g_2bf70;
extern unsigned short g_2bf62;
extern unsigned short g_2bf30;
extern unsigned short g_2bf6e;
extern unsigned short g_2bff0;
extern unsigned short g_2bf44;

/* ---- callees ---- */

void title_054f0(int a, int b);
unsigned int title_0c4a0(int a);
TitleRec *title_08900(int flags);
void *title_08d80(unsigned long flags);
void *title_091a0(unsigned long flags);
void title_094c0(void *slot);
void title_01dd0(...);
void title_04610(TitleObj *p);
void unlink_12c(TitleObj *p);
void title_098f0(TitleObj *p);
void clear_command_high_bit(TitleObj *p);
void title_097f0(TitleObj *p);
void title_09830(TitleObj *p);
void title_09870(TitleObj *p);
void unlink_0e4(TitleObj *p);

void title_08c90(void);
void title_09090(void);
void title_09480(void);
void title_085b0(void);
void title_08cd0(void *pv);
void title_090f0(void *pv);
int title_08ae0(TitleObj *p);
int title_08f50(TitleObj *p);
int title_09350(TitleObj *p);

/* ---- main menu (0x7510) ---- */

/* Game-state view of g_engine_interface.context_004 (offsets observed in 0x7510). */
typedef struct TitleMenuState {
    unsigned char level_000;
    unsigned char unknown_001[6 - 1];
    unsigned char byte_006;
    unsigned char unknown_007[0xb - 7];
    unsigned char byte_00b;
    unsigned char byte_00c;
    unsigned char unknown_00d;
    unsigned char byte_00e;
    unsigned char unknown_00f[0x30 - 0xf];
    unsigned long flags_030;
    unsigned char unknown_034[0x9c - 0x34];
    short word_09c;
} TitleMenuState;

/* Sparkle positions on the menu picture (x, y pairs): .data 0x242e8. */
int g_242e8[50] = {
    125, 27, 144, 41, 153, 38, 159, 41, 167, 39, 182, 35, 178, 40, 187, 38,
    91, 49, 105, 64, 133, 67, 122, 79, 153, 59, 155, 79, 182, 71, 196, 69,
    202, 80, 211, 63, 220, 79, 233, 60, 234, 80, 71, 78, 84, 64, 74, 57,
    234, 78,
};

unsigned long g_29fec;     /* buttons (cursor_5e) */
int g_29ff0;               /* chosen menu entry */
int g_29ff8;
int g_2a01c;               /* fade level */
unsigned long g_2a028;     /* directions (cursor_5c) */
unsigned int g_2a030;      /* highlighted entry */
int g_2a038;               /* idle ticks */
int g_2a040;               /* key repeat delay */
int g_2a068;
unsigned int g_2a074;      /* last entry */
int g_2a08c;               /* menu already shown */

extern int g_29f98;
extern int g_2bb30;
extern int g_2bb34;
extern char g_29128[];

void title_0c990(int a, int b);
void title_0c940(int a);
void title_0c9c0(int a, int b);
void title_197f0(void);
void title_0c560(int a, int b);
void title_1d790(int a);
void title_0c450(int *out, int size, int flags);
int title_0c3f0(char *a, char *name, int c, int d);
void title_0c8b0(int a0, int a1, int a2, int a3, int a4, int a5, int a6);
void title_0c6d0(int a);
void title_04b50(int a);
void title_04b70(int a, int b);
void title_06bb0(void);
void title_06fb0(void);
void title_071e0(int a);
void title_0c5d0(int a);
void title_0c3c0(void);
void title_01de0(int a0, int a1, int a2, int a3, int a4);
int title_04d80(int idx, int key);
int title_0c910(int a);
int title_0c9b0(int a);
void title_08320(void);
void title_08140(void);
void title_199b0(int limit);
void title_19a40(int a1, int a2, int a3, int a4, int a5, int a6, int a7);
void title_028c0(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10);
int title_0c950(void);
void title_19820(void);
void title_0c4f0(int a);
void title_0c2d0(void);
void title_0c330(int p);
void title_04ea0(int a);
void title_05db0(void (*fn)(TitleProc *), int a, int b, int c);
void title_05e90(void (*fn)(TitleProc *), int a);
void title_05f10(TitleProc *self);
void title_0f460(TitleProc *self);
void title_0a450(TitleProc *self);
void title_016f0(TitleProc *self);
void title_164b0(TitleProc *self);

/* Per-type handler tables (one row per object type; 0x17ad0 calls fn_00 after allocation). */
typedef struct TitleDisp8 {
    void (*fn_00)(void *p);
    void (*fn_04)(void *p);
} TitleDisp8;

typedef struct TitleDisp12 {
    void (*fn_00)(void *p);
    void (*fn_04)(void *p);
    void (*fn_08)(void *p);
} TitleDisp12;

extern TitleDisp12 *g_2bf2c;
extern TitleDisp8 *g_2bf34;
extern TitleDisp8 *g_2bf54;
extern long g_2c0a0[];
extern unsigned char g_2c080[];
extern int g_2bff4;
extern int g_2bff8;

/*@FUNCS@*/

/* ---- title object set (0x6bb0..0x7510) ---- */

TitleRec *title_17ad0(int a0, int a1, int a2, int size, int a4);
void title_0c2a0(char *s, char *name, int b, int c, int d);
void title_02090(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);

/*@BEGIN_FUNC 0x6bb0 _title_06bb0*/
void title_06bb0(void)
{
    g_2a004 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a004->d54 |= 5;
    g_29ff4 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_29ff4->d54 |= 6;
    g_2a000 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a000->d54 |= 5;
    g_29ffc = title_17ad0(0, 0, 0, 0x2002, 0);
    g_29ffc->d54 |= 6;
    g_2a000->field_34 = 1;
    g_29ffc->field_34 = 1;
    g_29e08 = (TitleObj *)title_17ad0(0, 0, 0, 0x2002, 0);
    g_29e08->frame_34 = 0x16;
    g_29e08->x_00 = 0xfea20000;
    g_29e08->y_04 = 0x500000;
    g_2cc64 = -1;

    g_2a050 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a050->field_34 = 0x3d;
    g_2a054 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a054->field_34 = 0x5c;
    g_2a048 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a048->field_34 = 0x7d;
    g_2a04c = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a04c->field_34 = 0xb0;
    g_2a058 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a058->field_34 = 0xcf;
    g_2a064 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a064->field_34 = 0x3d;
    g_2a05c = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a05c->field_34 = 0x5c;
    g_2a060 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a060->field_34 = 0x7d;
    g_2a06c = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a06c->field_34 = 0xb0;
    g_2a070 = title_17ad0(0, 0, 0, 0x2002, 0);
    g_2a070->field_34 = 0xcf;

    g_29e08->b23 = 6;
    g_29e08->w3e = 0x1e;
    g_2a048->b23 = 6;
    g_2a048->w3e = 0x18;
    g_2a050->b23 = 6;
    g_2a050->w3e = 0x17;
    g_2a058->b23 = 6;
    g_2a058->w3e = 0x14;
    g_2a054->b23 = 6;
    g_2a054->w3e = 0x15;
    g_2a04c->b23 = 6;
    g_2a04c->w3e = 0x16;
    g_2a064->b23 = 6;
    g_2a064->w3e = 0x1e;
    g_2a05c->b23 = 6;
    g_2a05c->w3e = 0x1e;
    g_2a060->b23 = 6;
    g_2a060->w3e = 0x1e;
    g_2a06c->b23 = 6;
    g_2a06c->w3e = 0x1e;
    g_2a070->b23 = 6;
    g_2a070->w3e = 0x1e;

    g_2a064->d54 |= 4;
    g_2a05c->d54 |= 4;
    g_2a060->d54 |= 4;
    g_2a06c->d54 |= 4;
    g_2a070->d54 |= 4;

    g_2a060->w74 = 0x80;
    g_2a064->w74 = 0x70;
    g_2a070->w74 = 0xff80;
    g_2a05c->w74 = 0xff90;
    g_2a06c->w74 = 0xffa0;

    g_2a070->w4a = 0;
    g_2a06c->w4a = 0;
    g_2a060->w4a = 0;
    g_2a05c->w4a = 0;
    g_2a064->w4a = 0;

    g_2a004->b23 = 6;
    g_2a004->w3e = 10;
    g_29ff4->b23 = 6;
    g_29ff4->w3e = 0xf;
    g_2a000->b23 = 6;
    g_2a000->w3e = 10;
    g_29ffc->b23 = 6;
    g_29ffc->w3e = 0xf;

    g_2a010 = 0;
    g_2a008 = 0;
    g_2a024 = 0;
    g_2a02c = 0;
    g_2a034 = 0;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x6fb0 _title_06fb0*/
void title_06fb0(void)
{
    g_2a048->d00 = (g_2a044 - 0x7d) << 16;
    g_2a048->d04 = (g_2a03c + 0x55) << 16;
    g_2a050->d00 = (g_2a044 - 0x50) << 16;
    g_2a050->d04 = (g_2a03c + 0x69) << 16;
    g_2a058->d00 = (g_2a044 + 0x50) << 16;
    g_2a058->d04 = (g_2a03c + 0x69) << 16;
    g_2a054->d00 = (g_2a044 + 0x6e) << 16;
    g_2a054->d04 = (g_2a03c + 0x5f) << 16;
    g_2a04c->d00 = (g_2a044 + 0x87) << 16;
    g_2a04c->d04 = (g_2a03c + 0x4b) << 16;
    g_2a060->d00 = (g_2a044 - 0x7d) << 16;
    g_2a060->d04 = (g_2a03c + 0x49) << 16;
    g_2a064->d00 = (g_2a044 - 0x50) << 16;
    g_2a064->d04 = (g_2a03c + 0x5d) << 16;
    g_2a070->d00 = (g_2a044 + 0x50) << 16;
    g_2a070->d04 = (g_2a03c + 0x5d) << 16;
    g_2a05c->d00 = (g_2a044 + 0x6e) << 16;
    g_2a05c->d04 = (g_2a03c + 0x53) << 16;
    g_2a06c->d00 = (g_2a044 + 0x87) << 16;
    g_2a06c->d04 = (g_2a03c + 0x3f) << 16;
    g_2a000->d00 = g_2a044 << 16;
    g_2a000->d04 = g_2a03c << 16;
    g_29ffc->d00 = (g_2a044 + 2) << 16;
    g_29ffc->d04 = (g_2a03c + 2) << 16;
    g_2a004->d00 = g_2a044 << 16;
    g_2a004->d04 = g_2a03c << 16;
    g_29ff4->d00 = (g_2a044 + 2) << 16;
    g_29ff4->d04 = (g_2a03c + 2) << 16;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x71e0 _title_071e0*/
void title_071e0(int a1)
{
    char *tbl[7];
    int x;
    int y;
    int s;

    x = (a1 * g_2a018) >> 7;
    y = ((0x80 - g_2a018) * a1) >> 7;
    tbl[0] = g_sequence_files[TITLE_SEQ_T015];
    tbl[1] = g_sequence_files[TITLE_SEQ_T013];
    tbl[2] = g_sequence_files[TITLE_SEQ_T003];
    tbl[3] = g_sequence_files[TITLE_SEQ_T001];
    tbl[4] = g_sequence_files[TITLE_SEQ_T012];
    tbl[5] = g_sequence_files[TITLE_SEQ_T010];
    tbl[6] = g_sequence_files[TITLE_SEQ_T004];

    switch (g_2a080) {
    case 0:
        title_0c2a0(g_29128, tbl[g_2a088], 0, g_2a078, 0x14312);
        g_2a088 += 1;
        if (g_2a088 == 6) {
            g_2a088 = 0;
        }
        g_2a080 = 1;
        /* fall through */
    case 1:
        if (*(short *)((char *)g_engine_interface.context_004 + 0x9c) == 0) {
            title_01dd0(g_2a078);
            title_0c8b0(g_2a078 + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
            g_2a080 = 2;
        }
        /* fall through */
    case 2:
        title_02090(0x140, 0, x, 0, g_2a044, g_2a03c, 0, 1);
        return;
    case 3:
        title_02090(0x140, 0, x, 0, g_2a044, g_2a03c, 0, 1);
        title_02090(0x280, 0x100, y, 1, g_2a044, g_2a03c, 1, 2);
        if (g_2a018 > 0) {
            g_2a018 -= 0x10;
            return;
        }
        g_2a018 = 0;
        g_2a080 = 4;
        return;
    case 4:
        title_0c2a0(g_29128, tbl[g_2a088], 0, g_2a078, 0x14312);
        g_2a088 += 1;
        if (g_2a088 == 6) {
            g_2a088 = 0;
        }
        g_2a080 = 5;
        /* fall through */
    case 5:
        if (*(short *)((char *)g_engine_interface.context_004 + 0x9c) == 0) {
            title_01dd0(g_2a078);
            title_0c8b0(g_2a078 + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
            g_2a080 = 6;
        }
        /* fall through */
    case 6:
        title_02090(0x280, 0x100, y, 0, g_2a044, g_2a03c, 1, 1);
        return;
    case 7:
        title_02090(0x140, 0, x, 0, g_2a044, g_2a03c, 0, 1);
        title_02090(0x280, 0x100, y, 1, g_2a044, g_2a03c, 1, 2);
        s = g_2a018;
        if (s < 0x80) {
            g_2a018 = s + 0x10;
            return;
        }
        g_2a018 = 0x80;
        g_2a080 = 0;
        return;
    default:
        return;
    }
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x7510 _title_07510*/
/* TITLE.DLL 0x7510 (3120 bytes): main menu process (TitleProc callback). Started by the title
   controller 0x164b0 and by 0x16f0 with title_05db0(title_07510, 0, 1, 0). Phases: 1 load menu
   pictures, 10 wait for the menu1 slot (first entry: Hercules walk-in), 2 menu loop (cursor
   0x2a028/buttons 0x29fec, highlighted entry 0x2a030, idle timeout 900 ticks), 4/5 fade out,
   6 release objects, 7 dispatch the chosen entry (0x29ff0), 0x14 load-game check,
   0xffff init, 0xfffe hand back to 0x164b0.
   Unit/language: its .data table 0x242e8 follows the u08140.cpp tables (0x23668..0x242e7) in the
   same object, so 0x7510 belongs to the C++ object-system unit (unit start 0x7510, not 0x8140).
   Lever: case 7's entry 2 breaks to the shared tail (compiler duplicates it); every other tail
   is explicit. */
void title_07510(TitleProc *self)
{
    int pick;
    int kind;
    int code;

    g_2a028 = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5c;
    g_29fec = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5e;
    switch (self->state_04) {
    case 1:
        title_0c990(-4, 0);
        title_0c940(1);
        title_0c9c0(0, 0x4000);
        title_197f0();
        g_2a080 = 0;
        if (g_2bb34 != 0) {
            self->state_04 = 0xfffe;
            self->delay_08 = 1;
            return;
        }
        title_0c560(0, 4);
        title_1d790(1);
        g_2a078 = 0;
        title_0c450(&g_2a078, 0x15000, 0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_T015], g_2a078, 0x14312);
        title_01dd0(g_2a078);
        title_0c8b0(g_2a078 + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_screen_files[11], g_2a078, 0x14312);
        title_0c8b0(g_2a078 + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0();
        g_2a020 = 0;
        g_2a01c = 0;
        g_2a018 = 0x80;
        if (g_2a08c == 0) {
            title_04b50(1);
            g_2a044 = -0x204;
            g_2a03c = 0;
            g_2a07c = 0;
            g_2a01c = 0x80;
            title_0c5d0(4);
            title_0c3c0();
            self->state_04 = 10;
            self->delay_08 = 1;
            return;
        }
        title_04b70(1, 0);
        title_06bb0();
        title_06fb0();
        g_2a01c = 0x80;
        g_2a03c = 0;
        g_2a044 = 0;
        g_2a07c = 2;
        title_0c5d0(4);
        title_0c3c0();
        self->state_04 = 2;
        self->delay_08 = 1;
        return;
    case 10:
        title_01de0(0x140, 0x100, g_2a01c, 0, 0);
        if (g_2a01c < 0x80) {
            g_2a01c += 0x10;
        }
        if (g_26110[1].state_10 != 3 && title_04d80(1, 0xf8) == 0) {
            self->state_04 = 10;
            self->delay_08 = 1;
            return;
        }
        title_06bb0();
        title_06fb0();
        g_2a07c = 1;
        g_2cc64 = 0;
        g_2cc60 = 0;
        self->state_04 = 2;
        self->delay_08 = 1;
        return;
    case 2:
        if (title_0c910(-1) != 0) {
            g_29ff0 = 3;
            self->state_04 = 4;
            self->delay_08 = 1;
            return;
        }
        if (g_29f98 == 0) {
            title_08320();
            title_08140();
            title_199b0(0xf8);
            if (title_0c4a0(100) < 7) {
                pick = title_0c4a0(0x18) * 2;
                kind = title_0c4a0(3);
                title_19a40(0xf8, 0x2002,
                            (g_242e8[pick] + g_2a044 - 0x98) << 16,
                            (g_242e8[pick + 1] + g_2a03c - 0x84) << 16,
                            0, kind, 0x80);
            }
            title_06fb0();
        }
        switch (g_2a07c) {
        case 0:
            title_01de0(0x140, 0x100, g_2a01c, 0, 0);
            if (g_2a01c < 0x80) {
                g_2a01c += 0x10;
            } else {
                g_2a07c = 1;
                g_2cc64 = 0;
                g_2cc60 = 0;
            }
            g_2a068 = 1;
            break;
        case 1:
            title_01de0(0x140, 0x100, 0x80, 0, 0);
            title_071e0(0x80);
            g_2a068 = 1;
            break;
        case 2:
            title_071e0(0x80);
            g_2a038++;
            g_2a068 = 0;
            if (g_2a038 == 900) {
                g_29ff0 = 5;
                g_2bb30 = 1;
                self->state_04 = 4;
                self->delay_08 = 1;
                return;
            }
            break;
        }
        if (g_2a020 > 0) {
            g_2a020 -= 8;
            title_028c0(0, 0, 0x140, 0x100, 0xff, 0xff, 0xff, g_2a020, 1, 0, 0x4fe);
        }
        if (g_2a028 == 0) {
            g_2a040 = 0;
        } else {
            g_2a038 = 0;
        }
        if (g_2a068 != 0) {
            if (g_2a07c != 2 && (g_29fec & 0x4008)) {
                g_2a01c = 0x80;
                g_2a03c = 0;
                g_2a044 = 0;
                g_2a07c = 2;
                g_2cc64 = 4;
                g_29e08->d54 &= 0x7fffffff;
                g_2a020 = 0x80;
                title_054f0(0x303, 0);
            }
            g_29fec = 0;
        } else if (g_2a040 == 0) {
            if ((g_2a028 & 0x10) && g_2a030 > 0) {
                title_054f0(0x301, 0);
                g_2a030--;
                if (g_2a030 == 2) {
                    g_2a030 = 1;
                }
                g_2a040 = 0xf;
            }
            g_2a074 = (((TitleMenuState *)g_engine_interface.context_004)->flags_030 & 0x80000000) ? 4 : 3;
            if ((g_2a028 & 0x40) && g_2a030 < g_2a074) {
                title_054f0(0x300, 0);
                g_2a030++;
                if (g_2a030 == 2) {
                    g_2a030 = 3;
                }
                g_2a040 = 0xf;
            }
        } else {
            g_2a040--;
        }
        switch (g_2a030) {
        case 0:
            g_2a004->field_34 = 2;
            g_29ff4->field_34 = 2;
            if (g_29fec & 0x4008) {
                title_054f0(0x302, 0);
                g_29ff0 = 1;
                self->state_04 = 4;
                self->delay_08 = 1;
                return;
            }
            break;
        case 1:
            g_2a004->field_34 = 3;
            g_29ff4->field_34 = 3;
            if (g_29fec & 0x4008) {
                title_054f0(0x302, 0);
                g_29ff0 = 0;
                self->state_04 = 4;
                self->delay_08 = 1;
                return;
            }
            break;
        case 3:
            g_2a004->field_34 = 4;
            g_29ff4->field_34 = 4;
            if (g_29fec & 0x4008) {
                title_054f0(0x302, 0);
                title_0c950();
            }
            break;
        case 4:
            g_2a004->field_34 = 5;
            g_29ff4->field_34 = 5;
            if (g_29fec & 0x4008) {
                title_054f0(0x302, 0);
                g_29ff0 = 4;
                self->state_04 = 4;
                self->delay_08 = 1;
                return;
            }
            break;
        }
        self->state_04 = 2;
        self->delay_08 = 1;
        return;
    case 4:
        title_19820();
        title_0c4f0(1);
        g_2a01c = 0x80;
        /* fall through */
    case 5:
        title_071e0(g_2a01c);
        if (g_29f98 == 0) {
            title_08140();
            g_2a058->w4a = g_2a01c;
            g_2a04c->w4a = g_2a01c;
            g_2a048->w4a = g_2a01c;
            g_2a054->w4a = g_2a01c;
            g_2a050->w4a = g_2a01c;
            g_29e08->w4a = g_2a01c;
            g_2a004->w4a = g_2a01c;
            g_29ff4->w4a = g_2a01c * 2;
            g_2a000->w4a = g_2a01c;
            g_29ffc->w4a = g_2a01c * 2;
            if (g_2a01c > 0) {
                g_2a01c -= 8;
            } else {
                self->state_04 = 6;
                self->delay_08 = 1;
                return;
            }
        }
        self->state_04 = 5;
        self->delay_08 = 1;
        return;
    case 6:
        if (g_2a050) {
            title_09350((TitleObj *)g_2a050);
            g_2a050 = 0;
        }
        if (g_2a054) {
            title_09350((TitleObj *)g_2a054);
            g_2a054 = 0;
        }
        if (g_2a048) {
            title_09350((TitleObj *)g_2a048);
            g_2a048 = 0;
        }
        if (g_2a04c) {
            title_09350((TitleObj *)g_2a04c);
            g_2a04c = 0;
        }
        if (g_2a058) {
            title_09350((TitleObj *)g_2a058);
            g_2a058 = 0;
        }
        if (g_2a064) {
            title_09350((TitleObj *)g_2a064);
            g_2a064 = 0;
        }
        if (g_2a05c) {
            title_09350((TitleObj *)g_2a05c);
            g_2a05c = 0;
        }
        if (g_2a060) {
            title_09350((TitleObj *)g_2a060);
            g_2a060 = 0;
        }
        if (g_2a06c) {
            title_09350((TitleObj *)g_2a06c);
            g_2a06c = 0;
        }
        if (g_2a070) {
            title_09350((TitleObj *)g_2a070);
            g_2a070 = 0;
        }
        if (g_29e08) {
            title_09350(g_29e08);
            g_29e08 = 0;
        }
        if (g_2a004) {
            title_09350((TitleObj *)g_2a004);
            g_2a004 = 0;
        }
        if (g_29ff4) {
            title_09350((TitleObj *)g_29ff4);
            g_29ff4 = 0;
        }
        if (g_2a000) {
            title_09350((TitleObj *)g_2a000);
            g_2a000 = 0;
        }
        if (g_29ffc) {
            title_09350((TitleObj *)g_29ffc);
            g_29ffc = 0;
        }
        if (((TitleMenuState *)g_engine_interface.context_004)->word_09c == 1) {
            title_0c2d0();
        }
        title_0c330(g_2a078);
        title_04ea0(1);
        self->state_04 = 7;
        self->delay_08 = 1;
        return;
    case 7:
        if (g_29ff0 >= 0 && g_29ff0 <= 5) {
            title_0c940(0);
            code = title_0c910(-2);
            ((TitleMenuState *)g_engine_interface.context_004)->byte_00e = code >> 8;
            ((TitleMenuState *)g_engine_interface.context_004)->byte_00c = code;
            ((TitleMenuState *)g_engine_interface.context_004)->byte_00b = ((TitleMenuState *)g_engine_interface.context_004)->byte_00c;
            title_0c9c0(0, 0);
        }
        switch (g_29ff0) {
        case 0:
            title_05db0(title_0f460, 0, 1, 0);
            break;
        case 1:
            ((TitleMenuState *)g_engine_interface.context_004)->level_000 = 0;
            self->state_04 = 0xfffe;
            self->delay_08 = 1;
            return;
        case 2:
            title_05db0(title_0a450, 0, 1, 0);
            break;
        case 4:
            title_05db0(title_016f0, 0, 1, 0);
            ((TitleMenuState *)g_engine_interface.context_004)->level_000--;
            self->state_04 = 0xfffe;
            self->delay_08 = 0;
            return;
        case 3:
            code = title_0c910(-1);
            if (code != 0) {
                code = title_0c910(-2);
            }
            if (code == 0) {
                code = title_0c9b0(-1);
            }
            ((TitleMenuState *)g_engine_interface.context_004)->level_000 = code >> 16;
            ((TitleMenuState *)g_engine_interface.context_004)->byte_00e = code >> 8;
            ((TitleMenuState *)g_engine_interface.context_004)->byte_00c = code;
            ((TitleMenuState *)g_engine_interface.context_004)->byte_00b = ((TitleMenuState *)g_engine_interface.context_004)->byte_00c;
            self->state_04 = 0x14;
            self->delay_08 = 1;
            return;
        case 5:
            if (((TitleMenuState *)g_engine_interface.context_004)->byte_006 == 0) {
                title_01dd0();
            }
            self->state_04 = 0xfffe;
            self->delay_08 = 1;
            return;
        }
        g_2a08c = 1;
        self->state_04 = 1;
        self->delay_08 = 0;
        return;
    case 0x14:
        if (((TitleMenuState *)g_engine_interface.context_004)->level_000 == 0) {
            g_2a08c = 1;
            self->state_04 = 1;
            self->delay_08 = 1;
            return;
        }
        self->state_04 = 0xfffe;
        self->delay_08 = 1;
        return;
    case 0xffff:
        g_2a014 = 1;
        g_2a030 = 0;
        g_2a040 = 0;
        g_2a038 = 0;
        title_1d790(1);
        g_2bb30 = 0;
        g_2bb34 = 0;
        g_29ff8 = 0;
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 0xfffe:
        title_0c940(0);
        title_0c910(0);
        title_0c9c0(0, 0);
        title_05e90(title_164b0, 0);
        title_05f10(self);
        return;
    }
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8140 _title_08140*/
void title_08140(void)
{
    int state;
    short neg = -1;

    g_2a050->field_34 = g_23af8[g_2a034] + 0x3c;
    g_2a054->field_34 = g_23668[g_2a02c] + 0x5b;
    g_2a048->field_34 = g_238b0[g_2a024] + 0x7c;
    g_2a04c->field_34 = g_23d40[g_2a008] + 0xaf;
    g_2a058->field_34 = g_23f88[g_2a010] + 0xce;
    g_2a034++;
    g_2a02c++;
    g_2a024++;
    g_2a008++;
    g_2a010++;
    if (++g_2a084 == 0x3c) {
        state = g_2a080;
        g_2a084 = 0;
        if (state == 2) {
            state = 3;
            g_2a080 = state;
        }
        if (state == 6) g_2a080 = 7;
    }
    if ((short)g_23af8[g_2a034] == neg) g_2a034 = 0x17;
    if ((short)g_23668[g_2a02c] == neg) g_2a02c = 0x17;
    if ((short)g_238b0[g_2a024] == neg) g_2a024 = 0x17;
    if ((short)g_23d40[g_2a008] == neg) g_2a008 = 0x17;
    if ((short)g_23f88[g_2a010] == neg) g_2a010 = 0x17;
    g_2a064->field_34 = g_2a050->field_34;
    g_2a05c->field_34 = g_2a054->field_34;
    g_2a060->field_34 = g_2a048->field_34;
    g_2a06c->field_34 = g_2a04c->field_34;
    g_2a070->field_34 = g_2a058->field_34;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8320 _title_08320*/
void title_08320(void)
{
    switch (g_2cc64) {
    case 0:
        g_29e08->frame_34 = g_24230[g_2cc60] + 0x16;
        g_2cc60++;
        if (g_24230[g_2cc60] == -1) g_2cc60 = 4;
        g_29e08->x_00 += 0x30000;
        g_2a044 = g_29e08->x_00 >> 16;
        g_2a03c = g_23f88[g_29e08->frame_34 + 270] + (g_29e08->y_04 >> 16) - 0xa9;
        if (g_29e08->x_00 > (int)0xffd00000) {
            g_29e08->frame_34 = 7;
            g_2cc64 = 1;
            g_2cc60 = 0;
        }
        return;
    case 1:
        g_29e08->frame_34 = g_24278[g_2cc60] + 7;
        g_2cc60++;
        if (g_24278[g_2cc60] == -1) {
            g_2cc64 = 2;
            g_2cc60 = 0;
            g_29e08->frame_34 = 0x26;
            g_2a03c += 0x14;
            g_2a020 = 0x80;
            return;
        }
        g_2a044 = g_241d0[g_29e08->frame_34 + 9] + (g_29e08->x_00 >> 16);
        g_2a03c = g_241f0[g_29e08->frame_34 + 9] + (g_29e08->y_04 >> 16) - 0xa9;
        return;
    case 2:
        g_29e08->frame_34 = g_242b8[g_2cc60] + 0x26;
        g_2cc60++;
        if (g_242b8[g_2cc60] == -1) {
            g_2cc64 = 3;
            g_2cc60 = 0;
        }
        break;
    case 3:
        g_29e08->frame_34 = g_232b0[g_2cc60] + 0x26;
        g_2cc60++;
        if (g_232b0[g_2cc60] == -1) g_2cc60 = 0;
        g_29e08->y_04 += 0x60000;
        if (g_29e08->y_04 > 0x1900000) g_2cc64 = 4;
        break;
    default:
        return;
    }
    g_2a044 = 0;
    if (g_2a03c < 0) {
        g_2a03c += 0x14;
        return;
    }
    if (g_2a014 != 0) {
        title_054f0(0x326, 0);
        g_2a014 = 0;
    }
    if (g_29e08->frame_34 < 0x2e) {
        g_2a03c = -title_0c4a0(0x10);
    } else {
        g_2a03c = 0;
        g_2a07c = 2;
    }
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x85b0 _title_085b0*/
void title_085b0(void)
{
    g_2bffc = 0;
    g_2bf7c = 0;
    title_08c90();
    title_09090();
    title_09480();
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x85d0 _title_085d0*/
/* TITLE.DLL 0x85d0: per-frame update pass. Clears g_2bff4/g_2bff8, snapshots the load state of
   the 18 resource slots into g_2c080, then walks the low slot ranges of the 0x94, 0xec and 0x134
   pools: a live record whose resource slot (+0x22) is in state 4 is released (0x9350, 0x8f50,
   0x8ae0); otherwise flag 1 at +0x50 calls its type handler fn_04. */
void title_085d0(void)
{
    int i;
    unsigned short t;
    TitleRec *recc;
    TitleRecEC *recb;
    TitleCell94 *reca;

    g_2bff4 = 0;
    g_2bff8 = 0;
    for (i = 0; i < 18; i++)
        g_2c080[i] = (unsigned char)g_26110[i].state_10;
    for (i = g_2bf44; i < g_2bf6e; i++) {
        reca = &g_2dfa0[i];
        t = reca->w2e;
        if (t != 0) {
            if (g_2c080[reca->b22] == 4)
                title_09350((TitleObj *)reca);
            else if (reca->d50 & 1)
                g_2bf54[t & 0xfff].fn_04(reca);
        }
    }
    for (i = g_2bf62; i < g_2bf58; i++) {
        recb = &((TitleRecEC *)g_2df4c)[i];
        t = recb->w2e;
        if (t != 0) {
            if (g_2c080[recb->b22] == 4)
                title_08f50((TitleObj *)recb);
            else if (recb->d50 & 1)
                g_2bf34[t & 0xfff].fn_04(recb);
        }
    }
    for (i = g_2bf32; i < g_2bf60; i++) {
        recc = &((TitleRec *)g_2d320)[i];
        t = recc->w2e;
        if (t != 0) {
            if (g_2c080[recc->b22] == 4)
                title_08ae0((TitleObj *)recc);
            else if (recc->d50 & 1)
                g_2bf2c[t].fn_04(recc);
        }
    }
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8770 _title_08770*/
/* TITLE.DLL 0x8770: second per-frame pass over the low (non-0x8000) slot ranges of the three
   pools. Records with flag 2 at +0x50 get their type handler (0x134: g_2bf2c[t].fn_08,
   0xec/0x94: fn_04); 0x94 records with flag 0x10000 are queued in g_2c0a0 (count in [0]) and
   replayed after the pools. Lever: the replay loop increments k before reading g_2c0a0[k]. */
void title_08770(void)
{
    int i;
    int k;
    unsigned short t;
    TitleRec *recc;
    TitleRecEC *recb;
    TitleCell94 *reca;

    g_2c0a0[0] = 0;
    for (i = g_2bf32; i < g_2bf60; i++) {
        recc = &((TitleRec *)g_2d320)[i];
        t = recc->w2e;
        if (t != 0) {
            if (recc->d50 & 2)
                g_2bf2c[t].fn_08(recc);
        }
    }
    for (i = g_2bf62; i < g_2bf58; i++) {
        recb = &((TitleRecEC *)g_2df4c)[i];
        t = recb->w2e;
        if (t != 0) {
            if (recb->d50 & 2)
                g_2bf34[t & 0xfff].fn_04(recb);
        }
    }
    for (i = g_2bf44; i < g_2bf6e; i++) {
        reca = &g_2dfa0[i];
        t = reca->w2e;
        if (t != 0) {
            if (reca->d50 & 0x10000) {
                g_2c0a0[0]++;
                g_2c0a0[g_2c0a0[0]] = (long)reca;
            }
            if (reca->d50 & 2)
                g_2bf54[t & 0xfff].fn_04(reca);
        }
    }
    for (k = 0; k < g_2c0a0[0]; ) {
        k++;
        reca = (TitleCell94 *)g_2c0a0[k];
        g_2bf54[reca->w2e & 0xfff].fn_04(reca);
    }
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8900 _title_08900*/
TitleRec *title_08900(int flags)
{
    int i;

    if (flags & 0x8000) {
        for (i = g_2bf28; i < g_2bf6c; i++) {
            if (((TitleRec *)g_2d320)[i].w2e == 0)
                break;
        }
        if (i < g_2bf6c) {
            g_2bf28 = i;
            return &((TitleRec *)g_2d320)[i];
        }
        for (i = g_2bf60; i < g_2bf28; i++) {
            if (((TitleRec *)g_2d320)[i].w2e == 0)
                break;
        }
        if (i < g_2bf28) {
            g_2bf28 = i;
            return &((TitleRec *)g_2d320)[i];
        }
        return 0;
    }
    for (i = g_2bf80; i < g_2bf60; i++) {
        if (((TitleRec *)g_2d320)[i].w2e == 0)
            break;
    }
    if (i < g_2bf60) {
        g_2bf80 = i;
        return &((TitleRec *)g_2d320)[i];
    }
    for (i = g_2bf32; i < g_2bf80; i++) {
        if (((TitleRec *)g_2d320)[i].w2e == 0)
            break;
    }
    if (i < g_2bf80) {
        g_2bf80 = i;
        return &((TitleRec *)g_2d320)[i];
    }
    return 0;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8a50 _title_08a50*/
TitleRec *title_08a50(int a1, int a2, int a3, int a4)
{
    TitleRec *rec;

    rec = title_08900(a4);
    if (rec != 0) {
        title_08cd0(rec);
        rec->w2e = (unsigned short)a4;
        rec->d00 = a1;
        rec->d9c = a1;
        rec->d04 = a2;
        rec->da0 = a2;
        rec->d08 = a3;
        rec->da4 = a3;
        rec->b22 = g_2bf40[a4 & 0xffff].d_0c;
        rec->d64 = g_2bf40[a4 & 0xffff].a_00;
        rec->d110 = g_2bf40[a4 & 0xffff].b_04;
        rec->d118 = g_2bf40[a4 & 0xffff].c_08;
    }
    return rec;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8ae0 _title_08ae0*/
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
        unlink_12c(p);
        p->w2e = 0;
        title_04610(p);
        return 1;
    }
    if (p->p120)
        title_01dd0(p->p120);
    frame = p->p58;
    if (p->p58 == 0) {
        clear_command_high_bit(p);
        title_097f0(p);
        title_09830(p);
        title_09870(p);
        unlink_12c(p);
        unlink_0e4(p);
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
            *(unsigned long *)(frame + 0) = p->x_00;
            *(unsigned long *)(frame + 4) = p->y_04;
            *(unsigned long *)(frame + 8) = p->dw08;
        }
    }
    if (*(unsigned char *)(frame + 0xc) & 0x80) {
        if (p->b1c != 0xff)
            p->w2a |= (unsigned short)(p->b1c << 8);
        if ((short)p->w38 == (short)g_2cc03)
            *(unsigned short *)(frame + 0xe) = p->w2a;
    }
    clear_command_high_bit(p);
    title_09830(p);
    title_09870(p);
    title_097f0(p);
    unlink_12c(p);
    unlink_0e4(p);
    p->w2e = 0;
    *(unsigned short *)(frame + 0xc) &= 0x7fff;
    title_04610(p);
    return 1;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8c90 _title_08c90*/
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
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8cd0 _title_08cd0*/
void title_08cd0(void *pv)
{
    TitleRec *p;
    unsigned long z;
    long m;

    p = (TitleRec *)pv;

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
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8d80 _title_08d80*/
void *title_08d80(unsigned long flags)
{
    int i;

    if (flags & 0x8000) {
        for (i = g_2bf64; i < g_2bff2; i++) {
            if (((TitleRecEC *)g_2df4c)[i].w2e == 0)
                break;
        }
        if (i < g_2bff2) {
            g_2bf64 = i;
            return &((TitleRecEC *)g_2df4c)[i];
        }
        for (i = g_2bf58; i < g_2bf64; i++) {
            if (((TitleRecEC *)g_2df4c)[i].w2e == 0)
                break;
        }
        if (i < g_2bf64) {
            g_2bf64 = i;
            return &((TitleRecEC *)g_2df4c)[i];
        }
        return 0;
    }
    for (i = g_2bf70; i < g_2bf58; i++) {
        if (((TitleRecEC *)g_2df4c)[i].w2e == 0)
            break;
    }
    if (i < g_2bf58) {
        g_2bf70 = i;
        return &((TitleRecEC *)g_2df4c)[i];
    }
    for (i = g_2bf62; i < g_2bf70; i++) {
        if (((TitleRecEC *)g_2df4c)[i].w2e == 0)
            break;
    }
    if (i < g_2bf70) {
        g_2bf70 = i;
        return &((TitleRecEC *)g_2df4c)[i];
    }
    return 0;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8ed0 _title_08ed0*/
TitleRec *title_08ed0(unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4)
{
    TitleRec *p = (TitleRec *)title_08d80(a4);
    if (p != 0) {
        title_090f0(p);
        p->w2e = (unsigned short)(a4 | 0x4000);
        p->d00 = a1;
        p->d9c = a1;
        p->d04 = a2;
        p->da0 = a2;
        p->d08 = a3;
        p->da4 = a3;
        p->b22 = g_2bf48[a4 & 0xfff].b6;
        p->d64 = g_2bf48[a4 & 0xfff].d0;
    }
    return p;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x8f50 _title_08f50*/
int title_08f50(TitleObj *p)
{
    char *edi;
    unsigned short ax;
    unsigned short cx;

    if (p->w2e == 0)
        return 1;
    if (p->w2e & 0x4000) {
        if (p->d50 & 0x10000000) {
            p->w2e = 0;
            title_04610(p);
            return 1;
        }
        edi = p->p58;
        if (p->p58 == 0) {
            title_09830(p);
            title_097f0(p);
            title_09870(p);
            unlink_0e4(p);
            p->w2e = (unsigned short)(unsigned long)edi;
            title_04610(p);
            return 1;
        }
        ax = p->w38;
        cx = *(unsigned short *)(edi + g_2a0a0[ax].a + 0xc);
        edi += g_2a0a0[ax].a;
        if (cx & 0x2000)
            return 0;
        if (cx & 0x100) {
            if ((short)ax == (short)g_2cc03) {
                *(unsigned long *)(edi + 0) = p->x_00;
                *(unsigned long *)(edi + 4) = p->y_04;
                *(unsigned long *)(edi + 8) = p->dw08;
                *(unsigned short *)(edi + 0xe) = p->w2a;
            }
        }
        if (*(unsigned char *)&cx & 0x80) {
            if ((short)p->w38 == (short)g_2cc03) {
                cx = p->w2a;
                *(unsigned short *)(edi + 0xe) = cx;
            }
        }
        title_09830(p);
        title_097f0(p);
        title_09870(p);
        unlink_0e4(p);
        p->w2e = 0;
        *(unsigned short *)(edi + 0xc) &= 0x7fff;
        title_04610(p);
        return 1;
    }
    return title_09350(p);
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x9090 _title_09090*/
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
/*@END_FUNC*/

/*@BEGIN_FUNC 0x90f0 _title_090f0*/
void title_090f0(void *pv)
{
    TitleRec *p;
    unsigned long z;

    p = (TitleRec *)pv;

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
/*@END_FUNC*/

/*@BEGIN_FUNC 0x91a0 _title_091a0*/
void *title_091a0(unsigned long flags)
{
    int i;

    if (flags & 0x8000) {
        for (i = g_2bf30; i < g_2bf72; i++) {
            if (g_2dfa0[i].w2e == 0)
                break;
        }
        if (i < g_2bf72) {
            g_2bf30 = i;
            return &g_2dfa0[i];
        }
        for (i = g_2bf6e; i < g_2bf30; i++) {
            if (g_2dfa0[i].w2e == 0)
                break;
        }
        if (i < g_2bf30) {
            g_2bf30 = i;
            return &g_2dfa0[i];
        }
        return 0;
    }
    for (i = g_2bff0; i < g_2bf6e; i++) {
        if (g_2dfa0[i].w2e == 0)
            break;
    }
    if (i < g_2bf6e) {
        g_2bff0 = i;
        return &g_2dfa0[i];
    }
    for (i = g_2bf44; i < g_2bff0; i++) {
        if (g_2dfa0[i].w2e == 0)
            break;
    }
    if (i < g_2bff0) {
        g_2bff0 = i;
        return &g_2dfa0[i];
    }
    return 0;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x92e0 _title_092e0*/
TitleRec *title_092e0(unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4)
{
    TitleRec *p = (TitleRec *)title_091a0(a4);
    if (p != 0) {
        title_094c0(p);
        p->w2e = (unsigned short)(a4 | 0x2000);
        p->d00 = a1;
        p->d04 = a2;
        p->d08 = a3;
        p->b22 = g_2bf5c[a4 & 0xfff].b6;
        p->d64 = g_2bf5c[a4 & 0xfff].d0;
    }
    return p;
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x9350 _title_09350*/
int title_09350(TitleObj *p)
{
    char *edi;
    unsigned short ax;
    unsigned short cx;

    if (p == 0)
        return 1;
    if (p->w2e == 0)
        return 1;
    if (p->w2e & 0x2000) {
        if (p->d50 & 0x10000000) {
            p->w2e = 0;
            title_04610(p);
            return 1;
        }
        edi = p->p58;
        if (p->p58 == 0) {
            title_09830(p);
            title_09870(p);
            title_097f0(p);
            p->w2e = (unsigned short)(unsigned long)edi;
            title_04610(p);
            return 1;
        }
        ax = p->w38;
        cx = *(unsigned short *)(edi + g_2a0a0[ax].a + 0xc);
        edi += g_2a0a0[ax].a;
        if (cx & 0x2000)
            return 0;
        if (cx & 0x100) {
            if ((short)ax == (short)g_2cc03) {
                *(unsigned long *)(edi + 0) = p->x_00;
                *(unsigned long *)(edi + 4) = p->y_04;
                *(unsigned long *)(edi + 8) = p->dw08;
                *(unsigned short *)(edi + 0xe) = p->w2a;
            }
        }
        if (*(unsigned char *)&cx & 0x80) {
            if ((short)p->w38 == (short)g_2cc03) {
                cx = p->w2a;
                *(unsigned short *)(edi + 0xe) = cx;
            }
        }
        title_09830(p);
        title_09870(p);
        title_097f0(p);
        p->w2e = 0;
        *(unsigned short *)(edi + 0xc) &= 0x7fff;
        title_04610(p);
        return 1;
    }
    return title_08ae0(p);
}
/*@END_FUNC*/

/*@BEGIN_FUNC 0x9480 _title_09480*/
void title_09480(void)
{
    int i;
    for (i = 0; i < g_2bf72; i++)
        title_094c0(&g_2dfa0[i]);
}
/*@END_FUNC*/

} /* extern "C" */
