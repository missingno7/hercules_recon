/* ENG1.DLL 0x1e50..0x2f30: level start, dialog text, level-state gate and
 * the scrolling particle/texture effect helpers. Region lane x1.
 * Local views only cover offsets observed in these bodies. */

#include <string.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

/* Interface context (g_engine_interface.context_004). */
typedef struct LevelContext {
    signed char level;          /* 0x00 */
    signed char sublevel;       /* 0x01 */
    u8 unknown_02;
    u8 next_state;              /* 0x03 */
    u8 state;                   /* 0x04 */
    u8 paused;                  /* 0x05 */
    u8 mode;                    /* 0x06 */
    u8 unknown_07[0x38 - 0x07];
    long tick;                  /* 0x38 */
    u8 unknown_3c[0x5c - 0x3c];
    volatile u8 held;           /* 0x5c: volatile per owner ruling 2026-10-20 (re-read at every test; codegen hypothesis) */
    u8 unknown_5d;
    volatile u16 pressed;       /* 0x5e: volatile per owner ruling 2026-10-20 (re-read at every test; codegen hypothesis) */
    u16 unknown_60;             /* 0x60 */
    u8 unknown_62[0x9c - 0x62];
    u16 mode_09c;               /* 0x9c */
    u8 unknown_9e[0xdc - 0x9e];
    u16 transfer_status;        /* 0xdc */
} LevelContext;

typedef struct EngineInterfaceHead {
    u32 signature;
    LevelContext *context;
} EngineInterfaceHead;

extern EngineInterfaceHead g_engine_interface;

/* Pipeline actor. */
typedef struct PipelineActor {
    long x;
    long y;
    long z;
    u8 unknown_0c[0x22 - 0x0c];
    u8 category;                /* 0x22 */
    u8 unknown_23[0x2e - 0x23];
    u16 kind;                   /* 0x2e */
} PipelineActor;

typedef struct ActorKindCallbacks {
    void (*init)(PipelineActor *);
    void (*update)(PipelineActor *);
    void (*start)(PipelineActor *);
} ActorKindCallbacks;

typedef struct LevelStart {
    u8 unknown_00[0x10];
    u16 x;
    u16 y;
    u16 z;
    u16 kind;
} LevelStart;

typedef struct DescriptorRow {
    void *pointer_000;
    u32 field_004;
    u32 field_008;
    LevelStart *starts;
    void *pointer_010;
    void *pointer_014;
} DescriptorRow;

extern void *g_normal_actor_pool;
extern PipelineActor *g_pipeline_actor_7200c;
extern long g_camera_x_70ca0;
extern long g_camera_y_70ca4;
extern ActorKindCallbacks *g_configuration_dword_71370;
extern DescriptorRow eng1_descriptor_rows_5a690[];
extern void initialize_normal_actor(PipelineActor *actor);

/* Dialog text. */
extern int g_context_sequence_phase;
extern char *g_eng1_dialog_scripts_4d720[];
extern u8 g_eng1_dialog_operations_4d760[][3];
extern void dispatch_local_24d0(int x, int y, int u, int v, int w, int h, int mode);

/* Level state. */
extern long g_eng1_setup_dword_6efac;
extern long g_resource_transition_ticks_6efb0;
extern long g_eng1_setup_dword_72114;
extern void dispatch_engine_2c420(void);
extern void dispatch_engine_2c450(void);
extern void dispatch_engine_2c4d0(void);
extern void dispatch_engine_2c590(void);
extern void play_sound(int id, int arg);

/* PSX-style primitives used by the effect. */
typedef struct Rect16 {
    short x;
    short y;
    short w;
    short h;
} Rect16;

typedef struct SVector {
    short vx;
    short vy;
    short vz;
    short pad;
} SVector;

typedef struct PolyFT3 {
    void *tag;
    u8 r0, g0, b0, code;
    short x0, y0;
    u8 u0, v0;
    u16 clut;
    short x1, y1;
    u8 u1, v1;
    u16 tpage;
    short x2, y2;
    u8 u2, v2;
    u16 pad;
} PolyFT3;

