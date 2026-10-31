extern "C" {
/* ENG3.DLL 0xe020..0x10370: level-object handler family (lane y14 candidate), followed in the
 * module by calibration/eng3/handlers_10370.c. Actor offsets are provisional shared names, not
 * recovered original types; declarations are those of handlers_10370.c plus the callees used here.
 * Measured as C++ (this text inside extern "C"): 0xf650 and 0xfab0 (call evaluation order) match
 * only as C++. 0xeeb0 differs by the evaluation order of one random_limit pair, the same
 * declaration-count phase residue as 0x10df0; 0xe700 differs by one store source register.
 * No count-only declarations are included (owner ruling 2026-10-20). */
typedef struct Actor Actor;
typedef union PendingActionGroup PendingActionGroup;

union PendingActionGroup {
    long count;
    Actor *entries[1];
};

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
    unsigned char unknown_76[0x88 - 0x76];
    unsigned char unknown_88[0x94 - 0x88];
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
    PendingActionGroup *unknown_120;
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
    unsigned char unknown_000[0x60];
    void (*method_60)(Actor *);
    unsigned char unknown_064[0x2f4 - 0x64];
    void (*method_2f4)(Actor *);
    unsigned char unknown_2f8[0x378 - 0x2f8];
    void (*method_378)(Actor *);
};

typedef struct Eng3Host Eng3Host;
struct Eng3Host {
    unsigned char unknown_00[0xa8];
    void (*method_a8)(Actor *);
    unsigned char unknown_ac[0xb0 - 0xac];
    void (*method_b0)(Actor *);
};

typedef struct EngineContextView EngineContextView;
struct EngineContextView {
    unsigned char unknown_00[0x11];
    unsigned char flags_11;
    unsigned char unknown_12[0x1e - 0x12];
    short unknown_1e;
    unsigned char unknown_20[0x24 - 0x20];
    short unknown_24;
};

typedef struct EngineInterfaceView EngineInterfaceView;
struct EngineInterfaceView {
    unsigned long signature_000;
    EngineContextView *context_004;
};

extern KindInfo g_kind_info[];
extern KindAction *g_eng3_middle_actions_40b98;
extern Actor *g_focus;
extern Eng3Game *g_eng3_game_40b8c;
extern Eng3Host *g_eng3_host_40bb4;
extern EngineInterfaceView g_engine_interface;
#define g_eng3_table_407b4 (*(short **)((char *)&g_engine_interface + 0x14))
extern short g_eng3_mode_41cb4;
extern int g_eng3_3fb20;
extern unsigned char g_eng3_script_35be8[];
extern unsigned int eng3_01010(unsigned int);
extern int within_radius_1140(const int *, const int *, int);
extern int eng3_01190(int, PendingActionGroup *, int, int);
extern void dispatch_primary(Actor *);
extern void dispatch_secondary(Actor *);
extern void vec_diff_fixed_1420(const int *, const int *, short *);
extern void play_sound(unsigned short, Actor *);
extern void stop_actor_sounds(Actor *);
extern void eng3_15340(Actor *);
extern void eng3_19200(void);
extern unsigned short eng3_1b4a0(short, short, short, int);
extern int eng3_1b920(Actor *, int);
extern int eng3_1dbc0(Actor *);
extern int eng3_1eff0(Actor *, Actor *);
extern Actor *title_08a50(long, long, long, int);
extern void remove_actor(Actor *);
extern Actor *create_eng3_middle_actor(long, long, long, unsigned long);
extern void remove_actor_variant(Actor *);
extern Actor *attach_actor(Actor *, int);
extern Actor *create_eng3_kind_actor(long, long, long, int);
extern void select_frame(Actor *, int);
extern void eng3_1eeb0(Actor *);
extern int eng3_1faf0(int);
extern PendingActionGroup *acquire_pending_action_group(void);
extern Actor *eng3_1f410(PendingActionGroup *, long, long, long, int, int);
extern void remove_counted_value(PendingActionGroup *, Actor *);
extern void release_pending_action(PendingActionGroup *);
extern void advance_planar_motion(Actor *);
extern void advance_vertical_motion(Actor *);
extern unsigned int random_limit(int);
extern int eng3_0dae0(Actor *, Actor *);
extern int eng3_01050(long, long, long, long, long, long);
extern int eng3_01600(Actor *);
extern void eng3_1cd40(Actor *);
extern int eng3_1fac0(void);

