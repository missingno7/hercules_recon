/* ENG3.DLL 0x279a0..0x28390: actor handler family (lane y1 candidate).
 * Actor offsets are provisional shared names, not recovered original types. */
typedef struct Actor Actor;
struct Actor {
    long x, y, z;
    unsigned char unknown_0c[0x2a - 0x0c];
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
    unsigned char unknown_5c[0x74 - 0x5c];
    short unknown_74;
    unsigned char unknown_76[0x88 - 0x76];
    unsigned char unknown_88[0x94 - 0x88];
    unsigned char unknown_94, unknown_95;
    unsigned char unknown_96[0x98 - 0x96];
    short unknown_98, unknown_9a;
    unsigned char unknown_9c[0xa8 - 0x9c];
    long unknown_a8, unknown_ac, unknown_b0, unknown_b4;
    long unknown_b8, unknown_bc, unknown_c0, unknown_c4;
    long unknown_c8, unknown_cc, unknown_d0, unknown_d4;
    long unknown_d8, unknown_dc;
    unsigned char unknown_e0[0x110 - 0xe0];
    Actor *unknown_110;
    unsigned char unknown_114[0x116 - 0x114];
    short unknown_116;
};

typedef struct KindInfo KindInfo;
struct KindInfo {
    unsigned long sidecar_offset;
    unsigned long unknown;
};

typedef struct Eng3Host Eng3Host;
struct Eng3Host {
    unsigned char unknown_00[0xb0];
    void (*method_b0)(Actor *);
};

extern KindInfo g_kind_info[];
extern Actor *g_focus;
extern Eng3Host *g_eng3_host_40bb4;
extern short g_eng3_mode_41cb4;
extern unsigned int random_limit(int);
extern Actor *create_eng3_kind_actor(long, long, long, int);
extern void select_frame(Actor *, int);
extern void remove_actor(Actor *);
extern void eng3_1eeb0(Actor *);
extern int eng3_1dbc0(Actor *);
extern int eng3_01460(Actor *);
extern int eng3_1b920(Actor *, int);
extern int eng3_1f0a0(void *, Actor *);
extern void eng3_19200(void);

void eng3_279a0(Actor *o)
{
    o->flags = 0x202;
    o->render_flags = 0x80000005;
    o->unknown_4a = 0xfe;
    eng3_1eeb0(o);
    select_frame(o, 4);
}

void eng3_279d0(Actor *o)
{
    if (eng3_1dbc0(o))
        remove_actor(o);
}

void eng3_279f0(Actor *o)
{
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->profile = g_focus->profile;
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    o->state = 8;
    o->ticks = 1;
    o->unknown_116 = 0x1e;
    eng3_1eeb0(o);
    o->scale = 0xfa;
    o->sprite = 0xb1;
    o->unknown_74 = 0xc0;
    o->unknown_b8 = o->unknown_bc = random_limit(0x200000) + 0x200000;
    o->unknown_c0 = random_limit(0x10000) + 0x10000;
    o->unknown_c4 = random_limit(0x80) + 0x100;
    o->unknown_ac = o->unknown_bc >> 3;
    o->unknown_a8 = o->unknown_bc >> 1;
    o->unknown_b4 = 0x100 - random_limit(0x20);
    o->unknown_b0 = 0x100 - random_limit(0x20);
    o->unknown_c8 = o->unknown_cc = o->unknown_ac >> 2;
    o->unknown_d0 = random_limit(0x100);
    o->unknown_d4 = random_limit(0x100);
    o->unknown_95 = 0;
    o->ticks = 1;
    select_frame(o, 0);
}

