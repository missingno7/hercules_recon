/* solZ2: canonical owner names replace loader-zero aliases; storage unchanged. */
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
    short screen_y;                     /* 0x0e */
    unsigned char unknown_010[0x1c - 0x10];
    unsigned char unknown_01c;          /* 0x1c */
    unsigned char unknown_01d[0x21 - 0x1d];
    unsigned char lane;                 /* 0x21 */
    unsigned char unknown_022;
    unsigned char unknown_023;          /* 0x23 */
    unsigned char unknown_024;
    unsigned char unknown_025;          /* 0x25 */
    short unknown_026;                  /* 0x26 */
    unsigned short unknown_028;         /* 0x28 */
    unsigned short state;               /* 0x2a */
    unsigned char unknown_02c[2];
    unsigned short kind;                /* 0x2e */
    short frame_ticks;                  /* 0x30 */
    short frame_state;                  /* 0x32 */
    short frame_command;                /* 0x34 */
    short unknown_036;                  /* 0x36 */
    unsigned short profile;             /* 0x38 */
    unsigned short scale;               /* 0x3a */
    unsigned short scale_y;             /* 0x3c */
    unsigned char unknown_03e[0x4a - 0x3e];
    unsigned short unknown_04a;         /* 0x4a */
    long unknown_04c;                   /* 0x4c */
    unsigned long flags;                /* 0x50 */
    unsigned long render_flags;         /* 0x54 */
    long record_offset;                 /* 0x58 */
    Actor *attachment;                  /* 0x5c */
    unsigned char unknown_060[4];
    short *frame_stream;                /* 0x64 */
    unsigned char unknown_068[0x70 - 0x68];
    short unknown_070;                  /* 0x70 */
    short unknown_072;                  /* 0x72 */
    short unknown_074;                  /* 0x74 */
    unsigned char unknown_076[0x88 - 0x76];
    Bounds bounds;                      /* 0x88 */
    unsigned char direction_x;          /* 0x94 */
    unsigned char direction_y;          /* 0x95 */
    unsigned char direction_z;          /* 0x96 */
    unsigned char unknown_097;          /* 0x97 */
    short unknown_098;                  /* 0x98 */
    unsigned char unknown_09a[2];
    long saved_x, saved_y, saved_z;     /* 0x9c */
    Motion motion_x, motion_y, motion_z;/* 0xa8 */
    long spin_x;                        /* 0xd8 */
    unsigned char unknown_0dc[4];
    long unknown_0e0;                   /* 0xe0 */
    unsigned char unknown_0e4[4];
    Actor *next;                        /* 0xe8 */
    unsigned char unknown_0ec[2];
    short hits;                         /* 0xee */
    unsigned char unknown_0f0[0x108 - 0xf0];
    long counter;                       /* 0x108 */
    unsigned char unknown_10c[0x114 - 0x10c];
    short frame_index;                  /* 0x114 */
    unsigned char unknown_116[2];
    struct SpawnRecord *spawns;         /* 0x118 */
    unsigned char unknown_11c[4];
    unsigned char *bob_profile;         /* 0x120 */
    short bob_step;                     /* 0x124 */
    short unknown_126;                  /* 0x126 */
};

typedef struct KindInfo { unsigned char *owner; long count; } KindInfo;
typedef struct Callback8 { void (*initialize)(Actor *); void *unknown_004; } Callback8;
typedef struct Callback12 { void (*initialize)(Actor *); void *unknown_004, *unknown_008; } Callback12;
typedef struct Debris { short x, y, z; unsigned short kind; } Debris;
typedef struct Placement { long x, dx, dy, dz, unknown_072, scale; } Placement;
typedef struct FixedPosition { long x, y, z; } FixedPosition;
typedef struct HomeRecord { long x, y, z; unsigned short flags; } HomeRecord;
typedef struct SpawnRecord { short x, y, z; short kind, frame, state; } SpawnRecord;
typedef struct LevelFile { const char *directory; unsigned long size; } LevelFile;
typedef struct SfxSlot { unsigned char bank, program, volume; } SfxSlot;
/* PlayStation libsnd-style attribute records filled by the host sound interface. */
typedef struct ProgramAttributes {
    unsigned char tones, volume, priority, mode, pan, reserved_05;
    short attribute;
    unsigned long reserved_08, reserved_0c;
} ProgramAttributes;
typedef struct ToneAttributes {
    unsigned char priority, mode, volume, pan, center, shift, min, max;
    unsigned char unknown_08[0x20 - 0x08];
} ToneAttributes;

