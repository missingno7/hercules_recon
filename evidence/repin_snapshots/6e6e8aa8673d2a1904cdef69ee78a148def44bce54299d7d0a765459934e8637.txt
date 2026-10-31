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
    short screen_x, screen_y, screen_z;
    unsigned char unknown_012[0x1c - 0x12];
    unsigned char clut_01c;
    unsigned char unknown_01d[0x1f - 0x1d];
    unsigned char status_01f;
    unsigned char unknown_020;
    unsigned char counter_021;
    unsigned char category;
    unsigned char mode;
    unsigned char unknown_024;
    unsigned char control_025;
    short unknown_026;
    unsigned char unknown_028[0x2a - 0x28];
    unsigned short state;
    unsigned char unknown_02c[0x2e - 0x2c];
    unsigned short kind;
    unsigned char unknown_030[0x34 - 0x30];
    unsigned short sprite;
    unsigned short entry_count;
    unsigned short profile;
    unsigned short scale;
    unsigned char unknown_03c[0x4a - 0x3c];
    unsigned short unknown_04a;
    long parameter;
    unsigned long flags;
    unsigned long render_flags;
    long record_offset;
    ActorEntry *entries;
    unsigned char unknown_060[0x64 - 0x60];
    void *image_064;
    unsigned char unknown_068[0x6c - 0x68];
    void *sound_06c;
    short angle_70;
    short angle_72;
    short angle_74;
    unsigned char unknown_076[0x88 - 0x76];
    Bounds bounds;
    unsigned char direction_x;
    unsigned char direction_y;
    unsigned char direction_z;
    unsigned char unknown_097;
    unsigned short unknown_098;
    short delay;
    long saved_x, saved_y, saved_z;
    long motion_x_velocity, motion_x_target, motion_x_rate, motion_x_unknown;
    long motion_y_velocity, motion_y_target, motion_y_rate, motion_y_unknown;
    long motion_z_velocity, motion_z_target, motion_z_rate, motion_z_unknown;
    unsigned char unknown_0d8[0xe0 - 0xd8];
    long limit_0e0;
    unsigned char unknown_0e4[0xec - 0xe4];
    unsigned char lifetime;
    unsigned char unknown_0ed;
    short hits;
    unsigned char unknown_0f0[0xf4 - 0xf0];
    long unknown_0f4;
    unsigned char unknown_0f8[0x108 - 0xf8];
    unsigned long unknown_108;
    unsigned char unknown_10c[0x114 - 0x10c];
    short unknown_114;
    unsigned char unknown_116[0x11e - 0x116];
    short unknown_11e;
    unsigned char unknown_120[0x124 - 0x120];
    short unknown_124;
    unsigned char unknown_126[0x12c - 0x126];
    Actor *link_12c;
    Actor *link_130;
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
void play_sound(unsigned short id, Actor *p);
void reset_frame_state(Actor *p);
void advance_frame(Actor *p);
void restart_steps(Actor *p);
void select_frame(Actor *p, int frame);
void apply_frame_motion_2d(Actor *p);
void eng1_interpret_word_stream(Actor *p);
int eng1_interpret_signed_pairs(Actor *p);
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

