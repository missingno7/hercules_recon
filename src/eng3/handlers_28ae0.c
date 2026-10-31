/* ENG3.DLL 0x28ae0..0x2a210: actor handler family (lane y4 candidate).
 * Actor offsets are provisional shared names, not recovered original types.
 * TU-context hypothesis: the lean windows.h include and the RVA-ordered extern list set the
 * VC5 declaration-count tie-break phase (see candidates/y4/result.json); not proven original text. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct Actor Actor;
typedef union PendingActionGroup PendingActionGroup;

union PendingActionGroup {
    long count;
    void *entries[1];
};

struct Actor {
    long x, y, z;
    unsigned char unknown_0c[0x22 - 0x0c];
    unsigned char category;
    unsigned char unknown_23[0x2a - 0x23];
    short state;
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
    PendingActionGroup *unknown_120;
    unsigned char unknown_124[0x126 - 0x124];
    short unknown_126;
};

/* 12-byte local filled by 0x24870: origin words, then z/y/x extents. */
typedef struct Box Box;
struct Box {
    short x, y, z;
    short size_z, size_y, size_x;
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
    unsigned char unknown_064[0x2c4 - 0x64];
    void (*method_2c4)(Actor *);
    unsigned char unknown_2c8[0x30c - 0x2c8];
    void (*method_30c)(Actor *);
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
extern long g_eng3_405fc;
extern unsigned char g_eng3_script_39f98[];
extern unsigned int eng3_01010(unsigned int);
extern void vec_diff_fixed_1420(const int *, const int *, short *);
extern int eng3_01460(Actor *);
extern int eng3_01600(Actor *);
extern void eng3_08120(int, int, int);
extern void play_sound(unsigned short, Actor *);
extern void eng3_19200(void);
extern unsigned short eng3_1b4a0(short, short, short, int);
extern int eng3_1b920(Actor *, int);
extern Actor *title_08a50(long, long, long, int);
extern void remove_actor(Actor *);
extern Actor *create_eng3_middle_actor(long, long, long, unsigned long);
extern void remove_actor_variant(Actor *);
extern Actor *create_eng3_kind_actor(long, long, long, int);
extern int eng3_1dbc0(Actor *);
extern void select_frame(Actor *, int);
extern void eng3_1eeb0(Actor *);
extern PendingActionGroup *acquire_pending_action_group(void);
extern Actor *eng3_1f410(PendingActionGroup *, long, long, long, int, int);
extern void remove_counted_value(long *, long);
extern void release_pending_action(PendingActionGroup *);
extern void advance_planar_motion(Actor *);
extern void advance_vertical_motion(Actor *);
extern unsigned int random_limit(int);
extern int eng3_24870(Actor *, Box *, int);

void eng3_28ae0(Actor *o)
{
    short d[6];
    int i;
    Actor *child;

    if (o->profile != g_focus->profile) {
        remove_actor(o);
        return;
    }
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->state & 8 && d[2] > -0x3c && d[2] < 0x3c && d[0] > -0x10 && d[0] < 0x10 &&
        d[1] - 0x1e != 0 && d[1] < 0x1e) {
        child = create_eng3_kind_actor(o->x, o->y, o->z, 0x2016);
        if (child)
            g_eng3_host_40bb4->method_b0(child);
        o->state = 2;
        o->sprite = 0;
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 0x32;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    if (o->state & 2) {
        play_sound(0xe286, o);
        for (i = 0; i < 8; i++) {
            child = title_08a50(o->x, o->y - (eng3_01010(0x40) << 16), o->z, 0x41);
            if (!child)
                continue;
            g_eng3_game_40b8c->method_30c(child);
            if (i == 0)
                child->sprite = 0x136;
            if (i == 1)
                child->sprite = 0x137;
            if (i == 2)
                child->sprite = 0x138;
            if (i == 3)
                child->sprite = 0x139;
            if (i == 4)
                child->sprite = 0x13a;
            if (i == 5)
                child->sprite = 0x13b;
            if (i == 6)
                child->sprite = 0x13c;
            if (i == 7)
                child->sprite = 0x13d;
            child->unknown_bc = random_limit(0x200000) + 0x300000;
            child->scale = 0x80;
            child->unknown_cc = child->unknown_ac = child->unknown_bc >> 2;
            if (i == 0 || i == 4) {
                child->unknown_94 = 0xff;
                child->unknown_96 = 0;
            }
            if (i == 1 || i == 5) {
                child->unknown_94 = 0;
                child->unknown_96 = 0;
            }
            if (i == 2 || i == 6) {
                child->unknown_94 = 0xff;
                child->unknown_96 = 0xff;
            }
            if (i == 3 || i == 7) {
                child->unknown_94 = 0;
                child->unknown_96 = 0xff;
            }
        }
        for (i = 0; i < 0xc; i++) {
            child = title_08a50(o->x, o->y - (random_limit(0x60) << 16),
                               (random_limit(0x40) << 16) - (random_limit(0x40) << 16) + o->z, 0x3b);
            if (!child)
                continue;
            g_eng3_game_40b8c->method_2c4(child);
            child->unknown_bc = (random_limit(0) + 0x40) << 16;
            child->unknown_ac = child->unknown_bc >> 1;
            child->unknown_cc = child->unknown_ac >> 2;
            child->unknown_4a = 0x80 - random_limit(0x40);
            child->scale = 0x100 - random_limit(0x64);
            child->unknown_95 = 0;
            if (i == 0 || i == 4 || i == 8 || i == 0xc || i == 0x10 || i == 0x14 || i == 0x18 || i == 0x1c) {
                child->sprite = 0x138;
                child->unknown_94 = 0;
                child->unknown_96 = 0;
                if (o->unknown_dc)
                    child->unknown_96 = 0xff;
            }
            if (i == 1 || i == 5 || i == 9 || i == 0xd || i == 0x11 || i == 0x15 || i == 0x19 || i == 0x1d) {
                child->unknown_94 = 0;
                child->sprite = 0x13b;
                child->unknown_96 = 0;
                if (o->unknown_dc)
                    child->unknown_96 = 0xff;
            }
            if (i == 2 || i == 6 || i == 0xa || i == 0xe || i == 0x12 || i == 0x16 || i == 0x1a || i == 0x1e) {
                child->unknown_94 = 0;
                child->unknown_96 = 0xff;
                child->sprite = 0x13d;
                if (o->unknown_dc)
                    child->unknown_96 = 0xff;
            }
            if (i == 3 || i == 7 || i == 0xb || i == 0xf || i == 0x13 || i == 0x17 || i == 0x1b || i == 0x1f) {
                child->unknown_94 = 0;
                child->unknown_96 = 0xff;
                child->sprite = 0x13c;
                if (o->unknown_dc)
                    child->unknown_96 = 0xff;
            }
        }
        o->state = 4;
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_28f20(Actor *o)
{
    if (o->state == 0)
        o->state = 2;
    o->flags = 2;
    o->render_flags = 0x80000000;
    o->state = 2;
    o->unknown_b8 = o->unknown_bc = random_limit(0x100000) + 0x100000;
    o->unknown_c0 = random_limit(0x10000) + 0x10000;
    o->unknown_c4 = random_limit(0x80) + 0x100;
    o->unknown_a8 = o->unknown_ac = o->unknown_bc >> 1;
    o->unknown_b4 = 0x100 - random_limit(0x20);
    o->unknown_b0 = 0x100 - random_limit(0x20);
    o->unknown_c8 = o->unknown_cc = o->unknown_ac >> 2;
    o->unknown_d0 = random_limit(0x100);
    o->unknown_d4 = random_limit(0x100);
    o->unknown_95 = 0;
    o->unknown_98 = 1;
    o->unknown_d8 = 1;
    o->unknown_116 = eng3_01010(0x40);
}

void eng3_29040(Actor *o)
{
    if (o->unknown_dc && o->script == g_eng3_script_39f98)
        eng3_1dbc0(o);
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
    if (eng3_01460(o)) {
        o->speed = 0;
        o->step = 0;
        o->unknown_74 = 0;
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_29140(Actor *o)
{
    PendingActionGroup *group;
    long i;
    Actor *child;

    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    if (o->state == 0)
        o->state = 8;
    o->sprite = 0;
    o->flags = 2;
    o->render_flags |= 0x10000640;
    o->unknown_116 = 8;
    o->unknown_11e = 0x20;
    o->ticks = 0x1e;
    o->unknown_98 = 1;
    group = acquire_pending_action_group();
    if (group) {
        o->unknown_120 = group;
        for (i = 0; i < 0x200000; i += 0x200000) {
            child = eng3_1f410(group, o->x - i, o->y, o->z, 0x402a, o->profile);
            if (child) {
                child->sprite = 7;
                child->flags = 2;
                child->render_flags |= 0x10000040;
                child->unknown_d8 = random_limit(0x14) + 0x14;
                child->unknown_dc = 0;
                child->ticks = 0x1e;
                child->state = 2;
                child->unknown_94 = 0xff;
                eng3_01010(0x80);
                child->unknown_96 = 0;
                child->scale = 0x200;
                child->unknown_4a = 0xfe;
                child->unknown_bc = 0x40000;
                child->unknown_ac = random_limit(0x40000) + 0x60000;
                child->unknown_cc = random_limit(0x40000) + 0x40000;
                child->unknown_b0 = random_limit(0x4000) + 0x2000;
                child->unknown_d0 = random_limit(0x4000) + 0x2000;
                child->unknown_a8 = 0;
                child->unknown_c8 = 0;
            }
        }
    }
}

void eng3_292f0(Actor *o)
{
    PendingActionGroup *group;
    int i, j;
    int t;
    Actor *e, *child;
    short d[6];
    Box b;

    group = o->unknown_120;
    if (group) {
        for (i = 0; i < group->count;) {
            e = (Actor *)group->entries[++i];
            if (o->unknown_98) {
                play_sound(0x2aa3, e);
                o->unknown_98 = 0;
            }
            e->speed += e->ticks;
            if (e->state != 0x20) {
                if (e->state & 2) {
                    e->unknown_dc -= 8;
                    if (e->unknown_dc <= random_limit(0x10) - 0x20)
                        e->state = 4;
                }
                if (e->state & 4) {
                    e->unknown_dc += 8;
                    if (e->unknown_dc >= 0) {
                        e->unknown_d8 = random_limit(0xa0);
                        e->state = 0x20;
                        e->unknown_dc = 0;
                    }
                }
            } else {
                if (e->unknown_d8-- < 0)
                    e->state = 2;
            }
            if (e->z > 0)
                t = eng3_1b4a0((short)(e->x >> 16), (short)(e->y >> 16), (short)(o->unknown_11e + (e->z >> 16)), o->profile);
            else
                t = eng3_1b4a0((short)(e->x >> 16), (short)(e->y >> 16), (short)((e->z >> 16) - o->unknown_11e), o->profile);
            if (t == 0x5e)
                e->unknown_96 = 0;
            if (t == 0x5d)
                e->unknown_96 = 0xff;
            vec_diff_fixed_1420((const int *)e, (const int *)g_focus, d);
            eng3_24870(e, &b, 1);
            if (d[2] > -(b.size_z >> 1) && d[2] < b.size_z >> 1 && d[0] > -(b.size_x >> 1) && d[0] < b.size_x >> 1 &&
                d[1] > -b.size_y && d[1] < b.size_y) {
                g_focus->unknown_116 = 0x26;
                g_focus->unknown_110 = o;
                if (g_eng3_mode_41cb4 != 5)
                    eng3_19200();
                if (g_eng3_3fb20) {
                    child = create_eng3_kind_actor(o->x - 0x400000, o->y, o->z, 0x2016);
                    if (child)
                        g_eng3_host_40bb4->method_b0(child);
                    if (o->unknown_dc && o->profile == 0 && !(g_engine_interface.context_004->flags_11 & 2)) {
                        child = title_08a50(g_focus->x, g_focus->y, g_focus->z, 8);
                        if (child) {
                            g_eng3_game_40b8c->method_60(child);
                            child->profile = o->profile;
                            select_frame(child, 5);
                        }
                    }
                    play_sound(0xe2a8, e);
                    for (j = 0; j < 0x10; j++) {
                        child = create_eng3_middle_actor(g_focus->x + 0x800000,
                                                         g_focus->y - (eng3_01010(0x80) << 16),
                                                         ((random_limit(0x100) - random_limit(0x100)) << 16) + g_focus->z,
                                                         0x4004);
                        if (!child)
                            continue;
                        g_eng3_middle_actions_40b98[4].init(child);
                        child->unknown_b8 = child->unknown_bc = (eng3_01010(4) + 0x10) << 16;
                        child->unknown_c4 = 0x28000;
                        child->unknown_a8 = child->unknown_ac = (eng3_01010(8) + 0x1e) << 16;
                        child->unknown_b0 = 0;
                        child->unknown_b4 = 0x100;
                        child->unknown_d0 = eng3_01010(0x20);
                        child->unknown_cc = 0x1e0000;
                        if (j == 0 || j == 4 || j == 8 || j == 0xc) {
                            child->sprite = 0xb;
                            child->unknown_94 = 0;
                            child->unknown_96 = 0;
                        }
                        if (j == 1 || j == 5 || j == 9 || j == 0xd) {
                            child->sprite = 0xc;
                            child->unknown_94 = 0;
                            child->unknown_96 = 0;
                        }
                        if (j == 2 || j == 6 || j == 0xa || j == 0xe) {
                            child->sprite = 0xc;
                            child->unknown_94 = 0;
                            child->unknown_96 = 0xff;
                        }
                        if (j == 3 || j == 7 || j == 0xb || j == 0xf) {
                            child->unknown_94 = 0;
                            child->sprite = 0xc;
                            child->unknown_96 = 0xff;
                        }
                    }
                    remove_actor(o);
                    return;
                }
            }
            eng3_1eeb0(e);
            e->y -= 0x360000;
            advance_vertical_motion(e);
            advance_planar_motion(e);
            if (e->x < g_focus->x - 0xc40000) {
                remove_counted_value((long *)group, (long)e);
                remove_actor_variant(e);
            }
        }
    }
    if (eng3_1b920(o, 0x100)) {
        release_pending_action(group);
        remove_actor(o);
    }
}

void eng3_297d0(Actor *o)
{
    PendingActionGroup *group;
    int i;
    Actor *child;

    o->unknown_dc = 0x14;
    o->unknown_e0 = 0x20000;
    o->unknown_98 = 1;
    o->unknown_11e = 0x64;
    group = acquire_pending_action_group();
    if (group) {
        o->unknown_120 = group;
        for (i = 0; i < o->unknown_dc; i++) {
            child = eng3_1f410(group, o->x - (i << 23), 0xfe800000,
                               (random_limit(0x100) << 16) - (random_limit(0x100) << 16) + o->z,
                               0x402b, o->profile);
            if (!child)
                continue;
            child->sprite = 7;
            child->flags = 2;
            child->render_flags |= 0x10000040;
            child->unknown_d8 = 1;
            child->unknown_dc = 0;
            child->ticks = 0x1e;
            child->unknown_98 = 1;
            child->unknown_e0 = 0x20;
            if (i == 0)
                child->scale = 0x40;
            if (i == 1)
                child->scale = 0x40;
            if (i == 2)
                child->scale = 0x80;
            if (i == 3)
                child->scale = 0xc0;
            if (i == 4)
                child->scale = 0x100;
            if (i == 5)
                child->scale = 0x140;
            if (i == 6)
                child->scale = 0x100;
            if (i == 7)
                child->scale = 0xc0;
            if (i == 8)
                child->scale = 0x80;
            if (i == 9)
                child->scale = 0x40;
            if (i == 0xa)
                child->scale = 0x40;
            if (i == 0xb)
                child->scale = 0x40;
            if (i == 0xc)
                child->scale = 0x80;
            if (i == 0xd)
                child->scale = 0xc0;
            if (i == 0xe)
                child->scale = 0x100;
            if (i == 0xf)
                child->scale = 0x140;
            if (i == 0x10)
                child->scale = 0x100;
            if (i == 0x11)
                child->scale = 0xc0;
            if (i == 0x12)
                child->scale = 0x80;
            if (i == 0x13)
                child->scale = 0x40;
            child->unknown_d8 = 1;
            child->state = 8;
            child->unknown_94 = 0xff;
            child->unknown_96 = 0;
            child->unknown_bc = 0x1e0000;
            child->unknown_ac = 0;
            child->unknown_cc = 0xf0000;
            child->unknown_b8 = 0;
            child->unknown_c0 = 0x20000;
            child->unknown_c4 = 0xffff;
        }
    }
}

void eng3_29a10(Actor *o)
{
    PendingActionGroup *group;
    int i, j;
    Actor *e, *child;
    short d[6];
    Box b;

    group = o->unknown_120;
    if (g_eng3_mode_41cb4 != 6) {
        if (o->unknown_11e-- > 0) {
            for (j = 0; j < 3; j++) {
                g_eng3_405fc = o->unknown_e0;
                o->unknown_e0 += 0x20000;
                if (o->unknown_e0 > 0x80000)
                    o->unknown_e0 = -0x80000;
            }
        }
    } else {
        g_eng3_405fc = 0;
    }
    if (group) {
        for (i = 0; i < group->count;) {
            e = (Actor *)group->entries[++i];
            if (o->unknown_98) {
                play_sound(0x2a7, 0);
                o->unknown_98 = 0;
            }
            vec_diff_fixed_1420((const int *)e, (const int *)g_focus, d);
            if (e->state & 8) {
                if (e->unknown_d8 == 1) {
                    for (j = 0; j < 3; j++) {
                        child = create_eng3_middle_actor(e->x, e->y, e->z, 0x4002);
                        if (child) {
                            g_eng3_middle_actions_40b98[2].init(child);
                            child->unknown_94 = 0xff;
                            child->unknown_96 = 0;
                            child->unknown_bc = 0x1e0000;
                            child->unknown_ac = 0;
                            child->unknown_cc = 0;
                            child->unknown_b8 = 0;
                            child->unknown_c0 = 0x20000;
                            child->unknown_c4 = 0xffff;
                            if (j == 0 || j == 4) {
                                child->unknown_94 = 0xff;
                                child->unknown_96 = 0;
                            }
                            if (j == 1 || j == 5) {
                                child->unknown_94 = 0;
                                child->unknown_96 = 0;
                            }
                            if (j == 2 || j == 6) {
                                child->unknown_94 = 0xff;
                                child->unknown_96 = 0xff;
                            }
                            if (j == 3 || j == 7) {
                                child->unknown_94 = 0;
                                child->unknown_96 = 0xff;
                            }
                        }
                        e->unknown_d8 = 0;
                    }
                }
                if (d[0] < 0x2bc)
                    e->state = 4;
            }
            if (e->state & 4) {
                e->step += e->unknown_e0 << 1;
                e->speed += e->unknown_e0 >> 1;
                e->unknown_74 += e->unknown_e0 >> 1;
                if (e->unknown_9a) {
                    e->unknown_e0 >>= 1;
                    if (e->unknown_98 == 1) {
                        if (e->scale == 0x140)
                            play_sound(0xe2a4, e);
                        if (e->scale == 0x100)
                            play_sound(0xe2a5, e);
                        if (e->scale == 0xc0)
                            play_sound(0xe2a6, e);
                        if (e->scale > 0x80) {
                            child = create_eng3_kind_actor(e->x - 0x800000, e->y, e->z, 0x2016);
                            if (child)
                                g_eng3_host_40bb4->method_b0(child);
                        }
                        for (j = 0; j < 2; j++) {
                            child = create_eng3_middle_actor(e->x, random_limit(0x200000) + e->y,
                                                             random_limit(0x200000) + e->z, 0x4002);
                            if (child) {
                                g_eng3_middle_actions_40b98[2].init(child);
                                if (j == 0 || j == 4) {
                                    child->unknown_94 = 0xff;
                                    child->unknown_96 = 0;
                                }
                                if (j == 1 || j == 5) {
                                    child->unknown_94 = 0;
                                    child->unknown_96 = 0;
                                }
                                if (j == 2 || j == 6) {
                                    child->unknown_94 = 0xff;
                                    child->unknown_96 = 0xff;
                                }
                                if (j == 3 || j == 7) {
                                    child->unknown_94 = 0;
                                    child->unknown_96 = 0xff;
                                }
                            }
                            e->unknown_98 = 0;
                        }
                    }
                }
                if (eng3_01600(e))
                    e->unknown_e0 = 0;
                eng3_24870(e, &b, 1);
                if (d[2] > -(b.size_z >> 1) && d[2] < b.size_z >> 1 && d[0] > -(b.size_x >> 1) && d[0] < b.size_x >> 1 &&
                    d[1] > -b.size_y && d[1] < b.size_y) {
                    g_focus->unknown_116 = 0x34;
                    g_focus->unknown_110 = o;
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

void eng3_29e50(Actor *o)
{
    o->flags = 0x400202;
    o->render_flags |= 0x10000040;
    o->sprite = 0xa;
    o->step = 0x200;
    o->scale = 0x180;
    o->y -= 0x600000;
    o->unknown_11e = 0;
    o->unknown_98 = 1;
    o->unknown_4a = 0xfe;
}

void eng3_29ea0(Actor *o)
{
    short d[6];
    int i;
    Actor *child;

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (d[0] < 0x200) {
        eng3_08120(0, 0, -1);
        if (o->unknown_98) {
            play_sound(0x2a2, o);
            for (i = 0; i < 0x10; i++) {
                child = create_eng3_middle_actor(o->x, o->y - (eng3_01010(0x40) << 16) - 0x800000,
                                                 (eng3_01010(0x100) << 16) - (eng3_01010(0x100) << 16) + o->z,
                                                 0x4002);
                if (child) {
                    g_eng3_middle_actions_40b98[2].init(child);
                    child->unknown_4a = 0xc8;
                    child->unknown_bc >>= 1;
                    child->unknown_ac >>= 4;
                    if (i == 0 || i == 4) {
                        child->unknown_94 = 0xff;
                        child->unknown_96 = 0;
                    }
                    if (i == 1 || i == 5) {
                        child->unknown_94 = 0;
                        child->unknown_96 = 0;
                    }
                    if (i == 2 || i == 6) {
                        child->unknown_94 = 0xff;
                        child->unknown_96 = 0xff;
                    }
                    if (i == 3 || i == 7) {
                        child->unknown_94 = 0;
                        child->unknown_96 = 0xff;
                        o->unknown_98 = 0;
                    }
                }
            }
            o->unknown_98 = 0;
        }
        o->y += -0x8000 - (random_limit(4) << 16);
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_2a040(Actor *o)
{
    o->flags = 0x202;
    o->render_flags |= 0x10000040;
    o->category = 0;
    o->state = 8;
    o->sprite = 0xf;
    o->ticks = 1;
    eng3_1eeb0(o);
    o->step = 0x200;
    o->y += 0x200000;
}

void eng3_2a090(Actor *o)
{
    short d[6];

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->profile == g_focus->profile && d[2] > -0x20 && d[2] < 0x20 && d[0] > -0x20 && d[0] < 0x20)
        o->state = 2;
    if (o->state & 2) {
        o->step += 0x30;
        o->speed = (g_eng3_table_407b4[(o->unknown_126 - 0x400) & 0xfff] << 6) >> 12;
        o->unknown_126 += 0x20;
        if (o->ticks) {
            g_focus->unknown_110 = o;
            g_focus->unknown_116 = 0x35;
            if (g_eng3_mode_41cb4 != 5)
                eng3_19200();
            o->ticks = 0;
        }
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_2a180(Actor *o)
{
    o->flags = 0x202;
    o->render_flags |= 0x10000040;
    o->category = 0;
    o->state = 8;
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    if (o->unknown_dc == 0)
        o->sprite = 0xd;
    if (o->unknown_dc == 1)
        o->sprite = 0xe;
    if (o->unknown_dc == 2)
        o->sprite = 0x10;
    o->scale = 0x12c;
    o->ticks = 1;
    eng3_1eeb0(o);
    o->step = 0x200;
}