typedef struct EngineContext {
    signed char level;                  /* 0x00 */
    unsigned char unknown_001;
    unsigned char unknown_002;
    unsigned char unknown_003[0x0e - 0x03];
    signed char unknown_00e;
    unsigned char unknown_00f[0x24 - 0x0f];
    short unknown_024;
    unsigned char unknown_026[0x38 - 0x26];
    unsigned long tick_038;
    unsigned char unknown_03c[0x62 - 0x3c];
    unsigned char input_062;
    unsigned char unknown_063[0x9c - 0x63];
    short unknown_09c;                  /* 0x9c */
    unsigned char unknown_09e[0xb0 - 0x9e];
    void (*load_complete_0b0)(void);    /* 0xb0 */
    void (*load_failed_0b4)(void);      /* 0xb4 */
    unsigned char unknown_0b8[0xdf - 0xb8];
    unsigned char unknown_0df;          /* 0xdf */
    unsigned char unknown_0e0[0x13c - 0xe0];
    unsigned long *music_buffers[5];    /* 0x13c */
    unsigned long *vab_heads[5];        /* 0x150 */
    unsigned long *vab_bodies[5];       /* 0x164 */
    unsigned long *vab_ranges[5];       /* 0x178 */
    unsigned long *vab_ends[5];         /* 0x18c */
    unsigned char unknown_1a0[0x24c - 0x1a0];
    unsigned char *sequences[5];        /* 0x24c */
    unsigned char unknown_260[0x2b0 - 0x260];
    unsigned short sfx_banks[32];       /* 0x2b0 */
    SfxSlot sfx_slots[0x800];           /* 0x2f0 */
} EngineContext;
typedef struct EngineInterfaceHead { unsigned long signature; EngineContext *context; } EngineInterfaceHead;

extern EngineInterfaceHead g_engine_interface;
extern KindInfo g_kind_info[];
extern Actor *g_normal_actor_pool;
extern Actor *g_collision_context;
extern unsigned short *g_configuration_dword_713bc;
extern Callback12 *g_configuration_dword_71370;
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

extern void engine_diagnostic(const char *, ...);
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
extern Actor *g_frame_global_734f4;
extern Callback8 *g_configuration_dword_7139c;
extern unsigned short g_eng1_collision_map_04f1e0[];
extern unsigned short g_eng1_setup_word_72012;
extern unsigned short g_native_word_72168;
extern unsigned short *g_spatial_sound_words_71714;
extern unsigned short eng1_native_words_04dce8[];
extern long g_camera_x_70ca0, g_camera_y_70ca4;
extern unsigned char g_bob_profile_51ff8[], g_bob_profile_52050[], g_bob_profile_520a8[];
/* Sound/music bank loader state (0x6ef1c..0x6ef3c) and its initialized level selection (0x51690..). */
extern int g_sfx_search_index_6ef1c;
extern unsigned long g_sfx_file_size_6ef20;
extern int g_music_slot_6ef24;
extern int g_sfx_replace_slot_6ef28;
extern int g_sfx_bank_slot_6ef2c;
extern unsigned long g_music_file_size_6ef30;
extern void *g_sfx_buffer_6ef34;
extern int g_music_file_51690;
extern int g_sfx_bank_51694;
extern int g_sfx_file_51698;
extern LevelFile g_level_files_516a0[];
extern int abs(int);
extern void eng1_tile_draw_2ff0(FixedPosition *);
extern void eng1_color_rgb_forward_5a30(int, int, int);
extern void eng1_08450(Actor *);
extern void resource_reclaim(int);
extern long grid_scan_20b50(long, long, long, int);
extern unsigned short grid_query_20d30(short, short, short, int);
extern Actor *spawn_kind(long, long, long, int);
extern void release_actor(Actor *);
extern void apply_frame_motion_3d(Actor *);
extern int eng1_29c20(Actor *);
extern void eng1_native_code_target_002ae50(Actor *);
extern void damped_contact_motion(Actor *);
extern unsigned long eng1_2bd20(Actor *);
extern int engine_file_load_async(const char *, const char *, int, void *, unsigned long);
extern void dispatch_engine_2c2d0(void *);
extern unsigned long engine_file_length(const char *, const char *);
extern void *engine_pool_allocate(void **, unsigned long, unsigned long);
extern int eng1_2c3e0(void *, void *, unsigned long);
extern unsigned int dispatch_engine_2c470(void);
extern void dispatch_engine_2c540(unsigned long, unsigned long);
extern void eng1_2c560(void);
extern void dispatch_engine_2caf0(void);
extern short eng1_2cb20(short, int, ProgramAttributes *);
extern short eng1_2cb40(short, int, int, ToneAttributes *);
extern unsigned long *eng1_2cb60(short);
extern short eng1_2cbd0(short);
extern short eng1_2cbe0(unsigned long *, short);
extern short eng1_2cc00(unsigned long *, short);
extern short eng1_2cc20(short);
extern Actor *spawn_at_group_300c0(long, long, long, int, int);
extern void eng1_native_code_target_000ef10(Actor *);

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
        g_configuration_dword_71370[47].initialize(q);
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

    engine_diagnostic("BensMusicControl here\n");
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
                g_configuration_dword_71370[kind & 0x1ff].initialize(q);
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

