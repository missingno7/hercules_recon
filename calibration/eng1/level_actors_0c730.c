/* ENG1.DLL 0xc730..0xe170: level actor handlers continuing calibration/eng1/level_actors_083e0.cpp
   (music trigger, rubble and debris, placement/scroll helpers, two walker enemies).  Lane x13
   scratch, 2026-10-20.  /O2 /Gy; types, names and idioms reused from the 0x83e0 unit.
   Compiled as C: 31/32 masked equal (0xcc30 is equal only as C; as C++, standalone or appended to the
   0x83e0 unit, it keeps a commutative add-destination residue).  0xc940 keeps one instruction-form
   residue (see result.json).  Local views cover only offsets observed in these bodies. */

typedef struct Bounds { short min_x, min_y, min_z, max_x, max_y, max_z; } Bounds;
typedef struct Motion { long velocity, target, rate, unknown; } Motion;

typedef struct Actor Actor;
struct Actor {
    long x, y, z;                       /* 0x00 */
    short screen_x;                     /* 0x0c */
    unsigned char unknown_00e[0x21 - 0x0e];
    unsigned char lane;                 /* 0x21 */
    unsigned char unknown_022;
    unsigned char unknown_023;          /* 0x23 */
    unsigned char unknown_024[0x2a - 0x24];
    unsigned short state;               /* 0x2a */
    unsigned char unknown_02c[2];
    unsigned short kind;                /* 0x2e */
    short frame_ticks;                  /* 0x30 */
    short frame_state;                  /* 0x32 */
    unsigned char unknown_034[4];
    unsigned short profile;             /* 0x38 */
    unsigned short scale;               /* 0x3a */
    unsigned short scale_y;             /* 0x3c */
    unsigned char unknown_03e[0x50 - 0x3e];
    unsigned long flags;                /* 0x50 */
    unsigned long render_flags;         /* 0x54 */
    long record_offset;                 /* 0x58 */
    unsigned char unknown_05c[0x72 - 0x5c];
    short unknown_072;                  /* 0x72 */
    unsigned char unknown_074[0x88 - 0x74];
    Bounds bounds;                      /* 0x88 */
    unsigned char direction_x;          /* 0x94 */
    unsigned char direction_y;          /* 0x95 */
    unsigned char unknown_096[0x9c - 0x96];
    long saved_x, saved_y, saved_z;     /* 0x9c */
    Motion motion_x, motion_y, motion_z;/* 0xa8 */
    long spin_x;                        /* 0xd8 */
    unsigned char unknown_0dc[0xee - 0xdc];
    short hits;                         /* 0xee */
    unsigned char unknown_0f0[0x108 - 0xf0];
    long counter;                       /* 0x108 */
    unsigned char unknown_10c[0x114 - 0x10c];
    short frame_index;                  /* 0x114 */
};

typedef struct KindInfo { unsigned char *owner; long count; } KindInfo;
typedef struct Callback8 { void (*initialize)(Actor *); void *unknown_004; } Callback8;
typedef struct Callback12 { void (*initialize)(Actor *); void *unknown_004, *unknown_008; } Callback12;
typedef struct Debris { short x, y, z; unsigned short kind; } Debris;
typedef struct Placement { long x, dx, dy, dz, unknown_072, scale; } Placement;

typedef struct EngineContext {
    unsigned char unknown_000[2];
    unsigned char unknown_002;
    unsigned char unknown_003[0x0e - 0x03];
    signed char unknown_00e;
    unsigned char unknown_00f[0x24 - 0x0f];
    short unknown_024;
    unsigned char unknown_026[0x38 - 0x26];
    unsigned long tick_038;
    unsigned char unknown_03c[0x62 - 0x3c];
    unsigned char input_062;
} EngineContext;
typedef struct EngineInterfaceHead { unsigned long signature; EngineContext *context; } EngineInterfaceHead;

