/* Behavioral source for TITLE.DLL 0x164b0. Source-unit ownership remains provisional;
   its four private cells are native BSS 0x2ace0..0x2acec. */
#include "../title_engine.h"
#include "../title_files.h"
#include "../title_screen.h"

typedef struct TitlePadMap {
    unsigned short button[8];
} TitlePadMap;

/* Game-state view of g_engine_interface.context_004 (offsets observed in 0x164b0). */
typedef struct TitleGameState {
    char level_000;
    unsigned char unknown_001[3 - 1];
    unsigned char unknown_003;
    unsigned char unknown_004;
    unsigned char unknown_005;
    unsigned char unknown_006;
    unsigned char unknown_007;
    unsigned char unknown_008;
    unsigned char unknown_009[0xb - 9];
    unsigned char unknown_00b;
    unsigned char unknown_00c;
    unsigned char unknown_00d;
    unsigned char unknown_00e;
    unsigned char unknown_00f;
    unsigned char unknown_010;
    unsigned char unknown_011;
    unsigned char unknown_012;
    unsigned char unknown_013;
    short unknown_014;
    short unknown_016;
    short unknown_018;
    short unknown_01a;
    short unknown_01c;
    unsigned char unknown_01e[0x30 - 0x1e];
    unsigned long flags_030;
    unsigned char unknown_034[0x6c - 0x34];
    TitlePadMap pad_06c;
    unsigned char unknown_07c[0x128 - 0x7c];
    unsigned char unknown_128;
    unsigned char unknown_129[0x138 - 0x129];
    unsigned short unknown_138;
} TitleGameState;

#define GS ((TitleGameState *)g_engine_interface.context_004)

/* Private loader-zero cells observed at RVAs 0x2ace0..0x2acef. */
static int g_2ace0, g_2ace4, g_2ace8, g_2acec;


extern int g_2bb30;
extern int g_2bb34;
extern int g_2bb38;
extern int g_2b370;
extern char g_29128[];
extern int g_26068[14];
extern TitlePadMap g_254e8[4];

int rand();
void title_01dd0();
void title_01de0(int a0, int a1, int a2, int a3, int a4);
void title_02a20(void);
void title_02a80(void);
void title_05c60(int a);
void title_05db0(void (*fn)(TitleProc *), int a, int b, int c);
int title_06350(int a);
int title_09980(int a, int b);
int title_0c280(char *a, char *b);
void title_0c320(void);
void title_0c330(void *p);
void title_0c370(int a, int b, int c, int d, int e);
int title_0c3f0(char *a, char *name, char *c, int d);
void title_0c410(int a, int b);
void title_0c430(int a, int b);
void title_0c450(void **out, int size, int flags);
void title_0c500(int a, int b);
void title_0c540(int a);
void title_0c5a0(void);
void title_0c5c0(void);
void title_0c6d0(int a);
void title_0c890(unsigned short *rect, int value);
void title_0c8b0(char *a0, int a1, int a2, int a3, int a4, int a5, int a6);
void title_0c990(int a, int b);
void title_0cc80(void (*callback)(void));
void title_16300(void);
void title_16470(void);

void title_01a70(TitleProc *self);
void title_07510(TitleProc *self);
void title_102f0(TitleProc *self);
void title_10670(TitleProc *self);
void title_109f0(TitleProc *self);
void title_12280(TitleProc *self);
void title_128e0(TitleProc *self);
void title_12e90(TitleProc *self);
void title_13500(TitleProc *self);
void title_13ba0(TitleProc *self);
void title_14300(TitleProc *self);
void title_14970(TitleProc *self);
void title_14ff0(TitleProc *self);
void title_15650(TitleProc *self);
void title_15cb0(TitleProc *self);
void title_1c500(TitleProc *self);

