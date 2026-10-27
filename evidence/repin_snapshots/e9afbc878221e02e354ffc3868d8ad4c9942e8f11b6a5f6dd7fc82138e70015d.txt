/* ENG3.DLL 0x26b40..0x27370 and 0x28390..0x28ae0: actor handler family (lane y2 candidate).
 * Actor offsets are provisional shared names, not recovered original types. */
typedef struct Actor Actor;
struct Actor {
    long x, y, z;
    unsigned char unknown_0c[0x1c - 0x0c];
    unsigned char unknown_1c;
    unsigned char unknown_1d[0x22 - 0x1d];
    unsigned char category;
    unsigned char unknown_23[0x2a - 0x23];
    short state;
    unsigned char unknown_2c[0x30 - 0x2c];
    short ticks, phase;
    unsigned short sprite;
    unsigned char unknown_36[0x38 - 0x36];
    unsigned short profile;
    short scale;
    unsigned char unknown_3c[0x4a - 0x3c];
    unsigned short unknown_4a;
    long parameter;
    unsigned long flags, render_flags;
    unsigned char *owner_context;
    unsigned char unknown_5c[0x64 - 0x5c];
    unsigned char *script;
    unsigned char unknown_68[0x70 - 0x68];
    short speed;
    unsigned char unknown_72[0x74 - 0x72];
    short unknown_74;
    unsigned char unknown_76[0x88 - 0x76];
    unsigned char unknown_88[0x94 - 0x88];
    unsigned char unknown_94, unknown_95, unknown_96;
    unsigned char unknown_97;
    short unknown_98, unknown_9a;
    unsigned char unknown_9c[0xa8 - 0x9c];
    long unknown_a8, unknown_ac, unknown_b0, unknown_b4;
    long unknown_b8, unknown_bc, unknown_c0, unknown_c4;
    long unknown_c8, unknown_cc, unknown_d0, unknown_d4;
    long unknown_d8, unknown_dc, unknown_e0;
    unsigned char unknown_e4[0x110 - 0xe4];
    Actor *unknown_110;
    unsigned char unknown_114[0x116 - 0x114];
    short unknown_116;
    Actor *unknown_118;
    unsigned char unknown_11c[0x11e - 0x11c];
    short unknown_11e;
    unsigned char unknown_120[0x126 - 0x120];
    short unknown_126;
};

typedef struct KindInfo KindInfo;
struct KindInfo {
    unsigned long sidecar_offset;
    unsigned long unknown;
};

typedef struct Eng3Game Eng3Game;
struct Eng3Game {
    unsigned char unknown_000[0x2c4];
    void (*method_2c4)(Actor *);
    unsigned char unknown_2c8[0x2e8 - 0x2c8];
    void (*method_2e8)(Actor *);
    unsigned char unknown_2ec[0x330 - 0x2ec];
    void (*method_330)(Actor *);
};

extern KindInfo g_kind_info[];
extern Actor *g_focus;
extern Eng3Game *g_eng3_game_40b8c;
extern unsigned char g_engine_interface[];
#define g_eng3_table_407b4 (*(short **)((char *)&g_engine_interface + 0x14))
extern Actor *g_eng3_actor_42540;
extern Actor *g_eng3_actor_431d4;
extern short g_eng3_mode_41cb4;
extern unsigned char g_eng3_script_39f98[];
extern unsigned char g_eng3_script_39fc0[];
extern unsigned int eng3_01010(unsigned int);
extern unsigned int random_limit(int);
extern void vec_diff_fixed_1420(const int *, const int *, short *);
extern void play_sound(unsigned short, Actor *);
extern Actor *attach_actor(Actor *, int);
extern Actor *eng3_1cb00(long, long, long, int);
extern void advance_frame(Actor *);
extern void select_frame(Actor *, int);
extern void remove_actor(Actor *);
extern void eng3_1eeb0(Actor *);
extern int eng3_1dbc0(Actor *);
extern int eng3_01460(Actor *);
extern int eng3_1b920(Actor *, int);
extern int eng3_1eaf0(Actor *);
extern int eng3_1eff0(Actor *, Actor *);
extern int eng3_1f0a0(void *, Actor *);
extern void eng3_19200(void);
extern void eng3_19a30(void);

