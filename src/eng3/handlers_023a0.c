/* ENG3.DLL 0x23a0..0x6150: level actor handler family (lane y15 candidate).
 * Actor offsets are provisional shared names (from calibration/eng3/handlers_10370.c), not recovered types. */
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
    unsigned char mode;
    unsigned char unknown_24[0x2a - 0x24];
    unsigned short state;
    unsigned char unknown_2c[0x30 - 0x2c];
    short ticks, phase;
    unsigned short sprite;
    unsigned char unknown_36[0x38 - 0x36];
    unsigned short profile;
    unsigned short scale;
    unsigned char unknown_3c[0x3e - 0x3c];
    unsigned short unknown_3e;
    unsigned char unknown_40[0x4a - 0x40];
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
    unsigned char unknown_e4[0xee - 0xe4];
    short unknown_ee;
    unsigned char unknown_f0[0x110 - 0xf0];
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

typedef struct Eng3Sidecar Eng3Sidecar;
struct Eng3Sidecar {
    long x, y, z;
    unsigned char count;
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
    unsigned char unknown_064[0x1ec - 0x64];
    void (*method_1ec)(Actor *);
    unsigned char unknown_1f0[0x240 - 0x1f0];
    void (*method_240)(Actor *);
    unsigned char unknown_244[0x2a0 - 0x244];
    void (*method_2a0)(Actor *);
    unsigned char unknown_2a4[0x2d0 - 0x2a4];
    void (*method_2d0)(Actor *);
    unsigned char unknown_2d4[0x324 - 0x2d4];
    void (*method_324)(Actor *);
    unsigned char unknown_328[0x378 - 0x328];
    void (*method_378)(Actor *);
};

typedef struct Eng3Host Eng3Host;
struct Eng3Host {
    unsigned char unknown_00[0xb0];
    void (*method_b0)(Actor *);
};

