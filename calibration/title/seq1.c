/* Sequence screen 1 unit: handler 0x12280 and helpers 0x12710, 0x12810.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
/* Private .bss (0x2ab1c..0x2ab47, address order): names chosen for meaning and for VC5's
   identifier-hash layout order (scripts/layout_names.py), not recovered identifiers. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"

static int seq1_unknown_2c;
static int seq1_unknown_30;
static int seq1_state;
static int seq1_fade_bottom2;
static int seq1_upper_fade;
static TitleObject *seq1_obj_main;
static TitleObject *seq1_sprite;
static TitleObject *seq1_twin_obj;
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
static int seq1_cel;
static int seq1_anim_phase;
static int seq1_tick_parity;
extern unsigned short g_25f50[];
int title_06670(void);
void title_0c2e0(void);

void title_12280(TitleProc *self)
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
        seq1_state = 0xa;
        seq1_unknown_30 = 0;
        seq1_unknown_2c = 0xf;
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
        seq1_upper_fade = 0x80;
        seq1_fade_bottom2 = 0;
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

    switch (seq1_state) {
    case 0:
        title_01de0(0x140, 0x100, seq1_fade_bottom2, 0, 0);
        if (seq1_fade_bottom2 < 0x80) {
            seq1_fade_bottom2 += 8;
        }
        break;
    case 1:
        seq1_sprite->unknown_054 &= 0x7fffffff;
        seq1_obj_main->unknown_054 &= 0x7fffffff;
        title_01de0(0x140, 0, seq1_upper_fade, 0, 0);
        title_01de0(0x140, 0x100, seq1_fade_bottom2, 1, 1);
        if (seq1_upper_fade < 0x80) {
            seq1_upper_fade += 8;
        }
        if (seq1_fade_bottom2 > 0) {
            seq1_fade_bottom2 -= 8;
        }
        break;
    case 2:
        title_0c4f0(1);
        seq1_state = 3;
        /* fall through */
    case 3:
        title_01de0(0x140, 0, seq1_upper_fade, 0, 0);
        if (seq1_upper_fade > 0) {
            seq1_upper_fade -= 4;
        }
        break;
    case 11:
        if (seq1_obj_main->unknown_04a < 0x80) {
            seq1_obj_main->unknown_04a += 8;
            seq1_twin_obj->unknown_04a = seq1_obj_main->unknown_04a * 2;
        }
        /* fall through */
    case 10:
        title_01de0(0x280, 0x100, 0x80, 0, 0);
        break;
    case 12:
        title_01de0(0x280, 0x100, seq1_upper_fade, 0, 0);
        seq1_sprite->unknown_04a = seq1_upper_fade;
        seq1_obj_main->unknown_04a = seq1_upper_fade;
        seq1_twin_obj->unknown_04a = seq1_upper_fade * 2;
        if (seq1_upper_fade > 0) {
            seq1_upper_fade -= 8;
        } else {
            seq1_sprite->unknown_054 &= 0x7fffffff;
            seq1_obj_main->unknown_054 &= 0x7fffffff;
            seq1_state = 0;
        }
        break;
    }

    if (g_29f98 == 0) {
        title_12810();
    }
    if (seq1_state == 0xc) {
        if (seq1_obj_main != 0) {
            title_09350(seq1_obj_main);
            seq1_obj_main = 0;
        }
        if (seq1_twin_obj != 0) {
            title_09350(seq1_twin_obj);
            seq1_twin_obj = 0;
        }
        if (seq1_sprite != 0) {
            title_09350(seq1_sprite);
            seq1_sprite = 0;
        }
        title_04ea0(4);
        title_16300();
        self->state_04 = 0xfffe;
        self->delay_08 = 2;
        return;
    }
    self->state_04 = 2;
    self->delay_08 = 1;
}

void title_12710(void)
{
    seq1_obj_main = title_17ad0(0, 0, 0, 0x2009, 0);
    seq1_obj_main->unknown_034 = 1;
    seq1_twin_obj = title_17ad0(0, 0, 0, 0x2009, 0);
    seq1_twin_obj->unknown_034 = 1;
    seq1_obj_main->unknown_054 |= 5;
    seq1_twin_obj->unknown_054 |= 6;
    seq1_twin_obj->unknown_04a = 0;
    seq1_obj_main->unknown_04a = 0;
    seq1_twin_obj->unknown_023 = 6;
    seq1_obj_main->unknown_023 = 6;
    seq1_obj_main->unknown_03e = 10;
    seq1_twin_obj->unknown_03e = 20;
    seq1_twin_obj->unknown_000 = 0x10000;
    seq1_twin_obj->unknown_004 = 0x10000;
    seq1_sprite = title_17ad0(0, 0, 0, 0x2009, 0);
    seq1_sprite->unknown_034 = 2;
    seq1_sprite->unknown_000 = 0x640000;
    seq1_sprite->unknown_004 = 0x640000;
    seq1_sprite->unknown_054 |= 0x10;
    seq1_anim_phase = 0;
    seq1_cel = 0;
}

void title_12810(void)
{
    seq1_tick_parity ^= 1;
    if (seq1_tick_parity != 0) {
        return;
    }
    switch (seq1_anim_phase) {
    case 0:
        seq1_sprite->unknown_034 = g_25f50[seq1_cel] + 1;
        seq1_cel++;
        if (g_25f50[seq1_cel] == 0x0a && seq1_state == 0x0a) {
            seq1_state = 0x0b;
        }
        if (g_25f50[seq1_cel] == 0xffff) {
            title_0c4f0(1);
            g_engine_interface.context_004->unknown_1b30 = 4;
            seq1_anim_phase = 1;
        }
        break;
    case 1:
        seq1_sprite->unknown_034++;
        if (seq1_sprite->unknown_034 == 0x10) {
            seq1_anim_phase = 2;
        }
        break;
    case 2:
        seq1_state = 0x0c;
        seq1_anim_phase = 3;
        break;
    }
}
