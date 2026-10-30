/* ENG3.DLL 0x12190..0x14190: actor handler family (lane y10 candidate), plain C.
 * Actor offsets are provisional shared names, not recovered original types (view follows
 * handlers_10370.c: Actor.state unsigned short).
 * +0x118/+0x120 hold attached actors in most handlers here; +0x120 is a pending-action
 * group in 0x12500/0x12820 (the same field handlers_10370.c types as PendingActionGroup *).
 * 0x12190, 0x121b0, 0x121f0 and 0x13840 are already matched in src/eng3/actor_handlers.c and
 * are repeated here only as unit context.
 * 0x12820 (frame slot order) and 0x130e0 (commutative add destination) differ standalone and
 * become equal when this text follows handlers_10370.c in one C unit; no count-only context
 * is included (owner ruling 2026-10-20). */
typedef struct Actor Actor;
typedef union PendingActionGroup PendingActionGroup;

union PendingActionGroup {
    long count;
    Actor *entries[1];
};

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
    unsigned char unknown_23[0x2a - 0x23];
    unsigned short state;
    unsigned char unknown_2c[0x30 - 0x2c];
    short ticks, phase;
    unsigned short sprite;
    unsigned char unknown_36[0x38 - 0x36];
    unsigned short profile;
    unsigned short scale;
    unsigned char unknown_3c[0x4a - 0x3c];
    unsigned short unknown_4a;
    long parameter;
    unsigned long flags, render_flags;
    unsigned char *owner_context;
    unsigned char unknown_5c[0x64 - 0x5c];
    unsigned char *script;
    unsigned char unknown_68[0x70 - 0x68];
    short speed, step;
    short unknown_74;
    unsigned char unknown_76[0x94 - 0x76];
    unsigned char unknown_94, unknown_95, unknown_96, unknown_97;
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
    Actor *unknown_120;
    unsigned char unknown_124[0x126 - 0x124];
    short unknown_126;
};

typedef struct KindInfo KindInfo;
struct KindInfo {
    unsigned long sidecar_offset;
    unsigned long unknown;
};

typedef struct KindAction KindAction;
struct KindAction {
    void (*init)(Actor *);
    unsigned long unknown;
};

typedef struct Eng3Game Eng3Game;
struct Eng3Game {
    unsigned char unknown_000[0x2a0];
    void (*method_2a0)(Actor *);
    unsigned char unknown_2a4[0x30c - 0x2a4];
    void (*method_30c)(Actor *);
    unsigned char unknown_310[0x33c - 0x310];
    void (*method_33c)(Actor *);
    unsigned char unknown_340[0x378 - 0x340];
    void (*method_378)(Actor *);
};

typedef struct Eng3Host Eng3Host;
struct Eng3Host {
    unsigned char unknown_00[0xb0];
    void (*method_b0)(Actor *);
};

extern KindInfo g_kind_info[];
extern KindAction *g_eng3_middle_actions_40b98;
extern Actor *g_focus;
extern Eng3Game *g_eng3_game_40b8c;
extern Eng3Host *g_eng3_host_40bb4;
extern unsigned char g_engine_interface[];
#define g_eng3_table_407b4 (*(short **)((char *)&g_engine_interface + 0x14))
extern short g_eng3_mode_41cb4;
extern unsigned char g_eng3_script_355b0[];
extern unsigned char g_eng3_script_355d0[];
extern unsigned int eng3_01010(unsigned int);
extern void vec_diff_fixed_1420(const int *, const int *, short *);
extern void eng3_08120(int, int, int);
extern void play_sound(unsigned short, Actor *);
extern void eng3_15340(Actor *);
extern void eng3_16550(Actor *);
extern void eng3_19200(void);
extern int eng3_1b920(Actor *, int);
extern Actor *eng3_1cb00(long, long, long, int);
extern void remove_actor(Actor *);
extern Actor *create_eng3_middle_actor(long, long, long, unsigned long);
extern Actor *attach_actor(Actor *, int);
extern Actor *create_eng3_kind_actor(long, long, long, int);
extern int eng3_1dbc0(Actor *);
extern void select_frame(Actor *, int);
extern void eng3_1eeb0(Actor *);
extern int eng3_1f1a0(Actor *, Bounds3D *, unsigned short);
extern PendingActionGroup *acquire_pending_action_group(void);
extern Actor *eng3_1f410(PendingActionGroup *, long, long, long, int, int);
extern void release_pending_action(PendingActionGroup *);
extern unsigned int random_limit(int);
extern int eng3_24870(Actor *, Bounds3D *, int);
int abs(int);