typedef struct EngineContextView EngineContextView;
struct EngineContextView {
    unsigned char unknown_00[3];
    unsigned char unknown_03;
    unsigned char unknown_04[0x11 - 4];
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
extern Actor *eng3_1cb00(long, long, long, int);
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
extern void advance_frame(Actor *);
extern int eng3_01050(long, long, long, long, long, long);
extern long g_eng3_3fb94, g_eng3_3fb98;
extern long g_eng3_405fc;
extern int g_eng3_3f218;
extern Actor *g_eng3_actor_3fb14;
extern int g_eng3_sprites_34f98[];
extern int g_eng3_sprites_34fe0[];
extern int eng3_1fac0(void);
extern int eng3_01460(Actor *);
extern int eng3_1ecf0(long, long, long, int);
extern int eng3_1b650(short *, int, int, long);
extern void eng3_19a30(void);
extern Actor *g_eng3_actor_431d4;
extern unsigned char g_eng3_script_334a8[], g_eng3_script_334d0[];
extern unsigned char g_eng3_script_335b0[], g_eng3_script_33418[], g_eng3_script_33448[];
extern unsigned char g_eng3_script_33610[], g_eng3_script_336e0[];
extern unsigned char g_eng3_script_33100[], g_eng3_script_331b0[];
extern Actor *g_eng3_actor_3fb0c;
extern int eng3_01600(Actor *);
extern unsigned char g_eng3_script_33710[];
extern unsigned int random_limit(int);

void eng3_023a0(Actor *o)
{
    if (o->state == 0)
        o->state = 8;
    o->unknown_116 = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    select_frame(o, 5);
    o->flags = 0x202;
    attach_actor(o, 0x2017);
    if (o->unknown_116 == 0) {
        o->state = 8;
        o->sprite = 0x55;
    }
    if (o->unknown_116 == 1) {
        o->state = 8;
        o->sprite = 0x55;
    }
    o->ticks = 1;
    o->scale = 0x12c;
    o->unknown_98 = 1;
}

void eng3_02440(Actor *o)
{
    short d[6];
    int surface;

    if (o->unknown_98) {
        play_sound(0x2a16, o);
        o->unknown_98 = 0;
    }
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (d[0] < 0x300 && (o->state & 8)) {
        o->state = 2;
        eng3_1dbc0(o);
    }
    if (o->state & 2) {
        o->render_flags &= ~0x10;
        o->z += 0x80000;
        eng3_1dbc0(o);
    }
    surface = eng3_1b4a0((short)(o->x >> 16), (short)(o->y >> 16), (short)(o->z >> 16), o->profile);
    if (surface == 0x5e)
        o->state = 2;
    if (surface == 0x5d)
        o->state = 4;
    if (o->state & 4)
        remove_actor(o);
    if (eng3_1eff0(o, g_focus)) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 0x400;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    eng3_1eeb0(o);
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_02580(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;

    o->unknown_116 = 0;
    o->unknown_11e = 0;
    o->flags = 2;
    o->unknown_d8 = -4;
    o->unknown_e0 = 0;
    o->ticks = 0;
    o->state = 8;
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->state = 2;
    o->unknown_ac = (o->x >> 23) * 0x800000 + 0x400000;
    o->unknown_cc = (o->z >> 23) * 0x800000 + 0x400000;
    o->x = o->unknown_ac;
    o->z = o->unknown_cc;
    group = acquire_pending_action_group();
    if (group == 0)
        return;
    o->unknown_120 = group;
    c = eng3_1f410(group, o->x, o->y, o->z, 0x4029, o->profile);
    if (c) {
        c->unknown_97 = 1;
        c->flags = 2;
        c->render_flags |= 0x10000040;
        c->sprite = 1;
        c->unknown_e0 = 1;
        c->ticks = 1;
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
        c->unknown_d8 = 0;
        c->unknown_95 = 0;
    }
    c = eng3_1f410(group, o->x, o->y - 0x800000, o->z, 0x4029, o->profile);
    if (c) {
        c->flags = 2;
        c->render_flags |= 0x10000040;
        c->sprite = 0xb;
        c->unknown_97 = 1;
        c->unknown_e0 = 0;
        c->ticks = 1;
        c->unknown_94 = 0;
        c->unknown_96 = 0;
        c->unknown_bc = 0xc00000;
        c->unknown_ac = 0xc00000;
        c->unknown_cc = 0;
        c->unknown_b8 = 0x900;
        c->unknown_c0 = 0x30000;
        c->unknown_c4 = 0;
        c->unknown_b0 = 0;
        c->unknown_b4 = 0;
        c->unknown_d8 = 1;
        c->unknown_95 = 0;
    }
    c = eng3_1f410(group, o->x, o->y - 0x1000000, o->z, 0x4029, o->profile);
    if (c) {
        c->unknown_e0 = 0;
        c->ticks = 0;
        c->render_flags |= 0x10000040;
        c->flags = 2;
        c->sprite = 2;
        c->unknown_97 = 1;
        c->unknown_94 = 0;
        c->unknown_96 = 0;
        c->unknown_bc = 0xc00000;
        c->unknown_ac = 0xc00000;
        c->unknown_cc = 0;
        c->unknown_b8 = 0x900;
        c->unknown_c0 = 0x30000;
        c->unknown_c4 = 0;
        c->unknown_b0 = 0;
        c->unknown_b4 = 0;
        c->unknown_d8 = 1;
        c->unknown_95 = 0;
    }
    c = eng3_1f410(group, o->x, o->y - 0x11b0000, o->z, 0x4029, o->profile);
    if (c == 0)
        return;
    c->flags = 2;
    c->render_flags |= 0x10000040;
    c->sprite = 0x28;
    if (o->unknown_dc)
        c->sprite = 0x33;
    c->unknown_97 = 1;
    c->unknown_e0 = 0;
    c->ticks = 0;
    c->unknown_94 = 0;
    c->unknown_96 = 0xff;
    c->unknown_bc = 0x1200000;
    c->unknown_ac = 0x1200000;
    c->unknown_cc = 0x200000;
    c->unknown_c8 = 0;
    c->unknown_b8 = 0x2300;
    c->unknown_c0 = 0x30000;
    c->unknown_c4 = 0;
    c->unknown_d0 = 0;
    c->unknown_b4 = 0;
    c->unknown_d8 = 1;
    c->unknown_95 = 0;
}

int eng3_028f0(Actor *o, Actor *c)
{
    long v[4][4]; /* four-long vectors, three used */
    long ox, oy, oz;
    int i, angle, pitch, limit;
    long sa, ca, sp, cp;
    long t, rz, ry, rx;
    long vx, vy, vz;

    i = o->unknown_e0;
    ox = o->x;
    oy = o->y;
    oz = o->z;
    angle = o->unknown_116;
    c->unknown_74 = angle >> 2;
    v[0][0] = 0;
    v[0][1] = 0;
    v[0][2] = 0;
    v[1][0] = 0;
    v[1][1] = -0x80;
    v[1][2] = 0;
    v[2][0] = 0;
    v[2][1] = -0x100;
    v[2][2] = 0;
    v[3][0] = 0;
    v[3][1] = -0x10b;
    v[3][2] = 0;
    pitch = angle;
    limit = 0x400;
    if (o->unknown_dc == 2) {
        pitch = 0;
        limit = 0x400;
    }
    if (o->unknown_dc == 1) {
        pitch = 0;
        limit = 0x400;
    }
    if (o->unknown_dc == 3) {
        pitch = -pitch;
        limit = 0x258;
    }
    if (o->unknown_dc == 0)
        limit = 0x258;
    c->speed = pitch >> 2;
    sa = g_eng3_table_407b4[(angle - 0x400) & 0xfff];
    ca = g_eng3_table_407b4[angle & 0xfff];
    vx = v[i][0];
    vy = v[i][1];
    vz = v[i][2];
    t = (vy * ca + vx * sa) >> 12;
    rz = (vx * ca - vy * sa) >> 12;
    sp = g_eng3_table_407b4[(pitch - 0x400) & 0xfff];
    cp = g_eng3_table_407b4[pitch & 0xfff];
    ry = (t * cp - vz * sp) >> 12;
    rx = (t * sp + vz * cp) >> 12;
    c->z = (rz << 16) + oz;
    c->y = (ry << 16) + oy;
    c->x = (rx << 16) + ox;
    if (++o->unknown_e0 >= 4)
        o->unknown_e0 = 0;
    return o->unknown_116 >= limit;
}

void eng3_02ac0(Actor *o)
{
    PendingActionGroup *group;
    Actor *c, *created;
    long focus[4], position[4]; /* 16-byte frame slots: four-long vectors, three used */
    short d[6];
    int i, j, still;

    group = o->unknown_120;
    focus[0] = g_focus->x;
    focus[1] = g_focus->y;
    focus[2] = g_focus->z;
    if (group == 0)
        goto done;
    if (g_eng3_actor_3fb0c->unknown_11e != 1 && o->ticks == 0)
        goto done;
    o->ticks = 1;
    for (i = 0; i < group->count;) {
        c = group->entries[++i];
        vec_diff_fixed_1420((const int *)c, (const int *)g_focus, d);
        position[0] = c->x;
        position[1] = c->y;
        position[2] = c->z;
        if (within_radius_1140((const int *)position, (const int *)focus, 0x80)) {
            g_focus->unknown_116 = 2;
            g_focus->unknown_110 = o;
            if (g_eng3_mode_41cb4 != 5)
                eng3_19200();
        }
        if (o->state != 0x20 && o->state != 8 && o->state != 0x10) {
            if (d[0] < (o->unknown_dc + 7) << 7)
                o->state = 4;
            if (o->state & 4) {
                if (o->unknown_126 == 0 && c->sprite == 0x33) {
                    play_sound(0x6204, c);
                    o->unknown_126 = 1;
                }
                o->unknown_116 = (o->unknown_116 + 6) & 0xfff;
                if (o->unknown_116 >= 0x190 && c->unknown_d8)
                    c->unknown_d8 = 0;
                if (eng3_028f0(o, c))
                    o->state = 0x20;
            }
        } else if (o->state & 0x20) {
            still = eng3_01600(c);
            if (still == 0)
                c->unknown_74--;
            if (c->unknown_9a && still == 0) {
                if (c->ticks != 1) {
                    if (i == 4) {
                        created = create_eng3_kind_actor(c->x - 0x800000, c->y, c->z + 0x600000, 0x2016);
                        if (created)
                            g_eng3_host_40bb4->method_b0(created);
                    }
                    c->ticks = 1;
                }
                if (c->unknown_e0 != 1) {
                    if (c->sprite == 0x28 || c->sprite == 0x33)
                        play_sound(0xe205, c);
                    for (j = 0; j < 6; j++) {
                        created = create_eng3_middle_actor(c->x, c->y - (random_limit(0x20) << 16),
                                                           c->z + (random_limit(0x80) << 16), 0x4003);
                        if (created) {
                            g_eng3_middle_actions_40b98[3].init(created);
                            created->unknown_ac = created->unknown_bc = random_limit(0x300000) + 0x300000;
                            created->unknown_cc = created->unknown_ac >> 1;
                            if (j < 1)
                                created->unknown_d8 = 1;
                            created->unknown_4a = 0x80 - random_limit(0x40) + random_limit(0x40);
                            if (j == 0 || j == 4 || j == 8 || j == 12) {
                                created->unknown_96 = 0;
                                created->unknown_94 = 0xff;
                            }
                            if (j == 1 || j == 5 || j == 9 || j == 13) {
                                created->unknown_94 = 0;
                                created->unknown_96 = 0;
                            }
                            if (j == 2 || j == 6 || j == 10 || j == 14) {
                                created->unknown_94 = 0xff;
                                created->unknown_96 = 0xff;
                            }
                            if (j == 3 || j == 7 || j == 11 || j == 15) {
                                created->unknown_94 = 0;
                                created->unknown_96 = 0xff;
                            }
                        }
                    }
                    c->unknown_e0 = 1;
                }
            }
            if (c->x < g_focus->x - 0x2580000) {
                remove_counted_value(group, c);
                remove_actor_variant(c);
            }
        }
    }
done:
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_02eb0(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;
    int i;

    if (o->state == 0)
        o->state = 8;
    o->sprite = 0;
    o->flags = 2;
    o->unknown_116 = 0;
    o->ticks = 1;
    group = acquire_pending_action_group();
    if (group == 0)
        return;
    o->unknown_120 = group;
    for (i = 0; i < 3; i++) {
        c = eng3_1f410(group, o->x, o->y, o->z, 0x4030, o->profile);
        if (c) {
            c->ticks = 1;
            c->flags = 2;
            c->state = 2;
            c->unknown_dc = 0;
            c->unknown_d8 = 1;
            c->unknown_94 = 0xff;
            c->unknown_96 = 0;
            select_frame(o, 0);
            c->script = o->script;
            if (i == 0) {
                o->unknown_118 = c;
                eng3_1eeb0(c);
            }
            if (i == 1) {
                c->z += 0x100000;
                select_frame(o, i);
                c->script = o->script;
                c->render_flags = 0x80000004;
            }
            if (i == 2) {
                c->z -= 0x100000;
                c->render_flags = 0x80000014;
                select_frame(o, i);
                c->script = o->script;
            }
            c->unknown_ac = 0x70000;
            c->unknown_b0 = 0x3000;
            c->unknown_a8 = 0;
            c->unknown_c8 = 0;
            c->unknown_d0 = 0;
            c->unknown_d4 = 0x6000;
            c->unknown_cc = 0x90000;
            c->unknown_e0 = 1;
        }
    }
}

void eng3_03020(Actor *o)
{
    PendingActionGroup *group;
    Actor *c, *head;
    short d[6];
    int i;

    group = o->unknown_120;
    if (group) {
        for (i = 0; i < group->count;) {
            c = group->entries[++i];
            if (c->unknown_e0 == 1 && c->script == g_eng3_script_33710) {
                play_sound(0x2a16, c);
                c->unknown_e0 = 0;
            }
            eng3_1dbc0(c);
            vec_diff_fixed_1420((const int *)c, (const int *)g_focus, d);
            if (i == 1 && eng3_1eff0(c, g_focus)) {
                g_focus->unknown_110 = c;
                g_focus->unknown_116 = 0x400;
                if (g_eng3_mode_41cb4 != 5)
                    eng3_19200();
            }
            c->unknown_d0 = 0;
            if (c->state & 2)
                c->unknown_96 = 0xff;
            if (c->state & 4)
                c->unknown_96 = 0;
            if (i == 1) {
                dispatch_primary(c);
                eng3_1eeb0(c);
                advance_vertical_motion(c);
                advance_planar_motion(c);
            }
            if (i == 2) {
                if (c->unknown_dc)
                    c->sprite = 0;
                head = o->unknown_118;
                c->x = head->x;
                c->y = head->y;
                c->z = head->z + 0x200000;
            }
            if (i == 3) {
                if (c->unknown_dc)
                    c->sprite = 0;
                head = o->unknown_118;
                c->x = head->x;
                c->y = head->y;
                c->z = head->z - 0x200000;
            }
            if (d[0] < -0x20) {
                if (c->unknown_d8) {
                    c->render_flags = 0x80000005;
                    c->unknown_4a = 0x40;
                    c->unknown_d8 = 0;
                    if (c->script == g_eng3_script_33710) {
                        if (random_limit(2))
                            play_sound(0x6217, c);
                        else
                            play_sound(0x6218, c);
                    }
                }
                c->unknown_4a -= 4;
            }
        }
    }
    if (eng3_1b920(o, 0))
        remove_actor(o);
}

void eng3_03200(Actor *o)
{
    o->unknown_98 = 0x50;
    o->unknown_cc = 0x50;
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    o->mode = 6;
    o->unknown_3e = 0x7fa;
    o->sprite = 0;
    o->ticks = 0;
    o->unknown_e0 = 0;
    o->unknown_11e = 0;
    o->unknown_dc = 0;
    o->unknown_bc = 0;
    o->unknown_ac = 1;
    o->unknown_d8 = 1;
    advance_frame(o);
    o->sprite = 0x2c;
    select_frame(g_eng3_actor_3fb0c, 0);
}

void eng3_03290(void)
{
    Actor *a;

    a = eng3_1cb00(0xa20000, 0x1c60000, 0, 0x3c);
    if (a) {
        g_eng3_game_40b8c->method_2d0(a);
        a->profile = g_focus->profile;
    }
    g_eng3_actor_3fb0c = eng3_1cb00(0xa20000, 0x1c60000, 0, 0x29);
    if (g_eng3_actor_3fb0c) {
        g_eng3_game_40b8c->method_1ec(g_eng3_actor_3fb0c);
        g_eng3_actor_3fb0c->profile = g_focus->profile;
        g_eng3_actor_3fb0c->x = g_focus->x + 0x6800000;
        g_eng3_actor_3fb0c->state = 0x42;
        select_frame(g_eng3_actor_3fb0c, 4);
    }
}

void eng3_03340(Actor *o)
{
    o->flags = 0x202;
    o->render_flags |= 0x10000040;
    o->unknown_94 = 0xff;
    if (o->state == 0)
        o->state = 8;
    o->state = 8;
    o->unknown_116 = 0;
    o->unknown_11e = 0x10;
    o->unknown_95 = 0;
    o->unknown_bc = 0xff00000;
    o->unknown_ac = 0;
    o->unknown_cc = 0;
    o->unknown_b8 = 0;
    o->unknown_c0 = 0xf000;
    o->unknown_c4 = 0;
    o->unknown_d8 = 0x200000;
    o->unknown_dc = 0x10;
    o->ticks = 1;
    o->scale = 0x200;
    o->unknown_9a = 0;
    o->sprite = 0x21;
    attach_actor(o, 0x2002);
}

void eng3_033f0(Actor *o)
{
    short d[6];
    Actor *created;
    int i;

    o->unknown_9a = 0;
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->state & 8) {
        o->unknown_98 = eng3_01050(g_focus->x, g_focus->y, g_focus->z, o->x, o->y, o->z);
        o->state = 2;
    }
    if (d[0] < 0x578) {
        if (d[2] > -0x36 && d[2] < 0x36 && d[0] > -0x80 && d[0] < 0x80) {
            g_focus->unknown_116 = 2;
            g_focus->unknown_110 = o;
            if (g_eng3_mode_41cb4 != 5)
                eng3_19200();
        }
        if (eng3_01600(o)) {
            o->unknown_d8 = 0;
            o->unknown_dc = 0;
            o->unknown_9a = 0;
        }
        if (o->unknown_9a) {
            o->unknown_d8 >>= 3;
            if (o->ticks == 1) {
                play_sound(0xe206, o);
                for (i = 0; i < 4; i++) {
                    created = create_eng3_middle_actor(o->x, o->y + random_limit(0x200000),
                                                       o->z + random_limit(0x200000), 0x4002);
                    if (created) {
                        g_eng3_middle_actions_40b98[2].init(created);
                        created->scale += 0x80;
                        if (i == 0) {
                            created->unknown_94 = 0xff;
                            created->unknown_96 = 0;
                        }
                        if (i == 1) {
                            created->unknown_94 = 0;
                            created->unknown_96 = 0;
                        }
                        if (i == 2) {
                            created->unknown_94 = 0xff;
                            created->unknown_96 = 0xff;
                        }
                        if (i == 3) {
                            created->unknown_94 = 0;
                            created->unknown_96 = 0xff;
                            o->ticks = 0;
                        }
                    }
                }
            }
        }
        o->x -= o->unknown_d8;
        o->step += o->unknown_dc;
    } else {
        o->x -= o->unknown_d8;
    }
    o->speed += o->unknown_dc;
    o->step += o->unknown_dc;
    o->unknown_74 += o->unknown_dc;
    o->z = 0x400000;
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_03610(void)
{
    long dx;
    int i, phase;

    dx = g_eng3_3fb94 - g_eng3_3fb98;
    if (dx < -0x180)
        dx = 0;
    if (g_eng3_actor_3fb0c->x >= 0x7f800000) {
        g_eng3_actor_3fb0c->profile = g_focus->profile + 1;
        g_eng3_actor_3fb0c->x = 0;
    }
    g_eng3_actor_3fb0c->x += dx << 16;
    if (g_eng3_actor_3fb0c->state == 0x41) {
        g_eng3_actor_3fb0c->unknown_ac = 1;
        if (g_eng3_actor_3fb0c->script == g_eng3_script_33100 || g_eng3_actor_3fb0c->script == g_eng3_script_331b0)
            g_eng3_actor_3fb0c->category = 7;
        else
            g_eng3_actor_3fb0c->category = 5;
        eng3_1dbc0(g_eng3_actor_3fb0c);
        if (g_eng3_actor_3fb0c->unknown_d8 == 0) {
            if (g_eng3_actor_3fb0c->sprite == 0x36)
                g_eng3_actor_3fb0c->unknown_11e = 1;
            if (g_eng3_actor_3fb0c->sprite == 0x3a) {
                g_eng3_actor_3fb0c->unknown_11e = 2;
                g_eng3_actor_3fb0c->unknown_d8 = 1;
                select_frame(g_eng3_actor_3fb0c, 1);
                g_eng3_actor_3fb0c->state = 0x45;
            }
        }
        if (g_eng3_actor_3fb0c->unknown_d8 == 1) {
            if (g_eng3_actor_3fb0c->sprite == 0x5b)
                g_eng3_actor_3fb0c->unknown_11e = 1;
            if (g_eng3_actor_3fb0c->sprite == 0x5f) {
                g_eng3_actor_3fb0c->unknown_11e = 2;
                g_eng3_actor_3fb0c->unknown_d8 = 0;
                select_frame(g_eng3_actor_3fb0c, 1);
                g_eng3_actor_3fb0c->state = 0x45;
            }
        }
    }
    if (g_eng3_actor_3fb0c->unknown_11e != 1 && g_eng3_actor_3fb0c->unknown_11e != 2) {
        g_eng3_actor_3fb0c->unknown_cc = 0x50;
        g_eng3_405fc = 0;
    } else if (g_eng3_actor_3fb0c->unknown_cc) {
        for (i = 0; i < 3; i++) {
            g_eng3_405fc = g_eng3_actor_3fb0c->unknown_e0;
            g_eng3_actor_3fb0c->unknown_e0 += 0x20000;
            if (g_eng3_actor_3fb0c->unknown_e0 > 0x80000)
                g_eng3_actor_3fb0c->unknown_e0 = -0x80000;
        }
        g_eng3_actor_3fb0c->unknown_cc--;
    }
    if (g_eng3_actor_3fb0c->state == 0x42 || g_eng3_actor_3fb0c->state == 0x45) {
        g_eng3_actor_3fb0c->unknown_ac = 1;
        if (g_eng3_actor_3fb0c->state == 0x42)
            g_eng3_actor_3fb0c->unknown_11e = 0;
        if (g_eng3_actor_3fb0c->script == g_eng3_script_33100 || g_eng3_actor_3fb0c->script == g_eng3_script_331b0)
            g_eng3_actor_3fb0c->category = 7;
        else
            g_eng3_actor_3fb0c->category = 5;
        eng3_1dbc0(g_eng3_actor_3fb0c);
        if (g_eng3_actor_3fb0c->sprite == 0xf)
            select_frame(g_eng3_actor_3fb0c, 4);
        if (g_eng3_actor_3fb0c->sprite == 0x21)
            select_frame(g_eng3_actor_3fb0c, 4);
        if (g_eng3_actor_3fb0c->sprite == 0x94)
            select_frame(g_eng3_actor_3fb0c, 4);
        if (g_eng3_actor_3fb0c->sprite == 0x72)
            select_frame(g_eng3_actor_3fb0c, 4);
        if (g_eng3_actor_3fb0c->sprite == 0x2b)
            select_frame(g_eng3_actor_3fb0c, 4);
        if (g_eng3_actor_3fb0c->sprite == 0x4d)
            select_frame(g_eng3_actor_3fb0c, 4);
        if (g_eng3_actor_3fb0c->sprite == 0xa7)
            select_frame(g_eng3_actor_3fb0c, 4);
        if (g_eng3_actor_3fb0c->sprite == 0x36)
            select_frame(g_eng3_actor_3fb0c, 4);
        if (g_eng3_actor_3fb0c->sprite == 0xc) {
            phase = g_eng3_3f218;
            if (++g_eng3_3f218 >= 3)
                g_eng3_3f218 = 0;
            g_eng3_actor_3fb0c->unknown_98++;
            if (phase == 0)
                select_frame(g_eng3_actor_3fb0c, 3);
            if (phase == 1)
                select_frame(g_eng3_actor_3fb0c, 6);
            if (phase == 2)
                select_frame(g_eng3_actor_3fb0c, 7);
        }
    }
    if (g_eng3_actor_3fb0c->state == 0x44) {
        if (g_eng3_actor_3fb0c->script == g_eng3_script_33100 || g_eng3_actor_3fb0c->script == g_eng3_script_331b0)
            g_eng3_actor_3fb0c->category = 7;
        else
            g_eng3_actor_3fb0c->category = 5;
        eng3_1dbc0(g_eng3_actor_3fb0c);
        if (g_eng3_actor_3fb0c->sprite == 0x20 && g_eng3_actor_3fb14 == 0) {
            g_eng3_actor_3fb14 = eng3_1cb00(g_eng3_actor_3fb0c->x + 0x1000000, g_eng3_actor_3fb0c->y,
                                            g_eng3_actor_3fb0c->z - 0x400000, 0x38);
            g_eng3_game_40b8c->method_2a0(g_eng3_actor_3fb14);
        }
        if (g_eng3_actor_3fb0c->sprite == 0x2b)
            g_eng3_actor_3fb0c->state = 0x42;
    }
    if (g_eng3_actor_3fb0c->state == 0x43) {
        if (g_eng3_actor_3fb0c->script == g_eng3_script_33100 || g_eng3_actor_3fb0c->script == g_eng3_script_331b0)
            g_eng3_actor_3fb0c->category = 7;
        else
            g_eng3_actor_3fb0c->category = 5;
        eng3_1dbc0(g_eng3_actor_3fb0c);
        if (g_eng3_actor_3fb0c->sprite == 0xa1 && g_eng3_actor_3fb0c->unknown_ac) {
            g_eng3_actor_3fb0c->unknown_118 = eng3_1cb00(g_eng3_actor_3fb0c->x, g_eng3_actor_3fb0c->y - 0x1800000,
                                                         g_eng3_actor_3fb0c->z + 0x800000, 0x30);
            g_eng3_game_40b8c->method_240(g_eng3_actor_3fb0c->unknown_118);
            g_eng3_actor_3fb0c->unknown_118->profile = g_eng3_actor_3fb0c->profile;
            g_eng3_actor_3fb0c->unknown_ac = 0;
        }
        if (g_eng3_actor_3fb0c->sprite == 0xa7) {
            select_frame(g_eng3_actor_3fb0c, 1);
            g_eng3_actor_3fb0c->state = 0x42;
        }
    }
    if (g_eng3_mode_41cb4 == 6 && g_eng3_actor_3fb0c->state != 0x47) {
        g_eng3_actor_3fb0c->unknown_ac = 1;
        g_eng3_actor_3fb0c->state = 0x47;
        g_eng3_actor_3fb0c->category = 7;
        select_frame(g_eng3_actor_3fb0c, 1);
    }
    if (g_eng3_actor_3fb0c->state == 0x47) {
        g_eng3_actor_3fb0c->unknown_ac = 1;
        eng3_1dbc0(g_eng3_actor_3fb0c);
    }
    if (g_engine_interface.context_004->unknown_03 == 2 && g_eng3_actor_3fb0c->state != 0x48) {
        g_eng3_actor_3fb0c->unknown_ac = 1;
        g_eng3_actor_3fb0c->state = 0x48;
        g_eng3_actor_3fb0c->category = 7;
        select_frame(g_eng3_actor_3fb0c, 3);
    }
    if (g_eng3_actor_3fb0c->state == 0x48) {
        g_eng3_actor_3fb0c->unknown_ac = 1;
        eng3_1dbc0(g_eng3_actor_3fb0c);
    }
}

void eng3_03bf0(Actor *o)
{
    Eng3Sidecar *spawn;
    PendingActionGroup *group;
    Actor *c;
    int i;

    if (o->state == 0)
        o->state = 8;
    spawn = (Eng3Sidecar *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
    o->unknown_dc = spawn->count;
    if (o->state == 0)
        o->state = 8;
    o->flags = 2;
    o->unknown_116 = 0;
    o->ticks = 0;
    group = acquire_pending_action_group();
    if (group == 0)
        return;
    o->unknown_120 = group;
    for (i = 0; i < 3; i++) {
        c = eng3_1f410(group, spawn->x + i * 0x200000, spawn->y, spawn->z + ((random_limit(0x20) * i) << 16),
                       0x402c, o->profile);
        if (c) {
            c->ticks = 1;
            c->flags = 2;
            c->state = 2;
            attach_actor(c, 0x2002);
            c->unknown_94 = 0xff;
            c->unknown_96 = 0;
            select_frame(o, 10);
            c->script = o->script;
            if (i == 0) {
                select_frame(o, 0xb);
                c->script = o->script;
            }
            if (i == 1) {
                select_frame(o, 0xd);
                c->script = o->script;
            }
            if (i == 2) {
                select_frame(o, 0xe);
                c->script = o->script;
            }
            if (i == 3) {
                select_frame(o, 0xe);
                c->script = o->script;
            }
            c->unknown_ac = c->unknown_bc = 0;
            c->unknown_cc = random_limit(0x40000) + 0x40000;
            if (i == 3)
                c->unknown_cc = 0x50000;
            c->unknown_b0 = 0x2000;
            c->unknown_d0 = random_limit(0xa000) + 0x3000;
            c->unknown_a8 = c->unknown_ac;
            c->unknown_c8 = c->unknown_cc;
        }
    }
}

void eng3_03dc0(Actor *o)
{
    Eng3Sidecar *spawn;
    PendingActionGroup *group;
    Actor *c;
    short d[6];
    int i, surface;

    group = o->unknown_120;
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    spawn = (Eng3Sidecar *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
    if (group == 0)
        return;
    for (i = 0; i < group->count;) {
        c = group->entries[++i];
        if (i == 3)
            c->render_flags |= 0x10;
        eng3_1dbc0(c);
        surface = eng3_1b4a0((short)(c->x >> 16), (short)(c->y >> 16), (short)(c->z >> 16), c->profile);
        vec_diff_fixed_1420((const int *)c, (const int *)g_focus, d);
        if (surface == 0x5e) {
            c->unknown_96 = 0;
            c->state = 2;
        } else if (c->z > spawn->z + 0x3800000) {
            c->z = spawn->z;
        }
        eng3_1eeb0(c);
        advance_vertical_motion(c);
        advance_planar_motion(c);
        if (eng3_1b920(o, 0x100))
            remove_actor(o);
    }
}

void eng3_03ef0(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;
    int column[18] = { 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2 };
    int row[18] = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5 };
    int i;

    if (o->state == 0)
        o->state = 8;
    o->sprite = 0;
    o->flags = 0x40002;
    o->render_flags |= 0x10000040;
    o->unknown_116 = 0;
    o->unknown_11e = 6;
    o->unknown_98 = 1;
    group = acquire_pending_action_group();
    if (group == 0)
        return;
    o->unknown_120 = group;
    for (i = 0; i < 18; i++) {
        c = eng3_1f410(group, o->x - (row[i] << 23), o->y, o->z + (column[i] << 23), 0x402f, o->profile);
        if (c) {
            c->y = 0x1800000;
            c->sprite = g_eng3_sprites_34f98[i];
            c->flags = 2;
            c->render_flags |= 0x10000040;
            c->state = 2;
            c->unknown_b8 = (random_limit(3) + 1) << 16;
            c->unknown_94 = 0xff;
            c->unknown_96 = 0;
            o->unknown_98 = 2;
            c->step = 0;
            c->scale = 0x200;
            o->unknown_e0 = 0;
            c->unknown_d8 = 1;
            c->unknown_dc = random_limit(0x28) + 2;
        }
    }
}

void eng3_04100(Actor *o)
{
    PendingActionGroup *group;
    Actor *c, *created;
    short d[6];
    int i, j;

    group = o->unknown_120;
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (d[0] < 0x400) {
        for (j = 0; j < 3; j++) {
            g_eng3_405fc = o->unknown_e0;
            o->unknown_e0 += 0x20000;
            if (o->unknown_e0 > 0x80000)
                o->unknown_e0 = -0x80000;
        }
    }
    if (group == 0)
        return;
    for (i = 0; i < group->count;) {
        c = group->entries[++i];
        if (d[0] < 0x400) {
            if (o->unknown_98) {
                play_sound(0x213, 0);
                o->unknown_98 = 0;
            }
            if (i != 0x10 && i != 0xd && i != 2 && i != 5) {
                if (--c->unknown_dc <= 0 && c->unknown_d8) {
                    created = create_eng3_kind_actor(c->x, c->y + 0x200000, c->z, 0x2016);
                    if (created)
                        g_eng3_host_40bb4->method_b0(created);
                    c->unknown_d8 = 0;
                }
                c->y += c->unknown_b8;
                c->unknown_4a -= 3;
                if (c->unknown_4a <= 0x32)
                    c->unknown_4a = 0x32;
                if (c->y >= 0x1c00000)
                    c->unknown_b8 = 0;
            }
        }
        if (eng3_1b920(o, 0x100))
            remove_actor(o);
    }
}

void eng3_042a0(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;
    int column[8] = { 0, 1, 0, 1, 0, 1, 0, 1 };
    int layer[8] = { 0, 0, 1, 1, 0, 0, 1, 1 };
    int row[8] = { 0, 0, 0, 0, 1, 1, 1, 1 };
    int i;

    if (o->state == 0)
        o->state = 8;
    o->sprite = 0;
    o->flags = 2;
    o->render_flags |= 0x10000040;
    group = acquire_pending_action_group();
    if (group == 0)
        return;
    o->unknown_120 = group;
    for (i = 0; i < 8; i++) {
        c = eng3_1f410(group, o->x - (row[i] << 23), o->y - (layer[i] << 23), o->z + (column[i] << 23), 0x4031,
                       o->profile);
        if (c) {
            c->sprite = 1;
            c->flags = 2;
            c->render_flags |= 0x10000040;
            c->state = 2;
            c->unknown_b8 = c->unknown_bc = random_limit(0x200000) + 0x200000;
            c->unknown_c0 = random_limit(0x6000) + 0x8fff;
            c->unknown_c4 = random_limit(0x6000) + 0xffff;
            c->unknown_ac = ((eng3_1fac0() & 0xbcf) << 8) + ((eng3_1fac0() & 0xbcf) << 7);
            c->unknown_a8 = c->unknown_ac << 5;
            c->unknown_b4 = random_limit(0x1000) + 0x1000;
            c->unknown_b0 = random_limit(0x1000) + 0x1000;
            c->unknown_cc = ((eng3_1fac0() & 0xfff) << 8) - ((eng3_1fac0() & 0xfff) << 7);
            c->unknown_c8 = c->unknown_cc << 5;
            c->unknown_d0 = random_limit(0x140) - 0x500;
            c->unknown_d4 = random_limit(0x1000) - 0x1000;
            c->unknown_95 = 0;
        }
    }
}

void eng3_044e0(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;
    short d[6];
    int i;

    group = o->unknown_120;
    if (group == 0)
        return;
    for (i = 0; i < group->count;) {
        c = group->entries[++i];
        vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
        if (d[0] < 0x100) {
            c->speed += random_limit(0x18);
            c->unknown_74 += random_limit(0x18);
            c->step += random_limit(0x18);
            if (eng3_01600(c))
                remove_actor_variant(c);
        }
    }
}

void eng3_04580(Actor *o)
{
    Eng3Sidecar *spawn;

    o->flags = 0x400202;
    o->render_flags |= 0x10000040;
    if (o->state == 0)
        o->state = 8;
    spawn = (Eng3Sidecar *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
    o->unknown_dc = random_limit(5);
    o->state = 8;
    o->unknown_98 = 1;
    o->unknown_e0 = 0;
    o->y = spawn->y - 0x1000000;
    o->unknown_116 = 0;
    o->unknown_11e = 0x10;
    o->unknown_95 = 0;
    o->unknown_97 = 2;
    o->unknown_bc = 0xf00000;
    o->unknown_ac = 0;
    o->unknown_cc = 0xf0000;
    o->unknown_b8 = 0;
    o->unknown_c0 = 0x20000;
    o->unknown_c4 = 0xffff;
    o->ticks = 0xa;
    o->unknown_9a = 0;
}

void eng3_04650(Actor *o)
{
    short d[6];
    Actor *created;
    int i, k;

    if (o->state & 8)
        o->sprite = 0;
    else
        o->sprite = g_eng3_sprites_34fe0[o->unknown_dc];
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (d[0] < 0x400 && o->unknown_116 == 0) {
        o->state = 4;
        o->unknown_116 = 1;
    }
    /* d[1] - 0x18 reproduces the original test (likely a typo for d[1] > -0x18) */
    if (d[2] > -0x36 && d[2] < 0x36 && d[0] > -10 && d[0] < 0x60 && d[1] - 0x18 && d[1] < 0x18) {
        g_focus->unknown_116 = 0x200;
        g_focus->unknown_110 = o;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    if (o->state & 4) {
        o->step += o->unknown_11e;
        o->speed += random_limit(o->unknown_11e * 2);
        o->unknown_74 += random_limit(o->unknown_11e * 2);
        if (o->unknown_9a) {
            for (k = 0; k < 6; k += 2)
                g_eng3_405fc = k << 16;
            o->unknown_11e /= 2;
            if (o->ticks != 1) {
                created = create_eng3_kind_actor(o->x - 0x800000, o->y, o->z, 0x2016);
                if (created)
                    g_eng3_host_40bb4->method_b0(created);
                o->ticks = 1;
            }
            if (o->unknown_98 == 1) {
                play_sound(0xe206, o);
                play_sound(0xe207, o);
                for (i = 0; i < 8; i++) {
                    created = create_eng3_middle_actor(o->x, o->y + random_limit(0x200000),
                                                       o->z + random_limit(0x200000), 0x4003);
                    if (created) {
                        g_eng3_middle_actions_40b98[3].init(created);
                        if (i == 0 || i == 4) {
                            created->unknown_94 = 0xff;
                            created->unknown_96 = 0;
                        }
                        if (i == 1 || i == 5) {
                            created->unknown_94 = 0;
                            created->unknown_96 = 0;
                        }
                        if (i == 2 || i == 6) {
                            created->unknown_94 = 0xff;
                            created->unknown_96 = 0xff;
                        }
                        if (i == 3 || i == 7) {
                            created->unknown_94 = 0;
                            created->unknown_96 = 0xff;
                            o->unknown_98 = 0;
                        }
                    }
                }
            }
        }
        if (eng3_01600(o)) {
            o->state = 2;
            g_eng3_405fc = 0;
        }
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_048e0(Actor *o)
{
    int first_random_term;
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->flags = 0x202;
    o->state = 4;
    o->y = 0x1c00000;
    if (o->unknown_dc == 1) {
        o->unknown_b8 = o->unknown_bc = random_limit(0x100000) + 0x200000;
        o->unknown_c0 = random_limit(0x6000) + 0x8fff;
        o->unknown_c4 = random_limit(0x6000) + 0xffff;
        o->unknown_ac = 0;
        o->unknown_a8 = 0;
        o->unknown_b4 = random_limit(0x1000) + 0x1000;
        o->unknown_b0 = random_limit(0x1000) + 0x1000;
        first_random_term = (eng3_1fac0() & 0x14ff) << 8;
        first_random_term -= (eng3_1fac0() & 0x14ff) << 8;
        o->unknown_cc = first_random_term;
        o->unknown_c8 = o->unknown_cc << 5;
        o->unknown_d0 = random_limit(0x7d0) - 0x2000;
        o->unknown_d4 = random_limit(0x1000) - 0x1000;
        o->unknown_95 = 0;
        select_frame(o, 1);
    }
    o->ticks = 1;
    eng3_1eeb0(o);
}

void eng3_04a40(Actor *o)
{
    short d[6];

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    /* d[1] - 0x20 reproduces the original test (likely a typo for d[1] > -0x20) */
    if (o->unknown_dc == 0 && d[2] > -0x20 && d[2] < 0x20 && d[0] > -0x20 && d[0] < 0x20 && d[1] - 0x20
        && d[1] < 0x20) {
        g_focus->unknown_ee = 10;
        g_eng3_3fb20 = 1;
        remove_actor(o);
    }
    if (o->unknown_dc == 1) {
        if (d[2] > -0x20 && d[2] < 0x20 && d[0] > -0x20 && d[0] < 0x20) {
            if (!(o->state & 2))
                g_engine_interface.context_004->unknown_24 += 2;
            o->state = 2;
        }
        if (o->state & 2) {
            if (eng3_01460(o) || o->z > 0x1800000 || o->z < -0x1800000 || o->y > 0x2000000) {
                remove_actor(o);
                return;
            }
        }
    }
    eng3_1dbc0(o);
    if (o->x < g_focus->x - 0x800000)
        remove_actor(o);
}

void eng3_04b60(Actor *o)
{
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    o->mode = 6;
    o->unknown_3e = 0x7da;
    attach_actor(o, 0x2002);
    o->ticks = 0;
    o->unknown_e0 = 0;
    o->unknown_11e = 0;
    o->unknown_dc = 0;
    o->unknown_d8 = 0;
    o->sprite = 0;
    o->x = g_focus->x + 0x6800000;
}

void eng3_04bc0(Actor *o)
{
    long dx;

    dx = g_eng3_3fb94 - g_eng3_3fb98;
    if (dx < -0x80)
        dx = 0;
    if (o->x >= 0x7f800000) {
        o->profile = g_focus->profile + 1;
        o->x = 0;
    }
    o->sprite = 0xf0;
    o->x += dx << 16;
}

void eng3_04c10(Actor *o)
{
    if (o->state == 0)
        o->state = 2;
    o->unknown_116 = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    attach_actor(o, 0x2017);
    o->unknown_e0 = 0;
    o->unknown_126 = 0x20;
    o->unknown_11e = 0;
    o->ticks = 1;
    o->state = 2;
    o->x = g_focus->x + 0x9000000;
    select_frame(o, 0);
}

void eng3_04ca0(Actor *o)
{
    Eng3Sidecar *spawn;

    spawn = (Eng3Sidecar *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
    if (o->unknown_11e >= 0xfff)
        o->unknown_126 += 4;
    o->unknown_11e &= 0xfff;
    o->z = spawn->z - (((g_eng3_table_407b4[(o->unknown_11e - 0x400) & 0xfff] * o->unknown_126) >> 12) << 17);
    o->y = g_focus->y - (((g_eng3_table_407b4[o->unknown_11e & 0xfff] * o->unknown_126) >> 12) << 16)
           - 0x600000;
    o->x -= 0x60000;
    o->unknown_11e += 0x4a;
    eng3_1dbc0(o);
    if (eng3_1eff0(o, g_focus)) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 0x800;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_04dc0(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;
    int column[24] = { 0, 2, 4, 5, 1, 3, 5, 3, 4, 3, 2, 1, 2, 3, 0, 3, 2, 3, 1, 0 };
    int row[24] = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 5, 6, 6, 6, 7, 7, 7, 7 };
    int i;

    if (o->state == 0)
        o->state = 8;
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->sprite = 0;
    o->flags = 2;
    o->unknown_116 = 0;
    o->ticks = 1;
    group = acquire_pending_action_group();
    if (group == 0)
        return;
    o->unknown_120 = group;
    for (i = 0; i < 1; i++) {
        c = eng3_1f410(group, o->x + row[i] * 0x1800000, o->y, o->z + (column[i] << 22), 0x4032, o->profile);
        if (c) {
            c->ticks = 1;
            c->unknown_d8 = i << 8;
            c->flags = 2;
            c->state = 2;
            if (i == 0)
                c->state = 8;
            attach_actor(c, 0x2017);
            c->unknown_97 = 1;
            c->unknown_94 = 0xff;
            c->unknown_96 = 0xff;
            select_frame(o, 0);
            c->script = o->script;
            c->unknown_bc = 0x40000;
            c->unknown_ac = random_limit(0x80000) + 0x90000;
            c->unknown_cc = 0;
            c->unknown_b0 = random_limit(0x4000) + 0x2000;
            c->unknown_d0 = random_limit(0x2000) + 0x2000;
            c->unknown_a8 = 0;
            c->unknown_c8 = 0;
        }
    }
}

void eng3_05070(Actor *o)
{
    Eng3Sidecar *spawn;
    PendingActionGroup *group;
    Actor *c;
    short d[6];
    int i, surface;

    spawn = (Eng3Sidecar *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
    group = o->unknown_120;
    if (group == 0)
        return;
    for (i = 0; i < group->count;) {
        c = group->entries[++i];
        c->unknown_cc = random_limit(0x10000) + 0x10000;
        vec_diff_fixed_1420((const int *)c, (const int *)g_focus, d);
        if (o->unknown_11e-- < 0) {
            if (random_limit(0x100) & 1)
                play_sound(0x6222, c);
            else
                play_sound(0x6223, c);
            o->unknown_11e = 0x64;
        }
        if (c->state & 8) {
            o->unknown_118 = eng3_1cb00(c->x, c->y, c->z, 0x43);
            if (o->unknown_118) {
                g_eng3_game_40b8c->method_324(o->unknown_118);
                o->unknown_118->profile = o->profile;
                c->state = 4;
            }
        }
        if (c->state & 4) {
            o->unknown_118->x = c->x;
            o->unknown_118->y = c->y + 0x600000;
            o->unknown_118->z = c->z;
            if (d[0] < 0x300) {
                o->unknown_118->state = 4;
                play_sound(0x6224, c);
                c->state = 2;
            }
        }
        eng3_1dbc0(c);
        if (eng3_01190(i, group, 1, 0x60))
            c->unknown_cc = random_limit(0x10000) + 0x10000;
        if (d[0] < 0x384) {
            c->y += (random_limit(7) + 7) << 16;
            if (c->y >= 0x1800000)
                c->y = 0x1800000;
        } else {
            c->y = (g_eng3_table_407b4[(c->unknown_d8 - 0x400) & 0xfff] << 10) + spawn->y;
            c->unknown_d8 += 0x80;
        }
        if (d[0] < 0x100) {
            if (c->z < 0)
                c->z -= 0x50000;
            if (c->z >= 0)
                c->z += 0x50000;
            c->y -= 0x120000;
        }
        if (eng3_1eff0(c, g_focus)) {
            g_focus->unknown_110 = c;
            g_focus->unknown_116 = 4;
            if (g_eng3_mode_41cb4 != 5)
                eng3_19200();
        }
        surface = eng3_1b4a0((short)(c->x >> 16), (short)(c->y >> 16), (short)((c->z >> 16) + 0x20), c->profile);
        if (surface == 0x5e) {
            c->unknown_96 = 0;
            c->unknown_cc = random_limit(0x10000) + 0x60000;
        }
        if (surface == 0x5d) {
            c->unknown_96 = 0xff;
            c->unknown_cc = random_limit(0x10000) + 0x60000;
        }
        advance_vertical_motion(c);
        advance_planar_motion(c);
        if (eng3_1b920(o, 0x100))
            remove_actor(o);
    }
}

void eng3_053f0(Actor *o)
{
    Actor *c;
    unsigned int i, pick;

    o->unknown_116++;
    if (o->state & 2) {
        for (i = 0; i < eng3_01010(6); i++) {
            if (++o->unknown_e0 > 0x20)
                return;
            c = create_eng3_middle_actor(g_focus->x + ((eng3_01010(0x180) + 0x680) << 16), 0x1c00000,
                                         (eng3_01010(0x80) - eng3_01010(0x80)) << 16, 0x4033);
            if (c) {
                g_eng3_middle_actions_40b98[0x33].init(c);
                c->unknown_d8 = (long)o;
                o->unknown_98 = o->unknown_126;
                c->unknown_96 = random_limit(2);
                c->profile = o->profile;
                o->unknown_126 = eng3_01010(0xb);
                if (o->unknown_98 == o->unknown_126)
                    o->unknown_126++;
                if (o->unknown_126 > 10)
                    o->unknown_126 = 0;
                if (o->unknown_126 != 5 && o->unknown_126 != 7) {
                    select_frame(o, o->unknown_126);
                } else {
                    select_frame(o, 6);
                    if (o->unknown_126 == 5)
                        c->unknown_1c = 3;
                    if (o->unknown_126 == 7)
                        c->unknown_1c = 5;
                }
                c->script = o->script;
                if (c->script == g_eng3_script_335b0) {
                    pick = eng3_01010(3);
                    if (pick == 0)
                        c->unknown_1c = 0x15;
                    if (pick == 1)
                        c->unknown_1c = 0x18;
                    if (pick == 2)
                        c->unknown_1c = 0x14;
                }
                if (c->script == g_eng3_script_33418) {
                    pick = eng3_01010(2);
                    if (pick == 0)
                        c->unknown_1c = 6;
                    if (pick == 1)
                        c->unknown_1c = 0x16;
                }
                if (c->script == g_eng3_script_33448) {
                    pick = eng3_01010(2);
                    if (pick == 0)
                        c->unknown_1c = 0x1a;
                    if (pick == 1)
                        c->unknown_1c = 7;
                }
                if (c->script == g_eng3_script_33610)
                    c->unknown_1c = 0x1b;
                if (c->script == g_eng3_script_336e0)
                    c->unknown_1c = 0x1d;
            }
        }
        o->state = 4;
    }
    if (o->state & 4) {
        if (o->unknown_11e-- < 0) {
            o->state = 2;
            o->unknown_11e = o->unknown_dc;
        }
    }
    switch (eng3_1ecf0(g_focus->x, g_focus->y, g_focus->z, g_focus->profile)) {
    case 0x6e:
        o->unknown_dc = o->unknown_11e = 0x40;
        break;
    case 0x6f:
        o->unknown_dc = o->unknown_11e = 0x20;
        break;
    case 0x70:
        remove_actor(o);
        break;
    }
}

void eng3_056a0(Actor *o)
{
    if (o) {
        o->flags = 0x202;
        o->render_flags = 0x80000000;
        o->unknown_bc = 0x40000;
        o->unknown_ac = random_limit(0x80000) + 0x90000;
        o->unknown_cc = 0;
        o->unknown_b0 = random_limit(0x4000) + 0x2000;
        o->unknown_d0 = random_limit(0x2000) + 0x2000;
        o->unknown_a8 = 0;
        o->unknown_c8 = 0;
        o->unknown_94 = 0xff;
        o->unknown_dc = 0;
        o->unknown_e0 = 1;
        o->unknown_4a = 0x80;
        o->state = 8;
        eng3_1eeb0(o);
        eng3_15340(o);
    }
}

void eng3_05760(Actor *o)
{
    short d[6];
    short bounds[8];
    int lane;

    lane = 0;
    if (o->profile == 0)
        lane = 2;
    if (o->profile == 1)
        lane = 5;
    if (o->profile == 2)
        lane = 8;
    o->unknown_98++;
    o->unknown_cc = random_limit(0x10000) + 0x10000;
    if (((Actor *)o->unknown_d8)->unknown_116 & 1)
        dispatch_primary(o);
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (d[0] < 0x20 && d[0] > -0x20 && d[2] < 0x20 && d[2] > -0x20 && d[1] < 0x20 && d[1] > -0x20) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 4;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    eng3_1b650(bounds, lane, 0x19, o->x >> 16);
    if (o->z <= bounds[0] << 16) {
        o->unknown_96 = 0;
        o->unknown_cc = random_limit(0x10000) + 0x70000;
    }
    if (o->z >= bounds[2] << 16) {
        o->unknown_96 = 0xff;
        o->unknown_cc = random_limit(0x10000) + 0x70000;
    }
    advance_vertical_motion(o);
    advance_planar_motion(o);
    if (o->y != 0x1c00000)
        o->y = 0x1c00000;
    eng3_1dbc0(o);
    if (o->x < g_focus->x + 0x1000000) {
        if (o->state & 8) {
            if (o->script == g_eng3_script_33418)
                play_sound(0xe21d, o);
            if (o->script == g_eng3_script_334a8)
                play_sound(0xe21e, o);
            if (o->script == g_eng3_script_334d0)
                play_sound(0xe21e, o);
            if (o->script == g_eng3_script_33448)
                play_sound(0xe21b, o);
            if (o->script == g_eng3_script_335b0)
                play_sound(0xe21c, o);
            if (o->script == g_eng3_script_336e0)
                play_sound(0xe215, o);
            if (o->script == g_eng3_script_33610)
                play_sound(0xe21b, o);
        }
        o->state = 4;
    }
    if (eng3_1b920(o, 0x100)) {
        ((Actor *)o->unknown_d8)->unknown_e0--;
        remove_actor_variant(o);
    }
}

void eng3_059d0(Actor *o)
{
    o->sprite = 0;
    g_eng3_actor_431d4 = o;
    g_focus->x = o->x;
    g_focus->z = o->z;
    g_focus->y = o->y;
    if (g_eng3_mode_41cb4 != 6)
        eng3_19a30();
}

void eng3_05a20(Actor *o)
{
    Eng3Sidecar *spawn;

    o->flags = 0x202;
    spawn = (Eng3Sidecar *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
    o->unknown_dc = spawn->count;
    o->render_flags = 0x80000000;
    o->state = 8;
    o->unknown_bc = 0;
    o->unknown_cc = 0;
    o->unknown_ac = 0;
    o->unknown_b0 = random_limit(0x4000) + 0x2000;
    o->unknown_d0 = random_limit(0x2000) + 0x2000;
    o->unknown_a8 = 0;
    o->unknown_c8 = 0;
    o->unknown_11e = random_limit(0x80);
    if (o->unknown_dc == 0) {
        if (spawn->z > 0)
            o->render_flags |= 0x10;
        select_frame(o, 0xd);
    }
    if (o->unknown_dc == 1) {
        o->render_flags |= 0x10;
        if (spawn->z > 0)
            o->render_flags &= ~0x10;
        select_frame(o, 0xc);
    }
    if (o->unknown_dc == 2) {
        if (spawn->z > 0)
            o->render_flags |= 0x10;
        select_frame(o, 0xb);
    }
    eng3_1eeb0(o);
    eng3_15340(o);
}

void eng3_05b40(Actor *o)
{
    Eng3Sidecar *spawn;
    short d[6];
    short bounds[8];
    int surface;

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->state & 8) {
        spawn = (Eng3Sidecar *)(o->owner_context + g_kind_info[o->profile].sidecar_offset);
        if (spawn->z < 0) {
            o->unknown_116 = 0;
            o->unknown_cc = (random_limit(3) + 3) << 16;
            o->unknown_96 = 0;
        } else {
            o->unknown_116 = 1;
            o->unknown_cc = (random_limit(3) + 3) << 16;
            o->unknown_96 = 0xff;
        }
        o->state = 2;
    }
    if (o->state & 2) {
        if (o->unknown_116 == 0) {
            surface = eng3_1b4a0((short)(o->x >> 16), (short)(o->y >> 16),
                                 (short)(o->unknown_11e + (o->z >> 16) + 0x100), o->profile);
            if (surface == 0x5d) {
                o->unknown_96 = 0xff;
                o->state = 4;
            }
        } else {
            surface = eng3_1b4a0((short)(o->x >> 16), (short)(o->y >> 16),
                                 (short)((o->z >> 16) - o->unknown_11e - 0x80), o->profile);
            if (surface == 0x5e) {
                o->unknown_96 = 0;
                o->state = 4;
            }
        }
    }
    if (o->state & 4) {
        if (o->unknown_dc == 0)
            select_frame(o, 0);
        if (o->unknown_dc == 1) {
            select_frame(o, 8);
            o->unknown_1c = 0x1b;
        }
        if (o->unknown_dc == 2)
            select_frame(o, 1);
        o->unknown_bc = 0x40000;
        o->unknown_ac = random_limit(0x80000) + 0x90000;
        o->unknown_cc = 0;
        o->unknown_b0 = random_limit(0x4000) + 0x2000;
        o->unknown_d0 = random_limit(0x2000) + 0x2000;
        o->unknown_a8 = 0;
        o->unknown_c8 = 0;
        o->unknown_94 = 0xff;
        o->state = 0x20;
    }
    if (o->state & 0x20) {
        o->unknown_cc = random_limit(0x10000) + 0x10000;
        if (eng3_1eff0(o, g_focus)) {
            g_focus->unknown_110 = o;
            g_focus->unknown_116 = 4;
            if (g_eng3_mode_41cb4 != 5)
                eng3_19200();
        }
        eng3_1b650(bounds, 0, 0x19, o->x >> 16);
        if (o->z <= bounds[0] << 16) {
            o->unknown_96 = 0;
            o->unknown_cc = random_limit(0x10000) + 0x70000;
        }
        if (o->z >= bounds[2] << 16) {
            o->unknown_96 = 0xff;
            o->unknown_cc = random_limit(0x10000) + 0x70000;
        }
    }
    advance_vertical_motion(o);
    advance_planar_motion(o);
    eng3_1dbc0(o);
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_05e50(Actor *o)
{
    o->render_flags |= 0x10000040;
    o->flags = 0x202;
    o->unknown_94 = 0xff;
    o->unknown_bc = 0xff00000;
    o->unknown_ac = 0;
    o->unknown_cc = 0;
    o->unknown_b8 = 0x40000;
    o->unknown_c0 = 0xf000;
    o->unknown_c4 = 0;
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->state = 8;
    o->sprite = 2;
    o->unknown_74 = 0x100;
    o->unknown_e0 = 0;
}

void eng3_05ee0(Actor *o)
{
    short d[6];
    Actor *created;
    int i;

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (o->state & 4) {
        o->x -= 0x40000;
        o->speed += 0x20;
        if (eng3_01600(o))
            o->state = 0x20;
        if (o->unknown_9a && o->unknown_e0 != 1) {
            play_sound(0xe206, o);
            created = create_eng3_kind_actor(o->x - 0x800000, o->y, o->z, 0x2016);
            if (created)
                g_eng3_host_40bb4->method_b0(created);
            for (i = 0; i < 0x10; i++) {
                created = create_eng3_middle_actor(o->x, o->y + (random_limit(0x20) << 16),
                                                   o->z + (random_limit(0x20) << 16), 0x4002);
                if (created) {
                    g_eng3_middle_actions_40b98[2].init(created);
                    created->unknown_ac = created->unknown_bc = random_limit(0x300000) + 0x300000;
                    created->unknown_cc = created->unknown_ac >> 1;
                    created->unknown_4a = random_limit(0x20) + 0x80;
                    if (i == 0 || i == 4 || i == 8 || i == 12) {
                        created->unknown_94 = 0xff;
                        created->unknown_96 = 0;
                    }
                    if (i == 1 || i == 5 || i == 9 || i == 13) {
                        created->unknown_94 = 0;
                        created->unknown_96 = 0;
                    }
                    if (i == 2 || i == 6 || i == 10 || i == 14) {
                        created->unknown_94 = 0xff;
                        created->unknown_96 = 0xff;
                    }
                    if (i == 3 || i == 7 || i == 11 || i == 15) {
                        created->unknown_94 = 0;
                        created->unknown_96 = 0xff;
                    }
                }
            }
            o->unknown_e0 = 1;
        }
    }
    if (o->state & 0x20) {
        o->x -= 0x40000;
        o->speed += 0x20;
    }
    /* d[1] - 0x20 reproduces the original test (likely a typo for d[1] > -0x20) */
    if (d[2] > -0x40 && d[2] < 0x40 && d[0] > -0x20 && d[0] < 0x40 && d[1] - 0x20 && d[1] < 0x20) {
        g_focus->unknown_116 = 0x200;
        g_focus->unknown_110 = o;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}
