#include <windows.h>

/* ENG3.DLL 0x25040..0x26b40: HUD/primitive helpers and actor handlers (lane y3 candidate).
 * Actor offsets are provisional shared names, not recovered original types. */
typedef struct Actor Actor;
struct Actor {
    long x, y, z;
    unsigned char unknown_0c[0x1c - 0x0c];
    unsigned char unknown_1c;
    unsigned char unknown_1d[0x22 - 0x1d];
    unsigned char category;
    unsigned char unknown_23[0x2a - 0x23];
    short state;
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
    short speed;
    short unknown_72;
    short unknown_74;
    unsigned char unknown_76[0x88 - 0x76];
    unsigned char unknown_88[0x94 - 0x88];
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
    struct ActorGroup *unknown_120;
    unsigned char unknown_124[0x126 - 0x124];
    short unknown_126;
};

typedef struct ActorGroup ActorGroup;
struct ActorGroup {
    long count;
    Actor *actors[1];
};

typedef struct Eng3Game Eng3Game;
struct Eng3Game {
    unsigned char unknown_000[0x1f8];
    void (*method_1f8)(Actor *);
    unsigned char unknown_1fc[0x204 - 0x1fc];
    void (*method_204)(Actor *);
    unsigned char unknown_208[0x2c4 - 0x208];
    void (*method_2c4)(Actor *);
    unsigned char unknown_2c8[0x2f4 - 0x2c8];
    void (*method_2f4)(Actor *);
    unsigned char unknown_2f8[0x324 - 0x2f8];
    void (*method_324)(Actor *);
};

typedef struct Eng3Host Eng3Host;
struct Eng3Host {
    unsigned char unknown_00[0xb0];
    void (*method_b0)(Actor *);
};

typedef struct Eng3Middle Eng3Middle;
struct Eng3Middle {
    unsigned char unknown_00[0x10];
    void (*method_10)(Actor *);
};

/* Interface context (g_engine_interface.context_004). */
typedef struct LevelContext {
    signed char level;
    signed char sublevel;
    unsigned char unknown_02;
    unsigned char next_state;
    unsigned char state;
    unsigned char paused;
    unsigned char mode;
    unsigned char unknown_07[0x1e - 0x07];
    short bar_length_target;
    short bar_length;
    short bar_value;
    short bar_value_target;
} LevelContext;

typedef struct EngineInterfaceHead {
    unsigned long signature;
    LevelContext *context;
    unsigned long unknown_08[3];
    short *sine;
} EngineInterfaceHead;

typedef struct Rect16 {
    short x, y, w, h;
} Rect16;

/* PSX-style sprite primitive and frame list. */
typedef struct Sprite {
    void *tag;
    unsigned char r0, g0, b0, code;
    short x0, y0;
    unsigned char u0, v0;
    unsigned short clut;
    short w, h;
} Sprite;

typedef struct PrimTag {
    void *next;
} PrimTag;

typedef struct FrameBuffer {
    unsigned char unknown_0000[0x2070];
    void *list;
} FrameBuffer;

extern EngineInterfaceHead g_engine_interface;
extern Actor *g_focus;
extern Eng3Game *g_eng3_game_40b8c;
extern Eng3Middle *g_eng3_middle_40b98;
extern Eng3Host *g_eng3_host_40bb4;
extern short g_eng3_mode_41cb4;
extern FrameBuffer *g_eng3_frame_41448;
extern unsigned char *g_eng3_prim_43164;
extern unsigned long g_eng3_color_43168;
extern long g_eng3_timer_3faa8;
typedef struct HudCells {
    unsigned short cell[2];
    unsigned short cached[2];
    unsigned short gauge;
} HudCells;

extern HudCells g_eng3_hud_40110;
extern unsigned char g_eng3_hud_cells_396e8[][2];
extern Rect16 g_eng3_rect_39868;
extern char g_eng3_data_path_3fc80[];
extern char *g_eng3_panel_file_3529c;

