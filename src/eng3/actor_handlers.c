/* ENG3.DLL EXACT functions merged from candidates/leaf-e.
 * Actor offsets are provisional shared names, not recovered original types. */
typedef struct Actor Actor;
struct Actor {
    long x, y, z;
    unsigned char unknown_0c[0x22 - 0x0c];
    unsigned char category;
    unsigned char mode;
    unsigned char unknown_24[0x2a - 0x24];
    short state;
    unsigned char unknown_2c[0x30 - 0x2c];
    short ticks, phase;
    unsigned short sprite;
    unsigned char unknown_36[0x3a - 0x36];
    short scale;
    unsigned char unknown_3c[0x4a - 0x3c];
    unsigned short unknown_4a;
    long parameter;
    unsigned long flags, render_flags;
    unsigned char unknown_58[0x5c - 0x58];
    Actor *attachment;
    unsigned char unknown_60[0x70 - 0x60];
    short speed, step;
    unsigned char unknown_74[0x98 - 0x74];
    short unknown_98;
    unsigned char unknown_9a[0xd8 - 0x9a];
    long unknown_d8, unknown_dc, unknown_e0;
    unsigned char unknown_e4[0x116 - 0xe4];
    short unknown_116;
    unsigned char unknown_118[0x11e - 0x118];
    short unknown_11e;
    unsigned char unknown_120[0x126 - 0x120];
    short unknown_126;
    unsigned char unknown_128[0x12c - 0x128];
    const long *unknown_12c;
};

extern long eng3_1f7b0(unsigned short, unsigned short);

long eng3_01050(long x1, long unused_y1, long z1, long x2, long unused_y2, long z2)
{
    long dx, dz, angle;
    int quadrant;

    quadrant = 0;
    if (z1 > z2) {
        dx = (z1 - z2) >> 16;
        quadrant = 1;
    } else {
        dx = (z2 - z1) >> 16;
    }
    if (x1 > x2) {
        dz = (x1 - x2) >> 16;
    } else {
        dz = (x2 - x1) >> 16;
        quadrant += 2;
    }
    angle = eng3_1f7b0((unsigned short)dx, (unsigned short)dz);
    switch (quadrant) {
    case 0:
        return 0xff - (angle / 90 >> 8);
    case 1:
        return 0x300 + (angle / 90 >> 8);
    case 2:
        return 0x100 + (angle / 90 >> 8);
    case 3:
        return 0x2ff - (angle / 90 >> 8);
    }
    return angle;
}

int within_radius_1140(const int *a, const int *b, int r)
{
    int dx = (b[0] - a[0]) >> 16;
    int dy = (b[1] - a[1]) >> 16;
    int dz = (b[2] - a[2]) >> 16;
    return (unsigned)(dx * dx + dy * dy + dz * dz) < (unsigned)(r * r);
}

/* These real ENG3 interface declarations are needed by the later 0x1870
 * handler. Keep them after the 0x1050/0x1140 functions to preserve their unit
 * context while testing whether the declarations explain 0x1190's tie. */
typedef struct Eng3EngineInterfacePrefix {
    unsigned long signature_000;
    unsigned char *context_004;
} Eng3EngineInterfacePrefix;
extern Eng3EngineInterfacePrefix g_engine_interface;
extern void eng3_03290(void);

typedef union PendingActionGroupView PendingActionGroupView;
union PendingActionGroupView {
    long count;
    Actor *entries[1];
};

extern void vec_diff_fixed_1420(const int *a, const int *b, short *out);

int eng3_01190(int start, PendingActionGroupView *group, int end, int radius)
{
    if (end - start > 0) {
        Actor **other = &group->entries[start + 1];
        int remaining = end - start;
        short difference[6];

        do {
            Actor *a = group->entries[start];
            Actor *b = *other;
            int state;

            vec_diff_fixed_1420((const int *)a, (const int *)b, difference);
            a->unknown_e0 = 0;
            if (difference[2] > -radius && difference[2] < radius &&
                difference[0] > -radius && difference[0] < radius) {
                a->unknown_e0 = 0x18;
                state = *((unsigned char *)a + 0x96);
                if (state == -1) {
                    *((unsigned char *)b + 0x96) = 0xff;
                    *((unsigned char *)a + 0x96) = 0;
                }
                if (state == 0) {
                    *((unsigned char *)b + 0x96) = state;
                    *((unsigned char *)a + 0x96) = 0xff;
                }
            }
            ++other;
        } while (--remaining != 0);
    }

    return 0;
}