/* Frame record list. */
typedef struct FrameRecord {
    struct FrameRecord *next;
    u8 unknown_04[0x14];
} FrameRecord;

typedef struct FrameBuffer {
    u8 unknown_000[0x88];
    FrameRecord *list;          /* 0x88 */
    u8 unknown_08c[0x1384 - 0x8c];
    void *ot[1];                /* 0x1384 */
} FrameBuffer;

typedef struct Effect Effect;
typedef struct EffectDescriptor {
    u16 prims;                  /* 0x00 */
    u16 count;                  /* 0x02 */
    u8 scroll;                  /* 0x04 */
    u8 divisor;                 /* 0x05 */
    u8 density;                 /* 0x06 */
    u8 blend;                   /* 0x07 */
    short x, y, z;              /* 0x08 */
    u8 r, g, b;                 /* 0x0e */
    u8 unknown_11[3];
    void (*place)(Effect *, int);   /* 0x14 */
    void (*update)(Effect *, int);  /* 0x18 */
    long *pattern;              /* 0x1c */
    void *palette;              /* 0x20 */
} EffectDescriptor;

struct Effect {
    long x;
    long y;
    long z;
    short sx;                   /* 0x0c */
    short sy;                   /* 0x0e */
    u8 unknown_10[0x1e - 0x10];
    u8 clut_row;                /* 0x1e */
    u8 flags;                   /* 0x1f */
    u8 unknown_20;
    u8 scroll;                  /* 0x21 */
    u8 unknown_22[0x30 - 0x22];
    short ticks;                /* 0x30 */
    short r;                    /* 0x32 */
    u16 rows;                   /* 0x34 */
    u16 step;                   /* 0x36 */
    u8 unknown_38[2];
    u16 range;                  /* 0x3a */
    u8 unknown_3c[4];
    u16 count;                  /* 0x40 */
    u16 blend;                  /* 0x42 */
    u16 prims;                  /* 0x44 */
    u16 g;                      /* 0x46 */
    u16 b;                      /* 0x48 */
    u8 unknown_4a[2];
    long offset;                /* 0x4c */
    u8 unknown_50[8];
    PolyFT3 *polys;             /* 0x58 */
    SVector *verts;             /* 0x5c */
    u8 unknown_60[4];
    long *image;                /* 0x64 */
    u8 unknown_68[4];
    long *pattern;              /* 0x6c */
    u8 unknown_70[0xfc - 0x70];
    void *palette;              /* 0xfc */
    u32 clut;                   /* 0x100 */
    u8 unknown_104[0x110 - 0x104];
    void (*place)(Effect *, int);   /* 0x110 */
    u8 unknown_114[4];
    void (*update)(Effect *, int);  /* 0x118 */
    u8 unknown_11c[4];
    EffectDescriptor *descriptor;   /* 0x120 */
};

extern FrameBuffer *g_dispatch_frame_current_71ec4;
extern FrameBuffer *g_dispatch_frame_base_71ec8;
extern u8 g_dispatch_key_7286c;
extern long g_effect_level_6ee64;
extern long g_effect_sequence_6ee68;
extern short g_frame_word_6ee84;
extern short *g_sine_table_70f34;
extern Rect16 g_effect_rect_4d7f0;
extern void *engine_pool_allocate(void *slot, unsigned long size, unsigned long flags);
extern void dispatch_engine_2ca00(PolyFT3 *p);
extern u8 dispatch_local_056c0(void *palette, int key, int c, int d);
extern u16 dispatch_engine_2c7c0(int x, int y);
extern void dispatch_engine_2c6c0(void);
extern void dispatch_engine_2c7f0(Rect16 *rect, void *pixels);
extern int eng1_320d0(Effect *e);
extern void eng1_32810(Effect *e, int n);
extern void dispatch_engine_2c690(SVector *v);
extern void dispatch_engine_2c6e0(void);
extern void dispatch_engine_2c760(short *xy);
extern void dispatch_engine_2c7a0(long *z);
extern int dispatch_engine_2c460(int range);

