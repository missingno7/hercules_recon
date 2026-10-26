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
    unsigned char unknown_0c[2];
    short unknown_0e;
    unsigned char unknown_10[0x1c - 0x10];
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
    unsigned char unknown_96, unknown_97;
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
/* Interface context (g_engine_interface.context_004). */
typedef struct LevelContext {
    signed char level;
    signed char sublevel;
    unsigned char unknown_02;
    unsigned char unknown_03;
    unsigned char unknown_04;
    unsigned char unknown_05;
    unsigned char unknown_06[0x0a - 0x06];
    unsigned char unknown_0a;
    unsigned char unknown_0b;
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
    long unknown_34;
    long tick;
    unsigned long unknown_3c;
    long unknown_40;
    unsigned char unknown_44[0x48 - 0x44];
    unsigned long unknown_48, unknown_4c;
    unsigned char unknown_50[0x54 - 0x50];
    long unknown_54;
    unsigned char unknown_58[0x5c - 0x58];
    volatile unsigned short held_5c;
    volatile unsigned short pressed_5e;
    unsigned short unknown_60, unknown_62, unknown_64, unknown_66;
    unsigned char unknown_68[0x6c - 0x68];
    unsigned short unknown_6c;
    unsigned short enabled_6e;
    unsigned long unknown_70[3];
    unsigned long saved_7c[4];
    unsigned long unknown_8c;
    unsigned long unknown_90;
    unsigned long unknown_94;
    unsigned char unknown_98[0x9c - 0x98];
    unsigned short unknown_9c;
    unsigned char unknown_9e[0xb4 - 0x9e];
    void (*unknown_b4)(int);
    unsigned char unknown_b8[0x128 - 0xb8];
    unsigned char unknown_128;
    unsigned char unknown_129[0x1b30 - 0x129];
    unsigned long unknown_1b30;
} LevelContext;