extern EngineInterfaceHead g_engine_interface;
extern KindInfo g_kind_info[];
extern Actor *g_normal_actor_pool;
extern Actor *g_collision_context;
extern unsigned short *g_configuration_dword_713bc;
extern Callback12 *g_normal_callbacks;
extern Callback8 *g_special_callbacks;
extern unsigned short g_eng1_setup_word_718e0;
extern Debris g_debris_50f60[24];
extern Debris g_debris_51020[24];
extern Debris g_debris_510e0[12];
extern Debris g_debris_51140[10];
extern Placement g_placements_511b0[11];
extern unsigned long eng1_native_cell_06ef10;
extern unsigned long g_eng1_dword_6ef0c;
extern long g_eng1_dword_6ef14;
extern long g_eng1_dword_6ef38;
extern long g_eng1_dword_6ef3c;

extern void dbg_printf(const char *, ...);
extern void eng1_02d80(int, unsigned int, unsigned int);
extern void play_sound(unsigned short, Actor *);
extern void stop_actor_sounds(Actor *);
extern void eng1_131c0(long, long, long, long);
extern long eng1_native_code_target_00266a0(Actor *, int, int);
extern int eng1_26e90(Actor *, int, int);
extern Actor *spawn_normal(long, long, long, int);
extern int remove_actor(Actor *);
extern void remove_actor_preserving_resource(Actor *);
extern Actor *spawn_middle_277e0(long, long, long, unsigned short);
extern int remove_actor_variant(Actor *);
extern void attach_actor(Actor *, int);
extern int eng1_interpret_signed_pairs(Actor *);
extern int eng1_interpret_word_stream(Actor *);
extern void reset_frame_state(Actor *);
extern void advance_frame(Actor *);
extern void select_frame(Actor *, int);
extern int eng1_29e20(Actor *);
extern unsigned long dispatch_local_2a170(long, long, long, int);
extern void eng1_native_code_target_002a290(Actor *);
extern int eng1_2a710(Bounds *, Actor *);
extern int bounds_overlap(Bounds *, Bounds *);
extern int actor_bounds_2a880(Actor *, Bounds *, int);
extern int eng1_2a9e0(Actor *, int);
extern int eng1_native_code_target_002aa30(Actor *, int, int);
extern unsigned int random_limit(int);

/* Walk one step along x in the facing direction; step back when the leading edge hits a wall cell. */
static __inline int walk_x_blocked(Actor *p, Bounds *b)
{
    p->saved_x = p->x;
    if (!p->direction_x) {
        p->x += p->motion_x.velocity;
        if (actor_bounds_2a880(p, b, 1) &&
            g_configuration_dword_713bc[(unsigned short)dispatch_local_2a170(b->max_x << 16, p->y, p->z, p->profile)] & 2) {
            p->x -= p->motion_x.velocity;
            return 1;
        }
    } else {
        p->x -= p->motion_x.velocity;
        if (actor_bounds_2a880(p, b, 1) &&
            g_configuration_dword_713bc[(unsigned short)dispatch_local_2a170(b->min_x << 16, p->y, p->z, p->profile)] & 2) {
            p->x += p->motion_x.velocity;
            return 1;
        }
    }
    return 0;
}

/* ---- x13 functions ---- */

void eng1_0c730(Actor *p)
{
    Actor *q;

    p->state = 0x52;
    p->counter = 0x3c;
    q = spawn_normal(g_normal_actor_pool->x + 0x1400000, p->y, p->z, 0x2f);
    if (q)
        g_normal_callbacks[47].initialize(q);
}

void eng1_native_code_target_00c780(Actor *p)
{
    if (!eng1_native_cell_06ef10 && p->x >= g_normal_actor_pool->x) {
        eng1_native_cell_06ef10 = 0xffffffff;
        if (!g_eng1_dword_6ef14) {
            p->state = 0x37;
            return;
        }
        g_eng1_dword_6ef14 = 0x21;
        eng1_0c730(p);
    }
    remove_actor(p);
}