/* VRAM moves. */
extern FrameRecord *g_frame_global_73484;
extern void dispatch_local_2c9b0(FrameRecord *record, Rect16 *rect, int x, int y);
extern signed char g_root_count_70ed1;
extern signed char g_root_count_70ed2;
extern long *g_header_pointer_70d38;
extern long *g_header_pointer_70d3c;
typedef struct ScrollPos {
    u8 u;
    u8 v;
} ScrollPos;

extern ScrollPos *g_resource_leaf_buffer_70c58;

void eng1_02400(u8 state);
void eng1_02430(void);
int eng1_02290(void);
void dispatch_local_2450(int x, int y, int script, int mode);
void eng1_02a10(Effect *e);
void eng1_02b30(Effect *e, int rows);
void dispatch_local_2c60(Rect16 *rect, int x, int y);

/* 0x1e50 */
void eng1_call_01e50(void)
{
    LevelStart *start;

    g_pipeline_actor_7200c = (PipelineActor *)((u8 *)g_normal_actor_pool + 0x134);
    initialize_normal_actor(g_pipeline_actor_7200c);
    start = &eng1_descriptor_rows_5a690[g_engine_interface.context->level]
                 .starts[g_engine_interface.context->sublevel];
    if (start->kind != 0) {
        g_pipeline_actor_7200c->x = start->x << 16;
        g_pipeline_actor_7200c->y = start->y << 16;
        g_pipeline_actor_7200c->z = start->z << 16;
        g_camera_x_70ca0 = start->x;
        g_camera_y_70ca4 = start->y;
        g_pipeline_actor_7200c->kind = start->kind;
        g_pipeline_actor_7200c->category = 1;
        g_configuration_dword_71370[g_pipeline_actor_7200c->kind & 0x3ff].init(g_pipeline_actor_7200c);
        g_configuration_dword_71370[g_pipeline_actor_7200c->kind & 0x3ff].start(g_pipeline_actor_7200c);
    }
}

/* 0x1f40 */
void dispatch_local_1f40(void)
{
    int a;
    int b;

    if (g_context_sequence_phase == 2 && (g_engine_interface.context->tick & 0x10))
        dispatch_local_2450(0x55, 0x70, 14, 3);
    if (g_engine_interface.context->state != 0) {
        a = 4;
        b = 5;
        switch (g_engine_interface.context->state) {
        case 2:
        case 3:
        case 0x40:
            if (g_engine_interface.context->state == 2) {
                a = 5;
                b = 4;
            }
            dispatch_local_2450(0x72, 0x50, 0, 3);
            dispatch_local_2450(0x62, 0x78, 1, a);
            dispatch_local_2450(0x83, 0x8c, 2, b);
            break;
        case 4:
        case 5:
            if (g_engine_interface.context->state == 4) {
                a = 5;
                b = 4;
            }
            dispatch_local_2450(0x3f, 0x50, 3, 3);
            dispatch_local_2450(0x8d, 0x78, 4, a);
            dispatch_local_2450(0x8b, 0x8c, 5, b);
            break;
        case 8:
            dispatch_local_2450(0x32, 0x64, 6, 3);
            dispatch_local_2450(0x2d, 0x78, 7, 3);
            dispatch_local_2450(0x20, 0x8c, 8, 3);
            break;
        case 0x80:
            if (g_engine_interface.context->tick & 8)
                dispatch_local_2450(0x67, 0x78, 13, 3);
            break;
        case 0x10:
            dispatch_local_2450(0x2f, 0x64, 9, 3);
            dispatch_local_2450(0x77, 0x78, 10, 3);
            if (g_engine_interface.context->transfer_status == 0x10)
                dispatch_local_2450(0x2d, 0x94, 12, 3);
            else
                dispatch_local_2450(0x3b, 0x94, 11, 3);
            break;
        }
    }
}

