/* Sequence screen 7 unit: handler 0x14ff0 and helpers 0x15480, 0x15580.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
/* Private .bss in address order; names chosen for meaning and for VC5's identifier-hash
   layout order (scripts/layout_names.py), not recovered identifiers. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"


extern unsigned short g_25f50[];
static TitleObject *s7_picture;
static TitleObject *s7_animated2;
static TitleObject *s7_obj_double;
static int s7_anim_index;
static int s7_anim_state2;
static int s7_state;
static int s7_parity2;
int title_0c4f0(int a);
static int s7_value_602;
static int s7_fade_low2;
static int s7_fade_top;
static int s7_value_7c;
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
int title_0c4f0(int a);
void title_0c6d0(int a);
void title_0c8b0(char *a0, int a1, int a2, int a3, int a4, int a5, int a6);
int title_0c3f0(char *a, char *name, char *c, int d);
void title_15480(void);
void title_15580(void);
void title_15ae0(void);
void title_15be0(void);
void title_164b0(void *req);
void title_16300(void);
void title_1d790(int a);
void title_01de0(int a0, int a1, int a2, int a3, int a4);

void title_14ff0(TitleProc *self)
{
    void *handle;
    short cursor_y = ((ScreenContext *)g_engine_interface.context_004)->cursor_5e;

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
        s7_value_7c = 0;
        s7_value_602 = 0x0f;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ7], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(0x0a, 0);
        title_15480();
        title_1d790(1);
        s7_fade_top = 0x80;
        s7_fade_low2 = 0;
        title_05e90(title_164b0, 0);
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        break;
    case 2:
        switch (s7_state) {
        case 0:
            title_01de0(0x140, 0x100, s7_fade_low2, 0, 0);
            if (s7_fade_low2 < 0x80) {
                s7_fade_low2 += 2;
            }
            break;
        case 1:
            title_01de0(0x140, 0, s7_fade_top, 0, 0);
            title_01de0(0x140, 0x100, s7_fade_low2, 1, 1);
            if (s7_fade_top < 0x80) {
                s7_fade_top += 8;
            } else {
                s7_fade_top = 0x80;
            }
            if (s7_fade_low2 > 0) {
                s7_fade_low2 -= 8;
            } else {
                s7_fade_low2 = 0;
            }
            break;
        case 2:
            title_01de0(0x140, 0, s7_fade_top, 0, 0);
            if (s7_fade_top > 0) {
                s7_fade_top -= 4;
            }
            break;
        case 11:
            if (s7_picture->unknown_04a < 0x80) {
                s7_picture->unknown_04a += 8;
                s7_obj_double->unknown_04a = s7_picture->unknown_04a;
            }
            /* fall through */
        case 10:
            title_01de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_01de0(0x280, 0x100, s7_fade_top, 0, 0);
            s7_animated2->unknown_04a = (unsigned short)s7_fade_top;
            s7_picture->unknown_04a = (unsigned short)s7_fade_top;
            s7_obj_double->unknown_04a = (unsigned short)s7_fade_top;
            if (s7_fade_top > 0) {
                s7_fade_top -= 8;
            } else {
                s7_animated2->unknown_054 &= 0x7fffffff;
                s7_picture->unknown_054 &= 0x7fffffff;
                s7_obj_double->unknown_054 &= 0x7fffffff;
                s7_state = 0;
            }
            break;
        default:
            break;
        }
        if (s7_state == 0x0c) {
            if (s7_animated2 != 0) {
                title_09350(s7_animated2);
                s7_animated2 = 0;
            }
            if (s7_picture != 0) {
                title_09350(s7_picture);
                s7_picture = 0;
            }
            if (s7_obj_double != 0) {
                title_09350(s7_obj_double);
                s7_obj_double = 0;
            }
            title_04ea0(0x0a);
            title_16300();
            self->state_04 = 0xfffe;
            self->delay_08 = 2;
            return;
        }
        if (g_29f98 == 0) {
            title_15580();
        }
        break;
    default:
        return;
    }
    self->state_04 = 2;
    self->delay_08 = 1;
}

void title_15480(void)
{
    s7_state = 10;
    s7_picture = title_17ad0(0, 0, 0, 0x200f, 0);
    s7_picture->unknown_034 = 1;
    s7_picture->unknown_054 |= 5;
    s7_picture->unknown_04a = 0;
    s7_obj_double = title_17ad0(0, 0, 0, 0x200f, 0);
    s7_obj_double->unknown_034 = 1;
    s7_obj_double->unknown_054 |= 6;
    s7_obj_double->unknown_04a = 0;
    s7_picture->unknown_023 = 6;
    s7_picture->unknown_03e = 10;
    s7_obj_double->unknown_023 = 6;
    s7_obj_double->unknown_03e = 20;
    s7_obj_double->unknown_000 = 0x10000;
    s7_obj_double->unknown_004 = 0x10000;
    s7_animated2 = title_17ad0(0, 0, 0, 0x200f, 0);
    s7_animated2->unknown_034 = 2;
    s7_animated2->unknown_000 = 0xff9c0000;
    s7_animated2->unknown_004 = 0x640000;
    s7_anim_state2 = 0;
    s7_anim_index = 0;
}

void title_15580(void)
{
    unsigned short code;

    s7_parity2 ^= 1;
    if (s7_parity2 != 0) {
        return;
    }
    switch (s7_anim_state2) {
    case 0:
        break;
    case 1:
        s7_animated2->unknown_034++;
        if (s7_animated2->unknown_034 != 0x10) {
            return;
        }
        s7_anim_state2 = 2;
        return;
    case 2:
        s7_state = 0xc;
        s7_anim_state2 = 3;
        return;
    default:
        return;
    }
    s7_animated2->unknown_034 = g_25f50[s7_anim_index] + 1;
    s7_anim_index++;
    code = g_25f50[s7_anim_index];
    if (code == 0xa && s7_state == 0xa) {
        s7_state = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    s7_anim_state2 = 1;
}