void eng1_native_code_target_00c7e0(Actor *p)
{
    long dx;

    dbg_printf("BensMusicControl here\n");
    dx = p->x - g_normal_actor_pool->x;
    switch (p->state) {
    case 0x37:
        if (dx > 0x2000000) {
            remove_actor(p);
            eng1_native_cell_06ef10 = 0;
            return;
        }
        if (!g_eng1_dword_6ef38) {
            g_eng1_dword_6ef38 = 1;
            p->state = 0x51;
        }
        break;
    case 0x51:
        if (g_eng1_dword_6ef38 == 5) {
            if (dx > 0x2000000) {
                g_eng1_dword_6ef3c = 1;
                remove_actor(p);
                eng1_native_cell_06ef10 = 0;
                return;
            }
            if (g_eng1_dword_6ef14) {
                g_eng1_dword_6ef38 = 6;
                eng1_0c730(p);
                g_engine_interface.context->unknown_002 = 5;
            }
        }
        break;
    case 0x52:
        if (p->counter-- <= 0) {
            play_sound(0x100, 0);
            remove_actor(p);
            eng1_native_cell_06ef10 = 0;
            return;
        }
        break;
    }
}

void eng1_native_code_target_00c8c0(Actor *p)
{
    p->flags = 2;
    advance_frame(p);
    p->direction_x = 0xff;
    p->render_flags |= 0x10;
    p->z = g_normal_actor_pool->z - 0x140000;
    p->flags |= 0x20000;
    p->render_flags |= 5;
}

void eng1_native_code_target_00c910(Actor *p)
{
    eng1_interpret_signed_pairs(p);
}

void eng1_native_code_target_00c920(Actor *p)
{
    reset_frame_state(p);
    p->state = 1;
}

void eng1_native_code_target_00c940(Actor *p)
{
    Bounds b;
    Actor *q;
    int kind;

    p->motion_y.velocity += 0x8000;
    p->y += p->motion_y.velocity;
    walk_x_blocked(p, &b);
    eng1_interpret_word_stream(p);
    if (eng1_29e20(p) || p->y > g_eng1_setup_word_718e0 << 16) {
        if (g_normal_actor_pool->y - p->y > 0x400000) {
            remove_actor_variant(p);
            return;
        }
        if (p->state == 0x3e && p->y > g_eng1_setup_word_718e0 << 16) {
            kind = ((signed char)~g_engine_interface.context->tick_038 & 1) | 0x48;
            q = spawn_normal(p->x, g_eng1_setup_word_718e0 << 16, p->z, kind);
            if (q) {
                g_normal_callbacks[kind & 0x1ff].initialize(q);
                q->scale = 0x80;
            }
            remove_actor_variant(p);
            return;
        }
        p->motion_y.velocity /= -3;
        if (p->motion_y.velocity < 0x2800 && p->motion_y.velocity > -0x2800) {
            p->motion_x.velocity = 0;
            p->frame_ticks = 999;
            if (p->state != 2) {
                remove_actor_variant(p);
                return;
            }
            if (random_limit(0x800) < 0x100) {
                p->scale >>= 1;
                if (p->scale < 0x20)
                    remove_actor_variant(p);
            }
        }
    }
    if (eng1_native_code_target_002aa30(p, 100, 200))
        remove_actor_variant(p);
}

void eng1_native_code_target_00cb80(Actor *p)
{
    reset_frame_state(p);
    p->motion_y.velocity = random_limit(0x200) << 8;
    p->motion_y.rate = 0x800;
    p->motion_x.velocity = random_limit(0x200) << 8;
    p->scale = random_limit(0x80) + 0x80;
    p->state = 0x1c;
    p->frame_state = random_limit(0x0f);
    if (random_limit(0x8000) < 0x4000)
        p->direction_x = 0;
    else
        p->direction_x = 0xff;
    p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
}

void eng1_native_code_target_00cc30(Actor *p)
{
    Bounds b;
    unsigned int r;

    eng1_interpret_word_stream(p);
    switch (p->state) {
    case 1:
        p->frame_ticks = 999;
        break;
    case 0x1c:
        p->motion_y.velocity += 0x600;
        p->y += p->motion_y.velocity;
        walk_x_blocked(p, &b);
        if (eng1_29e20(p)) {
            if (g_normal_actor_pool->y - p->y > 0x400000) {
                remove_actor_variant(p);
                return;
            }
            p->state = 0x23;
        }
        break;
    case 0x23:
        r = random_limit(0x8000);
        if (p->frame_state != 2 && p->frame_state != 0x0b) {
            if (p->frame_state == 3 || p->frame_state == 0x0c) {
                if (!(r & 6)) {
                    remove_actor_variant(p);
                    return;
                }
                p->state = 1;
                p->frame_ticks = 999;
            }
        }
        break;
    }
    if (eng1_native_code_target_002aa30(p, 100, 200))
        remove_actor_variant(p);
}

