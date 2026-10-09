#include "title_engine.h"
#include "title_screen.h"
#include "title_files.h"

/* Game-state view of g_engine_interface.context_004 (offsets observed in 0x1a70). */
typedef struct TitleLanguageState {
    unsigned char unknown_000[0x0e];
    unsigned char byte_00e;
    unsigned char unknown_00f[0x9c - 0x0f];
    short word_09c;
} TitleLanguageState;

static int g_2912c;
static int g_29130;
static int g_29134;
static int g_29138;
static int g_2913c;
static int g_29140;
static int g_29144;

void title_01a30(void)
{
    switch (g_29144) {
    case 0: if (g_29134 < 0x80) { g_29134 += 0x10; return; } break;
    case 1: if (g_29134 > 0) { g_29134 -= 0x10; return; } break;
    default: return;
    }
    g_29144 = 3;
}

extern char g_29128[];
void title_0c990(int a, int b);
void title_0c9c0(int a, int b);
void title_0c560(int a, int b);
void title_1d790(int a);
void title_0c450();
void title_0c3f0();
void title_0c2a0();
void title_0c8b0();
void title_0c6d0(int a);
void title_0c330();
void title_0c5a0(void);
void title_054f0(int a, int b);
void title_01de0(int a, int b, int c, int d, int e);
void title_05e90();
void title_05f10(void *target);
void title_164b0();

void title_01a70(TitleProc *out)
{
    int x = ((ScreenContext *)g_engine_interface.context_004)->cursor_5c;
    unsigned long buttons = (unsigned short)((ScreenContext *)g_engine_interface.context_004)->cursor_5e;

    switch (out->state_04) {
    case 1:
        title_01de0(0x140, 0, g_29134, 0, 0);
        if (buttons & 8) {
            title_054f0(0x303, 0);
            out->state_04 = 4;
            out->delay_08 = 1;
            return;
        }
        if (g_2913c < 2) {
            g_29144 = 0;
            title_01a30();
        }
        g_29138++;
        if (g_29138 > 0x3c) {
            g_2913c++;
            if (g_2913c == 0x1d) {
                g_2913c = 0x1e;
            }
            if (g_2913c >= 0x20) {
                out->state_04 = 4;
                out->delay_08 = 1;
                return;
            }
            title_0c2a0(g_29128, g_language_files[g_2913c], 0, g_29140, 0x14312);
            out->state_04 = 2;
            out->delay_08 = 1;
            return;
        }
        out->state_04 = 1;
        out->delay_08 = 1;
        return;
    case 2:
        title_01de0(0x140, 0, g_29134, 0, 0);
        if (buttons & 8) {
            title_054f0(0x303, 0);
            out->state_04 = 4;
            out->delay_08 = 1;
            return;
        }
        if (((TitleLanguageState *)g_engine_interface.context_004)->word_09c == 0) {
            out->state_04 = 3;
            out->delay_08 = 1;
            return;
        }
        out->state_04 = 2;
        out->delay_08 = 1;
        return;
    case 3:
        title_01de0(0x140, 0, g_29134, 0, 0);
        title_0c8b0(g_29140 + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        g_29138 = 0;
        out->state_04 = 1;
        out->delay_08 = 1;
        return;
    case 4:
        title_01de0(0x140, 0, g_29134, 0, 0);
        g_29144 = 1;
        title_01a30();
        if (g_29144 == 3) {
            out->state_04 = 0xfffe;
            out->delay_08 = 1;
            return;
        }
        out->state_04 = 4;
        out->delay_08 = 1;
        return;
    case 0xfffe:
        title_0c5a0();
        title_0c330(g_29140);
        title_05e90(title_164b0, 0);
        title_05f10(out);
        return;
    case 0xffff:
        title_0c990(-2, 0);
        title_0c9c0(8, 8);
        title_0c560(0, 4);
        g_2912c = 0xf;
        title_1d790(1);
        g_29140 = 0;
        title_0c450(&g_29140, 0x15000, 0);
        g_29134 = 0;
        g_29144 = 0;
        g_29130 = 0;
        if (((TitleLanguageState *)g_engine_interface.context_004)->byte_00e == 0) {
            g_2913c = 0;
            g_29138 = -0xf0;
        } else {
            g_2913c = 1;
            g_29138 = 0;
        }
        title_0c3f0(g_29128, g_language_files[g_2913c], g_29140, 0x14312);
        title_0c8b0(g_29140 + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        break;
    default:
        return;
    }
    out->state_04 = 1;
    out->delay_08 = 1;
}

void title_01dd0(void)
{
}
