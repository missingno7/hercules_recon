/* ENG1.DLL 0x32b10..0x34000: status panel (stpanel1) setup/draw, sprite/quad command writers and the
 * following level actor handlers (lane x8 scratch, 2026-10-20). Separate C unit: its .bss (0x70800..0x7083c)
 * and callees are disjoint from the renderer unit that ends at 0x32b10. ENG3 counterpart code lives in
 * calibration/eng3/handlers_25040.c (0x25060 panel load, 0x250f0 HUD draw, 0x25620 sprite writer differing
 * only in the frame-buffer OT slot displacement). <stdlib.h> is included for its real use (abs in 0x33990);
 * with a bare abs prototype instead, 0x33f30 is 40 bytes off and 0x33830 32 (context-sensitive).
 * The panel state 0x70820..0x7083b is one aggregate: aligned members (0x70820, 0x70828, 0x7082c) are read with
 * dword loads + mask, unaligned ones (0x70829, 0x7082e) with byte/word loads. */
#include <stdlib.h>

typedef struct Rect16 {
    short x, y, w, h;
} Rect16;

typedef struct PanelContext {       /* view of g_engine_interface.context_004 */
    unsigned char unknown_000[3];
    unsigned char state_003;
    unsigned char paused_004;
    unsigned char unknown_005;
    unsigned char mode_006;
    unsigned char unknown_007[0x1e - 0x07];
    short target_01e;
    short value_020;
    short value_022;
    short target_024;
} PanelContext;

typedef struct EngineInterfaceView {
    unsigned long signature_000;
    PanelContext *context_004;
} EngineInterfaceView;

typedef struct PanelSprite {        /* PSX SPRT */
    unsigned long tag;
    unsigned char r0, g0, b0, code;
    short x0, y0;
    unsigned char u0, v0;
    unsigned short clut;
    short w, h;
} PanelSprite;

typedef struct PanelQuad {          /* PSX POLY_FT4 */
    unsigned long tag;
    unsigned long rgbc;
    unsigned long xy0;
    unsigned short uv0;
    unsigned short clut;
    unsigned long xy1;
    unsigned short uv1;
    unsigned short tpage;
    unsigned long xy2;
    unsigned short uv2;
    unsigned short pad2;
    unsigned long xy3;
    unsigned short uv3;
    unsigned short pad3;
} PanelQuad;

typedef struct PanelPacket {        /* 12-byte draw-mode packet */
    unsigned long tag;
    unsigned long code[2];
} PanelPacket;

typedef struct PanelFrame {         /* ENG1 display buffer: ordering table slot used by the panel */
    unsigned char unknown_0000[0x1374];
    unsigned long ot_1374;
} PanelFrame;

extern EngineInterfaceView g_engine_interface;
extern PanelFrame *g_dispatch_frame_current_71ec4;
extern PanelPacket *g_frame_global_73484;
extern unsigned long g_frame_global_73488;

typedef struct PanelState {          /* 0x70820, 0x1c bytes */
    unsigned short value_00;
    unsigned short value_02;
    unsigned short shown_04;
    unsigned short shown_06;
    unsigned char rate_08;
    unsigned char rate_09;
    unsigned char rate_0a;
    unsigned char unknown_0b;
    unsigned short flash_0c;
    unsigned short flash_0e;
    short level_10;
    short target_12;                    /* 0x70832: g_native_word_70832 in the boss unit */
    unsigned char mode_14;
    unsigned char unknown_15;
    short fade_16;
    short x_18;
    short y_1a;
} PanelState;
extern PanelState g_panel_70820;
extern int g_context_sequence_index;
extern unsigned char g_frame_coordinate_prefix_5dd80[][2];
extern unsigned long g_frame_color_state_prefix_6ffc8[2];   /* two semi-transparent pass colours */
extern unsigned short g_frame_global_6ffd0;     /* fade level */
extern char g_panel_prefix_6ffc0[];
extern char *g_panel_file_4da18;
extern Rect16 g_stpanel1_upload_rectangle_words;

