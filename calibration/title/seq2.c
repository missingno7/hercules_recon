/* Sequence screen 2 unit: handler 0x12e90 and helpers 0x13310, 0x13430.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
/* Private .bss in address order; names chosen for meaning and for VC5's identifier-hash
   layout order (scripts/layout_names.py), not recovered identifiers. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"


static int seq2_var_88;
static int seq2_fade_top;
static int seq2_unk_90;
static int seq2_lower_fade;
static int seq2_state;
static TitleObject *seq2_picture;
static TitleObject *seq2_anim_sprite;
static TitleObject *seq2_echo_obj;
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
static int seq2_frame;
static int seq2_anim_state;
static int seq2_half_rate;
extern unsigned short g_25f50[];

void title_12e90(TitleProc *self)
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
        seq2_var_88 = 0;
        seq2_unk_90 = 0xf;
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
        seq2_fade_top = 0x80;
        seq2_lower_fade = 0;
        seq2_state = 0xa;
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        self->state_04 = 2;
        self->delay_08 = 1;
        return;
    case 2:
        break;
    default:
        return;
    }

    switch (seq2_state) {
    case 0:
        title_01de0(0x140, 0x100, seq2_lower_fade, 0);
        if (seq2_lower_fade < 0x80) {
            seq2_lower_fade += 8;
        }
        break;
    case 1:
        title_01de0(0x140, 0, seq2_fade_top, 0, 0);
        title_01de0(0x140, 0x100, seq2_lower_fade, 1, 1);
        if (seq2_fade_top < 0x80) {
            seq2_fade_top += 8;
        }
        if (seq2_lower_fade > 0) {
            seq2_lower_fade -= 8;
        }
        break;
    case 2:
        title_0c4f0(1);
        seq2_state = 3;
        /* fall through */
    case 3:
        title_01de0(0x140, 0, seq2_fade_top, 0, 0);
        if (seq2_fade_top > 0) {
            seq2_fade_top -= 4;
        }
        break;
    case 11:
        if (seq2_picture->unknown_04a < 0x80) {
            seq2_picture->unknown_04a += 8;
            seq2_echo_obj->unknown_04a = seq2_picture->unknown_04a * 2;
        }
        /* fall through */
    case 10:
        title_01de0(0x280, 0x100, 0x80, 0, 0);
        break;
    case 12:
        title_01de0(0x280, 0x100, seq2_fade_top, 0, 0);
        seq2_anim_sprite->unknown_04a = seq2_fade_top;
        seq2_picture->unknown_04a = seq2_fade_top;
        seq2_echo_obj->unknown_04a = seq2_fade_top * 2;
        if (seq2_fade_top > 0) {
            seq2_fade_top -= 8;
        } else {
            seq2_anim_sprite->unknown_054 &= 0x7fffffff;
            seq2_picture->unknown_054 &= 0x7fffffff;
            seq2_state = 0;
        }
        break;
    }

    if (seq2_state == 0xc) {
        if (seq2_anim_sprite != 0) {
            title_09350(seq2_anim_sprite);
            seq2_anim_sprite = 0;
        }
        if (seq2_picture != 0) {
            title_09350(seq2_picture);
            seq2_picture = 0;
        }
        if (seq2_echo_obj != 0) {
            title_09350(seq2_echo_obj);
            seq2_echo_obj = 0;
        }
        title_04ea0(5);
        title_16300();
        self->state_04 = 0xfffe;
        self->delay_08 = 2;
        return;
    }
    if (g_29f98 == 0) {
        title_13430();
    }
    self->state_04 = 2;
    self->delay_08 = 1;
}

void title_13310(void)
{
    seq2_picture = title_17ad0(0, 0, 0, 0x200a, 0);
    seq2_picture->unknown_034 = 0x10;
    seq2_picture->unknown_054 |= 5;
    seq2_picture->unknown_04a = 0;
    seq2_echo_obj = title_17ad0(0, 0, 0, 0x200a, 0);
    seq2_echo_obj->unknown_034 = 0x10;
    seq2_echo_obj->unknown_054 |= 6;
    seq2_echo_obj->unknown_04a = 0;
    seq2_picture->unknown_023 = 6;
    seq2_picture->unknown_03e = 0x0a;
    seq2_echo_obj->unknown_023 = 6;
    seq2_echo_obj->unknown_03e = 0x14;
    seq2_picture->unknown_000 = 0xfff60000;
    seq2_picture->unknown_004 = 0xffe80000;
    seq2_echo_obj->unknown_000 = 0xfff70000;
    seq2_echo_obj->unknown_004 = 0xffe90000;
    seq2_anim_sprite = title_17ad0(0, 0, 0, 0x200a, 0);
    seq2_anim_sprite->unknown_034 = 1;
    seq2_anim_sprite->unknown_054 |= 0x10;
    seq2_anim_sprite->unknown_000 = 0x640000;
    seq2_anim_sprite->unknown_004 = 0x640000;
    seq2_anim_state = 0;
    seq2_frame = 0;
}

void title_13430(void)
{
    seq2_half_rate ^= 1;
    if (seq2_half_rate != 0) {
        return;
    }
    switch (seq2_anim_state) {
    case 0:
        seq2_anim_sprite->unknown_034 = g_25f50[seq2_frame];
        seq2_frame++;
        if (g_25f50[seq2_frame] == 0x0a && seq2_state == 0x0a) {
            seq2_state = 0x0b;
        }
        if (g_25f50[seq2_frame] == 0xffff) {
            title_0c4f0(1);
            g_engine_interface.context_004->unknown_1b30 = 4;
            seq2_anim_state = 1;
        }
        return;
    case 1:
        seq2_anim_sprite->unknown_034++;
        if (seq2_anim_sprite->unknown_034 == 0x0f) {
            seq2_anim_state = 2;
        }
        return;
    case 2:
        seq2_state = 0x0c;
        seq2_anim_state = 3;
        return;
    }
}
