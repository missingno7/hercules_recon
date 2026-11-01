extern "C" {
/* ENG3.DLL 0x14190..0x16240: level-object handlers (lane y9 candidate).
 * Actor/context offsets are provisional shared names, not recovered original types.
 * C form; region.cpp is the same text inside extern "C" (0x14850 equal only there, a
 * whole-unit context effect: standalone it differs by one register in both languages). */
typedef struct Actor Actor;

typedef struct Bounds3D {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds3D;

struct Actor {
    long x, y, z;
    unsigned char unknown_0c[0x1c - 0x0c];
    unsigned char unknown_1c;
    unsigned char unknown_1d[0x22 - 0x1d];
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
    unsigned char unknown_3e[0x4a - 0x3e];
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
    unsigned char unknown_9c[0xa8 - 0x9c];
    long unknown_a8, unknown_ac, unknown_b0, unknown_b4;
    long unknown_b8, unknown_bc, unknown_c0, unknown_c4;
    long unknown_c8, unknown_cc, unknown_d0, unknown_d4;
    long unknown_d8, unknown_dc, unknown_e0;
    unsigned char unknown_e4[0xec - 0xe4];
    unsigned char unknown_ec;
    unsigned char unknown_ed;
    short unknown_ee;
    unsigned char unknown_f0[0x116 - 0xf0];
    short unknown_116;
    unsigned char unknown_118[0x12c - 0x118];
    const long *unknown_12c;
};

/* Interface context (g_engine_interface.context_004). */
typedef struct LevelContext {
    signed char level;
    signed char sublevel;
    unsigned char unknown_02;
    unsigned char unknown_03[0x0c - 0x03];
    signed char unknown_0c;
    unsigned char unknown_0d[0x11 - 0x0d];
    unsigned char flags_11;
    unsigned char unknown_12[0x14 - 0x12];
    short unknown_14;
    unsigned char unknown_16[0x18 - 0x16];
    short unknown_18;
    unsigned char unknown_1a[0x1c - 0x1a];
    unsigned short unknown_1c;
    short bar_length_target;
    short bar_length;
    short bar_value;
    short bar_value_target;
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

/* Follower trail: five 16-byte position records at 0x41d60, cursor at 0x37b60. */
typedef struct TrailPoint {
    long x, y, z;
    unsigned short profile;
    short unknown_0e;
} TrailPoint;

extern EngineInterfaceView g_engine_interface;
extern KindInfo g_kind_info[];
extern Actor *g_focus;
extern Actor *g_eng3_actor_431d4;
extern TrailPoint g_eng3_trail_41d60[];
extern TrailPoint *g_eng3_trail_cursor_37b60;
extern unsigned char g_eng3_image_37b24[];
extern unsigned char g_eng3_image_37b28[];
extern unsigned char g_eng3_image_37658[];
extern unsigned char g_eng3_image_375f0[];
extern unsigned char g_eng3_image_37618[];
extern unsigned char g_eng3_image_37678[];
extern short g_eng3_37b70[];
extern unsigned short g_eng3_water_level_410e0;
extern short g_eng3_40dc4;
extern long g_eng3_40ee4;
extern int g_eng3_3fad8;
extern int g_eng3_3fae4;
extern int g_eng3_3fae8;
extern int g_eng3_3faec;
extern int g_eng3_3fb20;
extern short g_eng3_41c72;
extern int g_eng3_41cb0;
extern int g_eng3_41d50;

extern unsigned int eng3_01010(unsigned int);
extern int eng3_01460(Actor *);
extern int eng3_01600(Actor *);
extern void eng3_078f0(Actor *, void *);
extern void play_sound(unsigned short, Actor *);
extern long eng3_1b2e0(long, long, long, int);
extern int eng3_1b920(Actor *, int);
extern void remove_actor(Actor *);
extern void eng3_1cd40(Actor *);
extern void eng3_1cd80(Actor *);
extern void remove_actor_variant(Actor *);
extern Actor *attach_actor(Actor *, int);
extern void release_actor(Actor *);
extern int eng3_1dbc0(Actor *);
extern int eng3_1e000(Actor *);
extern void reset_frame_state(Actor *);
extern void select_frame(Actor *, int);
extern void eng3_1eeb0(Actor *);
extern int eng1_2a660(Actor *, Actor *);
extern int eng3_1f1a0(Actor *, Bounds3D *, unsigned short);
extern unsigned int random_limit(int);

void eng3_14190(Actor *o)
{
    if (g_engine_interface.context_004->level == 8)
        o->category = 6;
    o->flags = 0x802;
    if (((o->kind + 5) & 0xf) == g_engine_interface.context_004->unknown_02)
        o->image = g_eng3_image_37b24;
    else
        o->image = g_eng3_image_37b28;
    reset_frame_state(o);
    eng3_1e000(o);
    eng3_1f1a0(o, &o->shape, 0);
    eng3_1eeb0(o);
}

void eng3_14200(Actor *o)
{
    if (o->image == g_eng3_image_37b28) {
        if (eng1_2a660(o, g_focus)) {
            o->image = g_eng3_image_37b24;
            g_engine_interface.context_004->unknown_02 = (o->kind + 5) & 0xf;
            reset_frame_state(o);
        }
    } else if (g_engine_interface.context_004->unknown_02 != ((o->kind + 5) & 0xf)) {
        o->image = g_eng3_image_37b28;
        reset_frame_state(o);
    }
    eng3_1e000(o);
}

void eng3_14270(Actor *o)
{
    o->flags = 0x10000;
    o->mode = 0;
    if (g_engine_interface.context_004->level == 2)
        o->unknown_4a = 0x50;
    if (g_engine_interface.context_004->level == 8)
        o->unknown_4a = 0;
    o->scale = 0x12c;
    o->category = 1;
}

void eng3_142b0(Actor *o)
{
    Actor *p = o->attachment;
    unsigned long h;

    o->z = p->z;
    o->y = p->y - 0x80000;
    o->x = p->x + 0x40000;
    o->render_flags = 0x80000004;
    o->profile = p->profile;
    h = eng3_1b2e0(o->x >> 16, o->y >> 16, o->z >> 16, o->profile) << 16;
    if (h > 0x2000000) {
        o->render_flags &= 0x7fffffff;
    } else {
        o->y += h;
        o->render_flags |= 0x80000000;
        if (p->render_flags & 0x10000000) {
            o->scale = 0x300 - (h >> 16);
            if (g_engine_interface.context_004->level == 2) {
                switch (p->sprite) {
                case 2:
                    o->category = 1;
                    o->sprite = 0xcd;
                    o->scale = 0x300 - (h >> 16);
                    o->x = p->x + 0x80000;
                    break;
                case 11:
                    o->category = 1;
                    o->sprite = 0xce;
                    o->scale = 0x200 - (h >> 16);
                    o->x = p->x - 0x400000;
                    break;
                case 18:
                    o->category = 1;
                    o->sprite = 0xcd;
                    o->scale = 0x200 - (h >> 16);
                    o->x = p->x - 0x400000;
                    break;
                case 3:
                    o->category = 1;
                    o->sprite = 0xcf;
                    o->scale = 0xc8 - (h >> 16);
                    break;
                case 6:
                    o->category = 1;
                    o->sprite = 0xcf;
                    o->scale = 0x200 - (h >> 16);
                    break;
                case 9:
                    o->category = 1;
                    o->sprite = 0xcd;
                    o->scale = 0x200 - (h >> 16);
                    o->x = p->x + 0x100000;
                    eng3_1eeb0(o);
                    break;
                case 13:
                    o->category = 1;
                    o->sprite = 0xcf;
                    o->scale = 0x200 - (h >> 16);
                    o->x = p->x - 0x80000;
                    eng3_1eeb0(o);
                    break;
                }
            }
            if (g_engine_interface.context_004->level == 8) {
                switch (p->sprite) {
                case 33:
                    o->category = 1;
                    o->sprite = 0xcd;
                    o->scale = 0x200 - (h >> 16);
                    o->x = p->x - 0x200000;
                    break;
                case 1:
                    o->category = 1;
                    o->sprite = 0xcd;
                    o->scale = 0x200 - (h >> 16);
                    o->x = p->x + 0x200000;
                    break;
                case 2:
                    o->category = 1;
                    o->sprite = 0xcd;
                    o->scale = 0x200 - (h >> 16);
                    o->x = p->x + 0x200000;
                    break;
                case 11:
                    o->category = 1;
                    o->sprite = 0xcd;
                    o->scale = 0x200 - (h >> 16);
                    o->x = p->x + 0x200000;
                    break;
                case 3:
                    o->category = 1;
                    o->sprite = 0xcd;
                    o->scale = 0x200 - (h >> 16);
                    o->x = p->x + 0x400000;
                    break;
                }
            }
        }
    }
    if ((short)o->scale < 0)
        o->scale = 0;
}

void eng3_145d0(Actor *o)
{
    Actor *p = o->attachment;
    long h;

    o->x = p->x;
    o->y = p->y - 0x20000;
    o->z = p->z;
    o->render_flags &= 0xf3ffffef;
    o->render_flags |= p->render_flags & 0x10;
    o->render_flags |= 0x80000000;
    o->category = p->category;
    o->sprite = p->sprite;
    o->unknown_36 = p->unknown_36;
    o->render_flags |= p->render_flags & 0x8000000 | 0x4000000;
    o->profile = p->profile;
    o->unknown_4a = 0;
    o->speed = 0x190;
    o->step = 0x384;
    h = eng3_1b2e0(o->x >> 16, o->y >> 16, o->z >> 16, o->profile) << 16;
    if (h > 0x1000000) {
        o->render_flags &= 0x7fffffff;
        return;
    }
    o->y += h;
    o->scale = o->unknown_3c = p->scale - (h >> 17);
    o->speed = p->speed - 0xff;
    o->step = 0x40;
    o->unknown_4a = 0;
    o->speed = p->speed - 0xff;
}

void eng3_146d0(Actor *o)
{
    int i;

    o->sprite = 0;
    o->render_flags = 0;
    o->flags = 2;
    o->state = 4;
    g_eng3_actor_431d4 = o;
    o->unknown_116 = 0;
    for (i = 0; i < 5; i++) {
        g_eng3_trail_cursor_37b60->x = o->x;
        g_eng3_trail_cursor_37b60->y = o->y;
        g_eng3_trail_cursor_37b60->z = o->z;
        g_eng3_trail_cursor_37b60->profile = o->profile;
        g_eng3_trail_cursor_37b60->unknown_0e = 0;
        g_eng3_trail_cursor_37b60++;
    }
    g_eng3_trail_cursor_37b60 = g_eng3_trail_41d60;
}

void eng3_14760(Actor *o)
{
    if (o->unknown_116 >= 5) {
        g_eng3_trail_cursor_37b60 = g_eng3_trail_41d60;
        o->unknown_116 = 0;
    }
    g_eng3_trail_cursor_37b60->x = g_focus->x;
    g_eng3_trail_cursor_37b60->y = g_focus->y;
    g_eng3_trail_cursor_37b60->z = g_focus->z;
    g_eng3_trail_cursor_37b60->profile = g_focus->profile;
    g_eng3_trail_cursor_37b60++;
    o->unknown_116++;
    o->x = g_eng3_trail_41d60[0].x;
    o->y = g_eng3_trail_41d60[0].y;
    o->z = g_eng3_trail_41d60[0].z;
    o->profile = g_eng3_trail_41d60[o->unknown_116].profile;
}

void eng3_14850(Actor *o)
{
    Actor *p;
    long h, d;

    o->profile = g_focus->profile;
    p = o->attachment;
    if (~p->flags & 8) {
        o->y = p->y;
        o->x = p->x;
    }
    o->y -= 0x20000;
    o->z = p->z;
    o->render_flags &= ~0x10;
    o->render_flags |= p->render_flags & 0x10;
    o->render_flags |= 0x80000000;
    o->category = p->category;
    o->sprite = p->sprite;
    o->unknown_24 = p->unknown_24;
    o->unknown_4a = p->unknown_4a;
    p->render_flags &= 0xfff8ffff;
    o->render_flags &= 0xfff8fffe;
    h = eng3_1b2e0(o->x >> 16, o->y >> 16, o->z >> 16, o->profile) << 16;
    if (h > 0x1000000) {
        o->render_flags &= 0x7fffffff;
    } else {
        o->y += h;
        o->scale = o->unknown_3c = p->scale - (h >> 17);
        o->speed = p->speed - 0xff;
        o->step = 0x40;
    }
    if (g_eng3_water_level_410e0) {
        if (h > 0x1000000) {
            d = ((g_eng3_water_level_410e0 + 0x20) << 16) - o->y;
            if (d < 0)
                d = 0;
            if (d < 0x1000000) {
                o->y += d;
                o->render_flags |= 0x80000000;
                o->scale = o->unknown_3c = p->scale - (d >> 17);
                o->speed = p->speed - 0xff;
                o->step = 0x40;
            }
        }
        if (o->y > g_eng3_water_level_410e0 << 16) {
            o->speed = p->speed + 0xff;
            o->step = g_eng3_40dc4;
            o->unknown_3c += g_eng3_40ee4 + 0x20;
            o->render_flags |= 1;
            o->z = p->z - 0x40000;
            d = (p->y >> 16) - g_eng3_water_level_410e0;
            if (d > 0) {
                p->unknown_26 = d;
                o->render_flags |= 0x50000;
                p->render_flags |= 0x30000;
                o->unknown_26 = p->unknown_26 - 1;
                o->y = (g_eng3_water_level_410e0 + o->unknown_26) << 16;
                return;
            }
            o->y = g_eng3_water_level_410e0 << 16;
            return;
        }
        o->unknown_4a = 0;
        o->speed = p->speed - 0xff;
        o->unknown_26 = 0;
        p->unknown_26 = 0;
    }
}

void eng3_14a80(Actor *o)
{
    if (o) {
        o->render_flags = 0x80000005;
        o->sprite = 0xc;
        o->flags = 0x202;
        o->profile = g_focus->profile;
        o->scale = 0x80;
        o->unknown_4a = 0x80;
        attach_actor(o, 0x2017);
    }
}

void eng3_14ad0(Actor *o)
{
    if (o->sprite >= 0xc)
        release_actor(o);
    o->sprite++;
    if (o->z > 0x1800000)
        release_actor(o);
    if (o->z < -0x1800000)
        release_actor(o);
    if (o->x < g_focus->x - 0xc00000)
        release_actor(o);
}

void eng3_14b30(Actor *o)
{
    if (o) {
        if (g_engine_interface.context_004->level == 8)
            o->sprite = 0xd1;
        if (g_engine_interface.context_004->level == 2) {
            o->category = 3;
            o->sprite = 0x25c;
        }
        if (g_engine_interface.context_004->level == 10) {
            o->category = 4;
            o->sprite = 0xb4;
        }
        o->render_flags = 0x80000007;
        o->flags = 2;
        o->scale = 0x180;
        o->ticks = 1;
        o->unknown_4a = 0xc8;
        o->profile = g_focus->profile;
    }
}

void eng3_14bb0(Actor *o)
{
    if (o->ticks-- > 0) {
        if (g_engine_interface.context_004->level == 8 && o->sprite >= 0xde)
            release_actor(o);
        if (g_engine_interface.context_004->level == 2 && o->sprite >= 0x269)
            release_actor(o);
        if (g_engine_interface.context_004->level == 10 && o->sprite >= 0xc1)
            release_actor(o);
        o->sprite++;
        o->ticks = 1;
    }
}

void eng3_14c30(Actor *o)
{
    o->unknown_d8 = 0;
    o->flags = 2;
    if (o) {
        o->unknown_b8 = o->unknown_bc = random_limit(0x200000) + 0x200000;
        o->unknown_c0 = random_limit(0x10000) + 0x10000;
        o->unknown_c4 = random_limit(0x80) + 0x100;
        o->unknown_a8 = o->unknown_ac = o->unknown_bc >> 1;
        o->unknown_b4 = 0x100 - random_limit(0x20);
        o->unknown_b0 = 0x100 - random_limit(0x20);
        o->unknown_c8 = o->unknown_cc = o->unknown_ac >> 2;
        o->unknown_d0 = random_limit(0x100);
        o->unknown_d4 = random_limit(0x100);
        o->unknown_95 = 0;
        o->ticks = eng3_01010(0x20);
        attach_actor(o, 0x2017);
    }
}

void eng3_14d40(Actor *o)
{
    o->unknown_74 += o->ticks;
    if (eng3_01460(o)) {
        o->ticks >>= 1;
        o->unknown_d8 = 0;
    }
    if (o->x < g_focus->x - 0x200000)
        remove_actor_variant(o);
}

void eng3_14d90(Actor *o)
{
    if (o) {
        if (g_engine_interface.context_004->level == 8) {
            o->category = 6;
            o->render_flags = 0x80000000;
            o->image = g_eng3_image_37658;
        } else {
            o->render_flags |= 0x10000040;
        }
        o->unknown_d8 = 0;
        o->unknown_98 = 0;
        o->profile = g_focus->profile;
        o->flags = 2;
        o->unknown_b8 = o->unknown_bc = random_limit(0x200000) + 0x200000;
        o->unknown_c0 = random_limit(0x10000) + 0x10000;
        o->unknown_c4 = random_limit(0x80) + 0x100;
        o->unknown_a8 = o->unknown_ac = o->unknown_bc >> 1;
        o->unknown_b4 = 0x100 - random_limit(0x20);
        o->unknown_b0 = 0x100 - random_limit(0x20);
        o->unknown_c8 = o->unknown_cc = o->unknown_ac >> 2;
        o->unknown_d0 = random_limit(0x100);
        o->unknown_d4 = random_limit(0x100);
        o->unknown_95 = 0;
        o->scale = 0x258 - random_limit(0x190);
        o->ticks = 1;
        if (g_engine_interface.context_004->level == 2)
            o->sprite = 4;
        if (g_engine_interface.context_004->level == 10)
            o->sprite = 4;
    }
}

void eng3_14f00(Actor *o)
{
    if (g_engine_interface.context_004->level == 8) {
        if (eng3_01460(o))
            o->unknown_d8 = 0;
        else
            eng3_1dbc0(o);
    } else if (eng3_01600(o)) {
        o->speed = 0;
        o->step = 0;
        o->unknown_74 = 0;
    } else if (g_engine_interface.context_004->level != 8) {
        o->speed += random_limit(0x40);
        o->step += random_limit(0x40);
        o->unknown_74 += random_limit(0x40);
    }
    if (o->unknown_9a) {
        if (o->unknown_e0) {
            remove_actor_variant(o);
            return;
        }
        if (o->scale > 0x60)
            o->scale >>= 1;
    }
    if (o->x < g_focus->x - 0x800000)
        remove_actor_variant(o);
}

void eng3_14fd0(Actor *o)
{
    if (o) {
        o->category = 6;
        o->render_flags = 0x80000000;
        o->image = g_eng3_image_37658;
        o->unknown_d8 = 0;
        o->unknown_98 = 0;
        o->profile = g_focus->profile;
        o->flags = 2;
        o->unknown_b8 = o->unknown_bc = random_limit(0x200000) + 0x200000;
        o->unknown_c0 = random_limit(0x10000) + 0x10000;
        o->unknown_c4 = random_limit(0x80) + 0x100;
        o->unknown_a8 = o->unknown_ac = o->unknown_bc >> 1;
        o->unknown_b4 = 0x100 - random_limit(0x20);
        o->unknown_b0 = 0x100 - random_limit(0x20);
        o->unknown_c8 = o->unknown_cc = o->unknown_ac >> 2;
        o->unknown_d0 = random_limit(0x100);
        o->unknown_d4 = random_limit(0x100);
        o->unknown_95 = 0;
        o->scale = 0x258 - random_limit(0x190);
        o->ticks = 1;
    }
}

void eng3_15110(Actor *o)
{
    if (eng3_01460(o))
        o->unknown_d8 = 0;
    else
        eng3_1dbc0(o);
    if (o->unknown_9a && o->scale > 0x60)
        o->scale >>= 1;
    if (o->x < g_focus->x - 0x200000)
        remove_actor_variant(o);
}

void eng3_15170(Actor *o)
{
    if (o) {
        o->render_flags = 0x90000040;
        o->unknown_d8 = 0;
        o->unknown_98 = 0;
        o->profile = g_focus->profile;
        o->flags = 2;
        o->unknown_b8 = o->unknown_bc = random_limit(0x200000) + 0x200000;
        o->unknown_c0 = random_limit(0x10000) + 0x10000;
        o->unknown_c4 = random_limit(0x80) + 0x100;
        o->unknown_a8 = o->unknown_ac = o->unknown_bc >> 1;
        o->unknown_b4 = 0x100 - random_limit(0x20);
        o->unknown_b0 = 0x100 - random_limit(0x20);
        o->unknown_cc = o->unknown_ac;
        o->unknown_c8 = o->unknown_ac >> 1;
        o->unknown_d0 = random_limit(0x100);
        o->unknown_d4 = random_limit(0x100);
        o->unknown_95 = 0xff;
        o->scale = 0x258 - random_limit(0x190);
        o->ticks = 1;
        o->unknown_dc = eng3_01010(0x20) + 0x10;
        o->sprite = 0x25;
    }
}

void eng3_152c0(Actor *o)
{
    if (eng3_01600(o)) {
        o->step >>= 1;
        o->unknown_74 >>= 1;
        o->speed >>= 1;
        o->unknown_d8 = 0;
    } else {
        o->step += o->unknown_dc;
        o->unknown_74 += o->unknown_dc;
        o->speed += o->unknown_dc;
    }
    if (o->unknown_9a && o->scale > 0x60)
        o->scale >>= 1;
    if (o->x < g_focus->x - 0x200000)
        remove_actor_variant(o);
}

void eng3_15340(Actor *o)
{
    unsigned int c;

    c = (unsigned short)eng3_1b2e0(o->x >> 16, o->y >> 16, o->z >> 16, o->profile);
    if (c == 0x7fff) {
        o->unknown_25 |= 2;
        return;
    }
    o->unknown_25 &= ~2;
    if (c > 0x100) {
        o->render_flags &= ~8;
        return;
    }
    o->unknown_28 = c;
    o->render_flags |= 8;
}

void eng3_153b0(Actor *o)
{
    o->render_flags = 0x80000800;
    o->flags |= 0x10000000;
    g_eng3_37b70[0] = o->unknown_12c[0] >> 16;
    g_eng3_37b70[1] = o->unknown_12c[1] >> 16;
    g_eng3_37b70[2] = o->unknown_12c[2] >> 16;
    eng3_078f0(o, o->image);
}

void eng3_15440(Actor *o)
{
    Sidecar *s = (Sidecar *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);

    o->unknown_dc = 0;
    o->unknown_dc = s->kind;
    o->flags = 2;
    o->render_flags = 0x80000000;
    o->state = 8;
    o->unknown_bc = (random_limit(0x100000) + 0xf00000) >> 5;
    o->unknown_ac = 0;
    o->unknown_cc = 0;
    o->unknown_b8 = 0;
    o->unknown_c0 = 0x14000;
    o->unknown_c4 = 0x14000;
    eng3_1eeb0(o);
    eng3_15340(o);
    s->y = o->y;
    if (g_engine_interface.context_004->unknown_0c >= 9 && o->unknown_dc == 6)
        o->unknown_dc = 1;
    if (o->unknown_dc == 0) {
        o->y -= 0x100000;
        select_frame(o, 0);
    }
    if (o->unknown_dc == 1)
        select_frame(o, 1);
    if (o->unknown_dc == 2) {
        o->category = 2;
        select_frame(o, 2);
    }
    if (o->unknown_dc == 3)
        select_frame(o, 3);
    if (o->unknown_dc == 4) {
        o->category = 2;
        o->scale = 0xc0;
        o->unknown_1c = 8;
        select_frame(o, 4);
    }
    if (o->unknown_dc == 5)
        select_frame(o, 5);
    if (o->unknown_dc == 6)
        select_frame(o, 8);
    if (o->unknown_dc == 7)
        select_frame(o, 9);
    if (o->unknown_dc == 8) {
        o->category = 8;
        select_frame(o, 0xa);
    }
    if (o->unknown_dc == 9) {
        o->category = 8;
        select_frame(o, 0xb);
    }
    if (o->unknown_dc == 10) {
        o->category = 8;
        select_frame(o, 0xc);
    }
    if (o->unknown_dc == 11) {
        o->category = 8;
        select_frame(o, 0xd);
    }
    if (o->unknown_dc == 12) {
        o->category = 8;
        select_frame(o, 0xe);
    }
    if (o->unknown_dc == 13) {
        o->category = 8;
        select_frame(o, 0xf);
    }
    if (o->unknown_dc == 14) {
        o->category = 8;
        select_frame(o, 0xb);
    }
    if (o->unknown_dc == 15) {
        o->category = 8;
        select_frame(o, 0x10);
    }
}

void eng3_15680(Actor *o)
{
    Sidecar *s = (Sidecar *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);

    if (o->unknown_dc == 0 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        g_eng3_3fae4 = 1;
        play_sound(0x603e, g_focus);
        g_focus->unknown_ec = 0xc8;
        o->state = 2;
    }
    if (o->unknown_dc == 1 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        g_engine_interface.context_004->bar_value_target += 0x20;
        if (g_engine_interface.context_004->bar_value_target > g_engine_interface.context_004->bar_length_target)
            g_engine_interface.context_004->bar_value_target = g_engine_interface.context_004->bar_length_target;
        play_sound(0x603b, g_focus);
        o->state = 2;
    }
    if (o->unknown_dc == 2 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        play_sound(0x603f, g_focus);
        g_engine_interface.context_004->unknown_18++;
        o->state = 2;
    }
    if (o->unknown_dc == 3 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        g_focus->unknown_ee = 0x56;
        g_eng3_3fb20 = 1;
        g_eng3_3fae8 = 1;
        play_sound(0x602c, g_focus);
        o->render_flags = 0x84000005;
        o->state = 2;
    }
    if (o->unknown_dc == 4 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        play_sound(0x603f, g_focus);
        g_engine_interface.context_004->unknown_14++;
        o->state = 2;
    }
    if (o->unknown_dc == 5 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        o->state = 2;
        play_sound(0x603e, g_focus);
        g_eng3_3faec++;
        if (g_engine_interface.context_004->level == 2) {
            if (g_focus->profile == 0)
                g_engine_interface.context_004->flags_11 |= 1;
            if (g_focus->profile == 1 || g_focus->profile == 2)
                g_engine_interface.context_004->flags_11 |= 2;
            if (g_focus->profile == 3 || g_focus->profile == 4)
                g_engine_interface.context_004->flags_11 |= 4;
            if (g_focus->profile == 5 || g_focus->profile == 6)
                g_engine_interface.context_004->flags_11 |= 4;
            if (g_focus->profile == 7 || g_focus->profile == 8)
                g_engine_interface.context_004->flags_11 |= 8;
            if (g_focus->profile == 9 || g_focus->profile == 10)
                g_engine_interface.context_004->flags_11 |= 8;
        }
        if (g_engine_interface.context_004->level == 8) {
            if (g_eng3_3faec == 1)
                g_engine_interface.context_004->flags_11 |= 1;
            if (g_eng3_3faec == 2)
                g_engine_interface.context_004->flags_11 |= 2;
            if (g_eng3_3faec == 3)
                g_engine_interface.context_004->flags_11 |= 4;
            if (g_eng3_3faec == 4)
                g_engine_interface.context_004->flags_11 |= 8;
        }
        if (g_engine_interface.context_004->level == 10) {
            if (g_focus->profile == 0)
                g_engine_interface.context_004->flags_11 |= 1;
            if (g_focus->profile == 1)
                g_engine_interface.context_004->flags_11 |= 4;
            if (g_focus->profile == 2)
                g_engine_interface.context_004->flags_11 |= 4;
            if (g_focus->profile == 3)
                g_engine_interface.context_004->flags_11 |= 8;
            if (g_focus->profile == 4)
                g_engine_interface.context_004->flags_11 |= 8;
        }
    }
    if (o->unknown_dc == 6 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        g_engine_interface.context_004->unknown_0c++;
        g_eng3_41c72 = 1;
        play_sound(0x603d, g_focus);
        o->state = 2;
    }
    if (o->unknown_dc == 7 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        g_engine_interface.context_004->bar_length_target += 0x20;
        if (g_engine_interface.context_004->bar_length_target >= 0xff)
            g_engine_interface.context_004->bar_length_target = 0xff;
        play_sound(0x603d, g_focus);
        o->state = 2;
    }
    if (o->unknown_dc == 8 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        g_eng3_41cb0 = 0;
        g_eng3_41d50 = 1;
        g_engine_interface.context_004->unknown_1c |= 1;
        play_sound(0x603c, g_focus);
        o->state = 2;
    }
    if (o->unknown_dc == 9 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        g_eng3_41cb0 = 1;
        g_eng3_41d50 = 1;
        g_engine_interface.context_004->unknown_1c |= 2;
        play_sound(0x603c, g_focus);
        o->state = 2;
    }
    if (o->unknown_dc == 10 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        g_eng3_41cb0 = 2;
        g_eng3_41d50 = 1;
        g_engine_interface.context_004->unknown_1c |= 4;
        play_sound(0x603c, g_focus);
        o->state = 2;
    }
    if (o->unknown_dc == 11 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        g_eng3_41cb0 = 3;
        play_sound(0x603c, g_focus);
        g_eng3_41d50 = 1;
        g_engine_interface.context_004->unknown_1c |= 8;
        o->state = 2;
    }
    if (o->unknown_dc == 12 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        g_eng3_41cb0 = 4;
        play_sound(0x603c, g_focus);
        g_eng3_41d50 = 1;
        g_engine_interface.context_004->unknown_1c |= 0x10;
        o->state = 2;
    }
    if (o->unknown_dc == 13 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        g_eng3_41cb0 = 5;
        play_sound(0x603c, g_focus);
        g_eng3_41d50 = 1;
        g_engine_interface.context_004->unknown_1c |= 0x20;
        o->state = 2;
    }
    if (o->unknown_dc == 14 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        g_eng3_41cb0 = 6;
        g_eng3_41d50 = 1;
        g_engine_interface.context_004->unknown_1c |= 0x40;
        play_sound(0x603c, g_focus);
        o->state = 2;
    }
    if (o->unknown_dc == 15 && (o->state & 8) && eng1_2a660(o, g_focus)) {
        g_eng3_41cb0 = 7;
        g_eng3_41d50 = 1;
        g_engine_interface.context_004->unknown_1c |= 0x80;
        play_sound(0x603c, g_focus);
        o->state = 2;
    }
    if (o->state & 4) {
        o->render_flags = 0x84000005;
        if (g_engine_interface.context_004->level == 10)
            o->unknown_4a = 0x80;
        if (g_engine_interface.context_004->level == 2)
            o->unknown_4a = 0x20;
        if (g_engine_interface.context_004->level == 8)
            o->unknown_4a = 0x40;
        if (o->unknown_dc != 3 && o->unknown_dc != 0 && o->unknown_dc != 1)
            eng3_1cd80(o);
        if (eng3_1dbc0(o)) {
            /* The original keeps both dc tests and sends this arm to the trailing removal call;
             * that only compiles when the two arms call distinct symbols (both resolve to 0x1cb90,
             * e.g. identical functions folded by LINK /OPT:ICF). Unresolved; see result.json. */
            if (o->unknown_dc == 3 || o->unknown_dc == 0) {
                remove_actor(o);
                return;
            }
            remove_actor(o);
            return;
        }
    }
    if (o->state & 2) {
        o->y -= 0x80000;
        if (o->y < s->y) {
            o->scale = 0x100;
            o->category = 1;
            o->unknown_1c = 0xb;
            select_frame(o, 7);
            o->state = 4;
        }
        eng3_1dbc0(o);
    }
    if (o->state & 8)
        eng3_1dbc0(o);
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_15e90(Actor *o)
{
    o->unknown_dc = 0;
    o->flags = 2;
    o->render_flags = 0x80000000;
    o->state = 8;
    attach_actor(o, 0x2017);
    o->unknown_b8 = o->unknown_bc = (eng3_01010(4) + 0x10) << 16;
    o->unknown_b0 = 0;
    o->unknown_c8 = o->unknown_cc = 0;
    o->unknown_d0 = 0;
    o->unknown_d4 = 0;
    o->unknown_c4 = 0x10000;
    o->unknown_a8 = o->unknown_ac = 0x1e0000;
    o->unknown_b4 = 0x80;
    o->unknown_95 = 0xff;
}

void eng3_15f30(Actor *o)
{
    if (o->state & 8) {
        if (eng3_01460(o) && eng1_2a660(o, g_focus)) {
            if (o->image == g_eng3_image_375f0) {
                o->state = 2;
                play_sound(0x603c, g_focus);
                g_eng3_3faec++;
                if (g_engine_interface.context_004->level == 2) {
                    if (g_focus->profile == 0)
                        g_engine_interface.context_004->flags_11 |= 1;
                    if (g_focus->profile == 1 || g_focus->profile == 2)
                        g_engine_interface.context_004->flags_11 |= 2;
                    if (g_focus->profile == 3 || g_focus->profile == 4)
                        g_engine_interface.context_004->flags_11 |= 4;
                    if (g_focus->profile == 5 || g_focus->profile == 6)
                        g_engine_interface.context_004->flags_11 |= 4;
                    if (g_focus->profile == 7 || g_focus->profile == 8)
                        g_engine_interface.context_004->flags_11 |= 8;
                    if (g_focus->profile == 9 || g_focus->profile == 10)
                        g_engine_interface.context_004->flags_11 |= 8;
                }
                if (g_engine_interface.context_004->level == 10 && g_focus->profile == 0)
                    g_engine_interface.context_004->flags_11 |= 2;
                if (g_engine_interface.context_004->level == 8) {
                    if (g_eng3_3faec == 1)
                        g_engine_interface.context_004->flags_11 |= 1;
                    if (g_eng3_3faec == 2)
                        g_engine_interface.context_004->flags_11 |= 2;
                    if (g_eng3_3faec == 3)
                        g_engine_interface.context_004->flags_11 |= 4;
                    if (g_eng3_3faec == 4)
                        g_engine_interface.context_004->flags_11 |= 8;
                }
            }
            if (o->image == g_eng3_image_37618) {
                g_eng3_3fad8 = 1;
                g_eng3_41c72 = 1;
                g_engine_interface.context_004->unknown_0c++;
                play_sound(0x603d, g_focus);
                o->state = 2;
            }
            if (o->image == g_eng3_image_37678 && (o->state & 8) && eng1_2a660(o, g_focus)) {
                g_engine_interface.context_004->bar_value_target += 0x20;
                if (g_engine_interface.context_004->bar_value_target > g_engine_interface.context_004->bar_length_target)
                    g_engine_interface.context_004->bar_value_target = g_engine_interface.context_004->bar_length_target;
                play_sound(0x603b, g_focus);
                o->state = 2;
            }
        }
        eng3_1dbc0(o);
    }
    if (o->state & 4) {
        o->render_flags = 0x84000005;
        if (g_engine_interface.context_004->level == 10)
            o->unknown_4a = 0x80;
        if (g_engine_interface.context_004->level == 2)
            o->unknown_4a = 0x20;
        if (g_engine_interface.context_004->level == 8)
            o->unknown_4a = 0x40;
        if (eng3_1dbc0(o)) {
            eng3_1cd40(o);
            return;
        }
    }
    if (o->state & 2) {
        o->y -= 0x80000;
        o->scale = 0x100;
        o->category = 1;
        select_frame(o, 7);
        o->state = 4;
        eng3_1dbc0(o);
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}
}