void *engine_pool_allocate(void **output, unsigned long bytes, unsigned long flags);
unsigned long engine_file_load_sync(char *prefix, char *name, void *destination, unsigned long bytes);
void dispatch_engine_2c7f0(Rect16 *rect, void *texels);
void dispatch_engine_2c630(unsigned long value);
void dispatch_engine_2c2d0(void *allocation);
void dispatch_local_2c60(Rect16 *rect, int x, int y);
void dispatch_local_1f40(void);
void dispatch_engine_2c980(void *p, int dfe, int dtd, int tpage, int tw);
unsigned short dispatch_engine_2c7c0(int x, int y);
void eng1_2ca10(void *p);
void eng1_2ca20(void *p, int on);
void dispatch_local_33420(int x, int y, unsigned char u, unsigned char v, int w, int h, int mode);
void dispatch_local_33620(int x, int y, int u, int v, int w, int h, int mode);

void dispatch_local_32b10(void)
{
    void *buffer;

    g_panel_70820.shown_04 = 0xffff;
    g_panel_70820.shown_06 = 0xffff;
    g_panel_70820.x_18 = 0xd0;
    g_panel_70820.y_1a = 0xd0;
    g_panel_70820.rate_08 = 2;
    g_panel_70820.rate_0a = 2;
    buffer = 0;
    g_panel_70820.value_00 = 0;
    g_panel_70820.value_02 = 0;
    g_panel_70820.level_10 = 0;
    g_panel_70820.target_12 = 0;
    g_panel_70820.mode_14 = 0;
    g_panel_70820.rate_09 = 3;
    engine_pool_allocate(&buffer, 0x21000, 0);
    engine_file_load_sync(g_panel_prefix_6ffc0, g_panel_file_4da18, buffer, 0x21000);
    dispatch_engine_2c7f0(&g_stpanel1_upload_rectangle_words, buffer);
    dispatch_engine_2c630(0);
    dispatch_engine_2c2d0(buffer);
}