typedef struct FixedPosition { long x, y, z; } FixedPosition;
typedef struct RelativeMeasure {
    short x, y, z;
    short planar_distance;
    short distance;
    unsigned short direction;
} RelativeMeasure;
typedef struct SVector { short vx, vy, vz, pad; } SVector;
typedef struct KindInfo { unsigned char *owner; long count; } KindInfo;
typedef struct SpawnRecord {        /* 12-byte child placement */
    short x, y, z;
    short kind;
    unsigned short sprite;
    unsigned char direction;
    unsigned char unknown_0b;
} SpawnRecord;
typedef struct Callback8 { void (*initialize)(Actor *); void (*update)(Actor *); } Callback8;
extern Actor *g_frame_global_734f4;
extern KindInfo g_kind_info[];
extern long g_pipeline_dword_71680[];
extern long g_camera_focal_70ce4;
extern short *g_sine_table_70f34;
extern short g_sine_table_5a848[];
extern unsigned short g_native_word_72168;
extern long g_level_effects_5e9c0[8][8];
extern unsigned short g_level_noise_5ef88[];
extern unsigned char g_debris_sprites_5ef60[];
extern unsigned char g_clut_rows_602a8[];
extern unsigned char g_clut_rows_602b0[];
extern unsigned char g_clut_rows_602b4[];
extern unsigned char g_clut_rows_602b8[];
extern unsigned char g_clut_rows_602bc[];
extern unsigned char g_clut_rows_602c0[];
extern unsigned long eng1_sim_state_72110;
extern SpawnRecord g_spawn_records_5ff60[];
extern Callback8 *g_configuration_dword_7137c;
extern unsigned short g_debris_sounds_5febc[];
unsigned int dispatch_engine_2c470(void);
void RemoveFxLinks_7ef6(Actor *p);
void apply_frame_motion_3d(Actor *p);
int eng1_2a710(Bounds *b, Actor *p);
int eng1_native_code_target_002aab0(Actor *p, int a, int b);
int remove_actor_variant(Actor *p);
void hide_and_unlink(Actor *p);
unsigned long eng1_26de0(Actor *p);
void eng1_26f20(Actor *p);
void eng1_131c0(long x, long y, long z, long id);
int eng1_19b40(int mask);
void eng1_1aba0(void);
void eng1_24980(Actor *p);
void eng1_30290(unsigned long hit, int b);
int eng1_2bd20(Actor *p);
void eng1_native_code_target_002ae50(Actor *p);
void measure_absolute_vector(FixedPosition *a, FixedPosition *b, RelativeMeasure *m);
void measure_relative_vector(FixedPosition *a, FixedPosition *b, RelativeMeasure *m);
int actor_bounds_2a880(Actor *p, Bounds *b, int mode);
int bounds_overlap(Bounds *a, Bounds *b);
Actor *find_tag(int tag);
void append_chain(Actor *owner, Actor *p);
int eng1_02500(Actor *p, void *descriptor);
void eng1_02920(Actor *p, int width);
void eng1_02a80(Actor *p, int y);
Actor *spawn_middle_277e0(long x, long y, long z, unsigned short kind);
void attach_actor(Actor *p, int tag);
int eng1_30050(long x, long y, long z);
unsigned int dispatch_local_30010(long x, long y, long z, unsigned short mask);
int eng1_native_code_target_002aa30(Actor *p, int a, int b);

void eng1_34000(Actor *p)
{
    static unsigned short effect_set;   /* 0x6ffd8: dword-loaded, so its alignment is known to this unit */
    RelativeMeasure m;
    int i;
    int t;

    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        measure_absolute_vector((FixedPosition *)g_native_pointer_7217c, (FixedPosition *)p, &m);
        if (p->unknown_097 != 2) {
            if (m.z < 0x10) {
                if (m.x < 0x30) {
                    select_frame(p, 4);
                    p->unknown_097 = 2;
                    if (p->state == 0) {
                        for (i = 0; i < 8; i++) {
                            eng1_131c0(p->x, p->y - 0x200000, p->z, g_level_effects_5e9c0[effect_set & 7][i]);
                        }
                        effect_set++;
                        p->state++;
                    }
                }
            } else if (m.z > 0x5a && m.x < 0x60) {
                select_frame(p, 4);
                p->unknown_097 = 2;
            }
        } else if (m.x > 0x80 || (m.z < 0x5a && m.z > 0x10)) {
            select_frame(p, 9);
            p->unknown_097 = 4;
        }
        t = p->unknown_097;
        p->unknown_097 = p->state;
        p->state = t;
        if (eng1_interpret_signed_pairs(p) && p->state == 1 && random_limit(5) == 0) {
            select_frame(p, 1);
            p->state = 4;
        }
        t = p->unknown_097;
        p->unknown_097 = p->state;
        p->state = t;
    }
    p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10);
    if (eng1_2a9e0(p, 0x140)) {
        remove_actor(p);
    }
}

void eng1_341a0(Actor *p)
{
    p->direction_x = -random_limit(2);
    advance_frame(p);
    select_frame(p, random_limit(6));
    restart_steps(p);
    p->render_flags |= 8;
    p->unknown_0f4 = 2;
    p->clut_01c = g_clut_rows_602b0[random_limit(3)];
    eng1_native_code_target_002a290(p);
}

void eng1_34210(Actor *p)
{
    unsigned int w;

    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        apply_frame_motion_2d(p);
        eng1_interpret_signed_pairs(p);
        if (!eng1_29c20(p)) {
            p->x = p->saved_x;
            p->y = p->saved_y;
            select_frame(p, 8);
        } else if (((w = g_configuration_dword_713bc[dispatch_local_2a170(p->x + (p->direction_x ? -0x180000 : 0x180000), p->y,
                                                              (p->z + 0x400000) & 0xf800000, p->profile)]) & 6)) {
            p->x = p->saved_x;
            p->y = p->saved_y;
            select_frame(p, 8);
        }
    }
    p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10);
    if (eng1_2a9e0(p, 0x200)) {
        remove_actor(p);
    }
}

void eng1_342f0(Actor *p)
{
    int c;

    if (p->record_offset) {
        c = g_kind_info[p->profile].owner[p->record_offset + 0xc] & 0x3f;
        if (c != 0x3f) {
            p->clut_01c = c;
        }
    }
    p->render_flags |= 8;
    advance_frame(p);
    select_frame(p, random_limit(0xc));
    eng1_native_code_target_002a290(p);
}