void init_flags_12190(Actor *o)
{
    o->flags = 0x202;
    o->render_flags = 0x80000000u;
}

void set_field34_121b0(Actor *o)
{
    o->sprite = 0x119;
}

void eng3_121c0(Actor *o)
{
    o->flags = 0x40202;
    o->render_flags |= 0x10000040;
    o->state = 8;
    o->unknown_116 = 0;
}

void set_field34_121f0(Actor *o)
{
    o->sprite = 0x12;
}

void eng3_12200(Actor *o)
{
    o->flags = 0x202;
    o->state = 8;
    o->ticks = 1;
    o->unknown_126 = 0;
    o->unknown_116 = 0;
    o->render_flags = 0x80080005;
    o->unknown_4a = 0xfe;
    o->unknown_98 = 1;
    o->unknown_e0 = 1;
    select_frame(o, 2);
}

void eng3_12250(Actor *o)
{
    long *home;
    short d[6];
    Bounds3D b;

    if (o->unknown_98) {
        play_sound(0x289a, o);
        o->unknown_98 = 0;
    }
    home = (long *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
    if (o->state & 8) {
        o->unknown_118 = eng3_1cb00(o->x, o->y, o->z, 0x38);
        if (o->unknown_118) {
            g_eng3_game_40b8c->method_2a0(o->unknown_118);
            o->unknown_118->profile = o->profile;
        }
        o->unknown_120 = eng3_1cb00(o->x, o->y - 0x12e0000, o->z, 0x41);
        if (o->unknown_120) {
            g_eng3_game_40b8c->method_30c(o->unknown_120);
            o->unknown_120->profile = o->profile;
        }
        o->state = 2;
    }
    if (o->state & 2)
        o->y -= 0x20000;
    if (o->state & 4)
        o->y += 0x20000;
    if (o->y < home[1] - 0x200000)
        o->state = 4;
    if (o->y > home[1] + 0x200000)
        o->state = 2;
    if (o->unknown_118) {
        o->unknown_118->y = o->y - 0x1360000;
        o->unknown_120->y = o->y - 0x14a0000;
    }
    o->unknown_116 = ((g_eng3_table_407b4[(o->unknown_126 - 0x400) & 0xfff] * 0x30) >> 12) + 0xc4;
    o->unknown_126 += 0x20;
    o->unknown_4a = o->unknown_116;
    eng3_1dbc0(o);
    eng3_1f1a0(o, &b, 2);
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->profile == g_focus->profile && d[2] > -0x38 && d[2] < 0x38 && d[0] > -0x10 && d[0] < 0x10
        && d[1] > -0x46 && d[1] < 0x5f) {
        if (d[2] > -0x28 && d[2] < 0x28 && d[1] > 7 && d[1] < 0x4c) {
            if (o->unknown_e0) {
                o->unknown_118->state = 2;
                g_focus->unknown_d8++;
                o->unknown_e0 = 0;
            }
        } else {
            g_focus->unknown_110 = o;
            g_focus->unknown_116 = 0x40;
            if (g_eng3_mode_41cb4 != 5)
                eng3_19200();
        }
    }
    if (eng3_1b920(o, 0x100)) {
        remove_actor(o->unknown_118);
        remove_actor(o->unknown_120);
        remove_actor(o);
    }
}