void eng3_26b40(Actor *o)
{
    if (eng3_1dbc0(o))
        remove_actor(o);
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_26b80(Actor *o)
{
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->unknown_4a = 0xfe;
    o->profile = g_focus->profile;
    attach_actor(o, 0x2017);
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    o->state = 8;
    o->ticks = 1;
    o->unknown_116 = 0x1e;
    eng3_1eeb0(o);
    advance_frame(o);
    select_frame(o, 0);
    o->unknown_126 = 1;
    o->unknown_11e = eng3_01010(3);
    if (o->unknown_11e == 0)
        o->unknown_1c = 0x14;
    if (o->unknown_11e == 1)
        o->unknown_1c = 6;
    if (o->unknown_11e == 2)
        o->unknown_1c = 0x13;
    if (o->unknown_11e == 3)
        o->unknown_1c = 0x15;
    if (o->unknown_dc)
        o->render_flags |= 0x10;
}

void eng3_26c60(Actor *o)
{
    short d[6];

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->state & 8) {
        o->x -= 0x60000;
        if (d[0] < 0x140)
            o->state = 2;
    }
    if (o->state & 2) {
        select_frame(o, 1);
        o->phase = 0;
        o->state = 4;
    }
    if (o->state & 4 && o->phase == 5) {
        select_frame(o, 2);
        o->phase = 0;
        o->state = 0x80;
    }
    eng3_1dbc0(o);
    if (eng3_1eff0(o, g_focus)) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 4;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    eng3_1eeb0(o);
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_26d50(Actor *o)
{
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->profile = g_focus->profile;
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    o->state = 8;
    o->ticks = 1;
    o->unknown_116 = 0x1e;
    eng3_1eeb0(o);
    advance_frame(o);
    select_frame(o, 0);
    if (o->unknown_dc)
        o->render_flags |= 0x10;
}

void eng3_26de0(Actor *o)
{
    short d[6];

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->state & 8) {
        o->x -= 0x20000;
        if (d[0] < 0x140)
            o->state = 2;
    }
    if (o->state & 2)
        o->state = 4;
    if (o->state & 4 && o->phase == 5)
        o->state = 0x80;
    if (eng3_1eff0(o, g_focus)) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 4;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    eng3_1dbc0(o);
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_26eb0(Actor *o)
{
    Actor *child;

    child = eng3_1cb00(o->x, o->y, o->z, 0x44);
    if (child) {
        g_eng3_game_40b8c->method_330(child);
        if (o->unknown_dc)
            child->render_flags |= 0x10;
    }
}

void eng3_26f00(Actor *o)
{
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->profile = g_focus->profile;
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    o->state = 8;
    o->ticks = 1;
    o->unknown_116 = 0x1e;
    eng3_1eeb0(o);
    advance_frame(o);
    select_frame(o, 4);
    o->unknown_98 = 1;
    o->unknown_e0 = 1;
    if (o->unknown_dc)
        o->render_flags |= 0x10;
    o->unknown_11e = eng3_01010(3);
    if (o->unknown_11e == 0)
        o->unknown_1c = 0x14;
    if (o->unknown_11e == 1)
        o->unknown_1c = 6;
    if (o->unknown_11e == 2)
        o->unknown_1c = 0x13;
    if (o->unknown_11e == 3)
        o->unknown_1c = 0x15;
    o->y = 0xfe800000;
}

void eng3_26fe0(Actor *o)
{
    short d[6];

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->state & 8 && d[0] < 0x3c4)
        o->state = 4;
    if (o->state & 4) {
        if (o->unknown_e0) {
            play_sound(0x62a1, o);
            o->unknown_e0 = 0;
        }
        o->y += 0x120000;
        if (eng3_1eaf0(o)) {
            play_sound(0x629b, o);
            o->state = 2;
        }
    }
    if (o->state & 2) {
        select_frame(o, 5);
        o->phase = 0;
        o->state = 0x80;
    }
    if (o->state & 0x80 && o->phase == 0x10) {
        select_frame(o, 2);
        o->phase = 0;
        o->state = 0x20;
    }
    if (eng3_1eff0(o, g_focus)) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 0x2f;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
        if (o->unknown_98) {
            play_sound(0x6282, o);
            o->unknown_98 = 0;
        }
    }
    eng3_1dbc0(o);
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_27180(Actor *o)
{
    short d[6];

    o->sprite = 1;
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    o->unknown_74 = (g_eng3_table_407b4[(o->unknown_126 - 0x400) & 0xfff] << 4) >> 12;
    o->unknown_126 += 0x20;
    if (g_focus->y >= 0x400000) {
        if (d[0] < 0x94 && d[0] > -0x94) {
            if (o->ticks == 0) {
                o->unknown_dc = 0x500000;
                if (o->y >= 0x6c0000)
                    o->ticks = 1;
            } else {
                o->unknown_dc = 0x400000;
            }
            o->speed = -(d[0] >> 2);
            if (d[2] < 0x54 && d[2] > -0x54)
                o->unknown_74 += d[2] >> 2;
        }
    } else {
        if (o->unknown_74 < 0)
            o->unknown_74 += 2;
        if (o->unknown_74 > 0)
            o->unknown_74 -= 2;
        if (o->speed < 0)
            o->speed += 2;
        if (o->speed > 0)
            o->speed -= 2;
    }
    if (o->y < o->unknown_dc)
        o->y += o->unknown_116 << 18;
    if (o->y > o->unknown_dc)
        o->y -= (o->unknown_116 >> 1) << 16;
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_272e0(Actor *o)
{
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->profile = g_focus->profile;
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    o->state = 8;
    o->ticks = 1;
    o->unknown_116 = 0;
    o->scale = 0x200;
    eng3_1eeb0(o);
    o->y -= 0x400000;
    o->sprite = 0xaf;
    if (o->unknown_dc)
        o->render_flags |= 0x10;
}

void eng3_28390(Actor *o)
{
    if (o->script == g_eng3_script_39f98 && o->unknown_9a) {
        o->script = g_eng3_script_39fc0;
        o->phase = 0;
    }
    if (o->script == g_eng3_script_39fc0 && eng3_1dbc0(o)) {
        remove_actor(o);
        return;
    }
    if (o->unknown_9a) {
        if (o->unknown_98) {
            o->unknown_dc = random_limit(5);
            if (o->unknown_dc == 0)
                play_sound(0x629c, o);
            if (o->unknown_dc == 1)
                play_sound(0x629d, o);
            if (o->unknown_dc == 2)
                play_sound(0x629e, o);
            if (o->unknown_dc == 3)
                play_sound(0x629f, o);
            if (o->unknown_dc == 4)
                play_sound(0x62a0, o);
        }
        o->unknown_98 = 0;
    }
    eng3_01460(o);
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_284a0(Actor *o)
{
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->profile = g_focus->profile;
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    o->unknown_11e = 0;
}

void eng3_284f0(Actor *o)
{
    int i;
    Actor *child;

    o->sprite = 0;
    g_eng3_actor_431d4 = o;
    g_eng3_actor_42540->unknown_ac = 0;
    g_eng3_actor_42540->unknown_a8 = 0;
    if (g_focus->y < 0xaa0000)
        return;
    g_focus->x = (o->x >> 23) * 0x800000;
    g_focus->unknown_126 = 1;
    if (g_eng3_mode_41cb4 == 6)
        return;
    for (i = 0; i < 0x18; i++) {
        child = eng3_1cb00(g_focus->x + ((eng3_01010(4) - eng3_01010(4)) << 16),
                           g_focus->y - 0x100000,
                           g_focus->z + ((eng3_01010(0x10) - eng3_01010(0x10)) << 16), 0x3b);
        if (!child)
            continue;
        g_eng3_game_40b8c->method_2c4(child);
        child->unknown_b8 = child->unknown_bc = eng3_01010(0x400) + 0x100000;
        child->unknown_ac = child->unknown_bc >> 2;
        child->category = 4;
        child->unknown_c4 = 0x18000;
        child->script = g_eng3_script_39f98;
        child->phase = 0;
        child->unknown_b4 *= 2;
        child->unknown_cc = child->unknown_ac >> 2;
        child->scale = random_limit(0x40) + 0x80;
        child->unknown_4a = random_limit(0x40) + 0x80;
        child->unknown_95 = 0xff;
        child->unknown_1c = 0x16;
        child->unknown_dc = 1;
        if (i == 0 || i == 4 || i == 8 || i == 0xc || i == 0x10 || i == 0x14 || i == 0x18 || i == 0x1c) {
            child->sprite = 0x152;
            child->unknown_94 = 0;
            child->unknown_96 = 0;
            if (o->unknown_dc)
                child->unknown_96 = 0xff;
        }
        if (i == 1 || i == 5 || i == 9 || i == 0xd || i == 0x11 || i == 0x15 || i == 0x19 || i == 0x1d) {
            child->unknown_94 = 0xff;
            child->sprite = 0x152;
            child->unknown_96 = 0;
            if (o->unknown_dc)
                child->unknown_96 = 0xff;
        }
        if (i == 2 || i == 6 || i == 0xa || i == 0xe || i == 0x12 || i == 0x16 || i == 0x1a || i == 0x1e) {
            child->unknown_94 = 0;
            child->unknown_96 = 0xff;
            child->sprite = 0x152;
            if (o->unknown_dc)
                child->unknown_96 = 0xff;
        }
        if (i == 3 || i == 7 || i == 0xb || i == 0xf || i == 0x13 || i == 0x17 || i == 0x1b || i == 0x1f) {
            child->unknown_94 = 0xff;
            child->unknown_96 = 0xff;
            child->sprite = 0x152;
            if (o->unknown_dc)
                child->unknown_96 = 0xff;
        }
    }
    eng3_19a30();
}

void eng3_287b0(Actor *o)
{
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->profile = g_focus->profile;
    o->flags = 0x202;
    o->scale = 0x100;
    o->unknown_4a = 0x80;
    o->render_flags = 0x80000600;
    o->state = 8;
    o->ticks = 1;
    o->unknown_116 = 0x1e;
    eng3_1eeb0(o);
    select_frame(o, 1);
    if (o->unknown_dc)
        o->render_flags |= 0x10;
}

void eng3_28840(Actor *o)
{
    Actor *child;

    eng3_1dbc0(o);
    o->unknown_4a = 0x80;
    if (o->phase == 9) {
        if (!o->unknown_dc) {
            child = eng3_1cb00(o->x, o->y - 0x200000, o->z - 0x400000, 0x3e);
            if (child) {
                child->profile = o->profile;
                g_eng3_game_40b8c->method_2e8(child);
            }
        } else {
            child = eng3_1cb00(o->x, o->y - 0x200000, o->z + 0x400000, 0x3e);
            if (child) {
                g_eng3_game_40b8c->method_2e8(child);
                child->profile = o->profile;
                child->render_flags &= ~0x10;
                child->unknown_74 += 0x200;
            }
        }
        o->unknown_118 = child;
    }
    if (eng3_1f0a0(g_focus->unknown_88, o)) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 2;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_28960(Actor *o)
{
    if (o->state == 0)
        o->state = 2;
    o->flags = 0x202;
    o->render_flags = 0x80000005;
    o->unknown_74 = 0xff00;
    select_frame(o, 2);
}

void eng3_289a0(Actor *o)
{
    short d[6];

    eng3_1dbc0(o);
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (d[2] > -0x40 && d[2] < 0x40 && d[0] > -0x30 && d[0] < 0x30 && d[1] - 0x1e != 0 && d[1] < 0x1e) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 0x2e;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    if (o->phase == 7)
        remove_actor(o);
}

void eng3_28a40(Actor *o)
{
    if (o->state == 0)
        o->state = 2;
    o->flags = 0x202;
    o->render_flags = 0x80000005;
    o->unknown_4a = 0xfe;
    select_frame(o, 3);
}

void eng3_28a80(Actor *o)
{
    eng3_1dbc0(o);
}

void eng3_28a90(Actor *o)
{
    if (o->state == 0)
        o->state = 8;
    o->flags = 0x202;
    o->render_flags = 0x90000640;
    o->sprite = 6;
    o->unknown_4a = 0x80;
    eng3_1eeb0(o);
    o->y += 0xc0000;
}