/* 0x2180 */
void dispatch_local_02180(void)
{
    if (g_engine_interface.context->state == 0x80) {
        if (g_engine_interface.context->mode_09c != 0)
            return;
        eng1_02430();
    }
    if (g_engine_interface.context->transfer_status != 0) {
        eng1_02400(0x10);
    } else if (g_engine_interface.context->unknown_60 != 0) {
        if (g_engine_interface.context->paused == 0)
            eng1_02400(8);
    } else if (g_engine_interface.context->state != 8) {
        if (g_resource_transition_ticks_6efb0 == 0) {
            if ((g_engine_interface.context->held & 9) == 1)
                g_resource_transition_ticks_6efb0 = 1;
        } else if ((g_engine_interface.context->held & 9) == 9) {
            if (++g_resource_transition_ticks_6efb0 > 60) {
                if (g_engine_interface.context->mode == 3)
                    g_engine_interface.context->next_state = 0x13;
                else
                    g_engine_interface.context->next_state = 0x12;
            }
            return;
        } else {
            g_resource_transition_ticks_6efb0 = (g_engine_interface.context->held & 9) == 1;
        }
    }
    if (g_engine_interface.context->state != 0) {
        if (!eng1_02290())
            return;
        if (g_engine_interface.context->pressed & 0x4008) {
            eng1_02430();
            dispatch_engine_2c420();
            return;
        }
    }
    if ((g_engine_interface.context->pressed & 8) && g_engine_interface.context->paused == 0 &&
        g_engine_interface.context->mode != 3)
        eng1_02400(0x40);
}

/* 0x2290 */
int eng1_02290(void)
{
    if (g_engine_interface.context->state == 8) {
        if (g_engine_interface.context->unknown_60 == 0) {
            if (g_engine_interface.context->mode == 3)
                eng1_02430();
            else
                g_engine_interface.context->state = 2;
        }
        g_eng1_setup_dword_6efac = 0;
        g_resource_transition_ticks_6efb0 = 0;
        return 0;
    }
    if (g_engine_interface.context->state == 0x10) {
        if (g_engine_interface.context->transfer_status == 0x10)
            g_engine_interface.context->transfer_status = 0;
        if (g_engine_interface.context->transfer_status == 0)
            eng1_02430();
        return 0;
    }
    if (g_engine_interface.context->state == 0x40) {
        play_sound(0x339, 0);
        g_engine_interface.context->state = 2;
        return 0;
    }
    if (g_engine_interface.context->pressed & 0x10) {
        if ((g_engine_interface.context->state & 1) == 1)
            play_sound(0x337, 0);
        g_engine_interface.context->state &= ~1;
        return 0;
    }
    if (g_engine_interface.context->pressed & 0x40) {
        if ((g_engine_interface.context->state & 1) != 1)
            play_sound(0x337, 0);
        g_engine_interface.context->state |= 1;
        return 0;
    }
    if (g_engine_interface.context->pressed & 0x4008) {
        play_sound(0x338, 0);
        switch (g_engine_interface.context->state) {
        case 2:
            break;
        case 3:
            g_engine_interface.context->state = 4;
            g_eng1_setup_dword_6efac = 0;
            return 0;
        case 4:
            g_engine_interface.context->state = 2;
            g_eng1_setup_dword_6efac = 0;
            return 0;
        case 5:
            g_engine_interface.context->next_state = 0x13;
            g_eng1_setup_dword_6efac = 0;
            return 0;
        }
    }
    return 1;
}

/* 0x2400 */
void eng1_02400(u8 state)
{
    dispatch_engine_2c450();
    dispatch_engine_2c4d0();
    g_engine_interface.context->state = state;
    g_eng1_setup_dword_6efac = 0;
}