void dispatch_local_32bd0(void)
{
    Rect16 rect;
    int n;
    int i;
    int height;
    int level;
    int t;
    int w;
    int q, r, k;
    int start;
    unsigned char c;

    if (g_engine_interface.context_004->mode_006 == 3) {
        n = (g_context_sequence_index + 30) / 60;
        dispatch_local_33620(0x18, 0xd0, 0xf0, (n / 10) * 16 + 0x28, 0x10, 0x10, 3);
        dispatch_local_33620(0x25, 0xd0, 0xf0, (n % 10) * 16 + 0x28, 0x10, 0x10, 3);
        if (--g_context_sequence_index < 0) {
            g_engine_interface.context_004->state_003 = 0x13;
        }
    }
    if (g_engine_interface.context_004->paused_004 == 0) {
        for (i = 0; i < g_panel_70820.rate_08; i++) {
            if (g_engine_interface.context_004->value_022 < g_engine_interface.context_004->target_024) {
                g_engine_interface.context_004->value_022++;
            } else if (g_engine_interface.context_004->value_022 > g_engine_interface.context_004->target_024) {
                g_engine_interface.context_004->value_022--;
            }
        }
        for (i = 0; i < g_panel_70820.rate_09; i++) {
            if (g_engine_interface.context_004->value_020 < g_engine_interface.context_004->target_01e) {
                g_engine_interface.context_004->value_020++;
            } else if (g_engine_interface.context_004->value_020 > g_engine_interface.context_004->target_01e) {
                g_engine_interface.context_004->value_020--;
            }
        }
        for (i = 0; i < g_panel_70820.rate_0a; i++) {
            if (g_panel_70820.level_10 < g_panel_70820.target_12) {
                g_panel_70820.level_10++;
            } else if (g_panel_70820.level_10 > g_panel_70820.target_12) {
                g_panel_70820.level_10--;
            }
        }
    }
    rect.w = 0xc;
    rect.h = 0x18;
    if (g_panel_70820.value_00 != g_panel_70820.shown_04) {
        g_panel_70820.shown_04 = g_panel_70820.value_00;
        rect.x = g_frame_coordinate_prefix_5dd80[g_panel_70820.value_00][0] * 12;
        rect.y = g_frame_coordinate_prefix_5dd80[g_panel_70820.value_00][1] * 24 + 0x238;
        dispatch_local_2c60(&rect, 0, 0x238);
    }
    if (g_panel_70820.value_02 != g_panel_70820.shown_06) {
        g_panel_70820.shown_06 = g_panel_70820.value_02;
        rect.x = g_frame_coordinate_prefix_5dd80[g_panel_70820.value_02][0] * 12;
        rect.y = g_frame_coordinate_prefix_5dd80[g_panel_70820.value_02][1] * 24 + 0x238;
        dispatch_local_2c60(&rect, 0xc, 0x238);
    }
    height = g_engine_interface.context_004->value_020 * 146 / 256;
    if (height < 0x1c) {
        height = 0x1c;
    } else if (height > 0xff) {
        height = 0xff;
    }
    level = g_engine_interface.context_004->value_022 * 146 / 256;
    if (level < 0) {
        level = 0;
    } else if (level > height - 1) {
        level = height - 1;
    }
    if (g_panel_70820.flash_0c != 0) {
        t = 18 - ((g_panel_70820.flash_0c * 18) >> 8);
        dispatch_local_33620(0x116, t + 0x15, 0xc8, t, 0x18, 18 - t, 1);
    }
    if (g_panel_70820.flash_0e != 0) {
        t = 20 - ((g_panel_70820.flash_0e * 20) >> 8);
        dispatch_local_33620(0x3f, t + 0x25, 0xd0, t + 0x12, 0x18, 20 - t, 1);
    }
    dispatch_local_33620(0xec, 0x14, 0x88, 0, 0x40, 0x1f, 0);
    dispatch_local_33620(height + 0x41, 0x17, 0xc8, 0x50, 0x24, 0x10, 0);
    dispatch_local_33620(0x10, 0x14, 0, 0, 0x50, 0x25, 0);
    dispatch_local_33620(0x60, 0x24, 0xc0, 0x20, 0x10, 5, 0);
    dispatch_local_33620(0x2b, 0x17, 0, 0x38, 0x18, 0x18, 2);
    dispatch_local_33620(0x103, 0x17, 0x18, 0x38, 0x18, 0x18, 2);
    w = height - 12;
    for (i = 0; i < w / 64; i++) {
        dispatch_local_33620(i * 64 + 0x5c, 0x1a, 0x18, 0x28, 0x40, 0xd, 0);
    }
    dispatch_local_33620(i * 64 + 0x5c, 0x1a, 0x18, 0x28, w & 0x3f, 0xd, 0);
    r = level % 24;
    q = level / 24;
    for (i = 0; i < q; i++) {
        dispatch_local_33620(i * 24 + 0x52, 0x1d, 0, 0x28, 0x18, 6, 1);
    }
    dispatch_local_33620(i * 24 + 0x52, 0x1d, 0, 0x28, r, 6, 1);
    k = height / 24 - q;
    if (k == 0) {
        height -= level;
        dispatch_local_33620(level + 0x52, 0x1d, r, 0x30, height, 6, 1);
    } else {
        dispatch_local_33620(level + 0x52, 0x1d, r, 0x30, 0x18 - r, 6, 1);
    }
    start = (q + 1) * 24;
    for (i = 0; i < k - 1; i++) {
        dispatch_local_33620(start + i * 24 + 0x52, 0x1d, 0, 0x30, 0x18, 6, 1);
    }
    w = height - start - i * 24;
    if (w > 0) {
        dispatch_local_33620(start + i * 24 + 0x52, 0x1d, 0, 0x30, w, 6, 1);
    }
    if (g_panel_70820.mode_14 != 0) {
        if (g_panel_70820.mode_14 == 2) {
            if (g_panel_70820.fade_16 < 0) {
                g_panel_70820.fade_16 = 0;
            } else if (g_panel_70820.fade_16 > 0x80) {
                g_panel_70820.fade_16 = 0x80;
            }
            g_frame_global_6ffd0 = g_panel_70820.fade_16;
            c = 0x80 - (0x80 - g_frame_global_6ffd0) * (0x80 - g_frame_global_6ffd0) / 0x80;
            g_frame_color_state_prefix_6ffc8[0] = ((c << 8 | c) << 8) | c;
            c = g_frame_global_6ffd0 * g_frame_global_6ffd0 / 0x80;
            g_frame_color_state_prefix_6ffc8[1] = ((c << 8 | c) << 8) | c;
        }
        t = g_panel_70820.level_10 * 64 / 256;
        if (t < 0) {
            t = 0;
        } else if (t > 0x3f) {
            t = 0x3f;
        }
        dispatch_local_33420(g_panel_70820.x_18 - 0x14, g_panel_70820.y_1a, 0xa0, 0x50, 0x24, 0x10, 0);
        dispatch_local_33420(g_panel_70820.x_18 + 0x30, g_panel_70820.y_1a, 0xc8, 0x50, 0x24, 0x10, 0);
        for (i = 0; i < 0x40; i += 0x40) {
            dispatch_local_33420(g_panel_70820.x_18 + i, g_panel_70820.y_1a + 3, 0x18, 0x28, 0x40, 0xd, 0);
        }
        r = t % 24;
        q = t / 24;
        for (i = 0; i < q; i++) {
            dispatch_local_33420(g_panel_70820.x_18 + i * 24, g_panel_70820.y_1a + 6, 0, 0x28, 0x18, 6, 1);
        }
        dispatch_local_33420(g_panel_70820.x_18 + i * 24, g_panel_70820.y_1a + 6, 0, 0x28, r, 6, 1);
        dispatch_local_33420(g_panel_70820.x_18 + t, g_panel_70820.y_1a + 6, r, 0x30, 0x18 - r, 6, 1);
        start = (q + 1) * 24;
        k = (0x40 - t - (0x18 - r)) / 24;
        for (i = 0; i < k; i++) {
            dispatch_local_33420(g_panel_70820.x_18 + start + i * 24, g_panel_70820.y_1a + 6, 0, 0x30, 0x18, 6, 1);
        }
        w = 0x40 - start - i * 24;
        if (w > 0) {
            dispatch_local_33420(g_panel_70820.x_18 + start + i * 24, g_panel_70820.y_1a + 6, 0, 0x30, w, 6, 1);
        }
    }
    dispatch_local_1f40();
    dispatch_engine_2c980(g_frame_global_73484, 0, 0, 0x180, 0);
    g_frame_global_73484->tag = g_dispatch_frame_current_71ec4->ot_1374;
    g_dispatch_frame_current_71ec4->ot_1374 = (unsigned long)g_frame_global_73484;
    g_frame_global_73484++;
}

