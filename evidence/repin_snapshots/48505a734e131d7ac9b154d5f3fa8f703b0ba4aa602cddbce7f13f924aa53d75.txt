/* ENG3.DLL 0x1e000..0x20010: actor program interpreter, support/floor searches over the
 * solid-actor worklist, box queries, pending-action append, tangent search and the
 * engine-interface slot wrappers (lane y5 candidate).
 * Bodies adapted from the shared ENG1 actor unit (calibration/eng1/actor_unit_282e0.cpp,
 * compiled here as C) and the TITLE wrapper unit (calibration/title/u0c1f0.c).
 * Local views only cover offsets observed in these bodies; names are provisional. */

#include <stdlib.h>

typedef struct Actor Actor;

typedef struct Bounds3D {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} Bounds3D;

struct Actor {
    long x, y, z;                       /* 0x00 */
    unsigned char unknown_00c[0x22 - 0x0c];
    unsigned char category;             /* 0x22 */
    unsigned char unknown_023[2];
    unsigned char state_025;            /* 0x25 */
    unsigned char unknown_026[0x2a - 0x26];
    unsigned short unknown_02a;         /* 0x2a */
    unsigned char unknown_02c[2];
    unsigned short kind;                /* 0x2e */
    short frame_ticks;                  /* 0x30 */
    short frame_state;                  /* 0x32 */
    short active_command;               /* 0x34 */
    short unknown_036;                  /* 0x36 */
    unsigned short frame_group;         /* 0x38 */
    unsigned char unknown_03a[0x4c - 0x3a];
    long interpolation_position;        /* 0x4c */
    unsigned long flags;                /* 0x50 */
    unsigned long render_flags;         /* 0x54 */
    char *unknown_058;                  /* 0x58 */
    Actor *attachment;                  /* 0x5c */
    Actor *child;                       /* 0x60 */
    void *image;                        /* 0x64 */
    void *unknown_068;
    void *buffer;                       /* 0x6c */
    short angle_yaw;                    /* 0x70 */
    short angle_pitch;                  /* 0x72 */
    short angle_roll;                   /* 0x74 */
    unsigned char unknown_076[0x88 - 0x76];
    Bounds3D shape;                     /* 0x88 */
    unsigned char direction_x;          /* 0x94 */
    unsigned char direction_y;
    unsigned char direction_z;
    unsigned char tag;                  /* 0x97 */
    short vertical_step;                /* 0x98 */
    unsigned char unknown_09a[2];
    long prev_x, prev_y, prev_z;        /* 0x9c */
    long unknown_0a8;                   /* 0xa8 */
    unsigned char unknown_0ac[0xe4 - 0xac];
    Actor *prev_scene, *next_scene;     /* 0xe4 */
    void *unknown_0ec;
    void *steps;                        /* 0xf0 */
    long step_index;                    /* 0xf4 */
    long unknown_0f8;                   /* 0xf8 */
    unsigned char unknown_0fc[0x110 - 0xfc];
    void **frames;                      /* 0x110 */
    short frame_index;
    short unknown_116;
    void *initial_steps;
    short unknown_11c, active_step;     /* 0x11e */
    signed char *heights;               /* 0x120 */
    short action_callback_index;        /* 0x124 */
};

typedef struct EngineContext { signed char level; } EngineContext;
typedef void (*ActorCallback)(Actor *);
typedef struct KindAction { ActorCallback init; unsigned long unknown; } KindAction;

typedef union PendingActionGroup {
    long count;
    Actor *entries[1];
} PendingActionGroup;

/* Observed 0x407a0 interface record (ENG3 PC_DLLEngineMain copies 250 dwords): slots used by the
 * wrappers below; argument counts from the call sites, types are physical int views. */