/* 0x2430 */
void eng1_02430(void)
{
    dispatch_engine_2c590();
    g_eng1_setup_dword_6efac = 0;
    g_eng1_setup_dword_72114 = 0;
    g_engine_interface.context->state = 0;
}

/* 0x2450 */
void dispatch_local_2450(int x, int y, int script, int mode)
{
    char *text;
    int c;
    u8 *op;

    text = g_eng1_dialog_scripts_4d720[script];
    while ((c = *text++) != 0) {
        c = (c - 1) & 0x1f;
        if (c != 0x1f) {
            op = g_eng1_dialog_operations_4d760[c];
            dispatch_local_24d0(x, y, op[0], op[1], op[2], 0x12, mode);
            x += g_eng1_dialog_operations_4d760[c][2];
        } else {
            x += 0x10;
        }
    }
}

/* 0x2500 */
int eng1_02500(Effect *e, EffectDescriptor *d)
{
    int n;
    int q;
    int size;
    int verts_size;
    int polys_size;
    int i;
    u16 tpage;
    PolyFT3 *p0;
    PolyFT3 *p1;
    int v;

    e->x = d->x << 16;
    e->y = d->y << 16;
    e->z = d->z << 16;
    e->r = d->r;
    e->g = d->g;
    e->b = d->b;
    e->prims = d->prims;
    e->count = d->count;
    e->scroll = d->scroll;
    n = e->prims;
    e->blend = d->blend;
    e->range = 0x240;
    e->offset = 0;
    e->rows = 0;
    e->ticks = 0;
    e->place = d->place;
    e->update = d->update;
    e->pattern = d->pattern;
    e->palette = d->palette;
    e->descriptor = d;
    q = 0x100 / d->divisor;
    e->step = q;
    verts_size = n * sizeof(SVector);
    polys_size = n * 2 * sizeof(PolyFT3);
    size = verts_size + polys_size + 0x400;
    if (!engine_pool_allocate(&e->polys, size, 0))
        return 0;
    p0 = e->polys;
    e->verts = (SVector *)((u8 *)p0 + polys_size);
    e->image = (long *)((u8 *)e->verts + verts_size);
    memset(p0, 0, size);
    eng1_02b30(e, d->density);
    tpage = ((e->blend & 3) << 5) | 0x195;
    p1 = p0 + n;
    e->clut_row = dispatch_local_056c0(e->palette, 0x7650, 0x100, 0);
    e->clut = dispatch_engine_2c7c0(0x300, e->clut_row + 0xe0);
    for (i = 0; i < n; i++) {
        dispatch_engine_2ca00(p0);
        p0->code |= 2;
        dispatch_engine_2ca00(p1);
        p1->code |= 2;
        p0->r0 = (u8)e->r;
        p0->g0 = (u8)e->g;
        p0->b0 = (u8)e->b;
        p1->r0 = (u8)e->r;
        p1->g0 = (u8)e->g;
        p1->b0 = (u8)e->b;
        p0->tpage = tpage;
        p1->tpage = tpage;
        p0->clut = e->clut;
        p1->clut = e->clut;
        v = (i * 0x44 & ~3) % (0x100 - q);
        p0->u0 = 0xff;
        p0->v0 = v + q;
        p0->u1 = 0xff;
        p0->v1 = v + q;
        p0->u2 = 0xff;
        p0->v2 = v;
        p1->u0 = 0xff;
        p1->v0 = v + q;
        p1->u1 = 0xff;
        p1->v1 = v + q;
        p1->u2 = 0xff;
        p1->v2 = v;
        p0++;
        p1++;
    }
    for (i = 0; i < n; i++)
        e->place(e, i);
    return 1;
}

