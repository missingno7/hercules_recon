#include <string.h>
#include "title_engine.h"
#include "title_screen.h"
#include "title_files.h"

typedef struct TitleCtxView {
    unsigned char unknown_000[6];
    unsigned char byte_006;
    unsigned char unknown_007[0x0e - 0x07];
    unsigned char byte_00e;
    unsigned char unknown_00f[0x12 - 0x0f];
    unsigned char byte_012;
    unsigned char unknown_013[0x2e - 0x13];
    unsigned short word_02e;
    unsigned long dword_030;
} TitleCtxView;

static TitleObject *g_2a38c;
static TitleObject *g_2a3a4;
static TitleObject *g_2a264;
static TitleObject *g_2a278;
static TitleObject *g_2a27c;
static TitleObject *g_2a280;
static TitleObject *g_2a274;
static int g_2a254;
extern volatile unsigned int g_25a28;  /* volatile: owner-approved codegen hypothesis (0xd720) */
static TitleObject *g_2a288[64];
static int g_2a9b8;
extern int g_25a30[43 * 6];
extern int g_29f98;
static int g_2a250;
extern int g_2bb3c;
static int g_2a270;
static int g_2a3ac;
static int g_2a3a0;
static int g_2a39c;
static int g_2a9bc;
static int g_2a9c0;
static int g_2a388;
static int g_2a268;
static int g_2a260;

TitleObject *title_17ad0(int a, int b, int c, int d, int e);
int title_0c4a0(int a);
int title_0c4c0(void);
int title_0cce0(void);
int title_0cd30(void);
void title_0c4e0(void);
void title_054f0(int a, TitleObject *b);
int title_0c4d0(int a);
int rand();

static int g_2a284;
static int g_2a258;
static int g_2a26c;
static int g_2a394;
static int g_2a398;
static int g_2a3a8;
static int g_2a3b0;
static int g_2a3b8[96];
static int g_2a538[96];
static int g_2a6b8[96];
static int g_2a838[96];
extern char g_29128[];
void title_01dd0();
void title_01de0(int a, int b, int c, int d, int e);
void title_02350(void);
void title_023c0(int a, int b);
void title_025c0(int a, int b);
void title_02860(void);
void title_04b70(int c, int d);
void title_04ea0(int a);
void title_05bc0(int a);
void title_05e90();
void title_05f10(void *target);
void title_07510();
void title_09350(TitleObject *p);
void title_0c330();
void title_0c3f0();
void title_0c450();
void title_0c4f0(int a);
void title_0c560(int a, int b);
void title_0c5a0(void);
void title_0c6d0(int a);
void title_0c8b0();
void title_0c920(int a);
void title_0c930(int a);
void title_0c990(int a, int b);
void title_0c9c0(int a, int b);
void title_1d790(int a);

int title_0cc90(void) {
    int d = title_0c4c0();
    g_2a9b8 = g_2a9b8 + ((d - g_2a9b8) >> 1);
    if (g_2a9b8 > 30) return 5;
    if (g_2a9b8 > 24) return 4;
    if (g_2a9b8 > 18) return 2;
    if (g_2a9b8 > 6) return 3;
    return 1;
}

int title_0cce0(void) {
    int i = 0;
    int p = (int)g_2a288;
    while (p < (int)(g_2a288 + 64)) {
        if (*(int *)p == 0) return i;
        p += 4;
        i++;
    }
    return -1;
}

int title_0cd00(int v) {
    int p = (int)g_2a288;
    while (p < (int)(g_2a288 + 64)) {
        if (*(int *)p == v) *(int *)p = 0;
        p += 4;
    }
    return -1;
}