typedef struct EngineInterface {
    unsigned long signature_000;
    EngineContext *context_004;
    unsigned long opaque_008[4];
    int (__cdecl *slot_018)(void);
    int (__cdecl *slot_01c)(int, int);
    int (__cdecl *slot_020)(int, int, int, int, int);
    int (__cdecl *slot_024)(void);
    unsigned long opaque_028[1];
    int (__cdecl *slot_02c)(void);
    int (__cdecl *slot_030)(void);
    int (__cdecl *slot_034)(int);
    unsigned long opaque_038[1];
    int (__cdecl *slot_03c)(void);
    int (__cdecl *slot_040)(int);
    int (__cdecl *slot_044)(int);
    int (__cdecl *slot_048)(int, int);
    int (__cdecl *slot_04c)(int, int);
    int (__cdecl *slot_050)(int, int);
    unsigned long opaque_054[2];
    int (__cdecl *slot_05c)(void);
    int (__cdecl *slot_060)(void);
    int (__cdecl *slot_064)(int, int);
    int (__cdecl *slot_068)(int, int, int, int);
    int (__cdecl *slot_06c)(int);
    unsigned long opaque_070[2];
    int (__cdecl *slot_078)(int, int, int);
    unsigned long opaque_07c[2];
    int (__cdecl *slot_084)(void);
    int (__cdecl *slot_088)(void);
    int (__cdecl *slot_08c)(void);
    int (__cdecl *slot_090)(void);
    int (__cdecl *slot_094)(int);
    int (__cdecl *slot_098)(void);
    unsigned long opaque_09c[1];
    int (__cdecl *slot_0a0)(int, int, int, int);
    unsigned long opaque_0a4[3];
    int (__cdecl *slot_0b0)(int);
    int (__cdecl *slot_0b4)(void);
    int (__cdecl *slot_0b8)(int);
    unsigned long opaque_0bc[3];
    int (__cdecl *slot_0c8)(int, int);
    int (__cdecl *slot_0cc)(int);
    int (__cdecl *slot_0d0)(int);
    int (__cdecl *slot_0d4)(int, int);
    unsigned long opaque_0d8[2];
    int (__cdecl *slot_0e0)(int);
    int (__cdecl *slot_0e4)(void);
    int (__cdecl *slot_0e8)(void);
    int (__cdecl *slot_0ec)(int);
    int (__cdecl *slot_0f0)(void);
    unsigned long opaque_0f4[5];
    int (__cdecl *slot_108)(int, int);
    unsigned long opaque_10c[13];
    int (__cdecl *slot_140)(int, int, int, int);
    int (__cdecl *slot_144)(int, int);
    unsigned long opaque_148[7];
    int (__cdecl *slot_164)(int);
    unsigned long opaque_168[1];
    int (__cdecl *slot_16c)(int);
    int (__cdecl *slot_170)(int);
    unsigned long opaque_174[1];
    int (__cdecl *slot_178)(void);
    unsigned long opaque_17c[1];
    int (__cdecl *slot_180)(void);
    unsigned long opaque_184[3];
    int (__cdecl *slot_190)(int, int, int);
    int (__cdecl *slot_194)(int);
    int (__cdecl *slot_198)(int, int, int);
    int (__cdecl *slot_19c)(void);
    int (__cdecl *slot_1a0)(void);
    int (__cdecl *slot_1a4)(void);
    int (__cdecl *slot_1a8)(void);
    int (__cdecl *slot_1ac)(void);
    int (__cdecl *slot_1b0)(int);
    int (__cdecl *slot_1b4)(int);
    int (__cdecl *slot_1b8)(int);
    int (__cdecl *slot_1bc)(int);
    int (__cdecl *slot_1c0)(int);
    int (__cdecl *slot_1c4)(int);
    int (__cdecl *slot_1c8)(int);
    unsigned long opaque_1cc[2];
    int (__cdecl *slot_1d4)(int);
    int (__cdecl *slot_1d8)(int);
    int (__cdecl *slot_1dc)(int);
    int (__cdecl *slot_1e0)(int);
    int (__cdecl *slot_1e4)(int);
    int (__cdecl *slot_1e8)(int, int);
    unsigned long opaque_1ec[2];
    int (__cdecl *slot_1f4)(int, int, int, int);
    int (__cdecl *slot_1f8)(void);
    unsigned long opaque_1fc[2];
    int (__cdecl *slot_204)(int, int);
    unsigned long opaque_208[1];
    int (__cdecl *slot_20c)(int, int, int);
    unsigned long opaque_210[11];
    int (__cdecl *slot_23c)(int);
    unsigned long opaque_240[1];
    int (__cdecl *slot_244)(int, int);
    unsigned long opaque_248[1];
    int (__cdecl *slot_24c)(int, int);
    unsigned long opaque_250[2];
    int (__cdecl *slot_258)(void);
    unsigned long opaque_25c[1];
    int (__cdecl *slot_260)(int);
    unsigned long opaque_264[3];
    int (__cdecl *slot_270)(int, int, int);
    int (__cdecl *slot_274)(int, int, int);
    unsigned long opaque_278[1];
    int (__cdecl *slot_27c)(int);
    int (__cdecl *slot_280)(int);
    unsigned long opaque_284[3];
    int (__cdecl *slot_290)(int, int);
    int (__cdecl *slot_294)(int, int);
    unsigned long opaque_298[3];
    int (__cdecl *slot_2a4)(int, int);
    unsigned long opaque_2a8[1];
    int (__cdecl *slot_2ac)(int, int, int, int, int);
    int (__cdecl *slot_2b0)(int, int, int, int);
    unsigned long opaque_2b4[1];
    int (__cdecl *slot_2b8)(int, int);
    unsigned long opaque_2bc[75];
} EngineInterface;

