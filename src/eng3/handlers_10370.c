#define WIN32_LEAN_AND_MEAN
#include <windows.h>
/* ENG3.DLL 0x10370..0x12190: actor handler family (lane y8 candidate).
 * Actor offsets are provisional shared names, not recovered original types.
 * Actor.state is unsigned short here: 0x11f90 tests state & 0x100 as a byte test of +0x2b.
 * 0x10590 and 0x10df0 carry declaration-count phase residues (commutative add destination,
 * random_limit pair evaluation order) that resolve only in a larger TU context; no count-only
 * headers are included (owner ruling 2026-10-20). */
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
    unsigned char unknown_064[0x378 - 0x64];
    void (*method_378)(Actor *);
};

typedef struct Eng3Host Eng3Host;
struct Eng3Host {
    unsigned char unknown_00[0xb0];
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
extern int eng1_2a660(Actor *, Actor *);
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

void eng3_10370(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;
    int i;

    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    if (o->state == 0)
        o->state = 8;
    o->unknown_116 = 8;
    o->sprite = 0;
    o->flags = 2;
    o->render_flags |= 0x10000040;
    if (o->unknown_dc)
        o->unknown_11e = 0x20;
    o->ticks = 0x1e;
    o->unknown_98 = 1;
    group = acquire_pending_action_group();
    if (group == 0)
        return;
    o->unknown_120 = group;
    for (i = 0; i < o->unknown_dc; i++) {
        c = eng3_1f410(group, o->x - i * 0x800000, o->y, o->z, 0x402b, o->profile);
        if (c) {
            c->sprite = 0xd;
            c->flags = 2;
            c->render_flags |= 0x10000040;
            c->unknown_d8 = random_limit(0x14) + 0x14;
            c->unknown_dc = 0;
            c->ticks = eng3_01010(0x10) + 0x14;
            c->state = 2;
            attach_actor(c, 0x2002);
            c->sprite = 0xd;
            c->unknown_94 = 0xff;
            c->unknown_96 = 0;
            c->scale = 0x100;
            if (o->unknown_dc == 1)
                c->scale = 0x200;
            if (i % 2) {
                c->unknown_96 = 0xff;
                c->x += -0x400000;
            }
            c->unknown_bc = 0x40000;
            c->unknown_ac = eng3_01010(0x40000) + 0x60000;
            c->unknown_cc = random_limit(0x40000) + 0x40000;
            c->unknown_b0 = random_limit(0x4000) + 0x2000;
            c->unknown_d0 = random_limit(0x4000) + 0x2000;
            c->unknown_a8 = 0;
            c->unknown_c8 = 0;
        }
    }
}

void eng3_10590(Actor *o)
{
    PendingActionGroup *group;
    Actor *c, *created;
    long count;
    long focus[4], position[4]; /* 16-byte frame slots: four-long vectors, three used */
    short d[6];
    int i, j;
    int surface;

    count = o->unknown_dc;
    group = o->unknown_120;
    focus[0] = g_focus->x;
    focus[1] = g_focus->y;
    focus[2] = g_focus->z;
    if (group) {
        vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
        for (i = 0; i < group->count;) {
            c = group->entries[++i];
            if (o->unknown_dc == 1) {
                if (o->unknown_98) {
                    play_sound(0x2892, c);
                    o->unknown_98 = 0;
                }
            } else if (o->unknown_98) {
                play_sound(0x2893, c);
                o->unknown_98 = 0;
            }
            c->speed += c->ticks;
            if (c->state != 0x20) {
                if (c->state & 2) {
                    c->unknown_dc -= 8;
                    if (c->unknown_dc <= random_limit(0x10) - 0x20)
                        c->state = 4;
                }
                if (c->state & 4) {
                    c->unknown_dc += 8;
                    if (c->unknown_dc >= 0) {
                        c->unknown_d8 = random_limit(0xa0);
                        c->state = 0x20;
                        c->unknown_dc = 0;
                    }
                }
            } else if (c->unknown_d8-- < 0) {
                c->state = 2;
            }
            eng3_01190(i, group, count, 0x30);
            if (c->z > 0)
                surface = eng3_1b4a0((short)(c->x >> 16), (short)(c->y >> 16), (short)(o->unknown_11e + (c->z >> 16)), o->profile);
            else
                surface = eng3_1b4a0((short)(c->x >> 16), (short)(c->y >> 16), (short)((c->z >> 16) - o->unknown_11e), o->profile);
            if (surface == 0x5e)
                c->unknown_96 = 0;
            else if (surface == 0x5d)
                c->unknown_96 = 0xff;
            vec_diff_fixed_1420((const int *)c, (const int *)g_focus, d);
            if (o->profile == g_focus->profile) {
                if (o->unknown_dc != 1) {
                    position[0] = c->x;
                    position[1] = c->y;
                    position[2] = c->z;
                    if (within_radius_1140((const int *)position, (const int *)focus, 0x40)) {
                        g_focus->unknown_116 = 0x27;
                        g_focus->unknown_110 = o;
                        if (g_eng3_mode_41cb4 != 5)
                            eng3_19200();
                    }
                } else if (d[2] > -0x40 && d[2] < 0x40 && d[0] > -0x60 && d[0] < 0x60
                           && d[1] > -0x50 && d[1] < 0x50) {
                    g_focus->unknown_116 = 0x26;
                    g_focus->unknown_110 = o;
                    if (g_eng3_mode_41cb4 != 5)
                        eng3_19200();
                    if (g_eng3_3fb20) {
                        created = create_eng3_kind_actor(o->x - 0x400000, o->y, o->z, 0x2016);
                        if (created)
                            g_eng3_host_40bb4->method_b0(created);
                        if (!(g_engine_interface.context_004->flags_11 & 8) && o->profile) {
                            created = title_08a50(g_focus->x, g_focus->y, g_focus->z, 8);
                            if (created) {
                                g_eng3_game_40b8c->method_60(created);
                                created->profile = o->profile;
                                select_frame(created, 5);
                            }
                        }
                        play_sound(0xe0a9, c);
                        for (j = 0; j < 0x10; j++) {
                            created = create_eng3_middle_actor(g_focus->x + 0x800000,
                                                               g_focus->y - (eng3_01010(0x80) << 16),
                                                               g_focus->z + ((random_limit(0x100) - random_limit(0x100)) << 16),
                                                               0x4004);
                            if (created) {
                                g_eng3_middle_actions_40b98[4].init(created);
                                created->unknown_b8 = created->unknown_bc = (eng3_01010(4) + 0x10) << 16;
                                created->unknown_c4 = 0x28000;
                                created->unknown_a8 = created->unknown_ac = (eng3_01010(8) + 0x1e) << 16;
                                created->unknown_b0 = 0;
                                created->unknown_b4 = 0x100;
                                created->unknown_d0 = eng3_01010(0x20);
                                created->unknown_cc = 0x1e0000;
                                if (j == 0 || j == 4 || j == 8 || j == 12) {
                                    created->sprite = 0x1a;
                                    created->unknown_94 = 0;
                                    created->unknown_96 = 0;
                                }
                                if (j == 1 || j == 5 || j == 9 || j == 13) {
                                    created->sprite = 0x1b;
                                    created->unknown_94 = 0;
                                    created->unknown_96 = 0;
                                }
                                if (j == 2 || j == 6 || j == 10 || j == 14) {
                                    created->sprite = 0x1b;
                                    created->unknown_94 = 0;
                                    created->unknown_96 = 0xff;
                                }
                                if (j == 3 || j == 7 || j == 11 || j == 15) {
                                    created->unknown_94 = 0;
                                    created->sprite = 0x1b;
                                    created->unknown_96 = 0xff;
                                }
                            }
                        }
                        remove_actor(o);
                        return;
                    }
                }
            }
            eng3_1eeb0(c);
            if (o->unknown_dc == 1)
                c->y -= 0x280000;
            else
                c->y -= 0x40000;
            advance_vertical_motion(c);
            advance_planar_motion(c);
            if (eng3_1b920(c, 0)) {
                remove_counted_value(group, c);
                remove_actor_variant(c);
            }
        }
    }
    if (eng3_1b920(o, 0)) {
        release_pending_action(group);
        remove_actor(o);
    }
}

void eng3_10b10(Actor *o)
{
    int z_steps[24] = {0, 2, 4, 5, 1, 3, 5, 3, 4, 3, 2, 1, 2, 3, 0, 3, 2, 3, 1, 0};
    int x_steps[24] = {0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 5, 6, 6, 6, 7, 7, 7, 7};
    PendingActionGroup *group;
    Actor *c;
    int i;

    if (o->state == 0)
        o->state = 8;
    o->unknown_dc = 4;
    o->sprite = 0;
    o->flags = 2;
    o->unknown_116 = 0;
    o->unknown_11e = 0x180;
    o->ticks = 1;
    o->unknown_98 = 0x40;
    group = acquire_pending_action_group();
    if (group == 0)
        return;
    o->unknown_120 = group;
    for (i = 0; i < o->unknown_dc; i++) {
        c = eng3_1f410(group, o->x + ((x_steps[i] * o->unknown_11e) << 16), o->y,
                       o->z + z_steps[i] * (o->unknown_98 << 16), 0x402c, o->profile);
        if (c) {
            c->ticks = 1;
            c->flags = 2;
            c->state = 2;
            attach_actor(c, 0x2017);
            c->unknown_97 = 0;
            c->unknown_94 = 0xff;
            c->unknown_96 = 0xff;
            c->unknown_d8 = 0;
            c->unknown_dc = 1;
            select_frame(o, 1);
            c->script = o->script;
            c->unknown_96 = 0xff;
            c->unknown_98 = 0;
            c->unknown_bc = 0x40000;
            c->unknown_ac = random_limit(0x80000) + 0x50000;
            c->unknown_cc = 0;
            c->unknown_b0 = random_limit(0x4000) + 0x2000;
            c->unknown_d0 = random_limit(0x2000) + 0x2000;
            c->unknown_a8 = 0;
            c->unknown_c8 = 0;
        }
    }
}

void eng3_10df0(Actor *o)
{
    PendingActionGroup *group;
    Actor *c, *created;
    long count;
    short d[6];
    int i, j;
    int surface;

    group = o->unknown_120;
    count = o->unknown_dc;
    if (group) {
        for (i = 0; i < group->count;) {
            c = group->entries[++i];
            c->unknown_cc = random_limit(0x10000) + 0x40000;
            if (c->state) {
                play_sound(0x28a6, c);
                c->unknown_dc = (long)title_08a50(c->x, c->y, c->z - 0x100000, 0x4a);
                if (c->unknown_dc) {
                    g_eng3_game_40b8c->method_378((Actor *)c->unknown_dc);
                    ((Actor *)c->unknown_dc)->render_flags |= 0x10;
                    ((Actor *)c->unknown_dc)->profile = o->profile;
                }
                c->unknown_d8 = (long)title_08a50(c->x, c->y, c->z + 0x100000, 0x4a);
                if (c->unknown_d8) {
                    g_eng3_game_40b8c->method_378((Actor *)c->unknown_d8);
                    ((Actor *)c->unknown_d8)->profile = o->profile;
                }
                c->state = 0;
            }
            if (eng3_01190(i, group, count, 0x20))
                c->unknown_cc = random_limit(0x10000) + 0x40000;
            dispatch_secondary(c);
            dispatch_primary(c);
            vec_diff_fixed_1420((const int *)c, (const int *)g_focus, d);
            if (o->profile == g_focus->profile) {
                if (c->unknown_97 == 0 && d[2] > -0x10 && d[2] < 0x10 && d[0] > -0x10 && d[0] < 0x20) {
                    g_focus->unknown_110 = o;
                    g_focus->unknown_116 = 0x22;
                    if (g_eng3_mode_41cb4 != 5)
                        eng3_19200();
                    c->unknown_97 = 1;
                    ((Actor *)c->unknown_dc)->unknown_e0 = 1;
                    ((Actor *)c->unknown_d8)->unknown_e0 = 1;
                    c->unknown_dc = 0;
                    c->unknown_d8 = 0;
                    created = create_eng3_kind_actor(c->x - 0x400000, c->y + 0x80000, c->z, 0x2016);
                    if (created)
                        g_eng3_host_40bb4->method_b0(created);
                    for (j = 0; j < 8; j++) {
                        created = create_eng3_middle_actor(c->x, c->y - (random_limit(0x80) << 16),
                                                           c->z + ((random_limit(0x20) << 16) - (random_limit(0x20) << 16)),
                                                           0x4001);
                        if (created) {
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
                                    created->sprite = 0x274;
                                if (j == 4)
                                    created->sprite = 0x275;
                                if (c->script == g_eng3_script_35be8) {
                                    if (j == 0)
                                        created->sprite = 0x274;
                                    if (j == 4)
                                        created->sprite = 0x275;
                                }
                                created->unknown_94 = 0;
                                created->unknown_96 = 0;
                            }
                            if (j == 1 || j == 5) {
                                if (j == 1)
                                    created->sprite = 0x275;
                                if (j == 5)
                                    created->sprite = 0x276;
                                if (c->script == g_eng3_script_35be8) {
                                    if (j == 0)
                                        created->sprite = 0x275;
                                    if (j == 4)
                                        created->sprite = 0x276;
                                }
                                created->unknown_94 = 0;
                                created->unknown_96 = 0;
                            }
                            if (j == 2 || j == 6) {
                                if (j == 2)
                                    created->sprite = 0x277;
                                if (j == 6)
                                    created->sprite = 0x278;
                                if (c->script == g_eng3_script_35be8) {
                                    if (j == 0)
                                        created->sprite = 0x277;
                                    if (j == 4)
                                        created->sprite = 0x278;
                                }
                                created->unknown_94 = 0;
                                created->unknown_96 = 0xff;
                            }
                            if (j == 3 || j == 7) {
                                if (j == 3)
                                    created->sprite = 0x279;
                                if (j == 7)
                                    created->sprite = 0x27a;
                                if (c->script == g_eng3_script_35be8) {
                                    if (j == 0)
                                        created->sprite = 0x279;
                                    if (j == 4)
                                        created->sprite = 0x27a;
                                }
                                created->unknown_94 = 0;
                                created->unknown_96 = 0xff;
                            }
                        }
                    }
                    stop_actor_sounds(c);
                    play_sound(0xe0a7, c);
                    c->sprite = 0;
                    remove_counted_value(group, c);
                }
                if (c->z >= 0)
                    surface = eng3_1b4a0((short)(c->x >> 16), (short)(c->y >> 16), (short)((c->z >> 16) + 0x20), c->profile);
                else
                    surface = eng3_1b4a0((short)(c->x >> 16), (short)(c->y >> 16), (short)(c->z >> 16), c->profile);
                if (surface == 0x5e) {
                    c->unknown_96 = 0;
                    c->unknown_cc = random_limit(0x10000) + 0x60000;
                }
                if (surface == 0x5d) {
                    c->unknown_96 = 0xff;
                    c->unknown_cc = random_limit(0x10000) + 0x60000;
                }
                if (c->unknown_97) {
                    c->unknown_cc = 0;
                    c->unknown_ac = 0;
                }
                eng3_1eeb0(c);
                advance_vertical_motion(c);
                advance_planar_motion(c);
                if (c->unknown_97 == 0)
                    eng3_1dbc0(c);
                if (c->unknown_dc && c->unknown_d8) {
                    ((Actor *)c->unknown_dc)->x = c->x;
                    ((Actor *)c->unknown_dc)->y = c->y;
                    ((Actor *)c->unknown_dc)->z = c->z - 0x100000;
                    ((Actor *)c->unknown_d8)->x = c->x;
                    ((Actor *)c->unknown_d8)->y = c->y;
                    ((Actor *)c->unknown_d8)->z = c->z + 0x100000;
                }
                if (eng3_1b920(o, 0x100)) {
                    release_pending_action(group);
                    remove_actor(o);
                }
            }
        }
    }
}

