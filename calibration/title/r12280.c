#include "title_engine.h"
#include "title_files.h"

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

typedef struct TitleStep {
    unsigned long unknown_000;
    unsigned long phase_004;
    unsigned long result_008;
} TitleStep;

typedef struct ScreenContext {
    unsigned char unknown_00[0x5e];
    volatile short cursor_5e;
} ScreenContext;

extern int g_2ab2c;
extern int g_2ab30;
extern int g_2ab38;
extern int g_2ab3c;
extern int g_2ab40;
extern TitleObject *g_2ab1c;
extern TitleObject *g_2ab24;
extern TitleObject *g_2ab34;
extern int g_2ab88;
extern int g_2ab8c;
extern int g_2ab90;
extern int g_2ab94;
extern int g_2ab9c;
extern TitleObject *g_2ab78;
extern TitleObject *g_2ab80;
extern TitleObject *g_2ab98;
extern int g_29f98;
extern char g_29128[];

extern void title_164b0(void);
void title_01dd0();
void title_01de0();
void title_04b70(int c, int d);
void title_04ea0(int a);
void title_05e90();
void title_05f10(void *target);
void title_09350(void *block);
int title_0c3f0(char *a, char *name, char *c, int d);
void title_0c450(void **out, int size, int flags);
void title_0c4f0(int a);
void title_0c330(void *value);
void title_0c6d0(int a);
void title_0c8b0(char *a1, int a2, int a3, int a4, int a5, int a6, int a7);
void title_12710(void);
void title_12810(void);
void title_13310(void);
void title_13430(void);
void title_16300(void);
void title_1d790(int a);

void title_12280(TitleStep *self)
{
    void *handle;
    short cursor_y = ((ScreenContext *)g_engine_interface.context_004)->cursor_5e;

    switch (self->phase_004) {
    case 0xfffd:
        title_05e90(title_164b0, 0);
        title_05f10(self);
        return;
    case 0xfffe:
        title_01dd0();
        self->phase_004 = 0xfffd;
        self->result_008 = 2;
        return;
    case 0xffff:
        g_2ab38 = 0xa;
        g_2ab30 = 0;
        g_2ab2c = 0xf;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ1], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(4, 0);
        title_12710();
        title_1d790(1);
        g_2ab40 = 0x80;
        g_2ab3c = 0;
        self->phase_004 = 1;
        self->result_008 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        self->phase_004 = 2;
        self->result_008 = 1;
        return;
    case 2:
        break;
    default:
        return;
    }

    switch (g_2ab38) {
    case 0:
        title_01de0(0x140, 0x100, g_2ab3c, 0, 0);
        if (g_2ab3c < 0x80) {
            g_2ab3c += 8;
        }
        break;
    case 1:
        g_2ab24->unknown_054 &= 0x7fffffff;
        g_2ab1c->unknown_054 &= 0x7fffffff;
        title_01de0(0x140, 0, g_2ab40, 0, 0);
        title_01de0(0x140, 0x100, g_2ab3c, 1, 1);
        if (g_2ab40 < 0x80) {
            g_2ab40 += 8;
        }
        if (g_2ab3c > 0) {
            g_2ab3c -= 8;
        }
        break;
    case 2:
        title_0c4f0(1);
        g_2ab38 = 3;
        /* fall through */
    case 3:
        title_01de0(0x140, 0, g_2ab40, 0, 0);
        if (g_2ab40 > 0) {
            g_2ab40 -= 4;
        }
        break;
    case 11:
        if (g_2ab1c->unknown_04a < 0x80) {
            g_2ab1c->unknown_04a += 8;
            g_2ab34->unknown_04a = g_2ab1c->unknown_04a * 2;
        }
        /* fall through */
    case 10:
        title_01de0(0x280, 0x100, 0x80, 0, 0);
        break;
    case 12:
        title_01de0(0x280, 0x100, g_2ab40, 0, 0);
        g_2ab24->unknown_04a = g_2ab40;
        g_2ab1c->unknown_04a = g_2ab40;
        g_2ab34->unknown_04a = g_2ab40 * 2;
        if (g_2ab40 > 0) {
            g_2ab40 -= 8;
        } else {
            g_2ab24->unknown_054 &= 0x7fffffff;
            g_2ab1c->unknown_054 &= 0x7fffffff;
            g_2ab38 = 0;
        }
        break;
    }

    if (g_29f98 == 0) {
        title_12810();
    }
    if (g_2ab38 == 0xc) {
        if (g_2ab1c != 0) {
            title_09350(g_2ab1c);
            g_2ab1c = 0;
        }
        if (g_2ab34 != 0) {
            title_09350(g_2ab34);
            g_2ab34 = 0;
        }
        if (g_2ab24 != 0) {
            title_09350(g_2ab24);
            g_2ab24 = 0;
        }
        title_04ea0(4);
        title_16300();
        self->phase_004 = 0xfffe;
        self->result_008 = 2;
        return;
    }
    self->phase_004 = 2;
    self->result_008 = 1;
}