void title_164b0(TitleProc *self)
{
    unsigned short rect[4];
    void *handle;
    int pick;

    switch (self->state_04) {
    case 0x34:
        title_01de0(0x140, 0, g_2ace0, 0, 0);
        g_2ace4++;
        if (g_2ace0 < 0x80) {
            g_2ace0 += 0x10;
            self->state_04 = 0x34;
            self->delay_08 = 1;
            return;
        }
        self->state_04 = 0x35;
        self->delay_08 = 1;
        return;
    case 0x35:
        title_01de0(0x140, 0, g_2ace0, 0, 0);
        g_2ace4++;
        if (g_2ace4 > 0x78) {
            self->state_04 = 0x36;
            self->delay_08 = 1;
            return;
        }
        self->state_04 = 0x35;
        self->delay_08 = 1;
        return;
    case 0x36:
        title_01de0(0x140, 0, g_2ace0, 0, 0);
        if (g_2ace0 > 0) {
            g_2ace0 -= 8;
            self->state_04 = 0x36;
            self->delay_08 = 1;
            return;
        }
        self->state_04 = 0x26;
        self->delay_08 = 1;
        return;
    case 0x33:
        self->state_04 = 0x26;
        self->delay_08 = 1;
        return;
    case 0x26:
        self->state_04 = 0x27;
        self->delay_08 = 1;
        return;
    case 0x27:
        if (g_2ace8 != 0) {
            if (title_09980(1, 0x20000) & 8) {
                title_01dd0();
                self->state_04 = 0x24;
                self->delay_08 = 2;
                return;
            }
        } else {
            if ((title_09980(0x1a, 0x20000) & 8) || (title_09980(1, 0x20000) & 8)) {
                title_01dd0();
                self->state_04 = 0x24;
                self->delay_08 = 2;
                return;
            }
        }
        self->state_04 = 0x25;
        self->delay_08 = 1;
        return;
    case 0x25:
        if (g_2ace8 != 0) {
            pick = title_09980(2, 0x20000);
        } else {
            pick = title_09980(2, 0x20000);
        }
        if (pick & 8) {
            title_01dd0();
            self->state_04 = 0x24;
            self->delay_08 = 2;
            return;
        }
        self->state_04 = 0x24;
        self->delay_08 = 1;
        return;
    case 0x24:
        if (g_2ace8 != 0) {
            title_09980(3, 0x10000);
        } else {
            title_09980(3, 0x10000);
        }
        title_01dd0();
        self->state_04 = 0xe7;
        self->delay_08 = 2;
        return;
    case 0x28:
        if (g_2bb30 != 0) {
            self->state_04 = 0xdc;
            self->delay_08 = 2;
            return;
        }
        if (g_2bb34 != 0) {
            GS->level_000 = g_2bb38 - 1;
        } else if (GS->level_000 != 0) {
            GS->level_000--;
            g_2acec = 0;
            self->state_04 = 0x29;
            self->delay_08 = 1;
            return;
        } else {
            GS->level_000 = 0;
        }
        GS->unknown_00c = GS->unknown_00b;
        GS->unknown_010 = 0;
        self->state_04 = 0x29;
        self->delay_08 = 1;
        return;
    case 0x29:
        title_0c990(-3, 0);
        if (GS->unknown_006 != 3) {
            GS->unknown_006 = 0;
        }
        GS->unknown_01c = 0;
        GS->unknown_011 = 0;
        GS->unknown_01a = 0;
        GS->unknown_018 = 0;
        GS->unknown_016 = 0;
        GS->unknown_014 = 0;
        title_0c5a0();
        GS->level_000++;
        switch (GS->level_000) {
        case 12:
            if (GS->unknown_00e == 0) {
                self->state_04 = 0xcf;
                self->delay_08 = 1;
                return;
            }
            self->state_04 = 0xfa;
            self->delay_08 = 1;
            return;
        case 11:
            if (GS->unknown_00e == 0) {
                self->state_04 = 0xcf;
                self->delay_08 = 1;
                return;
            }
            self->state_04 = 0x6e;
            self->delay_08 = 1;
            return;
        case 10:
            if (GS->unknown_00e == 0) {
                self->state_04 = 0xcf;
                self->delay_08 = 1;
                return;
            }
            self->state_04 = 0x6d;
            self->delay_08 = 1;
            return;
        case 9:
            self->state_04 = 0x6c;
            self->delay_08 = 1;
            return;
        case 8:
            self->state_04 = 0x6b;
            self->delay_08 = 1;
            return;
        case 7:
            self->state_04 = 0x6a;
            self->delay_08 = 1;
            return;
        case 6:
            self->state_04 = 0x69;
            self->delay_08 = 1;
            return;
        case 4:
            GS->level_000 = 5;
            /* fall through */
        case 5:
            self->state_04 = 0x67;
            self->delay_08 = 1;
            return;
        case 3:
            self->state_04 = 0x66;
            self->delay_08 = 1;
            return;
        case 2:
            self->state_04 = 0x65;
            self->delay_08 = 1;
            return;
        case 1:
            self->state_04 = 0x64;
            self->delay_08 = 1;
            return;
        case 0:
            self->state_04 = 0xe6;
            self->delay_08 = 1;
            return;
        default:
            self->state_04 = 0xe6;
            self->delay_08 = 1;
            return;
        }
    case 0x5a:
        title_0c5c0();
        self->state_04 = 0xffff;
        self->delay_08 = 1;
        return;

    /* level 1 */
    case 0x64:
        if (g_2acec == 0) {
            title_09980(0x12, 0x20000);
        }
        title_05db0(title_14300, 0, 1, 0);
        self->state_04 = 0xa0;
        self->delay_08 = 0;
        return;
    case 0xa0:
        title_0c280(g_29128, g_engine_paths[GS->level_000]);
        self->state_04 = 0x78;
        self->delay_08 = 0;
        return;
    case 0x78:
        title_16300();
        self->state_04 = 0x8c;
        self->delay_08 = 1;
        return;
    case 0x8c:
        title_09980(4, 0x20000);
        self->state_04 = 0xc8;
        self->delay_08 = 1;
        return;
    /* level 2 */
    case 0x65:
        title_05db0(title_14970, 0, 1, 0);
        self->state_04 = 0xa1;
        self->delay_08 = 0;
        return;
    case 0xa1:
        title_0c280(g_29128, g_engine_paths[GS->level_000]);
        self->state_04 = 0x79;
        self->delay_08 = 0;
        return;
    case 0x79:
        title_16300();
        self->state_04 = 0x8d;
        self->delay_08 = 1;
        return;
    case 0x8d:
        title_09980(4, 0x20000);
        self->state_04 = 0xc8;
        self->delay_08 = 1;
        return;
    /* level 3 */
    case 0x66:
        title_05db0(title_13ba0, 0, 1, 0);
        self->state_04 = 0xa2;
        self->delay_08 = 0;
        return;
    case 0xa2:
        title_0c280(g_29128, g_engine_paths[GS->level_000]);
        self->state_04 = 0x7a;
        self->delay_08 = 0;
        return;
    case 0x7a:
        title_16300();
        self->state_04 = 0x8e;
        self->delay_08 = 1;
        return;
    case 0x8e:
        title_09980(8, 0x20000);
        self->state_04 = 0xc8;
        self->delay_08 = 1;
        return;
    case 0xb6:
        title_09980(0xc, 0x20000);
        self->state_04 = 0x29;
        self->delay_08 = 1;
        return;
    /* level 4 */
    case 0x67:
        title_05db0(title_12280, 0, 1, 0);
        self->state_04 = 0xa3;
        self->delay_08 = 0;
        return;
    case 0xa3:
        title_0c280(g_29128, g_engine_paths[GS->level_000]);
        self->state_04 = 0x7b;
        self->delay_08 = 0;
        return;
    case 0x7b:
        title_16300();
        self->state_04 = 0x8f;
        self->delay_08 = 1;
        return;
    case 0x8f:
        title_09980(6, 0x20000);
        self->state_04 = 0xc8;
        self->delay_08 = 1;
        return;
    case 0xb7:
        title_09980(0xd, 0x20000);
        self->state_04 = 0x29;
        self->delay_08 = 1;
        return;
    /* level 6 */
    case 0x69:
        title_05db0(title_15cb0, 0, 1, 0);
        self->state_04 = 0xa5;
        self->delay_08 = 0;
        return;
    case 0xa5:
        title_0c280(g_29128, g_engine_paths[GS->level_000]);
        self->state_04 = 0x7d;
        self->delay_08 = 0;
        return;
    case 0x7d:
        title_16300();
        self->state_04 = 0x91;
        self->delay_08 = 1;
        return;
    case 0x91:
        title_09980(0x14, 0x20000);
        self->state_04 = 0xc8;
        self->delay_08 = 1;
        return;
    case 0xb9:
        title_09980(0xe, 0x20000);
        self->state_04 = 0x29;
        self->delay_08 = 1;
        return;
    /* level 7 */
    case 0x6a:
        title_05db0(title_12e90, 0, 1, 0);
        self->state_04 = 0xa6;
        self->delay_08 = 0;
        return;
    case 0xa6:
        title_0c280(g_29128, g_engine_paths[GS->level_000]);
        self->state_04 = 0x7e;
        self->delay_08 = 0;
        return;
    case 0x7e:
        title_16300();
        self->state_04 = 0x92;
        self->delay_08 = 1;
        return;
    case 0x92:
        title_09980(6, 0x20000);
        self->state_04 = 0xc8;
        self->delay_08 = 1;
        return;
    case 0xba:
        title_09980(0x11, 0x20000);
        self->state_04 = 0x29;
        self->delay_08 = 1;
        return;
    /* level 8 */
    case 0x6b:
        title_05db0(title_13500, 0, 1, 0);
        self->state_04 = 0xa7;
        self->delay_08 = 0;
        return;
    case 0xa7:
        title_0c280(g_29128, g_engine_paths[GS->level_000]);
        self->state_04 = 0x7f;
        self->delay_08 = 0;
        return;
    case 0x7f:
        title_16300();
        self->state_04 = 0x93;
        self->delay_08 = 1;
        return;
    case 0x93:
        title_09980(7, 0x20000);
        self->state_04 = 0xc8;
        self->delay_08 = 1;
        return;
    case 0xbb:
        title_09980(0x13, 0x20000);
        self->state_04 = 0x29;
        self->delay_08 = 1;
        return;
    /* level 9 */
    case 0x6c:
        title_05db0(title_14ff0, 0, 1, 0);
        self->state_04 = 0xa8;
        self->delay_08 = 0;
        return;
    case 0xa8:
        title_0c280(g_29128, g_engine_paths[GS->level_000]);
        self->state_04 = 0x80;
        self->delay_08 = 0;
        return;
    case 0x80:
        title_16300();
        self->state_04 = 0x94;
        self->delay_08 = 1;
        return;
    case 0x94:
        title_09980(8, 0x20000);
        self->state_04 = 0xc8;
        self->delay_08 = 1;
        return;
    case 0xbc:
        if (GS->unknown_00e == 0) {
            self->state_04 = 0xcd;
            self->delay_08 = 1;
            return;
        }
        title_09980(0x19, 0x20000);
        self->state_04 = 0x29;
        self->delay_08 = 1;
        return;
    /* level 10 */
    case 0x6d:
        title_05db0(title_15650, 0, 1, 0);
        self->state_04 = 0xa9;
        self->delay_08 = 0;
        return;
    case 0xa9:
        title_0c280(g_29128, g_engine_paths[GS->level_000]);
        self->state_04 = 0x81;
        self->delay_08 = 0;
        return;
    case 0x81:
        title_16300();
        self->state_04 = 0x95;
        self->delay_08 = 1;
        return;
    case 0x95:
        title_09980(0xa, 0x20000);
        self->state_04 = 0xc8;
        self->delay_08 = 1;
        return;
    case 0xbd:
        title_09980(0x10, 0x20000);
        self->state_04 = 0x29;
        self->delay_08 = 1;
        return;
    /* level 11 */
    case 0x6e:
        title_05db0(title_128e0, 0, 1, 0);
        self->state_04 = 0xaa;
        self->delay_08 = 0;
        return;
    case 0xaa:
        title_0c280(g_29128, g_engine_paths[GS->level_000]);
        self->state_04 = 0x82;
        self->delay_08 = 0;
        return;
    case 0x82:
        title_16300();
        self->state_04 = 0x96;
        self->delay_08 = 1;
        return;
    case 0x96:
        title_09980(0xa, 0x20000);
        self->state_04 = 0xc8;
        self->delay_08 = 1;
        return;
    case 0xbe:
        if (GS->unknown_00e == 2) {
            title_09980(0xf, 0x20000);
        } else {
            title_09980(0x1b, 0x20000);
        }
        self->state_04 = 0xcd;
        self->delay_08 = 1;
        return;

    case 0xc8:
        if (GS->flags_030 & 0x10) {
            GS->unknown_010 = 1;
        }
        if (GS->unknown_010 > 0) {
            g_2acec = ((unsigned)title_09980(0x16, 0x4000) >> 0xe) & 1;
            self->state_04 = 0xc9;
            self->delay_08 = 1;
            return;
        }
        g_2acec = 0;
        self->state_04 = 0xc9;
        self->delay_08 = 1;
        return;
    case 0xc9:
        if (g_2acec != 0) {
            GS->unknown_010--;
            GS->unknown_00c = GS->unknown_00b;
            GS->level_000--;
            title_09980(0x15, 0x20000);
            title_16470();
            self->state_04 = 0x29;
            self->delay_08 = 1;
            return;
        }
        if (title_09980(9, 0x20000) & 8) {
            self->state_04 = 0xe6;
            self->delay_08 = 1;
            return;
        }
        title_09980(3, 0x20000);
        self->state_04 = 0xe6;
        self->delay_08 = 1;
        return;
    case 0xd2:
        title_0c540(1);
        title_0c370(0x140, 0x100, -4, -4, 0x100);
        title_0cc80(title_0c320);
        rect[0] = 0x390;
        GS->unknown_128 = 0x14;
        rect[1] = GS->unknown_138;
        rect[2] = 0x50;
        rect[3] = 0xc8;
        title_0c890(rect, g_2b370);
        title_0c6d0(0);
        title_0c410(0xc, 1);
        title_0c430(0x26, 2);
        title_05db0(title_1c500, 0, 1, 0);
        self->state_04 = 0xd3;
        self->delay_08 = 0;
        return;
    case 0xd3:
        switch (GS->level_000) {
        case 3:
            self->state_04 = 0xb6;
            self->delay_08 = 1;
            return;
        case 4:
        case 5:
            self->state_04 = 0xb7;
            self->delay_08 = 1;
            return;
        case 6:
            self->state_04 = 0xb9;
            self->delay_08 = 1;
            return;
        case 7:
            self->state_04 = 0xba;
            self->delay_08 = 1;
            return;
        case 8:
            self->state_04 = 0xbb;
            self->delay_08 = 1;
            return;
        case 9:
            self->state_04 = 0xbc;
            self->delay_08 = 1;
            return;
        case 10:
            self->state_04 = 0xbd;
            self->delay_08 = 1;
            return;
        case 11:
            self->state_04 = 0xbe;
            self->delay_08 = 1;
            return;
        default:
            self->state_04 = 0x29;
            self->delay_08 = 1;
            return;
        }
    case 0xcd:
        title_0c5c0();
        title_05c60(0);
        title_0c500(0x55, 0x6e);
        title_16470();
        title_05db0(title_01a70, 0, 1, 0);
        self->state_04 = 0xce;
        self->delay_08 = 0;
        return;
    case 0xce:
        title_09980(3, 0x20000);
        self->state_04 = 0xf0;
        self->delay_08 = 1;
        return;
    case 0xcf:
        title_05db0(title_01a70, 0, 1, 0);
        self->state_04 = 0xce;
        self->delay_08 = 0;
        return;
    case 0xdc:
        title_0c5a0();
        GS->pad_06c = g_254e8[1];
        if (GS->unknown_006 == 0) {
            GS->unknown_006 = 1;
            if (GS->unknown_007 == 7) {
                GS->unknown_007 = 0;
            }
            if (GS->unknown_007 < 7) {
                GS->level_000 = g_26068[GS->unknown_007 * 2];
                GS->unknown_008 = g_26068[GS->unknown_007 * 2 + 1];
                GS->unknown_007++;
            } else {
                pick = rand(0xff) % 7;
                GS->level_000 = g_26068[pick * 2];
                GS->unknown_008 = g_26068[pick * 2 + 1];
            }
            title_01dd0();
            title_16300();
        } else {
            title_09980(0xb, 0x8000);
            GS->unknown_006 = 0;
        }
        self->state_04 = 0xf0;
        self->delay_08 = 1;
        return;
    case 0xe7:
        if (g_2ace8 != 0) {
            if (title_06350(0) == 1) {
                title_05db0(title_109f0, 0, 1, 0);
                self->state_04 = 0xe6;
                self->delay_08 = 0;
                return;
            }
        }
        self->state_04 = 0xe6;
        self->delay_08 = 1;
        return;
    case 0xe6:
        if (GS->unknown_006 == 3) {
            title_16470();
            if (GS->level_000 == 0xb) {
                GS->level_000 = 0;
            }
            self->state_04 = 0x29;
            self->delay_08 = 1;
            return;
        }
        title_0c540(1);
        title_0c370(0x140, 0x100, -4, -4, 0x100);
        title_0cc80(title_0c320);
        handle = 0;
        GS->unknown_128 = 0x14;
        rect[0] = 0x390;
        rect[1] = GS->unknown_138;
        rect[2] = 0x50;
        rect[3] = 0xc8;
        title_0c890(rect, g_2b370);
        title_0c6d0(0);
        title_0c450(&handle, 0x15000, 0);
        title_0c3f0(g_29128, g_screen_files[11], (char *)handle, 0x14312);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        GS->unknown_128 |= 0xa;
        title_16470();
        title_05db0(title_07510, 0, 1, 0);
        g_2acec = 0;
        self->state_04 = 0x28;
        self->delay_08 = 0;
        return;
    case 0xf0:
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_0c3f0(g_29128, g_screen_files[11], (char *)handle, 0x14312);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_05db0(title_07510, 0, 1, 0);
        g_2acec = 0;
        self->state_04 = 0x28;
        self->delay_08 = 0;
        return;
    case 0xfa:
        if (title_09980(0x12, 0x20000) & 8) {
            self->state_04 = 0xf0;
            self->delay_08 = 1;
            return;
        }
        if (title_09980(0xc, 0x20000) & 8) {
            self->state_04 = 0xf0;
            self->delay_08 = 1;
            return;
        }
        if (title_09980(0xd, 0x20000) & 8) {
            self->state_04 = 0xf0;
            self->delay_08 = 1;
            return;
        }
        if (title_09980(0xe, 0x20000) & 8) {
            self->state_04 = 0xf0;
            self->delay_08 = 1;
            return;
        }
        if (title_09980(0xb, 0x20000) & 8) {
            self->state_04 = 0xf0;
            self->delay_08 = 1;
            return;
        }
        if (title_09980(0x11, 0x20000) & 8) {
            self->state_04 = 0xf0;
            self->delay_08 = 1;
            return;
        }
        if (title_09980(0x13, 0x20000) & 8) {
            self->state_04 = 0xf0;
            self->delay_08 = 1;
            return;
        }
        if (title_09980(0x19, 0x20000) & 8) {
            self->state_04 = 0xf0;
            self->delay_08 = 1;
            return;
        }
        if (title_09980(0x10, 0x20000) & 8) {
            self->state_04 = 0xf0;
            self->delay_08 = 1;
            return;
        }
        if (GS->unknown_00e == 2) {
            if (title_09980(0xf, 0x20000) & 8) {
                self->state_04 = 0xf0;
                self->delay_08 = 1;
                return;
            }
        } else {
            if (title_09980(0x1b, 0x20000) & 8) {
                self->state_04 = 0xf0;
                self->delay_08 = 1;
                return;
            }
        }
        self->state_04 = 0xf0;
        self->delay_08 = 1;
        return;
    case 0x104:
        title_05db0(title_102f0, 0, 1, 0);
        self->state_04 = 0xe6;
        self->delay_08 = 0;
        return;
    case 0xffff:
        GS->unknown_004 = 0;
        title_05c60(0);
        title_0c500(0x55, 0x6e);
        title_02a20();
        title_02a80();
        g_2ace8 = 0;
        if (GS->unknown_003 == 0x11) {
            GS->unknown_007 = 0;
            GS->unknown_003 = 0;
            GS->unknown_012 = 0xff;
            g_2ace8 = 1;
            self->state_04 = 0x33;
            self->delay_08 = 1;
            return;
        }
        if (GS->unknown_003 == 0x12) {
            title_01dd0();
            self->state_04 = 0xe6;
            self->delay_08 = 2;
            return;
        }
        if (GS->unknown_003 == 0x13) {
            title_01dd0();
            self->state_04 = 0xe6;
            self->delay_08 = 2;
            return;
        }
        if (GS->unknown_003 == 0x31) {
            self->state_04 = 0xd2;
            self->delay_08 = 1;
            return;
        }
        switch (GS->level_000) {
        case 1:
            self->state_04 = 0x8c;
            self->delay_08 = 1;
            return;
        case 2:
            self->state_04 = 0x8d;
            self->delay_08 = 1;
            return;
        case 3:
            self->state_04 = 0x8e;
            self->delay_08 = 1;
            return;
        case 4:
            GS->level_000 = 5;
            /* fall through */
        case 5:
            self->state_04 = 0x8f;
            self->delay_08 = 1;
            return;
        case 6:
            self->state_04 = 0x91;
            self->delay_08 = 1;
            return;
        case 7:
            self->state_04 = 0x92;
            self->delay_08 = 1;
            return;
        case 8:
            self->state_04 = 0x93;
            self->delay_08 = 1;
            return;
        case 9:
            self->state_04 = 0x94;
            self->delay_08 = 1;
            return;
        case 10:
            self->state_04 = 0x95;
            self->delay_08 = 1;
            return;
        case 11:
            self->state_04 = 0x96;
            self->delay_08 = 1;
            return;
        }
        return;
    case 0x10e:
        title_05db0(title_10670, 0, 1, 0);
        self->state_04 = 0xe6;
        self->delay_08 = 0;
        return;
    }
}