void eng3_0e020(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;

    o->unknown_cc = 1;
    o->unknown_ac = 1;
    o->unknown_bc = 1;
    o->state = 0;
    o->unknown_116 = 0x800;
    o->flags = 2;
    o->unknown_11e = 0;
    o->unknown_126 = 0x800;
    o->unknown_98 = 0;
    o->unknown_dc = 4;
    o->unknown_e0 = 1;
    group = acquire_pending_action_group();
    if (group == 0)
        return;
    o->unknown_120 = group;
    c = eng3_1f410(group, o->x, o->y, o->z, 0x402a, o->profile);
    if (c) {
        c->sprite = 3;
        c->render_flags |= 0x10000040;
        c->unknown_94 = 0;
        c->unknown_96 = 0;
        c->unknown_bc = 0xc00000;
        c->unknown_ac = 0xc00000;
        c->unknown_cc = 0;
        c->unknown_b8 = 0x100;
        c->unknown_c0 = 0x30000;
        c->unknown_c4 = 0;
        c->unknown_b0 = 0;
        c->unknown_b4 = 0;
        c->unknown_95 = 0;
    }
    c = eng3_1f410(group, o->x, o->y - 0x400000, o->z, 0x402a, o->profile);
    if (c) {
        c->render_flags |= 0x10000040;
        c->sprite = 3;
        c->unknown_94 = 0;
        c->unknown_96 = 0;
        c->unknown_ac = 0xf00000;
        c->unknown_bc = 0xf00000;
        c->unknown_cc = 0;
        c->unknown_b8 = 0x100;
        c->unknown_c0 = 0x30000;
        c->unknown_c4 = 0;
        c->unknown_b0 = 0;
        c->unknown_b4 = 0;
        c->unknown_95 = 0;
        attach_actor(c, 0x2002);
    }
    c = eng3_1f410(group, o->x, o->y - 0x800000, o->z, 0x402a, o->profile);
    if (c) {
        c->render_flags |= 0x10000040;
        c->sprite = 3;
        c->unknown_94 = 0;
        c->unknown_96 = 0;
        c->unknown_bc = 0xc00000;
        c->unknown_ac = 0xc00000;
        c->unknown_cc = 0;
        c->unknown_b8 = 0x100;
        c->unknown_c0 = 0x30000;
        c->unknown_c4 = 0;
        c->unknown_b0 = 0;
        c->unknown_b4 = 0;
        c->unknown_95 = 0;
        attach_actor(c, 0x2002);
    }
    c = eng3_1f410(group, o->x, o->y - 0xc00000, o->z, 0x402a, o->profile);
    if (c) {
        c->render_flags |= 0x10000040;
        c->sprite = 3;
        c->unknown_94 = 0;
        c->unknown_96 = 0;
        c->unknown_bc = 0xf00000;
        c->unknown_ac = 0xf00000;
        c->unknown_cc = 0;
        c->unknown_b8 = 0x100;
        c->unknown_c0 = 0x30000;
        c->unknown_c4 = 0;
        c->unknown_b0 = 0;
        c->unknown_b4 = 0;
        c->unknown_95 = 0;
        attach_actor(c, 0x2002);
    }
}

void eng3_0e2f0(Actor *o)
{
    PendingActionGroup *group;
    Actor *c, *created;
    short d[6];
    int i, j;
    int landed;

    group = o->unknown_120;
    if (group) {
        for (i = 0; i < group->count;) {
            c = group->entries[++i];
            if (o->state != 0x20 && o->state != 8) {
                vec_diff_fixed_1420((const int *)c, (const int *)g_focus, d);
                if (d[0] < 0x280) {
                    o->state = 4;
                } else {
                    if (i == 4) {
                        if ((o->unknown_116 & 0xfff) > 0x800 && o->unknown_cc) {
                            o->unknown_cc = 0;
                            play_sound(0x6096, c);
                        }
                        if ((o->unknown_116 & 0xfff) < 0x800 && !o->unknown_cc) {
                            o->unknown_cc = 1;
                            play_sound(0x6097, c);
                        }
                    }
                    o->unknown_116 = ((g_eng3_table_407b4[(o->unknown_126 - 0x400) & 0xfff] * 0x30) >> 12) + 0x800;
                    eng3_0dae0(o, c);
                    o->unknown_126 += 0x20;
                }
                if (o->state & 4) {
                    if (i == 4 && o->unknown_ac) {
                        play_sound(0x6098, c);
                        o->unknown_ac = 0;
                    }
                    o->unknown_116 -= (short)(o->unknown_dc >> 16);
                    if (o->unknown_dc < 0x140000)
                        o->unknown_dc += 0x4000;
                    if (eng3_0dae0(o, c))
                        o->state = 0x20;
                }
            } else if (o->state & 0x20) {
                landed = eng3_01600(c);
                if (i == 4 && landed && o->unknown_bc) {
                    play_sound(0x6099, c);
                    o->unknown_bc = 0;
                }
                vec_diff_fixed_1420((const int *)c, (const int *)g_focus, d);
                if (o->profile == g_focus->profile && d[2] > -0x24 && d[2] < 0x24 && d[0] > 0 && d[0] < 0x20
                    && d[1] > -0x20 && d[1] < 0x18) {
                    g_focus->unknown_116 = 2;
                    g_focus->unknown_110 = o;
                    if (g_eng3_mode_41cb4 != 5)
                        eng3_19200();
                }
                if (c->unknown_9a && !landed) {
                    if (c->sprite != 6)
                        c->sprite = 6;
                    c->step += 0x10;
                    if (c->unknown_e0 != 1) {
                        created = create_eng3_kind_actor(c->x - 0x800000, c->y, c->z, 0x2016);
                        if (created)
                            g_eng3_host_40bb4->method_b0(created);
                        for (j = 0; j < 8; j++) {
                            created = create_eng3_middle_actor(c->x, c->y + random_limit(0x200000),
                                                               c->z + random_limit(0x200000), 0x4002);
                            if (created) {
                                g_eng3_middle_actions_40b98[2].init(created);
                                created->unknown_ac = created->unknown_bc = random_limit(0x200000) + 0x300000;
                                created->unknown_cc = created->unknown_ac >> 1;
                                if (j == 0 || j == 4) {
                                    created->unknown_94 = 0xff;
                                    created->unknown_96 = 0;
                                }
                                if (j == 1 || j == 5) {
                                    created->unknown_94 = 0;
                                    created->unknown_96 = 0;
                                }
                                if (j == 2 || j == 6) {
                                    created->unknown_94 = 0xff;
                                    created->unknown_96 = 0xff;
                                }
                                if (j == 3 || j == 7) {
                                    created->unknown_94 = 0;
                                    created->unknown_96 = 0xff;
                                }
                            }
                        }
                        c->unknown_e0 = 1;
                    }
                }
                if (c->x < g_focus->x - 0xc00000 && o->unknown_11e == 3)
                    o->state = 8;
            }
        }
    }
    if (eng3_1b920(o, 0x100)) {
        release_pending_action(group);
        remove_actor(o);
    }
}