int title_0cd30(void)
{
    int k;

    for (k = 0; k < 43; k++) {
        if (g_25a30[k * 6 + 0] == g_2a278->unknown_034 - 0x1e3 &&
            g_25a30[k * 6 + 1] == g_2a27c->unknown_034 - 0x121 &&
            g_25a30[k * 6 + 2] == g_2a280->unknown_034 - 0x182 &&
            g_25a30[k * 6 + 3] == g_2a274->unknown_034 - 0xc0)
            goto found;
    }
    return -1;
found:
    if (g_25a30[k * 6 + 4] == 0x64) {
        g_2a388 = 1;
        return g_25a30[k * 6 + 4];
    }
    if (g_25a30[k * 6 + 4] == 0x65 && g_2a388 == 1) {
        ((TitleCtxView *)g_engine_interface.context_004)->dword_030 |= 0x80000000;
        ((TitleCtxView *)g_engine_interface.context_004)->word_02e = 0;
        return g_25a30[k * 6 + 4];
    }
    if (g_25a30[k * 6 + 4] == 0x66) {
        g_2a268 = 1;
        return g_25a30[k * 6 + 4];
    }
    if (g_25a30[k * 6 + 4] == 0x67 && g_2a268 == 1) {
        ((TitleCtxView *)g_engine_interface.context_004)->dword_030 |= 0x80000000;
        ((TitleCtxView *)g_engine_interface.context_004)->word_02e = 1;
        return g_25a30[k * 6 + 4];
    }
    if (g_25a30[k * 6 + 4] == 0x68) {
        g_2a260 = 1;
        return g_25a30[k * 6 + 4];
    }
    if (g_25a30[k * 6 + 4] == 0x69 && g_2a260 == 1) {
        ((TitleCtxView *)g_engine_interface.context_004)->dword_030 |= 0x80000008;
        ((TitleCtxView *)g_engine_interface.context_004)->byte_006 = 3;
        return 1;
    }
    ((TitleCtxView *)g_engine_interface.context_004)->byte_00e = (unsigned char)g_25a30[k * 6 + 5];
    ((TitleCtxView *)g_engine_interface.context_004)->byte_012 = (unsigned char)k;
    return g_25a30[k * 6 + 4];
}

void title_0cf00(void) {
    g_2a38c = title_17ad0(0, 0, 0, 0x2006, 0);
    g_2a38c->unknown_034 = 1;
    g_2a38c->unknown_000 = 0x100000;
    g_2a38c->unknown_004 = 0x6c0000;
    g_2a38c->unknown_04a = 0;
    g_2a38c->unknown_054 |= 5;
    g_2a38c->unknown_03e = 0;
    g_2a38c->unknown_023 = 6;

    g_2a3a4 = title_17ad0(0, 0, 0, 0x2006, 0);
    g_2a3a4->unknown_034 = 1;
    g_2a3a4->unknown_000 = 0x110000;
    g_2a3a4->unknown_004 = 0x6d0000;
    g_2a3a4->unknown_04a = 0;
    g_2a3a4->unknown_054 |= 6;
    g_2a3a4->unknown_03e = 0xa;
    g_2a3a4->unknown_023 = 6;

    g_2a264 = title_17ad0(0, 0, 0, 0x2005, 0);
    g_2a264->unknown_034 = 2;
    g_2a264->unknown_04a = 0;
    g_2a264->unknown_000 = 0xff440000;
    g_2a264->unknown_004 = 0x5d0000;
    g_2a264->unknown_054 |= 0x2000;
    g_2a264->unknown_03e = 0xf;
    g_2a264->unknown_023 = 6;
    g_2a254 = 0;
    g_25a28 = 1;

    g_2a278 = title_17ad0(0, 0, 0, 0x2006, 0);
    g_2a278->unknown_034 = 0x1fb;
    g_2a278->unknown_000 = 0xffab0000;
    g_2a278->unknown_004 = 0xfffe0000;
    g_2a278->unknown_04a = 0;
    g_2a278->unknown_054 |= 0x2000;
    g_2a278->unknown_03e = 0x1e;
    g_2a278->unknown_023 = 6;

    g_2a27c = title_17ad0(0, 0, 0, 0x2006, 0);
    g_2a27c->unknown_034 = 0x139;
    g_2a27c->unknown_000 = 0xffee0000;
    g_2a27c->unknown_004 = 0xfffe0000;
    g_2a27c->unknown_04a = 0;
    g_2a27c->unknown_054 |= 0x2000;
    g_2a27c->unknown_03e = 0x1e;
    g_2a27c->unknown_023 = 6;

    g_2a280 = title_17ad0(0, 0, 0, 0x2006, 0);
    g_2a280->unknown_034 = 0x19a;
    g_2a280->unknown_000 = 0x2d0000;
    g_2a280->unknown_004 = 0xfffe0000;
    g_2a280->unknown_04a = 0;
    g_2a280->unknown_054 |= 0x2000;
    g_2a280->unknown_03e = 0x1e;
    g_2a280->unknown_023 = 6;

    g_2a274 = title_17ad0(0, 0, 0, 0x2006, 0);
    g_2a274->unknown_034 = 0xd8;
    g_2a274->unknown_000 = 0x6d0000;
    g_2a274->unknown_004 = 0xfffe0000;
    g_2a274->unknown_04a = 0;
    g_2a274->unknown_054 |= 0x2000;
    g_2a274->unknown_03e = 0x1e;
    g_2a274->unknown_023 = 6;

    {
        int idx;
        if (((TitleCtxView *)g_engine_interface.context_004)->byte_012 == 0xff) return;
        idx = (signed char)((TitleCtxView *)g_engine_interface.context_004)->byte_012;
        g_2a278->unknown_034 = g_25a30[idx * 6 + 0] + 0x1e3;
        idx = (signed char)((TitleCtxView *)g_engine_interface.context_004)->byte_012;
        g_2a27c->unknown_034 = g_25a30[idx * 6 + 1] + 0x121;
        idx = (signed char)((TitleCtxView *)g_engine_interface.context_004)->byte_012;
        g_2a280->unknown_034 = g_25a30[idx * 6 + 2] + 0x182;
        idx = (signed char)((TitleCtxView *)g_engine_interface.context_004)->byte_012;
        g_2a274->unknown_034 = g_25a30[idx * 6 + 3] + 0xc0;
    }
}

