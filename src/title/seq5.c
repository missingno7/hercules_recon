/* Sequence screen 5 unit: handler 0x14300 and helpers 0x14790, 0x14860.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
/* Private .bss in address order; names chosen for meaning and for VC5's identifier-hash
   layout order (scripts/layout_names.py), not recovered identifiers. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"


static TitleObject *s5_base_obj2;
static TitleObject *s5_anim_sprite;
static TitleObject *s5_echo_obj;
static int s5_value_002;
static int s5_fade_low2;
static int s5_screen_phase2;
static int s5_fade_top;
static int s5_var_24;
extern int g_29f98;
extern char g_29128[];
void title_01dd0();
void title_04b70(int c, int d);
void title_04ea0(int a);
void title_05e90(void (*fn)(void *), int n);
void title_05f10(TitleProc *req);
void title_09350(void *block);
void title_0c330(void *p);
void title_0c450(void **out, int size, int flags);
void title_0c6d0(int a);
void title_0c8b0(char *a0, int a1, int a2, int a3, int a4, int a5, int a6);
int title_0c3f0(char *a, char *name, char *c, int d);
int title_0c4f0(int a);
void title_14790(void);
void title_14860(void);
void title_164b0(void *req);
void title_16300(void);
void title_1d790(int a);
void title_01de0(int a0, int a1, int a2, int a3, int a4);
extern unsigned short g_25f50[];
void title_12cc0(void);
void title_12dc0(void);
static int s5_anim_index;
static int s5_anim_mode;
static int s5_parity2;
int title_0c4f0(int a);

void title_14300(TitleProc *self)
{
    void *handle;
    short cursor_x = ((ScreenContext *)g_engine_interface.context_004)->cursor_5c;
    short cursor_y = ((ScreenContext *)g_engine_interface.context_004)->cursor_5e;
    int step = 8;

    switch (self->state_04) {
    case 0xfffd:
        title_05e90(title_164b0, 0);
        title_05f10(self);
        return;
    case 0xfffe:
        title_01dd0();
        self->state_04 = 0xfffd;
        self->delay_08 = 2;
        return;
    case 0xffff:
        s5_var_24 = 0;
        s5_value_002 = 0x0f;
        s5_screen_phase2 = 0;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ5], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(8, 0);
        title_14860();
        title_1d790(1);
        s5_fade_top = 0x80;
        s5_fade_low2 = 0;
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        break;
    case 2:
        switch (s5_screen_phase2) {
        case 0:
            title_01de0(0x140, 0x100, s5_fade_low2, 0, 0);
            if (s5_fade_low2 < 0x80) {
                s5_fade_low2 += step;
            }
            break;
        case 1:
            title_01de0(0x140, 0, s5_fade_top, 0, 0);
            title_01de0(0x140, 0x100, s5_fade_low2, 1, 1);
            if (s5_fade_top < 0x80) {
                s5_fade_top += step;
            } else {
                s5_fade_top = 0x80;
            }
            if (s5_fade_low2 > 0) {
                s5_fade_low2 -= step;
            } else {
                s5_fade_low2 = 0;
            }
            break;
        case 2:
            title_0c4f0(1);
            s5_screen_phase2 = 3;
            /* fall through */
        case 3:
            title_01de0(0x140, 0, s5_fade_top, 0, 0);
            if (s5_fade_top > 0) {
                s5_fade_top -= 4;
            }
            break;
        case 11:
            if (s5_base_obj2->unknown_04a < 0x80) {
                s5_base_obj2->unknown_04a += 8;
                s5_echo_obj->unknown_04a = s5_base_obj2->unknown_04a * 2;
            }
            /* fall through */
        case 10:
            title_01de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_01de0(0x280, 0x100, s5_fade_top, 0, 0);
            s5_anim_sprite->unknown_04a = (unsigned short)s5_fade_top;
            s5_base_obj2->unknown_04a = (unsigned short)s5_fade_top;
            s5_echo_obj->unknown_04a = (unsigned short)(s5_fade_top * 2);
            if (s5_fade_top > 0) {
                s5_fade_top -= step;
            } else {
                s5_anim_sprite->unknown_054 &= 0x7fffffff;
                s5_base_obj2->unknown_054 &= 0x7fffffff;
                s5_screen_phase2 = 0;
            }
            break;
        default:
            break;
        }
        if (s5_screen_phase2 == 0x0c) {
            if (s5_base_obj2 != 0) {
                title_09350(s5_base_obj2);
                s5_base_obj2 = 0;
            }
            if (s5_echo_obj != 0) {
                title_09350(s5_echo_obj);
                s5_echo_obj = 0;
            }
            if (s5_anim_sprite != 0) {
                title_09350(s5_anim_sprite);
                s5_anim_sprite = 0;
            }
            title_04ea0(step);
            title_16300();
            self->state_04 = 0xfffe;
            self->delay_08 = 2;
            return;
        }
        if (g_29f98 == 0) {
            title_14790();
        }
        break;
    default:
        return;
    }
    self->state_04 = 2;
    self->delay_08 = 1;
}

void title_14790(void)
{
    unsigned short code;

    s5_parity2 ^= 1;
    if (s5_parity2 != 0) {
        return;
    }
    switch (s5_anim_mode) {
    case 0:
        break;
    case 1:
        s5_anim_sprite->unknown_034++;
        if (s5_anim_sprite->unknown_034 != 0x10) {
            return;
        }
        s5_anim_mode = 2;
        return;
    case 2:
        s5_screen_phase2 = 0xc;
        s5_anim_mode = 3;
        return;
    default:
        return;
    }
    s5_anim_sprite->unknown_034 = g_25f50[s5_anim_index] + 1;
    s5_anim_index++;
    code = g_25f50[s5_anim_index];
    if (code == 0xa && s5_screen_phase2 == 0xa) {
        s5_screen_phase2 = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    s5_anim_mode = 1;
}

void title_14860(void)
{
    s5_screen_phase2 = 10;
    s5_base_obj2 = title_17ad0(0, 0, 0, 0x200d, 0);
    s5_base_obj2->unknown_034 = 1;
    s5_base_obj2->unknown_054 |= 5;
    s5_base_obj2->unknown_04a = 0;
    s5_echo_obj = title_17ad0(0, 0, 0, 0x200d, 0);
    s5_echo_obj->unknown_034 = 1;
    s5_echo_obj->unknown_054 |= 6;
    s5_echo_obj->unknown_04a = 0;
    s5_base_obj2->unknown_023 = 6;
    s5_base_obj2->unknown_03e = 10;
    s5_echo_obj->unknown_023 = 6;
    s5_echo_obj->unknown_03e = 20;
    s5_base_obj2->unknown_000 = 0;
    s5_base_obj2->unknown_004 = 0xfff00000;
    s5_echo_obj->unknown_000 = 0x10000;
    s5_echo_obj->unknown_004 = 0xfff10000;
    s5_anim_sprite = title_17ad0(0, 0, 0, 0x200d, 0);
    s5_anim_sprite->unknown_034 = 2;
    s5_anim_sprite->unknown_000 = 0xff9c0000;
    s5_anim_sprite->unknown_004 = 0x640000;
    s5_anim_mode = 0;
    s5_anim_index = 0;
}