void eng1_34350(Actor *p)
{
    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        eng1_interpret_signed_pairs(p);
    }
    p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10);
    if (eng1_2a9e0(p, 0x200)) {
        remove_actor(p);
    }
}

void eng1_343a0(Actor *p)
{
    advance_frame(p);
    p->render_flags |= 8;
    p->unknown_124 = 2;
    p->clut_01c = g_clut_rows_602bc[random_limit(3)];
    eng1_native_code_target_002a290(p);
}

void eng1_343e0(Actor *p)
{
    unsigned int w;
    RelativeMeasure m;
    int d;
    int v;

    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        eng1_interpret_signed_pairs(p);
        if (!eng1_29c20(p) ||
            ((w = g_configuration_dword_713bc[dispatch_local_2a170(p->x + (p->direction_x ? -0x180000 : 0x180000), p->y,
                                                              (p->z + 0x400000) & 0xf800000, p->profile)]) & 7)) {
            p->x = p->saved_x;
            p->y = p->saved_y;
            select_frame(p, 3);
        }
        if (p->state == 4) {
            d = p->sprite - 0x22;
            if (d > 0 && d < 0x28) {
                measure_relative_vector((FixedPosition *)g_frame_global_734f4, (FixedPosition *)p, &m);
                v = 0x200 - (unsigned short)m.distance;
                if (v < 1) {
                    v = 0;
                }
                v *= g_level_noise_5ef88[d];
                if (v) {
                    g_pipeline_dword_71680[++g_pipeline_dword_71680[0]] = (random_limit(v + v) - v) << 6;
                }
                if (!random_limit(10)) {
                    spawn_at_group_300c0(p->x, p->y, p->z, 0x402a, p->profile);
                }
            }
        }
    }
    p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10);
    if (eng1_2a9e0(p, 0x200)) {
        remove_actor(p);
    }
}

void eng1_34570(Actor *p)
{
    int r;
    long dz;

    if (p->state == 4) {
        p->state = 1;
        select_frame(p, 0);
        return;
    }
    r = random_limit(0x28);
    dz = (g_native_pointer_7217c->z - p->z) >> 16;
    if ((p->status_01f & 2) && p->screen_x > 0x14 && p->screen_x < 0x12c) {
        if (r < 10) {
            return;
        }
        if (r < 0xf) {
            select_frame(p, 0x10);
            return;
        }
        if (dz > 0x20) {
            return;
        }
        if (r < 0x1c) {
            select_frame(p, 6);
            p->state = 4;
            return;
        }
    } else {
        if (r < 0x19) {
            return;
        }
        if (r < 0x26) {
            select_frame(p, 0x10);
            return;
        }
        if (dz > 0x20) {
            return;
        }
        if (r < 0x27) {
            select_frame(p, 6);
            p->state = 4;
            return;
        }
    }
    select_frame(p, 0xb);
    p->state = 4;
}

void eng1_34650(Actor *p)
{
    p->render_flags |= 0x10000040;
    p->y -= 0x12c0000;
    p->x += (random_limit(200) - 100) << 16;
    p->z += (random_limit(200) - 100) << 16;
    p->delay = random_limit(0x26);
    p->sprite = g_debris_sprites_5ef60[p->delay];
    if (p->delay > 0xf) {
        p->scale = random_limit(0x200) + 0x80;
    }
    p->direction_y = 0xff;
    p->direction_z = 0xff;
    p->motion_y_target = ((dispatch_engine_2c470() & 0x1ff) + 0xc00) << 8;
    p->motion_y_velocity = 0;
    p->motion_y_rate = 0;
    p->motion_y_unknown = random_limit(0x2000) + 0x8000;
    p->motion_z_velocity = 0;
    p->motion_z_target = 0xf0000;
    p->motion_z_rate = 0;
    p->motion_z_unknown = 0;
}

void eng1_34750(Actor *p)
{
    p->angle_70 = 0x400 - ((p->z >> 13) & 0x3ff);
    p->angle_72 = 0x400 - ((p->x >> 13) & 0x3ff);
    if (p->state == 1) {
        if (eng1_2bd20(p) & 2) {
            remove_actor_variant(p);
        }
    } else if (eng1_2bd20(p) & 1) {
        if (p->scale > 0x180 || p->delay < 0xb) {
            play_sound(0xd150, p);
            eng1_24980(p);
            remove_actor_variant(p);
            return;
        }
        if (p->motion_z_rate == 0 && random_limit(2)) {
            if (p->delay < 0x18) {
                play_sound(0xd151, p);
            } else {
                play_sound(0xd152, p);
            }
        }
        p->motion_z_velocity = 0x40000;
        p->motion_z_rate += 0x8000;
    }
    if (eng1_2a710(&g_native_pointer_7217c->bounds, p)) {
        eng1_native_code_target_00266a0(p, 1, 0);
    }
    if (p->z < -0x400000 || p->z > 0x1f00000 || p->y > 0x5800000 || eng1_2a9e0(p, 0x80)) {
        remove_actor_variant(p);
    }
}