void title_0d250(TitleObject *s) {
    s->unknown_03e = 0x14;
    s->unknown_054 |= 5;
    s->unknown_023 = 6;
    s->unknown_034 = title_0c4a0(3) + 0xb6;
    s->unknown_074 += rand(0x400);
}

/* 0xd290 240 */
/* 0xd290: per-frame update of a password-screen sprite: step its frame down to 0xb6 on odd
   ticks, run its 0x4a timer down by two, wobble x by a 16-entry table indexed by the y
   coordinate while it rises by one pixel; when the timer is out, release it and clear its
   table entry. */
void title_0d290(TitleObject *p)
{
    int wobble[16] = {0, 0, 2, 0, 1, 0, 0, 0, 0, 0, -2, 0, -1, 0, 0, 0};

    if (g_29f98 != 0)
        return;
    if (((unsigned char *)g_engine_interface.context_004)[0x38] & 1) {
        if (p->unknown_034 > 0xb6)
            p->unknown_034--;
    }
    if (p->unknown_04a > 0)
        p->unknown_04a--;
    if (p->unknown_04a > 0)
        p->unknown_04a--;
    p->unknown_074++;
    p->unknown_000 += wobble[(p->unknown_004 >> 16) & 15] << 16;
    p->unknown_004 += 0x10000;
    if (p->unknown_04a == 0) {
        if (p) {
            title_09350(p);
            p = 0;
        }
        title_0cd00((int)p);
    }
}

int title_0d380(int a, int b)
{
    int idx;

    if (++g_2a9c0 < 8) {
        return ++g_2a9c0;
    }
    g_2a9c0 = 0;
    idx = title_0cce0();
    if (idx != -1) {
        g_2a288[idx] = title_17ad0(0, 0, 0, 0x2007, 0);
        if (g_2a288[idx] != 0) {
            g_2a288[idx]->unknown_000 = a;
            g_2a288[idx]->unknown_004 = b;
            g_2a288[idx]->unknown_04a = title_0c4a0(0x80);
            return (unsigned int)g_2a288[idx]->unknown_04a >> 1;
        }
    }
    return 0;
}