/* 0x27a0 */
void eng1_027a0(Effect *e)
{
    int depth;
    int i;
    int h;

    if (g_dispatch_key_7286c != 0 || g_effect_level_6ee64 != 0x80)
        eng1_02a10(e);
    if (e->count == 0)
        return;
    if (e->count > 0x14)
        e->count = 0x14;
    depth = eng1_320d0(e);
    if (depth == -1)
        e->flags |= 8;
    if ((unsigned)(e->sx + 0x80) > 0x240)
        return;
    if ((unsigned)(e->sy + 0x80) > 0x1f0)
        return;
    dispatch_engine_2c6c0();
    eng1_32810(e, depth + g_effect_sequence_6ee68);
    e->update(e, (depth / 2) & 0xfff);
    if (g_engine_interface.context->state != 0)
        return;
    g_effect_sequence_6ee68++;
    h = 0x100 - e->offset;
    g_effect_rect_4d7f0.y = e->offset + 0x100;
    g_effect_rect_4d7f0.h = h;
    g_effect_rect_4d7f0.x = 0x7f;
    g_effect_rect_4d7f0.y += 0x100;
    dispatch_engine_2c7f0(&g_effect_rect_4d7f0, e->image);
    g_effect_rect_4d7f0.y = 0x100;
    g_effect_rect_4d7f0.h = e->offset;
    g_effect_rect_4d7f0.x = 0x7f;
    g_effect_rect_4d7f0.y += 0x100;
    dispatch_engine_2c7f0(&g_effect_rect_4d7f0, e->image + h / 2);
    e->offset = (e->scroll + e->offset) & 0xff;
    for (i = 0; i < 4; i++)
        e->place(e, (i + g_engine_interface.context->tick * 8) % e->prims);
}

/* 0x2920 */
void eng1_02920(Effect *e, int width)
{
    void **ot;
    SVector *v;
    PolyFT3 *p;
    int n;
    long z;
    int d;

    v = e->verts;
    p = e->polys;
    if (g_dispatch_frame_current_71ec4 == g_dispatch_frame_base_71ec8)
        p += e->prims;
    n = e->count + 1;
    ot = g_dispatch_frame_current_71ec4->ot;
    dispatch_engine_2c690(v);
    dispatch_engine_2c6e0();
    v++;
    while (--n) {
        dispatch_engine_2c760(&p->x0);
        dispatch_engine_2c7a0(&z);
        p->x1 = p->x0 - (short)z / 0x800 + 2;
        p->x2 = p->x0 + width;
        p->y1 = p->y0;
        d = (short)z & ~3;
        if (d < 0)
            d = 0;
        else if (d > 0x12f8)
            d = 0x12f8;
        p->tag = *(void **)((u8 *)ot - d);
        *(void **)((u8 *)ot - d) = p;
        p++;
        dispatch_engine_2c690(v);
        dispatch_engine_2c6e0();
        v++;
    }
}

/* 0x2a10 */
void eng1_02a10(Effect *e)
{
    int level;
    int r;
    int g;
    int b;
    int n;
    PolyFT3 *p;

    level = g_frame_word_6ee84;
    r = e->r * level >> 7;
    g = e->g * level >> 7;
    b = e->b * level >> 7;
    g_effect_level_6ee64 = level;
    n = e->count + 1;
    p = e->polys;
    if (g_dispatch_frame_current_71ec4 == g_dispatch_frame_base_71ec8)
        p += e->prims;
    do {
        p->r0 = r;
        p->g0 = g;
        p->b0 = b;
        p++;
    } while (--n);
}

/* 0x2a80 */
void eng1_02a80(Effect *e, int y)
{
    int n;
    PolyFT3 *p;

    n = e->count + 1;
    p = e->polys;
    if (g_dispatch_frame_current_71ec4 == g_dispatch_frame_base_71ec8)
        p += e->prims;
    do {
        p->y2 = y;
        p++;
    } while (--n);
}