void eng1_0cdd0(Actor *p)
{
    long x = p->x;
    long y = p->y;
    long z = p->z;
    int i;
    Actor *q;

    for (i = 0; i < 24; i++) {
        q = spawn_middle_277e0((g_debris_50f60[i].x << 16) + x, (g_debris_50f60[i].y << 16) + y,
                               (g_debris_50f60[i].z << 16) + z, g_debris_50f60[i].kind);
        if (q) {
            g_special_callbacks[40].initialize(q);
            q->direction_x = 0;
            q->motion_x.velocity = (random_limit(0xa00) - 0x500) << 8;
            q->motion_y.velocity = (-random_limit(0x600) - 0x200) << 8;
            q->motion_z.velocity = (random_limit(0x1800) - 0xc00) << 8;
            q->state = 2;
        }
    }
}

void eng1_0ceb0(Actor *p)
{
    long x = p->x;
    long y = p->y;
    long z = p->z;
    int i;
    Actor *q;

    for (i = 0; i < 24; i++) {
        q = spawn_middle_277e0((g_debris_51020[i].x << 16) + x, (g_debris_51020[i].y << 16) + y,
                               (g_debris_51020[i].z << 16) + z, g_debris_51020[i].kind);
        if (q) {
            g_special_callbacks[40].initialize(q);
            q->state = 0x3e;
            q->direction_x = 0;
            q->motion_x.velocity = (random_limit(0xa00) - 0x500) << 8;
            q->motion_y.velocity = (-random_limit(0x600) - 0x200) << 8;
            q->motion_z.velocity = (random_limit(0x1800) - 0xc00) << 8;
            attach_actor(q, 0x2024);
        }
    }
}

void eng1_0cfb0(Actor *p)
{
    long x = p->x;
    long y = p->y;
    long z = p->z;
    int i;
    Actor *q;

    play_sound(0xcc, 0);
    for (i = 0; i < 12; i++) {
        q = spawn_middle_277e0((g_debris_510e0[i].x << 16) + x, (g_debris_510e0[i].y << 16) + y,
                               (g_debris_510e0[i].z << 16) + z, g_debris_510e0[i].kind);
        if (q) {
            g_special_callbacks[40].initialize(q);
            q->state = 0x3e;
            q->direction_x = 0;
            q->motion_x.velocity = (random_limit(0xa00) - 0x500) << 8;
            q->motion_y.velocity = (-random_limit(0x600) - 0x200) << 8;
            q->motion_z.velocity = (random_limit(0x1800) - 0xc00) << 8;
            attach_actor(q, 0x2024);
        }
    }
}

void eng1_0d0c0(Actor *p)
{
    long x = p->x;
    long y = p->y;
    long z = p->z;
    int i;
    Actor *q;

    for (i = 0; i < 10; i++) {
        q = spawn_middle_277e0((g_debris_51140[i].x << 16) + x, (g_debris_51140[i].y << 16) + y,
                               (g_debris_51140[i].z << 16) + z, g_debris_51140[i].kind);
        if (q) {
            g_special_callbacks[56].initialize(q);
            q->direction_x = 0;
            q->motion_x.velocity = (random_limit(0x200) - 0x100) << 8;
            q->motion_y.velocity = random_limit(0x200) << 8;
            q->motion_z.velocity = (random_limit(0x1000) - 0x800) << 8;
        }
    }
}

void eng1_native_code_target_00d190(Actor *p)
{
    reset_frame_state(p);
    p->state = 1;
    p->motion_x.velocity = (random_limit(0x600) - 0x300) << 8;
    p->motion_y.velocity = -0x20000 - (random_limit(0x600) << 8);
    p->motion_z.velocity = (random_limit(0x100) - 0x80) << 8;
    attach_actor(p, 0x2024);
    p->direction_x = 0;
}