void eng1_348a0(Actor *p)
{
    advance_frame(p);
    restart_steps(p);
    p->render_flags |= 8;
    p->unknown_0f4 = 4;
    p->unknown_124 = 2;
    p->clut_01c = g_clut_rows_602b8[random_limit(2)];
    eng1_native_code_target_002a290(p);
    spawn_actor_with_owner(p, p->x, p->y, p->z, 0x42);
}

void eng1_34910(Actor *p)
{
    unsigned int w;

    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        apply_frame_motion_2d(p);
        eng1_interpret_signed_pairs(p);
        if (!eng1_29c20(p)) {
            p->x = p->saved_x;
            p->y = p->saved_y;
            select_frame(p, 0xc);
        } else if (((w = g_configuration_dword_713bc[dispatch_local_2a170(p->x + (p->direction_x ? -0x180000 : 0x180000), p->y,
                                                              p->z, p->profile)]) & 7)) {
            p->x = p->saved_x;
            p->y = p->saved_y;
            select_frame(p, 0xc);
        }
    }
    p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10);
    if (eng1_native_code_target_002aab0(p, 0x40, 0x40) || eng1_2a9e0(p, 0x400)) {
        remove_actor(p);
    }
}

void eng1_349f0(Actor *p)
{
    p->sprite = 1;
    p->render_flags |= 0x604;
    p->unknown_04a = 0x60;
    p->direction_x = -random_limit(2);
    eng1_26f20(p);
}

void eng1_34a30(Actor *p)
{
    Actor *q;
    int y;

    q = p->link_12c;
    if (q != 0) {
        if (g_native_pointer_7217c->z - p->z > 0x800000) {
            if (p->sound_06c != 0) {
                RemoveFxLinks_7ef6(p);
            }
        } else if (p->status_01f & 1) {
            if (p->sound_06c == 0) {
                p->unknown_097 = 1;
                play_sound(0x594f, p);
            }
            if (p->counter_021 != 0) {
                p->counter_021--;
            } else if (p->status_01f & 2) {
                play_sound(0x514e, p);
                p->counter_021 = random_limit(0x78) + 0x5a;
            }
        }
        eng1_native_code_target_002ae50(p);
        y = p->screen_y - (g_camera_focal_70ce4 >> 1) * 130 / p->screen_z;
        if (y < 0) {
            y = 0;
        }
        if (p->link_130 != 0) {
            p->link_130->unknown_026 = y;
        }
        while (q->link_12c != 0) {
            q = q->link_12c;
        }
        if (q->kind == 0x2c) {
            p->x = q->x;
            p->y = q->y;
            p->z = q->z;
            return;
        }
    }
    remove_actor(p);
}

void eng1_34b40(Actor *p, int i)
{
    SVector *v;
    int angle;
    int radius;
    int x;
    int z;

    v = (SVector *)p->entries;
    angle = random_limit(0x1000);
    radius = random_limit(0xc) + 8;
    x = g_sine_table_70f34[angle & 0xfff] * radius >> 12;
    z = g_sine_table_70f34[(angle - 0x400) & 0xfff] * radius >> 12;
    v[i].vx = x;
    v[i].vz = z;
    v[i].vy = 0;
}

void eng1_34bb0(Actor *p, int t)
{
    int w;

    w = g_sine_table_5a848[((t >> 2) + 0x100) & 0x3ff] * 20 >> 16;
    eng1_02a80(p, p->unknown_026);
    eng1_02920(p, w);
}

void eng1_34c00(Actor *p)
{
    p->flags |= 0x10000000;
    p->render_flags = 0x800;
    eng1_02500(p, p->image_064);
}

void eng1_34c30(Actor *p)
{
    Actor *q;

    q = p->link_12c;
    if (q != 0) {
        while (q != 0) {
            if (q->kind == 0x42) {
                p->x = q->x;
                p->y = q->y;
                p->z = q->z;
                return;
            }
            q = q->link_12c;
        }
        hide_and_unlink(p);
    } else {
        p->render_flags &= 0x7fffffff;
    }
}

void eng1_34c80(Actor *p)
{
    advance_frame(p);
    select_frame(p, random_limit(0x17));
    restart_steps(p);
    p->direction_x = -random_limit(2);
    p->render_flags |= 8;
    p->unknown_0f4 = random_limit(3) & 1;
    p->clut_01c = g_clut_rows_602a8[random_limit(5)];
    eng1_native_code_target_002a290(p);
}