extern int eng3_24280(int, int, int);
extern void dispatch_local_1f40(void);
extern void dispatch_local_2c60(Rect16 *rect, int x, int y);
extern unsigned int eng3_01010(unsigned int);
extern unsigned int random_limit(int);
extern void vec_diff_fixed_1420(const int *, const int *, short *);
extern int eng3_01600(Actor *);
extern void play_sound(unsigned short, Actor *);
extern Actor *attach_actor(Actor *, int);
extern Actor *eng3_1cb00(long, long, long, int);
extern Actor *create_eng3_middle_actor(long, long, long, int);
extern Actor *create_eng3_kind_actor(long, long, long, int);
extern void select_frame(Actor *, int);
extern void remove_actor(Actor *);
extern void eng3_1eeb0(Actor *);
extern int eng3_1dbc0(Actor *);
extern int eng3_1b920(Actor *, int);
extern int eng3_1ec20(Actor *);
extern int eng3_1ecf0(long, long, long, int);
extern int eng1_2a660(Actor *, Actor *);
extern void eng3_19200(void);
extern ActorGroup *acquire_pending_action_group(void);
extern Actor *eng3_1f410(ActorGroup *, long, long, long, int, int);
extern void eng3_slot_040(void *value);
extern unsigned long eng3_slot_068(const char *a, const char *b, void *c, unsigned long d);
extern void *eng3_slot_078(void **a, unsigned long b, unsigned long c);
extern void eng3_slot_16c(unsigned long value);
extern unsigned short eng3_slot_1e8(int x, int y);
extern void eng3_slot_204(Rect16 *rect, void *pixels);
extern void eng3_slot_2ac(void *p, unsigned long a, unsigned long b, unsigned long c, unsigned long d);

void eng3_25620(int x, int y, int u, int v, int w, int h, int mode);

int eng3_25040(int a, int b, int c)
{
    return eng3_24280(a, b, c);
}

void eng3_25060(void)
{
    void *pixels;

    g_eng3_hud_40110.cached[0] = 0xffff;
    g_eng3_hud_40110.cached[1] = 0xffff;
    pixels = 0;
    g_eng3_hud_40110.cell[0] = 0;
    g_eng3_hud_40110.cell[1] = 0;
    eng3_slot_078(&pixels, 0x21000, 0);
    eng3_slot_068(g_eng3_data_path_3fc80, g_eng3_panel_file_3529c, pixels, 0x21000);
    eng3_slot_204(&g_eng3_rect_39868, pixels);
    eng3_slot_16c(0);
    eng3_slot_040(pixels);
}