extern EngineInterface g_engine_interface;

extern Actor *g_support_actor_6fed8;
extern long g_actor_worklist_714c0[];
extern long g_eng3_step_tolerance_431e0;
extern long g_eng3_tangent_table_38e88[];
extern unsigned short pending_action_group_limit;
extern KindAction *g_eng3_middle_actions_40b98;
extern ActorCallback *g_configuration_dword_71384;

extern int remove_actor(Actor *p);
extern void remove_actor_preserving_resource(Actor *p);
extern void mark_actor_resource_for_release(Actor *p);
extern void title_17ad0(long x, long y, long z, short kind, int group);
extern void play_sound(short sound, Actor *p);
extern void stop_actor_sounds(Actor *p);
extern long grid_scan_20b50(long x, long y, long z, int group);
extern int extent_producer_30e40(Actor *p, Bounds3D *b, int mode);
extern Actor *create_eng3_middle_actor(long x, long y, long z, int kind);
extern void unlink_0e4(Actor *p);
extern Actor *insert_scene(Actor *p, Actor *after);
extern int eng1_2a220(Actor *p, Bounds3D *b);
int eng3_1eda0(Actor *p, Bounds3D *b);
long eng3_1ef50(Actor *p, long x, long z);
unsigned long eng3_1e6d0(Actor *p);
int eng3_1f1a0(Actor *p, Bounds3D *b, unsigned short mode);
/* Measured declaration-context hypothesis: the actor query entrypoints had
 * forward declarations alongside the existing query prototypes. Parameter-name
 * presence below and the named query result affect VC5 symbol identities; see
 * the solCMP findings. This does not establish original header/source text. */
int eng3_1eaf0(Actor *p);
int eng3_1ec20(Actor *p);
int eng3_1f0a0(Bounds3D *box, Actor *q);
int eng3_1f130(unsigned long, unsigned long, unsigned long, Bounds3D *);

#define WORD(p) (((short *)(p)->image)[(p)->frame_state])

