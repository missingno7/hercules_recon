/* ENG1.DLL 0x83e0..0xa420: level-specific actor handlers following the /Od sound unit
   (rock-throwing walker, leaping fish, centaur, lava rock, breakable rock).  Lane x9 scratch,
   2026-10-20.  /O2 /Gy code (aligned, no frame pointers); no ENG3/TITLE twins (ENG1-only strings).
   Compiled as C++ (extern "C"): 0x92c0, 0x9ec0 and 0xa2c0 are masked-equal as C++ with this
   minimal context; as C they need ~10/55/180 extra declarations of TU context (diagnostic scan only).
   Local views cover only offsets observed in these bodies. */

extern "C" {

typedef struct Bounds { short min_x, min_y, min_z, max_x, max_y, max_z; } Bounds;
typedef struct Motion { long velocity, target, rate, unknown; } Motion;

typedef struct Actor Actor;
struct Actor {
    long x, y, z;                       /* 0x00 */
    short screen_x;                     /* 0x0c */
    unsigned char unknown_00e[0x21 - 0x0e];
    unsigned char lane;                 /* 0x21 */
    unsigned char unknown_022[0x2a - 0x22];
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
    unsigned char unknown_05c[0x88 - 0x5c];
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

typedef struct Position { long x, y, z; } Position;
typedef struct KindInfo { unsigned char *owner; long count; } KindInfo;
typedef struct Callback8 { void (*initialize)(Actor *); void *unknown_004; } Callback8;
typedef struct Callback12 { void (*initialize)(Actor *); void *unknown_004, *unknown_008; } Callback12;
typedef struct ScriptStep { long op, value; } ScriptStep;
typedef struct Debris { short x, y, z; unsigned short kind; } Debris;

typedef struct EngineContext {
    unsigned char unknown_000[0x0e];
    signed char unknown_00e;
    unsigned char unknown_00f[0x24 - 0x0f];
    short unknown_024;
    unsigned char unknown_026[0x38 - 0x26];
    unsigned long tick_038;
} EngineContext;
typedef struct EngineInterfaceHead { unsigned long signature; EngineContext *context; } EngineInterfaceHead;

extern EngineInterfaceHead g_engine_interface;
extern long g_actor_worklist_71760[];
extern KindInfo g_kind_info[];
extern ScriptStep *g_actor_scripts_4f378[];
extern Actor *g_normal_actor_pool;
extern Actor *g_collision_context;
extern unsigned short *g_configuration_dword_713bc;
extern Callback12 *g_normal_callbacks;
extern Callback8 *g_special_callbacks;
extern unsigned short g_eng1_setup_word_718e0;
extern unsigned short g_eng1_setup_word_72012;
extern unsigned short g_native_word_72168;
extern unsigned short g_eng1_setup_word_7082e;
extern unsigned short g_eng1_setup_word_6ef90;
extern unsigned short g_pipeline_word_6fec4;
extern Bounds g_actor_pipeline_storage_71440;
extern Debris g_debris_4fb50[13];
extern Debris g_debris_4fbb8[12];
extern long g_bob_steps_4fc18[];

extern void dbg_printf(const char *, ...);
extern void play_sound(unsigned short, Actor *);
extern void stop_actor_sounds(Actor *);
extern void eng1_0ceb0(Actor *);
extern void eng1_0cfb0(Actor *);
extern void eng1_131c0(long, long, long, long);
extern void eng1_1ab50(void);
extern void eng1_1aba0(void);
extern long eng1_native_code_target_00266a0(Actor *, int, int);
extern unsigned long eng1_26de0(Actor *);
extern int eng1_26e90(Actor *, int, int);
extern Actor *spawn_normal(long, long, long, int);
extern int remove_actor(Actor *);
extern void remove_actor_preserving_resource(Actor *);
extern Actor *spawn_middle_277e0(long, long, long, unsigned short);
extern void attach_actor(Actor *, int);
extern int eng1_interpret_signed_pairs(Actor *);
extern void eng1_interpret_word_stream(Actor *);
extern void reset_frame_state(Actor *);
extern void advance_frame(Actor *);
extern void select_frame(Actor *, int);
extern int eng1_29c20(Actor *);
extern int eng1_29e20(Actor *);
extern unsigned long dispatch_local_2a170(long, long, long, int);
extern void eng1_native_code_target_002a290(Actor *);
extern int eng1_2a710(Bounds *, Actor *);
extern int bounds_overlap(Bounds *, Bounds *);
extern int actor_bounds_2a880(Actor *, Bounds *, int);
extern int eng1_native_code_target_002aa30(Actor *, int, int);
extern unsigned long eng1_2bc50(Actor *);
extern unsigned int random_limit(int);
extern Actor *spawn_at_group_300c0(long, long, long, int, int);
extern void eng1_30290(unsigned long, int);

typedef struct SlotState { unsigned short state_00; unsigned char unknown_02[0x12]; } SlotState;

extern SlotState g_slots_5cef0[];
extern unsigned short g_eng1_collision_map_04f1e0[];
extern unsigned long g_eng1_setup_dword_72118;
extern unsigned long g_eng1_setup_dword_72120;
extern unsigned long g_eng1_setup_dword_72188;
extern long g_eng1_dword_72178;
extern long g_boss_threshold_72184;
extern long g_eng1_dword_721b4;
extern short g_native_word_70832;

extern void resource_reset(int);
extern void resource_cancel(int);
extern void RemoveFxIdLinks_7f8e(unsigned short, Actor *);
extern void eng1_0cdd0(Actor *);
extern void eng1_0d0c0(Actor *);
extern void eng1_17340(void);
extern void eng1_19240(void);
extern unsigned short grid_query_20d30(short, short, short, int);
extern long eng1_2a010(long, long, long, int);
extern int abs(int);

/* Walk one step along x in the facing direction; step back when the leading edge hits a wall cell.
   C++ inline function (inlined under /O2's /Ob1): its return value is needed by 0xbc50, whose block
   layout only reproduces with the inlined-return form, not with an equivalent macro or goto. */
inline int walk_x_blocked(Actor *p, Bounds *b)
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


Actor *eng1_083e0(Actor *p, int kind, long low, long high)
{
    long x = p->x;
    int i = 0;
    Actor *q;
    long d;

    while (i < g_actor_worklist_71760[0]) {
        q = (Actor *)g_actor_worklist_71760[++i];
        if (q->kind == kind) {
            d = p->z - q->z;
            if (d <= 0x200000 && d >= -0x200000) {
                d = q->x - x;
                if (d >= low && d <= high)
                    return q;
            }
        }
    }
    return 0;
}

void eng1_08450(Actor *p)
{
    Position *home = (Position *)(g_kind_info[p->profile].owner + p->record_offset);
    p->x = home->x;
    p->y = home->y;
    p->z = home->z;
}

void eng1_08480(Actor *p, unsigned short floor)
{
    unsigned short ground = (unsigned short)eng1_2a010(p->x, p->y, p->z, p->profile);
    if (ground < floor)
        p->y += (ground - floor) << 16;
}

void eng1_084d0(Actor *p)
{
    ScriptStep *steps = g_actor_scripts_4f378[(short)p->counter];
    int i = p->counter >> 16;
    long value;

    while (steps[i].op) {
        value = steps[i].value;
        switch (steps[i].op) {
        case 1:
            if (p->x - g_normal_actor_pool->x < value)
                return;
            break;
        case 2:
            if (p->x < value)
                return;
            break;
        case 3:
            if (p->state == value)
                return;
            break;
        case 4:
            if (p->frame_index == value)
                return;
            break;
        case 5:
            if (p->frame_state != value)
                return;
            break;
        case 6:
            select_frame(p, value);
            p->frame_ticks = 0;
            break;
        case 7:
            p->state = (unsigned short)value;
            if (value == 0x1c) {
                p->direction_y = 0;
                p->motion_y.target = 0x80000;
                p->motion_y.velocity = 0;
                p->motion_y.rate = 0x8000;
                p->motion_y.unknown = 0;
            }
            break;
        case 8:
            p->motion_x.velocity = value;
            break;
        case 9:
            p->motion_z.velocity = value;
            break;
        case 10:
            i = -1;
            p->counter |= 0xffff0000;
            break;
        default:
            goto done;
        }
        i++;
        p->counter += 0x10000;
    }
done:
    p->state = 0;
    select_frame(p, 0);
}

void eng1_08630(Actor *p)
{
    p->state = 0x23;
    select_frame(p, 0x0c);
    eng1_interpret_signed_pairs(p);
}

void eng1_native_code_target_008660(Actor *p)
{
    p->flags = 0x212;
    p->scale = 0x200;
    p->hits = 100;
    p->lane = (unsigned char)(((p->z >> 16) + 0x40) >> 7);
    p->direction_x = 0xff;
    p->render_flags |= 0x10;
    p->state = 10;
    p->motion_x.velocity = 0x20000;
    select_frame(p, 2);
    eng1_interpret_signed_pairs(p);
    eng1_native_code_target_002a290(p);
    p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
    attach_actor(p, 0x2024);
}

void eng1_native_code_target_0086f0(Actor *p)
{
    Bounds b;
    Actor *q;
    long d;

    switch (p->state) {
    case 10:
        walk_x_blocked(p, &b);
        if (p->frame_state == 3 && p->frame_ticks == 0 && (p->y >> 16) < g_eng1_setup_word_718e0)
            play_sound(0xe0dd, p);
        eng1_native_code_target_002a290(p);
        if (eng1_interpret_signed_pairs(p)) {
            select_frame(p, 1);
            eng1_interpret_signed_pairs(p);
            p->state = 9;
        }
        break;
    case 9:
        walk_x_blocked(p, &b);
        if (p->frame_state == 3 && p->frame_ticks == 0 && (p->y >> 16) < g_eng1_setup_word_718e0)
            play_sound(0xe0de, p);
        if (eng1_interpret_signed_pairs(p)) {
            p->motion_x.velocity = -0x320000;
            walk_x_blocked(p, &b);
            p->motion_x.velocity = 0;
            eng1_native_code_target_002a290(p);
            p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
            p->state = 0x0b;
            select_frame(p, 3);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 11:
        if (eng1_interpret_signed_pairs(p)) {
            d = p->lane - g_normal_actor_pool->lane;
            if (d <= 3 &&
                ((p->y >> 16) <= g_eng1_setup_word_718e0 || g_normal_actor_pool->y >= g_eng1_setup_word_718e0 << 16) &&
                d >= 1 && p->x - g_normal_actor_pool->x <= 0xf00000 && p->x - g_normal_actor_pool->x >= -0xf00000) {
                p->state = 0x0d;
                select_frame(p, 4);
                eng1_interpret_signed_pairs(p);
            } else {
                p->state = 10;
                p->motion_x.velocity = 0x20000;
                select_frame(p, 2);
                eng1_interpret_signed_pairs(p);
            }
        }
        break;
    case 13:
        if (eng1_interpret_signed_pairs(p)) {
            if (p->screen_x >= -0x20 && p->screen_x <= 0x160) {
                p->state = 0x0f;
                select_frame(p, 7);
                eng1_interpret_signed_pairs(p);
            } else {
                p->state = 0x19;
                select_frame(p, 5);
                eng1_interpret_signed_pairs(p);
            }
        }
        break;
    case 15:
        if (eng1_interpret_signed_pairs(p)) {
            p->frame_ticks = -1;
            q = spawn_normal(p->x, p->y - 0x820000, p->z, 0x4a);
            if (q) {
                g_normal_callbacks[74].initialize(q);
                q->scale = 0x200;
                q->direction_x = p->direction_x;
                q->render_flags = (q->direction_x & 0x10) | (q->render_flags & ~0x10);
                if (p->y > g_eng1_setup_word_718e0 << 16)
                    q->state = 0x3e;
                else
                    q->state = 2;
                q->motion_y.velocity = -0xc0000;
                q->motion_z.velocity = 0x80000;
                if (!q->direction_x) {
                    q->x += 0x140000;
                    q->motion_x.velocity = (g_normal_actor_pool->x - q->x) / 30;
                } else {
                    q->x -= 0x140000;
                    q->motion_x.velocity = (g_normal_actor_pool->x - q->x) / -30;
                }
                if (g_normal_actor_pool->y < g_eng1_setup_word_718e0 && p->x - g_normal_actor_pool->x > 0x780000)
                    q->motion_x.velocity /= 2;
            }
            p->state = 0x19;
            select_frame(p, 5);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 25:
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 10;
            p->motion_x.velocity = 0x20000;
            select_frame(p, 2);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 35:
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 0x25;
            select_frame(p, 0x14);
            eng1_interpret_signed_pairs(p);
            p->scale = 0x200;
            p->scale_y = 0x200;
            p->render_flags |= 0x80;
        }
        break;
    case 37:
        if (p->y > g_eng1_setup_word_718e0 << 16) {
            if (g_engine_interface.context->tick_038 & 0x10) {
                p->scale_y -= 4;
                if (p->scale_y < 0x42)
                    p->scale_y = 0x42;
            }
            if (eng1_interpret_signed_pairs(p)) {
                remove_actor_preserving_resource(p);
                return;
            }
        } else
            eng1_interpret_signed_pairs(p);
        break;
    }
    if (p->state != 0x25)
        eng1_29c20(p);
    if (eng1_native_code_target_002aa30(p, 0x26e, 0x26e)) {
        if (p->state == 0x23 || p->state == 0x25)
            remove_actor_preserving_resource(p);
        else
            remove_actor(p);
    }
}

void eng1_native_code_target_008dd0(Actor *p)
{
    p->counter = random_limit(0x3c);
    p->state = 1;
    advance_frame(p);
    select_frame(p, 0);
    attach_actor(p, 0x2024);
}

void eng1_native_code_target_008e10(Actor *p)
{
    long r;
    Actor *q;

    switch (p->state) {
    default:
        if (!p->direction_x)
            p->x += p->motion_x.velocity;
        else
            p->x -= p->motion_x.velocity;
        p->z += p->motion_z.velocity;
        if (eng1_interpret_signed_pairs(p)) {
            p->counter = random_limit(0x78);
            p->state = 1;
            select_frame(p, 0);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 1:
        if (p->counter-- < 0) {
            p->motion_x.velocity = 0;
            p->motion_z.velocity = 0;
            eng1_08450(p);
            r = random_limit(0x8000) << 8;
            switch ((r >> 8) % 3) {
            case 0:
                p->state = 0x11;
                select_frame(p, 1);
                p->x += (r & 0x7f0000) - 0x400000;
                p->z += (r & 0xf0000) - 0x80000;
                if (random_limit(0x80) > 0x40)
                    p->direction_x = 0;
                else
                    p->direction_x = 0xff;
                p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
                break;
            case 1:
                p->state = 0x12;
                p->x += (r & 0x3f0000) - 0x200000;
                p->z += r & 0x3f0000;
                p->motion_z.velocity = -0x8000 - (r & 0xffff);
                select_frame(p, 2);
                if (r & 0x100000)
                    p->direction_x = 0xff;
                else
                    p->direction_x = 0;
                p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
                break;
            case 2:
                p->state = 0x13;
                select_frame(p, 3);
                p->motion_x.velocity = -0x10000 - (r & 0xffff);
                if (r & 0x100000) {
                    p->x += r & 0x3f0000;
                    p->direction_x = 0;
                } else {
                    p->x -= r & 0x3f0000;
                    p->direction_x = 0xff;
                }
                p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
                break;
            default:
                p->state = 1;
                select_frame(p, 0);
                break;
            }
            eng1_interpret_signed_pairs(p);
            q = spawn_normal(p->x - p->motion_x.velocity, p->y, p->z - p->motion_z.velocity, 0x56);
            if (q) {
                g_normal_callbacks[86].initialize(q);
                q->direction_x = p->direction_x;
                q->render_flags = (q->direction_x & 0x10) | (q->render_flags & ~0x10);
                q->motion_x.velocity = p->motion_x.velocity;
                q->motion_z.velocity = p->motion_z.velocity;
                select_frame(q, p->frame_index);
            }
        }
        break;
    }
    if (eng1_native_code_target_002aa30(p, 0x200, 0x200))
        remove_actor(p);
}

void eng1_native_code_target_0090c0(Actor *p)
{
    p->flags = 2;
    advance_frame(p);
    p->z -= 0x40000;
    p->flags |= 0x20000;
    p->render_flags |= 5;
}

void eng1_native_code_target_009100(Actor *p)
{
    if (!p->direction_x)
        p->x += p->motion_x.velocity;
    else
        p->x -= p->motion_x.velocity;
    p->z += p->motion_z.velocity;
    if (p->motion_z.velocity) {
        if (p->frame_state >= 9) {
            p->motion_x.velocity /= 2;
            p->motion_z.velocity /= 2;
        }
    } else if (p->motion_x.velocity && p->frame_state >= 0x11) {
        p->motion_x.velocity /= 2;
        p->motion_z.velocity = 0;
    }
    if (eng1_interpret_signed_pairs(p))
        remove_actor(p);
}

void eng1_native_code_target_009190(Actor *p)
{
    p->flags = 0x100202;
    p->state = 1;
    p->hits = 7;
    p->lane = (unsigned char)(((p->z >> 16) + 0x40) >> 7);
    p->direction_x = 0xff;
    p->render_flags |= 0x10;
    advance_frame(p);
    eng1_native_code_target_002a290(p);
    attach_actor(p, 0x2023);
}

void eng1_091f0(Actor *p)
{
    if (p->hits > 0) {
        stop_actor_sounds(p);
        eng1_1ab50();
        play_sound(0x60df, p);
        p->state = 0x24;
        select_frame(p, 9);
    } else {
        stop_actor_sounds(p);
        eng1_1ab50();
        play_sound(0x60e0, p);
        p->state = 0x23;
        eng1_131c0(p->x, p->y, p->z, 0x0a002041);
        eng1_131c0(p->x, p->y, p->z, 0x14002021);
        eng1_131c0(p->x, p->y, p->z, 0x1e002042);
        select_frame(p, 0x0b);
        p->flags &= ~0x200;
    }
    eng1_interpret_signed_pairs(p);
}

void eng1_native_code_target_0092c0(Actor *p)
{
    Bounds b;
    long d, distance;

    switch (p->state) {
    case 1:
        d = p->x - *(long *)(g_kind_info[p->profile].owner + p->record_offset);
        if (!g_engine_interface.context->unknown_024 ||
            (d < -0x1400000 && p->direction_x == 0xff) ||
            (d > 0x1400000 && !p->direction_x)) {
            p->state = 0x47;
            select_frame(p, 1);
            eng1_interpret_signed_pairs(p);
            eng1_interpret_signed_pairs(p);
            break;
        }
        d = p->x - g_normal_actor_pool->x;
        distance = d < 0 ? -d : d;
        eng1_native_code_target_002a290(p);
        if (p->lane != g_normal_actor_pool->lane || distance >= 0xf00000) {
            p->state = 0x0b;
            select_frame(p, 3);
            eng1_interpret_signed_pairs(p);
        } else if (d >= 0) {
            /* player at lower x: turn when facing +x, else close in */
            if (!p->direction_x) {
                select_frame(p, 1);
                p->state = 9;
                eng1_interpret_signed_pairs(p);
            } else if (distance >= 0x690000) {
                p->state = 10;
                p->motion_x.velocity = 0x30000;
                select_frame(p, 2);
                eng1_interpret_signed_pairs(p);
            } else {
                p->state = 0x15;
                select_frame(p, 4);
                eng1_interpret_signed_pairs(p);
            }
        } else if (p->direction_x == 0xff) {
            select_frame(p, 1);
            p->state = 9;
            eng1_interpret_signed_pairs(p);
        } else if (distance < 0x690000) {
            p->state = 0x15;
            select_frame(p, 4);
            eng1_interpret_signed_pairs(p);
        } else {
            p->state = 10;
            p->motion_x.velocity = 0x30000;
            select_frame(p, 2);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 9:
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 1;
            select_frame(p, 0);
            p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
        }
        break;
    case 0x47:
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 10;
            p->motion_x.velocity = 0x30000;
            select_frame(p, 2);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 10:
        dbg_printf("WCENTAUR WALKING\n");
        walk_x_blocked(p, &b);
        eng1_native_code_target_002a290(p);
        d = p->x - g_normal_actor_pool->x;
        if (d > -0x690000 && d < 0x690000) {
            stop_actor_sounds(p);
            if ((random_limit(0x8000) & 8) != 8) {
                p->state = 0x16;
                select_frame(p, 6);
                eng1_interpret_signed_pairs(p);
            } else {
                p->state = 0x17;
                select_frame(p, 0x10);
            }
        }
        if (eng1_interpret_signed_pairs(p))
            p->state = 1;
        else if (p->frame_state > 0x13)
            p->motion_x.velocity /= 2;
        break;
    case 11:
        if (eng1_interpret_signed_pairs(p)) {
            distance = p->x - g_normal_actor_pool->x;
            if (distance < 0)
                distance = -distance;
            if (p->lane != g_normal_actor_pool->lane || distance >= 0xf00000) {
                p->state = 9;
                select_frame(p, 1);
                eng1_interpret_signed_pairs(p);
            } else {
                p->state = 1;
                select_frame(p, 0);
                eng1_interpret_signed_pairs(p);
            }
        }
        break;
    case 0x15:
        eng1_interpret_signed_pairs(p);
        if ((random_limit(0x8000) & 8) != 8) {
            p->state = 0x16;
            select_frame(p, 6);
            eng1_interpret_signed_pairs(p);
        } else {
            p->state = 0x17;
            select_frame(p, 0x10);
        }
        break;
    case 0x16:
        eng1_interpret_signed_pairs(p);
        if (p->frame_index != 6) {
            p->state = 1;
            select_frame(p, 0);
        }
        break;
    case 0x17:
        eng1_interpret_signed_pairs(p);
        if (p->frame_index != 0x10) {
            p->state = 1;
            select_frame(p, 0);
        }
        break;
    case 0x24:
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 1;
            select_frame(p, 0);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 0x23:
        eng1_interpret_signed_pairs(p);
        if (p->frame_ticks > 0x62)
            p->frame_ticks = 0xc7;
        break;
    }
    if (p->state != 0x24 && p->state != 0x23) {
        if (actor_bounds_2a880(p, &b, 2) && bounds_overlap(&g_collision_context->bounds, &b)) {
            if (p->state == 0x16)
                eng1_native_code_target_00266a0(p, 4, 0x67);
            else
                eng1_native_code_target_00266a0(p, 4, 0x66);
        } else if (eng1_2a710(&g_collision_context->bounds, p)) {
            p->x = p->saved_x;
            eng1_native_code_target_00266a0(p, 0x22, 0);
        } else if (eng1_26e90(p, 2, 0))
            eng1_091f0(p);
    }
    eng1_29c20(p);
    if (eng1_native_code_target_002aa30(p, 0x200, 0x200)) {
        if (p->state == 0x23)
            remove_actor_preserving_resource(p);
        else
            remove_actor(p);
    }
}

void eng1_native_code_target_009850(Actor *p)
{
    p->flags = 0x212;
    p->state = 1;
    reset_frame_state(p);
    eng1_native_code_target_002a290(p);
    attach_actor(p, 0x2024);
}

void eng1_native_code_target_009890(Actor *p)
{
    switch (p->state) {
    case 1:
    case 13:
        eng1_interpret_word_stream(p);
        break;
    case 2:
    case 61:
        if (eng1_2bc50(p) & 2) {
            p->state = 0x3c;
            p->counter = 0x14;
            break;
        }
        if (eng1_2a710(&g_collision_context->bounds, p)) {
            play_sound(0x8ce, p);
            eng1_native_code_target_00266a0(p, 3, 0x66);
            g_eng1_setup_word_6ef90 = 1;
            p->state = 0x3d;
            eng1_0ceb0(p);
            remove_actor_preserving_resource(p);
            return;
        }
        eng1_interpret_word_stream(p);
        break;
    case 60:
        p->y += 0x20000;
        if (p->counter-- < 0) {
            remove_actor(p);
            return;
        }
        break;
    }
    if (eng1_native_code_target_002aa30(p, 200, 200))
        remove_actor(p);
}

void eng1_native_code_target_0099e0(Actor *p)
{
    p->flags = 0x202;
    p->state = 0x3d;
    reset_frame_state(p);
    eng1_interpret_word_stream(p);
    attach_actor(p, 0x2024);
    p->motion_z.velocity = 0x60000;
}

void eng1_native_code_target_009a20(Actor *p)
{
    Bounds b;
    long range;
    Actor *q;

    dbg_printf(" LCROCK->NAStatus = %d\n", p->state);
    p->z -= p->motion_z.velocity;
    range = (g_normal_actor_pool->z - p->z) >> 16;
    dbg_printf(" Range is %d\n", range);
    if (range > 0x100 || range < -0x200) {
        remove_actor(p);
        return;
    }
    p->y += p->motion_y.velocity;
    walk_x_blocked(p, &b);
    p->motion_y.velocity += 0x10000;
    if ((p->y >> 16) > g_eng1_setup_word_718e0) {
        q = spawn_normal(p->x, p->y, p->z, 0x49);
        if (q) {
            g_normal_callbacks[73].initialize(q);
            q->scale = 0x180;
        }
        play_sound(0xcb, 0);
        remove_actor(p);
        return;
    }
    if (eng1_29e20(p)) {
        p->motion_y.velocity = -(p->motion_y.velocity / 2);
        play_sound(0xd0d0, p);
    }
    if (g_pipeline_word_6fec4 && (g_eng1_setup_word_72012 & 0x1b) && eng1_2a710(&g_actor_pipeline_storage_71440, p)) {
        if (g_eng1_setup_word_72012 & 0x13) {
            eng1_1aba0();
            play_sound(0xce, 0);
            p->motion_z.velocity = -0x58000;
            p->motion_x.velocity = -p->motion_x.velocity;
            p->motion_y.velocity = -p->motion_y.velocity;
            g_eng1_setup_word_72012 = 0;
        } else {
            eng1_1aba0();
            play_sound(0x8cf, p);
            eng1_0cfb0(p);
            remove_actor_preserving_resource(p);
            return;
        }
    } else if (eng1_2a710(&g_collision_context->bounds, p)) {
        play_sound(0xcf, 0);
        eng1_native_code_target_00266a0(p, 3, 0x66);
        g_eng1_setup_word_6ef90 = 1;
        p->state = (p->y >> 16) > g_eng1_setup_word_718e0 ? 0x3e : 1;
        eng1_0cfb0(p);
        remove_actor_preserving_resource(p);
        return;
    }
    if (p->motion_y.velocity > 0) {
        q = eng1_083e0(p, 0x28, -0x500000, 0x500000);
        if (q) {
            play_sound(0xd0cd, p);
            play_sound(0xe0e0, p);
            eng1_0cfb0(p);
            eng1_08630(q);
            remove_actor_preserving_resource(p);
            return;
        }
    }
    eng1_interpret_word_stream(p);
}

void eng1_09db0(Actor *p)
{
    long x = p->x;
    long y = p->y;
    long z = p->z;
    int i;
    Actor *q;

    play_sound(0xcc, 0);
    for (i = 0; i < 13; i++) {
        q = spawn_middle_277e0((g_debris_4fb50[i].x << 16) + x, (g_debris_4fb50[i].y << 16) + y,
                               (g_debris_4fb50[i].z << 16) + z, g_debris_4fb50[i].kind);
        if (q) {
            g_special_callbacks[65].initialize(q);
            q->state = 0x3e;
        }
    }
    spawn_at_group_300c0(p->x, p->y - 0x300000, p->z - 0x100000, 0x2027, 0);
    spawn_at_group_300c0(p->x + 0x280000, p->y - 0x200000, p->z - 0x100000, 0x2027, 0);
    spawn_at_group_300c0(p->x - 0x280000, p->y - 0x100000, p->z - 0x100000, 0x2027, 0);
}

void eng1_09ec0(Actor *p)
{
    long x = (unsigned long)(p->x + g_normal_actor_pool->x) / 2;  /* unsigned in the original (shr) */
    long y = p->y;
    long z = p->z;
    int i, r;
    Actor *q;

    play_sound(0xcc, 0);
    for (i = 0; i < 4; i++) {
        r = random_limit(12);
        q = spawn_middle_277e0((g_debris_4fbb8[r].x << 16) + x, (g_debris_4fbb8[r].y << 16) + y,
                               (g_debris_4fbb8[r].z << 16) + z, g_debris_4fbb8[r].kind);
        if (q) {
            g_special_callbacks[65].initialize(q);
            q->state = 0x3e;
            attach_actor(q, 0x2024);
        }
    }
    spawn_at_group_300c0((p->x + g_normal_actor_pool->x) / 2, p->y, p->z - 0x300000, 0x2027, 0);
}

void eng1_native_code_target_009fb0(Actor *p)
{
    p->flags = 0x100002;
    p->render_flags |= 0x10000000;
    p->spin_x = 3;
    p->counter = 0;
    p->state = 1;
    reset_frame_state(p);
    p->y = (g_eng1_setup_word_718e0 - 0x20) << 16;
    p->scale = 0x100;
    p->hits = 0x32;
}

void eng1_native_code_target_00a010(Actor *p)
{
    unsigned long hit;
    long step;
    int mask;

    if (p->counter) {
        step = p->counter--;
        p->y += g_bob_steps_4fc18[step];
    }
    hit = eng1_26de0(p);
    if (hit) {
        if (hit == 0x80) {
            mask = g_eng1_setup_word_72012 & 0x5f & ~g_native_word_72168;
            if (!mask)
                goto done;
            eng1_1aba0();
            g_native_word_72168 |= mask;
            if ((g_eng1_setup_word_72012 & 2) && g_eng1_setup_word_7082e > 200) {
                play_sound(0xcf, 0);
                eng1_09db0(p);
                g_eng1_setup_word_6ef90 = 1;
                remove_actor_preserving_resource(p);
                return;
            }
        } else {
            eng1_30290(hit, 0);
            if (p->y != p->saved_y)
                goto done;
        }
        if (--p->hits <= 0) {
            play_sound(0xcf, 0);
            eng1_09db0(p);
            g_eng1_setup_word_6ef90 = 1;
            remove_actor_preserving_resource(p);
            return;
        }
        play_sound(0xce, 0);
        eng1_09ec0(p);
        g_eng1_setup_word_72012 = 0;
        p->counter = 5;
    } else
        eng1_interpret_word_stream(p);
done:
    if (eng1_native_code_target_002aa30(p, 0x200, 0x200))
        remove_actor(p);
}

void eng1_0a140(Actor *p)
{
    long x = p->x;
    long y = p->y;
    long z = p->z;
    int i;
    Actor *q;

    play_sound(0xcd, 0);
    for (i = 0; i < 13; i++) {
        q = spawn_middle_277e0((g_debris_4fb50[i].x << 16) + x, (g_debris_4fb50[i].y << 16) + y,
                               (g_debris_4fb50[i].z << 16) + z, g_debris_4fb50[i].kind);
        if (q) {
            g_special_callbacks[40].initialize(q);
            q->state = 1;
            q->direction_x = 0;
            q->motion_x.velocity = (random_limit(0x600) - 0x300) << 8;
            q->motion_y.velocity = (-random_limit(0xa00) - 0x200) << 8;
            q->motion_z.velocity = (random_limit(0x1000) - 0x800) << 8;
        }
    }
    spawn_at_group_300c0(p->x, p->y - 0x300000, p->z - 0x100000, 0x2027, 0);
    spawn_at_group_300c0(p->x + 0x280000, p->y - 0x200000, p->z - 0x300000, 0x2027, 0);
    spawn_at_group_300c0(p->x - 0x280000, p->y - 0x100000, p->z - 0x300000, 0x2027, 0);
}

void eng1_0a2c0(Actor *p)
{
    long x = (p->x + g_normal_actor_pool->x) / 2;
    long y = p->y;
    long z = p->z;
    int i, r;
    Actor *q;

    play_sound(0xcd, 0);
    for (i = 0; i < 2; i++) {
        r = random_limit(12);
        q = spawn_middle_277e0((g_debris_4fbb8[r].x << 16) + x, (g_debris_4fbb8[r].y << 16) + y,
                               (g_debris_4fbb8[r].z << 16) + z, g_debris_4fbb8[r].kind);
        if (q) {
            g_special_callbacks[40].initialize(q);
            q->state = 1;
            q->direction_x = 0;
            q->motion_x.velocity = (random_limit(0x500) - 0x280) << 8;
            q->motion_y.velocity = (-random_limit(0x480) - 0x180) << 8;
            q->motion_z.velocity = (random_limit(0xc00) - 0x600) << 8;
            attach_actor(q, 0x2023);
        }
    }
    spawn_at_group_300c0((p->x + g_normal_actor_pool->x) / 2, p->y, p->z - 0x300000, 0x2027, 0);
}

void eng1_native_code_target_00a420(Actor *p)
{
    p->flags = 0x100002;
    p->render_flags |= 0x10000000;
    p->spin_x = 3;
    p->lane = (unsigned char)(((p->z >> 16) + 0x40) >> 7);
    eng1_native_code_target_002a290(p);
    p->counter = 0;
    p->y -= 0x400000;
    p->state = 1;
    reset_frame_state(p);
    p->scale = 0x100;
    p->hits = 0x32;
}

void eng1_native_code_target_00a490(Actor *p)
{
    unsigned long hit;
    long step;
    int mask;

    if (p->lane != g_normal_actor_pool->lane) {
        eng1_interpret_word_stream(p);
        return;
    }
    if (p->counter) {
        step = p->counter--;
        p->y += g_bob_steps_4fc18[step];
    }
    if ((abs(g_collision_context->x - p->x) & ~0xffff) > 0x9f0000)
        p->hits = 0x32;
    hit = eng1_26de0(p);
    if (hit) {
        if (hit == 0x80) {
            mask = g_eng1_setup_word_72012 & 0x5f & ~g_native_word_72168;
            if (!mask)
                goto done;
            eng1_1aba0();
            g_native_word_72168 |= mask;
            if ((g_eng1_setup_word_72012 & 2) && g_eng1_setup_word_7082e > 200) {
                play_sound(0xcf, 0);
                eng1_0a140(p);
                g_eng1_setup_word_6ef90 = 1;
                remove_actor_preserving_resource(p);
                return;
            }
        } else {
            eng1_30290(hit, 0);
            if (p->y != p->saved_y)
                goto done;
        }
        if (--p->hits <= 0) {
            play_sound(0xcf, 0);
            eng1_0a140(p);
            g_eng1_setup_word_6ef90 = 1;
            remove_actor_preserving_resource(p);
            return;
        }
        play_sound(0xce, 0);
        eng1_0a2c0(p);
        g_eng1_setup_word_72012 = 0;
        p->counter = 5;
    } else
        eng1_interpret_word_stream(p);
done:
    if (eng1_native_code_target_002aa30(p, 0x200, 0x200))
        remove_actor(p);
}

void eng1_native_code_target_00a600(Actor *p)
{
    p->direction_x = 0;
    p->render_flags &= ~0x10;
    advance_frame(p);
    if (p->x == 0x3ac0000) {
        p->counter = 1;
        p->state = 0x9b;
        select_frame(p, 9);
    } else
        eng1_native_code_target_002a290(p);
    attach_actor(p, 0x2023);
}

void eng1_native_code_target_00a670(Actor *p)
{
    Bounds b;

    if (p->state & 0x80) {
        p->state ^= 0x80;
        eng1_084d0(p);
        eng1_native_code_target_00a670(p);
        if (p->state)
            p->state ^= 0x80;
        return;
    }
    p->lane = (unsigned char)(((p->z >> 16) + 0x40) >> 7);
    switch (p->state) {
    case 0x1c:
        eng1_2bc50(p);
        if (p->direction_y) {
            eng1_native_code_target_002a290(p);
            p->frame_ticks = 0;
            p->state = 0x1d;
        }
        break;
    case 0x1d:
        if (p->frame_index != 2) {
            p->state = 1;
            select_frame(p, 0);
        }
        break;
    case 0x1f:
        p->z += p->motion_z.velocity;
        walk_x_blocked(p, &b);
        eng1_29c20(p);
        break;
    }
    eng1_interpret_signed_pairs(p);
    if (eng1_native_code_target_002aa30(p, 0x140, 0x140)) {
        if (p->state == 0x1f && p->lane)
            remove_actor_preserving_resource(p);
        else
            remove_actor(p);
        if (g_slots_5cef0[0x2f].state_00 == 3)
            resource_cancel(0x2f);
    }
}

void eng1_native_code_target_00a870(Actor *p)
{
    p->flags = 0x202;
    p->state = 1;
    p->hits = 100;
    p->lane = (unsigned char)(((p->z >> 16) + 0x40) >> 7);
    p->direction_x = 0xff;
    p->render_flags |= 0x10;
    advance_frame(p);
    eng1_native_code_target_002a290(p);
    attach_actor(p, 0x2023);
}

void eng1_0a8d0(Actor *p)
{
    p->hits -= 0x19;
    if (p->hits > 0) {
        p->state = 0x24;
        select_frame(p, 4);
    } else {
        p->state = 0x23;
        select_frame(p, 5);
    }
}

void eng1_native_code_target_00a910(Actor *p)
{
    Bounds b;
    Actor *q;
    long d, distance;

    switch (p->state) {
    case 1:
        d = p->x - g_normal_actor_pool->x;
        distance = d < 0 ? -d : d;
        if (p->lane == g_normal_actor_pool->lane && distance < 0xf00000) {
            if (d >= 0) {
                if (!p->direction_x) {
                    select_frame(p, 1);
                    p->state = 9;
                    eng1_interpret_signed_pairs(p);
                } else {
                    p->state = 0x28;
                    select_frame(p, 6);
                    eng1_interpret_signed_pairs(p);
                }
            } else {
                if (p->direction_x == 0xff) {
                    select_frame(p, 1);
                    p->state = 9;
                    eng1_interpret_signed_pairs(p);
                } else {
                    p->state = 0x28;
                    select_frame(p, 6);
                    eng1_interpret_signed_pairs(p);
                }
            }
        } else {
            p->state = 0x0b;
            select_frame(p, 3);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 9:
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 1;
            select_frame(p, 0);
            p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
        }
        break;
    case 10:
        walk_x_blocked(p, &b);
        eng1_native_code_target_002a290(p);
        if (!eng1_interpret_signed_pairs(p))
            break;
        goto idle;
    case 11:
        eng1_interpret_signed_pairs(p);
        if (p->frame_index != 3) {
            distance = p->x - g_normal_actor_pool->x;
            if (distance < 0)
                distance = -distance;
            if (p->lane != g_normal_actor_pool->lane || distance >= 0xf00000) {
                p->state = 9;
                select_frame(p, 1);
            } else {
                p->state = 1;
                select_frame(p, 0);
            }
        }
        break;
    case 0x28:
        eng1_interpret_signed_pairs(p);
        if (p->frame_index != 6) {
            p->state = 0x29;
            select_frame(p, 7);
            eng1_interpret_signed_pairs(p);
            q = spawn_normal(p->x, p->y + 0x60000, p->z, 0x30);
            if (q) {
                g_normal_callbacks[48].initialize(q);
                q->direction_x = p->direction_x;
                q->render_flags = (q->direction_x & 0x10) | (q->render_flags & ~0x10);
                q->motion_x.velocity = 0x200000;
                walk_x_blocked(q, &b);
            }
        }
        break;
    case 0x24:
    case 0x29:
        if (eng1_interpret_signed_pairs(p)) {
idle:
            p->state = 1;
            select_frame(p, 0);
        }
        break;
    case 0x23:
        eng1_interpret_signed_pairs(p);
        if (p->frame_ticks > 0x62)
            p->frame_ticks = 0xc7;
        break;
    }
    if (p->state != 0x24 && p->state != 0x23) {
        if (actor_bounds_2a880(p, &b, 2) && bounds_overlap(&g_collision_context->bounds, &b))
            eng1_native_code_target_00266a0(p, 10, 0);
        else if (!eng1_2a710(&g_collision_context->bounds, p) &&
                 ((g_pipeline_word_6fec4 && eng1_2a710(&g_actor_pipeline_storage_71440, p)) || eng1_26de0(p)))
            eng1_0a8d0(p);
    }
    eng1_29c20(p);
    eng1_29c20(p);
    if (eng1_native_code_target_002aa30(p, 200, 200)) {
        if (p->state == 0x23)
            remove_actor_preserving_resource(p);
        else
            remove_actor(p);
    }
}

void eng1_native_code_target_00ae00(Actor *p)
{
    unsigned short height;

    p->flags = 0x202;
    p->lane = (unsigned char)(((p->z >> 16) + 0x40) >> 7);
    p->counter = 0;
    advance_frame(p);
    height = (unsigned short)eng1_2a010(p->x, p->y, p->z, p->profile);
    if (height != 0x7fff && height > 0x80) {
        if (!(g_eng1_collision_map_04f1e0[(unsigned short)grid_query_20d30((p->x >> 16) - 0x20, (p->y >> 16) + 0x80,
                                                                          p->z >> 16, p->profile)] & 2))
            p->direction_x = 0xff;
        else
            p->direction_x = 0;
        p->state = 0x1b;
        select_frame(p, 0);
    } else {
        p->direction_x = 0;
        eng1_native_code_target_002a290(p);
        p->y -= 0x80000;
        p->state = 0x26;
        select_frame(p, 4);
        eng1_interpret_signed_pairs(p);
    }
    p->render_flags = (p->render_flags & ~0x10) | (p->direction_x & 0x10);
    attach_actor(p, 0x2023);
    eng1_interpret_signed_pairs(p);
}

void eng1_0af20(Actor *p, long dx)
{
    int r = random_limit(0x8000);
    int lanes = p->lane - g_normal_actor_pool->lane;
    long dy = p->y - g_normal_actor_pool->y;
    long home = p->x - *(long *)(g_kind_info[p->profile].owner + p->record_offset);
    unsigned short gx, gy, gz;

    if ((home > 0x2800000 && p->x < g_normal_actor_pool->x) ||
        (home < -0x2800000 && p->x > g_normal_actor_pool->x)) {
        p->state = 0x2d;
        p->motion_y.velocity = 0;
        p->motion_y.rate = -0x8000;
        select_frame(p, 2);
        eng1_interpret_signed_pairs(p);
        return;
    }
    p->motion_y.velocity = 0x20000 - (random_limit(0x400) << 8);
    if (dy >= -0x100000)
        p->motion_y.velocity -= 0x40000;
    if (dy <= -0x800000)
        p->motion_y.velocity += 0x40000;
    if (p->counter) {
        if (p->counter == 2) {
            p->state = 2;
            select_frame(p, 2);
            p->counter = 0;
            eng1_interpret_signed_pairs(p);
            return;
        }
        p->counter = 0;
    }
    gx = p->x >> 16;
    gy = p->y >> 16;
    gz = p->z >> 16;
    if (!p->direction_x)
        gx += 0x40;
    else
        gx -= 0x40;
    if (g_eng1_collision_map_04f1e0[grid_query_20d30(gx, gy, gz, p->profile)] & 2) {
        p->state = 9;
        select_frame(p, 3);
        p->counter = 2;
        eng1_interpret_signed_pairs(p);
        return;
    }
    if (lanes && r > 0x4000) {
        if (lanes > 0) {
            gx = p->x >> 16;
            gy = p->y >> 16;
            gz = (p->z >> 16) - 1;
            if (!(g_eng1_collision_map_04f1e0[grid_query_20d30(gx, gy, gz, 0)] & 2)) {
                if (!p->direction_x)
                    gx += 0x40;
                else
                    gx -= 0x40;
                if (!(g_eng1_collision_map_04f1e0[grid_query_20d30(gx, gy, gz, 0)] & 2)) {
                    p->state = 0x2b;
                    select_frame(p, 9);
                    eng1_interpret_signed_pairs(p);
                    return;
                }
            }
        } else {
            gx = p->x >> 16;
            gy = p->y >> 16;
            gz = (p->z >> 16) + 1;
            if (!(g_eng1_collision_map_04f1e0[grid_query_20d30(gx, gy, gz, 0)] & 2)) {
                if (!p->direction_x)
                    gx++;
                else
                    gx--;
                if (!(g_eng1_collision_map_04f1e0[grid_query_20d30(gx, gy, gz, 0)] & 2)) {
                    p->state = 0x2c;
                    select_frame(p, 10);
                    eng1_interpret_signed_pairs(p);
                    return;
                }
            }
        }
    }
    if (!p->direction_x) {
        if (dx > 0x3aaaaa) {
            p->state = 9;
            select_frame(p, 3);
        } else if (!(r & 6)) {
            p->state = 0x27;
            select_frame(p, 7);
        } else if (dx < -0x3aaaaa && !lanes && dx > -0xb00000) {
            p->state = 0x22;
            select_frame(p, 5);
        } else if (!(r & 8)) {
            p->state = 0x27;
            select_frame(p, 7);
        } else {
            p->state = 2;
            select_frame(p, 2);
        }
    } else {
        if (dx < -0x3aaaaa) {
            p->state = 9;
            select_frame(p, 3);
        } else if (!(r & 6)) {
            p->state = 0x27;
            select_frame(p, 7);
        } else if (dx > 0x3aaaaa && !lanes && dx < 0xb00000) {
            p->state = 0x22;
            select_frame(p, 5);
        } else if (!(r & 8)) {
            p->state = 0x27;
            select_frame(p, 7);
        } else {
            p->state = 2;
            select_frame(p, 2);
        }
    }
    eng1_interpret_signed_pairs(p);
}

void eng1_0b250(Actor *p)
{
    Bounds b;

    p->y += p->motion_y.velocity;
    walk_x_blocked(p, &b);
}

void eng1_0b340(Actor *p)
{
    stop_actor_sounds(p);
    spawn_at_group_300c0(p->x, p->y, p->z - 0x80000, 0x2027, 0);
    p->state = 0x23;
    select_frame(p, 6);
    eng1_interpret_signed_pairs(p);
    if (p->motion_x.velocity > 0x30000)
        p->motion_x.velocity = 0x30000;
    if (p->motion_x.velocity < -0x30000)
        p->motion_x.velocity = -0x30000;
    eng1_0cdd0(p);
    if (random_limit(0x8000) > 0x4000)
        eng1_131c0(p->x, p->y, p->z, 1);
}

void eng1_native_code_target_00b3f0(Actor *p)
{
    Bounds b;
    long dx, distance;

    if (p->state != 0x26 && p->state != 0x1b && p->state != 0x21)
        eng1_08480(p, 0x28);
    dx = p->x - g_normal_actor_pool->x;
    distance = dx < 0 ? -dx : dx;
    switch (p->state) {
    case 0x1b:
        if (p->lane == g_normal_actor_pool->lane && distance < random_limit(0x600000) + 0xf00000) {
            eng1_0d0c0(p);
            p->state = 0x22;
            select_frame(p, 5);
            p->frame_state = 10;
            eng1_interpret_signed_pairs(p);
        } else {
            p->frame_ticks = 0x63;
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 0x21:
        if (p->frame_state > 5) {
            if (p->frame_state > 9)
                p->motion_x.velocity = 0x80000;
            else if (p->frame_state == 6) {
                p->motion_y.velocity = -0x18000;
                p->motion_x.velocity = 0x20000;
            } else if (p->frame_state == 7) {
                p->motion_y.velocity = -0x30000;
                p->motion_x.velocity = 0x2aaaa;
            } else if (p->frame_state >= 8) {
                p->motion_y.velocity = -0x60000;
                p->motion_x.velocity = 0x40000;
            } else
                p->motion_x.velocity = 0x60000;
            p->y += p->motion_y.velocity;
            walk_x_blocked(p, &b);
        }
        if (eng1_interpret_signed_pairs(p)) {
            p->motion_y.velocity = 0;
            p->state = 2;
            p->motion_x.velocity = 0x80000;
            select_frame(p, 2);
            p->frame_state = 2;
        }
        break;
    case 2:
    case 0x27:
        if (!eng1_interpret_signed_pairs(p))
            eng1_0b250(p);
        else
            eng1_0af20(p, dx);
        break;
    case 9:
        if (p->frame_state > 0x0d) {
            p->motion_x.velocity += 0x8000;
            if (p->motion_x.velocity > 0x80000)
                p->motion_x.velocity = 0x80000;
        } else {
            p->motion_x.velocity -= 0x8000;
            if (p->motion_x.velocity < 0)
                p->motion_x.velocity = 0;
        }
        eng1_0b250(p);
        if (eng1_interpret_signed_pairs(p))
            eng1_0af20(p, dx);
        break;
    case 0x22:
        if (p->frame_state < 10) {
            p->motion_x.velocity -= 0x4000;
            if (p->motion_x.velocity < 0x40000)
                p->motion_x.velocity = 0x40000;
        } else if (p->frame_state < 0x10) {
            p->motion_x.velocity += 0x10000;
            if (p->motion_x.velocity > 0x100000)
                p->motion_x.velocity = 0x100000;
            p->motion_y.velocity += 0x10000;
            if (p->motion_y.velocity > 0xc0000)
                p->motion_y.velocity = 0x60000;
        } else if (p->frame_state < 0x15) {
            p->motion_x.velocity -= 0x8000;
            if (p->motion_x.velocity < 0x40000)
                p->motion_x.velocity = 0x40000;
            p->motion_y.velocity -= 0x10000;
            if (p->motion_y.velocity < -0x60000)
                p->motion_y.velocity = -0x60000;
        } else {
            p->motion_x.velocity += 0x8000;
            if (p->motion_x.velocity > 0x80000)
                p->motion_x.velocity = 0x80000;
        }
        if (p->frame_index != 5) {
            p->state = 2;
            p->motion_x.velocity = 0x80000;
            p->motion_y.velocity = 0;
            select_frame(p, 2);
        }
        eng1_0b250(p);
        eng1_interpret_signed_pairs(p);
        break;
    case 0x23:
        eng1_0b250(p);
        eng1_08480(p, 0x28);
        if (eng1_interpret_signed_pairs(p)) {
            remove_actor_preserving_resource(p);
            return;
        }
        break;
    case 0x2b:
        if (p->frame_state < 8) {
            p->motion_x.velocity -= 0x8000;
            if (p->motion_x.velocity < 0)
                p->motion_x.velocity = 0;
        } else if (p->frame_state < 0x10) {
            p->z -= 0x80000;
            if (p->frame_state == 8)
                p->motion_x.velocity = 0;
        } else {
            p->motion_x.velocity += 0x8000;
            if (p->motion_x.velocity > 0x80000)
                p->motion_x.velocity = 0x80000;
        }
        eng1_0b250(p);
        eng1_08480(p, 0x28);
        if (eng1_interpret_signed_pairs(p)) {
            p->z = (p->z + 0x400000) & ~0x7f0000;
            p->lane = (unsigned char)(((p->z >> 16) + 0x40) >> 7);
            eng1_0af20(p, dx);
        }
        break;
    case 0x2c:
        if (p->frame_state < 8) {
            p->motion_x.velocity -= 0x8000;
            if (p->motion_x.velocity < 0)
                p->motion_x.velocity = 0;
        } else if (p->frame_state < 0x10) {
            p->z += 0x80000;
            if (p->frame_state == 8)
                p->motion_x.velocity = 0;
        } else {
            p->motion_x.velocity += 0x8000;
            if (p->motion_x.velocity > 0x80000)
                p->motion_x.velocity = 0x80000;
        }
        eng1_0b250(p);
        eng1_08480(p, 0x28);
        if (eng1_interpret_signed_pairs(p)) {
            p->z = (p->z + 0x400000) & ~0x7f0000;
            p->lane = (unsigned char)(((p->z >> 16) + 0x40) >> 7);
            eng1_0af20(p, dx);
        }
        break;
    case 8:
        if (p->frame_state > 0x0d) {
            p->motion_x.velocity += 0x8000;
            if (p->motion_x.velocity > 0x80000)
                p->motion_x.velocity = 0x80000;
        }
        eng1_0b250(p);
        eng1_08480(p, 0x28);
        eng1_interpret_signed_pairs(p);
        if (p->frame_ticks > 0x63)
            p->frame_ticks = 0xc7;
        break;
    case 0x26:
        p->state = 0x21;
        select_frame(p, 1);
        eng1_interpret_signed_pairs(p);
        break;
    case 0x2d:
        p->motion_y.velocity += p->motion_y.rate;
        eng1_0b250(p);
        if (eng1_native_code_target_002aa30(p, 100, 0x3c)) {
            remove_actor(p);
            return;
        }
        eng1_interpret_signed_pairs(p);
        break;
    default:
        p->state = 1;
        select_frame(p, 0);
        break;
    }
    if (p->state != 0x23) {
        if (eng1_26e90(p, 2, 1))
            eng1_0b340(p);
        else if (eng1_2a710(&g_collision_context->bounds, p)) {
            if (p->state == 5)
                eng1_native_code_target_00266a0(p, 0x14, 0);
            else
                eng1_native_code_target_00266a0(p, 0x12, 0);
        }
    }
    if (eng1_native_code_target_002aa30(p, 0x1f4, 0x140))
        remove_actor(p);
}

void eng1_native_code_target_00baf0(Actor *p)
{
    reset_frame_state(p);
    p->render_flags |= 4;
    p->scale = 0x100;
}

void eng1_native_code_target_00bb10(Actor *p)
{
    p->flags = 0x202;
    p->state = 0x37;
    p->counter = 2;
    p->direction_x = 0xff;
    p->render_flags |= 0x10;
    g_eng1_setup_dword_72118 = (unsigned long)p;
    advance_frame(p);
    eng1_interpret_signed_pairs(p);
    attach_actor(p, 0x2024);
    g_eng1_setup_dword_72120 = 0;
    g_eng1_dword_721b4 = 0;
    resource_reset(0x45);
    g_eng1_setup_dword_72188 = 5;
}

void eng1_0bb90(Actor *p)
{
    long x;

    g_normal_actor_pool->y = p->y - 0x3c0000;
    x = p->x;
    g_normal_actor_pool->direction_x = p->direction_x;
    g_normal_actor_pool->direction_x = ~g_normal_actor_pool->direction_x;
    g_collision_context->render_flags = (g_collision_context->direction_x & 0x10) | (g_collision_context->render_flags & ~0x10);
    if (p->direction_x == 0xff) {
        x += 0x440000;
        g_normal_actor_pool->motion_x.velocity = -0x80000;
    } else {
        x -= 0x440000;
        g_normal_actor_pool->motion_x.velocity = 0x80000;
    }
    g_normal_actor_pool->x = x;
    g_normal_actor_pool->motion_y.velocity = -0x30000;
    eng1_17340();
}

void eng1_native_code_target_00bc50(Actor *p)
{
    Bounds wall, b;
    Actor *q;
    long dx, distance;
    unsigned short gx, gy, gz;

    dx = p->x - g_normal_actor_pool->x;
    distance = dx < 0 ? -dx : dx;
    switch (p->state) {
    case 0x37:
        p->x -= 0x800000;
        eng1_native_code_target_002a290(p);
        p->x += 0x800000;
        eng1_interpret_signed_pairs(p);
        if (p->counter-- <= 0) {
            p->state = 1;
            select_frame(p, 0);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 1:
        if (g_engine_interface.context->unknown_024 <= 0) {
            p->state = 0x44;
            p->motion_x.velocity = -0x30000;
            select_frame(p, 0x0b);
            resource_cancel(0x45);
            resource_cancel(0x46);
            resource_reset(0x47);
            break;
        }
        if (dx >= 0) {
            if (!p->direction_x) {
                select_frame(p, 1);
                eng1_interpret_signed_pairs(p);
                p->state = 9;
                eng1_interpret_signed_pairs(p);
            } else if (distance >= 0x500000) {
                p->state = 10;
                p->motion_x.velocity = 0x60000;
                select_frame(p, 2);
                eng1_interpret_signed_pairs(p);
                eng1_interpret_signed_pairs(p);
            } else {
                if (!g_eng1_dword_721b4) {
                    if (g_slots_5cef0[0x45].state_00 != 3) {
                        p->state = 1;
                        select_frame(p, 0);
                        eng1_interpret_signed_pairs(p);
                        break;
                    }
                    p->state = 0x16;
                    select_frame(p, 10);
                } else {
                    if (g_slots_5cef0[0x46].state_00 != 3) {
                        p->state = 1;
                        select_frame(p, 0);
                        eng1_interpret_signed_pairs(p);
                        break;
                    }
                    p->state = 0x46;
                    select_frame(p, 5);
                }
                p->counter = random_limit(3) + g_engine_interface.context->unknown_00e + 1;
                eng1_interpret_signed_pairs(p);
                eng1_interpret_signed_pairs(p);
            }
        } else {
            if (p->direction_x == 0xff) {
                select_frame(p, 1);
                p->state = 9;
                eng1_interpret_signed_pairs(p);
            } else if (distance >= 0x500000) {
                p->state = 10;
                p->motion_x.velocity = 0x60000;
                select_frame(p, 2);
                eng1_interpret_signed_pairs(p);
            } else {
                if (!g_eng1_dword_721b4) {
                    if (g_slots_5cef0[0x45].state_00 != 3) {
                        p->state = 1;
                        select_frame(p, 0);
                        eng1_interpret_signed_pairs(p);
                        break;
                    }
                    p->state = 0x16;
                    select_frame(p, 10);
                } else {
                    if (g_slots_5cef0[0x46].state_00 != 3) {
                        p->state = 1;
                        select_frame(p, 0);
                        eng1_interpret_signed_pairs(p);
                        break;
                    }
                    p->state = 0x46;
                    select_frame(p, 5);
                }
                eng1_interpret_signed_pairs(p);
                p->counter = random_limit(3) + g_engine_interface.context->unknown_00e + 1;
                eng1_interpret_signed_pairs(p);
            }
        }
        break;
    case 9:
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 1;
            select_frame(p, 0);
            p->render_flags = (p->direction_x & 0x10) | (p->render_flags & ~0x10);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 10:
        if (walk_x_blocked(p, &wall) || (dx > -0x500000 && dx < 0x500000)) {
            if (!g_eng1_dword_721b4) {
                if (g_slots_5cef0[0x45].state_00 == 3) {
                    p->state = 0x16;
                    select_frame(p, 10);
                    eng1_interpret_signed_pairs(p);
                    p->counter = random_limit(3) + g_engine_interface.context->unknown_00e + 1;
                    break;
                }
            } else if (g_slots_5cef0[0x46].state_00 == 3) {
                p->state = 0x46;
                select_frame(p, 5);
                eng1_interpret_signed_pairs(p);
                p->counter = random_limit(3) + g_engine_interface.context->unknown_00e + 1;
                break;
            }
            p->state = 1;
            select_frame(p, 0);
            eng1_interpret_signed_pairs(p);
        } else if (eng1_interpret_signed_pairs(p)) {
            p->state = 1;
            select_frame(p, 0);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 0x16:
    case 0x46:
        if (eng1_interpret_signed_pairs(p)) {
            gx = p->x >> 16;
            gy = p->y >> 16;
            gz = p->z >> 16;
            if (!p->direction_x)
                gx += 0x40;
            else
                gx -= 0x40;
            if (g_eng1_collision_map_04f1e0[grid_query_20d30(gx, gy, gz, p->profile)] & 2) {
                if ((dx >= 0 && !p->direction_x) || (dx < 0 && p->direction_x == 0xff))
                    p->counter = 0;
                if (p->counter-- <= 0) {
                    p->state = 0x44;
                    p->motion_x.velocity = -0x30000;
                    select_frame(p, 0x0b);
                    resource_cancel(0x45);
                    resource_cancel(0x46);
                    resource_reset(0x47);
                    resource_reset(0x48);
                }
                eng1_interpret_signed_pairs(p);
            } else {
                p->state = 1;
                select_frame(p, 0);
                eng1_interpret_signed_pairs(p);
            }
        }
        break;
    case 0x44:
        walk_x_blocked(p, &wall);
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 0x45;
            select_frame(p, 9);
            eng1_interpret_signed_pairs(p);
            g_eng1_setup_dword_72120 = 1;
        }
        break;
    case 0x45:
        if (g_eng1_setup_dword_72120 == 2) {
            g_eng1_setup_dword_72120 = 0;
            p->state = 0x41;
            select_frame(p, 6);
            eng1_interpret_signed_pairs(p);
            resource_cancel(0x47);
            resource_reset(0x49);
        } else if (eng1_interpret_signed_pairs(p)) {
            p->state = 1;
            select_frame(p, 0);
            eng1_interpret_signed_pairs(p);
            resource_cancel(0x47);
            resource_cancel(0x48);
            g_eng1_dword_721b4 = (g_eng1_dword_721b4 - 1) & 1;
            if (!g_eng1_dword_721b4)
                resource_reset(0x45);
            else
                resource_reset(0x46);
        }
        break;
    case 0x41:
        p->x += 0x20000;
        if (g_engine_interface.context->tick_038 & 1)
            g_boss_threshold_72184 = 1;
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 0x42;
            select_frame(p, 7);
            eng1_interpret_signed_pairs(p);
            resource_cancel(0x48);
            if (g_native_word_70832 <= 0)
                resource_reset(0x4a);
            else {
                g_eng1_dword_721b4 = (g_eng1_dword_721b4 - 1) & 1;
                if (!g_eng1_dword_721b4)
                    resource_reset(0x45);
                else
                    resource_reset(0x46);
            }
        }
        break;
    case 0x42:
        if (p->frame_state == 7)
            eng1_0bb90(p);
        if (eng1_interpret_signed_pairs(p)) {
            p->state = 0x47;
            select_frame(p, 1);
            resource_cancel(0x49);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 0x47:
        if (eng1_interpret_signed_pairs(p)) {
            if (g_native_word_70832 <= 0) {
                p->state = 0x43;
                select_frame(p, 8);
                eng1_interpret_signed_pairs(p);
                resource_cancel(0x44);
                resource_reset(0x4b);
                play_sound(0x610f, p);
                p->counter = 10;
            } else {
                p->state = 1;
                select_frame(p, 0);
            }
        }
        break;
    case 0x43:
        eng1_interpret_signed_pairs(p);
        if ((g_eng1_setup_word_72012 & 2) && g_eng1_setup_word_7082e > 100) {
            if (g_pipeline_word_6fec4 && eng1_2a710(&g_actor_pipeline_storage_71440, p)) {
                p->state = 0x23;
                select_frame(p, 4);
                eng1_interpret_signed_pairs(p);
                resource_cancel(0x4a);
                RemoveFxIdLinks_7f8e(0x10f, p);
                q = spawn_normal(p->x, p->y, p->z, 0x54);
                if (q)
                    g_normal_callbacks[84].initialize(q);
            }
        } else if (p->counter-- < 0) {
            p->counter = random_limit(0x1e) + 0x1e;
            play_sound(0x110, p);
        }
        break;
    case 0x23:
        if (p->frame_state == 10)
            p->z = g_normal_actor_pool->z - 0x100000;
        if (eng1_interpret_signed_pairs(p)) {
            eng1_19240();
            p->state = 0x25;
            select_frame(p, 0x0c);
            eng1_interpret_signed_pairs(p);
        }
        break;
    case 0x25:
        eng1_interpret_signed_pairs(p);
        break;
    }
    if (p->state != 0x24 && p->state != 0x23) {
        if (actor_bounds_2a880(p, &b, 2)) {
            if (bounds_overlap(&g_collision_context->bounds, &b))
                eng1_native_code_target_00266a0(p, 5, 0);
            else if (!(g_engine_interface.context->tick_038 & 3)) {
                gx = p->x >> 16;
                gy = p->y >> 16;
                gz = p->z >> 16;
                if (!p->direction_x)
                    gx += 0x40;
                else
                    gx -= 0x40;
                if (g_eng1_collision_map_04f1e0[grid_query_20d30(gx, gy, gz, p->profile)] & 2) {
                    spawn_at_group_300c0(p->x - 0x200000, p->y - (b.min_y << 13), p->z - 0x100000, 0x2027, 0);
                    g_eng1_setup_word_6ef90 = 1;
                    g_eng1_dword_72178 = 1;
                    play_sound(0x24, 0);
                }
            }
        } else
            eng1_2a710(&g_collision_context->bounds, p);
    }
    eng1_26e90(p, 0, 0);
}

}