void eng1_34d00(Actor *p)
{
    unsigned int w;

    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        apply_frame_motion_2d(p);
        eng1_interpret_signed_pairs(p);
        if (!eng1_29c20(p) ||
            ((w = g_configuration_dword_713bc[dispatch_local_2a170(p->x + (p->direction_x ? -0x100000 : 0x100000), p->y,
                                                              (p->z + 0x400000) & 0xf800000, p->profile)]) & 6)) {
            p->x = p->saved_x;
            p->y = p->saved_y;
            p->direction_x = ~p->direction_x;
            select_frame(p, 0);
        }
    }
    p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10);
    if (eng1_2a9e0(p, 0x140)) {
        remove_actor(p);
    }
}

void eng1_34de0(Actor *p)
{
    p->unknown_124 = 1;
    p->direction_z = 0xff;
    p->render_flags = 8;
    advance_frame(p);
    restart_steps(p);
    eng1_native_code_target_002a290(p);
}

void eng1_34e20(Actor *p)
{
    unsigned int w;

    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        p->render_flags |= 0x80000000;
        eng1_interpret_signed_pairs(p);
        apply_frame_motion_3d(p);
        switch (p->state) {
        case 3:
            eng1_29c20(p);
            break;
        case 2:
        case 12:
            eng1_29c20(p);
            if (p->z > p->limit_0e0) {
                w = g_configuration_dword_713bc[dispatch_local_2a170(p->x, p->y, p->z, p->profile)];
                if (w & 0x10) {
                    p->limit_0e0 += 0x800000;
                    break;
                }
                p->z = p->limit_0e0;
                p->direction_x = ~p->direction_x;
                select_frame(p, 4);
                p->state = 0;
            }
            break;
        case 1:
        case 11:
            eng1_29c20(p);
            if (p->z < p->limit_0e0) {
                w = g_configuration_dword_713bc[dispatch_local_2a170(p->x, p->y, p->z, p->profile)];
                if (w & 0x20) {
                    p->limit_0e0 -= 0x800000;
                    break;
                }
                p->z = p->limit_0e0;
                p->direction_x = ~p->direction_x;
                select_frame(p, 4);
                p->state = 0;
            }
            break;
        default:
            if (!eng1_29c20(p)) {
                p->x = p->saved_x;
                p->y = p->saved_y - 0x80000;
                eng1_native_code_target_002a290(p);
                select_frame(p, 7);
                break;
            }
            if (eng1_2a710(&g_native_pointer_7217c->bounds, p)) {
                p->state = 3;
                eng1_native_code_target_00266a0(p, 1, 0);
                select_frame(p, 0x16);
                break;
            }
            w = g_configuration_dword_713bc[dispatch_local_2a170(p->x + (p->direction_x ? -0x100000 : 0x100000), p->y,
                                                                 p->z, p->profile)];
            if (w & 2) {
                p->state = 3;
                select_frame(p, 0x16);
            } else if (w & 4) {
                p->x = p->saved_x;
                p->y = p->saved_y;
                select_frame(p, 7);
            }
            break;
        }
    }
    p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10);
    if (abs((p->x - g_frame_global_734f4->x) >> 16) > 0x1000) {
        remove_actor(p);
    }
}

void eng1_350a0(Actor *p)
{
    unsigned long cell;
    unsigned int w;

    if (p->state == 1 || p->state == 2) {
        cell = dispatch_local_2a170(p->x, p->y, p->z, p->profile);
        if (cell != 0) {
            w = g_configuration_dword_713bc[cell];
            if (p->direction_x) {
                if (w & 0x20) {
                    p->state = 0xb;
                    return;
                }
            } else if (w & 0x10) {
                p->state = 0xc;
                return;
            }
        }
        select_frame(p, 0);
        p->state = 0;
        return;
    }
    p->state = 0;
    if (p->record_offset &&
        abs(p->x - *(long *)(g_kind_info[p->profile].owner + p->record_offset)) > 0x8000000) {
        select_frame(p, 7);
        return;
    }
    cell = dispatch_local_2a170(p->x, p->y, p->z, p->profile);
    if (cell != 0) {
        w = g_configuration_dword_713bc[cell];
        if (p->direction_x) {
            if (w & 0x20) {
                select_frame(p, 0x10);
                p->state = 1;
                p->limit_0e0 = (p->z & 0x7f800000) - 0x800000;
            }
        } else if (w & 0x10) {
            p->limit_0e0 = (p->z & 0x7f800000) + 0x800000;
            select_frame(p, 10);
            p->state = 2;
        }
    } else {
        p->unknown_097 = 0;
    }
}