int eng3_1e000(Actor *p)
{
    short op;
    int result = 0;

    if (--p->frame_ticks < 0) {
        do {
            ++p->frame_state;
            op = WORD(p);
            if (op < 0)
            switch (op) {
            case -2:
                p->direction_x = ~p->direction_x;
                break;
            case -3:
                p->render_flags ^= 0x10;
                break;
            case -31:
                ++p->frame_state;
                title_17ad0(p->x, p->y, p->z, WORD(p), p->frame_group);
                break;
            case -6:
                remove_actor(p);
                op = 0;
                break;
            case -7:
                remove_actor_preserving_resource(p);
                op = 0;
                break;
            case -8:
                mark_actor_resource_for_release(p);
                break;
            case -1:
                p->frame_state = 0;
                result = 1;
                break;
            case -9:
                ++p->frame_state;
                p->frame_ticks = WORD(p);
                break;
            case -10:
                g_configuration_dword_71384[p->action_callback_index](p);
                break;
            case -13:
                if (p->active_step > 0)
                    p->active_step = ~p->active_step + 1;
                p->unknown_0f8 = 0;
                break;
            case -14:
                if (p->active_step < 0)
                    p->active_step = ~p->active_step + 1;
                p->unknown_0f8 = 0;
                break;
            case -15:
                p->active_step = ~p->active_step + 1;
                p->unknown_0f8 = 0;
                break;
            case -16:
                ++p->frame_state;
                p->step_index = WORD(p);
                break;
            case -17:
                ++p->frame_state;
                p->vertical_step = (unsigned char)WORD(p);
                break;
            case -39:
                ++p->frame_state;
                play_sound(WORD(p), 0);
                break;
            case -40:
                ++p->frame_state;
                play_sound(WORD(p), p);
                break;
            case -46:
                stop_actor_sounds(p);
                break;
            case -41:
                ++p->frame_state;
                p->unknown_02a = (unsigned char)WORD(p);
                break;
            case -18:
                ++p->frame_state;
                p->category = (unsigned char)WORD(p);
                break;
            }
        } while (op < 0);
        p->active_command = op;
        if (p->frame_ticks < 0)
            p->frame_ticks = *(short *)p->image;
    }
    return result;
}

unsigned long eng3_1e540(Actor *p)
{
    long x = p->x, y = p->y, z = p->z;
    unsigned short above = 0x7fff, below = 0x7fff;
    Actor *above_actor = 0;
    Actor *q;
    long d;
    long lo;
    int i;
    Bounds3D b;

    g_support_actor_6fed8 = 0;
    for (i = 0; i < g_actor_worklist_714c0[0]; ) {
        q = (Actor *)g_actor_worklist_714c0[++i];
        if (q == p)
            continue;
        if (q->flags & 0x200000) {
            if (!(q->flags & 0x40000) &&
                (q->x > x || q->prev_x <= x || q->z <= z || q->prev_z >= z))
                continue;
            d = eng3_1ef50(q, x, z) - y;
            if (d < 0) {
                d = ~d + 1 >> 16;
                if ((unsigned short)d < above) {
                    above = (unsigned short)d;
                    above_actor = q;
                }
            } else {
                d >>= 16;
                if ((unsigned short)d < below) {
                    below = (unsigned short)d;
                    g_support_actor_6fed8 = q;
                }
            }
        } else {
            if (!eng3_1eda0(q, &b))
                continue;
            if ((lo = q->z + (b.min_z << 16)) > z || (b.max_z << 16) + lo < z)
                continue;
            if ((lo = q->x + (b.min_x << 16)) > x || (b.max_x << 16) + lo < x)
                continue;
            d = (b.min_y << 16) + q->y - y;
            if (d < 0) {
                d = ~d + 1 >> 16;
                if ((unsigned short)d < above) {
                    above = (unsigned short)d;
                    above_actor = q;
                }
            } else {
                d >>= 16;
                if ((unsigned short)d < below) {
                    below = (unsigned short)d;
                    g_support_actor_6fed8 = q;
                }
            }
        }
    }
    if (above < below)
        g_support_actor_6fed8 = above_actor;
    return ((unsigned long)above << 16) | below;
}