typedef struct EngineInterfaceView {
    unsigned long signature_000;
    LevelContext *context_004;
    unsigned long unknown_008, unknown_00c;
    unsigned long unknown_010;
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
/* Double-buffered frame (0x2088 bytes): draw environment, display environment, two mode
 * primitives and the 0x800-entry ordering table (PSX DB layout). */
typedef struct Eng3Buffer {
    unsigned char unknown_00[0x16];
    unsigned char dtd, dfe, isbg;
    unsigned char unknown_19[0x5c - 0x19];
    unsigned char disp_5c[0x70 - 0x5c];
    unsigned long prim_70[3];
    unsigned long prim_7c[3];
    unsigned long ot[0x800];
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
extern long eng3_1b2e0(unsigned short, short, unsigned short, int);
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
extern int eng3_1a070(void);
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
    if ((g_engine_interface.context_004->unknown_64 & 0x400) && (g_engine_interface.context_004->buttons_30 & 0x4000))
        eng3_19a30();
    if ((g_engine_interface.context_004->unknown_64 & 0x100) && (g_engine_interface.context_004->buttons_30 & 0x4000)) {
        eng3_19a30();
        if (g_engine_interface.context_004->unknown_10 == 0)
            g_engine_interface.context_004->unknown_10 = 1;
    }
    if ((g_engine_interface.context_004->unknown_64 & 0x200) && (g_engine_interface.context_004->buttons_30 & 0x4000)) {
        eng3_19a30();
        g_engine_interface.context_004->unknown_10 = 0;
    }
    if ((g_engine_interface.context_004->unknown_64 & 0x800) && (g_engine_interface.context_004->buttons_30 & 0x2000))
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
    long delta;

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
        delta = g_eng3_actor_42540->unknown_f0 - g_eng3_actor_42540->unknown_a8;
    else
        delta = g_eng3_actor_42540->unknown_a8 + g_eng3_actor_42540->unknown_f0;
    g_eng3_actor_42540->x += delta;
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

/* ---- lane y13 (0x1a070..0x1c010) declarations ----
 * ENG3.DLL 0x1a070..0x1c010 (29 functions; follows the level mode handlers): player axis/limit
 * helpers, level setup/teardown (allocation, frame DB, MAP_PSX.BIN load and header decode), the
 * engine main loop 0x1abb0, grid height/surface queries (0x1b100 decode, 0x1b2e0 height, 0x1b4a0
 * surface code, 0x1b650 track bounds), actor range tests and the per-frame update/draw passes.
 * Delivered as C++ (extern "C"), appended after calibration/eng3/handlers_16240.cpp; the widened
 * Actor / LevelContext / EngineInterfaceView / Eng3Buffer views keep their existing offsets.
 * Context +0x64 is a 16-bit word (0x1a560 clears it with a word store; with that view the
 * canonical 0x174e0 tests unknown_64 & 0x400.. become masked equal).  0x40650..0x4078b is one
 * map-header object (Eng3Map): 0x1af10 reloads lanes[i] after the column/row stores only when
 * they are fields of one object.  Open residues: 0x1a290 (pad word +0x5e loads, equal only with
 * that field volatile), 0x1abb0 (0x3fc70 load order, equal only with it volatile), 0x1b9f0
 * (one base/index order), 0x1b100/0x1b2e0/0x1b4a0/0x1b650 (register allocation/reassociation). */
/* 32-byte per-level row at 0x39870 (Eng3LevelInfo in track_unit_20010.cpp, widened view). */
typedef struct Eng3LevelRow {
    unsigned char unknown_00, unknown_01, unknown_02, unknown_03;
    unsigned char flags_04;
    unsigned char unknown_05[3];
    unsigned long prim_count_08;
    unsigned char unknown_0c[4];
    void (*after_draw_10)(void);
    unsigned char unknown_14[0x20 - 0x14];
} Eng3LevelRow;

/* 24-byte per-sublevel and per-level file rows at 0x38528. */
typedef struct Eng3SubFile {
    unsigned char unknown_00[8];
    unsigned long arg_08;
    unsigned char unknown_0c[0x18 - 0x0c];
} Eng3SubFile;

typedef struct Eng3LevelFile {
    const char *map_00;
    unsigned char unknown_04[8];
    Eng3SubFile *subs_0c;
    unsigned long arg_10;
    unsigned long arg_14;
} Eng3LevelFile;

/* 16-byte grid subdivision row at 0x39a68 (step, fraction mask, scale, row stride - 1). */
typedef struct Eng3GridScale {
    long step;
    long mask;
    long scale;
    long stride;
} Eng3GridScale;

/* 16-byte layer header (g_eng3_layers_40674); the shorts are self-relative offsets. */
typedef struct Eng3Layer {
    unsigned char link;
    unsigned char width;
    unsigned char extent;
    unsigned char depth;
    unsigned short rotation;
    short entries;
    short neighbours;
    short shape;
    unsigned char unknown_0c[4];
} Eng3Layer;

/* Per-kind hook records: host 0x40bb4 / middle 0x40b98 (8 bytes), game 0x40b8c (12 bytes). */
typedef struct Eng3Hooks8 {
    void (*create)(void *);
    void (*update)(void *);
} Eng3Hooks8;

typedef struct Eng3Hooks12 {
    void (*create)(Actor *);
    void (*update)(Actor *);
    void (*draw)(Actor *);
} Eng3Hooks12;

/* 0x94-byte host object (0x431c0) and 0xec-byte middle object (0x43174) views. */
typedef struct Eng3Host {
    long x, y, z;
    unsigned char unknown_0c[0x2e - 0x0c];
    unsigned short kind;
    unsigned char unknown_30[0x50 - 0x30];
    unsigned long flags;
    unsigned char unknown_54[0x94 - 0x54];
} Eng3Host;

typedef struct Eng3Middle {
    long x, y, z;
    unsigned char unknown_0c[0x2e - 0x0c];
    unsigned short kind;
    unsigned char unknown_30[0x50 - 0x30];
    unsigned long flags;
    unsigned char unknown_54[0x9c - 0x54];
    long prev_x, prev_y;
    unsigned char unknown_a4[0xec - 0xa4];
} Eng3Middle;

extern Eng3LevelRow g_eng3_levels_39870[];
extern Eng3LevelFile g_eng3_files_38528[];
extern Eng3GridScale g_eng3_grid_39a68[];
extern short g_eng3_sine_38688[];
extern Rect16 g_eng3_clear_rect_381c0;
extern unsigned char g_eng3_script_381d8[];
extern unsigned char g_eng3_script_38210[];
extern unsigned long g_eng3_version_38510;
extern unsigned long g_eng3_version_38518;
extern unsigned short g_eng3_prev_key_3805c;
extern unsigned short g_eng3_next_key_38060;
extern signed char g_eng3_selection_431e4;
extern int g_eng3_vsync_gate_41444;
extern Eng3Hooks8 *g_eng3_host_40bb4;
extern Eng3Hooks8 *g_eng3_middle_40b98;
extern Eng3Host *g_eng3_actor_431c0;
extern unsigned char *g_eng3_prims_405c4;
extern unsigned char *g_eng3_4316c;
extern long *g_eng3_42550;
extern long *g_eng3_431c8;
extern unsigned char *g_eng3_map_40790;
extern unsigned short g_eng3_40b94, g_eng3_40ba8, g_eng3_40bc2;
extern unsigned short g_eng3_40bcc, g_eng3_40bd2, g_eng3_40c32;
extern short g_eng3_40bc6;
extern int g_eng3_41048;
extern int g_eng3_41044;
extern unsigned char g_eng3_3fb80;
extern int g_eng3_3fb84;
extern unsigned long g_eng3_3fc70;
extern short g_eng3_3fa44;
extern short g_eng3_3fba4, g_eng3_3fba8;
extern long g_eng3_3fb94, g_eng3_3fb98;
extern long g_eng3_camera_x_405e0;
extern unsigned char g_eng3_431d0, g_eng3_431d1, g_eng3_431d2;
extern unsigned char g_eng3_43178;
extern long g_eng3_colour_40630;
extern signed char g_eng3_spread_4063b;
extern signed char g_eng3_spread_4063c;
extern short g_eng3_4063e;
extern void (*g_eng3_after_draw_4064c)(void);
extern unsigned char g_eng3_40628, g_eng3_40629, g_eng3_4062a;
/* Map header globals 0x40650..0x4078b (one object: stores to its arrays reload its pointers). */
typedef struct Eng3Map {
    unsigned long *table_50;
    unsigned long *shape_table_54;
    unsigned char *uvs_58;
    short *shapes_5c;
    unsigned long *table_60;
    unsigned long *table_64;
    unsigned long *table_68;
    unsigned long *table_6c;
    unsigned long *table_70;
    Eng3Layer *layers_74;
    unsigned long *shades_78;
    unsigned short *cell_flags_7c;
    unsigned long *table_80;
    unsigned long *lanes_84[32];
    short lane_rows_104[32];
    short lane_columns_144[32];
    signed char lane_limit_184;
    signed char groups_185;
    signed char layer_count_186;
    signed char lane_count_187;
    signed char shape_count_188;
    signed char count_189;
    signed char count_18a;
} Eng3Map;

extern Eng3Map g_eng3_map_40650;
extern short g_eng3_412a4, g_eng3_412a6;
extern short g_eng3_412c0;
extern long g_eng3_40dc4;
extern long g_eng3_40ee4;
extern short g_eng3_41380[];
extern short g_eng3_40f00[];
extern short g_eng3_40f20[];
extern long g_eng3_40cc0[];
extern long g_eng3_40d20[];
extern long g_eng3_40de0[];
extern long g_eng3_40e40[];
extern long g_eng3_40f40[];
extern long g_eng3_40fa0[];
extern long g_eng3_41100[];
extern long g_eng3_411c0[];
extern long g_eng3_41200[];
extern long g_eng3_413a0[];
extern Actor *g_eng3_actor_41c60;

extern void eng3_empty_1000(const char *);
extern void eng3_01870(void);
extern void eng3_01880(void);
extern void eng3_01890(Actor *);
extern void eng1_call_01e50(void);
extern void dispatch_local_02180(void);
extern void eng3_08050(void);
extern void eng3_082d0(void);
extern void eng3_08340(unsigned long, unsigned long);
extern void eng3_08540(void);
extern void eng3_085a0(void);
extern void eng3_08b20(void);
extern void title_041d0(void);
extern void eng3_0a700(void);
extern void title_04760(void);
extern void title_04870(void);
extern void eng3_0aaa0(int, int, int, int, int, int);
extern void eng1_color_fade_step_5d50(void);
extern void demo_load_5ea0(void);
extern void demo_step_6005(void);
extern int game_state_60f9(void);
extern void fx_update_666b(void);
extern void fx_mode_827f(int);
extern void fx_init_82b7(int);
extern void fx_nop_83d3(void);
extern void eng3_16ea0(void);
extern void eng3_17130(void);
extern void eng3_174e0(void);
extern void eng3_178a0(void);
extern void eng3_1c010(void);
extern void eng3_1c290(void);
extern void eng3_1c430(void);
extern void eng3_1c5c0(void);
extern void eng3_1c720(void);
extern void reset_normal_actor_pool(void);
extern void title_08c90(void);
extern void eng3_process_middle_actor_pool(void);
extern void title_09090(void);
extern void reset_alternate_actor_pool(void);
extern void title_09480(void);
extern void title_095d0(void);
extern void eng3_1f1a0(Actor *, Bounds3D *, int);
extern void initialize_pending_action_groups(void);
extern void eng3_slot_024(void);
extern void eng3_slot_02c(void);
extern void eng3_slot_030(void);
extern int eng3_slot_034(int);
extern void eng3_slot_040(void *);
extern void eng3_slot_044(unsigned long);
extern unsigned long eng3_slot_048(const char *, const char *);
extern void eng3_slot_05c(void);
extern void eng3_slot_060(void);
extern unsigned long eng3_slot_068(const char *, const char *, void *, unsigned long);
extern void eng3_slot_06c(unsigned long);
extern void *eng3_slot_078(void **, unsigned long, unsigned long);
extern void eng3_slot_084(void);
extern void eng3_slot_08c(void);
extern int eng3_1fad0(unsigned char *, long, int, int);
extern int eng3_1fb20(Eng3Buffer *, Eng3Buffer *);
extern int eng3_1fb40(int);
extern int eng3_1fb60(int, int);
extern int eng3_1fb80(Eng3Buffer *);
extern int eng3_1fbb0(int);
extern void eng3_slot_140(Rect16 *, int, int, int);
extern void eng3_slot_144(unsigned long *, unsigned long);
extern void eng3_slot_164(unsigned long *);
extern void eng3_slot_16c(unsigned long);
extern void eng3_slot_170(unsigned long);
extern void eng3_1fc60(void);
extern void eng3_1fc70(void);
extern unsigned short eng3_1fe00(int, int, int, int);
extern void eng3_1fe20(void);
extern int eng3_1fe30(Rect16 *, unsigned long *);
extern int eng3_1fe80(int, int);
extern int eng3_1fea0(int, int);
extern int eng3_1ff20(unsigned char *);
extern int eng3_1ff30(Eng3Buffer *);
extern void eng3_1ffa0(unsigned long *, int, int, int, int);
extern long eng3_slot_2ec(unsigned long);
extern void eng3_slot_330(void);
extern long eng3_slot_3e0(unsigned long);
extern void eng3_slot_3e4(void (*)(void));
extern void eng3_20190(void);
extern void eng3_22070(void);
extern void eng3_222c0(void);
extern void eng3_22790(void);
extern void eng3_22b90(long, long, long, int);
extern int eng3_24a80(Actor *);
extern void eng3_25060(void);
extern void eng3_250f0(void);

long eng3_1aa30(unsigned long);
void eng3_1af10(void);
void eng3_1b8f0(void);
void eng3_1b9c0(void);
void eng3_1b9f0(void);
void eng3_1bd40(void);

int eng3_1a070(void)
{
    if (g_eng3_actor_42540->unknown_c8 != g_eng3_actor_42540->unknown_cc) {
        if (g_eng3_actor_42540->unknown_c8 > g_eng3_actor_42540->unknown_cc) {
            g_eng3_actor_42540->unknown_c8 -= g_eng3_actor_42540->unknown_d4;
            if (g_eng3_actor_42540->unknown_c8 < g_eng3_actor_42540->unknown_cc)
                g_eng3_actor_42540->unknown_c8 = g_eng3_actor_42540->unknown_cc;
        } else {
            g_eng3_actor_42540->unknown_c8 += g_eng3_actor_42540->unknown_d0;
            if (g_eng3_actor_42540->unknown_c8 > g_eng3_actor_42540->unknown_cc)
                g_eng3_actor_42540->unknown_c8 = g_eng3_actor_42540->unknown_cc;
        }
    }
    if (g_eng3_actor_42540->unknown_96)
        g_eng3_actor_42540->z += g_eng3_actor_42540->unknown_c8;
    else
        g_eng3_actor_42540->z -= g_eng3_actor_42540->unknown_c8;
    return 0;
}

void eng3_1a110(void)
{
    if (g_engine_interface.context_004->level == 8 && (g_engine_interface.context_004->buttons_30 & 7)) {
        if (g_focus->y > 0x1c00000)
            g_focus->y = 0x1c00000;
    }
    if (g_engine_interface.context_004->level == 2 && g_focus->y > 0x2000000)
        g_focus->y = 0x2000000;
    if (g_engine_interface.context_004->level == 10 && (g_engine_interface.context_004->buttons_30 & 7)) {
        if (g_focus->y > 0xc80000)
            g_focus->y = 0xc80000;
    }
}

void eng3_1a180(void)
{
    if (g_eng3_actor_42540->unknown_95) {
        g_eng3_actor_42540->unknown_b8 -= g_eng3_actor_42540->unknown_c4;
        if (g_eng3_actor_42540->unknown_b8 < 0) {
            g_eng3_actor_42540->unknown_b8 = 0;
            g_eng3_actor_42540->unknown_95 = 0;
            g_eng3_actor_42540->unknown_bc = 0x100000;
            g_eng3_actor_42540->unknown_c0 = 0x18000;
            if (g_eng3_mode_41cb4 != 6)
                g_eng3_mode_41cb4 = 3;
            return;
        }
        g_eng3_actor_42540->y -= g_eng3_actor_42540->unknown_b8;
        eng3_1a110();
        return;
    }
    g_eng3_actor_42540->unknown_b8 += g_eng3_actor_42540->unknown_c0;
    if (g_eng3_actor_42540->unknown_b8 > g_eng3_actor_42540->unknown_bc)
        g_eng3_actor_42540->unknown_b8 = g_eng3_actor_42540->unknown_bc;
    g_eng3_actor_42540->y += g_eng3_actor_42540->unknown_b8;
    if (g_eng3_actor_42540->y > g_eng3_map_40650.lane_columns_144[g_eng3_layer_40638] << 23) {
        g_eng3_actor_42540->y = g_eng3_map_40650.lane_columns_144[g_eng3_layer_40638] << 23;
        g_eng3_actor_42540->unknown_b8 = 0x80000;
        return;
    }
    eng3_1a110();
}

void eng3_1a290(void)
{
    eng3_22b90(g_eng3_actor_42540->x, g_eng3_actor_42540->y, g_eng3_actor_42540->z, 1);
    if (g_eng3_mode_41cb4 != 0 && g_eng3_mode_41cb4 != 5)
        g_eng3_3fba0 = 0;
    if ((unsigned short)(g_eng3_prev_key_3805c & g_engine_interface.context_004->pressed_5e)) {
        if (--g_eng3_selection_431e4 < 0)
            g_eng3_selection_431e4 = 2;
    }
    if ((unsigned short)(g_engine_interface.context_004->pressed_5e & g_eng3_next_key_38060)) {
        if (++g_eng3_selection_431e4 > 3)
            g_eng3_selection_431e4 = 0;
    }
}

void eng3_1a320(void)
{
    if (g_eng3_vsync_gate_41444)
        eng3_slot_330();
}

void eng3_1a330(void)
{
    title_095d0();
    eng3_slot_078((void **)&g_eng3_actor_42540, 0x134 * g_eng3_40bcc, 16);
    eng3_slot_078((void **)&g_eng3_43174, 0xec * g_eng3_40c32 + 0x134, 16);
    eng3_slot_078((void **)&g_eng3_actor_431c0, 0x94 * g_eng3_40bd2 + 0x134, 16);
    eng3_1fe20();
    eng3_slot_078((void **)&g_eng3_frame_4144c, 0x4110, 16);
    eng3_slot_078((void **)&g_eng3_prims_405c4,
                  80 * g_eng3_levels_39870[g_engine_interface.context_004->level].prim_count_08, 16);
    eng3_slot_078((void **)&g_eng3_4316c, 0x4000, 0);
    eng3_slot_078((void **)&g_eng3_42550, 0x8000, 0);
    eng3_slot_078((void **)&g_eng3_431c8, 0x300, 0);
    memset(g_eng3_42550, 0, 0x8000);
    memset(g_eng3_431c8, 0, 0x300);
    title_04760();
    eng3_16ea0();
    g_eng3_layer_40638 = 1;
}

void eng3_1a460(void)
{
    g_eng3_vsync_gate_41444 = 0;
    eng3_slot_170(0);
    eng3_slot_3e4(0);
    if (g_engine_interface.context_004->unknown_9c == 1) {
        eng3_slot_024();
        if (g_engine_interface.context_004->unknown_b4)
            g_engine_interface.context_004->unknown_b4(0);
    } else if (g_engine_interface.context_004->unknown_9c) {
        eng3_slot_030();
    }
    eng3_slot_16c(0);
    eng3_slot_3e0(0);
    if (g_eng3_actor_42540)
        eng3_slot_040(g_eng3_actor_42540);
    if (g_eng3_43174)
        eng3_slot_040((void *)g_eng3_43174);
    if (g_eng3_actor_431c0)
        eng3_slot_040(g_eng3_actor_431c0);
    if (g_eng3_prims_405c4)
        eng3_slot_040(g_eng3_prims_405c4);
    if (g_eng3_frame_4144c)
        eng3_slot_040(g_eng3_frame_4144c);
    eng3_slot_044(0);
    memcpy(&g_engine_interface.context_004->unknown_6c, g_engine_interface.context_004->saved_7c, 16);
    g_engine_interface.context_004->difficulty = g_engine_interface.context_004->unknown_0f;
}

void eng3_1a560(void)
{
    int dtd;

    g_engine_interface.context_004->pressed_5e = 0;
    g_engine_interface.context_004->held_5c = 0;
    g_engine_interface.context_004->unknown_64 = 0;
    g_engine_interface.context_004->unknown_62 = 0;
    g_eng3_4254c = 0;
    g_eng3_3fa44 = 0;
    g_engine_interface.context_004->unknown_03 = 0;
    g_engine_interface.context_004->tick = 0;
    g_eng3_frame_41448 = g_eng3_frame_4144c;
    eng3_slot_140(&g_eng3_clear_rect_381c0, 0, 0, 0);
    g_eng3_after_draw_4064c = g_eng3_levels_39870[g_engine_interface.context_004->level].after_draw_10;
    g_eng3_actor_431d4 = g_eng3_actor_42540;
    eng3_08340(g_eng3_files_38528[g_engine_interface.context_004->level].arg_10,
               g_eng3_files_38528[g_engine_interface.context_004->level]
                   .subs_0c[g_engine_interface.context_004->sublevel].arg_08);
    eng3_1af10();
    g_eng3_431d0 = g_eng3_levels_39870[g_engine_interface.context_004->level].unknown_00;
    g_eng3_431d1 = g_eng3_levels_39870[g_engine_interface.context_004->level].unknown_01;
    g_eng3_431d2 = g_eng3_levels_39870[g_engine_interface.context_004->level].unknown_02;
    g_eng3_colour_40630 = -1;
    eng3_0aaa0(0, 0, 0, g_eng3_35168, g_eng3_35168, g_eng3_35168);
    g_eng3_4063e = 0x7fff;
    g_eng3_spread_4063b = 0x10;
    g_eng3_spread_4063c = 0;
    eng3_22790();
    eng3_082d0();
    eng3_178a0();
    eng3_1c720();
    eng3_1b9c0();
    eng3_slot_144(g_eng3_frame_4144c[0].ot, 0x800);
    eng3_slot_144(g_eng3_frame_4144c[1].ot, 0x800);
    if (g_eng3_frame_41448 == g_eng3_frame_4144c)
        g_eng3_prim_43164 = g_eng3_prims_405c4
            + g_eng3_levels_39870[g_engine_interface.context_004->level].prim_count_08 * 40;
    else
        g_eng3_prim_43164 = g_eng3_prims_405c4;
    eng3_1fb20(&g_eng3_frame_4144c[0], &g_eng3_frame_4144c[1]);
    dtd = g_eng3_levels_39870[g_engine_interface.context_004->level].flags_04 & 1;
    g_eng3_frame_4144c[1].dtd = dtd;
    g_eng3_frame_4144c[0].dtd = dtd;
    g_eng3_frame_4144c[1].dfe = 0;
    g_eng3_frame_4144c[0].dfe = 0;
    g_eng3_frame_4144c[1].isbg = g_eng3_levels_39870[g_engine_interface.context_004->level].flags_04 & 2;
    g_eng3_frame_4144c[0].isbg = g_eng3_frame_4144c[1].isbg;
    eng3_1ffa0(g_eng3_frame_4144c[0].prim_70, 1, dtd, eng3_1fe00(1, 0, 0x300, 0x100), 0);
    eng3_1ffa0(g_eng3_frame_4144c[1].prim_70, 1, dtd, eng3_1fe00(1, 0, 0x300, 0x100), 0);
    eng3_1ffa0(g_eng3_frame_4144c[0].prim_7c, 1, dtd, eng3_1fe00(1, 0, 0x380, 0x100), 0);
    eng3_1ffa0(g_eng3_frame_4144c[1].prim_7c, 1, dtd, eng3_1fe00(1, 0, 0x380, 0x100), 0);
    g_eng3_43178 = 1;
    eng1_color_fade_step_5d50();
    eng3_1fc60();
    title_04870();
    eng3_1fc70();
    eng3_1fb80(g_eng3_frame_41448);
    eng3_1fb80(g_eng3_frame_41448);
    eng3_22070();
    eng3_222c0();
    eng3_20190();
    eng3_1c290();
    eng3_1ff30(g_eng3_frame_41448);
    eng3_1ff20(g_eng3_frame_41448->disp_5c);
    g_eng3_frame_41448 = g_eng3_frame_41448 == g_eng3_frame_4144c ? &g_eng3_frame_4144c[1] : g_eng3_frame_4144c;
}

void eng3_1a8f0(int wait)
{
    if (wait)
        while (!eng3_slot_034(0))
            ;
    else
        eng3_slot_034(0);
    eng3_1af10();
}

int eng3_1a930(void)
{
    unsigned long size;

    eng3_slot_16c(0);
    eng3_slot_034(0);
    size = eng3_slot_048(g_eng3_files_38528[g_engine_interface.context_004->level].map_00, "MAP_PSX.BIN");
    if (!eng3_slot_078((void **)&g_eng3_map_40790, (size & 0xfffff800) + 0x1000, 0))
        return -1;
    g_engine_interface.context_004->unknown_128 |= 2;
    eng3_slot_068(g_eng3_files_38528[g_engine_interface.context_004->level].map_00, "MAP_PSX.BIN",
                  g_eng3_map_40790, size);
    g_engine_interface.context_004->unknown_90 = *(unsigned long *)g_eng3_map_40790;
    if (g_engine_interface.context_004->unknown_90 != g_engine_interface.context_004->unknown_8c)
        for (;;)
            ;
    eng3_1fbb0(2);
    size = eng3_1aa30(size);
    eng3_slot_16c(0);
    eng3_1fad0(g_eng3_map_40790, size, 0, 0);
    return 0;
}

long eng3_1aa30(unsigned long size)
{
    Rect16 rect;
    unsigned long *p;
    unsigned long *image;
    long end;

    p = (unsigned long *)(g_eng3_map_40790 + 0x18);
    image = p + (*p >> 2);
    rect.w = 0x300;
    rect.h = 0x200;
    end = (unsigned char *)image - g_eng3_map_40790 + 0x20;
    rect.x = 0;
    rect.y = 0;
    eng3_1fe30(&rect, image);
    p++;
    rect.x = 0x300;
    rect.y = 0x100;
    rect.w = 0x100;
    rect.h = 0x100;
    eng3_1fe30(&rect, p + (*p >> 2));
    return end;
}

void eng3_1aac0(Actor *o)
{
    o->sprite = 0;
    o->render_flags = 0;
    o->flags = 2;
    o->attachment = g_focus;
    o->unknown_f0 = (long)g_eng3_script_381d8;
    g_eng3_actor_431d4 = o;
}

void eng3_1aaf0(Actor *o)
{
    o->sprite = 0;
    o->render_flags = 0;
    o->flags = 2;
    o->attachment = g_focus;
    o->unknown_f0 = (long)g_eng3_script_38210;
    g_eng3_actor_431d4 = o;
}

void eng3_1ab20(Actor *o)
{
    eng3_01890(o);
    g_eng3_41380[++g_eng3_41380[0]] = o->shape.max_x;
    g_eng3_40f00[++g_eng3_40f00[0]] = o->shape.max_z;
    g_eng3_40f20[++g_eng3_40f20[0]] = o->shape.max_y;
    if (g_eng3_actor_431d4 == o->attachment)
        g_eng3_41048 = 1;
}

void eng3_1abb0(void)
{
    switch (g_eng3_3fb84) {
    case 0:
        eng3_1fea0(8, 8);
        g_engine_interface.context_004->sublevel = 0;
        g_engine_interface.context_004->unknown_02 = 0;
        g_engine_interface.context_004->unknown_0a = 3;
        g_engine_interface.context_004->unknown_8c = g_eng3_version_38518;
        g_engine_interface.context_004->unknown_94 = g_eng3_version_38510;
        g_engine_interface.context_004->unknown_48 = 0x13546547;
        g_engine_interface.context_004->unknown_4c = 0xecab9ab8;
        eng3_slot_05c();
        fx_nop_83d3();
        fx_init_82b7(0);
        eng3_slot_06c(g_eng3_files_38528[g_engine_interface.context_004->level].arg_14);
        eng3_slot_3e0(1);
        fx_mode_827f(1);
        g_eng3_3fb80 = 0;
        eng3_1a330();
        eng3_1a930();
        eng3_25060();
        eng3_1b8f0();
        demo_load_5ea0();
        eng3_1fe80(g_engine_interface.context_004->level, 0);
    restart:
        eng3_1a560();
        eng3_1fbb0(4);
        eng3_slot_060();
        if (!g_eng3_3fb80)
            eng3_1fb60(0, 0);
        g_eng3_3fb80 = 1;
        g_eng3_vsync_gate_41444 = 0;
        eng3_slot_3e4(eng3_1a320);
        g_eng3_4254c = 8;
        g_eng3_35168 = 0x80;
        g_eng3_3fa40 = 0;
        eng3_1fb10(0);
        g_engine_interface.context_004->unknown_1b30 = 0x100;
        eng3_1fb40(1);
        g_engine_interface.context_004->unknown_3c = g_eng3_3fc70;
        g_engine_interface.context_004->unknown_40 = 0;
        g_engine_interface.context_004->unknown_04 = 0;
        g_engine_interface.context_004->unknown_05 = 1;
        g_engine_interface.context_004->unknown_03 = 5;
        g_engine_interface.context_004->unknown_54 = eng3_slot_3e0(1);
        eng3_empty_1000("-------------------------------------------\n");
        g_eng3_3fb84 = 1;
        break;
    case 1:
        break;
    default:
        return;
    }
    g_engine_interface.context_004->unknown_34 = eng3_slot_2ec(g_engine_interface.unknown_010 + 0x400);
    eng3_slot_084();
    demo_step_6005();
    dispatch_local_02180();
    eng3_slot_02c();
    eng3_slot_08c();
    eng3_slot_3e4(eng3_1a320);
    eng1_color_fade_step_5d50();
    eng3_0a700();
    eng3_08540();
    if (!g_engine_interface.context_004->unknown_04) {
        eng3_222c0();
        eng3_22070();
        eng3_20190();
        eng3_08b20();
        eng3_1b9f0();
        eng3_085a0();
        eng3_174e0();
        eng3_01880();
        eng3_1bd40();
    } else {
        eng3_20190();
        eng3_08b20();
        eng3_085a0();
    }
    eng3_250f0();
    eng3_slot_16c(0);
    if (g_eng3_3fc70 < g_engine_interface.context_004->unknown_3c + 1)
        eng3_slot_3e0(0);
    g_eng3_vsync_gate_41444 = 0;
    eng3_slot_3e0(0);
    g_eng3_vsync_gate_41444 = 1;
    g_engine_interface.context_004->unknown_3c = g_eng3_3fc70;
    g_engine_interface.context_004->unknown_54 = eng3_slot_3e0(1);
    eng3_1fb80(g_eng3_frame_41448);
    if (!g_engine_interface.context_004->unknown_04)
        eng3_08050();
    title_041d0();
    title_04870();
    eng3_slot_164(g_eng3_frame_41448->ot);
    fx_update_666b();
    eng3_slot_2ec(g_engine_interface.context_004->unknown_34);
    g_eng3_frame_41448 = g_eng3_frame_41448 == g_eng3_frame_4144c ? &g_eng3_frame_4144c[1] : g_eng3_frame_4144c;
    g_engine_interface.context_004->tick++;
    if (game_state_60f9())
        goto restart;
}

void eng3_1af10(void)
{
    unsigned short *h;
    unsigned long *p;
    long colour;
    int shapes, lanes, layers, shades, cells, entries, groups;
    int i;

    h = (unsigned short *)(g_eng3_map_40790 + 8);
    shapes = *h++;
    lanes = *h++;
    layers = *h++;
    shades = *h++;
    cells = *h++;
    entries = *h++;
    groups = *h++;
    h++;
    g_eng3_map_40650.layer_count_186 = layers;
    g_eng3_map_40650.count_189 = cells;
    g_eng3_map_40650.count_18a = entries;
    g_eng3_map_40650.lane_count_187 = lanes;
    g_eng3_map_40650.shape_count_188 = shades;
    g_eng3_map_40650.groups_185 = groups;
    g_eng3_map_40650.lane_limit_184 = lanes > 6 ? 6 : lanes;
    p = (unsigned long *)h + 4;
    g_eng3_map_40650.table_50 = p + (*p >> 2);
    p++;
    g_eng3_map_40650.uvs_58 = (unsigned char *)(p + (*p >> 2));
    p++;
    g_eng3_map_40650.layers_74 = (Eng3Layer *)(p + (*p >> 2));
    p++;
    colour = *p++;
    g_eng3_40628 = colour;
    g_eng3_40629 = colour >> 8;
    g_eng3_4062a = colour >> 16;
    for (i = 0; i < lanes; i++, p++) {
        g_eng3_map_40650.lanes_84[i] = (unsigned long *)((unsigned short *)p + (*p >> 1));
        g_eng3_map_40650.lane_columns_144[i] = ((short *)g_eng3_map_40650.lanes_84[i])[0];
        g_eng3_map_40650.lane_rows_104[i] = ((short *)g_eng3_map_40650.lanes_84[i])[1];
        g_eng3_map_40650.lanes_84[i]++;
    }
    g_eng3_map_40650.table_80 = p + (*p >> 2);
    p++;
    g_eng3_map_40650.cell_flags_7c = (unsigned short *)(p + (*p >> 2));
    p++;
    g_eng3_map_40650.shades_78 = p + (*p >> 2);
    p++;
    g_eng3_map_40650.table_6c = p + (*p >> 2);
    p++;
    g_eng3_map_40650.shapes_5c = (short *)p;
    g_eng3_map_40650.shape_table_54 = p + shapes - 1;
    g_eng3_map_40650.table_60 = p + (shades + shapes);
    g_eng3_map_40650.table_64 = p + (cells + shades + shapes);
    g_eng3_map_40650.table_70 = p + (entries + cells + shades + shapes);
    g_eng3_map_40650.table_68 = p + (groups + entries + cells + shades + shapes);
}

short eng3_1b100(long fx, long fz, signed char *p)
{
    int type, xi, zi;
    unsigned short h0, h1, h2, h3;
    long a, b;

    type = *p++;
    if (type == 0) {
        h0 = *p++ << 1;
        if (h0 & 0x100)
            return -1;
        if (h0 & 0x80)
            return h0 & 0x7f;
        h1 = (signed char)(*p & 0x3f) << 1;
        return ((h1 - h0) * (fx & g_eng3_grid_39a68[0].mask) * g_eng3_grid_39a68[0].scale >> 8) + h0;
    }
    if (!(type & ~0xf)) {
        xi = type & 0xf;
        p += g_eng3_grid_39a68[xi].step * fx >> 8;
        h0 = *p++ << 1;
        if (h0 & 0x100)
            return -1;
        if (h0 & 0x80)
            return h0 & 0x7f;
        h1 = (signed char)(*p & 0x3f) << 1;
        return ((g_eng3_grid_39a68[xi].mask & fx) * g_eng3_grid_39a68[xi].scale * (h1 - h0) >> 8) + h0;
    }
    xi = type & 0xf;
    zi = (type >> 4) - 1;
    p += g_eng3_grid_39a68[xi].step * fx >> 8;
    p += (g_eng3_grid_39a68[zi].step * fz >> 8) * (g_eng3_grid_39a68[xi].stride + 1);
    h0 = *p++ << 1;
    if (h0 & 0x100)
        return -1;
    if (h0 & 0x80)
        return h0 & 0x7f;
    h1 = (signed char)(*p & 0x3f) << 1;
    p += g_eng3_grid_39a68[xi].stride;
    a = ((h1 - h0) * g_eng3_grid_39a68[xi].scale * (g_eng3_grid_39a68[xi].mask & fx) >> 8) + h0;
    h2 = *p++ << 1;
    if (h2 & 0x100)
        return -1;
    h3 = (signed char)(*p & 0x3f) << 1;
    b = ((h3 - h2) * g_eng3_grid_39a68[xi].scale * (g_eng3_grid_39a68[xi].mask & fx) >> 8) + h2;
    return ((g_eng3_grid_39a68[zi].mask & fz) * g_eng3_grid_39a68[zi].scale * (b - a) >> 8) + a;
}

long eng3_1b2e0(unsigned short x, short y, unsigned short z, int layer)
{
    unsigned char *entry;
    unsigned long *cell;
    short *shape;
    long value, flip, height, above, below, off;
    long cx, cz;
    int fx, fz, count, lane, r;

    z += 0x300;
    above = 0x7fff;
    below = 0x7fff;
    if (x >= 0x8000)
        x = 0x80;
    fx = (x >> 1) & 0x3f;
    fz = (z >> 1) & 0x3f;
    entry = (unsigned char *)&g_eng3_map_40650.layers_74[layer].entries + g_eng3_map_40650.layers_74[layer].entries;
    cx = x >> 7;
    cz = z >> 7;
    count = *entry++;
    while (count--) {
        lane = *entry++;
        height = (signed char)*entry++ << 5;
        if ((unsigned short)cz >= (unsigned short)g_eng3_map_40650.lane_columns_144[lane])
            continue;
        if ((unsigned short)cx >= (unsigned short)g_eng3_map_40650.lane_rows_104[lane])
            continue;
        cell = &g_eng3_map_40650.lanes_84[lane][cx * g_eng3_map_40650.lane_columns_144[lane] + cz];
        value = *cell;
        if (value & 1)
            continue;
        flip = value & 0x1000000;
        shape = g_eng3_map_40650.shapes_5c + ((value >> 1) & 0x3ff) * 2 - 1;
        off = *shape;
        if (!off)
            continue;
        r = eng3_1b100(flip ? 0x3f - fz : fz, fx, (signed char *)shape + off);
        if (r == -1)
            continue;
        height = height - r * 2 - (((signed char)(*cell >> 24) & ~1) << 1) - y + 0x40;
        if (height < 0) {
            if (height < below)
                below = -height;
        } else if (height < above) {
            above = height;
        }
    }
    if (above == 0x7fff && g_eng3_4063e != 0x7fff) {
        above = g_eng3_4063e - (unsigned short)y;
        if (above < 0)
            above = 0;
    }
    return (below << 16) + above;
}

unsigned short eng3_1b4a0(unsigned short x, short y, unsigned short z, int layer)
{
    unsigned char *entry;
    unsigned long value;
    unsigned long *cell;
    unsigned char *shape;
    long cx, cz, height, dy;
    int flag, info;
    int count, lane, xs, ys, zs;

    entry = (unsigned char *)&g_eng3_map_40650.layers_74[layer].entries + g_eng3_map_40650.layers_74[layer].entries;
    z += 0x300;
    x -= 0x40;
    count = *entry++;
    cz = z >> 7;
    cx = x >> 7;
    while (count--) {
        lane = *entry++;
        height = (signed char)*entry++ << 5;
        if ((unsigned short)cz >= (unsigned short)g_eng3_map_40650.lane_columns_144[lane])
            continue;
        if ((unsigned short)cx >= (unsigned short)g_eng3_map_40650.lane_rows_104[lane])
            continue;
        cell = &g_eng3_map_40650.lanes_84[lane][cx * g_eng3_map_40650.lane_columns_144[lane] + cz];
        dy = height - (((signed char)(*cell >> 24) & ~1) << 1) - y + 0x40;
        flag = g_eng3_map_40650.cell_flags_7c[((*cell >> 11) & 0x1fff) * 2];
        if (!flag)
            continue;
        shape = (unsigned char *)&g_eng3_map_40650.table_68[flag - 1] + g_eng3_map_40650.table_68[flag - 1];
        info = *(unsigned short *)shape;
        shape += 2;
        if (!info)
            return *(signed char *)shape;
        xs = (unsigned char)info >> 4;
        ys = info >> 8;
        zs = info & 0xf;
        if (xs)
            shape += ((0x7f - (x & 0x7f)) >> 1) * g_eng3_grid_39a68[xs].step >> 8;
        if (ys)
            shape += (g_eng3_grid_39a68[ys].step * (dy >> 1) >> 8) * g_eng3_grid_39a68[xs].stride;
        if (zs)
            shape += (g_eng3_grid_39a68[zs].step * ((z >> 1) & 0x3f) >> 8) * g_eng3_grid_39a68[ys].stride
                     * g_eng3_grid_39a68[xs].stride;
        return *(signed char *)shape;
    }
    return 0;
}

int eng3_1b650(short *out, int id, int mode, long pos)
{
    unsigned short *left, *right;
    unsigned long *p;
    long frac;
    long next;
    long l1, l3, r1, r3;
    int count, i;

    if (id >= g_eng3_map_40650.groups_185)
        return 0;
    if (pos < 0)
        pos = 0;
    p = &g_eng3_map_40650.table_70[id];
    p += *p >> 2;
    count = *(unsigned short *)p;
    right = (unsigned short *)(p + 1);
    left = right;
    next = 0;
    for (i = 1; i < count; i++) {
        right += 8;
        next = right[0];
        if (next > pos)
            break;
        left = right;
    }
    frac = next - left[0];
    if (frac)
        frac = ((pos - left[0]) << 16) / frac;
    if (mode) {
        r1 = right[1];
        l3 = left[3];
        l1 = left[1];
        r3 = right[3];
        out[0] = (((l3 + r1 - l1 - r3) * frac) >> 16) + l1 - l3 - 0x300;
        out[2] = (((r3 + r1 - l1 - l3) * frac) >> 16) + l3 + l1 - 0x300;
    }
    if (mode) {
        r1 = right[2];
        r3 = right[4];
        l1 = left[2];
        l3 = left[4];
        out[3] = out[1] = l1 + ((((r1 + r3 - l1 - l3) * frac) >> 16) + l3);
    }
    if (mode)
        out[4] = ((((signed char *)right)[0xa] - ((signed char *)left)[0xa]) * frac >> 16)
                 + (short)((signed char *)left)[0xa];
    if (mode)
        out[5] = ((((signed char *)right)[0xc] - ((signed char *)left)[0xc]) * frac >> 16)
                 + (short)((signed char *)left)[0xc];
    if (mode)
        out[6] = ((((signed char *)right)[0xc] - ((signed char *)left)[0xc]) * frac >> 16)
                 + (short)((signed char *)left)[0xc];
    return 1;
}

int eng3_1b7f0(Actor *o)
{
    long x;

    x = o->x >> 16;
    eng3_1eeb0(o);
    if (x < range_first(o->profile)) {
        release_actor(o);
        return 0;
    }
    if (x > range_end(o->profile))
        return 0;
    return eng3_24a80(o) != -1;
}

void eng3_1b890(Actor *o)
{
    if (eng3_1b7f0(o)) {
        if (g_eng3_unknown_3fb88 < o->unknown_0e)
            g_eng3_unknown_3fb88 = o->unknown_0e;
        g_eng3_unknown_3fb8c = 1;
    }
}

void eng3_1b8c0(Actor *o)
{
    if (eng3_1b7f0(o)) {
        if (g_eng3_unknown_38520 > o->unknown_0e)
            g_eng3_unknown_38520 = o->unknown_0e;
        g_eng3_unknown_3fb8c = 1;
    }
}

void eng3_1b8f0(void)
{
    g_eng3_41048 = 0;
    g_eng3_40bc6 = 0;
    initialize_pending_action_groups();
    title_08c90();
    title_09090();
    title_09480();
}

int eng3_1b920(Actor *o, int margin)
{
    long lo, hi;

    lo = range_first(o->profile);
    if (lo == -1)
        return 1;
    hi = range_end(o->profile);
    if (hi == -1)
        return 1;
    if ((short)(o->x >> 16) < lo - (unsigned short)margin
        || (short)(o->x >> 16) > hi + (unsigned short)margin + 0x100)
        return 1;
    return 0;
}

void eng3_1b9c0(void)
{
    eng1_call_01e50();
    eng3_17130();
    eng3_1c010();
    eng3_01870();
    g_eng3_40b92 = 3;
}

void eng3_1b9e0(void)
{
    reset_normal_actor_pool();
    eng3_process_middle_actor_pool();
    reset_alternate_actor_pool();
}

void eng3_1b9f0(void)
{
    int i;
    Eng3Host *h;
    Eng3Middle *m;
    Actor *a;
    unsigned long flags;
    unsigned short kind;

    g_eng3_41044 = 0;
    g_eng3_412a4 = g_eng3_actor_42540->z >> 16;
    g_eng3_412a6 = g_eng3_actor_42540->z >> 16;
    g_eng3_40dc4 = g_eng3_sine_38688[(g_engine_interface.context_004->tick & 0x7f) << 3] * 64 >> 16;
    g_eng3_40ee4 = g_eng3_sine_38688[((g_engine_interface.context_004->tick + 0x10) << 4) & 0x3ff] * 80 >> 16;
    g_eng3_41380[0] = 0;
    g_eng3_40f20[0] = 0;
    g_eng3_40f00[0] = 0;
    g_eng3_412c0 = 0;
    eng3_1c430();
    g_eng3_3fb98 = g_eng3_3fb94;
    g_eng3_3fb94 = g_eng3_camera_x_405e0;
    g_eng3_40d20[0] = 0;
    g_eng3_40fa0[0] = 0;
    g_eng3_40f40[0] = 0;
    g_eng3_40de0[0] = 0;
    g_eng3_411c0[0] = 0;
    g_eng3_41200[0] = 0;
    g_eng3_40e40[0] = 0;
    for (i = g_eng3_40ba8; i < g_eng3_40bd2; i++) {
        h = &g_eng3_actor_431c0[i];
        kind = h->kind;
        if (kind) {
            if (h->flags & 0x40)
                g_eng3_411c0[++g_eng3_411c0[0]] = (long)h;
            if (h->flags & 1)
                g_eng3_host_40bb4[kind & 0xfff].update(h);
        }
    }
    for (i = g_eng3_40bc2; i < g_eng3_40c32; i++) {
        m = &((Eng3Middle *)g_eng3_43174)[i];
        kind = m->kind;
        if (kind) {
            flags = m->flags;
            if (!(flags & 0x800)) {
                m->prev_x = m->x;
                m->prev_y = m->y;
            }
            if (flags & 0x40000)
                g_eng3_40d20[++g_eng3_40d20[0]] = (long)m;
            if (flags & 0x100000)
                g_eng3_41200[++g_eng3_41200[0]] = (long)m;
            if (flags & 1)
                g_eng3_middle_40b98[kind & 0xfff].update(m);
            if (flags & 0x400000)
                g_eng3_40e40[++g_eng3_40e40[0]] = (long)m;
        }
    }
    for (i = g_eng3_40b94; i < g_eng3_40bcc; i++) {
        Actor *base = g_eng3_actor_42540;
        a = base + i;
        if (a->kind) {
            flags = a->flags;
            if (!(flags & 0x800)) {
                a->prev_x = a->x;
                a->prev_y = a->y;
                a->prev_z = a->z;
            }
            if (flags & 0x100000)
                g_eng3_41200[++g_eng3_41200[0]] = (long)a;
            if (flags & 0x80)
                g_eng3_40de0[++g_eng3_40de0[0]] = (long)a;
            if (!(flags & 0x10)) {
                if (a->unknown_ec)
                    a->unknown_ec--;
                else
                    a->unknown_ed = 0;
            } else {
                g_eng3_40f40[++g_eng3_40f40[0]] = (long)a;
            }
            if (flags & 0x240000)
                g_eng3_40d20[++g_eng3_40d20[0]] = (long)a;
            if (flags & 1)
                ((Eng3Hooks12 *)g_eng3_game_40b8c)[a->kind].update(a);
            if (flags & 0x400000)
                g_eng3_40fa0[++g_eng3_40fa0[0]] = (long)a;
        }
    }
}

void eng3_1bd40(void)
{
    int i;
    Eng3Host *h;
    Eng3Middle *m;
    Actor *a;
    unsigned short kind;

    g_eng3_40cc0[0] = 0;
    g_eng3_41100[0] = 0;
    g_eng3_413a0[0] = 0;
    eng3_1f1a0(g_focus, &g_focus->shape, 0);
    g_eng3_3fba8 = 0;
    g_eng3_3fba4 = 0;
    if (g_eng3_40b92)
        g_eng3_40b92--;
    for (i = g_eng3_40ba8; i < g_eng3_40bd2; i++) {
        h = &g_eng3_actor_431c0[i];
        kind = h->kind;
        if (kind) {
            if (h->flags & 0x10000)
                g_eng3_413a0[++g_eng3_413a0[0]] = (long)h;
            if (h->flags & 2)
                g_eng3_host_40bb4[kind & 0xfff].update(h);
        }
    }
    for (i = g_eng3_40bc2; i < g_eng3_40c32; i++) {
        m = &((Eng3Middle *)g_eng3_43174)[i];
        kind = m->kind;
        if (kind && (m->flags & 2))
            g_eng3_middle_40b98[kind & 0xfff].update(m);
    }
    for (i = g_eng3_40b94; i < g_eng3_40bcc; i++) {
        a = &g_eng3_actor_42540[i];
        kind = a->kind;
        if (kind && (a->flags & 2))
            ((Eng3Hooks12 *)g_eng3_game_40b8c)[kind].draw(a);
    }
    for (i = 0; i < g_eng3_413a0[0];) {
        h = (Eng3Host *)g_eng3_413a0[++i];
        g_eng3_host_40bb4[h->kind & 0xfff].update(h);
    }
    a = g_eng3_actor_41c60;
    kind = a->kind;
    if (kind)
        ((Eng3Hooks12 *)g_eng3_game_40b8c)[kind].draw(a);
    eng3_1c5c0();
    if (g_eng3_levels_39870[g_engine_interface.context_004->level].unknown_03) {
        a = g_eng3_actor_42540;
        for (i = 0; i < g_eng3_40bcc; i++, a++)
            a->x = ((a->x - 0x800000) & 0x1fffffff) + 0x800000;
        m = ((Eng3Middle *)g_eng3_43174);
        for (i = 0; i < g_eng3_40c32; i++, m++)
            m->x = ((m->x - 0x800000) & 0x1fffffff) + 0x800000;
        h = g_eng3_actor_431c0;
        for (i = 0; i < g_eng3_40bd2; i++, h++)
            h->x = ((h->x - 0x800000) & 0x1fffffff) + 0x800000;
    }
}
}