void dispatch_local_33420(int x, int y, unsigned char u, unsigned char v, int w, int h, int mode)
{
    PanelQuad *p;
    int blend;
    int clut;
    unsigned long xy0, xy1, xy2, xy3;
    unsigned short uv0, uv1, uv2, uv3;
    unsigned short u0, u1, v0;
    unsigned short tpage;
    int i;

    p = (PanelQuad *)g_frame_global_73484;
    blend = 0;
    if (g_panel_70820.mode_14 == 2) {
        if (g_frame_global_6ffd0 == 0) {
            return;
        }
        if (g_frame_global_6ffd0 < 0x80) {
            blend = 1;
        }
    }
    if (!blend) {
        dispatch_local_33620(x, y, u, v, w, h, mode);
        return;
    }
    clut = dispatch_engine_2c7c0(0x160, mode + 0x20);
    xy0 = (x & 0xffff) + (y << 16);
    xy1 = ((x + w) & 0xffff) + (y << 16);
    xy2 = (x & 0xffff) + ((y + h) << 16);
    xy3 = ((x + w) & 0xffff) + ((y + h) << 16);
    u0 = u;
    v0 = v;
    u1 = (unsigned char)(u + w);
    uv0 = u0 + (v0 << 8);
    uv1 = u1 + (v0 << 8);
    uv2 = u0 + ((v0 + h) << 8);
    uv3 = u1 + ((v0 + h) << 8);
    tpage = ((v0 & 0x200) | 0x60) << 2;
    for (i = 0; i < 2; i++) {
        p->clut = clut;
        p->rgbc = g_frame_color_state_prefix_6ffc8[i];
        p->xy0 = xy0;
        p->xy1 = xy1;
        p->xy2 = xy2;
        p->xy3 = xy3;
        p->uv0 = uv0;
        p->uv1 = uv1;
        p->uv2 = uv2;
        p->uv3 = uv3;
        eng1_2ca10(p);
        eng1_2ca20(p, 1);
        p->tpage = (((i + 1) << 5) & 0x60) | tpage;
        p->tag = g_dispatch_frame_current_71ec4->ot_1374;
        g_dispatch_frame_current_71ec4->ot_1374 = (unsigned long)p;
        p++;
        clut = dispatch_engine_2c7c0(0x160, 0x26);
    }
    g_frame_global_73484 = (PanelPacket *)p;
}