unsigned long eng3_1e6d0(Actor *p)
{
    long x, y, z;
    unsigned short above;
    Actor *q;
    long d;
    long lo;
    int i;
    Bounds3D b;

    g_support_actor_6fed8 = 0;
    x = p->x;
    y = p->y;
    z = p->z;
    above = 0x7fff;
    for (i = 0; i < g_actor_worklist_714c0[0]; ) {
        q = (Actor *)g_actor_worklist_714c0[++i];
        if (p == q)
            continue;
        if (q->flags & 0x200000) {
            if (!(q->flags & 0x40000) &&
                (q->x > x || q->prev_x <= x || q->z <= z || q->prev_z >= z))
                continue;
            d = eng3_1ef50(q, x, z) - y;
            if (d < 0) {
                d = ~d + 1 >> 16;
                if ((unsigned short)d < above) {
                    above = (unsigned short)d;
                    g_support_actor_6fed8 = q;
                }
            }
        } else {
            if (!eng3_1eda0(q, &b))
                continue;
            if ((lo = q->z + (b.min_z << 16)) > z || (b.max_z << 16) + lo < z)
                continue;
            if ((lo = q->x + (b.min_x << 16)) > x || (b.max_x << 16) + lo < x)
                continue;
            d = (b.min_y << 16) + q->y - y;
            if (d < 0) {
                d = ~(d >> 16) + 1;
                if ((unsigned short)d < above) {
                    above = (unsigned short)d;
                    g_support_actor_6fed8 = q;
                }
            }
        }
    }
    return ((unsigned long)above << 16) | 0x7fff;
}

int eng3_1ea60(Actor *p)
{
    int limit;
    long r;

    limit = abs((short)((p->x - p->prev_x) >> 16));
    if (limit < 4)
        limit = 4;
    limit += g_eng3_step_tolerance_431e0;
    r = grid_scan_20b50(p->x >> 16, p->y >> 16, p->z >> 16, p->frame_group);
    if (r != 0x7fff7fff && (short)(r >> 16) <= limit) {
        p->y = (p->y & 0xffff0000) - (r & 0xffff0000);
        return 1;
    }
    return 0;
}

int eng3_1eaf0(Actor *p)
{
    short dx, dz, dy;
    long r;

    dy = abs((short)((p->y - p->prev_y) >> 16));
    dx = abs((short)((p->x - p->prev_x) >> 16));
    dz = abs((short)((p->z - p->prev_z) >> 16));
    if (!(p->flags & 0x80000)) {
        if (p->prev_scene)
            unlink_0e4(p);
        r = eng3_1e6d0(p);
        if (r != 0x7fff7fff && (short)(r >> 16) <= dy + dx + 2) {
            p->y -= r & 0xffff0000;
            insert_scene(p, 0);
            return 1;
        }
    }
    r = grid_scan_20b50(p->x >> 16, p->y >> 16, p->z >> 16, p->frame_group);
    if (r != 0x7fff7fff) {
        if (!(short)r)
            return 1;
        if ((short)(r >> 16) <= (dy + dx + dz + g_eng3_step_tolerance_431e0) * 2) {
            p->y -= r & 0xffff0000;
            return 1;
        }
    }
    return 0;
}

int eng3_1ec20(Actor *p)
{
    short dx, dz, dy;
    long r;

    dy = (short)((p->y - p->prev_y) >> 16);
    if (dy < 0)
        dy = ~dy + 1;
    dx = (short)((p->x - p->prev_x) >> 16);
    if (dx < 0)
        dx = ~dx + 1;
    dz = (short)((p->z - p->prev_z) >> 16);
    if (dz < 0)
        dz = ~dz + 1;
    r = grid_scan_20b50(p->x >> 16, p->y >> 16, p->z >> 16, p->frame_group);
    if (r != 0x7fff7fff) {
        if (!(short)r)
            return 1;
        if ((short)(r >> 16) <= dz + dy + dx + g_eng3_step_tolerance_431e0) {
            p->y -= r & 0xffff0000;
            return 1;
        }
    }
    return 0;
}