void title_0d440(TitleObject *p)
{
    if (g_29f98 == 0) {
        if (p->unknown_04a > 0) {
            p->unknown_04a--;
        }
        if (p->unknown_04a > 0) {
            p->unknown_04a--;
        }
        if (p->unknown_04a > 0) {
            p->unknown_04a--;
        }
        if (p->unknown_04a > 0) {
            p->unknown_04a--;
        }
        if (((unsigned char *)g_engine_interface.context_004)[0x38] & 1) {
            p->unknown_034++;
            if (p->unknown_034 == 0xc0) {
                p->unknown_034 = 0xb6;
            }
        }
    }
}

void title_0d4b0(void)
{
    if ((g_2a250 & 0x8) || (g_2a250 & 0x4000)) {
        g_2bb3c = title_0cd30();
        if (g_2bb3c != -1) {
            if (g_2bb3c < 0x64) {
                g_2a254 = 0x18;
            } else {
                title_054f0(0x302, 0);
            }
        } else {
            g_2a254 = 0x14;
        }
    }
    if (g_2a250 & 0x80) {
        if (g_25a28 > 0) {
            title_0c4e0();
            g_2a270 = 0;
            g_2a3ac = 0;
            g_2a254 = 0xa;
        } else {
            title_0c4e0();
            g_2a270 = 0;
            g_2a3ac = 0;
            g_2a3a0 = 5;
            g_2a254 = 0x1a;
        }
        title_054f0(0x301, 0);
    }
    if (g_2a250 & 0x20) {
        if (g_25a28 < 3) {
            title_0c4e0();
            g_2a270 = 0;
            g_2a3ac = 0;
            g_2a254 = 0xc;
        } else {
            title_0c4e0();
            g_2a270 = 0;
            g_2a3ac = 0;
            g_2a3a0 = 5;
            g_2a254 = 0x1b;
        }
        title_054f0(0x300, 0);
    }
    if (g_2a250 & 0x40) {
        g_2a3a0 = 5;
        g_2a3ac = 0;
        g_2a254 = 8;
        g_2a39c = 0;
        switch (g_25a28) {
        case 0: title_054f0(0x4311, g_2a278); break;
        case 1: title_054f0(0x4311, g_2a27c); break;
        case 2: title_054f0(0x4311, g_2a280); break;
        case 3: title_054f0(0x4311, g_2a274); break;
        }
        title_0c4e0();
        g_2a270 = 0;
    }
    if (g_2a250 & 0x10) {
        g_2a3a0 = 5;
        g_2a3ac = 0;
        g_2a254 = 9;
        g_2a39c = 0;
        switch (g_25a28) {
        case 0: title_054f0(0x4311, g_2a278); break;
        case 1: title_054f0(0x4311, g_2a27c); break;
        case 2: title_054f0(0x4311, g_2a280); break;
        case 3: title_054f0(0x4311, g_2a274); break;
        }
        title_0c4e0();
        g_2a270 = 0;
    }
}

void title_0d6c0(void)
{
    switch (g_2a9bc) {
    case 0: g_2a270 = title_0c4d0(1); break;
    case 1: g_2a270 = title_0c4d0(2); break;
    case 2: g_2a270 = title_0c4d0(3); break;
    case 3: g_2a270 = title_0c4d0(4); break;
    case 4: g_2a270 = title_0c4d0(5); break;
    }
    g_2a9bc++;
    if (g_2a9bc == 5) {
        g_2a9bc = 0;
    }
}

