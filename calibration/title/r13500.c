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

extern TitleObject *g_2aba4;
extern TitleObject *g_2abac;
extern TitleObject *g_2abc0;
extern TitleObject *g_2ac2c;
extern TitleObject *g_2ac34;
extern TitleObject *g_2ac48;
extern int g_2abb4;
extern int g_2abb8;
extern int g_2abbc;
extern int g_2abc4;
extern int g_2abc8;
extern int g_2ac3c;
extern int g_2ac40;
extern int g_2ac44;
extern int g_2ac4c;
extern int g_2ac50;
extern int g_22310;
extern int g_2231c;
extern int g_29f98;
extern char g_29128[];

void title_01dd0();
void title_02350(void);
void title_04b70(int c, int d);
void title_04ea0(int a);
void title_05e90(void (*fn)(void *), int n);
void title_05f10(TitleRequest *req);
void title_09350(void *block);
void title_0c330(void *p);
void title_0c450(void **out, int size, int flags);
void title_0c4f0(int a);
void title_0c6d0(int a);
void title_0c8b0(char *a0, int a1, int a2, int a3, int a4, int a5, int a6);
int title_0c3f0(char *a, int b, char *c, int d);
void title_139d0(void);
void title_13ad0(void);
void title_14e10(void);
void title_14f20(void);
void title_164b0(void *req);
void title_16300(void);
void title_1d790(int a);
void title_1de0(int a0, int a1, int a2, int a3, int a4);

void title_13500(TitleRequest *self)
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
        g_2abc8 = 0;
        g_2abbc = 0x0f;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_22310, (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(6, 0);
        title_139d0();
        title_1d790(1);
        g_2abb4 = 0x80;
        g_2abb8 = 0;
        g_2abc4 = 0x0a;
        self->unknown_004 = 1;
        self->unknown_008 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        break;
    case 2:
        switch (g_2abc4) {
        case 0:
            title_1de0(0x140, 0x100, g_2abb8, 0, 0);
            if (g_2abb8 < 0x80) {
                g_2abb8 += 8;
            }
            break;
        case 1:
            title_1de0(0x140, 0, g_2abb4, 0, 0);
            title_1de0(0x140, 0x100, g_2abb8, 1, 1);
            if (g_2abb4 < 0x80) {
                g_2abb4 += 8;
            } else {
                g_2abb4 = 0x80;
            }
            if (g_2abb8 > 0) {
                g_2abb8 -= 8;
            } else {
                g_2abb8 = 0;
            }
            break;
        case 2:
            title_1de0(0x140, 0, g_2abb4, 0, 0);
            title_1de0(0x140, 0x100, g_2abb8, 1, 1);
            if (g_2abb4 > 0) {
                g_2abb4 -= 4;
            } else {
                g_2abb4 = 0;
            }
            if (g_2abb8 < 0x80) {
                g_2abb8 += 4;
            } else {
                g_2abb8 = 0x80;
            }
            break;
        case 11:
            if (g_2aba4->unknown_04a < 0x80) {
                g_2aba4->unknown_04a += 8;
                g_2abc0->unknown_04a = g_2aba4->unknown_04a * 2;
            }
            /* fall through */
        case 10:
            title_1de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_1de0(0x280, 0x100, g_2abb4, 0, 0);
            g_2abac->unknown_04a = (unsigned short)g_2abb4;
            g_2aba4->unknown_04a = (unsigned short)g_2abb4;
            g_2abc0->unknown_04a = (unsigned short)(g_2abb4 * 2);
            if (g_2abb4 > 0) {
                g_2abb4 -= 8;
            } else {
                g_2abac->unknown_054 &= 0x7fffffff;
                g_2aba4->unknown_054 &= 0x7fffffff;
                g_2abc4 = 0;
            }
            break;
        default:
            break;
        }
        if (g_2abc4 == 0x0c) {
            title_02350();
            if (g_2abac != 0) {
                title_09350(g_2abac);
                g_2abac = 0;
            }
            if (g_2aba4 != 0) {
                title_09350(g_2aba4);
                g_2aba4 = 0;
            }
            if (g_2abc0 != 0) {
                title_09350(g_2abc0);
                g_2abc0 = 0;
            }
            title_04ea0(6);
            title_16300();
            self->unknown_004 = 0xfffe;
            self->unknown_008 = 2;
            return;
        }
        if (g_29f98 == 0) {
            title_13ad0();
        }
        break;
    default:
        return;
    }
    self->unknown_004 = 2;
    self->unknown_008 = 1;
}

void title_14970(TitleRequest *self)
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
        g_2ac3c = 0;
        g_2ac50 = 0x0f;
        g_2ac4c = 0;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_2231c, (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(9, 0);
        title_14e10();
        title_1d790(1);
        g_2ac40 = 0x80;
        g_2ac44 = 0;
        self->unknown_004 = 1;
        self->unknown_008 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        break;
    case 2:
        switch (g_2ac4c) {
        case 0:
            title_1de0(0x140, 0x100, g_2ac44, 0, 0);
            if (g_2ac44 < 0x80) {
                g_2ac44 += 8;
            }
            break;
        case 1:
            title_1de0(0x140, 0, g_2ac40, 0, 0);
            title_1de0(0x140, 0x100, g_2ac44, 1, 1);
            if (g_2ac40 < 0x80) {
                g_2ac40 += 8;
            } else {
                g_2ac40 = 0x80;
            }
            if (g_2ac44 > 0) {
                g_2ac44 -= 8;
            } else {
                g_2ac44 = 0;
            }
            break;
        case 2:
            title_0c4f0(1);
            g_2ac4c = 3;
            /* fall through */
        case 3:
            title_1de0(0x140, 0, g_2ac40, 0, 0);
            if (g_2ac40 > 0) {
                g_2ac40 -= 4;
            }
            break;
        case 11:
            if (g_2ac2c->unknown_04a < 0x80) {
                g_2ac2c->unknown_04a += 8;
            }
            g_2ac48->unknown_04a = g_2ac2c->unknown_04a * 2;
            /* fall through */
        case 10:
            title_1de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_1de0(0x280, 0x100, g_2ac40, 0, 0);
            g_2ac34->unknown_04a = (unsigned short)g_2ac40;
            g_2ac2c->unknown_04a = (unsigned short)g_2ac40;
            g_2ac48->unknown_04a = (unsigned short)(g_2ac40 * 2);
            if (g_2ac40 > 0) {
                g_2ac40 -= 8;
            } else {
                g_2ac34->unknown_054 &= 0x7fffffff;
                g_2ac2c->unknown_054 &= 0x7fffffff;
                g_2ac48->unknown_054 &= 0x7fffffff;
                g_2ac4c = 0;
            }
            break;
        default:
            break;
        }
        if (g_2ac4c == 0x0c) {
            if (g_2ac34 != 0) {
                title_09350(g_2ac34);
                g_2ac34 = 0;
            }
            if (g_2ac2c != 0) {
                title_09350(g_2ac2c);
                g_2ac2c = 0;
            }
            if (g_2ac48 != 0) {
                title_09350(g_2ac48);
                g_2ac48 = 0;
            }
            title_04ea0(9);
            title_16300();
            self->unknown_004 = 0xfffe;
            self->unknown_008 = 2;
            return;
        }
        if (g_29f98 == 0) {
            title_14f20();
        }
        break;
    default:
        return;
    }
    self->unknown_004 = 2;
    self->unknown_008 = 1;
}
