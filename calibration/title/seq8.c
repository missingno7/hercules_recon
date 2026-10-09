/* Sequence screen 8 unit: handler 0x15650 and helpers 0x15ae0, 0x15be0.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
/* Private .bss in address order; names chosen for meaning and for VC5's identifier-hash
   layout order (scripts/layout_names.py), not recovered identifiers. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"


extern unsigned short g_25f50[];
static TitleObject *seq8_main_obj9;
static TitleObject *seq8_animated;
static TitleObject *seq8_twin_obj;
static int seq8_frame;
static int seq8_anim_state;
static int seq8_phase;
static int seq8_odd_tick;
int title_0c4f0(int a);
static int seq8_unknown_94;
static int seq8_upper_fade;
static int seq8_unk_9c2;
static int seq8_fade_low;
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

void title_15650(TitleProc *self)
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
        seq8_unknown_94 = 0;
        seq8_unk_9c2 = 0x0f;
        seq8_phase = 0;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ8], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(0x0b, 0);
        title_15ae0();
        title_1d790(1);
        seq8_upper_fade = 0x80;
        seq8_fade_low = 0;
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        break;
    case 2:
        switch (seq8_phase) {
        case 0:
            title_01de0(0x140, 0x100, seq8_fade_low, 0, 0);
            if (seq8_fade_low < 0x80) {
                seq8_fade_low += 8;
            }
            break;
        case 1:
            title_01de0(0x140, 0, seq8_upper_fade, 0, 0);
            title_01de0(0x140, 0x100, seq8_fade_low, 1, 1);
            if (seq8_upper_fade < 0x80) {
                seq8_upper_fade += 8;
            } else {
                seq8_upper_fade = 0x80;
            }
            if (seq8_fade_low > 0) {
                seq8_fade_low -= 8;
            } else {
                seq8_fade_low = 0;
            }
            break;
        case 2:
            title_0c4f0(1);
            seq8_phase = 3;
            /* fall through */
        case 3:
            title_01de0(0x140, 0, seq8_upper_fade, 0, 0);
            if (seq8_upper_fade > 0) {
                seq8_upper_fade -= 4;
            }
            break;
        case 11:
            if (seq8_main_obj9->unknown_04a < 0x80) {
                seq8_main_obj9->unknown_04a += 8;
            }
            seq8_twin_obj->unknown_04a = (unsigned short)(seq8_main_obj9->unknown_04a * 2);
            /* fall through */
        case 10:
            title_01de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_01de0(0x280, 0x100, seq8_upper_fade, 0, 0);
            seq8_animated->unknown_04a = (unsigned short)seq8_upper_fade;
            seq8_main_obj9->unknown_04a = (unsigned short)seq8_upper_fade;
            seq8_twin_obj->unknown_04a = (unsigned short)(seq8_upper_fade * 2);
            if (seq8_upper_fade > 0) {
                seq8_upper_fade -= 8;
            } else {
                seq8_animated->unknown_054 &= 0x7fffffff;
                seq8_main_obj9->unknown_054 &= 0x7fffffff;
                seq8_phase = 0;
            }
            break;
        default:
            break;
        }
        if (seq8_phase == 0x0c) {
            if (seq8_animated != 0) {
                title_09350(seq8_animated);
                seq8_animated = 0;
            }
            if (seq8_main_obj9 != 0) {
                title_09350(seq8_main_obj9);
                seq8_main_obj9 = 0;
            }
            if (seq8_twin_obj != 0) {
                title_09350(seq8_twin_obj);
                seq8_twin_obj = 0;
            }
            title_04ea0(0x0b);
            title_16300();
            self->state_04 = 0xfffe;
            self->delay_08 = 2;
            return;
        }
        if (g_29f98 == 0) {
            title_15be0();
        }
        break;
    default:
        return;
    }
    self->state_04 = 2;
    self->delay_08 = 1;
}

void title_15ae0(void)
{
    seq8_phase = 10;
    seq8_main_obj9 = title_17ad0(0, 0, 0, 0x2010, 0);
    seq8_main_obj9->unknown_034 = 1;
    seq8_main_obj9->unknown_054 |= 5;
    seq8_main_obj9->unknown_04a = 0;
    seq8_twin_obj = title_17ad0(0, 0, 0, 0x2010, 0);
    seq8_twin_obj->unknown_034 = 1;
    seq8_twin_obj->unknown_054 |= 6;
    seq8_twin_obj->unknown_04a = 0;
    seq8_main_obj9->unknown_023 = 6;
    seq8_main_obj9->unknown_03e = 10;
    seq8_twin_obj->unknown_023 = 6;
    seq8_twin_obj->unknown_03e = 20;
    seq8_twin_obj->unknown_000 = 0x10000;
    seq8_twin_obj->unknown_004 = 0x10000;
    seq8_animated = title_17ad0(0, 0, 0, 0x2010, 0);
    seq8_animated->unknown_034 = 2;
    seq8_animated->unknown_000 = 0xff9c0000;
    seq8_animated->unknown_004 = 0x280000;
    seq8_anim_state = 0;
    seq8_frame = 0;
}

void title_15be0(void)
{
    unsigned short code;

    seq8_odd_tick ^= 1;
    if (seq8_odd_tick != 0) {
        return;
    }
    switch (seq8_anim_state) {
    case 0:
        break;
    case 1:
        seq8_animated->unknown_034++;
        if (seq8_animated->unknown_034 != 0x10) {
            return;
        }
        seq8_anim_state = 2;
        return;
    case 2:
        seq8_phase = 0xc;
        seq8_anim_state = 3;
        return;
    default:
        return;
    }
    seq8_animated->unknown_034 = g_25f50[seq8_frame] + 1;
    seq8_frame++;
    code = g_25f50[seq8_frame];
    if (code == 0xa && seq8_phase == 0xa) {
        seq8_phase = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    seq8_anim_state = 1;
}