void eng3_0e700(Actor *o)
{
    int count;

    o->flags = 2;
    o->render_flags |= 0x10000040;
    o->category = 0;
    o->speed = -0x400;
    o->state = 8;
    o->unknown_116 = 0;
    count = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->unknown_cc = 1;
    o->unknown_dc = count;
    if (count == 0)
        o->sprite = 5;
    if (count == 1)
        o->sprite = 0xa;
    o->scale = 0x180;
    eng3_1eeb0(o);
    o->y += 0x100000;
}

void eng3_0e7b0(Actor *o)
{
    short d[6];

    o->profile = g_focus->profile;
    if (o->state & 8) {
        vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
        o->unknown_d8 = d[0] - 0x40;
        o->unknown_dc = d[1];
        o->unknown_e0 = d[2];
        o->state = 2;
    }
    if (o->state & 2) {
        g_eng3_host_40bb4->method_a8(create_eng3_kind_actor(o->x, o->y - 0x200000, o->z, 0x2015));
        o->x += -0x100000;
        o->y -= (o->unknown_dc << 16) / 16;
        o->z -= (o->unknown_e0 << 16) / o->unknown_d8 * 16;
        vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
        if (o->profile == g_focus->profile) {
            if (d[2] > -0x10 && d[2] < 0x10 && d[0] > -0x10 && d[0] < 0x10) {
                g_focus->unknown_116 = 0x24;
                g_focus->unknown_110 = o;
                if (g_eng3_mode_41cb4 != 5)
                    eng3_19200();
            }
            if (o->z > 0x2800000)
                remove_actor_variant(o);
            if (o->z < -0x2800000)
                remove_actor_variant(o);
            if (o->x < g_focus->x - 0x1800000)
                remove_actor_variant(o);
            if (o->x > g_focus->x + 0x4000000)
                remove_actor_variant(o);
        }
    }
}