void title_0d720(void)
{
    if ((g_2a250 & 0x8) || (g_2a250 & 0x4000)) {
        g_2bb3c = title_0cd30();
        if (g_2bb3c != -1) {
            if (g_2bb3c < 0x64) {
                g_2a254 = 0x18;
            } else {
                title_054f0(0x302, 0);
            }
        } else {
            g_2a254 = 0x14;
        }
    }
    if (g_2a250 & 0x80) {
        if (g_25a28 > 0) {
            g_2a3ac = 0;
            g_2a254 = 0xf;
            title_0c4e0();
            g_2a270 = 0;
            title_054f0(0x301, 0);
        }
    }
    if (g_2a250 & 0x20) {
        if (g_25a28 < 3) {
            title_0c4e0();
            g_2a270 = 0;
            g_2a3ac = 0;
            g_2a254 = 0x12;
        } else {
            title_0c4e0();
            g_2a270 = 0;
            g_2a3ac = 0;
            g_2a3a0 = 4;
            g_2a254 = 0x1b;
        }
        title_054f0(0x300, 0);
    }
    if (g_2a250 & 0x10) {
        g_2a3a0 = 4;
        g_2a3ac = 0;
        g_2a254 = 9;
        g_2a39c = 0;
        switch (g_25a28) {
        case 0: title_054f0(0x4311, g_2a278); break;
        case 1: title_054f0(0x4311, g_2a27c); break;
        case 2: title_054f0(0x4311, g_2a280); break;
        case 3: title_054f0(0x4311, g_2a274); break;
        }
        title_0c4e0();
        g_2a270 = 0;
    }
    if (g_2a250 & 0x40) {
        g_2a3a0 = 4;
        g_2a3ac = 0;
        g_2a254 = 8;
        g_2a39c = 0;
        switch (g_25a28) {
        case 0: title_054f0(0x4311, g_2a278); break;
        case 1: title_054f0(0x4311, g_2a27c); break;
        case 2: title_054f0(0x4311, g_2a280); break;
        case 3: title_054f0(0x4311, g_2a274); break;
        }
        title_0c4e0();
        g_2a270 = 0;
    }
}