void eng3_250f0(void)
{
    int seconds;
    int length;
    int value;
    int gauge;
    int span;
    int count;
    int i;
    int rows;
    int remainder;
    int extra;
    int base;
    Rect16 rect;

    if (g_engine_interface.context->mode == 3) {
        seconds = (g_eng3_timer_3faa8 + 30) / 60;
        eng3_25620(0x18, 0x20, 0xf0, seconds / 10 * 16 + 0x28, 16, 16, 3);
        eng3_25620(0x25, 0x20, 0xf0, seconds % 10 * 16 + 0x28, 16, 16, 3);
        if (--g_eng3_timer_3faa8 < 0)
            g_engine_interface.context->next_state = 0x13;
    }
    if (g_engine_interface.context->state == 0) {
        for (i = 0; i < 2; i++) {
            if (g_engine_interface.context->bar_value < g_engine_interface.context->bar_value_target)
                g_engine_interface.context->bar_value++;
            else if (g_engine_interface.context->bar_value > g_engine_interface.context->bar_value_target)
                g_engine_interface.context->bar_value--;
        }
        for (i = 0; i < 3; i++) {
            if (g_engine_interface.context->bar_length < g_engine_interface.context->bar_length_target)
                g_engine_interface.context->bar_length++;
            else if (g_engine_interface.context->bar_length > g_engine_interface.context->bar_length_target)
                g_engine_interface.context->bar_length--;
        }
    }
    rect.w = 12;
    rect.h = 24;
    if (g_eng3_hud_40110.cell[0] != g_eng3_hud_40110.cached[0]) {
        g_eng3_hud_40110.cached[0] = g_eng3_hud_40110.cell[0];
        rect.x = g_eng3_hud_cells_396e8[g_eng3_hud_40110.cell[0]][0] * 12;
        rect.y = g_eng3_hud_cells_396e8[g_eng3_hud_40110.cell[0]][1] * 24 + 0x238;
        dispatch_local_2c60(&rect, 0, 0x238);
    }
    if (g_eng3_hud_40110.cell[1] != g_eng3_hud_40110.cached[1]) {
        g_eng3_hud_40110.cached[1] = g_eng3_hud_40110.cell[1];
        rect.x = g_eng3_hud_cells_396e8[g_eng3_hud_40110.cell[1]][0] * 12;
        rect.y = g_eng3_hud_cells_396e8[g_eng3_hud_40110.cell[1]][1] * 24 + 0x238;
        dispatch_local_2c60(&rect, 0xc, 0x238);
    }
    length = g_engine_interface.context->bar_length * 134 / 256;
    if (length < 0x1c)
        length = 0x1c;
    else if (length > 0xff)
        length = 0xff;
    value = g_engine_interface.context->bar_value * 134 / 256;
    if (value < 0)
        value = 0;
    else if (value > length - 1)
        value = length - 1;
    if (g_eng3_hud_40110.gauge) {
        gauge = 0x12 - ((g_eng3_hud_40110.gauge + 8) * 18 >> 8);
        eng3_25620(0x117, gauge + 0xb9, 0xc8, gauge, 0x18, 0x12 - gauge, 1);
    }
    eng3_25620(0xec, 0xb8, 0x88, 0, 0x40, 0x1f, 0);
    eng3_25620(length + 0x41, 0xc7, 0xc8, 0x50, 0x24, 0x10, 0);
    eng3_25620(0x10, 0xb8, 0, 0, 0x50, 0x25, 0);
    eng3_25620(0x60, 0xd4, 0xc0, 0x20, 0x10, 5, 0);
    eng3_25620(0x2b, 0xbb, 0, 0x38, 0x18, 0x18, 2);
    eng3_25620(0x103, 0xbb, 0x18, 0x38, 0x18, 0x18, 2);
    span = length - 12;
    count = span / 64;
    for (i = 0; i < count; i++)
        eng3_25620(0x5c + i * 64, 0xca, 0x18, 0x28, 0x40, 0xd, 0);
    eng3_25620(0x5c + i * 64, 0xca, 0x18, 0x28, span & 0x3f, 0xd, 0);
    remainder = value % 24;
    rows = value / 24;
    for (i = 0; i < rows; i++)
        eng3_25620(0x52 + i * 24, 0xcd, 0, 0x28, 0x18, 6, 1);
    eng3_25620(0x52 + i * 24, 0xcd, 0, 0x28, remainder, 6, 1);
    extra = length / 24 - rows;
    if (extra == 0) {
        eng3_25620(0x52 + value, 0xcd, remainder, 0x30, length - value, 6, 1);
    } else {
        eng3_25620(0x52 + value, 0xcd, remainder, 0x30, 24 - remainder, 6, 1);
        base = (rows + 1) * 24;
        for (i = 0; i < extra - 1; i++)
            eng3_25620(0x52 + base + i * 24, 0xcd, 0, 0x30, 0x18, 6, 1);
        length = length - base - i * 24;
        if (length > 0)
            eng3_25620(0x52 + base + i * 24, 0xcd, 0, 0x30, length, 6, 1);
    }
    dispatch_local_1f40();
    eng3_slot_2ac(g_eng3_prim_43164, 0, 0, 0x180, 0);
    ((PrimTag *)g_eng3_prim_43164)->next = g_eng3_frame_41448->list;
    g_eng3_frame_41448->list = g_eng3_prim_43164;
    g_eng3_prim_43164 += 0xc;
}