/* ---- x16 functions ---- */

int eng1_0e720(void);
void eng1_0e8d0(void);
void eng1_0eb60(void);
void eng1_0eba0(void);
int eng1_0ebd0(void);
void eng1_0ed40(void);
void eng1_0edf0(void);
void eng1_0ee40(void);
void eng1_native_dep_00ff70(Actor *, int);
void eng1_10120(Actor *);

void eng1_native_code_target_00e170(Actor *p)
{
    if (!(g_eng1_collision_map_04f1e0[grid_query_20d30((p->x >> 16) - 0x20, (p->y >> 16) - 0x80, p->z >> 16, p->profile)] & 2))
        p->direction_x = 0xff;
    else
        p->direction_x = 0;
    p->counter = random_limit(0x3c);
    p->state = 1;
    advance_frame(p);
    select_frame(p, 0);
    attach_actor(p, 0x2024);
}

void eng1_native_code_target_00e200(Actor *p)
{
    unsigned short cell;

    switch (p->state) {
    case 1:
        if (p->counter-- < 0) {
            p->counter = random_limit(0x3c);
            eng1_08450(p);
            p->state = 0x21;
            select_frame(p, 1);
        }
        eng1_interpret_signed_pairs(p);
        break;
    case 0x21:
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 2;
            select_frame(p, 2);
        }
        break;
    case 2:
        p->y -= 0x40000;
        if (p->direction_x == 0xff)
            cell = grid_query_20d30((p->x >> 16) - 0x20, p->y >> 16, p->z >> 16, p->profile);
        else
            cell = grid_query_20d30((p->x >> 16) + 0x20, p->y >> 16, p->z >> 16, p->profile);
        if (!(g_eng1_collision_map_04f1e0[cell] & 2) || random_limit(0x8000) < 0x600) {
            p->state = 0x1d;
            select_frame(p, 3);
        }
        eng1_interpret_signed_pairs(p);
        break;
    case 0x1d:
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 1;
            select_frame(p, 0);
            eng1_interpret_signed_pairs(p);
        }
        break;
    default:
        engine_diagnostic("SALMON SHOULDN'T BE HERE\n");
        p->state = 1;
        select_frame(p, 0);
        break;
    }
    if (eng1_native_code_target_002aa30(p, 0x200, 0x200))
        remove_actor(p);
}

void eng1_native_code_target_00e3b0(Actor *p)
{
    p->z += 0x180000;
    p->direction_x = 0xff;
    p->render_flags |= 0x10;
    advance_frame(p);
}

void eng1_frame_target_00e400(void)
{
    FixedPosition xy;

    xy.x = g_camera_x_70ca0 / 4;
    xy.y = g_camera_y_70ca4 / 4 - 0x30;
    eng1_tile_draw_2ff0(&xy);
    if (xy.y > 0x50)
        eng1_color_rgb_forward_5a30(0xa3, 0xa7, 0x57);
    else
        eng1_color_rgb_forward_5a30(0x57, 0xab, 0xf3);
}

void eng1_native_code_target_00e470(Actor *p)
{
    eng1_interpret_signed_pairs(p);
    if (p->counter-- < 0) {
        p->counter = ((random_limit(0x30) >> 4) + 3) * 30;
        if (random_limit(0x8000) < 0x2700)
            play_sound(0x58c5, p);
        else if (random_limit(0x8000) < 0x4000)
            play_sound(0x58c7, p);
        else
            play_sound(0x58c9, p);
    }
    if (eng1_native_code_target_002aa30(p, 0x200, 0x200))
        remove_actor(p);
}

void eng1_native_code_target_00e520(Actor *p)
{
    p->counter = 1;
    p->scale = 2;
    advance_frame(p);
    select_frame(p, 0);
    eng1_interpret_signed_pairs(p);
}

void eng1_native_code_target_00e560(Actor *p)
{
    eng1_interpret_signed_pairs(p);
    if (p->counter-- < 0) {
        p->counter = ((random_limit(0x30) >> 4) + 3) * 30;
        if (random_limit(0x8000) < 0x2700)
            play_sound(0x58c6, p);
        else if (random_limit(0x8000) < 0x4000)
            play_sound(0x58c8, p);
        else
            play_sound(0x58c9, p);
    }
    if (eng1_native_code_target_002aa30(p, 300, 300))
        remove_actor(p);
}

void eng1_native_code_target_00e620(Actor *p)
{
    engine_diagnostic("FloorRubbleNoise here, delay %d\n", p->spin_x);
    if (p->spin_x-- < 0) {
        play_sound(0xd3, 0);
        remove_actor_variant(p);
    }
}