void vec_diff_fixed_1420(const int *a, const int *b, short *out)
{
    out[0] = (short)((a[0] - b[0]) >> 16);
    out[1] = (short)((a[1] - b[1]) >> 16);
    out[2] = (short)((a[2] - b[2]) >> 16);
}

void reset_state_53a0(Actor *s)
{
    s->state = 2;
    s->unknown_11e = 8;
    s->unknown_dc = 0x18;
    s->unknown_126 = 0;
    s->unknown_98 = 0;
    s->unknown_d8 = 0;
    s->unknown_e0 = 0;
    s->unknown_116 = 0;
}

void init_defaults_e790(Actor *o)
{
    o->flags = 0x202;
    o->sprite = 0;
    o->state = 8;
}

void init_flags_12190(Actor *o)
{
    o->flags = 0x202;
    o->render_flags = 0x80000000u;
}

void set_field34_121b0(Actor *o)
{
    o->sprite = 0x119;
}

void set_field34_121f0(Actor *o)
{
    o->sprite = 0x12;
}

void init_defaults_13840(Actor *o)
{
    o->flags = 0x202;
    o->state = 8;
}

void setup_flags_145a0(Actor *o)
{
    o->flags = 0x10000;
    o->render_flags |= 0x84;
    o->mode = 5;
    o->unknown_4a = 0;
    o->sprite = 0;
    o->category = 0;
}

void setup_flags_14820(Actor *o)
{
    o->flags = 0x10000;
    o->render_flags |= 0x684;
    o->mode = 5;
    o->unknown_4a = 0;
}

void copy_vec_adjust_15410(Actor *o)
{
    const long *s = o->unknown_12c;
    o->x = s[0];
    o->y = s[1] - 0x40000;
    o->z = s[2];
}

void init_state_27130(Actor *o)
{
    o->flags = 0x40202;
    o->render_flags |= 0x10000000;
    o->state = 2;
    o->unknown_dc = 0x400000;
    o->step = 0x200;
    o->scale = 0x300;
    o->ticks = 0;
    o->unknown_4a = 0xfe;
    o->unknown_126 = 0;
    o->unknown_116 = 1;
}

typedef struct AnimationRecord AnimationRecord;
struct AnimationRecord {
    short x, y, z;
    short rotation_y, rotation_z, rotation_x;
    short duration;
};

typedef struct AnimationActorView AnimationActorView;
struct AnimationActorView {
    long x, y, z;
    unsigned char unknown_0c[0x2e - 0x0c];
    short animation_command_2e;
    unsigned char unknown_30[0x5c - 0x30];
    Actor *attachment_5c;
    unsigned char unknown_60[0x8e - 0x60];
    short rotation_x_8e, rotation_y_90, rotation_z_92;
    unsigned char unknown_94[0xf0 - 0x94];
    const AnimationRecord *animation_frames_f0;
    long animation_index_f4;
    long animation_remaining_f8;
};

void eng3_01870(void)
{
    if (*g_engine_interface.context_004 == 8)
        eng3_03290();
}

/* Seven-word transform records are advanced only after the current duration
 * expires. Sentinel X values carry the two actor commands seen in the stream. */
extern Actor *g_eng3_actor_431d4;

void eng3_01890(AnimationActorView *actor)
{
    const AnimationRecord *frame;
    short delta_x, delta_y, delta_z;

    --actor->animation_remaining_f8;
    if (actor->animation_remaining_f8 < 0) {
        do {
            ++actor->animation_index_f4;
            frame = actor->animation_frames_f0 + actor->animation_index_f4;
            delta_x = frame->x;
            delta_y = frame->y;
            delta_z = frame->z;
            if (delta_x == 0x7fff) {
                int command = delta_y;
                if (command != -25) {
                    if (command == -6)
                        actor->animation_command_2e = 0;
                } else {
                    g_eng3_actor_431d4 = actor->attachment_5c;
                }
            }
        } while (delta_x == 0x7fff);
        actor->animation_remaining_f8 =
            actor->animation_frames_f0[actor->animation_index_f4].duration;
        frame = actor->animation_frames_f0 + actor->animation_index_f4;
    } else {
        frame = actor->animation_frames_f0 + actor->animation_index_f4;
        delta_x = frame->x;
        delta_y = frame->y;
        delta_z = frame->z;
    }

    actor->x += (long)delta_x << 16;
    actor->y += (long)delta_y << 16;
    actor->z += (long)delta_z << 16;
    actor->rotation_x_8e += frame->rotation_x;
    actor->rotation_y_90 += frame->rotation_y;
    actor->rotation_z_92 += frame->rotation_z;
    actor->rotation_y_90 &= 0x0fff;
    actor->rotation_z_92 &= 0x0fff;
}