void eng3_25620(int x, int y, int u, int v, int w, int h, int mode)
{
    Sprite *p;

    p = (Sprite *)g_eng3_prim_43164;
    p->clut = eng3_slot_1e8(0x160, mode + 0x20);
    p->x0 = x;
    p->y0 = y;
    p->u0 = u;
    p->v0 = v;
    p->w = w;
    p->h = h;
    *(unsigned long *)&p->r0 = g_eng3_color_43168;
    p->code = 0x64;
    if (mode == 4)
        p->code = 0x66;
    p->tag = g_eng3_frame_41448->list;
    g_eng3_frame_41448->list = p;
    g_eng3_prim_43164 = (unsigned char *)(p + 1);
    if (mode == 4) {
        eng3_slot_2ac(g_eng3_prim_43164, 0, 0, 0x180, 0);
        ((PrimTag *)g_eng3_prim_43164)->next = g_eng3_frame_41448->list;
        g_eng3_frame_41448->list = g_eng3_prim_43164;
        g_eng3_prim_43164 += 0xc;
    }
}

void eng3_25700(Actor *o)
{
    if (o->state == 0)
        o->state = 8;
    o->flags = 0x202;
    attach_actor(o, 0x2017);
    select_frame(o, 0);
    o->sprite = 0;
    o->ticks = 1;
    o->unknown_4a = 0xfe;
    o->unknown_cc = 1;
    o->unknown_11e = 0;
    o->unknown_126 = 0;
    o->unknown_116 = 1;
}

void eng3_25770(Actor *o)
{
    short d[6];
    Actor *child;

    if (o->unknown_116 == 0)
        return;
    if (o->state == 8) {
        o->x = g_focus->x - 0x1800000;
        o->y = -0x400000;
        if (eng3_01010(0x100) & 1)
            o->z = 0x100000;
        else
            o->z = -0x100000;
        o->unknown_118 = eng3_1cb00(o->x - 0x400000, o->y, o->z, 0x3f);
        if (o->unknown_118) {
            g_eng3_game_40b8c->method_2f4(o->unknown_118);
            o->unknown_118->profile = o->profile;
        }
        play_sound(0x282, 0);
        o->state = 2;
    }
    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (d[0] > 0x258)
        o->y -= 0x20000;
    if (d[0] > -0x80 && d[0] <= 0x258 && d[0] > 0x19c)
        o->state = 4;
    if (d[0] > 0x258)
        o->state = 0x20;
    if (o->state == 4) {
        if (o->unknown_126-- < 0) {
            o->unknown_11e++;
            if (o->unknown_11e < 6) {
                child = eng3_1cb00(o->x, o->y, o->z, 0x2a);
                if (child) {
                    g_eng3_game_40b8c->method_1f8(child);
                    play_sound(0x2a84, o);
                    child->profile = o->profile;
                }
            }
            o->unknown_126 = 2;
        }
    }
    o->x += 0x160000;
    if (o->unknown_118) {
        o->unknown_118->x = o->x - 0x200000;
        o->unknown_118->y = o->y;
        o->unknown_118->z = o->z;
    }
    eng3_1dbc0(o);
    if (d[0] > 0x400) {
        o->unknown_116 = 0;
        if (o->unknown_118)
            remove_actor(o->unknown_118);
        remove_actor(o);
    }
}