void dispatch_local_0e670(void)
{
    switch (g_eng1_dword_6ef38) {
    case 0:
        g_eng1_dword_6ef3c = 0;
        break;
    case 1:
        eng1_0e720();
        break;
    case 3:
        eng1_0ebd0();
        break;
    case 5:
        if (g_eng1_dword_6ef3c) {
            if (g_sfx_buffer_6ef34)
                dispatch_engine_2c2d0(g_sfx_buffer_6ef34);
            if (g_engine_interface.context->music_buffers[g_music_slot_6ef24])
                dispatch_engine_2c2d0(g_engine_interface.context->music_buffers[g_music_slot_6ef24]);
            g_eng1_dword_6ef3c = 0;
            g_eng1_dword_6ef38 = 0;
        }
        break;
    case 6:
        eng1_0ed40();
        eng1_0e8d0();
        g_eng1_dword_6ef38 = 7;
        break;
    }
}

int eng1_0e720(void)
{
    if (g_engine_interface.context->unknown_09c == 1)
        return -1;
    if (g_engine_interface.context->unknown_0df)
        return -1;
    g_sfx_replace_slot_6ef28 = -1;
    for (g_sfx_search_index_6ef1c = 0; g_sfx_search_index_6ef1c < 32; g_sfx_search_index_6ef1c++) {
        if (g_engine_interface.context->sfx_banks[g_sfx_search_index_6ef1c] == (unsigned char)g_sfx_bank_51694) {
            g_sfx_replace_slot_6ef28 = g_sfx_search_index_6ef1c;
            engine_diagnostic("ReplaceSfx Found Bank ! UnLoadSfx %d %d :: 0x%x 0x%x \n", (unsigned char)g_sfx_bank_51694,
                       g_sfx_search_index_6ef1c, g_engine_interface.context->vab_ranges[g_sfx_search_index_6ef1c],
                       g_engine_interface.context->vab_ends[g_sfx_search_index_6ef1c]);
        }
    }
    if (g_sfx_replace_slot_6ef28 == -1)
        return -1;
    g_sfx_file_size_6ef20 = g_level_files_516a0[(unsigned char)g_sfx_file_51698].size;
    g_sfx_file_size_6ef20 = engine_file_length(g_level_files_516a0[(unsigned char)g_sfx_file_51698].directory, "SFX.SFX");
    if (g_sfx_buffer_6ef34)
        dispatch_engine_2c2d0(g_sfx_buffer_6ef34);
    if (!engine_pool_allocate(&g_sfx_buffer_6ef34, (g_sfx_file_size_6ef20 & ~0x7ff) + 0x1000, 0)) {
        resource_reclaim(1);
        if (!engine_pool_allocate(&g_sfx_buffer_6ef34, (g_sfx_file_size_6ef20 & ~0x7ff) + 0x1000, 0))
            return -1;
    }
    g_sfx_bank_slot_6ef2c = g_sfx_replace_slot_6ef28;
    g_engine_interface.context->load_complete_0b0 = eng1_0eba0;
    g_engine_interface.context->load_failed_0b4 = eng1_0eb60;
    engine_file_load_async(g_level_files_516a0[(unsigned char)g_sfx_file_51698].directory, "SFX.SFX", 0,
                           g_sfx_buffer_6ef34, g_sfx_file_size_6ef20);
    g_eng1_dword_6ef38 = 2;
    return 0;
}

void eng1_0e8d0(void)
{
    ProgramAttributes program;
    ToneAttributes tone;
    unsigned long *header;
    unsigned char *base;
    unsigned char *body;
    unsigned long size;
    int i;

    for (i = 0; i < 0x800; i++) {
        if (g_engine_interface.context->sfx_slots[i].bank == g_sfx_bank_slot_6ef2c) {
            g_engine_interface.context->sfx_slots[i].bank = 0xff;
            g_engine_interface.context->sfx_slots[i].program = 0xff;
        }
    }
    g_engine_interface.context->sfx_banks[g_sfx_bank_slot_6ef2c] = (unsigned char)g_sfx_file_51698;
    header = (unsigned long *)g_sfx_buffer_6ef34;
    base = (unsigned char *)g_sfx_buffer_6ef34;
    g_engine_interface.context->vab_bodies[g_sfx_bank_slot_6ef2c] = (unsigned long *)(base + header[0]);
    body = base + header[1];
    size = (g_sfx_file_size_6ef20 / 0x800 + 2) * 0x800 - (body - base);
    eng1_2cbd0((short)g_sfx_bank_slot_6ef2c);
    dispatch_engine_2caf0();
    eng1_2c3e0(g_engine_interface.context->vab_heads[g_sfx_bank_slot_6ef2c], body, size);
    if (eng1_2cbe0(g_engine_interface.context->vab_heads[g_sfx_bank_slot_6ef2c], (short)g_sfx_bank_slot_6ef2c) < 0) {
        engine_diagnostic("\n Unable to open VAB\n");
        return;
    }
    if (eng1_2cc00(g_engine_interface.context->vab_bodies[g_sfx_bank_slot_6ef2c], (short)g_sfx_bank_slot_6ef2c) < 0) {
        engine_diagnostic("\n Unable to down load VAB\n");
        return;
    }
    eng1_2cc20(1);
    g_engine_interface.context->vab_ranges[g_sfx_bank_slot_6ef2c] = eng1_2cb60((short)g_sfx_bank_slot_6ef2c);
    g_engine_interface.context->vab_ends[g_sfx_bank_slot_6ef2c] =
        g_engine_interface.context->vab_ranges[g_sfx_bank_slot_6ef2c] +
        (g_engine_interface.context->vab_heads[g_sfx_bank_slot_6ef2c] - g_engine_interface.context->vab_bodies[g_sfx_bank_slot_6ef2c]);
    for (i = 0; i < 0x7f; i++) {
        eng1_2cb20((short)g_sfx_bank_slot_6ef2c, i, &program);
        if (program.attribute) {
            if (g_engine_interface.context->sfx_slots[program.attribute].bank != 0xff) {
                engine_diagnostic("\n************************************************************\n");
                engine_diagnostic("\n\nTwo or more sound effects have been assigned the same ");
                engine_diagnostic("\nattribute number:: FX_ID %d Attribute %d", i, program.attribute);
                engine_diagnostic("\n\n************************************************************");
            }
            g_engine_interface.context->sfx_slots[program.attribute].bank = (unsigned char)g_sfx_bank_slot_6ef2c;
            g_engine_interface.context->sfx_slots[program.attribute].program = (unsigned char)i;
            eng1_2cb40((short)g_sfx_bank_slot_6ef2c, i, 0, &tone);
            g_engine_interface.context->sfx_slots[program.attribute].volume = tone.volume;
        }
    }
    dispatch_engine_2c2d0(g_sfx_buffer_6ef34);
}