void dispatch_local_33620(int x, int y, int u, int v, int w, int h, int mode)
{
    PanelSprite *s;

    s = (PanelSprite *)g_frame_global_73484;
    s->clut = dispatch_engine_2c7c0(0x160, mode + 0x20);
    s->x0 = x;
    s->y0 = y;
    s->u0 = u;
    s->v0 = v;
    s->w = w;
    s->h = h;
    *(unsigned long *)&s->r0 = g_frame_global_73488;
    s->code = 0x64;
    if (mode == 4) {
        s->code = 0x66;
    }
    s->tag = g_dispatch_frame_current_71ec4->ot_1374;
    g_dispatch_frame_current_71ec4->ot_1374 = (unsigned long)s;
    g_frame_global_73484 = (PanelPacket *)(s + 1);
    if (mode == 4) {
        dispatch_engine_2c980(g_frame_global_73484, 0, 0, 0x180, 0);
        g_frame_global_73484->tag = g_dispatch_frame_current_71ec4->ot_1374;
        g_dispatch_frame_current_71ec4->ot_1374 = (unsigned long)g_frame_global_73484;
        g_frame_global_73484++;
    }
}

/* ---- level actor handlers ---- */

typedef struct Actor Actor;
typedef struct ActorEntry {         /* 16-byte attached entry */
    unsigned char unknown_00[0x0c];
    unsigned short sprite;
    unsigned short tag;
} ActorEntry;
typedef struct Bounds {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds;
struct Actor {
    long x, y, z;
    unsigned char unknown_00c[0x1f - 0x0c];
    unsigned char status_01f;
    unsigned char unknown_020[0x22 - 0x20];
    unsigned char category;
    unsigned char mode;
    unsigned char unknown_024;
    unsigned char control_025;
    unsigned char unknown_026[0x2a - 0x26];
    unsigned short state;
    unsigned char unknown_02c[0x34 - 0x2c];
    unsigned short sprite;
    unsigned short entry_count;
    unsigned short profile;
    unsigned char unknown_03a[0x4a - 0x3a];
    unsigned short unknown_04a;
    long parameter;
    unsigned long flags;
    unsigned long render_flags;
    unsigned char unknown_058[0x5c - 0x58];
    ActorEntry *entries;
    unsigned char unknown_060[0x64 - 0x60];
    void *image_064;
    unsigned char unknown_068[0x74 - 0x68];
    short angle_74;
    unsigned char unknown_076[0x88 - 0x76];
    Bounds bounds;
    unsigned char direction_x;
    unsigned char direction_y;
    unsigned char direction_z;
    unsigned char unknown_097;
    unsigned short unknown_098;
    short delay;
    long saved_x, saved_y;
    unsigned char unknown_0a4[0xee - 0xa4];
    short hits;
    unsigned char unknown_0f0[0xf4 - 0xf0];
    long unknown_0f4;
    unsigned char unknown_0f8[0x114 - 0xf8];
    short unknown_114;
    unsigned char unknown_116[0x11e - 0x116];
    short unknown_11e;
    unsigned char unknown_120[0x124 - 0x120];
    short unknown_124;
    unsigned char unknown_126[0x12c - 0x126];
    Actor *link_12c;
};

extern short g_level_timer_70800;
extern unsigned short g_level_toggle_70802;
extern unsigned short *g_spatial_sound_words_71714;
extern unsigned short g_level_sound_words_60268[];
extern long g_eng1_setup_dword_72188;
extern unsigned short g_boss_state_6ffda;
extern Actor *g_native_pointer_7217c;
extern long g_level_actors_6fe78[];
extern unsigned short *g_configuration_dword_713bc;
extern void *g_level_images_5e678[];
extern unsigned short g_level_word_pool_554f0[];
extern unsigned char g_level_entries_5e688[];

unsigned int random_limit(int limit);
Actor *spawn_at_group_300c0(long x, long y, long z, int kind, int group);
void spawn_actor_with_owner(Actor *owner, long x, long y, long z, int kind);
Actor *action_spawn_27d40(Actor *actor, unsigned long action);
void play_sound(int id, Actor *p);
void reset_frame_state(Actor *p);
void advance_frame(Actor *p);
void restart_steps(Actor *p);
void select_frame(Actor *p, int frame);
void apply_frame_motion_2d(Actor *p);
void eng1_interpret_word_stream(Actor *p);
void eng1_interpret_signed_pairs(Actor *p);
int eng1_29c20(Actor *p);
unsigned long dispatch_local_2a170(long x, long y, long z, int step);
void eng1_native_code_target_002a290(Actor *p);
int eng1_2a460(Bounds *b, Actor *p);
int eng1_2a9e0(Actor *p, int range);
int remove_actor(Actor *p);
long eng1_native_code_target_00266a0(Actor *p, int a, int b);
void attach_entries(Actor *p, void *entries);

void eng1_33700(void)
{
    g_level_toggle_70802 = 0;
    g_level_timer_70800 = -1;
    g_spatial_sound_words_71714 = g_level_sound_words_60268;
}

void eng1_33720(void)
{
    Actor *player;

    if (--g_level_timer_70800 < 0) {
        if (g_level_toggle_70802 != 0) {
            g_level_toggle_70802 = 0;
            g_level_timer_70800 = random_limit(30) + 0xb4;
        } else {
            g_level_toggle_70802 = 1;
            g_level_timer_70800 = random_limit(30) + 0x5a;
        }
    }
    if (g_eng1_setup_dword_72188 == 0 && g_boss_state_6ffda < 8) {
        player = g_native_pointer_7217c;
        if (player->x > 0x15100000 && player->x < 0x17800000 && player->y > 0x4400000) {
            g_eng1_setup_dword_72188 = 2;
        }
    }
}

void eng1_337b0(void)
{
    Actor *a;

    g_level_actors_6fe78[0] = 0;
    a = spawn_at_group_300c0(0, 0, 0, 0x35, 0);
    if (a != 0) {
        g_level_actors_6fe78[++g_level_actors_6fe78[0]] = (long)a;
    }
}

void eng1_337f0(Actor *p)
{
    p->direction_x = 0xff;
    p->render_flags |= 8;
    advance_frame(p);
    restart_steps(p);
    p->unknown_0f4 = 0;
    eng1_native_code_target_002a290(p);
}

void eng1_33830(Actor *p)
{
    unsigned int w;

    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        eng1_interpret_signed_pairs(p);
        apply_frame_motion_2d(p);
        if (!eng1_29c20(p) ||
            ((w = g_configuration_dword_713bc[dispatch_local_2a170(p->x + (p->direction_x ? -0xa0000 : 0xa0000), p->y,
                                                                   (p->z + 0x400000) & 0xf800000, p->profile)]) & 0x26)) {
            p->x = p->saved_x;
            p->y = p->saved_y;
            select_frame(p, 8);
        }
    }
    p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10);
    if (eng1_2a9e0(p, 0x140)) {
        remove_actor(p);
    }
}