void eng3_25960(Actor *o)
{
    if (o->state == 0)
        o->state = 8;
    o->unknown_11e = 0;
    o->unknown_98 = 0;
    o->flags = 0x202;
    o->render_flags = 0x80000005;
    o->unknown_4a = 0xfe;
    o->ticks = 1;
    o->unknown_126 = 0x1e;
    o->scale = 0x12c;
    o->unknown_b8 = 0xa0000;
    o->unknown_c0 = 0x60000;
    o->unknown_c8 = 0x10000;
    o->unknown_dc = o->y;
    select_frame(o, 1);
}

void eng3_259e0(Actor *o)
{
    Actor *child;

    if (o->state & 8)
        o->state = 2;
    if (o->state & 2) {
        o->y = o->unknown_dc - (((g_engine_interface.sine[(o->unknown_11e - 0x400) & 0xfff] * 64) >> 12) << 16);
        o->unknown_11e += 0x60;
        if (o->unknown_11e >= 0x7d0)
            o->state = 4;
    }
    if (o->state & 4) {
        o->y += o->unknown_b8;
        o->unknown_116 = eng3_1ec20(o);
        if (o->unknown_116) {
            o->unknown_c8 = 0;
            child = eng3_1cb00(o->x, o->y, o->z, 0x2b);
            if (child)
                g_eng3_game_40b8c->method_204(child);
            play_sound(0xe285, o);
            remove_actor(o);
            return;
        }
    }
    if (o->state & 6) {
        if (o->unknown_98)
            o->z += o->unknown_c8;
        else
            o->z -= o->unknown_c8;
    }
    eng3_1dbc0(o);
}

void eng3_25b00(Actor *o)
{
    if (o->state == 0)
        o->state = 2;
    o->flags = 0x202;
    o->render_flags = 0x80000005;
    o->unknown_e0 = 0;
    o->unknown_d8 = 0;
    o->unknown_dc = 1;
    o->unknown_126 = 0;
    o->unknown_11e = 0;
    o->ticks = 1;
    o->unknown_4a = 0;
    o->state = 2;
    select_frame(o, 0);
}

void eng3_25b60(Actor *o)
{
    short d[6];
    short a;

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    o->unknown_11e &= 0xfff;
    a = o->unknown_11e;
    if (d[0] >= 0)
        o->unknown_4a = 0x94 - (d[0] * 0x5765 >> 16);
    if (d[0] < 0)
        o->unknown_4a = 0x94;
    if (o->unknown_e0 >= 0x1000000)
        o->state = 4;
    if (o->unknown_e0 <= 0)
        o->state = 2;
    if (o->state & 2)
        o->unknown_e0 += 0x20000;
    if (o->state & 4)
        o->unknown_e0 -= 0x20000;
    o->z = g_focus->z - (((g_engine_interface.sine[(a - 0x400) & 0xfff] * 128) >> 12) << 16);
    o->x = g_focus->x - (((g_engine_interface.sine[a & 0xfff] * 128) >> 12) << 16) + o->unknown_e0;
    o->y = g_focus->y - (((g_engine_interface.sine[(a * 2 - 0x400) & 0xfff] * 32) >> 12) << 16) - 0x400000;
    o->unknown_11e = (a + 0x20) & 0xfff;
    eng3_1dbc0(o);
}

void eng3_25cb0(Actor *o)
{
    if (o->state == 0)
        o->state = 2;
    o->flags = 0x202;
    o->render_flags = 0x80000005;
    o->unknown_4a = 0xfe;
    o->unknown_e0 = 0;
    o->unknown_126 = 0x1c;
    o->unknown_11e = 0;
    o->ticks = 1;
    o->state = 2;
    o->x = g_focus->x + 0x9000000;
    o->unknown_1c = 0xe;
    select_frame(o, 1);
}

void eng3_25d20(Actor *o)
{
    o->profile = g_focus->profile;
    o->flags = 0x202;
    o->render_flags = 0x80000005;
    o->unknown_4a = 0xfe;
    o->state = 2;
    o->ticks = 1;
    o->unknown_e0 = 0;
    o->unknown_116 = 0x1e;
    eng3_1eeb0(o);
    select_frame(o, 3);
}