void eng1_0eb60(void)
{
    if (g_sfx_buffer_6ef34)
        dispatch_engine_2c2d0(g_sfx_buffer_6ef34);
    g_engine_interface.context->load_complete_0b0 = 0;
    g_engine_interface.context->load_failed_0b4 = 0;
    g_eng1_dword_6ef38 = 1;
}

void eng1_0eba0(void)
{
    g_eng1_dword_6ef38 = 3;
    g_engine_interface.context->load_complete_0b0 = 0;
    g_engine_interface.context->load_failed_0b4 = 0;
}

int eng1_0ebd0(void)
{
    int music = g_music_file_51690;

    if (g_engine_interface.context->unknown_09c == 1)
        return -1;
    if (g_engine_interface.context->unknown_0df)
        return -1;
    g_music_slot_6ef24 = 0;
    g_music_file_size_6ef30 = g_level_files_516a0[music].size;
    engine_diagnostic("LoadMusicSeqData %d %d\n", music, g_music_slot_6ef24);
    g_engine_interface.context->sfx_banks[g_music_slot_6ef24] = (unsigned short)music;
    g_music_file_size_6ef30 = engine_file_length(g_level_files_516a0[music].directory, "MUSIC.MUS");
    g_engine_interface.context->music_buffers[g_music_slot_6ef24] = 0;
    if (!engine_pool_allocate((void **)&g_engine_interface.context->music_buffers[g_music_slot_6ef24],
                              (g_music_file_size_6ef30 & ~0x7ff) + 0x1000, 0)) {
        resource_reclaim(1);
        if (!engine_pool_allocate((void **)&g_engine_interface.context->music_buffers[g_music_slot_6ef24],
                                  (g_music_file_size_6ef30 & ~0x7ff) + 0x1000, 0))
            return -1;
    }
    g_engine_interface.context->load_complete_0b0 = eng1_0ee40;
    g_engine_interface.context->load_failed_0b4 = eng1_0edf0;
    engine_file_load_async(g_level_files_516a0[music].directory, "MUSIC.MUS", 0,
                           g_engine_interface.context->music_buffers[0], g_music_file_size_6ef30);
    g_eng1_dword_6ef38 = 4;
    return 0;
}

void eng1_0ed40(void)
{
    unsigned long *buffer = g_engine_interface.context->music_buffers[g_music_slot_6ef24];

    g_engine_interface.context->vab_bodies[g_music_slot_6ef24] = 0;
    g_engine_interface.context->vab_heads[g_music_slot_6ef24] = 0;
    g_engine_interface.context->sequences[g_music_slot_6ef24] = (unsigned char *)buffer + buffer[2];
    eng1_2c560();
    eng1_2c3e0(g_engine_interface.context->sequences[1], g_engine_interface.context->sequences[g_music_slot_6ef24],
               g_music_file_size_6ef30);
    dispatch_engine_2c2d0(g_engine_interface.context->music_buffers[g_music_slot_6ef24]);
    dispatch_engine_2c540(0, 0);
}