void eng3_12500(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;

    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->unknown_116 = 0x800;
    o->unknown_11e = 0;
    o->unknown_126 = 0x800;
    o->unknown_98 = 0;
    o->unknown_e0 = 0;
    o->ticks = 0;
    o->flags = 2;
    o->state = 8;
    o->unknown_bc = o->unknown_cc = o->unknown_ac = 1;
    group = acquire_pending_action_group();
    if (group == 0)
        return;
    o->unknown_120 = (Actor *)group;
    c = eng3_1f410(group, o->x, o->y, o->z, 0x402a, o->profile);
    if (c) {
        c->render_flags |= 0x10000040;
        c->sprite = 0xc;
        c->unknown_94 = 0;
        c->unknown_96 = 0;
        c->unknown_ac = c->unknown_bc = 0xc00000;
        c->unknown_cc = 0;
        c->unknown_b8 = 0x100;
        c->unknown_c0 = 0x30000;
        c->unknown_c4 = 0;
        c->unknown_b0 = 0;
        c->unknown_b4 = 0;
        c->unknown_e0 = 1;
        c->ticks = 4;
        c->unknown_95 = 0;
    }
    c = eng3_1f410(group, o->x, o->y + 0x780000, o->z, 0x402a, o->profile);
    if (c) {
        c->render_flags |= 0x10000040;
        c->sprite = 0xc;
        c->unknown_94 = 0;
        c->unknown_96 = 0;
        c->unknown_bc = c->unknown_ac = 0xf00000;
        c->unknown_cc = 0;
        c->unknown_b8 = 0x100;
        c->unknown_c0 = 0x30000;
        c->unknown_c4 = 0;
        c->unknown_b0 = 0;
        c->unknown_b4 = 0;
        c->unknown_e0 = 0;
        c->ticks = 8;
        c->unknown_95 = 0;
    }
    c = eng3_1f410(group, o->x, o->y + 0xf40000, o->z, 0x402a, o->profile);
    if (c) {
        c->render_flags |= 0x10000040;
        c->sprite = 0xc;
        c->unknown_94 = 0;
        c->unknown_96 = 0;
        c->unknown_ac = c->unknown_bc = 0xc00000;
        c->unknown_cc = 0;
        c->unknown_b8 = 0x100;
        c->unknown_c0 = 0x30000;
        c->unknown_c4 = 0;
        c->unknown_b0 = 0;
        c->unknown_b4 = 0;
        c->unknown_e0 = 0;
        c->ticks = 0xc;
        c->unknown_95 = 0;
    }
    c = eng3_1f410(group, o->x, o->y + 0x1b80000, o->z, 0x402a, o->profile);
    if (c) {
        c->render_flags |= 0x10000040;
        c->sprite = 0x15;
        if (o->unknown_dc)
            c->sprite = 0x16;
        c->render_flags &= ~0x20;
        c->scale = 0x190;
        c->unknown_94 = 0;
        c->unknown_96 = 0;
        c->unknown_ac = c->unknown_bc = 0xf00000;
        c->unknown_cc = 0;
        c->unknown_b8 = 0x100;
        c->unknown_c0 = 0x30000;
        c->unknown_c4 = 0;
        c->unknown_b0 = 0;
        c->unknown_b4 = 0;
        c->unknown_e0 = 0;
        c->ticks = 0x10;
        c->unknown_95 = 0;
        attach_actor(c, 0x2002);
    }
}

int eng3_12aa0(Actor *o, Actor *c);

void eng3_12820(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;
    int i;
    short d[6];
    Bounds3D b;

    group = (PendingActionGroup *)o->unknown_120;
    if (group) {
        for (i = 0; i < group->count;) {
            c = group->entries[++i];
            if (o->state & 8) {
                vec_diff_fixed_1420((const int *)c, (const int *)g_focus, d);
                o->unknown_116 = ((g_eng3_table_407b4[(o->unknown_126 - 0x400) & 0xfff] << 9) >> 12) + 0x800;
                o->unknown_126 &= 0xfff;
                if (o->unknown_dc == 0) {
                    if (o->unknown_116 >= 0x800)
                        o->unknown_116 = -o->unknown_116;
                } else {
                    if (o->unknown_126 > 0 && o->unknown_126 < 0x800 && o->unknown_cc) {
                        play_sound(0x60a0, c);
                        o->unknown_cc = 0;
                    }
                    if (o->unknown_126 >= 0x800 && o->unknown_cc == 0) {
                        play_sound(0x60a0, c);
                        o->unknown_cc = 1;
                    }
                    if (o->unknown_126 > 0x400 && o->unknown_126 < 0xc00 && o->unknown_bc) {
                        play_sound(0x609f, c);
                        o->unknown_bc = 0;
                    }
                    if (o->unknown_126 >= 0xc00 && o->unknown_bc == 0) {
                        play_sound(0x609f, c);
                        o->unknown_bc = 1;
                    }
                    if (o->unknown_116 <= 0x800)
                        o->unknown_116 = -o->unknown_116;
                }
                eng3_12aa0(o, c);
                if (o->unknown_98 == 0)
                    o->unknown_126 += 0xc;
            }
            if (c->sprite == 0x15 && o->profile == g_focus->profile) {
                eng3_24870(c, &b, 1);
                if (d[2] > -(b.max_x >> 1) && d[2] < b.max_x >> 1
                    && d[0] > -(b.max_z >> 1) && d[0] < b.max_z >> 1
                    && d[1] > -b.max_y && d[1] < b.max_y) {
                    g_focus->unknown_110 = o;
                    g_focus->unknown_116 = 0x25;
                    if (g_eng3_mode_41cb4 != 5)
                        eng3_19200();
                }
            }
        }
    }
    if (eng3_1b920(o, 0x100)) {
        release_pending_action(group);
        remove_actor(o);
    }
}