void eng1_33910(Actor *p)
{
    p->direction_x = 0xff;
    p->render_flags |= 8;
    advance_frame(p);
    restart_steps(p);
    p->unknown_11e = -1;
    p->unknown_0f4 = 10;
    p->hits = random_limit(0x32) + 0x1e;
    p->unknown_097 = 0;
    p->state = 0;
    eng1_native_code_target_002a290(p);
    spawn_actor_with_owner(p, p->x, p->y, p->z, 0x45);
}

void eng1_33990(Actor *p)
{
    long dx, dz;
    Actor *child;
    unsigned int w;
    long step;

    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
        return;
    }
    eng1_interpret_signed_pairs(p);
    apply_frame_motion_2d(p);
    if (p->parameter != 0) {
        p->parameter--;
    }
    dx = (p->x - g_native_pointer_7217c->x) >> 16;
    dz = (p->z - g_native_pointer_7217c->z) >> 16;
    switch (p->state) {
    case 0:
    case 1:
        if (p->parameter == 0) {
            if (abs(dz) < 0x20) {
                play_sound(0x5140, p);
            }
            p->parameter = random_limit(0x3c) + 0x5a;
        }
        if (p->unknown_097 == 0) {
            if (abs(dx) < 0x80 && abs(dz) < 8) {
                spawn_at_group_300c0(p->x, p->y - 0x100000, p->z, 0x4a, 0);
                child = action_spawn_27d40(p, 0xa001);
                if (child != 0) {
                    child->render_flags = (p->render_flags & 0x10) | 0x605;
                    child->unknown_04a = 0x80;
                    child->sprite = 0x25;
                    child->category = 0x14;
                }
                play_sound(0x4012, p);
                select_frame(p, 7);
                eng1_interpret_signed_pairs(p);
                p->hits = 0xa0;
            }
        } else if (!(p->status_01f & 2)) {
            p->unknown_097--;
        }
        if (p->unknown_114 == 0) {
            if ((p->direction_x && dx < 0) || (!p->direction_x && dx > 0)) {
                select_frame(p, 1);
                p->hits += 100;
            }
        }
        if (p->hits-- < 0) {
            select_frame(p, 4);
            p->hits = random_limit(0x50) + 0x32;
        }
        break;
    case 3:
        if (!eng1_29c20(p) && p->unknown_114 < 0xb) {
            p->x = p->saved_x;
            p->y = p->saved_y;
            p->direction_x = ~p->direction_x;
            break;
        }
        step = p->direction_x ? -0x100000 : 0x100000;
        w = g_configuration_dword_713bc[dispatch_local_2a170(p->x + step, p->y, p->z, p->profile)];
        if (w & 7) {
            p->x = p->saved_x;
            p->y = p->saved_y;
            p->direction_x = ~p->direction_x;
            break;
        }
        if (p->hits-- < 0) {
            select_frame(p, 4);
            p->unknown_097 = 0x14;
        }
        break;
    case 5:
        if ((p->direction_x && dx < 0) || (!p->direction_x && dx > 0)) {
            p->direction_x = ~p->direction_x;
        }
        if (dz + 0x20 > 0) {
            play_sound(0x5941, p);
            play_sound(0x5942, p);
        }
        p->state = 3;
        p->render_flags &= ~0x800000;
        break;
    }
    p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10);
    if (eng1_2a9e0(p, 0x400)) {
        remove_actor(p);
    }
}