void eng1_0edf0(void)
{
    if (g_engine_interface.context->music_buffers[g_music_slot_6ef24])
        dispatch_engine_2c2d0(g_engine_interface.context->music_buffers[g_music_slot_6ef24]);
    g_eng1_dword_6ef38 = 3;
    g_engine_interface.context->load_complete_0b0 = 0;
    g_engine_interface.context->load_failed_0b4 = 0;
}

void eng1_0ee40(void)
{
    g_eng1_dword_6ef38 = 5;
    g_engine_interface.context->load_complete_0b0 = 0;
    g_engine_interface.context->load_failed_0b4 = 0;
}

void eng1_native_code_target_00ee70(Actor *p)
{
    eng1_interpret_word_stream(p);
    p->z += p->unknown_098 << 16;
    if (eng1_native_code_target_002aa30(p, 0x84, 0x84))
        remove_actor_variant(p);
}

void eng1_native_code_target_00eec0(Actor *p)
{
    reset_frame_state(p);
    p->render_flags |= 5;
    p->unknown_04a = 0x60;
}

void eng1_native_code_target_00eee0(Actor *p)
{
    eng1_interpret_word_stream(p);
    if (eng1_native_code_target_002aa30(p, 0x84, 0x84))
        release_actor(p);
}

void eng1_native_code_target_00efa0(Actor *p)
{
    p->flags = 0x10000;
    p->render_flags |= 0x604;
    p->unknown_023 = 5;
    p->unknown_04a = 0;
    reset_frame_state(p);
    eng1_interpret_word_stream(p);
}

void eng1_native_code_target_00efe0(Actor *p)
{
    Actor *q = p->attachment;
    unsigned long h;

    p->x = q->x;
    p->y = q->y;
    p->z = q->z;
    h = grid_scan_20b50(p->x >> 16, p->y >> 16, p->z >> 16, p->profile) << 16;
    if (h > 0x1000000) {
        p->render_flags &= 0x7fffffff;
    } else {
        p->y += h;
        p->render_flags |= 0x80000000;
        p->scale = 0x100 - (h >> 16);
    }
    if (g_engine_interface.context->level == 3 && p->y > 0x2d80000)
        p->render_flags &= 0x7fffffff;
}

void eng1_native_code_target_00f3b0(Actor *p)
{
    Actor *q = p->attachment;
    long h;

    if (~q->flags & 8)
        p->y = q->y - 0x20000;
    p->x = q->x;
    p->z = q->z + 0x280000;
    h = grid_scan_20b50(p->x >> 16, p->y >> 16, p->z >> 16, p->profile) << 16;
    if (h > 0x1000000) {
        p->render_flags &= 0x7fffffff;
        return;
    }
    p->y += h;
    p->scale = q->scale - (h >> 17);
    p->unknown_072 = 0x80;
}

void eng1_native_code_target_00f4f0(Actor *p)
{
    long plane;
    unsigned long tile;
    HomeRecord *home;

    p->render_flags |= 8;
    p->unknown_097 = 0;
    p->unknown_028 = 0;
    if (p->unknown_025 & 1) {
        eng1_native_code_target_002a290(p);
    } else {
        eng1_interpret_signed_pairs(p);
        apply_frame_motion_3d(p);
        plane = (p->z + 0x400000) & 0xf800000;
        switch (p->state) {
        case 2:
        case 12:
            if (p->z > p->unknown_0e0) {
                tile = g_configuration_dword_713bc[dispatch_local_2a170(p->x, p->y, plane, p->profile)];
                if (tile & 0x10) {
                    p->unknown_0e0 += 0x800000;
                } else {
                    p->z = p->unknown_0e0;
                    p->direction_x = ~p->direction_x;
                    select_frame(p, 0x0f);
                    p->state = 0;
                }
            }
            break;
        case 1:
        case 11:
            if (p->z < p->unknown_0e0) {
                tile = g_configuration_dword_713bc[dispatch_local_2a170(p->x, p->y, plane, p->profile)];
                if (tile & 0x20) {
                    p->unknown_0e0 -= 0x800000;
                } else {
                    p->z = p->unknown_0e0;
                    p->direction_x = ~p->direction_x;
                    select_frame(p, 0x15);
                    p->state = 0;
                }
            }
            break;
        case 3:
            break;
        default:
            tile = g_configuration_dword_713bc[dispatch_local_2a170(p->x + (p->direction_x ? -0x180000 : 0x180000),
                                                                    p->y - 0x100000, plane, p->profile)];
            if (tile & 3) {
                select_frame(p, 9);
            } else {
                tile = g_configuration_dword_713bc[dispatch_local_2a170(p->x, p->y, plane, p->profile)];
                if (tile & 0xc0)
                    select_frame(p, 0x1b);
            }
            break;
        }
    }
    if (!eng1_29c20(p)) {
        damped_contact_motion(p);
        eng1_native_code_target_000ef10(p);
    }
    p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
    if (p->y > 0x4f00000) {
        /* Native 32-bit relative address: retain one carrier through the null
         * test and owner-base addition so C2 coalesces both definitions. */
        home = (HomeRecord *)p->record_offset;
        if (home) {
            home = (HomeRecord *)(g_kind_info[p->profile].owner + (long)home);
            home->flags &= ~0x100;
        }
        remove_actor(p);
        return;
    }
    if (abs((p->x - g_frame_global_734f4->x) >> 16) > 0x500)
        remove_actor(p);
}