void eng1_native_code_target_00d210(Actor *p)
{
    p->state = 2;
    reset_frame_state(p);
    attach_actor(p, 0x2023);
}

void eng1_native_code_target_00d240(Actor *p)
{
    if (!p->direction_x)
        p->x += p->motion_x.velocity;
    else
        p->x -= p->motion_x.velocity;
    eng1_interpret_word_stream(p);
    if (eng1_2a710(&g_collision_context->bounds, p))
        eng1_native_code_target_00266a0(p, 3, 0);
    if (eng1_native_code_target_002aa30(p, 200, 200))
        remove_actor(p);
}

void eng1_native_code_target_00d2c0(Actor *p)
{
    int i;

    for (i = 0; i < 11; i++) {
        if (g_placements_511b0[i].x == p->x) {
            p->x += g_placements_511b0[i].dx;
            p->y += g_placements_511b0[i].dy;
            p->z += g_placements_511b0[i].dz;
            p->unknown_072 = (short)g_placements_511b0[i].unknown_072;
            p->scale = (unsigned short)g_placements_511b0[i].scale;
            i = 11;
        }
    }
    play_sound(0x8ca, p);
    p->unknown_023 = 5;
    p->render_flags |= 5;
    reset_frame_state(p);
}

void eng1_native_code_target_00d350(Actor *p)
{
    eng1_interpret_word_stream(p);
    if (eng1_2a9e0(p, 0x22b))
        remove_actor(p);
}

void eng1_native_code_target_00d380(Actor *p)
{
    reset_frame_state(p);
}

void eng1_native_code_target_00d390(Actor *p)
{
    p->unknown_072 += 4;
    eng1_interpret_word_stream(p);
    if (g_engine_interface.context->input_062 & 0x10)
        p->y -= 0x10000;
    if (g_engine_interface.context->input_062 & 0x40)
        p->y += 0x10000;
}

void eng1_native_code_target_00d3d0(Actor *p)
{
    eng1_interpret_word_stream(p);
    if (g_engine_interface.context->input_062 & 0x10)
        p->y -= 0x10000;
    if (g_engine_interface.context->input_062 & 0x40)
        p->y += 0x10000;
}

void eng1_native_code_target_00d410(Actor *p)
{
    p->render_flags |= 5;
    p->y = g_eng1_setup_word_718e0 << 16;
    reset_frame_state(p);
}

void eng1_native_code_target_00d440(Actor *p)
{
    if (eng1_interpret_word_stream(p))
        remove_actor(p);
}

void eng1_native_code_target_00d460(Actor *p)
{
    if (g_eng1_dword_6ef0c != g_engine_interface.context->tick_038) {
        g_eng1_dword_6ef0c = g_engine_interface.context->tick_038;
        eng1_02d80(0, 0, -0x10);
    }
}

void eng1_0d490(Actor *p)
{
    if (p->hits > 0) {
        stop_actor_sounds(p);
        play_sound(0xe0e6, p);
        p->state = 0x24;
        select_frame(p, 8);
    } else {
        stop_actor_sounds(p);
        play_sound(0xe0e7, p);
        p->state = 0x23;
        select_frame(p, 7);
        eng1_131c0(p->x, p->y, p->z, 0x0a002041);
        eng1_131c0(p->x, p->y, p->z, 0x14002021);
        eng1_131c0(p->x, p->y, p->z, 0x1e002041);
        p->flags &= ~0x200;
    }
    eng1_interpret_signed_pairs(p);
}

