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

typedef struct Eng3InterfaceView {
    unsigned char unknown_000[0x14];
    short *sine_table_014;
} Eng3InterfaceView;
extern Eng3InterfaceView g_engine_interface;
int eng3_0dae0(Actor *o, Actor *c)
{
    long base[3];
    long offsets[4][4];
    int angle;
    long dz, dy;

    base[0] = o->z;
    base[1] = o->y;
    offsets[0][0] = 0;
    offsets[0][1] = 0;
    offsets[0][2] = 0;
    offsets[1][0] = 0;
    offsets[1][1] = 0x40;
    offsets[1][2] = 0;
    offsets[2][0] = 0;
    offsets[2][1] = 0x80;
    offsets[2][2] = 0;
    offsets[3][0] = 0;
    offsets[3][1] = 0xc0;
    offsets[3][2] = 0;
    base[2] = o->x;
    angle = o->unknown_116;
    c->unknown_74 = angle >> 2;
    dz = (offsets[o->ticks][0] * g_engine_interface.sine_table_014[angle & 0xfff] - offsets[o->ticks][1] * g_engine_interface.sine_table_014[(angle - 0x400) & 0xfff]) >> 12;
    dy = (offsets[o->ticks][0] * g_engine_interface.sine_table_014[(angle - 0x400) & 0xfff] + offsets[o->ticks][1] * g_engine_interface.sine_table_014[angle & 0xfff]) >> 12;
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

/* 0x0d280..0x0dae0: bracketed after the packed sound unit and before
 * eng3_0dae0, which rotates the four children created by 0d7d0. */
typedef struct Eng3Hooks8 {
    void (*create)(Actor *);
    void (*update)(Actor *);
} Eng3Hooks8;
extern Eng3Hooks8 *g_eng3_middle_actions_40b98;
extern Eng3Hooks8 *g_eng3_host_40bb4;
extern Actor *g_focus;
extern Actor *g_pipeline_actor_7200c;
extern short g_eng3_mode_41cb4;
extern long g_eng3_3fadc;
extern long g_eng3_405fc;
typedef struct KindInfo { unsigned char *owner; long count; } KindInfo;
extern KindInfo g_kind_info[];
typedef struct RelativeMeasure {
    short distance, y, z, direction, unknown_08, unknown_0a;
} RelativeMeasure;
extern unsigned int random_limit(int);
extern Actor *create_eng3_middle_actor(long, long, long, int);
extern Actor *create_eng3_kind_actor(long, long, long, int);
extern void attach_actor(Actor *, int);
extern void measure_relative_vector(Actor *, Actor *, RelativeMeasure *);
extern void vec_diff_fixed_1420(const int *, const int *, short *);
extern void play_sound(unsigned short, Actor *);
extern int eng3_01600(Actor *);
extern void eng3_19200(void);
extern void remove_actor_variant(Actor *);
extern void remove_actor(Actor *);
extern int eng3_1b920(Actor *, unsigned short);
extern PendingActionGroup *acquire_pending_action_group(void);
extern Actor *eng3_1f410(PendingActionGroup *, long, long, long, int, int);

void eng3_0d280(Actor *o)
{
    Actor *c;
    o->flags = 2;
    o->render_flags |= 0x10000040;
    o->state = 8;
    o->unknown_116 = 0;
    o->scale = 0x200;
    o->unknown_bc = random_limit(0x100000) + 0xffffff;
    o->unknown_ac = 0;
    o->unknown_cc = 0;
    o->unknown_b8 = 0;
    o->unknown_c0 = random_limit(0x20000) + 0x20000;
    o->y -= 0x800000;
    o->unknown_e0 = 1;
    o->unknown_98 = 1;
    o->unknown_d8 = 1;
    o->unknown_c4 = 0xffff;
    c = create_eng3_middle_actor(o->x, o->y - 0x800000, o->z, 0x4029);
    c->scale = 0x258;
    if (c) {
        o->unknown_110 = c;
        g_eng3_middle_actions_40b98[0x29].create(c);
        c->profile = o->profile;
    }
    c = create_eng3_middle_actor(o->x, o->y - 0x1000000, o->z, 0x4029);
    if (c) {
        o->unknown_118 = c;
        g_eng3_middle_actions_40b98[0x29].create(c);
        c->profile = o->profile;
    }
    o->unknown_95 = 0;
    o->unknown_9a = 0;
    attach_actor(o, 0x2002);
}

void eng3_0d3d0(int count)
{
    g_eng3_3fadc = count;
    if (count)
        g_eng3_405fc = (random_limit(6) << 16) - (random_limit(6) << 16);
    g_eng3_3fadc--;
}

void eng3_0d410(Actor *o)
{
    RelativeMeasure d;
    int i;
    long z;
    Actor *c;
    o->sprite = 2;
    measure_relative_vector(o, g_focus, &d);
    if (d.distance < 0x4c8 && !o->unknown_116) {
        o->state = 4;
        o->unknown_116 = 1;
    }
    if (o->state & 4) {
        if (o->unknown_98) {
            play_sound(0x609d, o);
            o->unknown_98 = 0;
        }
        o->step += 4;
        if (o->unknown_9a) {
            z = 0;
            if (o->unknown_d8) {
                play_sound(0xe09e, o);
                o->unknown_d8 = 0;
            }
            eng3_0d3d0(2);
            if (o->unknown_e0) {
                for (i = 0; i < 3; i++) {
                    if (i == 0) z = -0x400000;
                    if (i == 1) z = 0;
                    if (i == 2) z = 0x400000;
                    c = create_eng3_kind_actor(o->x - 0xc40000, o->y + 0x800000,
                        o->z + z, 0x2016);
                    if (c) {
                        g_eng3_host_40bb4[0x16].create(c);
                        c->scale = 0x200;
                    }
                }
                o->unknown_e0 = 0;
            }
        }
        if (eng3_01600(o)) o->state = 2;
        o->unknown_110->x = o->x;
        o->unknown_110->y = o->y - 0x400000;
        o->unknown_110->z = o->z;
        o->unknown_118->x = o->x;
        o->unknown_118->y = o->y - 0x800000;
        o->unknown_118->z = o->z;
        vec_diff_fixed_1420((const int *)o, (const int *)g_focus, (short *)&d);
        if (o->profile == g_focus->profile && d.z > -0x80 && d.z < 0x80
            && d.distance > -0x80 && d.distance < 0x80 && d.y > -0xc0 && d.y < 0xc0) {
            g_focus->unknown_116 = 0x21;
            g_focus->unknown_110 = o;
            if (g_eng3_mode_41cb4 != 5) eng3_19200();
        }
        if (o->x < g_focus->x - 0x400000) {
            remove_actor_variant(o->unknown_110);
            remove_actor_variant(o->unknown_118);
            remove_actor(o);
        }
    }
    if (o->state & 2) {
        if (!o->unknown_98) {
            play_sound(0x609c, o);
            o->unknown_98 = 1;
        }
        o->unknown_d8 = 1;
        o->unknown_9a = 0;
        if (o->y <= *(long *)(g_kind_info[o->profile].owner + (long)o->owner_context + 4)) {
            o->state = 4;
            vec_diff_fixed_1420((const int *)o, (const int *)g_focus, (short *)&d);
            if (d.distance < 0) {
                o->state = 8;
                return;
            }
        } else
            o->y -= 0x80000;
        o->unknown_110->x = o->x;
        o->unknown_110->y = o->y - 0x400000;
        o->unknown_110->z = o->z;
        o->unknown_118->x = o->x;
        o->unknown_118->y = o->y - 0x800000;
        o->unknown_118->z = o->z;
        if (o->profile == g_focus->profile && d.z > -0x80 && d.z < 0x80
            && d.distance > -0x80 && d.distance < 0x80 && d.y > -0xc0 && d.y < 0xc0) {
            g_focus->unknown_116 = 0x21;
            g_focus->unknown_110 = o;
            if (g_eng3_mode_41cb4 != 5) eng3_19200();
        }
        if (eng3_1b920(o, 0x100)) {
            remove_actor_variant(o->unknown_110);
            remove_actor(o);
            o->state = 8;
        }
    }
}

void eng3_0d7d0(Actor *o)
{
    PendingActionGroup *group;
    Actor *c;
    o->unknown_116 = 0x800;
    o->unknown_126 = 0x800;
    o->state = 0;
    o->unknown_11e = 0;
    o->unknown_98 = 0;
    o->unknown_dc = 4;
    o->unknown_e0 = 0;
    o->ticks = 0;
    o->unknown_cc = 1;
    o->unknown_ac = 1;
    o->unknown_bc = 1;
    o->flags = 2;
    group = acquire_pending_action_group();
    if (!group) return;
    o->unknown_120 = (Actor *)group;
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
        c->unknown_e0 = 1;
        c->ticks = 4;
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
        c->unknown_e0 = 0;
        c->ticks = 8;
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
        c->unknown_e0 = 0;
        c->ticks = 0xc;
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
        c->unknown_e0 = 0;
        c->ticks = 0x10;
        c->unknown_d8 = 1;
        c->unknown_95 = 0;
        attach_actor(c, 0x2002);
    }
}