void eng1_native_code_target_00f770(Actor *p)
{
    long plane = (p->z + 0x400000) & 0xf800000;
    unsigned long cell;
    unsigned long tile;

    if (p->state != 5)
        p->state = 0;
    if (p->record_offset &&
        abs(p->x - ((HomeRecord *)(g_kind_info[p->profile].owner + p->record_offset))->x) > 0x8000000) {
        select_frame(p, 9);
        return;
    }
    cell = dispatch_local_2a170(p->x, p->y, plane, p->profile);
    if (cell) {
        tile = g_configuration_dword_713bc[cell];
        if (p->direction_x) {
            if (tile & 0x20) {
                select_frame(p, 0x12);
                p->state = 1;
                p->unknown_0e0 = (p->z & 0x7f800000) - 0x800000;
                return;
            }
        } else if (tile & 0x10) {
            p->unknown_0e0 = (p->z & 0x7f800000) + 0x800000;
            select_frame(p, 0x0c);
            p->state = 2;
            return;
        }
    }
    if (p->unknown_126) {
        p->unknown_126--;
        return;
    }
    p->unknown_126 = random_limit(4) + 4;
    if (p->state == 5 || p->state == 6) {
        select_frame(p, 0x28);
        return;
    }
    switch (random_limit(12)) {
    case 0:
        select_frame(p, 0x1f);
        return;
    case 1:
        select_frame(p, 0x20);
        return;
    case 2:
        select_frame(p, 0x21);
        return;
    case 3:
        select_frame(p, 0x1b);
        return;
    default:
        select_frame(p, 0x26);
        return;
    }
}

void eng1_native_code_target_00f910(Actor *p)
{
    int state = p->state;
    int v;

    p->render_flags |= 0x10000000;
    p->flags |= 0x240000;
    p->unknown_097 = 0;
    if (p->state == 3)
        p->unknown_097 = 9;
    if (p->state == 2)
        p->unknown_097 = 8;
    p->hits = 0x1e;
    reset_frame_state(p);
    eng1_interpret_word_stream(p);
    p->state = 0;
    if (state) {
        do {
            v = p->frame_stream[++p->frame_state];
            if (v < 0) {
                if (v == -0x28)
                    p->frame_state++;
                if (v == -0x29) {
                    p->state = p->frame_stream[++p->frame_state];
                    if (state == p->state)
                        v = 0;
                }
            }
        } while (v);
        do {
            v = p->frame_stream[++p->frame_state];
            if (v < 0)
                p->frame_state++;
        } while (v < 0);
        p->frame_command = v;
    }
    p->bob_profile = g_bob_profile_51ff8;
    p->bob_step = 4;
}

void eng1_native_code_target_00fa00(Actor *p)
{
    Actor *q;
    Actor *r;
    int i, kind;

    eng1_native_code_target_002ae50(p);
    switch (p->state) {
    case 1:
        p->bob_profile = g_bob_profile_52050;
        break;
    case 2:
        p->bob_profile = g_bob_profile_520a8;
        break;
    case 3:
    case 9:
        p->flags &= ~0x200000;
        break;
    }
    if (p->state < 3) {
        if (abs((p->x - g_collision_context->x) >> 16) < 0x40) {
            for (q = p->next; q; q = q->next) {
                if (q == g_collision_context && (g_eng1_setup_word_72012 & 0x20 & ~g_native_word_72168)) {
                    spawn_at_group_300c0(g_collision_context->x, g_collision_context->y, g_collision_context->z, 0x2027, 0);
                    r = spawn_at_group_300c0(g_collision_context->x + ((random_limit(0x40) - 0x20) << 16),
                                             g_collision_context->y - (random_limit(0x20) << 16),
                                             g_collision_context->z - 0x100000, 0x2027, 0);
                    if (r)
                        r->unknown_04c = random_limit(10) + 5;
                    eng1_native_dep_00ff70(p, 0x20);
                    g_native_word_72168 |= 0x20;
                    eng1_interpret_word_stream(p);
                    if (p->state == 3) {
                        p->unknown_097 = 9;
                        eng1_10120(p);
                        for (i = 0; (kind = p->spawns[i].kind) != 0; i++) {
                            if (kind & 0x2000) {
                                r = spawn_kind(p->x + (p->spawns[i].x << 16), p->y + (p->spawns[i].y << 16),
                                               p->z + (p->spawns[i].z << 16), kind);
                                if (r) {
                                    r->state = p->spawns[i].state;
                                    g_configuration_dword_7139c[kind & 0x1ff].initialize(r);
                                }
                            }
                            if (kind & 0x4000) {
                                r = spawn_middle_277e0(p->x + (p->spawns[i].x << 16), p->y + (p->spawns[i].y << 16),
                                                       p->z + (p->spawns[i].z << 16), kind);
                                if (r) {
                                    r->state = p->spawns[i].state;
                                    g_special_callbacks[kind & 0x1ff].initialize(r);
                                    r->frame_command = p->spawns[i].frame;
                                }
                            }
                        }
                        break;
                    }
                    if (p->state == 2) {
                        p->unknown_097 = 8;
                        break;
                    }
                }
            }
        }
    } else if (p->state != 9) {
        if (--p->hits < 0 || p->screen_y < -0x10) {
            eng1_interpret_word_stream(p);
            remove_actor_preserving_resource(p);
            return;
        }
    }
    if (eng1_2a9e0(p, 0x140))
        remove_actor(p);
}