void eng1_0d560(Actor *p)
{
    long home, dx, distance, lanes;

    eng1_native_code_target_002a290(p);
    home = *(long *)(g_kind_info[p->profile].owner + p->record_offset);
    dx = p->x - home;
    if (!g_engine_interface.context->unknown_024 ||
        (dx < -0x1400000 && p->direction_x == 0xff) ||
        (dx > 0x1400000 && !p->direction_x)) {
        p->state = 0x47;
        select_frame(p, 1);
        eng1_interpret_signed_pairs(p);
        eng1_interpret_signed_pairs(p);
        return;
    }
    dx = p->x - g_normal_actor_pool->x;
    distance = dx < 0 ? -dx : dx;
    lanes = p->lane - g_normal_actor_pool->lane;
    if (distance <= 0xb40000 && !lanes) {
        if ((dx < 0 && p->direction_x == 0xff) || (dx > 0 && !p->direction_x)) {
            select_frame(p, 1);
            p->state = 9;
            eng1_interpret_signed_pairs(p);
            return;
        }
        if (distance > 0x960000) {
            p->state = 10;
            p->motion_x.velocity = 0x50000;
            select_frame(p, 2);
            eng1_interpret_signed_pairs(p);
            return;
        }
        if (distance > 0x3c0000) {
            select_frame(p, 3);
            p->state = 0x3f;
            play_sound(0x68e1, p);
            eng1_interpret_signed_pairs(p);
            return;
        }
        if (random_limit(0x8000) < 0x4000) {
            select_frame(p, 5);
            p->state = 0x3a;
            eng1_interpret_signed_pairs(p);
            return;
        }
        select_frame(p, 5);
        p->state = 0x3a;
        eng1_interpret_signed_pairs(p);
        return;
    }
    if (random_limit(0x8000) > 0x4000) {
        p->state = 0x0b;
        select_frame(p, 6);
        eng1_interpret_signed_pairs(p);
        return;
    }
    p->state = 0x40;
    select_frame(p, 9);
    eng1_interpret_signed_pairs(p);
}

void eng1_native_code_target_00d760(Actor *p)
{
    p->flags = 0x100202;
    p->state = 1;
    p->hits = 6;
    p->lane = (unsigned char)(((p->z >> 16) + 0x40) >> 7);
    p->direction_x = 0xff;
    p->render_flags |= 0x10;
    attach_actor(p, 0x2023);
    eng1_0d560(p);
    p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
    eng1_native_code_target_002a290(p);
}

void eng1_native_code_target_00d7d0(Actor *p)
{
    Bounds b;

    switch (p->state) {
    case 10:
        walk_x_blocked(p, &b);
        eng1_native_code_target_002a290(p);
        if (eng1_interpret_signed_pairs(p))
            eng1_0d560(p);
        break;
    case 0x3f:
        if (eng1_interpret_signed_pairs(p)) {
            if (random_limit(0x8000) > 0x4000)
                play_sound(0x68e2, p);
            else
                play_sound(0x68e3, p);
            select_frame(p, 4);
            p->state = 0x3b;
            p->motion_y.velocity = -0x58000;
            p->motion_y.rate = 0x8800;
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 0x3b:
        walk_x_blocked(p, &b);
        if (p->motion_y.velocity > 0 && eng1_29e20(p))
            p->motion_y.velocity = 0;
        else if (p->motion_x.velocity) {
            p->motion_y.velocity += p->motion_y.rate;
            p->y += p->motion_y.velocity;
        }
        if (eng1_interpret_signed_pairs(p))
            eng1_0d560(p);
        break;
    case 0x23:
        if (p->motion_y.velocity >= 0 && eng1_29e20(p))
            p->motion_y.velocity = 0;
        else {
            p->motion_y.velocity += p->motion_y.rate;
            p->y += p->motion_y.velocity;
        }
        eng1_interpret_signed_pairs(p);
        if (p->frame_ticks >= 0x63)
            p->frame_ticks = 0xc7;
        break;
    case 0x24:
        if (p->motion_y.velocity >= 0 && eng1_29e20(p))
            p->motion_y.velocity = 0;
        else {
            p->motion_y.velocity += p->motion_y.rate;
            p->y += p->motion_y.velocity;
        }
        if (eng1_interpret_signed_pairs(p)) {
            eng1_0d560(p);
            eng1_native_code_target_002a290(p);
        }
        break;
    case 0x47:
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 10;
            p->motion_x.velocity = 0x50000;
            select_frame(p, 2);
            eng1_interpret_signed_pairs(p);
        }
        break;
    default:
        if (eng1_interpret_signed_pairs(p))
            eng1_0d560(p);
        break;
    }
    if (p->state != 0x24 && p->state != 0x23) {
        if (actor_bounds_2a880(p, &b, 2) && bounds_overlap(&g_collision_context->bounds, &b))
            eng1_native_code_target_00266a0(p, 4, 0);
        else if (eng1_26e90(p, 2, 0))
            eng1_0d490(p);
        else if (actor_bounds_2a880(p, &b, 1) && bounds_overlap(&g_collision_context->bounds, &b))
            eng1_native_code_target_00266a0(p, 0x22, 0);
    }
    if (eng1_native_code_target_002aa30(p, 0x200, 0x200)) {
        if (p->state == 0x23)
            remove_actor_preserving_resource(p);
        else
            remove_actor(p);
    }
}

