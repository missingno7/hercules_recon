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
