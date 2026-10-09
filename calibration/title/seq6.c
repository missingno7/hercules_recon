/* Sequence screen 6 unit: handler 0x14970 and helpers 0x14e10, 0x14f20.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
/* Private .bss in address order; names chosen for meaning and for VC5's identifier-hash
   layout order (scripts/layout_names.py), not recovered identifiers. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"


static TitleObject *seq6_main_obj;
static TitleObject *seq6_anim_sprite;
static TitleObject *seq6_twin_obj;
static int seq6_field_3c;
static int seq6_top_fade7;
static int seq6_fade_bottom;
static int seq6_phase;
static int seq6_unknown_50;
extern int g_29f98;
extern char g_29128[];
void title_01dd0();
void title_02350(void);
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
void title_139d0(void);
void title_13ad0(void);
void title_14e10(void);
void title_14f20(void);
void title_164b0(void *req);
void title_16300(void);
void title_1d790(int a);
void title_01de0(int a0, int a1, int a2, int a3, int a4);
extern unsigned short g_25f50[];
static int seq6_frame_pos;
static int seq6_anim_state2;
static int seq6_tick_parity;
int title_0c4f0(int a);

void title_14970(TitleProc *self)
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
        seq6_field_3c = 0;
        seq6_unknown_50 = 0x0f;
        seq6_phase = 0;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ6], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(9, 0);
        title_14e10();
        title_1d790(1);
        seq6_top_fade7 = 0x80;
        seq6_fade_bottom = 0;
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        break;
    case 2:
        switch (seq6_phase) {
        case 0:
            title_01de0(0x140, 0x100, seq6_fade_bottom, 0, 0);
            if (seq6_fade_bottom < 0x80) {
                seq6_fade_bottom += 8;
            }
            break;
        case 1:
            title_01de0(0x140, 0, seq6_top_fade7, 0, 0);
            title_01de0(0x140, 0x100, seq6_fade_bottom, 1, 1);
            if (seq6_top_fade7 < 0x80) {
                seq6_top_fade7 += 8;
            } else {
                seq6_top_fade7 = 0x80;
            }
            if (seq6_fade_bottom > 0) {
                seq6_fade_bottom -= 8;
            } else {
                seq6_fade_bottom = 0;
            }
            break;
        case 2:
            title_0c4f0(1);
            seq6_phase = 3;
            /* fall through */
        case 3:
            title_01de0(0x140, 0, seq6_top_fade7, 0, 0);
            if (seq6_top_fade7 > 0) {
                seq6_top_fade7 -= 4;
            }
            break;
        case 11:
            if (seq6_main_obj->unknown_04a < 0x80) {
                seq6_main_obj->unknown_04a += 8;
            }
            seq6_twin_obj->unknown_04a = seq6_main_obj->unknown_04a * 2;
            /* fall through */
        case 10:
            title_01de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_01de0(0x280, 0x100, seq6_top_fade7, 0, 0);
            seq6_anim_sprite->unknown_04a = (unsigned short)seq6_top_fade7;
            seq6_main_obj->unknown_04a = (unsigned short)seq6_top_fade7;
            seq6_twin_obj->unknown_04a = (unsigned short)(seq6_top_fade7 * 2);
            if (seq6_top_fade7 > 0) {
                seq6_top_fade7 -= 8;
            } else {
                seq6_anim_sprite->unknown_054 &= 0x7fffffff;
                seq6_main_obj->unknown_054 &= 0x7fffffff;
                seq6_twin_obj->unknown_054 &= 0x7fffffff;
                seq6_phase = 0;
            }
            break;
        default:
            break;
        }
        if (seq6_phase == 0x0c) {
            if (seq6_anim_sprite != 0) {
                title_09350(seq6_anim_sprite);
                seq6_anim_sprite = 0;
            }
            if (seq6_main_obj != 0) {
                title_09350(seq6_main_obj);
                seq6_main_obj = 0;
            }
            if (seq6_twin_obj != 0) {
                title_09350(seq6_twin_obj);
                seq6_twin_obj = 0;
            }
            title_04ea0(9);
            title_16300();
            self->state_04 = 0xfffe;
            self->delay_08 = 2;
            return;
        }
        if (g_29f98 == 0) {
            title_14f20();
        }
        break;
    default:
        return;
    }
    self->state_04 = 2;
    self->delay_08 = 1;
}

void title_14e10(void)
{
    seq6_phase = 10;
    seq6_main_obj = title_17ad0(0, 0, 0, 0x200e, 0);
    seq6_main_obj->unknown_034 = 1;
    seq6_main_obj->unknown_054 |= 5;
    seq6_main_obj->unknown_04a = 0;
    seq6_twin_obj = title_17ad0(0, 0, 0, 0x200e, 0);
    seq6_twin_obj->unknown_034 = 1;
    seq6_twin_obj->unknown_054 |= 6;
    seq6_twin_obj->unknown_04a = 0;
    seq6_main_obj->unknown_023 = 6;
    seq6_main_obj->unknown_03e = 10;
    seq6_twin_obj->unknown_023 = 6;
    seq6_twin_obj->unknown_03e = 20;
    seq6_main_obj->unknown_000 = 0;
    seq6_main_obj->unknown_004 = 0xfff00000;
    seq6_twin_obj->unknown_000 = 0x10000;
    seq6_twin_obj->unknown_004 = 0xfff10000;
    seq6_anim_sprite = title_17ad0(0, 0, 0, 0x200e, 0);
    seq6_anim_sprite->unknown_034 = 2;
    seq6_anim_sprite->unknown_000 = 0xff9c0000;
    seq6_anim_sprite->unknown_004 = 0x640000;
    seq6_anim_state2 = 0;
    seq6_frame_pos = 0;
}

void title_14f20(void)
{
    unsigned short code;

    seq6_tick_parity ^= 1;
    if (seq6_tick_parity != 0) {
        return;
    }
    switch (seq6_anim_state2) {
    case 0:
        break;
    case 1:
        seq6_anim_sprite->unknown_034++;
        if (seq6_anim_sprite->unknown_034 != 0x10) {
            return;
        }
        seq6_anim_state2 = 2;
        return;
    case 2:
        seq6_phase = 0xc;
        seq6_anim_state2 = 3;
        return;
    default:
        return;
    }
    seq6_anim_sprite->unknown_034 = g_25f50[seq6_frame_pos] + 1;
    seq6_frame_pos++;
    code = g_25f50[seq6_frame_pos];
    if (code == 0xa && seq6_phase == 0xa) {
        seq6_phase = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    seq6_anim_state2 = 1;
}
