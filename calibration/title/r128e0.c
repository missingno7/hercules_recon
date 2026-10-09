#include "title_engine.h"

typedef struct ScreenContext {
    unsigned char unknown_00[0x5c];
    volatile short cursor_5c;
    volatile short cursor_5e;
} ScreenContext;

typedef struct TitleObject {
    unsigned long unknown_000;
    unsigned long unknown_004;
    unsigned char unknown_008[0x23 - 0x08];
    unsigned char unknown_023;
    unsigned char unknown_024[0x34 - 0x24];
    unsigned short unknown_034;
    unsigned char unknown_036[0x3e - 0x36];
    unsigned short unknown_03e;
    unsigned char unknown_040[0x4a - 0x40];
    unsigned short unknown_04a;
    unsigned char unknown_04c[0x54 - 0x4c];
    unsigned long unknown_054;
} TitleObject;

typedef struct TitleRequest {
    unsigned long unknown_000;
    unsigned long unknown_004;
    unsigned long unknown_008;
} TitleRequest;

extern TitleObject *g_2ac04;
extern TitleObject *g_2ac0c;
extern TitleObject *g_2ac18;
extern int g_2ac00;
extern int g_2ac14;
extern int g_2ac1c;
extern int g_2ac20;
extern int g_2ac24;
extern int g_29f98;
extern int g_22318;
extern char g_29128[];

void title_01dd0();
void title_04b70(int c, int d);
void title_04ea0(int a);
void title_05e90(void (*fn)(void *), int n);
void title_05f10(TitleRequest *req);
void title_09350(void *block);
void title_0c330(void *p);
void title_0c450(void **out, int size, int flags);
void title_0c6d0(int a);
void title_0c8b0(char *a0, int a1, int a2, int a3, int a4, int a5, int a6);
int title_0c3f0(char *a, int b, char *c, int d);
void title_0c4f0(int a);
void title_14790(void);
void title_14860(void);
void title_164b0(void *req);
void title_16300(void);
void title_1d790(int a);
void title_01de0(int a0, int a1, int a2, int a3, int a4);


extern TitleObject *g_2ab48;
extern TitleObject *g_2ab54;
extern TitleObject *g_2ab64;
extern int g_2ab50;
extern int g_2ab5c;
extern int g_2ab60;
extern int g_2ab68;
extern int g_2ab6c;
extern int g_2ab70;
extern int g_2232c;
extern unsigned short g_25f50[];
void title_12cc0(void);
void title_12dc0(void);