void eng1_35200(Actor *p)
{
    Actor *t;

    t = find_tag(0x2a);
    if (t != 0) {
        append_chain(t, p);
    }
    advance_frame(p);
    restart_steps(p);
    p->render_flags |= 8;
    if (p->record_offset) {
        p->unknown_097 = g_kind_info[p->profile].owner[p->record_offset + 0xc] & 0x3f;
    }
    if (p->state == 0) {
        p->clut_01c = g_clut_rows_602c0[random_limit(4)];
    } else {
        p->clut_01c = p->state >> 8;
        p->state &= 0xff;
    }
    if (p->state >= 8) {
        select_frame(p, 0x16);
        p->hits = -1;
        p->flags = 2;
    } else {
        select_frame(p, 0x22);
        p->hits = 3;
        p->flags = 0x202;
    }
    p->unknown_124 = 4;
    p->unknown_108 = p->clut_01c;
    p->lifetime = 0x3c;
    eng1_native_code_target_002a290(p);
}

void eng1_352f0(Actor *p)
{
    Bounds b;
    unsigned int w;
    unsigned long hit;
    int n;

    if (p->parameter != 0) {
        p->parameter--;
    }
    if (eng1_2a9e0(p, 0x80)) {
        remove_actor(p);
        return;
    }
    if (p->control_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        eng1_interpret_signed_pairs(p);
        if (p->hits <= 0) {
            return;
        }
        apply_frame_motion_2d(p);
        hit = eng1_26de0(p);
        if (hit && !p->lifetime) {
            if (hit == 0x80) {
                n = eng1_19b40(~g_native_word_72168 & 0x5f);
                if (n) {
                    eng1_1aba0();
                    p->hits -= n;
                    select_frame(p, 0x12);
                }
            } else {
                eng1_30290(hit, 0);
                p->hits -= 100;
                select_frame(p, 0x12);
            }
            p->lifetime = 0x28;
        }
        if (p->hits <= 0) {
            select_frame(p, 0x15);
            spawn_at_group_300c0(p->x, p->y - 0x200000, p->z, 0x2027, 0);
            p->flags &= ~0x200;
            return;
        }
        if (p->state > 1) {
            if (!eng1_29c20(p)) {
                p->x = p->saved_x;
                p->y = p->saved_y;
                select_frame(p, 1);
            } else if ((w = g_configuration_dword_713bc[dispatch_local_2a170(p->x, p->y, p->z, p->profile)]) & 2) {
                p->x = p->saved_x;
                p->y = p->saved_y;
                select_frame(p, 1);
            }
        }
    }
    if (p->state == 3 && eng1_2a710(&g_native_pointer_7217c->bounds, p)) {
        select_frame(p, 6);
    }
    if (actor_bounds_2a880(p, &b, 3) && bounds_overlap(&g_native_pointer_7217c->bounds, &b)) {
        eng1_native_code_target_00266a0(p, 3, 0);
        if (p->parameter == 0) {
            play_sound(0x51dd, p);
            p->parameter = 0x14;
        }
        p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10);
        return;
    }
    if (actor_bounds_2a880(p, &b, 2) && bounds_overlap(&g_native_pointer_7217c->bounds, &b)) {
        eng1_native_code_target_00266a0(p, 3, 0);
        if (p->parameter == 0) {
            play_sound(0x51dd, p);
            p->parameter = 0x14;
        }
    }
    p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10);
}