void eng3_113f0(Actor *o)
{
    long *home = (long *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);

    o->flags = 0x202;
    o->state = 2;
    o->ticks = 1;
    o->unknown_116 = random_limit(2) - random_limit(2);
    attach_actor(o, 0x2018);
    o->unknown_126 = 0;
    o->y = home[1] = 0x1e80000;
    select_frame(o, 0);
}

void eng3_11470(Actor *o)
{
    long *home = (long *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
    short d[6];

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->phase > 5 && o->phase < 0x12) {
        o->x -= 0x40000;
        o->z += o->unknown_116 << 16;
        o->y = home[1] - ((g_eng3_table_407b4[(o->unknown_126 - 0x400) & 0xfff] * 0xa40 >> 16) << 16);
        o->unknown_126 += 0x46;
    } else {
        o->y = home[1];
    }
    if (o->phase == 0x1a) {
        o->x = home[0] - (random_limit(0) << 16);
        o->z = home[2] - (random_limit(0) << 16);
        o->y = home[1];
        o->unknown_116 = random_limit(2) - random_limit(2);
        o->unknown_126 = 0;
    }
    if (eng1_2a660(o, g_focus)) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 0x2a;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    eng3_1dbc0(o);
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_115e0(Actor *o)
{
    o->flags = 2;
    o->state = 2;
    o->ticks = 1;
    o->unknown_116 = 0x1e;
    attach_actor(o, 0x2018);
    o->y = 0x1e10000;
    o->unknown_98 = 1;
    select_frame(o, 1);
}

void eng3_11630(Actor *o)
{
    long *home = (long *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
    short d[6];

    o->z = home[2];
    if (o->phase > 0xd && o->phase < 0x16 && o->unknown_98) {
        o->z = home[2] - 0x640000;
        o->unknown_98 = 0;
    }
    if (o->phase >= 0x16)
        o->unknown_98 = 1;
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (eng1_2a660(o, g_focus)) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 0x2a;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    eng3_1dbc0(o);
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_11710(Actor *o)
{
    o->flags = 0x202;
    o->unknown_e0 = 0xb40000;
    eng3_1eeb0(o);
    o->sprite = 0x2b;
}

void eng3_11740(Actor *o)
{
    o->flags = 0x202;
    eng3_1eeb0(o);
    o->sprite = 0;
}

void eng3_11760(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;

    o->state = 0;
    o->unknown_116 = 0x800;
    o->unknown_11e = 0;
    o->unknown_126 = 0x800;
    o->unknown_98 = 0;
    o->unknown_dc = 4;
    o->unknown_e0 = 0;
    o->ticks = 0;
    o->unknown_d8 = random_limit(6) + 0xc;
    o->unknown_ac = o->unknown_cc = 1;
    o->flags = 2;
    o->state = 8;
    group = acquire_pending_action_group();
    if (group == 0)
        return;
    o->unknown_120 = group;
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
        attach_actor(c, 0x2002);
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
        attach_actor(c, 0x2002);
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
        attach_actor(c, 0x2002);
    }
    c = eng3_1f410(group, o->x, o->y + 0x1b80000, o->z, 0x402a, o->profile);
    if (c) {
        c->render_flags |= 0x10000040;
        c->sprite = 0xb;
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

int eng3_11b90(Actor *o, Actor *c);

void eng3_11a80(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;
    int i;

    c = 0;
    group = o->unknown_120;
    if (group) {
        for (i = 0; i < group->count;) {
            c = group->entries[++i];
            o->unknown_116 = ((g_eng3_table_407b4[(o->unknown_126 - 0x400) & 0xfff] << 9) >> 12) + 0x800;
            eng3_11b90(o, c);
            o->unknown_126 += o->unknown_d8;
        }
        if (c->sprite == 0xb && o->profile == g_focus->profile
            && g_focus->z > c->z - 0x820000 && g_focus->z < c->z + 0x820000
            && g_focus->x > c->x - 0x80000 && g_focus->x < c->x + 0x80000) {
            g_focus->unknown_110 = o;
            g_focus->unknown_116 = 0x2c;
            if (g_eng3_mode_41cb4 != 5)
                eng3_19200();
        }
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

int eng3_11b90(Actor *o, Actor *c)
{
    long base[3];          /* z, y, x: frame order puts the z copy lowest */
    long offsets[4][4];    /* 16-byte rows, fourth long never written */
    int angle;
    long dz, dy;

    base[0] = o->z;
    base[1] = o->y - 0x800000;
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
    offsets[3][1] = -0x1b4;
    offsets[3][2] = 0;
    base[2] = o->x;
    angle = o->unknown_116;
    if (c->sprite == 0xb) {
        if (o->unknown_116 < 0x800 && o->unknown_cc) {
            o->unknown_cc = 0;
            o->unknown_ac = 1;
            play_sound(0x6094, c);
        }
        if (o->unknown_116 > 0x800 && o->unknown_ac) {
            o->unknown_cc = 1;
            o->unknown_ac = 0;
            play_sound(0x6095, c);
        }
    }
    c->unknown_74 = angle >> 2;
    dz = (offsets[o->ticks][0] * g_eng3_table_407b4[angle & 0xfff] - offsets[o->ticks][1] * g_eng3_table_407b4[(angle - 0x400) & 0xfff]) >> 12;
    dy = (offsets[o->ticks][0] * g_eng3_table_407b4[(angle - 0x400) & 0xfff] + offsets[o->ticks][1] * g_eng3_table_407b4[angle & 0xfff]) >> 12;
    c->z = (dz << 16) + base[0];
    c->x = base[2];
    c->y = (dy << 16) + base[1];
    o->ticks = (o->ticks + 1) & 3;
    if (o->unknown_e0 == 0 && o->unknown_116 >= 0xc00)
        return 1;
    if (o->unknown_e0 == 1 && o->unknown_116 <= 0x400)
        return 1;
    return 0;
}

void eng3_11d30(Actor *o)
{
    o->flags = 0x202;
    o->state = 8;
    o->ticks = 1;
    o->unknown_126 = 0;
    o->unknown_116 = 0x1e;
    attach_actor(o, 0x2017);
    o->y -= 0x200000;
    select_frame(o, 0);
}

void eng3_11d80(Actor *o)
{
    Actor *created;

    if (o->state & 2) {
        select_frame(o, 1);
        o->phase = 0;
        o->render_flags = 0x80000004;
        play_sound(0x603e, g_focus);
        created = title_08a50(g_focus->x, g_focus->y, g_focus->z, 8);
        if (created) {
            g_eng3_game_40b8c->method_60(created);
            created->profile = o->profile;
            created->unknown_ac = 0x160000;
            created->unknown_bc = 0x100000;
            select_frame(created, 1);
        }
        o->state = 4;
    }
    if (o->state & 4 && o->phase == 0x14) {
        remove_actor(o);
    } else {
        eng3_1dbc0(o);
        if (eng3_1b920(o, 0x100))
            remove_actor(o);
    }
}

void eng3_11e50(Actor *o)
{
    o->flags = 0x202;
    o->render_flags = 0x80000007;
    o->unknown_4a = 0xfe;
    o->state = 2;
    o->ticks = 1;
    o->unknown_116 = 0x1e;
    eng3_1eeb0(o);
    select_frame(o, 1);
}

void eng3_11ea0(Actor *o)
{
    short d[6];

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    eng3_1dbc0(o);
    if (o->x < g_focus->x - 0xc00000)
        remove_actor(o);
}

void eng3_11ef0(Actor *o)
{
    o->category = 3;
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->flags = 0x202;
    o->ticks = 1;
    o->unknown_ac = 0;
    o->unknown_a8 = 0;
    o->unknown_b0 = 0;
    o->unknown_126 = 0;
    o->unknown_cc = 0;
    o->unknown_c8 = 0;
    o->unknown_d0 = 0;
    o->unknown_98 = 0;
    o->unknown_d8 = 0;
    o->unknown_e0 = 1;
    o->state = 2;
    select_frame(o, 0);
    eng3_1eeb0(o);
    eng3_15340(o);
}

void eng3_11f90(Actor *o)
{
    short d[6];

    o->category = 3;
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (d[0] < 0x280 && o->unknown_e0) {
        if (o->profile > 6) {
            eng3_1faf0(0x1b);
            o->state = 0x100;
        }
        if (o->profile == 0) {
            eng3_1faf0(0x19);
            o->state = 0x10;
        }
        if (o->profile > 0 && o->profile <= 6) {
            if (g_engine_interface.context_004->unknown_24 == g_engine_interface.context_004->unknown_1e) {
                eng3_1faf0(0x1b);
                o->state = 0x10;
            }
            if (g_engine_interface.context_004->unknown_24 < g_engine_interface.context_004->unknown_1e
                && g_engine_interface.context_004->unknown_24 > g_engine_interface.context_004->unknown_1e >> 1) {
                eng3_1faf0(0x1a);
                o->state = 8;
            }
            if (g_engine_interface.context_004->unknown_24 <= g_engine_interface.context_004->unknown_1e >> 1
                && g_engine_interface.context_004->unknown_24 > g_engine_interface.context_004->unknown_1e >> 2) {
                eng3_1faf0(0x16);
                o->state = 8;
            }
            if (g_engine_interface.context_004->unknown_24 <= g_engine_interface.context_004->unknown_1e >> 2
                && g_engine_interface.context_004->unknown_24 > 0) {
                eng3_1faf0(0x1c);
                o->state = 4;
            }
        }
        o->unknown_e0 = 0;
    }
    if (o->state & 2)
        eng3_1dbc0(o);
    if (o->state & 4) {
        select_frame(o, 1);
        o->state = 0x20;
    }
    if (o->state & 8) {
        select_frame(o, 2);
        o->state = 0x20;
    }
    if (o->state & 0x10) {
        select_frame(o, 3);
        o->state = 0x20;
    }
    if (o->state & 0x100) {
        select_frame(o, 4);
        o->state = 0x20;
    }
    if (o->state & 0x20 && eng3_1dbc0(o))
        o->state = 0x40;
    if (o->state & 0x40) {
        select_frame(o, 0);
        o->state = 0x80;
    }
    if (o->state & 0x80)
        eng3_1dbc0(o);
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}