void title_128e0(TitleRequest *self)
{
    void *handle;
    short cursor_y = ((ScreenContext *)g_engine_interface.context_004)->cursor_5e;

    switch (self->unknown_004) {
    case 0xfffd:
        title_05e90(title_164b0, 0);
        title_05f10(self);
        return;
    case 0xfffe:
        title_01dd0();
        self->unknown_004 = 0xfffd;
        self->unknown_008 = 2;
        return;
    case 0xffff:
        g_2ab5c = 0;
        g_2ab68 = 0x0a;
        g_2ab60 = 0;
        g_2ab50 = 0x0f;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_2232c, (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(0x0f, 0);
        title_12cc0();
        title_1d790(1);
        g_2ab6c = 0x80;
        g_2ab70 = 0;
        self->unknown_004 = 1;
        self->unknown_008 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        break;
    case 2:
        switch (g_2ab68) {
        case 0:
            title_01de0(0x140, 0, g_2ab70, 3, 0);
            title_1d790(0);
            if (g_2ab70 < 0x80) {
                g_2ab70 += 8;
            }
            break;
        case 1:
            title_0c4f0(1);
            g_2ab68 = 2;
            /* fall through */
        case 2:
            title_01de0(0x140, 0, g_2ab70, 3, 0);
            title_1d790(0);
            if (g_2ab70 > 0) {
                g_2ab70 -= 8;
            } else {
                g_2ab68 = 3;
            }
            break;
        case 11:
            if (g_2ab48->unknown_04a < 0x80) {
                g_2ab48->unknown_04a += 8;
                g_2ab64->unknown_04a = g_2ab48->unknown_04a * 2;
            }
            /* fall through */
        case 10:
            title_01de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_01de0(0x280, 0x100, g_2ab6c, 0, 0);
            g_2ab54->unknown_04a = (unsigned short)g_2ab6c;
            g_2ab48->unknown_04a = (unsigned short)g_2ab6c;
            g_2ab64->unknown_04a = (unsigned short)(g_2ab6c * 2);
            if (g_2ab6c > 0) {
                g_2ab6c -= 8;
            } else {
                g_2ab54->unknown_054 &= 0x7fffffff;
                g_2ab48->unknown_054 &= 0x7fffffff;
                g_2ab68 = 0;
            }
            break;
        default:
            break;
        }
        title_12dc0();
        if (g_2ab68 == 0x0c) {
            if (g_2ab48 != 0) {
                title_09350(g_2ab48);
                g_2ab48 = 0;
            }
            if (g_2ab64 != 0) {
                title_09350(g_2ab64);
                g_2ab64 = 0;
            }
            if (g_2ab54 != 0) {
                title_09350(g_2ab54);
                g_2ab54 = 0;
            }
            title_04ea0(0x0f);
            title_16300();
            self->unknown_004 = 0xfffe;
            self->unknown_008 = 2;
            return;
        }
        break;
    default:
        return;
    }
    self->unknown_004 = 2;
    self->unknown_008 = 1;
}

void title_14300(TitleRequest *self)
{
    void *handle;
    short cursor_x = ((ScreenContext *)g_engine_interface.context_004)->cursor_5c;
    short cursor_y = ((ScreenContext *)g_engine_interface.context_004)->cursor_5e;
    int step = 8;

    switch (self->unknown_004) {
    case 0xfffd:
        title_05e90(title_164b0, 0);
        title_05f10(self);
        return;
    case 0xfffe:
        title_01dd0();
        self->unknown_004 = 0xfffd;
        self->unknown_008 = 2;
        return;
    case 0xffff:
        g_2ac24 = 0;
        g_2ac00 = 0x0f;
        g_2ac1c = 0;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_22318, (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(8, 0);
        title_14860();
        title_1d790(1);
        g_2ac20 = 0x80;
        g_2ac14 = 0;
        self->unknown_004 = 1;
        self->unknown_008 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        break;
    case 2:
        switch (g_2ac1c) {
        case 0:
            title_01de0(0x140, 0x100, g_2ac14, 0, 0);
            if (g_2ac14 < 0x80) {
                g_2ac14 += step;
            }
            break;
        case 1:
            title_01de0(0x140, 0, g_2ac20, 0, 0);
            title_01de0(0x140, 0x100, g_2ac14, 1, 1);
            if (g_2ac20 < 0x80) {
                g_2ac20 += step;
            } else {
                g_2ac20 = 0x80;
            }
            if (g_2ac14 > 0) {
                g_2ac14 -= step;
            } else {
                g_2ac14 = 0;
            }
            break;
        case 2:
            title_0c4f0(1);
            g_2ac1c = 3;
            /* fall through */
        case 3:
            title_01de0(0x140, 0, g_2ac20, 0, 0);
            if (g_2ac20 > 0) {
                g_2ac20 -= 4;
            }
            break;
        case 11:
            if (g_2ac04->unknown_04a < 0x80) {
                g_2ac04->unknown_04a += 8;
                g_2ac18->unknown_04a = g_2ac04->unknown_04a * 2;
            }
            /* fall through */
        case 10:
            title_01de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_01de0(0x280, 0x100, g_2ac20, 0, 0);
            g_2ac0c->unknown_04a = (unsigned short)g_2ac20;
            g_2ac04->unknown_04a = (unsigned short)g_2ac20;
            g_2ac18->unknown_04a = (unsigned short)(g_2ac20 * 2);
            if (g_2ac20 > 0) {
                g_2ac20 -= step;
            } else {
                g_2ac0c->unknown_054 &= 0x7fffffff;
                g_2ac04->unknown_054 &= 0x7fffffff;
                g_2ac1c = 0;
            }
            break;
        default:
            break;
        }
        if (g_2ac1c == 0x0c) {
            if (g_2ac04 != 0) {
                title_09350(g_2ac04);
                g_2ac04 = 0;
            }
            if (g_2ac18 != 0) {
                title_09350(g_2ac18);
                g_2ac18 = 0;
            }
            if (g_2ac0c != 0) {
                title_09350(g_2ac0c);
                g_2ac0c = 0;
            }
            title_04ea0(step);
            title_16300();
            self->unknown_004 = 0xfffe;
            self->unknown_008 = 2;
            return;
        }
        if (g_29f98 == 0) {
            title_14790();
        }
        break;
    default:
        return;
    }
    self->unknown_004 = 2;
    self->unknown_008 = 1;
}