void eng1_35590(Actor *p)
{
    RelativeMeasure m;
    long dx;
    int active;
    int attacking;
    Actor *q;
    Actor *c;
    SpawnRecord *r;
    long x, y, z;
    int kind;
    Actor *child;
    int d;
    int n;

    dx = (p->x - g_native_pointer_7217c->x) >> 16;
    active = 0;
    attacking = 0;
    if (p->link_12c != 0) {
        for (q = p; q->link_12c != 0; q = q->link_12c) {
        }
        for (c = q->link_130; c != 0; c = c->link_130) {
            if (c->state > 0) {
                if (c->state < 10) {
                    active++;
                }
                if (c->state == 4) {
                    attacking++;
                }
            }
        }
    }
    if (eng1_sim_state_72110) {
        attacking = 999;
    }
    measure_absolute_vector((FixedPosition *)g_native_pointer_7217c, (FixedPosition *)p, &m);
    switch (p->state) {
    case 0:
        if (m.z > 0x10 || m.y > 0x10 || m.x < 0x20) {
            break;
        }
        if (active < 1) {
            if (m.x > 0x100) {
                break;
            }
            eng1_native_code_target_002ae50(p);
            if (p->status_01f & 2) {
                select_frame(p, 0x1f);
                return;
            }
            select_frame(p, 0);
            return;
        }
        if (active >= random_limit(3) + 2 || m.x > 0x100) {
            break;
        }
        eng1_native_code_target_002ae50(p);
        if (p->status_01f & 2) {
            select_frame(p, 0x1f);
            return;
        }
        break;
    case 1:
    case 2:
    case 3:
        d = abs(dx);
        if (d > 0x40 && random_limit(9) == 0) {
            select_frame(p, 0);
            return;
        }
        if ((dx < 0 && p->direction_x == 0xff) || (dx >= 0 && p->direction_x == 0)) {
            if (d < 0x14) {
                select_frame(p, 0x19);
                return;
            }
            select_frame(p, 1);
            return;
        }
        if (attacking < 3) {
            if (d < 0x10) {
                select_frame(p, 0x19);
                return;
            }
            if (d < 0x22) {
                select_frame(p, 0x1c);
                return;
            }
        } else {
            select_frame(p, 0x19);
        }
        if (d < 0x2c) {
            if (attacking < 5) {
                if (random_limit(5) == 0) {
                    select_frame(p, 0x19);
                    return;
                }
                if (random_limit(2)) {
                    select_frame(p, 9);
                    return;
                }
                select_frame(p, 0xc);
                return;
            }
            if (random_limit(5) == 0) {
                select_frame(p, 0x1c);
                return;
            }
            select_frame(p, 0x19);
            return;
        }
        if (d < 0x56) {
            if (attacking < 2) {
                if (d > 0x3c && random_limit(4) == 0) {
                    select_frame(p, 0xf);
                    return;
                }
                if (random_limit(10) == 0) {
                    select_frame(p, 0x19);
                    return;
                }
                if (random_limit(8) == 0) {
                    select_frame(p, 0x1c);
                    return;
                }
                select_frame(p, 0xc);
                return;
            }
            if (random_limit(6) == 0) {
                select_frame(p, 0x1c);
                return;
            }
            if (random_limit(8) == 0) {
                select_frame(p, 0x19);
                return;
            }
            select_frame(p, 0);
            return;
        }
        if (p->unknown_114 == 4 || p->unknown_114 == 5) {
            select_frame(p, 5);
            return;
        }
        select_frame(p, 4);
        return;
    case 8:
        p->flags &= ~0x200;
        x = p->x;
        y = p->y;
        z = p->z;
        n = p->unknown_097;
        if (n) {
            eng1_131c0(x, y - 0x200000, z, n - 1);
        }
        for (r = g_spawn_records_5ff60; (kind = r->kind) != 0; r++) {
            child = spawn_middle_277e0((r->x << 16) + x, (r->y << 16) + y, (r->z << 16) + z, kind);
            if (child != 0) {
                g_configuration_dword_7137c[kind & 0x1ff].initialize(child);
                child->sprite = r->sprite;
                child->direction_x = r->direction;
                if (child->direction_x == 0x69) {
                    child->direction_x = p->direction_x;
                }
                if (child->direction_x == 0x99) {
                    child->direction_x = p->direction_x == 0;
                }
                child->clut_01c = p->clut_01c;
            }
        }
        break;
    }
}

void eng1_35a30(Actor *p)
{
    p->direction_x = p->state == 0;
    p->direction_z = -random_limit(2);
    p->direction_y = 0xff;
    p->state = 0;
    p->motion_y_velocity = p->motion_y_target = ((dispatch_engine_2c470() & 0x1ff) + 0x800) << 8;
    p->motion_y_rate = 0;
    p->motion_y_unknown = random_limit(0x2000) + 0x8000;
    p->motion_z_velocity = (p->motion_z_target = ((dispatch_engine_2c470() & 0x7f) + 0x80) << 8) >> 1;
    p->motion_z_rate = random_limit(0x1000) + 0x1000;
    p->motion_z_unknown = 0;
    p->motion_x_velocity = (p->motion_x_target = ((dispatch_engine_2c470() & 0x3ff) + 0x300) << 8) >> 1;
    p->motion_x_rate = random_limit(0x4000) + 0x1000;
    p->motion_x_unknown = 0;
}

void eng1_35b20(Actor *p)
{
    long step;

    p->angle_74 = 0x400 - ((p->x >> 13) & 0x3ff);
    if ((eng1_2bd20(p) & 1) && p->state < random_limit(3) + 1) {
        play_sound(g_debris_sounds_5febc[p->sprite], p);
        p->state++;
    }
    step = 8;
    if (p->direction_x) {
        step = -8;
    }
    if (eng1_30050((step << 16) + p->x, p->y, p->z)) {
        p->direction_x = ~p->direction_x;
    } else if (dispatch_local_30010((step << 16) + p->x, p->y, p->z, 1)) {
        p->direction_x = ~p->direction_x;
    }
    if (p->motion_y_velocity < 0x10000) {
        if (random_limit(0x800) < 0x100) {
            p->scale -= 0x10;
        }
        if (p->scale < 0x20) {
            remove_actor_variant(p);
        }
    } else {
        p->angle_74 += (signed char)p->direction_x * 8;
    }
    if (eng1_native_code_target_002aa30(p, 0x40, 0x40) || p->z < -0x800000) {
        remove_actor_variant(p);
    }
}

