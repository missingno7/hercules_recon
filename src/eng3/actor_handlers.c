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

int within_radius_1140(const int *a, const int *b, int r)
{
    int dx = (b[0] - a[0]) >> 16;
    int dy = (b[1] - a[1]) >> 16;
    int dz = (b[2] - a[2]) >> 16;
    return (unsigned)(dx * dx + dy * dy + dz * dz) < (unsigned)(r * r);
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
