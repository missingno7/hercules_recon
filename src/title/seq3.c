/* Sequence screen 3 unit: handler 0x13500 and helpers 0x139d0, 0x13ad0.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
/* Private .bss in address order; names chosen for meaning and for VC5's identifier-hash
   layout order (scripts/layout_names.py), not recovered identifiers. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"


static TitleObject *s3_picture;
static TitleObject *s3_anim_obj;
static TitleObject *s3_obj_double;
static int s3_cel;
static int s3_anim_mode;
static int s3_state2;
static int s3_half_rate;
extern unsigned short g_25f50[];
void title_0c4f0(int a);
static int s3_high_fade;
static int s3_low_fade;
static int s3_var_bc;
static int s3_unknown_c8;
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

void title_13500(TitleProc *self)
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
        s3_unknown_c8 = 0;
        s3_var_bc = 0x0f;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ3], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(6, 0);
        title_139d0();
        title_1d790(1);
        s3_high_fade = 0x80;
        s3_low_fade = 0;
        s3_state2 = 0x0a;
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        break;
    case 2:
        switch (s3_state2) {
        case 0:
            title_01de0(0x140, 0x100, s3_low_fade, 0, 0);
            if (s3_low_fade < 0x80) {
                s3_low_fade += 8;
            }
            break;
        case 1:
            title_01de0(0x140, 0, s3_high_fade, 0, 0);
            title_01de0(0x140, 0x100, s3_low_fade, 1, 1);
            if (s3_high_fade < 0x80) {
                s3_high_fade += 8;
            } else {
                s3_high_fade = 0x80;
            }
            if (s3_low_fade > 0) {
                s3_low_fade -= 8;
            } else {
                s3_low_fade = 0;
            }
            break;
        case 2:
            title_01de0(0x140, 0, s3_high_fade, 0, 0);
            title_01de0(0x140, 0x100, s3_low_fade, 1, 1);
            if (s3_high_fade > 0) {
                s3_high_fade -= 4;
            } else {
                s3_high_fade = 0;
            }
            if (s3_low_fade < 0x80) {
                s3_low_fade += 4;
            } else {
                s3_low_fade = 0x80;
            }
            break;
        case 11:
            if (s3_picture->unknown_04a < 0x80) {
                s3_picture->unknown_04a += 8;
                s3_obj_double->unknown_04a = s3_picture->unknown_04a * 2;
            }
            /* fall through */
        case 10:
            title_01de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_01de0(0x280, 0x100, s3_high_fade, 0, 0);
            s3_anim_obj->unknown_04a = (unsigned short)s3_high_fade;
            s3_picture->unknown_04a = (unsigned short)s3_high_fade;
            s3_obj_double->unknown_04a = (unsigned short)(s3_high_fade * 2);
            if (s3_high_fade > 0) {
                s3_high_fade -= 8;
            } else {
                s3_anim_obj->unknown_054 &= 0x7fffffff;
                s3_picture->unknown_054 &= 0x7fffffff;
                s3_state2 = 0;
            }
            break;
        default:
            break;
        }
        if (s3_state2 == 0x0c) {
            title_02350();
            if (s3_anim_obj != 0) {
                title_09350(s3_anim_obj);
                s3_anim_obj = 0;
            }
            if (s3_picture != 0) {
                title_09350(s3_picture);
                s3_picture = 0;
            }
            if (s3_obj_double != 0) {
                title_09350(s3_obj_double);
                s3_obj_double = 0;
            }
            title_04ea0(6);
            title_16300();
            self->state_04 = 0xfffe;
            self->delay_08 = 2;
            return;
        }
        if (g_29f98 == 0) {
            title_13ad0();
        }
        break;
    default:
        return;
    }
    self->state_04 = 2;
    self->delay_08 = 1;
}

void title_139d0(void)
{
    s3_picture = title_17ad0(0, 0, 0, 0x200b, 0);
    s3_picture->unknown_034 = 1;
    s3_picture->unknown_054 |= 5;
    s3_picture->unknown_04a = 0;
    s3_obj_double = title_17ad0(0, 0, 0, 0x200b, 0);
    s3_obj_double->unknown_034 = 1;
    s3_obj_double->unknown_054 |= 6;
    s3_obj_double->unknown_04a = 0;
    s3_picture->unknown_023 = 6;
    s3_picture->unknown_03e = 0x0a;
    s3_obj_double->unknown_023 = 6;
    s3_obj_double->unknown_03e = 0x14;
    s3_obj_double->unknown_000 = 0x10000;
    s3_obj_double->unknown_004 = 0x10000;
    s3_anim_obj = title_17ad0(0, 0, 0, 0x200b, 0);
    s3_anim_obj->unknown_034 = 2;
    s3_anim_obj->unknown_000 = 0xff9c0000;
    s3_anim_obj->unknown_004 = 0x640000;
    s3_anim_mode = 0;
    s3_cel = 0;
}

void title_13ad0(void)
{
    s3_half_rate ^= 1;
    if (s3_half_rate != 0) {
        return;
    }
    switch (s3_anim_mode) {
    case 0:
        s3_anim_obj->unknown_034 = g_25f50[s3_cel] + 1;
        s3_cel++;
        if (g_25f50[s3_cel] == 0x0a && s3_state2 == 0x0a) {
            s3_state2 = 0x0b;
        }
        if (g_25f50[s3_cel] == 0xffff) {
            title_0c4f0(1);
            g_engine_interface.context_004->unknown_1b30 = 4;
            s3_anim_mode = 1;
        }
        return;
    case 1:
        s3_anim_obj->unknown_034++;
        if (s3_anim_obj->unknown_034 == 0x10) {
            s3_anim_mode = 2;
        }
        return;
    case 2:
        s3_state2 = 0x0c;
        s3_anim_mode = 3;
        return;
    }
}