void title_12e90(TitleStep *self)
{
    void *handle;
    short cursor_y = ((ScreenContext *)g_engine_interface.context_004)->cursor_5e;

    switch (self->phase_004) {
    case 0xfffd:
        title_05e90(title_164b0, 0);
        title_05f10(self);
        return;
    case 0xfffe:
        title_01dd0();
        self->phase_004 = 0xfffd;
        self->result_008 = 2;
        return;
    case 0xffff:
        g_2ab88 = 0;
        g_2ab90 = 0xf;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ2], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(5, 0);
        title_13310();
        title_1d790(1);
        g_2ab8c = 0x80;
        g_2ab94 = 0;
        g_2ab9c = 0xa;
        self->phase_004 = 1;
        self->result_008 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        self->phase_004 = 2;
        self->result_008 = 1;
        return;
    case 2:
        break;
    default:
        return;
    }

    switch (g_2ab9c) {
    case 0:
        title_01de0(0x140, 0x100, g_2ab94, 0);
        if (g_2ab94 < 0x80) {
            g_2ab94 += 8;
        }
        break;
    case 1:
        title_01de0(0x140, 0, g_2ab8c, 0, 0);
        title_01de0(0x140, 0x100, g_2ab94, 1, 1);
        if (g_2ab8c < 0x80) {
            g_2ab8c += 8;
        }
        if (g_2ab94 > 0) {
            g_2ab94 -= 8;
        }
        break;
    case 2:
        title_0c4f0(1);
        g_2ab9c = 3;
        /* fall through */
    case 3:
        title_01de0(0x140, 0, g_2ab8c, 0, 0);
        if (g_2ab8c > 0) {
            g_2ab8c -= 4;
        }
        break;
    case 11:
        if (g_2ab78->unknown_04a < 0x80) {
            g_2ab78->unknown_04a += 8;
            g_2ab98->unknown_04a = g_2ab78->unknown_04a * 2;
        }
        /* fall through */
    case 10:
        title_01de0(0x280, 0x100, 0x80, 0, 0);
        break;
    case 12:
        title_01de0(0x280, 0x100, g_2ab8c, 0, 0);
        g_2ab80->unknown_04a = g_2ab8c;
        g_2ab78->unknown_04a = g_2ab8c;
        g_2ab98->unknown_04a = g_2ab8c * 2;
        if (g_2ab8c > 0) {
            g_2ab8c -= 8;
        } else {
            g_2ab80->unknown_054 &= 0x7fffffff;
            g_2ab78->unknown_054 &= 0x7fffffff;
            g_2ab9c = 0;
        }
        break;
    }

    if (g_2ab9c == 0xc) {
        if (g_2ab80 != 0) {
            title_09350(g_2ab80);
            g_2ab80 = 0;
        }
        if (g_2ab78 != 0) {
            title_09350(g_2ab78);
            g_2ab78 = 0;
        }
        if (g_2ab98 != 0) {
            title_09350(g_2ab98);
            g_2ab98 = 0;
        }
        title_04ea0(5);
        title_16300();
        self->phase_004 = 0xfffe;
        self->result_008 = 2;
        return;
    }
    if (g_29f98 == 0) {
        title_13430();
    }
    self->phase_004 = 2;
    self->result_008 = 1;
}