void eng1_35c60(Actor *p)
{
    advance_frame(p);
    select_frame(p, 1);
    restart_steps(p);
    p->render_flags |= 8;
    p->clut_01c = g_clut_rows_602b4[random_limit(4)];
    p->unknown_0f4 = random_limit(0xe) + 1;
    p->y = 0x4e00000;
    p->direction_z = 0xff;
    p->flags |= 0x800;
    p->saved_x = p->x;
    p->state = 2;
    p->mode = 8;
    attach_actor(p, 0x202e);
}

void eng1_35cf0(Actor *p)
{
    advance_frame(p);
    restart_steps(p);
    p->render_flags |= 8;
    p->clut_01c = g_clut_rows_602b4[random_limit(4)];
    p->unknown_0f4 = random_limit(0xe) + 1;
    p->y = 0x4e00000;
    p->direction_z = 0;
    p->flags |= 0x800;
    p->saved_x = p->x;
    p->state = 1;
    p->mode = 8;
    attach_actor(p, 0x202e);
}

void eng1_35d70(Actor *p)
{
    long z;
    int a;

    if (p->sound_06c == 0 && (p->status_01f & 2)) {
        if (p->unknown_0f4 > 6) {
            play_sound(0x5949, p);
        } else {
            play_sound(0x594a, p);
        }
    }
    eng1_interpret_signed_pairs(p);
    if (p->state != 0) {
        apply_frame_motion_3d(p);
    }
    z = p->z >> 16;
    if (z < -0x30) {
        p->render_flags |= 0x105;
        a = z * 2 + 0xe0;
        if (a < 0) {
            a = 0;
        }
        p->unknown_04a = a;
    } else {
        p->unknown_04a = 0x80;
        p->render_flags &= ~0x107;
    }
    if (g_level_toggle_70802 == 0 && p->state == 0) {
        p->state = 1;
        if (p->unknown_114 > 1) {
            select_frame(p, p->unknown_114 - 2);
        }
    }
    if (p->direction_z != 0) {
        if (z > 0x180) {
            p->x = p->saved_x - (z << 16) + 0x1800000;
        }
        if (z > 0x240) {
            p->z = -0x800000;
            p->x = p->saved_x;
            p->unknown_0f4 = random_limit(0xe) + 1;
            p->sprite = 0;
            if (p->unknown_0f4 > 4) {
                p->state = 3;
            }
            if (p->unknown_0f4 > 8) {
                p->state = 2;
            }
            RemoveFxLinks_7ef6(p);
            p->sound_06c = 0;
        }
        if (p->unknown_0f4 > 10 && z < -0x20 && z > -0x40 && g_level_toggle_70802 == 1) {
            p->state = 0;
        }
    } else {
        if (z > 0x180) {
            p->x = ((z - 0x180) << 16) + p->saved_x;
        }
        if (z < -0x6c) {
            p->z = 0x2400000;
            p->x = p->saved_x;
            p->unknown_0f4 = random_limit(0xe) + 1;
            p->sprite = 0;
            p->state = 1;
            if (p->unknown_0f4 > 4) {
                p->state = 2;
            }
            if (p->unknown_0f4 > 8) {
                p->state = 3;
            }
            RemoveFxLinks_7ef6(p);
            p->sound_06c = 0;
        }
        if (p->unknown_0f4 > 10 && z > 0x50 && z < 0x80 && g_level_toggle_70802 == 1) {
            p->state = 0;
        }
    }
    if (p->state == 0 && p->unknown_114 < 2) {
        select_frame(p, p->unknown_114 + 2);
    }
    if (eng1_2a710(&g_native_pointer_7217c->bounds, p)) {
        eng1_native_code_target_00266a0(p, 2, 0);
    }
}

void eng1_35fb0(Actor *p)
{
    p->flags = 0x10000;
    p->render_flags |= 5;
    p->unknown_04a = 0x20;
    reset_frame_state(p);
    eng1_interpret_word_stream(p);
}

void eng1_35fe0(Actor *p)
{
    Actor *q;

    q = (Actor *)p->entries;
    p->x = q->x;
    p->y = q->y;
    p->z = q->z;
    eng1_interpret_word_stream(p);
    if (q->state == 0) {
        p->render_flags &= 0x7fffffff;
    } else {
        p->render_flags |= 0x80000000;
    }
}