void eng3_0e960(Actor *o)
{
    short d[6], e[6];
    int pick;

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (eng3_1b920(o, 0x100)) {
        remove_actor(o);
        return;
    }
    if (o->profile == g_focus->profile && d[2] > -0x10 && d[2] < 0x10 && d[0] > -0x20 && d[0] < 0x20) {
        g_focus->unknown_116 = 2;
        g_focus->unknown_110 = o;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    if (o->state & 8) {
        o->speed = 0xf18;
        if (d[0] < 0x400)
            o->state = 2;
        if (o->x < g_focus->x - 0xc00000) {
            o->state = 0x20;
            eng3_1cd40(o);
            return;
        }
    }
    if (o->state & 2) {
        if (o->unknown_cc) {
            play_sound(0x6089, o);
            o->unknown_cc = 0;
        }
        o->speed += 0x18;
        if (o->speed > 0xfff)
            o->state = 0x40;
        if (eng3_1b920(o, 0x100)) {
            o->state = 0x20;
            remove_actor(o);
            return;
        }
    }
    if (o->state & 4) {
        o->speed -= 0x18;
        if (o->speed < 0xf19)
            o->state = 8;
        if (eng3_1b920(o, 0x100)) {
            o->state = 0x20;
            remove_actor(o);
            return;
        }
    }
    if (o->state & 0x40) {
        o->step = -eng3_01050(o->x, o->y, o->z, g_focus->x, g_focus->y, g_focus->z);
        vec_diff_fixed_1420((const int *)o, (const int *)g_focus, e);
        if (e[0] > 0x80 && e[2] < 0xec && e[2] > -0xec) {
            if (o->unknown_11e >= 10) {
                g_eng3_middle_actions_40b98[46].init(create_eng3_middle_actor(o->x, o->y - 0x200000, o->z - 0x80000, 0x402e));
                pick = random_limit(3);
                if (pick == 0)
                    play_sound(0x288a, o);
                if (pick == 1)
                    play_sound(0x288b, o);
                if (pick == 2)
                    play_sound(0x288c, o);
                o->unknown_11e = 0;
            }
            o->unknown_11e++;
        }
        if (eng3_1b920(o, 0x100)) {
            o->state = 0x20;
            remove_actor(o);
        }
    }
}

void eng3_0ebe0(Actor *o)
{
    o->sprite = 0;
    o->flags = 2;
    o->unknown_116 = random_limit(0x800);
    select_frame(o, 0);
    o->state = 8;
    o->unknown_118 = 0;
    o->scale = 0x168;
    o->unknown_11e = 0;
    o->unknown_126 = 1;
    o->unknown_98 = 1;
    attach_actor(o, 0x2017);
}

void eng3_0ec60(Actor *o)
{
    short d[6];
    int swing, reach;

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    swing = (g_eng3_table_407b4[(o->unknown_116 - 0x400) & 0xfff] * 0x5a) >> 12;
    reach = ((g_eng3_table_407b4[(swing - 0x400) & 0xfff] << 8) >> 12) * 4;
    if (swing < 0)
        o->state = 2;
    if (swing >= 0)
        o->state = 4;
    if (swing > 0x58 && o->unknown_126) {
        o->unknown_126 = 0;
        play_sound(0x6081, o);
    }
    if (swing < -0x58 && !o->unknown_126) {
        o->unknown_126 = 1;
        play_sound(0x6082, o);
    }
    if (o->state & 4)
        o->unknown_11e = 1;
    o->unknown_116 = (o->unknown_116 + 0x30) & 0xfff;
    eng3_1dbc0(o);
    o->unknown_74 = swing;
    if (o->profile == g_focus->profile && d[2] > reach - 0x10 && d[2] < reach + 0x10 && d[0] > -0x40 && d[0] < 0x40) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 0x23;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
        if (o->unknown_98) {
            play_sound(0x6080, g_focus);
            o->unknown_98 = 0;
        }
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_0ee00(Actor *o)
{
    o->flags = 2;
    o->render_flags |= 0x80000;
    o->unknown_116 = 0;
    o->sprite = 0x27d;
    attach_actor(o, 0x2017);
    o->scale = 0x190;
}

void eng3_0ee40(Actor *o)
{
    o->flags = 2;
    o->state = 2;
    o->ticks = 1;
    o->unknown_11e = 1;
    o->unknown_116 = 0;
    o->unknown_126 = 8;
    o->unknown_dc = 1;
    o->unknown_98 = 0;
    select_frame(o, 4);
    eng3_1dbc0(o);
    o->y = 0x1c00000;
    eng3_1eeb0(o);
    eng3_15340(o);
}

void eng3_0eeb0(Actor *o)
{
    short d[6];
    Actor *c;
    int i;

    o->unknown_98 = 1;
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->unknown_116 && o->unknown_11e) {
        select_frame(o, 3);
        o->unknown_11e = 0;
    }
    if (o->unknown_98) {
        eng3_1dbc0(o);
        if (o->unknown_dc) {
            play_sound(0x28a6, o);
            o->unknown_118 = title_08a50(o->x, o->y, o->z - 0x100000, 0x4a);
            g_eng3_game_40b8c->method_378(o->unknown_118);
            o->unknown_118->render_flags |= 0x10;
            o->unknown_120 = (PendingActionGroup *)title_08a50(o->x, o->y, o->z + 0x100000, 0x4a);
            g_eng3_game_40b8c->method_378((Actor *)o->unknown_120);
            o->unknown_118->profile = o->profile;
            ((Actor *)o->unknown_120)->profile = o->profile;
            o->unknown_dc = 0;
        }
        if (o->state & 2) {
            if (o->unknown_11e)
                o->z += o->unknown_126 << 16;
            if (o->z >= 0xc20000)
                o->state = 4;
            o->x += (g_eng3_table_407b4[((o->z >> 16) - 0x400) & 0xfff] * 0xa0) & 0xffff0000;
        }
        if (o->state & 4) {
            if (o->unknown_11e)
                o->z -= o->unknown_126 << 16;
            if (o->z <= -0xc20000)
                o->state = 2;
            o->x -= (g_eng3_table_407b4[((-o->z >> 16) - 0x400) & 0xfff] * 0xa0) & 0xffff0000;
        }
        if (o->unknown_11e && o->profile == g_focus->profile && d[2] > -0x20 && d[2] < 0x20 && d[0] > -0x20
            && d[0] < 0x20) {
            g_focus->unknown_116 = 0x22;
            g_focus->unknown_110 = o;
            if (g_eng3_mode_41cb4 != 5)
                eng3_19200();
            play_sound(0xe0a7, o);
            o->unknown_116 = 1;
            o->unknown_126 = 0;
            o->unknown_118->unknown_e0 = 1;
            ((Actor *)o->unknown_120)->unknown_e0 = 1;
            o->unknown_120 = 0;
            o->unknown_118 = 0;
            g_eng3_host_40bb4->method_b0(create_eng3_kind_actor(o->x - 0x400000, o->y + 0x80000, o->z, 0x2016));
            for (i = 0; i < 8; i++) {
                c = create_eng3_middle_actor(o->x, o->y - (random_limit(0x80) << 16),
                                             o->z + ((random_limit(0x20) << 16) - (random_limit(0x20) << 16)), 0x4001);
                g_eng3_middle_actions_40b98[1].init(c);
                c->category = 3;
                c->unknown_b8 = c->unknown_bc = (eng3_01010(4) + 0x10) << 16;
                c->unknown_c4 = 0x28000;
                c->unknown_ac = (eng3_01010(8) + 0x1e) << 16;
                c->unknown_a8 = c->unknown_ac >> 2;
                c->unknown_b0 = 0;
                c->unknown_b4 = 0x100;
                c->unknown_95 = 0xff;
                c->unknown_d0 = eng3_01010(0x20);
                c->unknown_cc = 0x1e0000;
                c->profile = o->profile;
                if (i == 0 || i == 4) {
                    if (i == 0)
                        c->sprite = 0x26b;
                    if (i == 4)
                        c->sprite = 0x26c;
                    c->unknown_94 = 0;
                    c->unknown_96 = 0;
                }
                if (i == 1 || i == 5) {
                    if (i == 1)
                        c->sprite = 0x26c;
                    if (i == 5)
                        c->sprite = 0x26d;
                    c->unknown_94 = 0;
                    c->unknown_96 = 0;
                }
                if (i == 2 || i == 6) {
                    if (i == 2)
                        c->sprite = 0x26e;
                    if (i == 6)
                        c->sprite = 0x26f;
                    c->unknown_94 = 0;
                    c->unknown_96 = 0xff;
                }
                if (i == 3 || i == 7) {
                    if (i == 3)
                        c->sprite = 0x270;
                    if (i == 7)
                        c->sprite = 0x271;
                    c->unknown_94 = 0;
                    c->unknown_96 = 0xff;
                }
            }
            remove_actor(o);
            return;
        }
        if (o->unknown_120 && o->unknown_118) {
            o->unknown_118->x = o->x;
            o->unknown_118->y = o->y;
            o->unknown_118->z = o->z - 0x100000;
            ((Actor *)o->unknown_120)->x = o->x;
            ((Actor *)o->unknown_120)->y = o->y;
            ((Actor *)o->unknown_120)->z = o->z + 0x100000;
        }
        if (eng3_1b920(o, 0x100)) {
            o->state = 8;
            remove_actor(o);
        }
    }
}

void eng3_0f3a0(Actor *o)
{
    o->flags = 2;
    o->render_flags |= 0x10000040;
    o->category = 0;
    o->sprite = 1;
    o->step = 0x200;
}

void eng3_0f3d0(Actor *o)
{
    o->sprite = 1;
    if (eng3_1b920(o, 0x100))
        remove_actor_variant(o);
}

void eng3_0f400(Actor *o)
{
    o->flags = 2;
    o->render_flags |= 0x10000040;
    o->category = 0;
    o->unknown_74 = 0x100;
    o->scale = 0x200;
    o->unknown_4a = 0x76;
}

void eng3_0f430(Actor *o)
{
    o->sprite = 0x11;
    if (eng3_1b920(o, 0x100))
        remove_actor_variant(o);
}

void eng3_0f460(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;

    o->unknown_98 = 1;
    if (o->state == 0)
        o->state = 8;
    o->sprite = 0;
    o->flags = 2;
    o->render_flags |= 0x10000040;
    o->unknown_116 = 0;
    eng3_1eeb0(o);
    o->y += -0x140000;
    group = acquire_pending_action_group();
    if (group == 0)
        return;
    o->unknown_120 = group;
    c = eng3_1f410(group, o->x, o->y, o->z, 0x402d, o->profile);
    if (c) {
        c->render_flags |= 0x10000040;
        c->sprite = 0x17;
        c->unknown_94 = 0;
        c->unknown_96 = 0xff;
        c->unknown_dc = 0x20;
        if (c->z < 0)
            c->unknown_dc = -0x20;
    }
    c = eng3_1f410(group, o->x, o->y - 0x500000, o->z, 0x402d, o->profile);
    if (c) {
        c->render_flags |= 0x10000040;
        c->unknown_dc = 0x20;
        if (c->z < 0)
            c->unknown_dc = -0x20;
        c->sprite = 0x17;
        c->unknown_94 = 0;
        c->unknown_96 = 0;
    }
    c = eng3_1f410(group, o->x, o->y, o->z - 0x500000, 0x402d, o->profile);
    if (c) {
        c->render_flags |= 0x10000040;
        c->sprite = 0x17;
        c->unknown_94 = 0;
        c->unknown_96 = 0xff;
        c->unknown_dc = 0x20;
        if (c->z < 0)
            c->unknown_dc = -0x20;
    }
    c = eng3_1f410(group, o->x, o->y - 0x500000, o->z - 0x500000, 0x402d, o->profile);
    if (c) {
        c->render_flags |= 0x10000040;
        c->sprite = 0x17;
        c->unknown_94 = 0xff;
        c->unknown_96 = 0xff;
        c->unknown_dc = 0x20;
        if (c->z < 0)
            c->unknown_dc = -0x20;
    }
}

void eng3_0f650(Actor *o)
{
    o->flags = 2;
    o->render_flags |= 0x10000040;
    o->unknown_b8 = o->unknown_bc = random_limit(0x200000) + 0x200000;
    o->unknown_c0 = random_limit(0x6000) + 0x8fff;
    o->unknown_c4 = random_limit(0x6000) + 0xffff;
    o->unknown_ac = ((eng3_1fac0() & 0xbcf) * 2 + (eng3_1fac0() & 0xbcf)) << 7;
    o->unknown_a8 = o->unknown_ac << 5;
    o->unknown_b4 = random_limit(0x1000) + 0x1000;
    o->unknown_b0 = random_limit(0x1000) + 0x1000;
    o->unknown_cc = ((eng3_1fac0() & 0xfff) << 8) - ((eng3_1fac0() & 0xfff) << 7);
    o->unknown_c8 = o->unknown_cc << 5;
    o->unknown_d0 = random_limit(0x140) - 0x500;
    o->unknown_d4 = random_limit(0x1000) - 0x1000;
    o->unknown_95 = 0;
}

void eng3_0f780(Actor *o)
{
    if (eng3_1b920(o, 0x100))
        remove_actor_variant(o);
}

void eng3_0f7b0(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;
    short d[6];
    int i;

    group = o->unknown_120;
    if (group) {
        for (i = 0; i < group->count;) {
            c = group->entries[++i];
            if (i == 1 && c->unknown_dc == 0x20 && o->unknown_98) {
                play_sound(0x28a5, c);
                o->unknown_98 = 0;
            }
            if (o->state & 2) {
                if (c->unknown_dc > 0) {
                    c->unknown_dc--;
                    if (c->unknown_dc < 0)
                        c->unknown_dc = 0;
                    c->step += (short)c->unknown_dc;
                }
                if (c->unknown_dc < 0) {
                    c->unknown_dc++;
                    if (c->unknown_dc > 0)
                        c->unknown_dc = 0;
                    c->step += (short)c->unknown_dc;
                }
                if (d[2] > -0x40 && d[2] < 0x40 && d[0] > -0x40 && d[0] < 0x40) {
                    g_focus->unknown_110 = o;
                    g_focus->unknown_116 = 0x2b;
                    if (g_eng3_mode_41cb4 != 5)
                        eng3_19200();
                }
            }
            if (o->state & 8) {
                vec_diff_fixed_1420((const int *)c, (const int *)g_focus, d);
                c->step += (short)c->unknown_dc;
                if (o->profile == g_focus->profile && d[2] > -0x36 && d[2] < 0x36 && d[0] > -0x40 && d[0] < 0x40) {
                    g_focus->unknown_110 = o;
                    g_focus->unknown_116 = 0x2b;
                    if (g_eng3_mode_41cb4 != 5)
                        eng3_19200();
                }
            }
        }
    }
    if (o->unknown_116 == 0) {
        if (eng3_1b920(o, 0x100)) {
            release_pending_action(group);
            remove_actor(o);
        }
    } else if (eng3_1b920(o, 0x100)) {
        release_pending_action(group);
        remove_actor(o);
    }
}

void eng3_0f980(Actor *o)
{
    unsigned char *home;
    Actor *c;

    o->flags = 2;
    o->render_flags |= 0x10000040;
    o->category = 0;
    o->sprite = 9;
    o->unknown_116 = 0;
    if (o->state == 0)
        o->state = 8;
    home = o->owner_context + g_kind_info[o->profile].sidecar_offset;
    o->unknown_dc = home[0xc];
    o->unknown_126 = -1;
    o->unknown_116 = home[0xc];
    o->state = 2;
    o->unknown_c8 = 0;
    o->unknown_120 = 0;
    o->unknown_d8 = 1;
    o->unknown_98 = 0;
    o->unknown_e0 = 1;
    attach_actor(o, 0x2002);
    c = create_eng3_middle_actor(o->x, o->y, o->z - 0xc00000, 0x402f);
    if (c) {
        g_eng3_middle_actions_40b98[47].init(c);
        o->unknown_110 = c;
        c->profile = o->profile;
    }
    c = create_eng3_middle_actor(o->x, o->y, o->z - 0x1c00000, 0x402f);
    if (c) {
        g_eng3_middle_actions_40b98[47].init(c);
        o->unknown_118 = c;
        c->profile = o->profile;
    }
}

void eng3_0fab0(Actor *o)
{
    short d[6];
    long *home;
    Actor *created;

    o->scale = 0x2a0;
    o->sprite = 9;
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    home = (long *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
    if (o->unknown_126 == -1) {
        o->unknown_120 = (PendingActionGroup *)title_08a50(home[0] - 0x400000, home[1], home[2] - 0x1000000, 0x3f);
        if (o->unknown_120) {
            g_eng3_game_40b8c->method_2f4((Actor *)o->unknown_120);
            ((Actor *)o->unknown_120)->profile = o->profile;
            ((Actor *)o->unknown_120)->unknown_11e = 1;
        }
        o->unknown_d8 = (long)title_08a50(home[0] - 0xc40000, home[1], home[2] - 0x1600000, 0x3f);
        if (o->unknown_d8) {
            g_eng3_game_40b8c->method_2f4((Actor *)o->unknown_d8);
            ((Actor *)o->unknown_d8)->profile = o->profile;
            ((Actor *)o->unknown_d8)->unknown_11e = 2;
        }
        o->unknown_126 = 1;
    }
    o->unknown_98 = 1;
    if (o->z >= home[2] + 0xac0000) {
        o->state = 4;
        if (o->unknown_e0) {
            if (d[0] > 0x180) {
                created = create_eng3_kind_actor(o->x - 0x800000, o->y + 0x400000, o->z + 0x300000, 0x2016);
                if (created)
                    g_eng3_host_40bb4->method_b0(created);
                created = create_eng3_kind_actor(o->x - 0x800000,
                                                 o->y + (((random_limit(0x10) + 0x54) << 16) - (random_limit(0x10) << 16)),
                                                 o->z + (((random_limit(0x10) + 0x30) << 16) - (random_limit(0x10) << 16)),
                                                 0x2016);
                if (created)
                    g_eng3_host_40bb4->method_b0(created);
            }
            o->unknown_e0 = 0;
        }
    }
    if (o->z <= home[2] + 0x200000) {
        o->state = 2;
        o->unknown_e0 = 1;
        if (d[0] < 0) {
            o->state = 8;
            o->unknown_d0 = 0;
            o->unknown_cc = 0;
        }
    }
    if (o->state & 2) {
        if (o->unknown_dc == 0) {
            o->unknown_d0 = 0x24000;
            o->unknown_cc = 0x240000;
        } else {
            o->unknown_d0 = 0x48000;
            o->unknown_cc = 0x480000;
        }
        ((Actor *)o->unknown_120)->unknown_74 = o->speed = ((home[2] >> 16) - (o->z >> 16)) * 2;
        ((Actor *)o->unknown_d8)->unknown_74 = o->speed;
    }
    if (o->state & 4) {
        if (o->unknown_dc == 0) {
            o->unknown_d0 = -0x2000;
            o->unknown_cc = -0x40000;
        } else {
            o->unknown_d0 = -0x4000;
            o->unknown_cc = -0x80000;
        }
        ((Actor *)o->unknown_120)->unknown_74 = o->speed = ((home[2] >> 16) - (o->z >> 16)) * 2;
        ((Actor *)o->unknown_d8)->unknown_74 = o->speed;
    }
    advance_vertical_motion(o);
    o->unknown_110->x = o->x;
    o->unknown_110->y = o->y;
    o->unknown_110->z = o->z - 0xc00000;
    o->unknown_110->speed = o->speed;
    o->unknown_118->x = o->x;
    o->unknown_118->y = o->y;
    o->unknown_118->z = o->z - 0x1c00000;
    o->unknown_118->speed = o->speed;
    if (o->profile == g_focus->profile && g_focus->z > o->z - 0x540000 && g_focus->z < o->z + 0x540000
        && g_focus->x > o->x - 0x4e8000 && g_focus->x < o->x + 0x358000) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 0x28;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    if (eng3_1b920(o, 0x100)) {
        remove_actor_variant(o->unknown_110);
        remove_actor_variant(o->unknown_118);
        remove_actor(o);
    }
}

void eng3_0fec0(Actor *o)
{
    unsigned char *home;
    Actor *c;

    home = o->owner_context + g_kind_info[o->profile].sidecar_offset;
    o->render_flags |= 0x10000040;
    o->flags = 0x202;
    o->category = 0;
    o->unknown_126--;
    o->sprite = 9;
    o->unknown_116 = 0;
    o->state = 2;
    o->unknown_c8 = 0;
    o->unknown_dc = home[0xc];
    o->unknown_e0 = 1;
    attach_actor(o, 0x2002);
    c = create_eng3_middle_actor(o->x, o->y, o->z + 0xc00000, 0x402f);
    o->unknown_98 = 0;
    if (c) {
        o->unknown_110 = c;
        g_eng3_middle_actions_40b98[47].init(c);
        c->profile = o->profile;
    }
    c = create_eng3_middle_actor(o->x, o->y, o->z + 0x1c00000, 0x402f);
    if (c) {
        o->unknown_118 = c;
        g_eng3_middle_actions_40b98[47].init(c);
        c->profile = o->profile;
    }
}

void eng3_0ffd0(Actor *o)
{
    short d[6];
    long *home;

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    home = (long *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
    if (o->unknown_126 == -1) {
        o->unknown_120 = (PendingActionGroup *)title_08a50(home[0] - 0x400000, home[1], home[2] + 0x1000000, 0x3f);
        if (o->unknown_120) {
            g_eng3_game_40b8c->method_2f4((Actor *)o->unknown_120);
            ((Actor *)o->unknown_120)->profile = o->profile;
            ((Actor *)o->unknown_120)->unknown_11e = 1;
        }
        o->unknown_d8 = (long)title_08a50(home[0] - 0xc40000, home[1], home[2] + 0x1600000, 0x3f);
        if (o->unknown_d8) {
            g_eng3_game_40b8c->method_2f4((Actor *)o->unknown_d8);
            ((Actor *)o->unknown_d8)->profile = o->profile;
            ((Actor *)o->unknown_d8)->unknown_11e = 3;
        }
        o->unknown_126 = 1;
    }
    o->unknown_98 = 1;
    o->scale = 0x2a0;
    o->sprite = 9;
    o->unknown_74 = 0x200;
    o->unknown_96 = 0xff;
    if (o->z <= home[2] - 0xac0000)
        o->state = 4;
    if (o->z >= home[2] - 0x200000) {
        o->state = 2;
        if (d[0] < 0) {
            o->state = 8;
            o->unknown_d0 = 0;
            o->unknown_cc = 0;
        }
    }
    if (o->state & 2) {
        if (o->unknown_e0) {
            play_sound(0x60a3, o);
            o->unknown_e0 = 0;
        }
        if (o->unknown_dc == 0) {
            o->unknown_d0 = 0x24000;
            o->unknown_cc = 0x240000;
        } else {
            o->unknown_d0 = 0x48000;
            o->unknown_cc = 0x480000;
        }
        ((Actor *)o->unknown_120)->unknown_74 = o->speed = ((home[2] >> 16) - (o->z >> 16)) * 2;
        ((Actor *)o->unknown_d8)->unknown_74 = o->speed;
    }
    if (o->state & 4) {
        if (!o->unknown_e0) {
            play_sound(0xe0a4, o);
            o->unknown_e0 = 1;
        }
        if (o->unknown_dc == 0) {
            o->unknown_d0 = -0x2000;
            o->unknown_cc = -0x40000;
        } else {
            o->unknown_d0 = -0x4000;
            o->unknown_cc = -0x80000;
        }
        ((Actor *)o->unknown_120)->unknown_74 = o->speed = ((home[2] >> 16) - (o->z >> 16)) * 2;
        ((Actor *)o->unknown_d8)->unknown_74 = o->speed;
    }
    advance_vertical_motion(o);
    o->unknown_110->x = o->x;
    o->unknown_110->y = o->y;
    o->unknown_110->z = o->z + 0xc00000;
    o->unknown_110->speed = o->speed;
    o->unknown_118->x = o->x;
    o->unknown_118->y = o->y;
    o->unknown_118->z = o->z + 0x1c00000;
    o->unknown_118->speed = o->speed;
    if (o->profile == g_focus->profile && g_focus->z > o->z - 0x540000 && g_focus->z < o->z + 0x540000
        && g_focus->x > o->x - 0x4e8000 && g_focus->x < o->x + 0x358000) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 0x28;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    if (eng3_1b920(o, 0x100)) {
        remove_actor_variant(o->unknown_110);
        remove_actor_variant(o->unknown_118);
        remove_actor(o);
    }
}
}