int eng3_12aa0(Actor *o, Actor *c)
{
    long x, y, z;
    long offsets[4][4];    /* 16-byte rows, fourth long never written */
    int angle;
    long dz, dy;

    z = o->z;
    y = o->y - 0x800000;
    x = o->x;
    angle = o->unknown_116;
    c->unknown_74 = angle >> 2;
    offsets[0][0] = 0;
    offsets[0][1] = 0;
    offsets[0][2] = 0;
    offsets[1][0] = 0;
    offsets[1][1] = -0x78;
    offsets[1][2] = 0;
    offsets[2][0] = 0;
    offsets[2][1] = -0xf4;
    offsets[2][2] = 0;
    offsets[3][0] = 0;
    offsets[3][1] = -0x198;
    offsets[3][2] = 0;
    dz = (offsets[o->ticks][0] * g_eng3_table_407b4[angle & 0xfff] - offsets[o->ticks][1] * g_eng3_table_407b4[(angle - 0x400) & 0xfff]) >> 12;
    dy = (offsets[o->ticks][0] * g_eng3_table_407b4[(angle - 0x400) & 0xfff] + offsets[o->ticks][1] * g_eng3_table_407b4[angle & 0xfff]) >> 12;
    c->z = (dz << 16) + z;
    c->y = (dy << 16) + y;
    c->x = x;
    o->ticks = (o->ticks + 1) & 3;
    if (o->unknown_e0 == 0 && o->unknown_116 >= 0xc00)
        return 1;
    if (o->unknown_e0 == 1 && o->unknown_116 <= 0x400)
        return 1;
    return 0;
}

void eng3_12be0(Actor *o)
{
    o->flags = 0x202;
    o->render_flags |= 0x10000040;
    o->category = 0;
    o->sprite = 0x18;
    o->unknown_11e = 0x10;
    eng3_1eeb0(o);
    o->y -= 0x1000000;
    o->scale = 0x200;
    o->unknown_116 = 0xc0;
    o->unknown_98 = 1;
    o->state = 8;
}