/* 0x2ac0 */
void eng1_02ac0(Effect *e, int i)
{
    SVector *v;
    int angle;
    int radius;
    int x;
    int z;

    v = e->verts;
    angle = dispatch_engine_2c460(0x1000);
    radius = dispatch_engine_2c460(0x60) + 0xc0;
    x = g_sine_table_70f34[angle & 0xfff] * radius >> 12;
    z = g_sine_table_70f34[(angle - 0x400) & 0xfff] * radius >> 12;
    v[i].vx = x;
    v[i].vz = z;
    v[i].vy = 0;
}

/* 0x2b30 */
void eng1_02b30(Effect *e, int rows)
{
    long *src;
    int width;
    int clear_step;
    int step;
    int off;
    int row;
    int col;

    src = e->pattern;
    width = *src++;
    if (e->rows != 0) {
        off = 0;
        clear_step = 0x80 / e->rows * 4;
        for (row = 0; row < e->rows; row++) {
            for (col = 0; col < width; col++)
                *(long *)((u8 *)e->image + off + col * 4) = 0;
            off += clear_step;
        }
    }
    if (rows < 0)
        rows = 0;
    else if (rows > 0x10)
        rows = 0x10;
    if (rows != 0) {
        off = 0;
        step = 0x80 / rows * 4;
        for (row = 0; row < rows; row++) {
            for (col = 0; col < width; col++)
                *(long *)((u8 *)e->image + off + col * 4) = *src++ ? 0xff00ffff : 0;
            off += step;
        }
    }
    e->rows = rows;
}

/* 0x2c60 */
void dispatch_local_2c60(Rect16 *rect, int x, int y)
{
    FrameRecord *record;

    record = g_frame_global_73484;
    dispatch_local_2c9b0(record, rect, x, y);
    record->next = g_dispatch_frame_current_71ec4->list;
    g_dispatch_frame_current_71ec4->list = record;
    g_frame_global_73484 = record + 1;
}

/* 0x2cb0 */
void dispatch_local_02cb0(void)
{
    long *entry;
    int i;
    u16 *p;
    int frames;
    int period;
    Rect16 r;

    entry = g_header_pointer_70d38;
    for (i = 0; i < g_root_count_70ed1; i++) {
        p = (u16 *)((u8 *)entry + *entry);
        entry++;
        frames = *p++;
        period = *p++;
        if (period != 0 && g_engine_interface.context->tick % period == 0) {
            r.x = *p++;
            r.y = *p++;
            r.w = *p++;
            r.h = *p++;
            dispatch_engine_2c7f0(&r, p + g_engine_interface.context->tick / period % frames * r.w * r.h);
        }
    }
}

/* 0x2d80 */
void eng1_02d80(int index, unsigned int du, unsigned int dv)
{
    long *entry;
    u16 *p;
    int x;
    int y;
    unsigned int w;
    unsigned int h;
    int sx;
    int sy;
    unsigned int u;
    unsigned int v;
    Rect16 r;

    if (index >= g_root_count_70ed2)
        return;
    entry = &g_header_pointer_70d3c[index];
    p = (u16 *)((u8 *)entry + *entry + 6);
    x = *p++;
    y = *p++;
    w = *p++;
    h = *p++;
    sx = *p++;
    sy = *p;
    u = (g_resource_leaf_buffer_70c58[index].u + du) % w;
    v = (g_resource_leaf_buffer_70c58[index].v + dv) % h;
    g_resource_leaf_buffer_70c58[index].u = u;
    g_resource_leaf_buffer_70c58[index].v = v;
    r.x = sx + u;
    r.y = sy + v;
    r.w = w - u;
    r.h = h - v;
    dispatch_local_2c60(&r, x, y);
    r.x = sx + u;
    r.y = sy;
    r.w = w - u;
    r.h = v;
    dispatch_local_2c60(&r, x, y + h - v);
    r.x = sx;
    r.y = sy + v;
    r.w = u;
    r.h = h - v;
    dispatch_local_2c60(&r, x + w - u, y);
    r.x = sx;
    r.y = sy;
    r.w = u;
    r.h = v;
    dispatch_local_2c60(&r, x + w - u, y + h - v);
}