int eng3_1eda0(Actor *p, Bounds3D *b)
{
    int t;

    if (!(p->shape.min_x | p->shape.max_x)) {
        if (extent_producer_30e40(p, b, 1)) {
            t = b->min_x;
            b->min_x = b->min_z;
            b->min_z = t;
            t = b->max_x;
            b->max_x = b->max_z;
            b->max_z = t;
            return 1;
        }
        return 0;
    }
    b->min_x = p->shape.min_x;
    b->max_x = p->shape.max_x;
    b->min_y = p->shape.min_y;
    b->max_y = p->shape.max_y;
    b->min_z = p->shape.min_z;
    b->max_z = p->shape.max_z;
    return 1;
}

void eng3_1eeb0(Actor *p)
{
    long r = grid_scan_20b50(p->x >> 16, p->y >> 16, p->z >> 16, p->frame_group);
    p->state_025 |= 1;
    if ((unsigned short)r < 0x200) {
        if ((g_engine_interface.context_004->level == 2 || g_engine_interface.context_004->level == 8 ||
             g_engine_interface.context_004->level == 10) && (p->render_flags & 0x10000000)) {
            p->y += (r - 0x20) << 16;
            return;
        }
        p->y += r << 16;
        p->state_025 &= ~1;
    } else if ((r & 0xffff0000) < 0x1000000) {
        p->y -= r & 0xffff0000;
        p->state_025 &= ~1;
    }
}

long eng3_1ef50(Actor *p, long x, long z)
{
    Bounds3D b;
    long y = 0x7fff;
    long lo;
    unsigned long h;

    if (eng1_2a220(p, &b) &&
        (lo = p->z + (b.min_z << 16)) <= z && (b.max_z << 16) + lo >= z &&
        (lo = p->x + (b.min_x << 16)) <= x && (b.max_x << 16) + lo >= x) {
        h = p->heights[((x - lo) >> 16) / p->action_callback_index] << 16;
        if (h != 0x7f0000)
            y = (b.min_y << 16) + p->y + h;
    }
    return y;
}

int eng3_1f0a0(Bounds3D *box, Actor *q)
{
    Bounds3D a;
    int valid;

    valid = eng3_1f1a0(q, &a, 0);
    if (valid)
        return !((a.max_y < box->min_y) | (a.max_z < box->min_z) |
                 (box->max_x < a.min_x) | (box->max_z < a.min_z) |
                 (box->max_y < a.min_y) | (a.max_x < box->min_x));
    return 0;
}

int eng3_1f130(unsigned long x, unsigned long y, unsigned long z, Bounds3D *b)
{
    short hx = (short)(x >> 16);
    short hy = (short)(y >> 16);
    short hz = (short)(z >> 16);

    return (hz >= b->min_z) & (hx <= b->max_x) & (hy >= b->min_y) &
           (hy <= b->max_y) & (hz <= b->max_z) & (hx >= b->min_x);
}

int eng3_1f1a0(Actor *p, Bounds3D *b, unsigned short mode)
{
    Bounds3D e;
    unsigned short x, y, z;

    if (extent_producer_30e40(p, &e, mode) && (e.max_x | e.max_y)) {
        x = (unsigned short)((unsigned long)p->x >> 16);
        y = (unsigned short)((unsigned long)p->y >> 16);
        z = (unsigned short)((unsigned long)p->z >> 16);
        if (p->render_flags & 0x10) {
            b->max_z = x - e.min_z;
            b->min_z = x - e.min_z - e.max_z;
        } else {
            b->min_z = x + e.min_z;
            b->max_z = x + e.min_z + e.max_z;
        }
        if (p->render_flags & 0x20) {
            b->max_y = y - e.min_y;
            b->min_y = y - e.min_y - e.max_y;
        } else {
            b->min_y = y + e.min_y;
            b->max_y = y + e.min_y + e.max_y;
        }
        b->min_x = z + e.min_x;
        b->max_x = z + e.min_x + e.max_x;
        return 1;
    }
    return 0;
}


