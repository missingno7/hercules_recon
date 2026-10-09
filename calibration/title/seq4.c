/* Sequence screen 4 unit: handler 0x13ba0 and helpers 0x14120, 0x14230.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
/* Private .bss in address order; names chosen for meaning and for VC5's identifier-hash
   layout order (scripts/layout_names.py), not recovered identifiers. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"


static TitleObject *seq4_main_obj2;
static int seq4_cel;
static TitleObject *seq4_anim_sprite2;
static int seq4_anim_phase;
static TitleObject *seq4_echo_obj;
static int seq4_stage;
static int seq4_tick_parity2;
extern unsigned short g_25f50[];
int title_0c4f0(int a);
static unsigned int seq4_flash_level;
static unsigned int seq4_upper_fade;
static unsigned int seq4_fade_lo;
static int seq4_var_ec;
static int seq4_unknown_f8;
extern int g_29f98;
extern char g_29128[];
void title_01dd0();
void title_028c0(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
                 int a9, int a10);
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
void title_14120(void);
void title_14230(void);
void title_164b0(void *req);
void title_16300(void);
void title_1d790(int a);
void title_01de0(int a0, int a1, int a2, int a3, int a4);

void title_13ba0(TitleProc *self)
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
        seq4_var_ec = 0;
        seq4_unknown_f8 = 0x0f;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ4], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(7, 0);
        title_14120();
        title_1d790(1);
        seq4_upper_fade = 0x80;
        seq4_fade_lo = 0;
        seq4_flash_level = 0;
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        self->state_04 = 2;
        self->delay_08 = 1;
        return;
    case 2:
        switch (seq4_stage) {
        case 0:
            title_01de0(0x140, 0x100, seq4_fade_lo, 0, 0);
            if (seq4_fade_lo < 0x80) {
                seq4_fade_lo += 8;
            }
            break;
        case 1:
            title_01de0(0x140, 0, 0x80, 0, 0);
            break;
        case 2:
            title_01de0(0x140, 0, seq4_upper_fade, 0, 0);
            title_01de0(0x140, 0x100, seq4_fade_lo, 1, 1);
            if (seq4_upper_fade > 0) {
                seq4_upper_fade -= 4;
            } else {
                seq4_upper_fade = 0;
            }
            if (seq4_fade_lo < 0x80) {
                seq4_fade_lo += 4;
            } else {
                seq4_fade_lo = 0x80;
            }
            break;
        case 3:
            title_01de0(0x140, 0, seq4_upper_fade, 0, 0);
            title_01de0(0x140, 0x100, seq4_fade_lo, 1, 1);
            if (seq4_upper_fade < 0x80) {
                seq4_upper_fade += 8;
            } else {
                seq4_upper_fade = 0x80;
            }
            if (seq4_fade_lo > 0) {
                seq4_fade_lo -= 8;
            } else {
                seq4_fade_lo = 0;
            }
            break;
        case 4:
            title_01de0(0x140, 0, seq4_fade_lo, 0, 0);
            break;
        case 5:
            title_0c4f0(1);
            title_01de0(0x140, 0, seq4_fade_lo, 0, 0);
            if (seq4_fade_lo > 0) {
                seq4_fade_lo -= 4;
            } else {
                seq4_fade_lo = 0;
                seq4_stage = 6;
            }
            break;
        case 11:
            if (seq4_main_obj2->unknown_04a < 0x80) {
                seq4_main_obj2->unknown_04a += 8;
                seq4_echo_obj->unknown_04a = seq4_main_obj2->unknown_04a * 2;
            }
            /* fall through */
        case 10:
            title_01de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_01de0(0x280, 0x100, seq4_upper_fade, 0, 0);
            seq4_anim_sprite2->unknown_04a = (unsigned short)seq4_upper_fade;
            seq4_main_obj2->unknown_04a = (unsigned short)seq4_upper_fade;
            seq4_echo_obj->unknown_04a = (unsigned short)(seq4_upper_fade * 2);
            if (seq4_upper_fade > 0) {
                seq4_upper_fade -= 8;
            } else {
                seq4_anim_sprite2->unknown_054 &= 0x7fffffff;
                seq4_main_obj2->unknown_054 &= 0x7fffffff;
                seq4_stage = 0;
            }
            break;
        default:
            break;
        }
        if (seq4_flash_level > 0) {
            seq4_flash_level -= 8;
            title_028c0(0, 0, 0x140, 0x100, 0xff, 0xff, 0xff, seq4_flash_level, 1, 0, 0x4fe);
        }
        if (seq4_stage == 0x0c) {
            if (seq4_main_obj2 != 0) {
                title_09350(seq4_main_obj2);
                seq4_main_obj2 = 0;
            }
            if (seq4_echo_obj != 0) {
                title_09350(seq4_echo_obj);
                seq4_echo_obj = 0;
            }
            if (seq4_anim_sprite2 != 0) {
                title_09350(seq4_anim_sprite2);
                seq4_anim_sprite2 = 0;
            }
            title_04ea0(7);
            title_16300();
            self->state_04 = 0xfffe;
            self->delay_08 = 2;
            return;
        }
        if (g_29f98 == 0) {
            title_14230();
        }
        break;
    default:
        return;
    }
    self->state_04 = 2;
    self->delay_08 = 1;
}

void title_14120(void)
{
    seq4_stage = 10;
    seq4_main_obj2 = title_17ad0(0, 0, 0, 0x200c, 0);
    seq4_main_obj2->unknown_034 = 1;
    seq4_main_obj2->unknown_054 |= 5;
    seq4_main_obj2->unknown_04a = 0;
    seq4_echo_obj = title_17ad0(0, 0, 0, 0x200c, 0);
    seq4_echo_obj->unknown_034 = 1;
    seq4_echo_obj->unknown_054 |= 6;
    seq4_echo_obj->unknown_04a = 0;
    seq4_main_obj2->unknown_023 = 6;
    seq4_main_obj2->unknown_03e = 10;
    seq4_echo_obj->unknown_023 = 6;
    seq4_echo_obj->unknown_03e = 20;
    seq4_echo_obj->unknown_000 = 0x10000;
    seq4_echo_obj->unknown_004 = 0x10000;
    seq4_anim_sprite2 = title_17ad0(0, 0, 0, 0x200c, 0);
    seq4_anim_sprite2->unknown_034 = 2;
    seq4_anim_sprite2->unknown_054 |= 0x10;
    seq4_anim_sprite2->unknown_000 = 0x640000;
    seq4_anim_sprite2->unknown_004 = 0x280000;
    seq4_anim_phase = 0;
    seq4_cel = 0;
}

void title_14230(void)
{
    unsigned short code;

    seq4_tick_parity2 ^= 1;
    if (seq4_tick_parity2 != 0) {
        return;
    }
    switch (seq4_anim_phase) {
    case 0:
        break;
    case 1:
        seq4_anim_sprite2->unknown_034++;
        if (seq4_anim_sprite2->unknown_034 != 0x10) {
            return;
        }
        seq4_anim_phase = 2;
        return;
    case 2:
        seq4_stage = 0xc;
        seq4_anim_phase = 3;
        return;
    default:
        return;
    }
    seq4_anim_sprite2->unknown_034 = g_25f50[seq4_cel] + 1;
    seq4_cel++;
    code = g_25f50[seq4_cel];
    if (code == 0xa && seq4_stage == 0xa) {
        seq4_stage = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    seq4_anim_phase = 1;
}