void eng1_33cc0(Actor *p)
{
    p->unknown_04a = 0xa0;
    p->render_flags |= 0x605;
    reset_frame_state(p);
    eng1_interpret_word_stream(p);
}

void eng1_33cf0(Actor *p)
{
    Actor *q;

    q = p->link_12c;
    if (q != 0) {
        while (q->link_12c != 0) {
            q = q->link_12c;
        }
        if (q->state == 3) {
            eng1_interpret_word_stream(p);
            p->x = q->x;
            p->y = q->y;
            p->z = q->z;
            p->direction_x = q->direction_x;
            p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10) | 0x80000000;
        } else {
            p->render_flags &= 0x7fffffff;
        }
    } else {
        remove_actor(p);
    }
}

void eng1_33d70(Actor *p)
{
    p->unknown_04a = 0xa0;
    p->render_flags |= 0x605;
    p->image_064 = g_level_images_5e678[random_limit(3)];
    reset_frame_state(p);
}

void eng1_33db0(Actor *p)
{
    eng1_interpret_word_stream(p);
    if (((p->z ^ g_native_pointer_7217c->z) & 0xfff00000) == 0 &&
        ((p->x ^ g_native_pointer_7217c->x) & 0xfff00000) == 0 &&
        eng1_2a460(&g_native_pointer_7217c->bounds, p)) {
        eng1_native_code_target_00266a0(p, 0x10, 0);
    }
}