Actor *eng3_1f410(PendingActionGroup *group, long x, long y, long z, int kind, int profile)
{
    Actor *actor;

    if (group->count < pending_action_group_limit) {
        actor = create_eng3_middle_actor(x, y, z, kind);
        if (actor) {
            actor->frame_group = profile;
            g_eng3_middle_actions_40b98[kind & 0x1ff].init(actor);
            group->entries[++group->count] = actor;
            return actor;
        }
    }
    return 0;
}

long eng3_1f7b0(unsigned short a, unsigned short b)
{
    long ratio;
    int lo = 0, hi = 359, mid;

    if (!a)
        return 0x5a0000;
    ratio = ((long)b << 16) / a;
    mid = 179;
    while (g_eng3_tangent_table_38e88[mid] != ratio) {
        if (g_eng3_tangent_table_38e88[mid] < ratio)
            lo = mid;
        else
            hi = mid;
        if (lo >= hi - 1)
            return lo << 14;
        mid = (hi - lo) / 2 + lo;
    }
    return mid << 14;
}

void eng3_1f8b0(void)
{
    g_engine_interface.slot_018();
}

int eng3_1f8c0(int a, int b)
{
    return g_engine_interface.slot_01c(a, b);
}

void eng3_1f950(void)
{
    g_engine_interface.slot_03c();
}

int eng3_1f9a0(int a, int b)
{
    return g_engine_interface.slot_04c(a, b);
}

int eng3_1fa00(int a, int b)
{
    return g_engine_interface.slot_064(a, b);
}

void eng3_1fa80(void)
{
    g_engine_interface.slot_088();
}

void eng3_1faa0(void)
{
    g_engine_interface.slot_090();
}

int eng3_1fab0(int a)
{
    return g_engine_interface.slot_094(a);
}

void eng3_1fac0(void)
{
    g_engine_interface.slot_098();
}

int eng3_1fad0(int a, int b, int c, int d)
{
    return g_engine_interface.slot_0a0(a, b, c, d);
}

int eng3_1faf0(int a)
{
    return g_engine_interface.slot_0b0(a);
}

void eng3_1fb00(void)
{
    g_engine_interface.slot_0b4();
}

int eng3_1fb10(int a)
{
    return g_engine_interface.slot_0b8(a);
}

int eng3_1fb20(int a, int b)
{
    return g_engine_interface.slot_0c8(a, b);
}

int eng3_1fb40(int a)
{
    return g_engine_interface.slot_0cc(a);
}

int eng3_1fb50(int a)
{
    return g_engine_interface.slot_0d0(a);
}

int eng3_1fb60(int a, int b)
{
    return g_engine_interface.slot_0d4(a, b);
}

int eng3_1fb80(int a)
{
    return g_engine_interface.slot_0e0(a);
}

void eng3_1fb90(void)
{
    g_engine_interface.slot_0e4();
}

void eng3_1fba0(void)
{
    g_engine_interface.slot_0e8();
}

int eng3_1fbb0(int a)
{
    return g_engine_interface.slot_0ec(a);
}

void eng3_1fbc0(void)
{
    g_engine_interface.slot_0f0();
}

int eng3_1fbd0(int a, int b)
{
    return g_engine_interface.slot_108(a, b);
}

void eng3_1fc60(void)
{
    g_engine_interface.slot_178();
}

void eng3_1fc70(void)
{
    g_engine_interface.slot_180();
}

