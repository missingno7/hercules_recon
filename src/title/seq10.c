/* Sequence screen 10 unit: handler 0x128e0 and helpers 0x12cc0, 0x12dc0.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
/* Private .bss in address order; names chosen for meaning and for VC5's identifier-hash
   layout order (scripts/layout_names.py), not recovered identifiers. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"


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
void title_0c4f0(int a);
void title_14790(void);
void title_14860(void);
void title_164b0(void *req);
void title_16300(void);
void title_1d790(int a);
void title_01de0(int a0, int a1, int a2, int a3, int a4);
static TitleObject *seq10_main_sprite;
static TitleObject *seq10_anim_obj;
static TitleObject *seq10_echo_obj;
static int seq10_value_50;
static int seq10_unknown_5c2;
static int seq10_unk_602;
static int seq10_phase;
static int seq10_out_fade;
static int seq10_top_fade;
extern unsigned short g_25f50[];
void title_12cc0(void);
void title_12dc0(void);
static int seq10_cel;
static int seq10_anim_phase2;
static int seq10_tick_parity2;

void title_128e0(TitleProc *self)
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
        seq10_unknown_5c2 = 0;
        seq10_phase = 0x0a;
        seq10_unk_602 = 0;
        seq10_value_50 = 0x0f;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ10], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(0x0f, 0);
        title_12cc0();
        title_1d790(1);
        seq10_out_fade = 0x80;
        seq10_top_fade = 0;
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        break;
    case 2:
        switch (seq10_phase) {
        case 0:
            title_01de0(0x140, 0, seq10_top_fade, 3, 0);
            title_1d790(0);
            if (seq10_top_fade < 0x80) {
                seq10_top_fade += 8;
            }
            break;
        case 1:
            title_0c4f0(1);
            seq10_phase = 2;
            /* fall through */
        case 2:
            title_01de0(0x140, 0, seq10_top_fade, 3, 0);
            title_1d790(0);
            if (seq10_top_fade > 0) {
                seq10_top_fade -= 8;
            } else {
                seq10_phase = 3;
            }
            break;
        case 11:
            if (seq10_main_sprite->unknown_04a < 0x80) {
                seq10_main_sprite->unknown_04a += 8;
                seq10_echo_obj->unknown_04a = seq10_main_sprite->unknown_04a * 2;
            }
            /* fall through */
        case 10:
            title_01de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_01de0(0x280, 0x100, seq10_out_fade, 0, 0);
            seq10_anim_obj->unknown_04a = (unsigned short)seq10_out_fade;
            seq10_main_sprite->unknown_04a = (unsigned short)seq10_out_fade;
            seq10_echo_obj->unknown_04a = (unsigned short)(seq10_out_fade * 2);
            if (seq10_out_fade > 0) {
                seq10_out_fade -= 8;
            } else {
                seq10_anim_obj->unknown_054 &= 0x7fffffff;
                seq10_main_sprite->unknown_054 &= 0x7fffffff;
                seq10_phase = 0;
            }
            break;
        default:
            break;
        }
        title_12dc0();
        if (seq10_phase == 0x0c) {
            if (seq10_main_sprite != 0) {
                title_09350(seq10_main_sprite);
                seq10_main_sprite = 0;
            }
            if (seq10_echo_obj != 0) {
                title_09350(seq10_echo_obj);
                seq10_echo_obj = 0;
            }
            if (seq10_anim_obj != 0) {
                title_09350(seq10_anim_obj);
                seq10_anim_obj = 0;
            }
            title_04ea0(0x0f);
            title_16300();
            self->state_04 = 0xfffe;
            self->delay_08 = 2;
            return;
        }
        break;
    default:
        return;
    }
    self->state_04 = 2;
    self->delay_08 = 1;
}

void title_12cc0(void)
{
    seq10_main_sprite = title_17ad0(0, 0, 0, 0x2016, 0);
    seq10_main_sprite->unknown_034 = 0x10;
    seq10_main_sprite->unknown_054 |= 5;
    seq10_main_sprite->unknown_04a = 0;
    seq10_echo_obj = title_17ad0(0, 0, 0, 0x2016, 0);
    seq10_echo_obj->unknown_034 = 0x10;
    seq10_echo_obj->unknown_054 |= 6;
    seq10_echo_obj->unknown_04a = 0;
    seq10_main_sprite->unknown_023 = 6;
    seq10_main_sprite->unknown_03e = 0x0a;
    seq10_echo_obj->unknown_023 = 6;
    seq10_echo_obj->unknown_03e = 0x14;
    seq10_echo_obj->unknown_000 = 0x10000;
    seq10_echo_obj->unknown_004 = 0x10000;
    seq10_anim_obj = title_17ad0(0, 0, 0, 0x2016, 0);
    seq10_anim_obj->unknown_034 = 1;
    seq10_anim_obj->unknown_000 = 0xff9c0000;
    seq10_anim_obj->unknown_004 = 0x280000;
    seq10_anim_phase2 = 0;
    seq10_cel = 0;
}

void title_12dc0(void)
{
    seq10_tick_parity2 ^= 1;
    if (seq10_tick_parity2 != 0) {
        return;
    }
    switch (seq10_anim_phase2) {
    case 0:
        seq10_anim_obj->unknown_034 = g_25f50[seq10_cel];
        seq10_cel++;
        if (g_25f50[seq10_cel] == 0x0a && seq10_phase == 0x0a) {
            seq10_phase = 0x0b;
        }
        if (g_25f50[seq10_cel] == 0xffff) {
            title_0c4f0(1);
            g_engine_interface.context_004->unknown_1b30 = 4;
            seq10_anim_phase2 = 1;
        }
        return;
    case 1:
        seq10_anim_obj->unknown_034++;
        if (seq10_anim_obj->unknown_034 == 0x0f) {
            seq10_anim_phase2 = 2;
        }
        return;
    case 2:
        seq10_phase = 0x0c;
        seq10_anim_phase2 = 3;
        return;
    }
}
