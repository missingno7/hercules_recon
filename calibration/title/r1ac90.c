#include "title_engine.h"

typedef struct ScreenContext {
    unsigned char unknown_00[0x5c];
} ScreenContext;

typedef struct TitleObject {
    unsigned long unknown_000;
    unsigned long unknown_004;
    int unknown_008;
    unsigned char unknown_00c[0x34 - 0x0c];
    unsigned short unknown_034;
    unsigned char unknown_036[0x4a - 0x36];
    unsigned short unknown_04a;
    unsigned char unknown_04c[0x54 - 0x4c];
    unsigned long unknown_054;
} TitleObject;

extern int g_2aff8[4];
extern TitleObject *g_2b1d8[4];
extern int g_2b0f0[4];
extern int g_2b0ec;
extern int g_2b1d0;
extern int g_2afe4;

unsigned int title_0c4a0(unsigned int a);
TitleObject *title_17ad0(int a0, int a1, int a2, int size, int a4);
void title_054f0(int a, void *b);
void title_05ad0(int a, void *b);
void title_1ab70(int which);
void title_1ac10(int which);
void title_19a40(int a, int b, int c, int d, int e, int f, int g);

void title_1ac90(void)
{
    int i;

    for (i = 0; i < 4; i++) {
        switch (g_2aff8[i]) {
        case 0:
            g_2b1d8[i] = title_17ad0(0, 0, 0, 0x2012, 0);
            switch (i) {
            case 0:
                g_2b1d8[0]->unknown_034 = g_2b0f0[0] + 0x14a;
                if (g_2b1d8[0]->unknown_034 < 0x18e)
                    g_2b1d8[0]->unknown_034 += 0x5c;
                break;
            case 1:
                g_2b1d8[1]->unknown_034 = g_2b0f0[1] + 0x88;
                if (g_2b1d8[1]->unknown_034 < 0xcc)
                    g_2b1d8[1]->unknown_034 += 0x5c;
                break;
            case 2:
                g_2b1d8[2]->unknown_034 = g_2b0f0[2] + 0xe9;
                if (g_2b1d8[2]->unknown_034 < 0x12d)
                    g_2b1d8[2]->unknown_034 += 0x5c;
                break;
            case 3:
                g_2b1d8[3]->unknown_034 = g_2b0f0[3] + 0x27;
                if (g_2b1d8[3]->unknown_034 < 0x6b)
                    g_2b1d8[3]->unknown_034 += 0x5c;
                break;
            }
            g_2b1d8[i]->unknown_000 = (i * 15 << 18) - 0x5a0000;
            g_2b1d8[i]->unknown_004 = 0x80000;
            g_2b1d8[i]->unknown_008 = 0x3e80000;
            g_2b1d8[i]->unknown_054 &= 0x7fffffff;
            g_2aff8[i] = 1;
            break;
        case 1:
            break;
        case 2:
            g_2b1d8[i]->unknown_054 |= 0x80000000;
            if (g_2b1d8[i]->unknown_008 > 0) {
                g_2b1d8[i]->unknown_008 -= 0x640000;
                title_1ab70(i);
            } else {
                if (g_2b0ec == 0)
                    title_054f0(0x4319, g_2b1d8[i]);
                g_2b1d8[i]->unknown_008 = 0;
                g_2aff8[i] = 4;
                title_1ab70(i);
            }
            break;
        case 3:
            break;
        case 4:
            title_1ab70(i);
            switch (i) {
            case 0:
                if (g_2b0f0[0] != -1 && g_2b0f0[0] == (int)g_2b1d8[0]->unknown_034 - 0x18a) {
                    title_05ad0(0x319, g_2b1d8[0]);
                    if (g_2b0ec == 0)
                        title_054f0(0x31a, 0);
                    g_2aff8[0] = 5;
                    g_2b1d8[0]->unknown_04a = 0xff;
                    title_1ac10(0);
                }
                break;
            case 1:
                if (g_2aff8[0] > 4 && g_2b0f0[1] != -1 && g_2b0f0[1] == (int)g_2b1d8[1]->unknown_034 - 0xc8) {
                    title_05ad0(0x319, g_2b1d8[1]);
                    if (g_2b0ec == 0)
                        title_054f0(0x31a, 0);
                    g_2aff8[1] = 5;
                    g_2b1d8[1]->unknown_04a = 0xff;
                    title_1ac10(1);
                }
                break;
            case 2:
                if (g_2aff8[1] > 4 && g_2b0f0[2] != -1 && g_2b0f0[2] == (int)g_2b1d8[2]->unknown_034 - 0x129) {
                    title_05ad0(0x319, g_2b1d8[2]);
                    if (g_2b0ec == 0)
                        title_054f0(0x31a, 0);
                    g_2aff8[2] = 5;
                    g_2b1d8[2]->unknown_04a = 0xff;
                    title_1ac10(2);
                }
                break;
            case 3:
                if (g_2aff8[2] > 4 && g_2b0f0[3] != -1 && g_2b0f0[3] == (int)g_2b1d8[3]->unknown_034 - 0x67) {
                    title_05ad0(0x319, g_2b1d8[3]);
                    if (g_2b0ec == 0) {
                        title_054f0(0x31a, 0);
                        if (((ScreenContext *)g_engine_interface.context_004)->unknown_00[0x11] == 0xf)
                            title_054f0(0x31e, 0);
                    }
                    g_2aff8[3] = 5;
                    g_2b1d8[3]->unknown_04a = 0xff;
                    title_1ac10(3);
                }
                break;
            }
            break;
        case 5:
            if (g_2b1d8[i]->unknown_04a > 0x80)
                g_2b1d8[i]->unknown_04a -= 4;
            else
                g_2aff8[i] = 6;
            if (g_2b1d0 != 4 && title_0c4a0(0x14) == 0) {
                title_19a40(0x23f, 0x2012,
                            ((title_0c4a0(0x20) - 0x10) << 16) + g_2b1d8[i]->unknown_000,
                            ((title_0c4a0(4) - 8) << 16) + g_2b1d8[i]->unknown_004,
                            0, title_0c4a0(3), g_2afe4);
            }
            break;
        default:
            break;
        }
        if (g_2aff8[i] != 5)
            g_2b1d8[i]->unknown_04a = (unsigned short)g_2afe4;
    }
}