int eng3_1fc80(int a, int b, int c)
{
    return g_engine_interface.slot_190(a, b, c);
}

int eng3_1fca0(int a)
{
    return g_engine_interface.slot_194(a);
}

int eng3_1fcb0(int a, int b, int c)
{
    return g_engine_interface.slot_198(a, b, c);
}

void eng3_1fcd0(void)
{
    g_engine_interface.slot_19c();
}

void eng3_1fce0(void)
{
    g_engine_interface.slot_1a0();
}

void eng3_1fcf0(void)
{
    g_engine_interface.slot_1a4();
}

void eng3_1fd00(void)
{
    g_engine_interface.slot_1a8();
}

void eng3_1fd10(void)
{
    g_engine_interface.slot_1ac();
}

int eng3_1fd20(int a)
{
    return g_engine_interface.slot_1b0(a);
}

int eng3_1fd30(int a)
{
    return g_engine_interface.slot_1b4(a);
}

int eng3_1fd40(int a)
{
    return g_engine_interface.slot_1b8(a);
}

int eng3_1fd50(int a)
{
    return g_engine_interface.slot_1bc(a);
}

int eng3_1fd60(int a)
{
    return g_engine_interface.slot_1c0(a);
}

int eng3_1fd70(int a)
{
    return g_engine_interface.slot_1c4(a);
}

int eng3_1fd80(int a)
{
    return g_engine_interface.slot_1c8(a);
}

int eng3_1fd90(int a)
{
    return g_engine_interface.slot_1d4(a);
}

int eng3_1fda0(int a)
{
    return g_engine_interface.slot_1d8(a);
}

int eng3_1fdb0(int a)
{
    return g_engine_interface.slot_1dc(a);
}

int eng3_1fdc0(int a)
{
    return g_engine_interface.slot_1e0(a);
}

int eng3_1fdd0(int a)
{
    return g_engine_interface.slot_1e4(a);
}

int eng3_1fde0(int a, int b)
{
    return g_engine_interface.slot_1e8(a, b);
}

int eng3_1fe00(int a, int b, int c, int d)
{
    return g_engine_interface.slot_1f4(a, b, c, d);
}

void eng3_1fe20(void)
{
    g_engine_interface.slot_1f8();
}

int eng3_1fe30(int a, int b)
{
    return g_engine_interface.slot_204(a, b);
}

int eng3_1fe50(int a, int b, int c)
{
    return g_engine_interface.slot_20c(a, b, c);
}

int eng3_1fe70(int a)
{
    return g_engine_interface.slot_23c(a);
}

int eng3_1fe80(int a, int b)
{
    return g_engine_interface.slot_244(a, b);
}

int eng3_1fea0(int a, int b)
{
    return g_engine_interface.slot_24c(a, b);
}

void eng3_1fec0(void)
{
    g_engine_interface.slot_258();
}

int eng3_1fed0(int a)
{
    return g_engine_interface.slot_260(a);
}

int eng3_1fee0(int a, int b, int c)
{
    return g_engine_interface.slot_270(a, b, c);
}

int eng3_1ff00(int a, int b, int c)
{
    return g_engine_interface.slot_274(a, b, c);
}

int eng3_1ff20(int a)
{
    return g_engine_interface.slot_27c(a);
}

int eng3_1ff30(int a)
{
    return g_engine_interface.slot_280(a);
}

int eng3_1ff40(int a, int b)
{
    return g_engine_interface.slot_290(a, b);
}

int eng3_1ff60(int a, int b)
{
    return g_engine_interface.slot_294(a, b);
}

int eng3_1ff80(int a, int b)
{
    return g_engine_interface.slot_2a4(a, b);
}

int eng3_1ffd0(int a, int b, int c, int d)
{
    return g_engine_interface.slot_2b0(a, b, c, d);
}

int eng3_1fff0(int a, int b)
{
    return g_engine_interface.slot_2b8(a, b);
}