void eng1_0dcb0(Actor *p)
{
    p->hits -= 100;
    p->state = 0x23;
    select_frame(p, 6);
    eng1_131c0(p->x, p->y, p->z, 1);
    stop_actor_sounds(p);
    play_sound(0xe0e7, p);
    p->flags &= ~0x200;
}

void eng1_0dd10(Actor *p)
{
    long dx = p->x - g_normal_actor_pool->x;
    long distance = dx < 0 ? -dx : dx;
    long lanes = p->lane - g_normal_actor_pool->lane;

    if (distance > 0xb40000 || lanes) {
        p->state = 0x0b;
        select_frame(p, 5);
        eng1_interpret_signed_pairs(p);
        return;
    }
    select_frame(p, 3);
    p->state = 0x3b;
    p->motion_x.velocity = 0;
    p->motion_y.velocity = -0x58000;
    p->motion_y.rate = 0x8800;
    if (random_limit(0x8000) < 0x4000)
        play_sound(0x68e2, p);
    else
        play_sound(0x68e3, p);
    eng1_interpret_signed_pairs(p);
}

void eng1_native_code_target_00ddd0(Actor *p)
{
    p->state = 1;
    p->hits = 1;
    p->flags = 0x100202;
    p->lane = (unsigned char)(((p->z >> 16) + 0x40) >> 7);
    p->direction_x = 0xff;
    p->render_flags |= 0x10;
    attach_actor(p, 0x2023);
    eng1_0dd10(p);
    p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
    eng1_native_code_target_002a290(p);
    p->y -= 0x10000;
}

void eng1_native_code_target_00de50(Actor *p)
{
    Bounds b;

    switch (p->state) {
    case 0x3b:
        walk_x_blocked(p, &b);
        if (p->motion_y.velocity > 0 && eng1_29e20(p))
            p->motion_y.velocity = 0;
        else if (p->motion_x.velocity) {
            p->motion_y.velocity += p->motion_y.rate;
            p->y += p->motion_y.velocity;
        }
        if (eng1_interpret_signed_pairs(p)) {
            select_frame(p, 1);
            eng1_interpret_signed_pairs(p);
            p->state = 9;
        }
        break;
    case 9:
    case 0x0b:
        if (eng1_interpret_signed_pairs(p))
            eng1_0dd10(p);
        break;
    case 0x23:
        if (p->motion_y.velocity >= 0 && eng1_29e20(p))
            p->motion_y.velocity = 0;
        else {
            p->motion_y.velocity += p->motion_y.rate;
            p->y += p->motion_y.velocity;
        }
        eng1_interpret_signed_pairs(p);
        if (p->frame_ticks >= 0x63)
            p->frame_ticks = 0x6f;
        break;
    default:
        if (eng1_interpret_signed_pairs(p))
            eng1_0dd10(p);
        break;
    }
    if (p->state != 0x24 && p->state != 0x23) {
        if (actor_bounds_2a880(p, &b, 2) && bounds_overlap(&g_collision_context->bounds, &b))
            eng1_native_code_target_00266a0(p, 4, 0);
        else if (eng1_26e90(p, 2, 0))
            eng1_0dcb0(p);
        else if (actor_bounds_2a880(p, &b, 1) && bounds_overlap(&g_collision_context->bounds, &b))
            eng1_native_code_target_00266a0(p, 0x22, 0);
    }
    if (eng1_native_code_target_002aa30(p, 0x200, 0x200)) {
        if (p->state == 0x23)
            remove_actor_preserving_resource(p);
        else
            remove_actor(p);
    }
}
