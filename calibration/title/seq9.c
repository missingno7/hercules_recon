/* Sequence screen 9 unit: handler 0x15cb0 and helpers 0x16110, 0x16230.
   One object: these functions share a private .bss block (scripts/unit_bounds.py), and the
   alphabetical file order seq1, seq10, seq2 .. seq9 is the code order of the ten units. */
/* Private .bss in address order; names chosen for meaning and for VC5's identifier-hash
   layout order (scripts/layout_names.py), not recovered identifiers. */
#include "title_engine.h"
#include "title_files.h"
#include "title_screen.h"

static TitleObject *s9_image;
static TitleObject *s9_obj_twin;
static TitleObject *s9_sprite;
static int s9_anim_state;
static int s9_anim_index;
static int s9_screen_phase;
static int s9_half_rate5;
extern unsigned short g_25f50[];
int title_0c4f0(int a);
void title_016e0(char *text);
void title_0c410(int a, int b);
void title_0c430(int a, int b);
void title_0c890(unsigned short *rect, int value);
void title_0cc70(int a);
void title_0c2f0(void);
void title_0c480(void);
void title_0c5c0(void);
void title_1d4e0(void);
void title_0c8e0(unsigned short *rect, int a, int b);
void title_0c370(int a, int b, int c, int d, int e);
void title_0cc80(void (*callback)(void));
void title_0c320(void);
void title_0c270(void);
void title_0c3d0(char *text, char *name);
void *title_08ed0(int a0, int a1, int a2, int size);
void *title_092e0(int a0, int a1, int a2, int size);
void *title_08a50(int a0, int a1, int a2, int size);
int title_195f0(TitleObject *obj);
int title_19470(void *a, void *b);
void title_04410(void *p);
void title_02dc0(void *p);
void title_0ca40(unsigned short *a, unsigned short *b);
void title_0c7d0(unsigned short *a);
void title_0c790(void);

static int s9_unknown_b4;
static int s9_fade_low;
static int s9_fade_high;
static int s9_field_d4;
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
void title_16110(void);
void title_16230(void);
void title_164b0(void *req);
void title_16300(void);
void title_1d790(int a);
void title_01de0(int a0, int a1, int a2, int a3, int a4);

void title_15cb0(TitleProc *self)
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
        s9_screen_phase = 0x0a;
        s9_field_d4 = 0;
        s9_unknown_b4 = 0x0f;
        handle = 0;
        title_0c450(&handle, 0x15000, 0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0, 0xa0, 0x100);
        title_0c6d0(0);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x140, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c3f0(g_29128, g_sequence_files[TITLE_SEQ_SEQ9], (char *)handle, 0x14312);
        title_01dd0(handle);
        title_0c8b0((char *)handle + 0x312, 2, 0, 0x280, 0x100, 0xa0, 0x100);
        title_0c6d0(0);
        title_0c330(handle);
        title_04b70(0x0d, 0);
        title_16110();
        title_1d790(1);
        s9_fade_high = 0x80;
        s9_fade_low = 0;
        self->state_04 = 1;
        self->delay_08 = 1;
        return;
    case 1:
        title_05e90(title_164b0, 0);
        break;
    case 2:
        switch (s9_screen_phase) {
        case 0:
            title_01de0(0x140, 0x100, s9_fade_low, 0, 0);
            if (s9_fade_low < 0x80) {
                s9_fade_low += 8;
            }
            break;
        case 1:
            title_01de0(0x140, 0, s9_fade_high, 0, 0);
            title_01de0(0x140, 0x100, s9_fade_low, 1, 1);
            if (s9_fade_high < 0x80) {
                s9_fade_high += 8;
            }
            if (s9_fade_low > 0) {
                s9_fade_low -= 8;
            }
            break;
        case 2:
            title_0c4f0(1);
            s9_screen_phase = 3;
            /* fall through */
        case 3:
            title_01de0(0x140, 0, s9_fade_high, 0, 0);
            if (s9_fade_high > 0) {
                s9_fade_high -= 4;
            }
            break;
        case 11:
            if (s9_image->unknown_04a < 0x80) {
                s9_image->unknown_04a += 8;
                s9_obj_twin->unknown_04a = s9_image->unknown_04a * 2;
            }
            /* fall through */
        case 10:
            title_01de0(0x280, 0x100, 0x80, 0, 0);
            break;
        case 12:
            title_01de0(0x280, 0x100, s9_fade_high, 0, 0);
            s9_sprite->unknown_04a = (unsigned short)s9_fade_high;
            s9_image->unknown_04a = (unsigned short)s9_fade_high;
            s9_obj_twin->unknown_04a = (unsigned short)(s9_fade_high * 2);
            if (s9_fade_high > 0) {
                s9_fade_high -= 8;
            } else {
                s9_sprite->unknown_054 &= 0x7fffffff;
                s9_image->unknown_054 &= 0x7fffffff;
                s9_screen_phase = 0;
            }
            break;
        default:
            break;
        }
        title_16230();
        if (s9_screen_phase == 0x0c) {
            if (s9_image != 0) {
                title_09350(s9_image);
                s9_image = 0;
            }
            if (s9_obj_twin != 0) {
                title_09350(s9_obj_twin);
                s9_obj_twin = 0;
            }
            if (s9_sprite != 0) {
                title_09350(s9_sprite);
                s9_sprite = 0;
            }
            title_04ea0(0x0d);
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

void title_16110(void)
{
    s9_image = title_17ad0(0, 0, 0, 0x2013, 0);
    s9_image->unknown_034 = 1;
    s9_image->unknown_054 |= 5;
    s9_image->unknown_04a = 0;

    s9_obj_twin = title_17ad0(0, 0, 0, 0x2013, 0);
    s9_obj_twin->unknown_034 = 1;
    s9_obj_twin->unknown_054 |= 6;
    s9_obj_twin->unknown_04a = 0;

    s9_image->unknown_023 = 6;
    s9_image->unknown_03e = 10;
    s9_obj_twin->unknown_023 = 6;
    s9_obj_twin->unknown_03e = 20;
    s9_image->unknown_000 = 0xfff80000;
    s9_image->unknown_004 = 0xffe80000;
    s9_obj_twin->unknown_000 = 0xfff90000;
    s9_obj_twin->unknown_004 = 0xffe90000;

    s9_sprite = title_17ad0(0, 0, 0, 0x2013, 0);
    s9_sprite->unknown_034 = 2;
    s9_sprite->unknown_054 |= 0x10;
    s9_sprite->unknown_000 = 0x640000;
    s9_sprite->unknown_004 = 0x640000;
    s9_anim_state = 0;
    s9_anim_index = 0;
}

void title_16230(void)
{
    unsigned short code;

    s9_half_rate5 ^= 1;
    if (s9_half_rate5 != 0) {
        return;
    }
    switch (s9_anim_state) {
    case 0:
        break;
    case 1:
        s9_sprite->unknown_034++;
        if (s9_sprite->unknown_034 != 0x10) {
            return;
        }
        s9_anim_state = 2;
        return;
    case 2:
        s9_screen_phase = 0xc;
        s9_anim_state = 3;
        return;
    default:
        return;
    }
    s9_sprite->unknown_034 = g_25f50[s9_anim_index] + 1;
    s9_anim_index++;
    code = g_25f50[s9_anim_index];
    if (code == 0xa && s9_screen_phase == 0xa) {
        s9_screen_phase = 0xb;
    }
    if (code != 0xffff) {
        return;
    }
    title_0c4f0(1);
    g_engine_interface.context_004->unknown_1b30 = 4;
    s9_anim_state = 1;
}
