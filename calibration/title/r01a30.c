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

/* Private .bss of the 0x1a30 unit (0x2912c..0x29147) in address order: names chosen for meaning and
   for VC5's identifier-hash layout order (scripts/layout_names.py), not recovered identifiers. */
static int s_unknown_2c;
static int s_unknown_30;
static int s_fade;
static int s_frame_count;
static int s_language_index;
static int s_buffer;
static int s_phase;

void title_01a30(void)
{
    switch (s_phase) {
    case 0: if (s_fade < 0x80) { s_fade += 0x10; return; } break;
    case 1: if (s_fade > 0) { s_fade -= 0x10; return; } break;
    default: return;
    }
    s_phase = 3;
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
        title_01de0(0x140, 0, s_fade, 0, 0);
        if (buttons & 8) {
            title_054f0(0x303, 0);
            out->state_04 = 4;
            out->delay_08 = 1;
            return;
        }
        if (s_language_index < 2) {
            s_phase = 0;
            title_01a30();
        }
        s_frame_count++;
        if (s_frame_count > 0x3c) {
            s_language_index++;
            if (s_language_index == 0x1d) {
                s_language_index = 0x1e;
            }
            if (s_language_index >= 0x20) {
                out->state_04 = 4;
                out->delay_08 = 1;
                return;
            }
            title_0c2a0(g_29128, g_language_files[s_language_index], 0, s_buffer, 0x14312);
            out->state_04 = 2;
            out->delay_08 = 1;
            return;
        }
        out->state_04 = 1;
        out->delay_08 = 1;
        return;
    case 2:
        title_01de0(0x140, 0, s_fade, 0, 0);
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
        title_01de0(0x140, 0, s_fade, 0, 0);
        title_0c8b0(s_buffer + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        s_frame_count = 0;
        out->state_04 = 1;
        out->delay_08 = 1;
        return;
    case 4:
        title_01de0(0x140, 0, s_fade, 0, 0);
        s_phase = 1;
        title_01a30();
        if (s_phase == 3) {
            out->state_04 = 0xfffe;
            out->delay_08 = 1;
            return;
        }
        out->state_04 = 4;
        out->delay_08 = 1;
        return;
    case 0xfffe:
        title_0c5a0();
        title_0c330(s_buffer);
        title_05e90(title_164b0, 0);
        title_05f10(out);
        return;
    case 0xffff:
        title_0c990(-2, 0);
        title_0c9c0(8, 8);
        title_0c560(0, 4);
        s_unknown_2c = 0xf;
        title_1d790(1);
        s_buffer = 0;
        title_0c450(&s_buffer, 0x15000, 0);
        s_fade = 0;
        s_phase = 0;
        s_unknown_30 = 0;
        if (((TitleLanguageState *)g_engine_interface.context_004)->byte_00e == 0) {
            s_language_index = 0;
            s_frame_count = -0xf0;
        } else {
            s_language_index = 1;
            s_frame_count = 0;
        }
        title_0c3f0(g_29128, g_language_files[s_language_index], s_buffer, 0x14312);
        title_0c8b0(s_buffer + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
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