void eng3_25d80(Actor *o)
{
    short d[6];

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    eng3_1dbc0(o);
    if (o->unknown_e0 == 0 && o->phase == 10) {
        select_frame(o, 2);
        o->phase = 0;
        o->unknown_e0 = 1;
    }
    if (eng1_2a660(o, g_focus)) {
        g_focus->unknown_110 = o;
        g_focus->unknown_116 = 0x30;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_25e30(Actor *o)
{
    if (o) {
        o->profile = g_focus->profile;
        o->flags = 0x202;
        o->render_flags = 0x80000007;
        o->unknown_4a = 0xb4;
        o->state = 2;
        o->ticks = 1;
        o->unknown_e0 = 0;
        o->unknown_116 = 0x1e;
        select_frame(o, 0);
        o->sprite = 0x47;
    }
}

void eng3_25e90(Actor *o)
{
    o->unknown_4a -= 0xf;
    o->scale -= 8;
    if (o->unknown_4a <= 0x1e)
        remove_actor(o);
}

void eng3_25ec0(Actor *o)
{
    int dx[24] = {0, 1, 2, 1, 0, -1, -2, -1, 3, 2, 1, 2, 3, 0, 3, 2, 3, 1, 0};
    int dz[24] = {0, 1, 2, 3, 4, 5, 6, 7, 2, 3, 4, 5, 5, 6, 6, 6, 7, 7, 7, 7};
    int dy[24] = {2, 1, 0, -1, -2, -1, 0, 1, 2, 3, 4, 5, 5, 6, 6, 6, 7, 7, 7, 7};
    ActorGroup *group;
    Actor *child;
    int i;

    if (o->state == 0)
        o->state = 8;
    o->unknown_dc = 4;
    o->sprite = 0;
    o->flags = 2;
    o->unknown_116 = 0;
    o->unknown_11e = 0x180;
    o->ticks = 1;
    o->unknown_98 = 0x40;
    o->unknown_126 = 0xa;
    o->unknown_e0 = 100;
    group = acquire_pending_action_group();
    if (group == 0)
        return;
    o->unknown_120 = group;
    for (i = 0; i < o->unknown_dc; i++) {
        child = eng3_1f410(group,
                           ((dz[i] * o->unknown_11e) << 16) + o->x,
                           dy[i] * (o->unknown_98 << 15) + o->y,
                           dx[i] * (o->unknown_98 << 16) + o->z,
                           0x4029, o->profile);
        if (child) {
            child->ticks = 1;
            child->flags = 2;
            child->state = 2;
            child->unknown_97 = 1;
            child->unknown_94 = 0xff;
            child->unknown_96 = 0xff;
            child->unknown_d8 = 0;
            child->unknown_dc = 1;
            child->y -= 0x400000;
            child->render_flags = 0x80000004;
            child->unknown_4a = 0xfe;
            child->scale = 0x180;
            select_frame(o, 0);
            child->unknown_d8 = 0;
            child->script = o->script;
            child->phase += eng3_01010(10);
            child->state = 8;
            child->unknown_dc = eng3_01010(0x10) + 0x18;
            child->unknown_e0 = eng3_01010(0xfff);
            child->unknown_97 = eng3_01010(0xfff);
            child->unknown_ac = random_limit(0x80000) + 0x10000;
            child->unknown_b0 = random_limit(0x4000) + 0x2000;
            child->unknown_a8 = 0;
            child->unknown_d0 = random_limit(0x4000) + 0x2000;
            child->unknown_cc = 0;
            child->unknown_c8 = 0x60 - eng3_01010(0x20);
            child->unknown_b8 = 0x40 - eng3_01010(0x10);
            child->unknown_c0 = random_limit(0x4000) + 0x2000;
            child->unknown_bc = random_limit(0x10000) + 0x20000;
        }
    }
}

void eng3_26340(Actor *o)
{
    short d[6];
    ActorGroup *group;
    Actor *a;
    Actor *child;
    int i;

    group = o->unknown_120;
    if (o->profile != g_focus->profile) {
        remove_actor(o);
        return;
    }
    if (group) {
        for (i = 0; i < group->count;) {
            a = group->actors[i++];
            if (i == 1 && o->unknown_e0-- < 0) {
                o->unknown_dc = random_limit(4);
                if (o->unknown_dc == 0)
                    play_sound(0xe28e, a);
                if (o->unknown_dc == 1)
                    play_sound(0xe28f, a);
                if (o->unknown_dc == 2)
                    play_sound(0xe290, a);
                if (o->unknown_dc == 3)
                    play_sound(0xe291, a);
                o->unknown_e0 = 100;
            }
            child = eng3_1cb00(a->x + 0x100000, a->y, a->z, 0x43);
            if (child) {
                g_eng3_game_40b8c->method_324(child);
                child->profile = o->profile;
            }
            eng3_1dbc0(a);
            a->x -= a->unknown_ac;
            a->y = (g_engine_interface.sine[(a->unknown_e0 & 0x7ff) * 2] * a->unknown_b8 - 0x40000) << 4;
            a->z = (g_engine_interface.sine[(a->unknown_e0 - 0x400) & 0xfff] * a->unknown_c8) << 4;
            a->unknown_e0 = (a->unknown_e0 + 0x20) & 0xfff;
            if (a->y < -0x1000000)
                a->unknown_95 = 0;
            if (a->y > 0xa0000)
                a->unknown_95 = 0xff;
            vec_diff_fixed_1420((const int *)a, (const int *)g_focus, d);
            if (d[0] < -0x80)
                a->x = g_focus->x + 0x9000000;
        }
    }
    if (eng3_1ecf0(g_focus->x, g_focus->y, g_focus->z, g_focus->profile) == 0xff86)
        remove_actor(o);
}

void eng3_26550(Actor *o)
{
    o->unknown_e0 = 1;
    o->flags = 0x202;
    o->render_flags = 0x80000005;
    o->state = 8;
    o->ticks = 1;
    o->unknown_116 = 0x1e;
    eng3_1eeb0(o);
    select_frame(o, 3);
    o->unknown_11e = 2;
    if ((o->profile == 3 || o->profile == 4) && o->x > 0x74000000) {
        o->y += eng3_01010(0x80) - eng3_01010(0x40);
        o->scale = 0x100 - eng3_01010(0x20);
    }
}

void eng3_26600(Actor *o)
{
    short d[6];

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    o->y += -0x8000 - random_limit(0x8000);
    eng3_1dbc0(o);
    if (o->state & 8 && o->y < -0x600000) {
        play_sound(0xe28f, o);
        o->state = 2;
    }
    if (o->state & 2) {
        select_frame(o, 4);
        o->phase = 0;
        o->state = 4;
    }
    if (o->state & 4 && o->phase >= 0xb)
        remove_actor(o);
}

void eng3_266a0(Actor *o)
{
    o->render_flags |= 0x10000040;
    o->flags = 0x202;
    o->state = 8;
    o->unknown_98 = 1;
    o->unknown_e0 = 0;
    o->unknown_116 = 0;
    o->unknown_11e = 1;
    o->unknown_95 = 0;
    o->unknown_bc = 0x1e0000;
    o->unknown_ac = 0;
    o->unknown_cc = 0xf0000;
    o->unknown_b8 = 0;
    o->unknown_c0 = 0x20000;
    o->unknown_c4 = 0xffff;
    o->ticks = 10;
    o->unknown_9a = 0;
    o->sprite = 2;
    o->unknown_72 = 0x100;
    o->y -= 0x4000000;
    o->scale = 0x200;
    o->unknown_4a = 0xfe;
    o->unknown_dc = 1;
    o->unknown_d8 = eng3_01010(0x10) + 0x60;
}

void eng3_26770(Actor *o)
{
    short d[6];
    int i;
    Actor *child;

    vec_diff_fixed_1420((const int *)o, (const int *)g_focus, d);
    if (d[0] < 0x484 && o->unknown_11e) {
        for (i = 0; i < 5; i++) {
            child = eng3_1cb00(o->x + ((eng3_01010(0x14) - eng3_01010(0x14)) << 16),
                               o->y - (eng3_01010(0x20) << 16) + (eng3_01010(0x20) << 16),
                               o->z + ((eng3_01010(0xf) - eng3_01010(0xf)) << 16), 0x3b);
            if (child) {
                g_eng3_game_40b8c->method_2c4(child);
                child->unknown_b8 = child->unknown_bc = ((eng3_01010(0x10) + 0x14) << 16) - (eng3_01010(0x10) << 16);
                child->unknown_a8 = 0;
                child->unknown_c8 = 0;
                child->category = 4;
                child->unknown_c4 = 0x8000;
                child->phase = 0;
                child->unknown_ac = 0;
                child->unknown_cc = 0;
                child->scale = random_limit(0x40) + 0x100;
                child->unknown_4a = 0xa0;
                child->unknown_95 = 0;
            }
        }
        o->unknown_11e = 0;
    }
    if (d[0] < 0x3e8 && o->unknown_116 == 0) {
        play_sound(0x6280, o);
        o->state = 4;
        o->unknown_116 = 1;
    }
    if (d[2] > -0x20 && d[2] < 0x20 && d[0] > -0x20 && d[0] < 0x20 && d[1] - 0x20 != 0 && d[1] < 0x20) {
        g_focus->unknown_116 = 0x31;
        g_focus->unknown_110 = o;
        if (g_eng3_mode_41cb4 != 5)
            eng3_19200();
    }
    if (o->state & 4) {
        o->unknown_72 += (short)o->unknown_d8;
        if (o->unknown_9a) {
            o->unknown_d8 >>= 1;
            if (o->unknown_dc) {
                play_sound(0xe281, o);
                o->unknown_dc = 0;
            }
            if (o->unknown_98 == 1) {
                child = create_eng3_kind_actor(o->x - 0x800000, o->y, o->z, 0x2016);
                if (child)
                    g_eng3_host_40bb4->method_b0(child);
                for (i = 0; i < 4; i++) {
                    child = create_eng3_middle_actor(o->x, o->y + random_limit(0x200000),
                                                     o->z + random_limit(0x200000), 0x4002);
                    if (!child)
                        continue;
                    g_eng3_middle_40b98->method_10(child);
                    if (i == 0 || i == 4) {
                        child->unknown_94 = 0xff;
                        child->unknown_96 = 0;
                    }
                    if (i == 1 || i == 5) {
                        child->unknown_94 = 0;
                        child->unknown_96 = 0;
                    }
                    if (i == 2 || i == 6) {
                        child->unknown_94 = 0xff;
                        child->unknown_96 = 0xff;
                    }
                    if (i == 3 || i == 7) {
                        child->unknown_94 = 0;
                        child->unknown_96 = 0xff;
                        o->unknown_98 = 0;
                    }
                }
            }
        }
        if (eng3_01600(o)) {
            o->unknown_d8 = 0;
            o->state = 2;
            o->sprite = 3;
        }
    }
    if (eng3_1b920(o, 0x100))
        remove_actor(o);
}

void eng3_26af0(Actor *o)
{
    o->profile = g_focus->profile;
    o->flags = 0x202;
    o->render_flags = 0x80000005;
    o->unknown_4a = 0x40;
    o->state = 2;
    o->ticks = 1;
    o->unknown_e0 = 0;
    o->unknown_116 = 0x1e;
    select_frame(o, 7);
}
