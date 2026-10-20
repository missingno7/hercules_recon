#include <string.h>
extern "C" {
/* ENG3.DLL 0x16240..0x180e0: level-object handlers, HUD/backdrop helpers and the level driver
 * (lane y11 candidate; follows calibration/eng3/handlers_14190.cpp at 0x14190..0x16240).
 * One C++ unit (functions in extern "C"): C++ is 17/20 masked equal, C 14/20 (0x16f30, 0x17090,
 * 0x17130 base/index order only in C++; 0x16c40's /2 vs /10 evaluation order only in C).
 * <string.h> is a genuine use (memset in 0x171a0).
 * Type views follow handlers_14190.cpp, widened at unchanged offsets: Actor +0x21, +0x3e, +0x11e,
 * +0x126 and size 0x134 (array stride of g_eng3_actor_42540); LevelContext +0x03..+0x05, +0x0d,
 * +0x10, +0x26..+0x2c, +0x30, +0x38 (tick), +0x65.  Offsets are provisional shared names.
 * Open residues: 0x16c40 commutative operand order (two sites), 0x171a0 one store scheduled
 * across the memset expansion, 0x174e0 one memory-operand test loaded into al. */
typedef struct Actor Actor;

typedef struct Bounds3D {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds3D;

struct Actor {
    long x, y, z;
    unsigned char unknown_0c[0x1c - 0x0c];
    unsigned char unknown_1c;
    unsigned char unknown_1d[0x21 - 0x1d];
    unsigned char unknown_21;
    unsigned char category;
    unsigned char mode;
    unsigned char unknown_24;
    unsigned char unknown_25;
    short unknown_26;
    unsigned short unknown_28;
    short state;
    unsigned char unknown_2c[0x2e - 0x2c];
    unsigned short kind;
    short ticks, phase;
    unsigned short sprite;
    short unknown_36;
    unsigned short profile;
    unsigned short scale;
    short unknown_3c;
    short unknown_3e;
    unsigned char unknown_40[0x4a - 0x40];
    unsigned short unknown_4a;
    long parameter;
    unsigned long flags, render_flags;
    unsigned char *owner_context;
    Actor *attachment;
    unsigned char unknown_60[0x64 - 0x60];
    void *image;
    unsigned char unknown_68[0x70 - 0x68];
    short speed, step;
    short unknown_74;
    unsigned char unknown_76[0x88 - 0x76];
    Bounds3D shape;
    unsigned char unknown_94, unknown_95;
    unsigned char unknown_96[0x98 - 0x96];
    short unknown_98, unknown_9a;
    long prev_x, prev_y, prev_z;
    long unknown_a8, unknown_ac, unknown_b0, unknown_b4;
    long unknown_b8, unknown_bc, unknown_c0, unknown_c4;
    long unknown_c8, unknown_cc, unknown_d0, unknown_d4;
    long unknown_d8, unknown_dc, unknown_e0;
    unsigned char unknown_e4[0xec - 0xe4];
    unsigned char unknown_ec;
    unsigned char unknown_ed;
    short unknown_ee;
    long unknown_f0, unknown_f4;
    unsigned char unknown_f8[0xfc - 0xf8];
    long unknown_fc, unknown_100;
    unsigned char unknown_104[0x110 - 0x104];
    Actor *unknown_110;
    unsigned char unknown_114[0x116 - 0x114];
    short unknown_116;
    unsigned char unknown_118[0x11e - 0x118];
    short unknown_11e;
    unsigned char unknown_120[0x126 - 0x120];
    short unknown_126;
    unsigned char unknown_128[0x12c - 0x128];
    const long *unknown_12c;
    unsigned char unknown_130[0x134 - 0x130];
};

/* Interface context (g_engine_interface.context_004). */
typedef struct LevelContext {
    signed char level;
    signed char sublevel;
    unsigned char unknown_02;
    unsigned char unknown_03;
    unsigned char unknown_04;
    unsigned char unknown_05;
    unsigned char unknown_06[0x0c - 0x06];
    signed char unknown_0c;
    unsigned char unknown_0d;
    unsigned char difficulty;
    unsigned char unknown_0f;
    unsigned char unknown_10;
    unsigned char flags_11;
    unsigned char unknown_12[0x14 - 0x12];
    short unknown_14;
    short unknown_16;
    short unknown_18;
    short unknown_1a;
    unsigned short unknown_1c;
    short bar_length_target;
    short bar_length;
    short bar_value;
    short bar_value_target;
    short unknown_26, unknown_28, unknown_2a, unknown_2c;
    unsigned char unknown_2e[0x30 - 0x2e];
    unsigned long buttons_30;
    unsigned char unknown_34[0x38 - 0x34];
    long tick;
    unsigned char unknown_3c[0x5c - 0x3c];
    volatile unsigned short held_5c;
    unsigned short pressed_5e;
    unsigned char unknown_60[0x65 - 0x60];
    unsigned char unknown_65;
    unsigned char unknown_66[0x6e - 0x66];
    unsigned short enabled_6e;
    unsigned char unknown_70[0x1b30 - 0x70];
    unsigned long unknown_1b30;
} LevelContext;

typedef struct EngineInterfaceView {
    unsigned long signature_000;
    LevelContext *context_004;
} EngineInterfaceView;

typedef struct KindInfo {
    unsigned long sidecar_offset;
    unsigned long unknown;
} KindInfo;

/* Per-object record in the owner context, located through g_kind_info[profile]. */
typedef struct Sidecar {
    unsigned char unknown_00[4];
    long y;
    unsigned char unknown_08[4];
    unsigned char kind;
} Sidecar;

/* 12-byte per-layer visible range (g_eng3_ranges_4078c, track_unit_20010.cpp). */
typedef struct Eng3Range {
    long tick;
    long first;
    long end;
} Eng3Range;

typedef struct Eng3Game {
    unsigned char unknown_000[0x54];
    void (*method_54)(Actor *);
    unsigned char unknown_058[0x2d0 - 0x58];
    void (*method_2d0)(Actor *);
    unsigned char unknown_2d4[0x30c - 0x2d4];
    void (*method_30c)(Actor *);
} Eng3Game;

/* Frame buffer view: two 12-byte primitives ahead of the ordering table. */
typedef struct Eng3Buffer {
    unsigned char unknown_00[0x70];
    unsigned long prim_70[3];
    unsigned long prim_7c[3];
    unsigned long ot[4];
} Eng3Buffer;

typedef struct Rect16 {
    short x, y, w, h;
} Rect16;

typedef struct FramePos {
    short x, y;
} FramePos;

extern EngineInterfaceView g_engine_interface;
extern KindInfo g_kind_info[];
extern Actor *g_focus;
extern Actor *g_eng3_actor_42540;
extern Actor *g_eng3_actor_431d4;
extern Actor *g_eng3_actor_3fb0c;
extern Actor *g_eng3_actor_3fb50;
extern Actor *g_eng3_actor_3fb54;
extern Eng3Game *g_eng3_game_40b8c;
extern Eng3Range *g_eng3_ranges_4078c;
extern Eng3Buffer *g_eng3_frame_41448;
extern Eng3Buffer *g_eng3_frame_4144c;
extern unsigned char *g_eng3_prim_43164;
extern unsigned char g_eng3_image_37ba8[];
extern unsigned char g_eng3_image_37c00[];
extern unsigned char g_eng3_image_37c30[];
extern unsigned char g_eng3_image_37c38[];
extern unsigned char g_eng3_image_37ca0[];
extern unsigned char g_eng3_image_37cd8[];
extern void *g_eng3_images_38080[];
extern long g_eng3_x_38100[];
extern long g_eng3_sprites_38120[];
extern short g_eng3_targets_38068[];
extern void (*g_eng3_mode_handlers_41c80[])(void);
extern unsigned short g_eng3_water_level_410e0;
extern short g_eng3_mode_41cb4;
extern short g_eng3_40b92;
extern short g_eng3_height_40640;
extern signed char g_eng3_layer_40638;
extern signed char g_eng3_bounds_id_40639;
extern long g_eng3_unknown_40580;
extern long g_eng3_405b0;
extern long g_eng3_camera_y_405e4;
extern long g_eng3_camera_z_405e8;
extern long g_eng3_center_y_40620;
extern long g_eng3_unknown_3fb88;
extern long g_eng3_unknown_3fb8c;
extern long g_eng3_unknown_38520;
extern Rect16 g_eng3_rect_35110;
extern Rect16 g_eng3_rect_35118;
extern long g_eng3_3803c;
extern long g_eng3_38040;
extern int g_eng3_37b8c, g_eng3_37b90, g_eng3_37b94, g_eng3_37b98, g_eng3_37b9c, g_eng3_37ba0;
extern int g_eng3_38140;
extern int g_eng3_3fae4;
extern int g_eng3_3fae8;
extern int g_eng3_3faf0;
extern int g_eng3_3faf4;
extern int g_eng3_3faf8;
extern short g_eng3_3fafc;
extern short g_eng3_3fb00;
extern int g_eng3_3fb04;
extern int g_eng3_3fb08;
extern int g_eng3_3fb20;
extern int g_eng3_3fb2c;
extern int g_eng3_3fb34;
extern int g_eng3_3fb40;
extern int g_eng3_3fb44;
extern int g_eng3_3fb9c;
/* HUD cell state at 0x40110 (view from calibration/eng3/handlers_25040.c, extended). */
typedef struct HudCells {
    unsigned short cell[2];
    unsigned short cached[2];
    unsigned short gauge;
    unsigned short unknown_0a;
} HudCells;

extern HudCells g_eng3_hud_40110;
extern int g_eng3_41c64;
extern int g_eng3_41c68;
extern short g_eng3_41c6c;
extern short g_eng3_41c6e;
extern short g_eng3_41c70;
extern short g_eng3_41c72;
extern long g_eng3_41c74;
extern int g_eng3_41cb0;
extern int g_eng3_41cbc;
extern int g_eng3_41cc0[];
extern short g_eng3_41cfc;
extern short g_eng3_41cfe;
extern short g_eng3_41d00;
extern long g_eng3_41d20[];
extern short g_eng3_41d48;
extern int g_eng3_41d50;
extern long g_eng3_43174;

extern void vec_diff_fixed_1420(const int *, const int *, short *);
extern void eng3_072a0(int, int, int, int, int, int, int);
extern void play_sound(unsigned short, Actor *);
extern void eng3_15340(Actor *);
extern int eng3_18370(void);
extern void eng3_180e0(void);
extern void eng3_18600(void);
extern void eng3_18880(void);
extern void eng3_18950(void);
extern void eng3_18c90(void);
extern void eng3_18eb0(void);
extern void eng3_19030(void);
extern void eng3_19130(void);
extern void eng3_195d0(void);
extern void eng3_19b70(void);
extern void eng3_19cd0(void);
extern void eng3_19d40(void);
extern void eng3_empty_2390(void);
extern void eng3_19200(void);
extern void eng3_19a30(void);
extern void eng3_19c20(void);
extern long eng3_1b2e0(long, long, long, int);
extern int eng3_1b650(short *, int, int, long);
extern int eng3_1b920(Actor *, int);
extern Actor *eng3_1cb00(long, long, long, int);
extern void title_08cd0(void *);
extern Actor *attach_actor(Actor *, int);
extern void release_actor(Actor *);
extern int eng3_1dbc0(Actor *);
extern int eng3_1e000(Actor *);
extern void reset_frame_state(Actor *);
extern void select_frame(Actor *, int);
extern int dispatch_local_2a170(long, long, long, int);
extern void advance_planar_motion(Actor *);
extern unsigned int random_limit(int);
extern void eng3_1fbd0(unsigned long, unsigned char *);
extern void eng3_1ff80(unsigned char *, Rect16 *);
extern void eng3_22b30(int, Actor *);
extern Actor *eng3_22bd0(long, long, long, int, int);
extern int title_18600(Actor *, FramePos *);
extern void eng3_284f0(Actor *);

/* Signed-pair animation script step (op < 0: command, else sprite with duration arg). */
typedef struct ScriptStep {
    short op;
    short arg;
} ScriptStep;
/* Per-difficulty damage row at 0x380a0 (32 bytes). */
typedef struct DamageRow {
    long hit_a0, hit_a4, hit_a8, hit_ac, hit_b0;
    long unknown_14[3];
} DamageRow;
/* Profile offset pair at 0x40c40. */
typedef struct ProfileOffset {
    long dx, dy;
} ProfileOffset;
extern Actor *g_eng3_actor_3fb48;
extern void (**g_eng3_callbacks_40ba0)(Actor *);
extern DamageRow g_eng3_damage_380a0[];
extern ProfileOffset g_eng3_offsets_40c40[];
extern long g_eng3_fade_38148[];
extern unsigned char g_eng3_image_37df0[];
extern unsigned char g_eng3_image_37e08[];
extern unsigned char g_eng3_image_37e48[];
extern unsigned char g_eng3_image_37e58[];
extern unsigned char g_eng3_image_37e98[];
extern unsigned char g_eng3_image_37ea0[];
extern unsigned char g_eng3_image_37f50[];
extern unsigned char g_eng3_image_37f88[];
extern unsigned char g_eng3_image_37fb8[];
extern unsigned char g_eng3_image_37fe8[];
extern signed char g_eng3_4063a;
extern signed char g_eng3_iterations_x_40635;
extern long g_eng3_view_angle_40614;
extern long g_eng3_38038;
extern unsigned char g_eng3_4254c;
extern short g_eng3_35168;
extern short g_eng3_3fa40;
extern int g_eng3_3fb24;
extern int g_eng3_3fb28;
extern int g_eng3_3fb68;
extern int g_eng3_3fb6c;
extern int g_eng3_3fb70;
extern int g_eng3_3fb74;
extern int g_eng3_3fb78;
extern int g_eng3_3fb7c;
extern short g_eng3_3fba0;
extern int g_eng3_41cb8;
extern long g_eng3_41d4c;
extern void RemoveFxIdLinks_7f8e(unsigned short, Actor *);
extern void eng3_17ae0(int);
extern void eng3_1a070(void);
extern void eng3_1a110(void);
extern void eng3_1a180(void);
extern void eng3_1a290(void);
extern void unlink_0e4(Actor *);
extern int eng1_29c20(Actor *);
extern int eng3_1ea60(Actor *);
extern int eng3_1eaf0(Actor *);
extern void eng3_1eeb0(Actor *);
extern void eng3_1fb00(void);
extern int eng3_1fb10(int);

/* Functions in address order. */
void eng3_16240(Actor *o)
{
    o->unknown_dc = ((Sidecar *)(o->owner_context + g_kind_info[o->profile].sidecar_offset))->kind;
    o->profile = g_focus->profile;
    o->flags = 2;
    o->render_flags = 0x80000000;
    o->unknown_11e = 0;
    g_eng3_mode_41cb4 = 10;
    g_eng3_actor_42540->image = g_eng3_image_37ca0;
    g_eng3_actor_42540->ticks = -1;
    g_eng3_actor_42540->phase = -1;
    eng3_18370();
    o->unknown_ac = g_focus->unknown_ac;
    o->unknown_a8 = g_focus->unknown_a8;
    o->unknown_b4 = 0;
    o->unknown_b0 = 0;
    o->unknown_cc = 0;
    o->unknown_c8 = 0;
    o->unknown_d0 = 0;
    o->unknown_d4 = 0;
    o->unknown_98 = 1;
    g_eng3_3faf0 = 1;
}

void eng3_16320(Actor *o)
{
    short d[6];

    o->sprite = 0;
    g_eng3_actor_431d4 = o;
    g_eng3_3803c = 0x140;
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (g_engine_interface.context_004->level != 2 && g_engine_interface.context_004->level != 10) {
        if (o->y > 0x17f0000)
            o->y -= 0x10000;
        if (g_focus->image == g_eng3_image_37c38 || g_focus->image == g_eng3_image_37ca0
            || g_focus->image == g_eng3_image_37cd8) {
            if (o->z > -0x400000)
                o->z -= 0x20000;
            if (o->z < -0x400000)
                o->z += 0x20000;
            g_focus->z = o->z;
        }
    } else if (g_focus->image == g_eng3_image_37c38 || g_focus->image == g_eng3_image_37ca0
               || g_focus->image == g_eng3_image_37cd8) {
        if (o->z > 0)
            o->z -= 0x20000;
        if (o->z < 0)
            o->z += 0x20000;
        g_focus->z = o->z;
    }
    o->unknown_cc = 0;
    advance_planar_motion(o);
    if (g_engine_interface.context_004->level == 2)
        o->unknown_ac -= 0x2000;
    if (g_engine_interface.context_004->level == 10)
        o->unknown_ac -= 0x4000;
    if (g_engine_interface.context_004->level == 8)
        o->unknown_ac -= 0x2000;
    if (g_engine_interface.context_004->level == 8) {
        if (o->unknown_ac <= 0)
            o->unknown_ac = 0;
    } else {
        if (g_engine_interface.context_004->level == 2 && (o->unknown_ac <= 0 || d[0] >= -2))
            o->unknown_ac = 0;
        if (g_engine_interface.context_004->level == 10 && (o->unknown_ac <= 0 || d[0] >= 5))
            o->unknown_ac = 0;
    }
    if (g_engine_interface.context_004->level == 2) {
        if (g_eng3_37b8c) {
            g_engine_interface.context_004->unknown_03 = 2;
            g_eng3_37b8c = 0;
        }
        if (g_focus->x >= 0x3c000000 && g_eng3_37b90) {
            eng3_19c20();
            g_eng3_37b90 = 0;
        }
    }
    if (g_engine_interface.context_004->level == 8) {
        if (g_eng3_37b94)
            g_eng3_37b94 = 0;
        if (g_focus->x >= 0x77800000 && g_eng3_37b98) {
            eng3_19c20();
            g_eng3_37b98 = 0;
        }
    }
    if (g_engine_interface.context_004->level == 10) {
        if (g_eng3_37b9c) {
            g_engine_interface.context_004->unknown_03 = 2;
            g_eng3_37b9c = 0;
        }
        if (g_focus->x >= 0x74000000 && g_eng3_37ba0) {
            eng3_19c20();
            g_eng3_37ba0 = 0;
        }
    }
}

static __inline long range_first(int layer)
{
    long v;

    if (g_eng3_ranges_4078c[layer].tick != g_engine_interface.context_004->tick)
        return -1;
    v = g_eng3_ranges_4078c[layer].first;
    if (v < 0)
        v = 0;
    return v << 7;
}

static __inline long range_end(int layer)
{
    long v;

    if (g_eng3_ranges_4078c[layer].tick != g_engine_interface.context_004->tick)
        return -1;
    v = g_eng3_ranges_4078c[layer].end;
    if (v < 0)
        v = 0;
    return v << 7;
}

int eng3_16550(Actor *o)
{
    long lo, hi;

    if (g_eng3_40b92)
        return 0;
    lo = range_first(o->profile);
    if (lo == -1)
        return 0;
    hi = range_end(o->profile);
    if (hi == -1)
        return 0;
    hi += 0x100;
    if (o->x >> 16 < lo || o->x >> 16 > hi)
        return 1;
    return 0;
}

void eng3_165f0(Actor *o)
{
    o->flags = 2;
    if (((o->kind + 6) & 0xf) == g_engine_interface.context_004->unknown_02)
        o->image = g_eng3_image_37ba8;
    else
        o->image = g_eng3_image_37c00;
    o->state = 1;
    reset_frame_state(o);
    eng3_1e000(o);
    if (g_engine_interface.context_004->level != 2 && g_engine_interface.context_004->level != 8) {
        o->y = -0x100000;
        eng3_15340(o);
    } else {
        o->y = 0x1a00000;
        eng3_15340(o);
    }
}

void eng3_16670(Actor *o)
{
    short d[6];
    FramePos pos;
    Actor *a;

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->image == g_eng3_image_37c00) {
        if (d[0] < 700 && o->profile == g_focus->profile) {
            if (o->state) {
                play_sound(0x6028, o);
                o->state = 0;
            }
            o->image = g_eng3_image_37ba8;
            g_engine_interface.context_004->unknown_02 = (o->kind + 6) & 0xf;
            reset_frame_state(o);
        } else {
            eng3_1b2e0(o->x >> 16, o->y >> 16, o->z >> 16, o->profile);
            o->state += 4;
            eng3_15340(o);
        }
    } else if (g_engine_interface.context_004->unknown_02 != ((o->kind + 6) & 0xf)) {
        o->image = g_eng3_image_37c00;
        reset_frame_state(o);
    }
    if (g_engine_interface.context_004->unknown_02 == ((o->kind + 6) & 0xf)) {
        title_18600(o, &pos);
        if (g_engine_interface.context_004->tick & 1) {
            a = eng3_22bd0(o->x, o->y + ((random_limit(3) + pos.y) << 16),
                           o->z + ((random_limit(3) + pos.x) << 16), 0x201b, 0);
            if (a) {
                a->state = 0;
                a->profile = o->profile;
            }
            a = eng3_22bd0(o->x, o->y + ((random_limit(3) + pos.y) << 16),
                           o->z + ((random_limit(3) + pos.x) << 16), 0x201b, 0);
            if (a) {
                a->state = 1;
                a->profile = o->profile;
            }
            a = eng3_22bd0(o->x, o->y + ((random_limit(3) + pos.y) << 16),
                           o->z + ((random_limit(3) + pos.x) << 16), 0x201b, 0);
            if (a) {
                a->state = 2;
                a->profile = o->profile;
            }
        }
    }
    eng3_1e000(o);
    if (eng3_1b920(o, 0x100))
        release_actor(o);
}

void eng3_168b0(Actor *o)
{
    o->render_flags |= 5;
    o->unknown_74 += random_limit(0x400);
    o->unknown_4a = 0x80;
    o->category = 1;
    o->image = g_eng3_image_37c30;
    o->phase = 0;
}

void eng3_168f0(Actor *o)
{
    long t[16];

    t[0] = 0;
    t[1] = 0;
    t[2] = 2;
    t[3] = 0;
    t[4] = 1;
    t[5] = 0;
    t[6] = 0;
    t[7] = 0;
    t[8] = 0;
    t[9] = 0;
    t[10] = -2;
    t[11] = 0;
    t[12] = -1;
    t[13] = 0;
    t[14] = 0;
    t[15] = 0;
    if (o->state)
        o->y += (unsigned short)o->state << 16;
    o->state++;
    o->z += t[o->state & 0xf] << 17;
    o->unknown_74 += 4;
    if (o->unknown_4a > 0x10)
        o->unknown_4a -= 0x10;
    else
        o->unknown_4a = 0;
    eng3_1e000(o);
}

void eng3_169c0(Actor *o)
{
    if (g_engine_interface.context_004->level == 2) {
        if (g_focus->y > g_eng3_water_level_410e0 << 16) {
            if (g_focus->unknown_98 == 0) {
                play_sound(0x6035, g_focus);
                g_eng3_3faf4 = 0;
                g_focus->unknown_98 = 1;
            }
            play_sound(0x6034, g_focus);
            return;
        }
        if (g_focus->unknown_98 == 1) {
            g_focus->unknown_98 = 0;
            g_eng3_3faf4 = 3;
        }
        if (g_eng3_3faf4 > 0) {
            if (g_eng3_3faf4 > 2 && g_eng3_3faf4 <= 3)
                play_sound(0x6031, g_focus);
            if (g_eng3_3faf4 > 1 && g_eng3_3faf4 <= 2)
                play_sound(0x6030, g_focus);
            if (g_eng3_3faf4 > 0 && g_eng3_3faf4 <= 1)
                play_sound(0x603a, g_focus);
            g_eng3_3faf4--;
            return;
        }
        play_sound(0x603a, g_focus);
        return;
    }
    play_sound(0x603a, g_focus);
}

void eng3_16af0(Actor *o)
{
    if (g_engine_interface.context_004->level == 2) {
        if (g_focus->y > g_eng3_water_level_410e0 << 16) {
            if (g_focus->unknown_e0 == 0) {
                g_focus->unknown_e0 = 1;
                g_eng3_3faf8 = 0;
            }
            return;
        }
        if (g_focus->unknown_e0 == 1) {
            g_focus->unknown_e0 = 0;
            g_eng3_3faf8 = 3;
        }
        if (g_eng3_3faf8 > 0) {
            if (g_eng3_3faf8 > 2 && g_eng3_3faf8 <= 3)
                play_sound(0x6031, g_focus);
            if (g_eng3_3faf8 > 1 && g_eng3_3faf8 <= 2)
                play_sound(0x602f, g_focus);
            if (g_eng3_3faf8 > 0 && g_eng3_3faf8 <= 1)
                play_sound(0x6039, g_focus);
            g_eng3_3faf8--;
            return;
        }
        play_sound(0x6039, g_focus);
        return;
    }
    play_sound(0x6039, g_focus);
}

void eng3_16c00(Actor *o)
{
    if (g_focus->unknown_ec || o->unknown_116 == 0x10 || o->unknown_116 == 0x20)
        play_sound(0x6037, g_focus);
}

void eng3_16c40(void)
{
    long x, y, w, rest;

    x = ((g_eng3_405b0 >> 1) + (g_eng3_camera_z_405e8 >> 4)) & 0x1ff;
    y = g_eng3_unknown_40580 / 2 - g_eng3_camera_y_405e4 / 10 + g_eng3_center_y_40620 - 0x108;
    if (x < 0xff) {
        w = 0x100 - x;
        rest = 0x40 - w;
    } else {
        w = 0x200 - x;
        rest = 0x40 - w;
    }
    if (g_eng3_unknown_3fb8c) {
        if (g_eng3_unknown_38520 == 0x1e0)
            g_eng3_unknown_38520 = 0;
        if (g_eng3_unknown_3fb88 < g_eng3_unknown_38520)
            g_eng3_unknown_3fb88 = 0xf0;
        if (g_eng3_unknown_3fb88 > 0xf0)
            g_eng3_unknown_3fb88 = 0xf0;
        g_eng3_rect_35110.y = (g_eng3_frame_41448 != g_eng3_frame_4144c) * 0x100;
        eng3_1ff80(g_eng3_prim_43164, &g_eng3_rect_35110);
        eng3_1fbd0(g_eng3_frame_41448->ot[3], g_eng3_prim_43164);
        g_eng3_prim_43164 += 0xc;
        eng3_072a0(0, y + 0x100, w, 0x100, x, 0, 1);
        eng3_072a0(w, y + 0x100, 0x100, 0x100, 0, 0, 1);
        g_eng3_rect_35118.x = 0;
        g_eng3_rect_35118.w = 0x140;
        g_eng3_rect_35118.y = (short)g_eng3_unknown_38520 + g_eng3_rect_35110.y;
        g_eng3_rect_35118.h = g_eng3_unknown_3fb88 - g_eng3_unknown_38520;
        eng3_1ff80(g_eng3_prim_43164, &g_eng3_rect_35118);
        eng3_1fbd0(g_eng3_frame_41448->ot[2], g_eng3_prim_43164);
        g_eng3_prim_43164 += 0xc;
    }
    eng3_072a0(0, y, w, 0x100, x, 0, 0);
    eng3_072a0(w, y, 0x100, 0x100, 0, 0, 0);
    if (rest > 0)
        eng3_072a0(w + 0x100, y, rest, 0x100, 0, 0, 0);
    g_eng3_frame_41448->prim_70[0] = g_eng3_frame_41448->ot[1];
    g_eng3_frame_41448->ot[1] = (unsigned long)g_eng3_frame_41448->prim_70;
    g_eng3_frame_41448->prim_7c[0] = g_eng3_frame_41448->ot[2];
    g_eng3_frame_41448->ot[2] = (unsigned long)g_eng3_frame_41448->prim_7c;
}

void eng3_16ea0(void)
{
    g_engine_interface.context_004->bar_length_target = 0x80;
    g_engine_interface.context_004->bar_length = 0;
    g_engine_interface.context_004->bar_value = 0;
    if (g_engine_interface.context_004->unknown_0c == 0)
        g_engine_interface.context_004->unknown_0c = 1;
    if (g_engine_interface.context_004->unknown_0c > 9)
        g_engine_interface.context_004->unknown_0c = 9;
    g_engine_interface.context_004->bar_value_target = 0x80;
    g_engine_interface.context_004->unknown_1c = 0;
    g_engine_interface.context_004->flags_11 = 0;
    g_engine_interface.context_004->unknown_14 = 0;
    g_engine_interface.context_004->unknown_18 = 0;
    g_engine_interface.context_004->unknown_0d = 0;
    g_eng3_41d50 = 0;
    g_eng3_41cb0 = 0;
}

void eng3_17090(Actor *a, int i);

void eng3_16f30(void)
{
    int i;
    Actor *a;

    switch (g_eng3_41d50) {
    case 0:
        for (i = 2; i < 10; i++)
            g_eng3_actor_42540[i].y = 0;
        return;
    case 1:
        g_eng3_41d50++;
        g_eng3_3fafc = 0x8c;
        for (i = 0; i < 8; i++) {
            a = &g_eng3_actor_42540[i + 2];
            a->y = 0;
            a->state = i * 4;
            a->parameter = 0x320000;
            if (i == g_eng3_41cb0) {
                a->unknown_21 = 0x3c;
                a->image = g_eng3_images_38080[i];
            } else {
                a->sprite = g_eng3_sprites_38120[i];
            }
        }
        break;
    case 2:
        if (--g_eng3_3fafc <= 0)
            g_eng3_41d50++;
        break;
    case 3:
        g_eng3_41d50++;
        g_eng3_3fafc = 0x64;
        for (i = 0; i < 8; i++) {
            a = &g_eng3_actor_42540[i + 2];
            a->state = i * 4;
            a->parameter = 0;
        }
        break;
    case 4:
        if (--g_eng3_3fafc <= 0)
            g_eng3_41d50 = 0;
        break;
    }
    for (i = 0; i < 8; i++)
        eng3_17090(&g_eng3_actor_42540[i + 2], i);
}

void eng3_17090(Actor *a, int i)
{
    int n;

    if (a->state) {
        if (--a->state == 0)
            play_sound(0x600d, a);
    } else {
        for (n = 0; n < 8; n++) {
            if (a->y < a->parameter)
                a->y += 0x10000;
            if (a->y > a->parameter)
                a->y -= 0x10000;
        }
    }
    if (i == g_eng3_41cb0) {
        eng3_1dbc0(a);
        if (a->unknown_21)
            a->unknown_21--;
    }
    if (g_engine_interface.context_004->unknown_1c & (1 << i))
        a->render_flags &= ~7;
    else
        a->render_flags |= 7;
}

void eng3_17130(void)
{
    int i;
    Actor *a;

    for (i = 0; i < 8; i++) {
        a = &g_eng3_actor_42540[i + 2];
        title_08cd0(a);
        a->kind = 6;
        a->render_flags = 0xc0000000;
        a->unknown_3e = 4;
        a->mode = 6;
        a->sprite = g_eng3_sprites_38120[i];
        a->category = 8;
        a->x = g_eng3_x_38100[i];
        a->y = 0;
        a->z = 0;
    }
}

void eng3_171a0(void)
{
    g_eng3_hud_40110.cell[1] = 0xb9;
    g_eng3_41c70 = 0xb9;
    memset(g_eng3_41d20, 0, 6 * sizeof(long));
    g_eng3_hud_40110.unknown_0a = 0;
    g_eng3_hud_40110.cell[0] = 0x7d;
    g_eng3_41c6e = 0x7d;
    g_eng3_41cfc = 0;
    g_eng3_41cfe = 0;
    g_eng3_41c6c = 0;
    g_eng3_41d00 = 0x64;
    g_eng3_38140 = 0;
    g_eng3_41c72 = 0;
}

void eng3_17210(void)
{
    if (g_eng3_41c72 && g_eng3_41d00 > 10)
        g_eng3_41d00 = 10;
    if (g_eng3_41d00 > 0x200)
        g_eng3_41d00 = 0x200;
    if (--g_eng3_41d00 == 0) {
        if (!g_eng3_41c72 && g_eng3_41c6e != 0x7d) {
            g_eng3_41c6e = 0x7d;
            g_eng3_41cfe = 0;
            g_eng3_41d00 = 0x7f;
        } else {
            if (g_engine_interface.context_004->unknown_0c > 9)
                g_engine_interface.context_004->unknown_0c = 9;
            g_eng3_41c6e = g_eng3_targets_38068[g_engine_interface.context_004->unknown_0c];
            g_eng3_41cfe = 1;
            g_eng3_41d00 = 0x7f;
            g_eng3_41c72 = 0;
        }
    }
    if (g_eng3_3fae4 == 1) {
        g_eng3_41c70 = 0x95;
        g_eng3_41cfc = 1;
        g_eng3_3fae4 = 4;
    }
    if (g_eng3_3fae4 == 2) {
        g_eng3_41c70 = 0xb9;
        g_eng3_41cfc = 1;
        g_eng3_3fae4 = 3;
    }
    if (g_eng3_3fae8 == 1) {
        g_eng3_41c70 = 0xa1;
        g_eng3_41cfc = 1;
        g_eng3_3fae8 = 0;
    }
    if (g_eng3_3fae8 == 2) {
        g_eng3_41cfc = 1;
        g_eng3_41c70 = 0xb9;
        g_eng3_3fae8 = 0;
    }
    if (--g_eng3_3fb00 & 1) {
        if (g_eng3_hud_40110.cell[0] != g_eng3_41c6e) {
            if (g_eng3_41cfe) {
                g_eng3_hud_40110.cell[0]++;
                if (g_eng3_hud_40110.cell[0] % 12 == 0)
                    g_eng3_hud_40110.cell[0] = g_eng3_41c6e - 5;
            } else {
                g_eng3_hud_40110.cell[0]--;
                if (g_eng3_hud_40110.cell[0] % 12 == 0)
                    g_eng3_hud_40110.cell[0] = g_eng3_41c6e + 6;
            }
        }
    }
    if (g_eng3_hud_40110.cell[1] != g_eng3_41c70) {
        if (g_eng3_38140-- < 0) {
            g_eng3_38140 = 0;
            if (g_eng3_41cfc) {
                g_eng3_hud_40110.cell[1]++;
                if (g_eng3_hud_40110.cell[1] % 12 == 0)
                    g_eng3_hud_40110.cell[1] = g_eng3_41c70 - 5;
            } else {
                g_eng3_hud_40110.cell[1]--;
                if (g_eng3_hud_40110.cell[1] % 12 == 0)
                    g_eng3_hud_40110.cell[1] = g_eng3_41c70 + 6;
            }
        }
    }
    if (g_eng3_3fb20 == 0)
        g_focus->unknown_ee = 0;
    if (g_eng3_3fae4 == 4 || g_eng3_mode_41cb4 == 6)
        g_eng3_hud_40110.gauge = (g_focus->unknown_ec * 0x14666) >> 16;
    if (g_eng3_3fb20 || g_eng3_mode_41cb4 == 6)
        g_eng3_hud_40110.gauge = (g_focus->unknown_ee * 0x2f711) >> 16;
}

void eng3_174e0(void)
{
    short bounds[7];

    g_focus->category = 1;
    if (g_focus->profile > g_eng3_3fb04) {
        g_eng3_41cc0[++g_eng3_3fb08] = g_focus->profile;
        g_eng3_3fb04 = g_focus->profile;
    }
    if (g_eng3_height_40640 <= g_eng3_3803c)
        g_eng3_height_40640 += g_eng3_38040;
    if (g_eng3_height_40640 >= g_eng3_3803c)
        g_eng3_height_40640 -= g_eng3_38040;
    if (g_engine_interface.context_004->level == 8) {
        if (g_eng3_layer_40638 == 0) {
            g_eng3_41c64 = 0;
            g_eng3_41c68 = 1;
        }
        if (g_eng3_layer_40638 == 1) {
            g_eng3_41c64 = 3;
            g_eng3_41c68 = 4;
        }
        if (g_eng3_layer_40638 == 2) {
            g_eng3_41c64 = 6;
            g_eng3_41c68 = 7;
        }
    }
    if (g_engine_interface.context_004->level == 2) {
        if (g_eng3_layer_40638 == 0) {
            g_eng3_41c64 = 0;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 1) {
            g_eng3_41c64 = 1;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 2) {
            g_eng3_41c64 = 2;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 3) {
            g_eng3_41c64 = 3;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 4) {
            g_eng3_41c64 = 4;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 5) {
            g_eng3_41c64 = 5;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 6) {
            g_eng3_41c64 = 6;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 7) {
            g_eng3_41c64 = 7;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 8) {
            g_eng3_41c64 = 7;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 9) {
            g_eng3_41c64 = 7;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 10) {
            g_eng3_41c64 = 7;
            g_eng3_41c68 = 999;
        }
    }
    if (g_engine_interface.context_004->level == 10) {
        if (g_eng3_layer_40638 == 0) {
            g_eng3_41c64 = 4;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 1) {
            g_eng3_41c64 = 0;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 2) {
            g_eng3_41c64 = 1;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 3) {
            g_eng3_41c64 = 2;
            g_eng3_41c68 = 999;
        }
        if (g_eng3_layer_40638 == 4) {
            g_eng3_41c64 = 3;
            g_eng3_41c68 = 999;
        }
    }
    if ((g_engine_interface.context_004->unknown_65 & 4) && (g_engine_interface.context_004->buttons_30 & 0x4000))
        eng3_19a30();
    if ((g_engine_interface.context_004->unknown_65 & 1) && (g_engine_interface.context_004->buttons_30 & 0x4000)) {
        eng3_19a30();
        if (g_engine_interface.context_004->unknown_10 == 0)
            g_engine_interface.context_004->unknown_10 = 1;
    }
    if ((g_engine_interface.context_004->unknown_65 & 2) && (g_engine_interface.context_004->buttons_30 & 0x4000)) {
        eng3_19a30();
        g_engine_interface.context_004->unknown_10 = 0;
    }
    if ((g_engine_interface.context_004->unknown_65 & 8) && (g_engine_interface.context_004->buttons_30 & 0x2000))
        eng3_19c20();
    if (g_focus->unknown_ec)
        g_focus->unknown_ec--;
    else
        g_focus->unknown_ed = 0;
    eng3_22b30(dispatch_local_2a170(g_eng3_actor_42540->x, g_eng3_actor_42540->y, g_focus->z, g_focus->profile),
               g_focus);
    eng3_180e0();
    eng3_19130();
    g_eng3_mode_handlers_41c80[g_eng3_mode_41cb4 & 0x7fff]();
    if (g_engine_interface.context_004->level == 8 && g_eng3_bounds_id_40639 == g_eng3_41c68) {
        if (g_focus->y <= 0x1c00000 && g_focus->y > 0x1400000)
            g_eng3_3fb2c = 1;
        else
            g_eng3_3fb2c = 0;
        if (eng3_1b650(bounds, g_eng3_41c68, 0x19, g_focus->x >> 16)) {
            if (g_focus->z <= bounds[0] << 16)
                g_focus->z = bounds[0] << 16;
            if (g_focus->z >= bounds[2] << 16)
                g_focus->z = bounds[2] << 16;
        }
    }
    if (g_focus->y >= 0x1c00000)
        g_eng3_3fb2c = 0;
    eng3_17210();
    eng3_16f30();
}

void eng3_178a0(void)
{
    g_eng3_actor_42540->category = 1;
    g_eng3_mode_handlers_41c80[0] = eng3_18600;
    g_eng3_mode_handlers_41c80[1] = eng3_18950;
    g_eng3_mode_handlers_41c80[2] = eng3_18eb0;
    g_eng3_mode_handlers_41c80[3] = eng3_19030;
    g_eng3_mode_handlers_41c80[4] = eng3_empty_2390;
    g_eng3_mode_handlers_41c80[5] = eng3_195d0;
    g_eng3_mode_handlers_41c80[6] = eng3_19b70;
    g_eng3_mode_handlers_41c80[7] = eng3_19d40;
    g_eng3_mode_handlers_41c80[8] = eng3_18880;
    g_eng3_mode_handlers_41c80[9] = eng3_19cd0;
    g_eng3_mode_handlers_41c80[10] = eng3_18c90;
    if (g_engine_interface.context_004->unknown_0c == 0 || g_engine_interface.context_004->unknown_0c > 9)
        g_engine_interface.context_004->unknown_0c = 1;
    g_engine_interface.context_004->bar_value_target = 0x40;
    g_engine_interface.context_004->unknown_26 = (g_engine_interface.context_004->buttons_30 & 0x200) ? 0xff : 0;
    g_engine_interface.context_004->unknown_28 = (g_engine_interface.context_004->buttons_30 & 0x400) ? 0xff : 0;
    g_engine_interface.context_004->unknown_2a = (g_engine_interface.context_004->buttons_30 & 0x800) ? 0xff : 0;
    g_engine_interface.context_004->unknown_2c = (g_engine_interface.context_004->buttons_30 & 0x1000) ? 0xff : 0;
    eng3_171a0();
    g_eng3_mode_41cb4 = 0;
    g_engine_interface.context_004->unknown_05 = 0;
    g_engine_interface.context_004->unknown_04 = 0;
    g_eng3_41d48 = 0;
    g_eng3_41cbc = 0;
    g_focus = g_eng3_actor_42540;
    title_08cd0(g_eng3_actor_42540);
    g_eng3_3fb34 = 0;
    g_focus->kind = 1;
    g_eng3_actor_42540->category = 1;
    g_focus->unknown_ee = 0x384;
    g_eng3_mode_handlers_41c80[g_eng3_mode_41cb4 & 0x7fff]();
    g_eng3_41c74 = g_eng3_43174;
    attach_actor(g_focus, 0x2018);
    g_focus->unknown_dc = 0;
    g_engine_interface.context_004->bar_value_target = 0x80;
    g_focus->profile = 0;
    g_focus->unknown_ec = 0;
    g_focus->unknown_98 = 0;
    g_focus->unknown_e0 = 0;
    g_focus->unknown_d8 = 0;
    g_focus->unknown_126 = 0;
    eng3_171a0();
    if ((g_engine_interface.context_004->level == 2 || g_engine_interface.context_004->level == 8)
        && g_focus->y != 0x1c00000)
        g_focus->y = 0x1c00000;
    if (g_engine_interface.context_004->level == 8)
        g_focus->unknown_1c = 10;
}

void eng3_17ae0(int code)
{
    int tile;

    tile = 0;
    switch (code) {
    case 0x57:
        g_eng3_actor_3fb0c->state = 0x41;
        if (g_eng3_actor_3fb0c->unknown_d8 == 0)
            select_frame(g_eng3_actor_3fb0c, 0);
        if (g_eng3_actor_3fb0c->unknown_d8 == 1)
            select_frame(g_eng3_actor_3fb0c, 2);
        break;
    case 0x53:
        g_eng3_actor_3fb0c->state = 0x42;
        break;
    case 0x5b:
        g_eng3_actor_3fb0c->state = 0x43;
        select_frame(g_eng3_actor_3fb0c, 8);
        break;
    case 0x54:
        g_eng3_actor_3fb0c->state = 0x44;
        select_frame(g_eng3_actor_3fb0c, 5);
        break;
    case 0x50:
        g_eng3_3fb9c = g_eng3_41c68;
        break;
    case 0x51:
        g_eng3_3fb9c = g_eng3_41c64;
        break;
    case 0x63:
        if (g_focus->y >= 0x7c0000) {
            if (g_focus->unknown_e0 == 0) {
                g_eng3_actor_3fb50 = &g_eng3_actor_42540[14];
                title_08cd0(g_eng3_actor_3fb50);
                g_eng3_game_40b8c->method_2d0(g_eng3_actor_3fb50);
                g_eng3_actor_3fb50->x = g_focus->x;
                g_eng3_actor_3fb50->y = g_focus->y - 0x500000;
                g_eng3_actor_3fb50->z = g_focus->z;
                g_eng3_actor_3fb50->kind = 0x3c;
                g_focus->unknown_e0 = 1;
            }
            if (g_focus->unknown_e0)
                eng3_284f0(g_eng3_actor_3fb50);
        }
        break;
    case 0x77:
        if (g_focus->y >= 0x7c0000) {
            if (g_focus->unknown_e0 == 0) {
                g_eng3_actor_3fb54 = &g_eng3_actor_42540[14];
                title_08cd0(g_eng3_actor_3fb54);
                g_eng3_game_40b8c->method_2d0(g_eng3_actor_3fb54);
                g_eng3_actor_3fb54->x = g_focus->x;
                g_eng3_actor_3fb54->y = g_focus->y - 0x500000;
                g_eng3_actor_3fb54->z = g_focus->z;
                g_eng3_actor_3fb54->kind = 0x3c;
                g_focus->unknown_e0 = 1;
            }
            if (g_focus->unknown_e0)
                eng3_284f0(g_eng3_actor_3fb54);
        }
        break;
    case 0x76:
        g_focus->unknown_ac = 0x70000;
        g_focus->unknown_a8 = 0x70000;
        if (g_focus->unknown_dc == 0) {
            g_eng3_game_40b8c->method_54(eng3_1cb00(g_focus->x, g_focus->y, g_focus->z, 7));
            g_focus->unknown_dc = 1;
        }
        break;
    case 0x75:
        if (g_focus->y >= 0x1f40000 && g_focus->unknown_e0 == 0) {
            g_eng3_game_40b8c->method_30c(eng3_1cb00(g_focus->x, g_focus->y, g_focus->z, 0x41));
            g_focus->unknown_e0 = 1;
        }
        break;
    case 0x64:
        g_eng3_3fb2c = 1;
        break;
    case 0x65:
        g_eng3_3fb2c = 0;
        break;
    }
    if (g_engine_interface.context_004->level == 8 || g_engine_interface.context_004->level == 10) {
        if (g_eng3_actor_42540->z < 0)
            tile = dispatch_local_2a170(g_eng3_actor_42540->x + 0x800000, g_eng3_actor_42540->y,
                                        g_focus->z - 0x100000, g_focus->profile);
        else
            tile = dispatch_local_2a170(g_eng3_actor_42540->x + 0x800000, g_eng3_actor_42540->y,
                                        g_focus->z + 0x100000, g_focus->profile);
    }
    if (g_engine_interface.context_004->level == 2) {
        if (g_eng3_actor_42540->z < 0)
            tile = dispatch_local_2a170(g_eng3_actor_42540->x + 0x400000, g_eng3_actor_42540->y,
                                        g_focus->z - 0x40000, g_focus->profile);
        else
            tile = dispatch_local_2a170(g_eng3_actor_42540->x + 0x400000, g_eng3_actor_42540->y,
                                        g_focus->z + 0x40000, g_focus->profile);
    }
    switch (tile) {
    case 0x5d:
        g_eng3_3fb44 = 1;
        g_eng3_3fb40 = 0;
        break;
    case 0x5e:
        g_eng3_3fb40 = 1;
        g_eng3_3fb44 = 0;
        break;
    case 0x55:
        if (g_focus->z > 0)
            g_eng3_3fb44 = 1;
        else
            g_eng3_3fb40 = 1;
        break;
    case 0x5a:
        if (g_focus->z > 0)
            g_eng3_3fb44 = 1;
        else
            g_eng3_3fb40 = 1;
        break;
    case 0x56:
        g_eng3_3fb44 = 1;
        g_eng3_3fb40 = 0;
        g_focus->unknown_116 = 0x10;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
        break;
    case 0x52:
        g_eng3_3fb44 = 1;
        g_eng3_3fb40 = 0;
        g_focus->unknown_116 = 0x20;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
        break;
    case 0x7a:
        g_eng3_3fb44 = 1;
        g_eng3_3fb40 = 1;
        break;
    case 0x7b:
        g_eng3_3fb44 = 1;
        g_eng3_3fb40 = 1;
        break;
    }
    if (dispatch_local_2a170(g_eng3_actor_42540->x + 0x600000, g_eng3_actor_42540->y, g_focus->z, g_focus->profile)
            == 0x73
        && g_focus->y > 0x1c00000) {
        g_focus->unknown_116 = 0x80;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    if (g_engine_interface.context_004->level == 8 && g_focus->z < 0) {
        tile = dispatch_local_2a170(g_eng3_actor_42540->x + 0x800000, g_eng3_actor_42540->y,
                                    g_focus->z + 0x100000, g_focus->profile);
        switch (tile) {
        case 0x5d:
            g_eng3_3fb44 = 1;
            g_eng3_3fb40 = 0;
            break;
        case 0x56:
            g_eng3_3fb44 = 1;
            g_eng3_3fb40 = 0;
            if ((g_focus->x & 0x7f0000) > 0x200000) {
                g_focus->unknown_116 = 0x10;
                if (g_eng3_mode_41cb4 != 5)
                    eng3_19200();
            }
            break;
        }
    }
}
int eng3_18630(void);
void eng3_18690(void);
void eng3_18790(void);
void eng3_18bc0(void);
void eng3_18d00(void);
int eng3_18dc0(void);
void eng3_18de0(void);
void eng3_18f70(void);
void eng3_19100(void);
void eng3_19410(short kind);
void eng3_19a30(void);
void eng3_19f30(void);
void eng3_19fd0(void);
void eng3_185c0(void);
void eng3_18350(int tile, short *bounds);


void eng3_180e0(void)
{
    short bounds[8];
    int tile;

    eng3_1a290();
    g_eng3_actor_42540->prev_x = g_eng3_actor_42540->x;
    g_eng3_actor_42540->prev_y = g_eng3_actor_42540->y;
    g_eng3_actor_42540->prev_z = g_eng3_actor_42540->z;
    if (g_eng3_mode_41cb4 != 7) {
        if (g_eng3_mode_41cb4 == 6)
            return;
        if (g_engine_interface.context_004->buttons_30 & 0x8000) {
            if (g_eng3_41d48 < 0) {
                g_eng3_41d48++;
            } else if (g_engine_interface.context_004->held_5c & 0xc00) {
                g_eng3_mode_41cb4 = 7;
                g_eng3_actor_42540->sprite = 0x32;
                g_eng3_actor_42540->category = 1;
                g_eng3_41d48 = 0;
                g_eng3_41d4c = (g_engine_interface.context_004->held_5c & 0x400) ? 0x100000 : 0x400000;
                g_eng3_actor_42540->unknown_bc = g_eng3_41d4c;
                g_eng3_actor_42540->unknown_ac = g_eng3_41d4c;
                g_eng3_actor_42540->unknown_b8 = 0;
                g_eng3_actor_42540->unknown_a8 = 0;
                g_eng3_actor_42540->unknown_c0 = 0x10000;
                g_eng3_actor_42540->unknown_b0 = 0x10000;
                unlink_0e4(g_eng3_actor_42540);
                return;
            }
        }
    }
    tile = dispatch_local_2a170(g_eng3_actor_42540->x, g_eng3_actor_42540->y, g_eng3_actor_42540->z, g_focus->profile);
    eng3_17ae0(tile);
    if (g_eng3_actor_3fb48)
        g_eng3_actor_3fb48->profile = g_focus->profile;
    g_eng3_bounds_id_40639 = g_eng3_3fb9c;
    g_eng3_4063a = 0x3e;
    eng3_1b650(bounds, g_eng3_3fb9c, 0x19, g_focus->x >> 16);
    if (g_engine_interface.context_004->level == 2 && g_focus->x < 0x3000000) {
        if ((g_focus->profile == 2 || g_focus->profile == 6 || g_focus->profile == 4) && g_focus->z <= 0x200000)
            g_focus->z = 0x200000;
        if ((g_focus->profile == 1 || g_focus->profile == 5 || g_focus->profile == 3) && g_focus->z >= -0x200000)
            g_focus->z = -0x200000;
    }
    if (g_engine_interface.context_004->level == 10 && g_focus->x < 0x3000000) {
        if (g_focus->profile == 2 && g_focus->z <= 0x200000)
            g_focus->z = 0x200000;
        if (g_focus->profile == 1 && g_focus->z >= -0x200000)
            g_focus->z = -0x200000;
    }
    if (g_focus->unknown_ee > 0)
        g_focus->unknown_ee--;
    eng3_18350(tile, bounds);
}

void eng3_18350(int tile, short *bounds)
{
    g_eng3_center_y_40620 = 0xe8 - bounds[4];
}

/* Signed-pair script runner for the player actor (ops -1..-40); returns 1 on op -3. */
int eng3_18370(void)
{
    short op;

    if (--g_eng3_actor_42540->ticks >= 0)
        return 0;
    do {
        g_eng3_actor_42540->phase++;
        op = ((ScriptStep *)g_eng3_actor_42540->image)[g_eng3_actor_42540->phase].op;
        if (op < 0)
        switch (op) {
        case -5:
            if (!(g_engine_interface.context_004->held_5c & 0x40))
                g_eng3_actor_42540->image = (ScriptStep *)g_eng3_actor_42540->image + g_eng3_actor_42540->phase + 1;
            g_eng3_actor_42540->phase = -1;
            break;
        case -9:
            if (g_engine_interface.context_004->held_5c & 0x10)
                goto restart;
        case -2:
            g_eng3_actor_42540->image = (ScriptStep *)g_eng3_actor_42540->image + g_eng3_actor_42540->phase + 1;
        case -1:
        restart:
            g_eng3_actor_42540->phase = -1;
            break;
        case -4:
            g_eng3_actor_42540->unknown_94 = ~g_eng3_actor_42540->unknown_94;
            g_eng3_actor_42540->render_flags = g_eng3_actor_42540->unknown_94 & 0x10
                                             | g_eng3_actor_42540->render_flags & ~0x10;
            if (g_eng3_actor_42540->unknown_94) {
                if ((g_eng3_actor_42540->x & 0xf0000) < 0x50000)
                    g_eng3_actor_42540->x = g_eng3_actor_42540->x & 0xfff50000 | 0x50000;
            } else {
                if ((g_eng3_actor_42540->x & 0xf0000) > 0xb0000)
                    g_eng3_actor_42540->x = g_eng3_actor_42540->x & 0xfffb0000 | 0xb0000;
            }
            break;
        case -6:
            g_eng3_actor_42540->render_flags ^= 0x10;
            break;
        case -7:
            g_eng3_4254c = 0xf8;
            g_eng3_35168 = 0x80;
            g_eng3_3fa40 = 0;
            eng3_1fb10(1);
            g_engine_interface.context_004->unknown_1b30 = 4;
            break;
        case -8:
            g_engine_interface.context_004->unknown_03 = 0x21;
        case -39:
            /* reads the short at image + phase * 8 + 6 of g_focus's script (stride as observed) */
            play_sound(((ScriptStep *)g_focus->image)[g_focus->phase * 2 + 1].arg, 0);
            break;
        case -40:
            play_sound(((ScriptStep *)g_eng3_actor_42540->image)[g_eng3_actor_42540->phase].arg, g_focus);
            break;
        case -11:
            g_eng3_callbacks_40ba0[((ScriptStep *)g_focus->image)[g_focus->phase].arg](g_focus);
            break;
        case -3:
            return 1;
        }
    } while (op < 0);
    g_eng3_actor_42540->sprite = op;
    g_eng3_actor_42540->ticks = ((ScriptStep *)g_eng3_actor_42540->image)[g_eng3_actor_42540->phase].arg;
    if (g_eng3_actor_42540->sprite > 0x143)
        g_eng3_actor_42540->sprite = 0x32;
    return 0;
}

void eng3_185c0(void)
{
    g_eng3_mode_41cb4 = 0;
    g_eng3_actor_42540->image = g_eng3_image_37df0;
    g_eng3_actor_42540->ticks = -1;
    g_eng3_actor_42540->phase = -1;
    eng3_18370();
}

void eng3_18600(void)
{
    if (!eng1_29c20(g_eng3_actor_42540)) {
        eng3_18f70();
        return;
    }
    if (!eng3_18630())
        eng3_18370();
}

int eng3_18630(void)
{
    if (g_eng3_3faf0) {
        eng3_18bc0();
        return 1;
    }
    if (g_engine_interface.context_004->held_5c & 0x10) {
        eng3_18690();
        g_eng3_actor_42540->unknown_94 = 0;
        return 1;
    }
    if (g_engine_interface.context_004->held_5c & 0x40) {
        eng3_18690();
        g_eng3_actor_42540->unknown_94 = 0xff;
        return 1;
    }
    return 0;
}

void eng3_18690(void)
{
    g_eng3_mode_41cb4 = 1;
    g_eng3_actor_42540->image = g_eng3_image_37c38;
    g_eng3_actor_42540->ticks = -1;
    g_eng3_actor_42540->phase = -1;
    eng3_18370();
    g_eng3_actor_42540->unknown_f0 = 0;
    g_eng3_actor_42540->unknown_f4 = 0;
    g_eng3_actor_42540->unknown_fc = 0;
    g_eng3_actor_42540->unknown_100 = 0;
    g_eng3_actor_42540->unknown_b4 = 0x6000;
    g_eng3_actor_42540->unknown_a8 = 0xc0000;
    g_eng3_actor_42540->unknown_b8 = 0;
    g_eng3_actor_42540->unknown_bc = 0;
    g_eng3_actor_42540->unknown_c8 = 0;
    g_eng3_actor_42540->unknown_cc = 0;
    g_eng3_actor_42540->unknown_d0 = 0;
    g_focus->unknown_dc = 0;
    g_eng3_3fb24 = 0;
    g_eng3_actor_42540->unknown_ac = g_eng3_38038;
    g_eng3_actor_42540->unknown_b0 = 0x60000;
}

void eng3_18790(void)
{
    g_eng3_mode_41cb4 = 8;
    g_eng3_actor_42540->image = g_eng3_image_37fe8;
    g_eng3_actor_42540->ticks = -1;
    g_eng3_actor_42540->phase = -1;
    eng3_18370();
    g_eng3_actor_42540->unknown_f0 = 0;
    g_eng3_actor_42540->unknown_f4 = 0;
    g_eng3_actor_42540->unknown_fc = 0;
    g_eng3_actor_42540->unknown_100 = 0;
    g_eng3_actor_42540->unknown_ac = 0x280000;
    g_eng3_actor_42540->unknown_b0 = 0x6000;
    g_eng3_actor_42540->unknown_b4 = 0x6000;
    g_eng3_actor_42540->unknown_a8 = 0xc0000;
    g_eng3_actor_42540->unknown_b8 = 0;
    g_eng3_actor_42540->unknown_bc = 0;
    g_eng3_actor_42540->unknown_c8 = 0;
    g_eng3_actor_42540->unknown_cc = 0;
    g_eng3_actor_42540->unknown_d0 = 0;
    g_focus->unknown_d8 = 8;
}

void eng3_18880(void)
{
    g_eng3_actor_42540->unknown_ac = 0x180000;
    g_eng3_actor_42540->unknown_b0 = 0x60000;
    eng3_1a110();
    if (g_focus->sprite == 0x4a) {
        if (g_focus->unknown_ee > 0) {
            g_focus->phase = (short)g_focus->unknown_d8;
            g_focus->unknown_d8 = 8;
        } else {
            g_focus->phase = 0xd;
        }
    }
    if (g_focus->sprite != 0x4b && g_focus->unknown_ee > 0) {
        if (eng3_18dc0())
            return;
        eng3_19f30();
        eng3_1a070();
        if (!eng1_29c20(g_eng3_actor_42540))
            eng3_18f70();
        eng3_18d00();
        eng3_18370();
        return;
    }
    g_eng3_3fb20 = 0;
    g_eng3_3fae8 = 2;
    eng3_18690();
}

void eng3_18950(void)
{
    if (g_eng3_3faf0) {
        if (g_eng3_mode_41cb4 != 10)
            eng3_18bc0();
        return;
    }
    eng3_1a110();
    if (g_engine_interface.context_004->bar_value_target <= 0
        && (!(g_engine_interface.context_004->buttons_30 & 1) || !(g_engine_interface.context_004->buttons_30 & 2))) {
        eng3_19a30();
        return;
    }
    if (g_focus->y > 0x2000000)
        g_focus->y = 0x2000000;
    if (g_engine_interface.context_004->level == 2) {
        if (g_focus->x >= 0x39800000) {
            if (g_focus->profile == 7 || g_focus->profile == 8 || g_focus->profile == 9 || g_focus->profile == 10)
                goto moving;
            if (eng3_18dc0())
                return;
        } else if (eng3_18dc0()) {
            return;
        }
    }
    if (g_engine_interface.context_004->level == 8) {
        if (g_focus->x >= 0x74800000) {
            if (g_focus->profile == 2)
                goto moving;
            if (eng3_18dc0())
                return;
        } else if (eng3_18dc0()) {
            return;
        }
    }
    if (g_engine_interface.context_004->level == 10) {
        if (g_focus->x >= 0x71000000) {
            if (g_focus->profile == 3 || g_focus->profile == 4)
                goto moving;
            if (eng3_18dc0())
                return;
        } else if (eng3_18dc0()) {
            return;
        }
    }
moving:
    if (g_eng3_view_angle_40614 <= 0x4e8)
        g_focus->image = g_eng3_image_37c38;
    g_eng3_actor_42540->unknown_ac = g_eng3_38038;
    if (g_eng3_3fb20) {
        eng3_18790();
        return;
    }
    eng3_18d00();
    g_eng3_iterations_x_40635 = 2;
    g_eng3_3803c = 0x140;
    if (!g_eng3_3faf0) {
        if (g_engine_interface.context_004->held_5c & 0x10) {
            g_eng3_actor_42540->unknown_ac = 0x100000;
            g_focus->image = g_eng3_image_37cd8;
            g_eng3_3803c = 0x168;
        }
        if (g_engine_interface.context_004->held_5c & 0x40) {
            g_eng3_actor_42540->unknown_ac = -0x40000;
            g_focus->image = g_eng3_image_37ca0;
            g_eng3_3803c = 0xf0;
            if (g_eng3_height_40640 <= 0xf0)
                g_eng3_actor_42540->unknown_ac = 0x50000;
        }
    }
    if (g_engine_interface.context_004->level == 2 && g_focus->y > 0x1e10000)
        g_eng3_actor_42540->unknown_ac >>= 1;
    if (g_eng3_3fb2c)
        g_eng3_actor_42540->unknown_ac = g_eng3_38038 >> 1;
    eng3_19f30();
    eng3_1a070();
    if (!eng1_29c20(g_eng3_actor_42540))
        eng3_18f70();
    eng3_18370();
}

void eng3_18bc0(void)
{
    g_eng3_mode_41cb4 = 10;
    g_eng3_actor_42540->image = g_eng3_image_37ca0;
    g_eng3_actor_42540->ticks = -1;
    g_eng3_actor_42540->phase = -1;
    eng3_18370();
    g_eng3_actor_42540->unknown_f0 = 0;
    g_eng3_actor_42540->unknown_f4 = 0;
    g_eng3_actor_42540->unknown_fc = 0;
    g_eng3_actor_42540->unknown_100 = 0;
    g_eng3_actor_42540->unknown_ac = 0x10000;
    g_eng3_actor_42540->unknown_a8 = 0x10000;
    g_eng3_actor_42540->unknown_b8 = 0;
    g_eng3_actor_42540->unknown_bc = 0;
    g_eng3_actor_42540->unknown_c8 = 0;
    g_eng3_actor_42540->unknown_cc = 0;
    g_eng3_actor_42540->unknown_d0 = 0;
    g_focus->unknown_dc = 0;
    g_eng3_3fb24 = 0;
}

void eng3_18c90(void)
{
    if (g_engine_interface.context_004->level == 2)
        g_eng3_actor_42540->unknown_ac = 0x10000;
    if (g_engine_interface.context_004->level == 8)
        g_eng3_actor_42540->unknown_ac = 0x50000;
    if (g_engine_interface.context_004->level == 10)
        g_eng3_actor_42540->unknown_ac = 0x10000;
    eng3_19f30();
    if (!eng1_29c20(g_eng3_actor_42540))
        eng3_18f70();
    eng3_18370();
}

void eng3_18d00(void)
{
    if (!(g_engine_interface.context_004->held_5c & 0xa0)) {
        g_eng3_actor_42540->unknown_c8 = 0;
        return;
    }
    if (g_engine_interface.context_004->held_5c & 0x80) {
        g_eng3_actor_42540->unknown_c8 = 0x40000;
        g_eng3_actor_42540->unknown_cc = 0x40000;
        if (g_eng3_3fb40) {
            g_eng3_actor_42540->unknown_c8 = 0;
            g_eng3_actor_42540->unknown_cc = 0;
            g_eng3_3fb40 = 0;
        }
    }
    if (g_engine_interface.context_004->held_5c & 0x20) {
        g_eng3_actor_42540->unknown_c8 = -0x40000;
        g_eng3_actor_42540->unknown_cc = -0x40000;
        if (g_eng3_3fb44) {
            g_eng3_actor_42540->unknown_c8 = 0;
            g_eng3_actor_42540->unknown_cc = 0;
            g_eng3_3fb44 = 0;
        }
    }
}

int eng3_18dc0(void)
{
    if (g_engine_interface.context_004->pressed_5e & g_engine_interface.context_004->enabled_6e) {
        eng3_18de0();
        return 1;
    }
    return 0;
}

void eng3_18de0(void)
{
    g_eng3_mode_41cb4 = 2;
    g_eng3_actor_42540->image = g_eng3_image_37e08;
    g_eng3_actor_42540->ticks = -1;
    g_eng3_actor_42540->phase = -1;
    eng3_18370();
    if (g_eng3_3fb20) {
        g_eng3_mode_41cb4 = 2;
        g_eng3_actor_42540->image = g_eng3_image_37fe8;
        g_eng3_actor_42540->ticks = -1;
        g_eng3_actor_42540->phase = -1;
        eng3_18370();
    }
    g_eng3_actor_42540->unknown_95 = 0xff;
    g_eng3_actor_42540->unknown_b8 = 0xba000;
    g_eng3_actor_42540->unknown_c0 = 0x6000;
    g_eng3_actor_42540->unknown_c4 = 0xa000;
    g_eng3_actor_42540->unknown_bc = 0x100000;
    unlink_0e4(g_eng3_actor_42540);
    g_eng3_3fb24 = 1;
}

void eng3_18eb0(void)
{
    g_eng3_actor_42540->unknown_ac -= 0x100;
    eng3_18370();
    eng3_18d00();
    if (!g_eng3_3faf0 && (g_engine_interface.context_004->held_5c & 0x40)) {
        g_eng3_actor_42540->unknown_ac = -0x60000;
        g_eng3_3803c = 0xf0;
        if (g_eng3_height_40640 <= 0xf0)
            g_eng3_actor_42540->unknown_ac = 0x50000;
    }
    eng3_19f30();
    eng3_1a070();
    if (!(g_engine_interface.context_004->held_5c & g_engine_interface.context_004->enabled_6e)) {
        g_eng3_actor_42540->unknown_b8 -= 0x10000;
        if (g_eng3_actor_42540->unknown_b8 < 0)
            g_eng3_actor_42540->unknown_b8 = 0;
    }
    eng3_1a180();
    eng3_1ea60(g_focus);
}

void eng3_18f70(void)
{
    g_eng3_mode_41cb4 = 3;
    g_eng3_actor_42540->image = g_eng3_image_37e48;
    g_eng3_actor_42540->ticks = -1;
    g_eng3_actor_42540->phase = -1;
    eng3_18370();
    g_eng3_actor_42540->unknown_fc = 0;
    g_eng3_actor_42540->unknown_100 = 0;
    g_eng3_actor_42540->unknown_b8 = 0x40000;
    g_eng3_actor_42540->unknown_bc = 0x100000;
    g_eng3_actor_42540->unknown_c0 = 0x28000;
    g_eng3_actor_42540->unknown_95 = 0;
    g_eng3_actor_42540->unknown_c0 = 0x6000;
    g_eng3_actor_42540->unknown_c4 = 0xa000;
    g_eng3_actor_42540->unknown_bc = 0x100000;
    g_eng3_3fb24 = 1;
}

void eng3_19030(void)
{
    if (!g_eng3_3faf0 && (g_engine_interface.context_004->held_5c & 0x40)) {
        g_eng3_actor_42540->unknown_ac = -0x60000;
        g_eng3_3803c = 0xf0;
        if (g_eng3_height_40640 <= 0xf0)
            g_eng3_actor_42540->unknown_ac = 0x50000;
    }
    eng3_18d00();
    eng3_19f30();
    eng3_1a070();
    eng3_1a180();
    if (eng3_1eaf0(g_eng3_actor_42540)) {
        if (g_eng3_3fb68)
            play_sound(0x6037, g_focus);
        g_eng3_3fb68 = 1;
        if (g_engine_interface.context_004->level == 2 && g_focus->y > g_eng3_water_level_410e0 << 16)
            play_sound(0x6036, g_focus);
        eng3_18690();
    } else {
        eng3_18370();
    }
}

void eng3_19100(void)
{
    g_eng3_3fb6c = 0;
    g_focus->render_flags &= ~4;
    g_focus->unknown_4a = 0xa0;
}

void eng3_19130(void)
{
    if (g_focus->unknown_ec) {
        g_focus->render_flags &= ~3;
        g_focus->render_flags |= 4;
        if ((g_focus->unknown_4a = (unsigned short)g_eng3_fade_38148[g_eng3_3fb6c++]) == 0)
            eng3_19100();
        g_focus->unknown_4a = (unsigned short)g_eng3_fade_38148[g_eng3_3fb6c++];
        g_focus->render_flags &= ~3;
        g_focus->render_flags |= 4;
        g_focus->unknown_4a -= 0x40;
        return;
    }
    if (g_eng3_3fae4 == 4)
        g_eng3_3fae4 = 2;
    eng3_19100();
}

void eng3_19200(void)
{
    g_eng3_3fb70 = (g_focus->z >> 16) & 0x7f;
    g_eng3_3fb74 = (g_focus->x >> 16) & 0x7f;
    g_eng3_3fb78 = (g_focus->z >> 16) - g_eng3_3fb70 + 0x40;
    g_eng3_3fb7c = (g_focus->z >> 16) - g_eng3_3fb70;
    if (g_eng3_3fb20
        && (g_focus->unknown_116 == 0x22 || g_focus->unknown_116 == 0x26 || g_focus->unknown_116 == 0x36
            || g_focus->unknown_116 == 0x32))
        return;
    g_eng3_3fb20 = 0;
    g_focus->unknown_ee = 0;
    g_eng3_hud_40110.gauge = 0;
    if (!g_focus->unknown_ec)
        g_eng3_3fae8 = 2;
    RemoveFxIdLinks_7f8e(0x2c, g_focus);
    if (g_eng3_mode_41cb4 == 6 || g_eng3_mode_41cb4 == 7)
        return;
    if ((g_focus->x & 0x7f0000) < 0x400000 && g_focus->unknown_116 == 1)
        return;
    if (!g_focus->unknown_ec) {
        if (!(g_engine_interface.context_004->buttons_30 & 7)) {
            eng3_19410(g_focus->unknown_116);
            if (g_engine_interface.context_004->bar_value_target <= 0
                && (!(g_engine_interface.context_004->buttons_30 & 1)
                    || !(g_engine_interface.context_004->buttons_30 & 2))) {
                eng3_19a30();
                return;
            }
            g_focus->unknown_ec = 0x40;
        }
    } else if (g_focus->unknown_116 == 4 || g_focus->unknown_116 == 0x30) {
        return;
    }
    if (g_focus->unknown_116 == 0x80) {
        g_eng3_mode_41cb4 = 5;
        g_eng3_actor_42540->image = g_eng3_image_37f50;
        g_eng3_actor_42540->ticks = -1;
        g_eng3_actor_42540->phase = -1;
        eng3_18370();
        return;
    }
    if (!g_eng3_3fb24) {
        g_eng3_mode_41cb4 = 5;
        g_eng3_actor_42540->image = g_eng3_image_37f50;
        g_eng3_actor_42540->ticks = -1;
        g_eng3_actor_42540->phase = -1;
        eng3_18370();
    } else {
        g_eng3_mode_41cb4 = 5;
        g_eng3_actor_42540->image = g_eng3_image_37f88;
        g_eng3_actor_42540->ticks = -1;
        g_eng3_actor_42540->phase = -1;
        eng3_18370();
    }
}

/* Damage by tile kind; every case is a separate body (the compiler merges identical bodies;
 * grouped labels change the switch lowering). */
void eng3_19410(short kind)
{
    if (g_focus->unknown_ec)
        return;
    if (!g_eng3_3fb24) {
        if (kind != 0x10 && kind != 0x20)
            play_sound(0x602e, g_focus);
    } else {
        if (kind != 0x10 && kind != 0x20)
            play_sound(0x602d, g_focus);
    }
    switch (kind) {
    case 2:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_ac;
        break;
    case 4:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a0;
        break;
    case 0x21:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a4;
        break;
    case 0x22:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a0;
        break;
    case 0x23:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a0;
        break;
    case 0x24:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a0;
        break;
    case 0x25:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a0;
        break;
    case 0x26:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a4;
        break;
    case 0x27:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a0;
        break;
    case 0x28:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a8;
        break;
    case 0x29:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a4;
        break;
    case 0x2a:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a4;
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a4;
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a0;
        break;
    case 0x2b:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a4;
        break;
    case 0x2c:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a4;
        break;
    case 0x2d:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a4;
        break;
    case 0x2e:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a8;
        break;
    case 0x2f:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a8;
        break;
    case 0x30:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a8;
        break;
    case 0x31:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a8;
        break;
    case 0x32:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a8;
        break;
    case 0x33:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a8;
        break;
    case 0x34:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a0;
        break;
    case 0x35:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a0;
        break;
    case 0x36:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a8;
        break;
    case 0x40:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a4;
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a0;
        break;
    case 0x80:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_a4;
        break;
    case 0x100:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_b0;
        break;
    case 0x200:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_ac;
        break;
    case 0x400:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_ac;
        break;
    case 0x800:
        g_engine_interface.context_004->bar_value_target
            -= g_eng3_damage_380a0[g_engine_interface.context_004->difficulty & 3].hit_ac;
        break;
    }
}

/* Mode 5 (hit/push) handler.  Separate case bodies; the 0x21..0x36 cases are listed in descending
 * order (ascending gives 1168 bytes, descending the original 1120); identical bodies still merge into
 * a different register partition than the original (open residue). */
void eng3_195d0(void)
{
    short d[6];

    eng1_29c20(g_eng3_actor_42540);
    eng3_1a110();
    if (g_focus->unknown_ec && g_eng3_3fae4 == 4)
        g_focus->image = g_eng3_image_37fb8;
    if (g_focus->unknown_116 == 0x10 || g_focus->unknown_116 == 0x20)
        g_focus->image = g_eng3_image_37fb8;
    switch (g_focus->unknown_116) {
    case 2:
        if (g_focus->unknown_110->z < 0) {
            g_focus->render_flags |= 0x10;
            g_focus->x -= 0x10000;
            g_focus->z += 0x30000;
        } else {
            g_focus->render_flags &= ~0x10;
            g_focus->x -= 0x10000;
            g_focus->z -= 0x30000;
        }
        break;
    case 4:
        g_eng3_actor_42540->unknown_ac = 0x80000;
        eng3_19f30();
        break;
    case 8:
        if (g_focus->unknown_110->z < 0) {
            g_focus->render_flags |= 0x10;
            g_focus->z += 0x30000;
        } else {
            g_focus->render_flags &= ~0x10;
            g_focus->z -= 0x30000;
        }
        break;
    case 0x10:
        g_focus->render_flags &= ~0x10;
        g_focus->z -= 0x20000;
        g_focus->x -= 0x10000;
        break;
    case 0x20:
        g_focus->render_flags |= 0x10;
        g_focus->z += 0x20000;
        g_focus->x -= 0x10000;
        break;
    case 0x40:
        g_eng3_actor_42540->unknown_ac = 0x80000;
        eng3_19f30();
        break;
    case 0x80:
        if (g_focus->z >= g_eng3_3fb78 << 16) {
            g_focus->render_flags |= 0x10;
            g_focus->z += 0x20000;
        } else {
            g_focus->render_flags &= ~0x10;
            g_focus->z -= 0x20000;
        }
        g_focus->x -= 0x10000;
        break;
    case 0x100:
        if (g_focus->unknown_110->z < 0) {
            g_focus->render_flags |= 0x10;
            g_focus->x -= 0x10000;
            g_focus->z += 0x30000;
        } else {
            g_focus->render_flags &= ~0x10;
            g_focus->x -= 0x10000;
            g_focus->z -= 0x30000;
        }
        break;
    case 0x200:
        if (g_focus->unknown_110->z < 0) {
            g_focus->render_flags |= 0x10;
            g_focus->x -= 0x10000;
            g_focus->z += 0x30000;
        } else {
            g_focus->render_flags &= ~0x10;
            g_focus->x -= 0x10000;
            g_focus->z -= 0x30000;
        }
        break;
    case 0x400:
        g_focus->x -= 0x10000;
        break;
    case 0x800:
        g_focus->x -= 0x10000;
        break;
    case 0x36:
        if (g_focus->unknown_110->z < 0) {
            g_focus->render_flags |= 0x10;
            g_focus->x -= 0x10000;
            g_focus->z += 0x30000;
        } else {
            g_focus->render_flags &= ~0x10;
            g_focus->x -= 0x10000;
            g_focus->z -= 0x30000;
        }
        break;
    case 0x35:
        vec_diff_fixed_1420((const int *)g_focus->unknown_110, (const int *)g_focus, d);
        if (d[2] < 0) {
            g_focus->render_flags |= 0x10;
            g_focus->x -= 0x10000;
            g_focus->z += 0x38000;
        } else {
            g_focus->render_flags &= ~0x10;
            g_focus->x -= 0x10000;
            g_focus->z -= 0x30000;
        }
        break;
    case 0x34:
        g_eng3_actor_42540->unknown_ac = 0x80000;
        eng3_19f30();
        break;
    case 0x33:
        g_eng3_actor_42540->unknown_ac = 0x80000;
        eng3_19f30();
        break;
    case 0x32:
        g_eng3_actor_42540->unknown_ac = 0x80000;
        eng3_19f30();
        break;
    case 0x31:
        if (g_focus->unknown_110->z < 0) {
            g_focus->render_flags |= 0x10;
            g_focus->x -= 0x10000;
            g_focus->z += 0x30000;
        } else {
            g_focus->render_flags &= ~0x10;
            g_focus->x -= 0x10000;
            g_focus->z -= 0x30000;
        }
        break;
    case 0x30:
        g_eng3_actor_42540->unknown_ac = 0x80000;
        eng3_19f30();
        break;
    case 0x2f:
        g_eng3_actor_42540->unknown_ac = 0x80000;
        eng3_19f30();
        break;
    case 0x2e:
        g_eng3_actor_42540->unknown_ac = 0x80000;
        eng3_19f30();
        break;
    case 0x2d:
        if (g_focus->unknown_110->z < 0)
            g_focus->render_flags |= 0x10;
        else
            g_focus->render_flags &= ~0x10;
        g_focus->x -= 0x10000;
        break;
    case 0x2c:
        g_focus->x -= 0x10000;
        break;
    case 0x2b:
        if (g_focus->unknown_110->z < 0) {
            g_focus->render_flags |= 0x10;
            g_focus->x -= 0x10000;
            g_focus->z += 0x10000;
        } else {
            g_focus->render_flags &= ~0x10;
            g_focus->x -= 0x10000;
            g_focus->z -= 0x10000;
        }
        break;
    case 0x2a:
        g_focus->x -= 0x10000;
        break;
    case 0x29:
        if (g_focus->unknown_110->z < 0) {
            g_focus->render_flags |= 0x10;
            g_focus->x -= 0x10000;
            g_focus->z += 0x10000;
        } else {
            g_focus->render_flags &= ~0x10;
            g_focus->x -= 0x10000;
            g_focus->z -= 0x10000;
        }
        break;
    case 0x28:
        g_eng3_actor_42540->unknown_ac = 0x80000;
        eng3_19f30();
        break;
    case 0x27:
        g_focus->x -= 0x10000;
        break;
    case 0x26:
        g_focus->x -= 0x10000;
        break;
    case 0x25:
        if (g_focus->unknown_110->z < 0) {
            g_focus->render_flags |= 0x10;
            g_focus->x -= 0x10000;
            g_focus->z += 0x30000;
        } else {
            g_focus->render_flags &= ~0x10;
            g_focus->x -= 0x10000;
            g_focus->z -= 0x30000;
        }
        break;
    case 0x24:
        g_eng3_actor_42540->unknown_ac = 0x80000;
        eng3_19f30();
        break;
    case 0x23:
        g_eng3_actor_42540->unknown_ac = 0x80000;
        eng3_19f30();
        break;
    case 0x22:
        g_focus->x -= 0x10000;
        break;
    case 0x21:
        g_focus->x -= 0x10000;
        break;
    }
    if (g_focus->y > 0x2000000)
        g_focus->y = 0x2000000;
    if (!eng1_29c20(g_eng3_actor_42540)) {
        g_eng3_actor_42540->unknown_b8 = 0x40000;
        g_eng3_actor_42540->unknown_bc = 0x80000;
        g_eng3_actor_42540->unknown_c0 = 0x28000;
        g_eng3_actor_42540->unknown_95 = 0;
        eng3_1a180();
    }
    if (eng3_18370() == 1) {
        g_eng3_3fba0 = 0;
        eng3_18690();
        g_eng3_41cb8 = 10;
    }
}

void eng3_19a30(void)
{
    if (g_eng3_3fb2c)
        return;
    g_engine_interface.context_004->unknown_05 = 1;
    g_eng3_3fb68 = 0;
    g_eng3_3fb20 = 0;
    g_eng3_3fae4 = 2;
    g_focus->unknown_ec = 0;
    g_focus->unknown_ee = 0;
    g_eng3_41d50 = 0;
    eng3_1fb00();
    if (g_engine_interface.context_004->unknown_0c && !(g_engine_interface.context_004->buttons_30 & 8)) {
        g_engine_interface.context_004->unknown_0c--;
        g_engine_interface.context_004->unknown_0d++;
    }
    if (g_engine_interface.context_004->unknown_0c == 0)
        g_engine_interface.context_004->unknown_03 = 1;
    g_eng3_actor_42540->unknown_ac = 0;
    g_eng3_actor_42540->unknown_bc = 0;
    g_eng3_actor_42540->unknown_cc = 0;
    unlink_0e4(g_eng3_actor_42540);
    g_engine_interface.context_004->sublevel = g_engine_interface.context_004->unknown_02;
    g_eng3_mode_41cb4 = 6;
    g_eng3_actor_42540->image = g_eng3_image_37e58;
    g_eng3_actor_42540->ticks = -1;
    g_eng3_actor_42540->phase = -1;
    eng3_18370();
    if (g_focus->unknown_126) {
        g_focus->scale = 0x200;
        g_eng3_mode_41cb4 = 6;
        g_eng3_actor_42540->image = g_eng3_image_37e98;
        g_eng3_actor_42540->ticks = -1;
        g_eng3_actor_42540->phase = -1;
        eng3_18370();
    }
}

void eng3_19b70(void)
{
    g_engine_interface.context_004->unknown_05 = 1;
    eng3_1a180();
    eng3_1a110();
    if (g_focus->phase >= 0xb) {
        g_eng3_4254c = 0xf8;
        g_eng3_35168 = 0x80;
        g_eng3_3fa40 = 0;
        eng3_1fb10(1);
        g_engine_interface.context_004->unknown_1b30 = 4;
    }
    if (!g_focus->unknown_e0 && !g_focus->unknown_dc) {
        eng3_1eeb0(g_focus);
        if (eng3_18370()) {
            if (g_engine_interface.context_004->unknown_0c == 0)
                g_engine_interface.context_004->unknown_03 = 0x22;
            else
                g_engine_interface.context_004->unknown_03 = 0x21;
        }
    } else if (eng3_18370()) {
        if (g_engine_interface.context_004->unknown_0c == 0)
            g_engine_interface.context_004->unknown_03 = 0x22;
        else
            g_engine_interface.context_004->unknown_03 = 0x21;
    }
}

void eng3_19c20(void)
{
    int i;

    g_eng3_mode_41cb4 = 9;
    g_eng3_actor_42540->image = g_eng3_image_37ea0;
    g_eng3_actor_42540->ticks = -1;
    g_eng3_actor_42540->phase = -1;
    eng3_18370();
    g_engine_interface.context_004->sublevel = g_engine_interface.context_004->unknown_02;
    g_eng3_actor_42540->unknown_ac = 0;
    g_eng3_actor_42540->unknown_bc = 0;
    g_eng3_actor_42540->unknown_cc = 0;
    for (i = 0; i < g_eng3_3fb08 + 1; i++) {
        g_engine_interface.context_004->unknown_16 += g_eng3_offsets_40c40[g_eng3_41cc0[i]].dx;
        g_engine_interface.context_004->unknown_1a += g_eng3_offsets_40c40[g_eng3_41cc0[i]].dy;
    }
}

void eng3_19cd0(void)
{
    if (g_focus->phase > 0x1e) {
        g_eng3_4254c = 0xf8;
        g_eng3_35168 = 0x80;
        g_eng3_3fa40 = 0;
        eng3_1fb10(1);
        g_engine_interface.context_004->unknown_1b30 = 4;
    }
    if (eng3_18370()) {
        g_eng3_3fba0 = 0;
        eng3_185c0();
        g_engine_interface.context_004->unknown_03 = 0x31;
    }
}

void eng3_19d40(void)
{
    g_eng3_3fb28 = 1;
    if (g_eng3_41d48 < 0) {
        if (g_engine_interface.context_004->pressed_5e & 0xc00) {
            eng3_18f70();
            return;
        }
    } else if (g_eng3_41d48 > 5) {
        if (!(g_engine_interface.context_004->held_5c & 0xc00)) {
            if (eng3_1b2e0(g_eng3_actor_42540->x >> 16, g_eng3_actor_42540->y >> 16, g_focus->z >> 16,
                           g_focus->profile) & 0xffff)
                eng3_18f70();
            else
                eng3_185c0();
            return;
        }
    } else if (g_eng3_41d48++ < 6) {
        if (!(g_engine_interface.context_004->held_5c & 0xc00))
            g_eng3_41d48 = -8;
    } else {
        g_eng3_41d48 = 6;
    }
    if (g_engine_interface.context_004->held_5c & 0x1000) {
        if (g_eng3_actor_42540->unknown_95 == 0)
            g_eng3_actor_42540->unknown_b8 = 0;
        eng3_19fd0();
        g_eng3_actor_42540->unknown_95 = 1;
    } else if (g_engine_interface.context_004->held_5c & 0x4000) {
        if (g_eng3_actor_42540->unknown_95 != 0)
            g_eng3_actor_42540->unknown_b8 = 0;
        eng3_19fd0();
        g_eng3_actor_42540->unknown_95 = 0;
    } else {
        g_eng3_actor_42540->unknown_b8 = 0;
    }
    if (g_engine_interface.context_004->held_5c & 0x40) {
        if (g_eng3_actor_42540->unknown_94 == 0)
            g_eng3_actor_42540->unknown_a8 = 0;
        eng3_19f30();
        g_eng3_actor_42540->unknown_94 = 0xff;
        g_eng3_actor_42540->render_flags = g_eng3_actor_42540->unknown_94 & 0x10
                                         | g_eng3_actor_42540->render_flags & ~0x10;
        g_eng3_actor_42540->render_flags |= 0x80000000;
    } else if (g_engine_interface.context_004->held_5c & 0x10) {
        if ((g_eng3_actor_42540->unknown_94 & 0x10) == 0x10)
            g_eng3_actor_42540->unknown_a8 = 0;
        eng3_19f30();
        g_eng3_actor_42540->unknown_94 = 0;
        g_eng3_actor_42540->render_flags = g_eng3_actor_42540->unknown_94 & 0x10
                                         | g_eng3_actor_42540->render_flags & ~0x10;
        g_eng3_actor_42540->render_flags |= 0x80000000;
    } else if (g_engine_interface.context_004->held_5c & 0x80) {
        g_focus->z -= 0x40000;
    } else if (g_engine_interface.context_004->held_5c & 0x20) {
        g_focus->z += 0x40000;
    } else {
        g_eng3_actor_42540->unknown_a8 = 0;
    }
}

void eng3_19f30(void)
{
    if (g_eng3_actor_42540->unknown_a8 != g_eng3_actor_42540->unknown_ac) {
        if (g_eng3_actor_42540->unknown_a8 > g_eng3_actor_42540->unknown_ac) {
            g_eng3_actor_42540->unknown_a8 -= g_eng3_actor_42540->unknown_b4;
            if (g_eng3_actor_42540->unknown_a8 < g_eng3_actor_42540->unknown_ac)
                g_eng3_actor_42540->unknown_a8 = g_eng3_actor_42540->unknown_ac;
        } else {
            g_eng3_actor_42540->unknown_a8 += g_eng3_actor_42540->unknown_b0;
            if (g_eng3_actor_42540->unknown_a8 > g_eng3_actor_42540->unknown_ac)
                g_eng3_actor_42540->unknown_a8 = g_eng3_actor_42540->unknown_ac;
        }
    }
    if (g_eng3_actor_42540->unknown_94)
        g_eng3_actor_42540->x += g_eng3_actor_42540->unknown_f0 - g_eng3_actor_42540->unknown_a8;
    else
        g_eng3_actor_42540->x += g_eng3_actor_42540->unknown_a8 + g_eng3_actor_42540->unknown_f0;
    eng3_empty_2390();
}

void eng3_19fd0(void)
{
    if (g_eng3_actor_42540->unknown_b8 != g_eng3_actor_42540->unknown_bc) {
        if (g_eng3_actor_42540->unknown_b8 > g_eng3_actor_42540->unknown_bc) {
            g_eng3_actor_42540->unknown_b8 -= g_eng3_actor_42540->unknown_c4;
            if (g_eng3_actor_42540->unknown_b8 < g_eng3_actor_42540->unknown_bc)
                g_eng3_actor_42540->unknown_b8 = g_eng3_actor_42540->unknown_bc;
        } else {
            g_eng3_actor_42540->unknown_b8 += g_eng3_actor_42540->unknown_c0;
            if (g_eng3_actor_42540->unknown_b8 > g_eng3_actor_42540->unknown_bc)
                g_eng3_actor_42540->unknown_b8 = g_eng3_actor_42540->unknown_bc;
        }
    }
    if (g_eng3_actor_42540->unknown_95)
        g_eng3_actor_42540->y -= g_eng3_actor_42540->unknown_b8;
    else
        g_eng3_actor_42540->y += g_eng3_actor_42540->unknown_b8;
    eng3_1a110();
}
}