void eng1_33e10(Actor *p)
{
    p->flags |= 0x20800;
    p->render_flags |= 0x20000000;
    p->mode = 4;
    p->angle_74 = 0;
    p->unknown_04a = 0xe0;
    p->image_064 = g_level_word_pool_554f0;
    p->category = 2;
    reset_frame_state(p);
    eng1_interpret_word_stream(p);
    attach_entries(p, g_level_entries_5e688);
    p->unknown_098 = p->entry_count;
    p->parameter = 6;
}

void eng1_33e80(Actor *p)
{
    ActorEntry *e;
    int s;
    int i;

    if (p->parameter != 0) {
        p->parameter--;
        eng1_interpret_word_stream(p);
        e = p->entries;
        s = p->sprite;
        if (e != 0) {
            for (i = 0; i < p->entry_count; i++) {
                e[i].sprite = s;
                if (--s < 5) {
                    s = 0x14;
                }
            }
        }
    } else {
        remove_actor(p);
    }
}

void eng1_33ee0(Actor *p)
{
    advance_frame(p);
    eng1_native_code_target_002a290(p);
    p->render_flags |= 8;
    p->state = g_level_toggle_70802;
    if (g_level_toggle_70802 != 0) {
        select_frame(p, 0);
    } else {
        select_frame(p, 2);
    }
}

void eng1_33f30(Actor *p)
{
    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        if (g_level_toggle_70802 != p->state) {
            p->state = g_level_toggle_70802;
            if (g_level_toggle_70802 != 0) {
                select_frame(p, 0);
            } else {
                select_frame(p, 2);
            }
            play_sound(0x5148, p);
        }
        eng1_interpret_signed_pairs(p);
    }
    p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
    if (eng1_2a9e0(p, 0x140)) {
        remove_actor(p);
    }
}

void eng1_33fc0(Actor *p)
{
    advance_frame(p);
    p->unknown_124 = 3;
    eng1_native_code_target_002a290(p);
    p->render_flags |= 8;
    p->unknown_097 = 1;
}