void title_0f460(TitleProc *out)
{
    g_2a284 = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5c;
    g_2a250 = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5e;

    switch (out->state_04) {
    case 1:
        switch (g_2a394) {
        case 0:
            title_01de0(0x140, 0, g_2a26c, 0, 0);
            title_01de0(0x140, 0x100, g_2a26c - (g_2a26c >> 1), 1, 0x4fe);
            if (g_29f98 == 0) {
                if (g_2a26c < 0x80) {
                    g_2a26c = g_2a26c + 0x10;
                } else {
                    g_2a26c = 0x80;
                    g_2a394 = 1;
                }
                g_2a274->unknown_04a = g_2a26c;
                g_2a280->unknown_04a = g_2a26c;
                g_2a27c->unknown_04a = g_2a26c;
                g_2a278->unknown_04a = g_2a26c;
                g_2a3a4->unknown_04a = g_2a26c;
                g_2a38c->unknown_04a = g_2a26c;
                g_2a264->unknown_04a = g_2a26c;
            }
            break;
        case 1:
            title_01de0(0x140, 0, 0x80, 0, 0);
            title_01de0(0x140, 0x100, 0x40, 1, 0x4fe);
            break;
        case 2:
            title_01de0(0x140, 0, g_2a26c, 0, 0);
            title_01de0(0x140, 0x100, g_2a26c - (g_2a26c >> 1), 1, 0x4fe);
            g_2a394 = 3;
            g_2a258 = 0;
            g_2a26c = 0x80;
            break;
        case 3:
            title_01de0(0x140, 0, g_2a26c, 0, 0);
            title_01de0(0x140, 0x100, g_2a26c - (g_2a26c >> 1), 1, 0x4fe);
            title_025c0(g_2a258, 1);
            title_02860();
            if (g_29f98 == 0) {
                g_2a264->unknown_054 |= 5;
                g_2a274->unknown_04a = g_2a26c;
                g_2a280->unknown_04a = g_2a26c;
                g_2a27c->unknown_04a = g_2a26c;
                g_2a278->unknown_04a = g_2a26c;
                g_2a264->unknown_04a = g_2a26c;
                g_2a38c->unknown_04a = g_2a26c;
                g_2a3a4->unknown_04a = g_2a26c;
                if (g_2a26c > 0) {
                    g_2a26c = g_2a26c - 0x10;
                }
                if (g_2a258 < 0x80) {
                    g_2a258 = g_2a258 + 0x10;
                } else {
                    g_2a38c->unknown_054 &= 0x7fffffff;
                    g_2a3a4->unknown_054 &= 0x7fffffff;
                    g_2a264->unknown_054 &= 0x7fffffff;
                    g_2a278->unknown_054 &= 0x7fffffff;
                    g_2a27c->unknown_054 &= 0x7fffffff;
                    g_2a280->unknown_054 &= 0x7fffffff;
                    g_2a274->unknown_054 &= 0x7fffffff;
                }
            }
            break;
        case 4:
            title_0c4f0(1);
            title_025c0(g_2a258, 1);
            title_02860();
            if (g_29f98 == 0) {
                if (g_2a258 > 0) {
                    g_2a258 = g_2a258 - 4;
                } else {
                    out->state_04 = 10;
                    out->delay_08 = 1;
                    return;
                }
            }
            break;
        case 5:
            title_01de0(0x140, 0, g_2a26c, 0, 0);
            title_01de0(0x140, 0x100, g_2a26c - (g_2a26c >> 1), 1, 0x4fe);
            if (g_29f98 == 0) {
                if (g_2a26c > 0) {
                    g_2a26c = g_2a26c - 8;
                    g_2a274->unknown_04a = g_2a26c;
                    g_2a280->unknown_04a = g_2a26c;
                    g_2a27c->unknown_04a = g_2a26c;
                    g_2a278->unknown_04a = g_2a26c;
                    g_2a3a4->unknown_04a = g_2a26c;
                    g_2a38c->unknown_04a = g_2a26c;
                    g_2a264->unknown_04a = g_2a26c;
                } else {
                    g_2a26c = 0;
                    out->state_04 = 10;
                    out->delay_08 = 1;
                    return;
                }
            }
            break;
        }
        if (g_2a284 == 0) {
            g_2a398 = 0;
        }
        if (g_2a398 == 0) {
            if ((g_2a284 & 0x1000) && g_2a394 != 5) {
                title_054f0(0x303, 0);
                title_0c4e0();
                g_2a270 = 0;
                title_0c4f0(1);
                g_2a394 = 5;
            }
        } else {
            g_2a398 = g_2a398 - 1;
        }
        out->state_04 = 1;
        out->delay_08 = 1;
        return;
    case 10:
        title_0c5a0();
        title_0c4e0();
        g_2a270 = 0;
        title_05bc0(0x303);
        {
            int i;
            for (i = 0; i < 64; i++) {
                if (g_2a288[i] != 0) {
                    title_09350(g_2a288[i]);
                    g_2a288[i] = 0;
                }
            }
        }
        if (g_2a38c != 0) {
            title_09350(g_2a38c);
            g_2a38c = 0;
        }
        if (g_2a3a4 != 0) {
            title_09350(g_2a3a4);
            g_2a3a4 = 0;
        }
        if (g_2a278 != 0) {
            title_09350(g_2a278);
            g_2a278 = 0;
        }
        if (g_2a27c != 0) {
            title_09350(g_2a27c);
            g_2a27c = 0;
        }
        if (g_2a280 != 0) {
            title_09350(g_2a280);
            g_2a280 = 0;
        }
        if (g_2a274 != 0) {
            title_09350(g_2a274);
            g_2a274 = 0;
        }
        if (g_2a264 != 0) {
            title_09350(g_2a264);
            g_2a264 = 0;
        }
        title_02350();
        title_04ea0(3);
        out->state_04 = 0xfffe;
        out->delay_08 = 1;
        return;
    case 0xffff:
        title_0c920(0x3e9);
        title_0c920(0x3ea);
        title_0c920(0x3eb);
        title_0c920(0x3ec);
        title_0c920(0x3ed);
        title_0c920(0x3e8);
        title_0c920(0x3ef);
        title_0c920(0x3fc);
        title_0c920(0x3ee);
        title_0c920(0x3fd);
        title_0c920(0x3fb);
        title_0c920(0x3f1);
        title_0c920(0x3f2);
        title_0c920(0x3f4);
        title_0c920(0x3fa);
        title_0c920(0x3f0);
        title_0c920(0x3f6);
        title_0c920(0x3f7);
        title_0c920(0x3f8);
        title_0c920(0x3f1);
        title_0c920(0x3f9);
        title_0c9c0(0x1000, 0x4000);
        title_0c990(-5, 0);
        g_2a3b8[8] = 1;
        g_2a3b8[16] = 1;
        g_2a3b8[24] = 1;
        g_2a3b8[32] = 1;
        g_2a3b8[40] = 1;
        g_2a3b8[48] = 1;
        g_2a3b8[56] = 1;
        g_2a3b8[64] = 1;
        g_2a3b8[72] = 1;
        g_2a3b8[80] = 1;
        g_2a3b8[89] = 1;
        g_2a538[8] = 1;
        g_2a538[16] = 1;
        g_2a538[24] = 1;
        g_2a538[32] = 1;
        g_2a538[40] = 1;
        g_2a538[48] = 1;
        g_2a538[56] = 1;
        g_2a538[65] = 1;
        g_2a538[73] = 1;
        g_2a538[81] = 1;
        g_2a538[89] = 1;
        g_2a6b8[8] = 1;
        g_2a6b8[16] = 1;
        g_2a6b8[24] = 1;
        g_2a6b8[32] = 1;
        g_2a6b8[40] = 1;
        g_2a6b8[48] = 1;
        g_2a6b8[56] = 1;
        g_2a6b8[64] = 1;
        g_2a6b8[72] = 1;
        g_2a6b8[81] = 1;
        g_2a6b8[89] = 1;
        g_2a838[8] = 1;
        g_2a838[16] = 1;
        g_2a838[24] = 1;
        g_2a838[32] = 1;
        g_2a838[40] = 1;
        g_2a838[48] = 1;
        g_2a838[56] = 1;
        g_2a838[64] = 1;
        g_2a838[72] = 1;
        g_2a838[80] = 1;
        g_2a838[89] = 1;
        g_2a388 = 0;
        g_2a268 = 0;
        g_2a260 = 0;
        g_engine_interface.context_004->unknown_048 = 0x13546547;
        g_engine_interface.context_004->unknown_04c = 0xecab9ab8;
        memset(g_2a288, 0, sizeof(g_2a288));
        g_2a3a8 = 0;
        g_2a398 = 0xf;
        title_0c560(0, 1);
        title_1d790(1);
        g_2a3b0 = 0;
        title_0c450(&g_2a3b0, 0x29000, 0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_PSWD], g_2a3b0, 0x14312);
        memset((void *)(g_2a3b0 + 0x14312), 0, 0x14312);
        title_01dd0(g_2a3b0);
        title_0c8b0(g_2a3b0 + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(g_2a3b0 + 0x14312);
        title_0c8b0(g_2a3b0 + 0x14624, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(g_2a3b0);
        title_04b70(3, 0);
        title_0cf00();
        title_023c0(0x140, 0x100);
        g_2a394 = 0;
        g_2a26c = 0;
        g_2a258 = 0;
        break;
    case 0xfffe:
        title_0c930(0x3e9);
        title_0c930(0x3ea);
        title_0c930(0x3eb);
        title_0c930(0x3ec);
        title_0c930(0x3ed);
        title_0c930(0x3e8);
        title_0c930(0x3ef);
        title_0c930(0x3fc);
        title_0c930(0x3ee);
        title_0c930(0x3fd);
        title_0c930(0x3fb);
        title_0c930(0x3f1);
        title_0c930(0x3f2);
        title_0c930(0x3f4);
        title_0c930(0x3fa);
        title_0c930(0x3f0);
        title_0c930(0x3f6);
        title_0c930(0x3f7);
        title_0c930(0x3f8);
        title_0c930(0x3f1);
        title_0c930(0x3f9);
        title_0c9c0(0, 0);
        title_0c4e0();
        g_2a270 = 0;
        title_05e90(title_07510, 0);
        title_05f10(out);
        return;
    default:
        return;
    }
    out->state_04 = 1;
    out->delay_08 = 1;
}