void eng1_native_code_target_00fd00(Actor *p)
{
    p->direction_x = (unsigned char)p->state;
    p->render_flags |= 0x10000000;
    p->direction_y = 0;
    switch (p->state) {
    case 0:
        p->motion_y.target = ((dispatch_engine_2c470() & 0x1ff) + 0x480) << 8;
        p->motion_y.velocity = p->motion_y.target >> 2;
        p->motion_y.rate = random_limit(0x1000) + 0x2000;
        p->motion_x.target = 0x20000;
        p->motion_x.rate = 0x1000;
        p->direction_x = 0xff;
        p->unknown_0e0 = 8;
        p->spin_x = 2;
        break;
    case 1:
        p->motion_y.target = ((dispatch_engine_2c470() & 0x1ff) + 0x600) << 8;
        p->motion_y.velocity = p->motion_y.target >> 2;
        p->motion_y.rate = random_limit(0x2000) + 0x2000;
        p->unknown_0e0 = 0;
        p->spin_x = 6;
        break;
    case 2:
        p->motion_y.target = ((dispatch_engine_2c470() & 0x1ff) + 0x400) << 8;
        p->motion_y.velocity = p->motion_y.target >> 2;
        p->motion_y.rate = random_limit(0x1000) + 0x1000;
        p->motion_x.target = 0x20000;
        p->motion_x.rate = 0x1000;
        p->direction_x = 0;
        p->unknown_0e0 = -12;
        p->spin_x = 0;
        break;
    }
}

void eng1_native_code_target_00fe60(Actor *p)
{
    Actor *q;

    p->unknown_074 += (short)p->unknown_0e0;
    p->unknown_070 += (short)p->spin_x;
    p->y += 0x200000;
    p->saved_y += 0x200000;
    if (eng1_2bd20(p) & 1) {
        eng1_native_dep_00ff70(p, 0x10);
        play_sound(g_spatial_sound_words_71714[0] | 0xc000, p);
        spawn_at_group_300c0(p->x, p->y - 0x200000, p->z - 0x200000, 0x2027, 0);
        q = spawn_at_group_300c0(p->x + ((random_limit(0x20) - 0x10) << 16), p->y - (random_limit(0x20) << 16) - 0x100000,
                                 p->z - 0x200000, 0x2027, 0);
        if (q)
            q->unknown_04c = random_limit(10) + 10;
        remove_actor_variant(p);
    }
    p->y -= 0x200000;
    p->saved_y -= 0x200000;
}

void eng1_native_dep_00ff70(Actor *p, int spread)
{
    int i, n;
    long offset;
    Actor *q;

    n = random_limit(11) + 8;
    for (i = 0; i < n; i++) {
        offset = (random_limit(spread * 2) - spread) << 16;
        random_limit(spread);
        q = spawn_at_group_300c0(p->x + offset, p->y, p->z, 0x4024, 0);
        if (q) {
            q->render_flags |= 0x10000000;
            q->frame_command = g_spatial_sound_words_71714[random_limit(2) + 3];
            q->scale = random_limit(0x80 - n * 3) + (0x40 - n * 3);
            q->direction_x = -random_limit(2);
            q->direction_z = 0xff;
            q->direction_y = 0xff;
            q->motion_y.velocity = q->motion_y.target = ((dispatch_engine_2c470() & 0x1ff) + 0x800) << 8;
            q->motion_y.rate = 0;
            q->motion_y.unknown = random_limit(0x2000) + 0x4000;
            q->motion_z.velocity = q->motion_z.target = ((dispatch_engine_2c470() & 0xff) + 0x100) << 8;
            q->motion_z.rate = random_limit(0x1000) + 0x1000;
            q->motion_z.unknown = 0;
            q->motion_x.velocity = q->motion_x.target = ((dispatch_engine_2c470() & 0x1ff) + 0x100) << 8;
            q->motion_x.rate = random_limit(0x1000) + 0x1000;
            q->motion_x.unknown = 0;
            q->state = 0;
        }
    }
}