void eng3_12c40(Actor *o)
{
    long y1, z1, y2, z2;

    o->unknown_74 -= o->unknown_11e;
    z1 = (g_eng3_table_407b4[((o->unknown_74 << 2) - 0x400) & 0xfff] * o->unknown_116) << 4;
    y1 = (g_eng3_table_407b4[(o->unknown_74 << 2) & 0xfff] * o->unknown_116) << 4;
    if (o->unknown_98) {
        play_sound(0x28a2, o);
        o->unknown_98 = 0;
    }
    z2 = -z1;
    y2 = -y1;
    if (y1 >> 16 == o->unknown_116 || y2 >> 16 == o->unknown_116)
        play_sound(0x60a1, o);
    z2 += o->z;
    z1 += o->z;
    y1 = abs(y1 - o->y);
    y2 = abs(y2 - o->y);
    if (o->profile == g_focus->profile) {
        if (g_focus->z < z1 + 0x400000 && g_focus->z > z1 - 0x400000
            && g_focus->y < y1 + 0x6e0000 && g_focus->y > y1 - 0x6e0000
            && g_focus->x < o->x + 0x5e0000 && g_focus->x > o->x - 0x5e0000) {
            g_focus->unknown_116 = 0x2d;
            g_focus->unknown_110 = o;
            if (g_eng3_mode_41cb4 != 5)
                eng3_19200();
            o->state = 2;
        }
        if (g_focus->z < z2 + 0x400000 && g_focus->z > z2 - 0x400000
            && g_focus->y < y2 + 0x6e0000 && g_focus->y > y2 - 0x6e0000
            && g_focus->x < o->x + 0x5e0000 && g_focus->x > o->x - 0x5e0000) {
            g_focus->unknown_116 = 0x2d;
            g_focus->unknown_110 = o;
            if (g_eng3_mode_41cb4 != 5)
                eng3_19200();
            o->state = 2;
        }
        if (o->state & 2)
            o->unknown_11e = 8;
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
    eng3_16550(o);
}

void eng3_12e40(Actor *o)
{
    o->flags = 0x202;
    o->render_flags |= 0x10000040;
    o->sprite = 0x10;
    o->category = 0;
    o->unknown_116 = 0;
    o->unknown_11e = 0;
    o->scale = 0x140;
    o->unknown_98 = 1;
}

void eng3_12e80(Actor *o)
{
    if (o->unknown_11e == 0) {
        o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
        if (o->unknown_dc == 1) {
            o->unknown_74 -= 8;
            o->sprite = 0x14;
            if (o->unknown_98) {
                play_sound(0x289b, o);
                o->unknown_98 = 0;
            }
            if (o->x < g_focus->x - 0x800000)
                remove_actor(o);
        } else if (o->unknown_dc == 2) {
            o->unknown_74 -= 8;
            if (o->x < g_focus->x - 0x800000)
                remove_actor(o);
        } else if (o->unknown_dc == 3) {
            o->unknown_74 -= 8;
            o->scale = 0xc8;
            o->step = 0x300;
            if (o->x < g_focus->x - 0x800000)
                remove_actor(o);
        } else if (o->unknown_dc != 4) {
            o->unknown_74 -= 8;
            o->scale = 0x190;
            o->step = 0x11e;
            if (eng3_1b920(o, 0x100))
                remove_actor(o);
        } else {
            o->scale = 0xc8;
            o->step = 0x11e;
            if (o->unknown_dc == 4)
                o->unknown_74 -= 8;
            if (o->x < g_focus->x - 0x800000)
                remove_actor(o);
        }
    } else if (o->unknown_11e == 2) {
        o->scale = 0x100;
        o->step = 0x300;
        if (o->x < g_focus->x - 0x800000)
            remove_actor(o);
    } else if (o->unknown_11e == 3) {
        o->step = o->scale = 0x100;
        if (eng3_1b920(o, 0x100))
            remove_actor(o);
    } else {
        if (eng3_1b920(o, 0x100))
            remove_actor(o);
    }
}

void eng3_13010(Actor *o)
{
    o->unknown_98 = eng3_01010(3);
    if (o->unknown_98 == 0)
        o->unknown_1c = 0xc;
    if (o->unknown_98 == 1)
        o->unknown_1c = 0xd;
    if (o->unknown_98 == 2)
        o->unknown_1c = 0xe;
    if (o->unknown_98 == 3)
        o->unknown_1c = 0xf;
    o->unknown_4a += eng3_01010(0x20);
    o->flags = 0x202;
    o->state = 2;
    o->ticks = 1;
    o->unknown_126 = 0x14;
    o->unknown_11e = 0;
    o->unknown_116 = eng3_01010(0x78);
    o->unknown_d8 = eng3_01010(0x80) + 0x40;
    o->unknown_e0 = eng3_01010(1) + 1;
    eng3_1eeb0(o);
    eng3_15340(o);
    select_frame(o, 0);
}

void eng3_130e0(Actor *o)
{
    long *home;
    short d[6];

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    eng3_1eeb0(o);
    home = (long *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
    if (o->state & 2) {
        if (--o->unknown_126 < 0) {
            o->unknown_11e = 2;
            o->state = 0x20;
            select_frame(o, 2);
        }
        if (o->z > home[2] + (o->unknown_d8 << 16))
            o->state = 4;
        eng3_1dbc0(o);
        o->z += o->unknown_e0 << 16;
    }
    if (o->state & 0x20 && eng3_1dbc0(o)) {
        o->unknown_126 = eng3_01010(0x78);
        if (o->unknown_11e & 2) {
            select_frame(o, 0);
            o->state = o->unknown_11e;
        }
        if (o->unknown_11e & 8) {
            select_frame(o, 0);
            o->state = o->unknown_11e;
        }
    }
    if (o->state & 4) {
        select_frame(o, 1);
        o->state = 0x10;
    }
    if (o->state & 0x10 && eng3_1dbc0(o)) {
        if (o->z > home[2]) {
            o->state = 8;
            o->render_flags |= 0x10;
        }
        if (o->z < home[2]) {
            o->state = 2;
            o->render_flags &= ~0x10;
        }
        select_frame(o, 0);
    }
    if (o->state & 8) {
        if (--o->unknown_126 < 0) {
            o->unknown_11e = 8;
            o->state = 0x20;
            select_frame(o, 2);
        }
        if (o->z < home[2] - 0x200000)
            o->state = 4;
        eng3_1dbc0(o);
        o->z -= o->unknown_e0 << 16;
    }
    if (o->state & 0x40) {
        eng3_1dbc0(o);
        if (o->script == g_eng3_script_355b0)
            o->x += 0x30000;
    }
    if (d[0] < 0x200 && o->state != 0x40) {
        o->state = 0x40;
        if (eng3_01010(0x100) & 5)
            play_sound(0x6083, o);
        select_frame(o, 3);
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_13330(Actor *o)
{
    o->flags = 0x202;
    attach_actor(o, 0x2017);
    o->state = 8;
    o->ticks = 1;
    o->unknown_11e = 1;
    o->unknown_116 = 0xa;
    o->unknown_ac = o->x;
    o->unknown_98 = 1;
    o->unknown_126 = 8;
    o->unknown_cc = 0;
    select_frame(o, 4);
    eng3_1eeb0(o);
}

void eng3_133b0(Actor *o)
{
    short d[6];
    Actor *created;
    int j;

    if (o->unknown_98) {
        play_sound(0x28a6, o);
        o->unknown_118 = eng3_1cb00(o->x, o->y, o->z - 0x100000, 0x4a);
        if (o->unknown_118) {
            g_eng3_game_40b8c->method_378(o->unknown_118);
            o->unknown_118->profile = o->profile;
            o->unknown_118->render_flags |= 0x10;
        }
        o->unknown_120 = eng3_1cb00(o->x, o->y, o->z + 0x100000, 0x4a);
        if (o->unknown_120) {
            g_eng3_game_40b8c->method_378(o->unknown_120);
            o->unknown_120->profile = o->profile;
        }
        o->unknown_98 = 0;
    }
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->state & 8) {
        if (--o->unknown_116 < 0) {
            o->unknown_d8 = g_focus->x;
            o->unknown_dc = g_focus->y;
            o->unknown_e0 = g_focus->z;
            o->unknown_116 = 5;
        }
        if (o->unknown_cc < o->unknown_e0)
            o->unknown_cc += 0x30000;
        if (o->unknown_cc > o->unknown_e0)
            o->unknown_cc -= 0x30000;
        o->x = o->unknown_ac;
        o->z = o->unknown_cc;
        o->unknown_11e = 0x40;
        o->unknown_ac -= 0x80000;
        if (o->profile == g_focus->profile && d[2] > -0x20 && d[2] < 0x20 && d[0] > -0x20 && d[0] < 0x20) {
            g_focus->unknown_116 = 0x22;
            g_focus->unknown_110 = o;
            if (g_eng3_mode_41cb4 != 5)
                eng3_19200();
            play_sound(0xe0a7, o);
            o->state = 2;
        }
        eng3_1dbc0(o);
    }
    if (o->state & 2) {
        o->state = 4;
        o->unknown_118->unknown_e0 = 1;
        o->unknown_120->unknown_e0 = 1;
        o->unknown_120 = 0;
        o->unknown_118 = 0;
        g_eng3_host_40bb4->method_b0(create_eng3_kind_actor(o->x - 0x400000, o->y + 0x80000, o->z, 0x2016));
        for (j = 0; j < 8; j++) {
            created = create_eng3_middle_actor(o->x, o->y - (random_limit(0x80) << 16),
                                               o->z + ((random_limit(0x20) << 16) - (random_limit(0x20) << 16)),
                                               0x4001);
            g_eng3_middle_actions_40b98[1].init(created);
            created->category = 3;
            created->unknown_b8 = created->unknown_bc = (eng3_01010(4) + 0x10) << 16;
            created->unknown_c4 = 0x28000;
            created->unknown_ac = (eng3_01010(8) + 0x1e) << 16;
            created->unknown_a8 = created->unknown_ac >> 2;
            created->unknown_b0 = 0;
            created->unknown_b4 = 0x100;
            created->unknown_95 = 0xff;
            created->unknown_d0 = eng3_01010(0x20);
            created->unknown_cc = 0x1e0000;
            created->profile = o->profile;
            if (j == 0 || j == 4) {
                if (j == 0)
                    created->sprite = 0x26b;
                if (j == 4)
                    created->sprite = 0x26c;
                created->unknown_94 = 0;
                created->unknown_96 = 0;
            }
            if (j == 1 || j == 5) {
                if (j == 1)
                    created->sprite = 0x26c;
                if (j == 5)
                    created->sprite = 0x26d;
                created->unknown_94 = 0;
                created->unknown_96 = 0;
            }
            if (j == 2 || j == 6) {
                if (j == 2)
                    created->sprite = 0x26e;
                if (j == 6)
                    created->sprite = 0x26f;
                created->unknown_94 = 0;
                created->unknown_96 = 0xff;
            }
            if (j == 3 || j == 7) {
                if (j == 3)
                    created->sprite = 0x270;
                if (j == 7)
                    created->sprite = 0x271;
                created->unknown_94 = 0;
                created->unknown_96 = 0xff;
            }
        }
        remove_actor(o);
        return;
    }
    eng3_1eeb0(o);
    if (o->unknown_120 && o->unknown_118) {
        o->unknown_118->x = o->x;
        o->unknown_118->y = o->y;
        o->unknown_118->z = o->z - 0x100000;
        o->unknown_120->x = o->x;
        o->unknown_120->y = o->y;
        o->unknown_120->z = o->z + 0x100000;
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void init_defaults_13840(Actor *o)
{
    o->flags = 0x202;
    o->state = 8;
}

void eng3_13860(Actor *o)
{
    Actor *c;

    eng3_08120(1, 0, 1);
    if (o->state & 8) {
        c = eng3_1cb00(o->x, o->y, o->z, 0x45);
        if (c) {
            g_eng3_game_40b8c->method_33c(c);
            c->unknown_118 = o;
            c->script = g_eng3_script_355d0;
            c->phase = 0;
        }
        c = eng3_1cb00(o->x, o->y, o->z, 0x45);
        if (c) {
            g_eng3_game_40b8c->method_33c(c);
            c->unknown_118 = o;
            c->z -= 0x800000;
            c->y -= 0x400000;
        }
        c = eng3_1cb00(o->x, o->y, o->z, 0x45);
        if (c) {
            g_eng3_game_40b8c->method_33c(c);
            c->unknown_118 = o;
            c->z -= 0x1000000;
            c->y -= 0x800000;
        }
        c = eng3_1cb00(o->x, o->y, o->z, 0x45);
        if (c) {
            g_eng3_game_40b8c->method_33c(c);
            c->unknown_118 = o;
            c->z -= 0x1800000;
            c->y -= 0xc40000;
        }
        c = eng3_1cb00(o->x, o->y, o->z, 0x45);
        if (c) {
            g_eng3_game_40b8c->method_33c(c);
            c->unknown_118 = o;
            c->z -= 0x2000000;
            c->y -= 0x1000000;
        }
        o->state = 2;
    }
    if (o->state & 4) {
        c = eng3_1cb00(o->x, o->y, o->z, 0x45);
        if (c) {
            g_eng3_game_40b8c->method_33c(c);
            c->unknown_118 = o;
        }
        o->state = 2;
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_13a40(Actor *o)
{
    o->flags = 0x202;
    o->state = 2;
    o->sprite = 1;
    eng3_1eeb0(o);
    o->y += 0x400000;
    o->scale = 0x200;
    o->unknown_11e = eng3_01010(0x40) + 0x40;
}

void eng3_13a90(Actor *o)
{
    o->z -= 0x20000;
    o->y -= 0x10000;
    if (o->script == g_eng3_script_355d0)
        eng3_1dbc0(o);
    o->unknown_116 = (g_eng3_table_407b4[(o->unknown_126 - 0x400) & 0xfff] * 0x4c) >> 12;
    o->unknown_126 += o->unknown_11e;
    o->unknown_74 = o->unknown_116;
    if (o->z < -0x2000000) {
        o->unknown_118->state = 4;
        remove_actor(o);
    }
}

void eng3_13b30(Actor *o)
{
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    if (o->unknown_dc)
        o->render_flags |= 0x10;
    o->unknown_126 = eng3_01010(3);
    if (o->unknown_126 == 0)
        o->unknown_1c = 0xc;
    if (o->unknown_126 == 1)
        o->unknown_1c = 0xd;
    if (o->unknown_126 == 2)
        o->unknown_1c = 0xe;
    if (o->unknown_126 == 3)
        o->unknown_1c = 0xf;
    o->unknown_4a += eng3_01010(0x40);
    o->unknown_98 = eng3_01010(0x28) + 0xc8;
    o->flags = 0x202;
    o->state = 8;
    eng3_1eeb0(o);
    select_frame(o, 2);
    eng3_15340(o);
    o->phase += eng3_01010(5);
    o->unknown_11e = eng3_01010(0x14);
}

void eng3_13c10(Actor *o)
{
    if (o->unknown_126 == 0 && o->unknown_98-- < 0) {
        play_sound(0x6083, o);
        o->unknown_98 = eng3_01010(0x28) + 0xc8;
    }
    if (o->unknown_11e-- < 0 && eng3_1dbc0(o))
        o->unknown_11e = eng3_01010(0x1e);
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_13cb0(Actor *o)
{
    o->flags = 0x202;
    o->render_flags |= 0x10000040;
    o->category = 0;
    o->state = 8;
    o->sprite = 0x19;
    o->scale = 0x180;
    o->ticks = 1;
    eng3_1eeb0(o);
    o->y += 0x200000;
}

void eng3_13d00(Actor *o)
{
    o->flags = 0x202;
    o->state = 8;
    o->unknown_e0 = 0;
    o->render_flags = 0x80000004;
    select_frame(o, 0);
}

void eng3_13d30(Actor *o)
{
    eng3_1dbc0(o);
    if (o->unknown_e0 || eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_13d70(Actor *o)
{
    o->flags = 0x202;
    o->render_flags |= 0x10000040;
    o->category = 0;
    o->state = 8;
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    if (o->unknown_dc == 0)
        o->sprite = 0x1c;
    if (o->unknown_dc == 1)
        o->sprite = 0x1d;
    if (o->unknown_dc == 2)
        o->sprite = 0x1e;
    o->scale = 0x12c;
    o->ticks = 1;
    eng3_1eeb0(o);
    o->step = 0x200;
    o->z = 0;
}

void eng3_13e00(Actor *o)
{
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_13e30(Actor *o)
{
    o->flags = 0x202;
    o->state = 8;
    o->ticks = 1;
    o->unknown_126 = 0;
    o->unknown_11e = 0x34;
    o->unknown_116 = 0;
    o->render_flags = 0x80000005;
    o->unknown_4a = 0xfe;
    o->unknown_98 = 1;
    o->unknown_e0 = 1;
    o->y -= 0x800000;
    select_frame(o, 3);
}

void eng3_13ea0(Actor *o)
{
    long *home = (long *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
    short d[6];
    long z0, y0, zmin, zmax, ymin;

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->unknown_98) {
        o->unknown_118 = eng3_1cb00(o->x, o->y, o->z, 0x38);
        g_eng3_game_40b8c->method_2a0(o->unknown_118);
        o->unknown_118->profile = o->profile;
        o->unknown_120 = eng3_1cb00(o->x, o->y + 0x340000, o->z + 0x80000, 0x41);
        g_eng3_game_40b8c->method_30c(o->unknown_120);
        o->unknown_120->profile = o->profile;
        o->unknown_120->sprite = 0x27c;
        o->unknown_120->scale -= 0x40;
        play_sound(0x289a, o);
        o->unknown_98 = 0;
    }
    o->unknown_116 = (g_eng3_table_407b4[(o->unknown_126 - 0x400) & 0xfff] << 6) >> 12;
    o->unknown_126 += o->unknown_11e;
    o->unknown_74 = o->unknown_116;
    o->unknown_118->y = o->y;
    o->unknown_116 = (g_eng3_table_407b4[(o->unknown_126 - 0x400) & 0xfff] * 0x35) >> 12;
    o->unknown_118->unknown_74 = o->unknown_116;
    o->unknown_120->unknown_74 = o->unknown_116;
    eng3_1dbc0(o);
    z0 = (home[2] >> 16) - ((g_eng3_table_407b4[(o->unknown_126 - 0x400) & 0xfff] << 6) >> 12);
    zmin = z0 - 0x46;
    zmax = z0 + 0x46;
    y0 = o->y >> 16;
    ymin = y0 + 0xba;
    if (o->profile == g_focus->profile
        && g_focus->z > zmin << 16 && g_focus->z < zmax << 16
        && g_focus->y > ymin << 16 && g_focus->y < (y0 + 0x246) << 16
        && d[0] > -0x20 && d[0] < 0x20) {
        if (g_focus->z > (zmin + 0x10) << 16 && g_focus->z < (zmax - 0x10) << 16
            && g_focus->y > (ymin + 0x10) << 16 && g_focus->y < (y0 + 0x136) << 16) {
            if (o->unknown_e0) {
                o->unknown_118->state = 2;
                g_focus->unknown_d8++;
                o->unknown_e0 = 0;
            }
        } else {
            g_focus->unknown_116 = 0x40;
            g_focus->unknown_110 = o;
            if (g_eng3_mode_41cb4 != 5)
                eng3_19200();
        }
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}