void eng3_27b50(Actor *o)
{
    Actor *child;

    if (!eng3_01460(o)) {
        o->unknown_74 += 0x10;
        if (o->unknown_9a) {
            child = create_eng3_kind_actor(o->x - 0x400000, o->y, o->z, 0x2016);
            if (child)
                g_eng3_host_40bb4->method_b0(child);
        }
        eng3_1dbc0(o);
    }
    if (eng3_1f0a0(g_focus->unknown_88, o)) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 0x33;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_27c10(Actor *o)
{
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->profile = g_focus->profile;
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    o->state = 8;
    o->ticks = 1;
    o->unknown_116 = 0x1e;
    eng3_1eeb0(o);
    o->sprite = 0xb2;
    o->unknown_74 = 0xc0;
    o->unknown_b8 = o->unknown_bc = random_limit(0x200000) + 0x200000;
    o->unknown_c0 = random_limit(0x10000) + 0x10000;
    o->unknown_c4 = random_limit(0x80) + 0x100;
    o->unknown_ac = o->unknown_bc >> 3;
    o->unknown_a8 = o->unknown_bc >> 1;
    o->unknown_b4 = 0x100 - random_limit(0x20);
    o->unknown_b0 = 0x100 - random_limit(0x20);
    o->unknown_c8 = o->unknown_cc = o->unknown_ac >> 2;
    o->unknown_d0 = random_limit(0x100);
    o->unknown_d4 = random_limit(0x100);
    o->unknown_95 = 0;
    o->ticks = 1;
    o->unknown_94 = 0xff;
    select_frame(o, 1);
}

void eng3_27d70(Actor *o)
{
    if (!eng3_01460(o)) {
        o->unknown_74 += 0x10;
        eng3_1dbc0(o);
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_27db0(Actor *o)
{
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->profile = g_focus->profile;
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    o->state = 8;
    o->ticks = 1;
    o->unknown_116 = 0x1e;
    eng3_1eeb0(o);
    o->sprite = 0xb3;
    o->unknown_74 = 0xc0;
    o->unknown_b8 = o->unknown_bc = random_limit(0x200000) + 0x200000;
    o->unknown_c0 = random_limit(0x10000) + 0x10000;
    o->unknown_c4 = random_limit(0x80) + 0x100;
    o->unknown_ac = o->unknown_bc >> 3;
    o->unknown_a8 = o->unknown_bc >> 1;
    o->unknown_b4 = 0x100 - random_limit(0x20);
    o->unknown_b0 = 0x100 - random_limit(0x20);
    o->unknown_c8 = o->unknown_cc = o->unknown_ac >> 2;
    o->unknown_d0 = random_limit(0x100);
    o->unknown_d4 = random_limit(0x100);
    o->unknown_95 = 0;
    o->ticks = 1;
    o->unknown_94 = 0xff;
    select_frame(o, 2);
}

void eng3_27f10(Actor *o)
{
    if (!eng3_01460(o))
        eng3_1dbc0(o);
    if (o->x < g_focus->x - 0x400000)
        remove_actor(o);
}

void eng3_27f50(Actor *o)
{
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->profile = g_focus->profile;
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    o->state = 8;
    o->ticks = 1;
    o->unknown_116 = 0x1e;
    o->scale = 0x200;
    eng3_1eeb0(o);
    o->sprite = 0xb3;
    o->unknown_74 = 0xc0;
    o->unknown_b8 = o->unknown_bc = random_limit(0x200000) + 0x200000;
    o->unknown_c0 = random_limit(0x10000) + 0x10000;
    o->unknown_c4 = random_limit(0x80) + 0x100;
    o->unknown_a8 = o->unknown_ac = o->unknown_bc >> 1;
    o->unknown_b4 = 0x100 - random_limit(0x20);
    o->unknown_b0 = 0x100 - random_limit(0x20);
    o->unknown_c8 = o->unknown_cc = o->unknown_ac >> 2;
    o->unknown_d0 = random_limit(0x100);
    o->unknown_d4 = random_limit(0x100);
    o->unknown_95 = 0;
    o->ticks = 1;
    o->unknown_94 = 0xff;
    select_frame(o, 3);
}

void eng3_280b0(Actor *o)
{
    if (!eng3_01460(o))
        eng3_1dbc0(o);
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_280f0(Actor *o)
{
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->profile = g_focus->profile;
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    o->state = 8;
    o->ticks = 1;
    o->unknown_116 = 0x1e;
    eng3_1eeb0(o);
    o->sprite = 0xb3;
    o->unknown_74 = 0xc0;
    o->unknown_b8 = o->unknown_bc = random_limit(0x200000) + 0x200000;
    o->unknown_c0 = random_limit(0x10000) + 0x10000;
    o->unknown_c4 = random_limit(0x80) + 0x100;
    o->unknown_a8 = o->unknown_ac = o->unknown_bc >> 1;
    o->unknown_b4 = 0x100 - random_limit(0x20);
    o->unknown_b0 = 0x100 - random_limit(0x20);
    o->unknown_c8 = o->unknown_cc = o->unknown_ac >> 2;
    o->unknown_d0 = random_limit(0x100);
    o->unknown_d4 = random_limit(0x100);
    o->unknown_95 = 0;
    o->ticks = 1;
    select_frame(o, 4);
}

void eng3_28240(Actor *o)
{
    o->unknown_dc = o->owner_context[g_kind_info[o->profile].sidecar_offset + 0xc];
    o->profile = g_focus->profile;
    o->flags = 0x202;
    o->render_flags = 0x80000000;
    o->state = 8;
    o->ticks = 1;
    o->unknown_116 = 0x1e;
    o->scale = 0x100;
    o->sprite = 0xd4;
    o->unknown_74 = 0xc0;
    o->unknown_b8 = o->unknown_bc = random_limit(0x200000) + 0x200000;
    o->unknown_c0 = random_limit(0x8000) + 0x8000;
    o->unknown_c4 = random_limit(0x80) + 0x100;
    o->unknown_a8 = o->unknown_ac = o->unknown_bc >> 1;
    o->unknown_b4 = 0x100 - random_limit(0x20);
    o->unknown_b0 = 0x100 - random_limit(0x20);
    o->unknown_c8 = o->unknown_cc = o->unknown_ac >> 2;
    o->unknown_d0 = random_limit(0x100);
    o->unknown_d4 = random_limit(0x100);
    o->unknown_95 = 0;
    o->ticks = 1;
    o->unknown_98 = 1;
}
