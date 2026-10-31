/* ENG1.DLL 0x283e0..0x2a700: actor program interpreters, kind change, command
 * attach, tag lookup, floor/ceiling (support) searches over the solid-actor
 * worklist and the actor box queries. Region lane x3.
 * Local views only cover offsets observed in these bodies.
 * Unit hypothesis: contiguous ENG1 0x282e0..0x2a660 including the recorded
 * neighbours (command table, frame motion/state, chain/scene links, detach
 * helpers, planar_bounds_overlap); point_in_planar_bounds (0x2a520) is left to
 * its own recorded unit. Compiled as C++ (change_kind SIB order) with
 * <string.h> as a TU-context (declaration-count) hypothesis only. */

#include <stdlib.h>
#include <string.h>

extern "C" {

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
    unsigned char unknown_0ac[0xb8 - 0xac];
    long unknown_0b8;                   /* 0xb8 */
    unsigned char unknown_0bc[0xc8 - 0xbc];
    long unknown_0c8;                   /* 0xc8 */
    unsigned char unknown_0cc[0xd8 - 0xcc];
    unsigned long unknown_0d8;          /* 0xd8 */
    unsigned char unknown_0dc[0xe4 - 0xdc];
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
    short unknown_126;
    unsigned short **command;           /* 0x128 */
    Actor *prev_chain, *next_chain;     /* 0x12c */
};

typedef struct CommandPair { short opcode, operand; } CommandPair;
typedef struct CommandTriple { short opcode, operand_002, operand_004; } CommandTriple;

typedef struct AnimationControl {
    unsigned short attributes;
    short x_current, x_target, x_step;
    short yaw_target, yaw_step;
    short pitch_target, pitch_step;
    short roll_target, roll_step;
} AnimationControl;

typedef struct Asset8 {
    void *image;
    unsigned short unknown_004;
    unsigned char category, unknown_007;
} Asset8;
typedef struct Asset16 {
    void *image;
    unsigned char unknown_004[8];
    unsigned char category;
    unsigned char unknown_00d[3];
} Asset16;
typedef void (*ActorCallback)(Actor *);
typedef struct Callback8 { ActorCallback init; void *unknown_004; } Callback8;
typedef struct Callback12 { ActorCallback init; void *unknown_004, *unknown_008; } Callback12;

typedef struct KindInfo { long offset; unsigned long count; } KindInfo;

typedef struct EngineContext { signed char level; } EngineContext;
typedef struct EngineInterfaceHead {
    unsigned long signature;
    EngineContext *context;
} EngineInterfaceHead;

extern EngineInterfaceHead g_engine_interface;
extern Asset8 *g_special_assets, *g_alternate_assets;
extern Asset16 *g_normal_assets;
extern Callback8 *g_configuration_dword_7137c, *g_configuration_dword_7139c;
extern Callback12 *g_configuration_dword_71370;
extern ActorCallback *g_configuration_dword_71384;
extern KindInfo g_kind_info[];
extern long g_actor_worklist_714c0[];
extern long g_actor_worklist_716c0[];
extern long g_actor_worklist_71b20[];
extern Actor *g_support_actor_6fed8;
extern Actor *g_eng1_spatial_match_actor_6fedc;

extern int remove_actor(Actor *p);
extern void remove_actor_preserving_resource(Actor *p);
extern void mark_actor_resource_for_release(Actor *p);
extern void spawn_actor_with_owner(Actor *p, long x, long y, long z, short kind);
extern void title_17ad0(long x, long y, long z, short kind, int group);
extern void play_sound(short sound, Actor *p);
extern void stop_actor_sounds(Actor *p);
extern int wrapped_ratio(int from, int to, int steps);
extern int dispatch_engine_2c460(int index);
extern long grid_scan_20b50(long x, long y, long z, int group);
extern long grid_query_20d30(long x, long y, long z, int step);
extern int extent_producer_30e40(Actor *p, Bounds3D *b, int mode);
extern int point_in_box_2a810(long x, long y, long z, Bounds3D *b);
extern int actor_bounds_2a880(Actor *p, Bounds3D *b, int mode);

extern int release_actor(Actor *p);
extern void remove_fx_links(Actor *p);
extern void *engine_pool_allocate(void **slot, unsigned long bytes, unsigned long flags);
extern unsigned short *g_command_slot_storage_71320[20];
extern Actor *g_scene_anchor;
void reset_frame_state(Actor *p);
void advance_frame(Actor *p);
void select_frame(Actor *p, int index);
Actor *insert_scene(Actor *p, Actor *after);
void unlink_0e4(Actor *p);
long eng1_2a330(Actor *p, long x, long z);
int eng1_2a220(Actor *p, Bounds3D *b);

#define PAIR(p) (((CommandPair *)(p)->image)[(p)->frame_state])
#define WORD(p) (((short *)(p)->image)[(p)->frame_state])
#define TRIPLE(p) (((CommandTriple *)(p)->image)[(p)->frame_state])

void reset_command_table(void)
{
    unsigned long index;
    for (index = 0; index < 20; ++index)
        g_command_slot_storage_71320[index] = 0;
}

int initialize_command_table(unsigned long bytes, unsigned int opcode, unsigned long count)
{
    unsigned long index;
    unsigned long remaining;
    unsigned short *command;

    remaining = count;
    for (;;) {
        for (index = 0; index < 20; ++index) {
            if (!g_command_slot_storage_71320[index])
                break;
        }
        if (index == 20)
            return 0;
        command = (unsigned short *)engine_pool_allocate((void **)&g_command_slot_storage_71320[index], bytes, 0);
        if (!command)
            return 0;
        *command = (unsigned short)opcode;
        if (--remaining == 0)
            return 1;
    }
}

unsigned short **allocate_command(unsigned int opcode)
{
    int index;

    for (index = 0; index < 20; ++index) {
        if (g_command_slot_storage_71320[index] &&
            (unsigned int)g_command_slot_storage_71320[index][0] == opcode)
            return &g_command_slot_storage_71320[index];
    }
    return 0;
}

void clear_command_high_bit(Actor *p)
{
    if (!(p->flags & 0x10000000)) {
        if (p->command) {
            unsigned short *command = *p->command;
            unsigned short code = *command;
            if ((code & ~0x7f) == 0xb780)
                *command = code & 0x7fff;
        }
    }
}

int eng1_interpret_signed_pairs(Actor *p)
{
    CommandPair *rec;
    Actor *child;
    long *position;
    short op;
    int result = 0;

    if (--p->frame_ticks < 0) {
        do {
            ++p->frame_state;
            rec = &PAIR(p);
            op = rec->opcode;
            if (op < 0)
            switch (op) {
            case -2:
                p->direction_x = ~p->direction_x;
                break;
            case -3:
                p->render_flags ^= 0x10;
                break;
            case -5:
                spawn_actor_with_owner(p, p->x, p->y, p->z, rec->operand);
                break;
            case -31:
                title_17ad0(p->x, p->y, p->z, rec->operand, p->frame_group);
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
            case -4:
                advance_frame(p);
            case -1:
                p->frame_state = -1;
                result = 1;
                break;
            case -10:
                g_configuration_dword_71384[p->action_callback_index](p);
                break;
            case -11:
                g_configuration_dword_71384[rec->operand](p);
                break;
            case -12:
                select_frame(p, rec->operand);
                result = 1;
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
                p->step_index = rec->operand;
                break;
            case -17:
                p->vertical_step = (unsigned char)rec->operand;
                break;
            case -18:
                p->category = (unsigned char)rec->operand;
                break;
            case -20:
                p->render_flags |= rec->operand;
                break;
            case -21:
                p->render_flags |= rec->operand << 16;
                break;
            case -22:
                p->render_flags &= ~rec->operand;
                break;
            case -23:
                p->render_flags &= ~(rec->operand << 16);
                break;
            case -24:
                p->unknown_036 = rec->operand;
                break;
            case -26:
                if (p->direction_x)
                    p->x -= rec->operand << 16;
                else
                    p->x += rec->operand << 16;
                break;
            case -27:
                p->y += rec->operand << 16;
                break;
            case -28:
                p->z += rec->operand << 16;
                break;
            case -29:
                p->flags |= rec->operand;
                break;
            case -30:
                p->flags &= ~rec->operand;
                break;
            case -32:
                p->interpolation_position = rec->operand;
                break;
            case -39:
                play_sound(rec->operand, 0);
                break;
            case -40:
                play_sound(rec->operand, p);
                break;
            case -46:
                stop_actor_sounds(p);
                break;
            case -41:
                p->unknown_02a = rec->operand;
                break;
            case -43:
                p->unknown_0a8 = rec->operand << 8;
                break;
            case -44:
                p->unknown_0b8 = rec->operand << 8;
                break;
            case -45:
                p->unknown_0c8 = rec->operand << 8;
                break;
            case -25:
                position = (long *)p->unknown_058;
                if (position) {
                    position = (long *)((char *)position + g_kind_info[p->frame_group].offset);
                    p->x = position[0];
                    p->y = position[1];
                    p->z = position[2];
                }
                break;
            case -47:
                child = p->child;
                if (child) {
                    p->render_flags |= 0x800000;
                    child->active_command = rec->operand;
                    child->render_flags = p->direction_x & 0x10 | child->render_flags & ~0x10;
                }
                break;
            case -48:
                p->render_flags &= ~0x800000;
                break;
            }
        } while (op < 0);
        p->active_command = op;
        p->frame_ticks = PAIR(p).operand;
    }
    return result;
}

int eng1_interpret_word_stream(Actor *p)
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

int eng1_interpret_animation_triples(Actor *p)
{
    AnimationControl *c;
    long *position;
    short op;
    short ticks;
    short target;
    short dest;
    int result = 0;

    if (!p->command)
        return 0;
    c = (AnimationControl *)*p->command;
    if (--p->frame_ticks < 0) {
        do {
            ++p->frame_state;
            op = TRIPLE(p).opcode;
            if (op < 0)
            switch (op) {
            case -2:
                p->direction_x = ~p->direction_x;
                break;
            case -3:
                p->render_flags ^= 0x10;
                break;
            case -5:
                spawn_actor_with_owner(p, p->x, p->y, p->z, PAIR(p).operand);
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
            case -4:
                advance_frame(p);
                p->frame_state = -1;
                result = 1;
                break;
            case -10:
                g_configuration_dword_71384[p->action_callback_index](p);
                break;
            case -11:
                g_configuration_dword_71384[TRIPLE(p).operand_004](p);
                break;
            case -12:
                select_frame(p, TRIPLE(p).operand_004);
                result = 1;
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
                p->step_index = TRIPLE(p).operand_004;
                break;
            case -20:
                p->render_flags |= TRIPLE(p).operand_004;
                break;
            case -21:
                p->render_flags |= TRIPLE(p).operand_004 << 16;
                break;
            case -22:
                p->render_flags &= ~TRIPLE(p).operand_004;
                break;
            case -23:
                p->render_flags &= ~(TRIPLE(p).operand_004 << 16);
                break;
            case -24:
                p->unknown_036 = TRIPLE(p).operand_004;
                c->x_target = TRIPLE(p).operand_002;
                break;
            case -26:
                if (p->direction_x)
                    p->x -= TRIPLE(p).operand_004 << 16;
                else
                    p->x += TRIPLE(p).operand_004 << 16;
                break;
            case -27:
                p->y += TRIPLE(p).operand_004 << 16;
                break;
            case -28:
                p->z += TRIPLE(p).operand_004 << 16;
                break;
            case -29:
                p->flags |= TRIPLE(p).operand_004;
                break;
            case -30:
                p->flags &= ~TRIPLE(p).operand_004;
                break;
            case -32:
                p->interpolation_position = TRIPLE(p).operand_004;
                break;
            case -33:
                c->yaw_target = TRIPLE(p).operand_002;
                c->yaw_step = wrapped_ratio(p->angle_yaw, c->yaw_target, TRIPLE(p).operand_004);
                break;
            case -34:
                c->pitch_target = TRIPLE(p).operand_002;
                c->pitch_step = wrapped_ratio(p->angle_pitch, c->pitch_target, TRIPLE(p).operand_004);
                break;
            case -35:
                c->roll_target = TRIPLE(p).operand_002;
                c->roll_step = wrapped_ratio(p->angle_roll, c->roll_target, TRIPLE(p).operand_004);
                break;
            case -36:
                c->yaw_target = (TRIPLE(p).operand_002 + c->yaw_target) & 0x3ff;
                c->yaw_step = wrapped_ratio(p->angle_yaw, c->yaw_target, TRIPLE(p).operand_004);
                break;
            case -37:
                c->pitch_target = (TRIPLE(p).operand_002 + c->pitch_target) & 0x3ff;
                c->pitch_step = wrapped_ratio(p->angle_pitch, c->pitch_target, TRIPLE(p).operand_004);
                break;
            case -38:
                c->roll_target = (TRIPLE(p).operand_002 + c->roll_target) & 0x3ff;
                c->roll_step = wrapped_ratio(p->angle_roll, c->roll_target, TRIPLE(p).operand_004);
                break;
            case -39:
                play_sound(TRIPLE(p).operand_004, 0);
                break;
            case -40:
                play_sound(TRIPLE(p).operand_004, p);
                break;
            case -46:
                stop_actor_sounds(p);
                break;
            case -41:
                p->unknown_02a = TRIPLE(p).operand_004;
                break;
            case -43:
                p->unknown_0a8 = TRIPLE(p).operand_004 << 8;
                break;
            case -44:
                p->unknown_0b8 = TRIPLE(p).operand_004 << 8;
                break;
            case -45:
                p->unknown_0c8 = TRIPLE(p).operand_004 << 8;
                break;
            case -25:
                position = (long *)p->unknown_058;
                if (position) {
                    position = (long *)((char *)position + g_kind_info[p->frame_group].offset);
                    p->x = position[0];
                    p->y = position[1];
                    p->z = position[2];
                }
                break;
            case -42:
                p->image = p->frames[TRIPLE(p).operand_002];
            case -1:
                p->frame_state = -1;
                result = 1;
                break;
            }
        } while (op < 0);
        p->active_command = op;
        ticks = TRIPLE(p).operand_004;
        if (ticks & 0x4000)
            ticks = (ticks & 0x3fff) + dispatch_engine_2c460(6);
        p->frame_ticks = ticks;
        dest = c->x_target;
        target = TRIPLE(p).operand_002;
        if (target < 0) {
            c->x_current = target;
            c->x_step = -target;
        } else {
            if (ticks < 1)
                ticks = 1;
            p->interpolation_position = target;
            c->x_step = (dest - target) / ticks;
            c->x_current = target;
        }
    } else if (p->frame_ticks) {
        p->interpolation_position += c->x_step;
    } else if (c->x_current >= 0) {
        p->interpolation_position = c->x_target;
    }
    if (c->yaw_step) {
        if (abs(c->yaw_target - p->angle_yaw) < abs(c->yaw_step)) {
            p->angle_yaw = c->yaw_target;
            c->yaw_step = 0;
        } else {
            p->angle_yaw -= c->yaw_step;
        }
        p->angle_yaw &= 0x3ff;
    }
    if (c->pitch_step) {
        if (abs(c->pitch_target - p->angle_pitch) < abs(c->pitch_step)) {
            p->angle_pitch = c->pitch_target;
            c->pitch_step = 0;
        } else {
            p->angle_pitch += c->pitch_step;
        }
        p->angle_pitch &= 0x3ff;
    }
    if (c->roll_step) {
        if (abs(c->roll_target - p->angle_roll) < abs(c->roll_step)) {
            p->angle_roll = c->roll_target;
            c->roll_step = 0;
        } else {
            p->angle_roll += c->roll_step;
        }
        p->angle_roll &= 0x3ff;
    }
    return result;
}

typedef struct Step2 { short x, y; } Step2;
typedef struct Step3 { short x, y, z; } Step3;

void apply_frame_motion_2d(Actor *p)
{
    Step2 *step;
    short x;
    if (p->active_step >= 0) {
        step = (Step2 *)p->steps + p->step_index;
        x = step->x;
        if (p->direction_x) p->x -= x << 16;
        else p->x += x << 16;
        p->y += step->y << 16;
        p->z += p->vertical_step << 16;
    }
}

void apply_frame_motion_3d(Actor *p)
{
    Step3 *step;
    short x, z;
    if (p->active_step >= 0) {
        step = (Step3 *)p->steps + p->step_index;
        x = step->x;
        z = step->z;
        if (p->direction_x) p->x -= x << 16;
        else p->x += x << 16;
        p->y += step->y << 16;
        if (p->direction_z) p->z += z << 16;
        else p->z -= z << 16;
    }
}

void reset_frame_state(Actor *p)
{
    p->frame_ticks = p->frame_state = 0;
}

void advance_frame(Actor *p)
{
    void *image;
    ++p->frame_index;
    do {
        image = p->frames[p->frame_index];
        if (!image) {
            ++p->frame_index;
            p->frame_index = (short)(long)p->frames[p->frame_index];
        }
    } while (!image);
    p->image = image;
}

void restart_steps(Actor *p)
{
    p->steps = p->initial_steps;
    p->step_index = 0;
    p->active_step = 1;
}

void select_frame(Actor *p, int index)
{
    p->frame_index = index;
    p->image = p->frames[index];
    p->frame_state = -1;
    p->frame_ticks = 0;
}

void change_kind(Actor *p, unsigned short kind)
{
    if (p->kind & 0x6000) {
        if (p->kind & 0x4000) {
            p->kind = kind;
            p->category = g_special_assets[kind & 0xfff].category;
            p->image = g_special_assets[kind & 0xfff].image;
            g_configuration_dword_7137c[kind & 0xfff].init(p);
        } else {
            p->kind = kind;
            p->category = g_alternate_assets[kind & 0xfff].category;
            p->image = g_alternate_assets[kind & 0xfff].image;
            g_configuration_dword_7139c[kind & 0xfff].init(p);
        }
    } else {
        p->kind = kind;
        p->category = g_normal_assets[kind].category;
        p->image = g_normal_assets[kind].image;
        g_configuration_dword_71370[kind].init(p);
    }
    reset_frame_state(p);
    eng1_interpret_word_stream(p);
}

int create_command(Actor *p)
{
    unsigned short **handle = allocate_command(0x3781);
    unsigned short *command;
    if (handle) {
        command = *handle;
        p->flags |= 0x20000000;
        p->command = handle;
        command[0] |= 0x8000;
        command[1] = command[2] = command[3] = 0;
        command[4] = command[5] = 0;
        command[6] = command[7] = 0;
        command[8] = command[9] = 0;
        return 1;
    }
    return 0;
}

int count_chain(Actor *p)
{
    int count = 0;
    p = p->next_chain;
    while (p) { ++count; p = p->next_chain; }
    return count;
}

void append_chain(Actor *p, Actor *next)
{
    if (!((p->kind | next->kind) & 0x6000)) {
        while (p->next_chain) p = p->next_chain;
        p->next_chain = next;
        next->prev_chain = p;
    }
}

void unlink_12c(Actor *p)
{
    if (p->prev_chain) p->prev_chain->next_chain = p->next_chain;
    if (p->next_chain) p->next_chain->prev_chain = p->prev_chain;
}

void detach_attachment(Actor *p)
{
    Actor *attachment = p->attachment;

    if (attachment && !(p->flags & 0x00020000)) {
        if (p->flags & 0x00010000)
            p = attachment;
        p->attachment = 0;
        if (attachment->flags & 0x00010000)
            release_actor(attachment);
    }
}

void detach_child(Actor *p)
{
    Actor *child = p->child;

    if (child && !(p->flags & 0x40000000)) {
        p->child = 0;
        p->render_flags &= ~0x00800000;
        release_actor(child);
    }
}

void detach_buffer(Actor *p)
{
    if (!(p->flags & 0x10000000) && p->buffer) {
        p->buffer = 0;
        remove_fx_links(p);
    }
}

Actor *insert_scene(Actor *p, Actor *after)
{
    Actor *anchor;
    if (p->flags & 0x80000) return 0;
    if (!after) after = g_scene_anchor;
    anchor = after;
    if (after) {
        if (p->flags & 0x40000) {
            while (after->next_scene && !(after->next_scene->flags & 0x40000))
                after = after->next_scene;
        }
        p->next_scene = after->next_scene;
        if (p->next_scene) p->next_scene->prev_scene = p;
        after->next_scene = p;
        p->prev_scene = after;
    }
    return anchor;
}

void unlink_0e4(Actor *p)
{
    if (!(p->flags & 0x80000)) {
        if (p->prev_scene) p->prev_scene->next_scene = p->next_scene;
        if (p->next_scene) p->next_scene->prev_scene = p->prev_scene;
        p->prev_scene = 0;
        p->next_scene = 0;
    }
}

Actor *find_tag(int tag)
{
    int i, count = g_actor_worklist_716c0[0];
    Actor *p;
    for (i = 0; i < count; ) {
        p = (Actor *)g_actor_worklist_716c0[++i];
        if (p->tag == tag)
            return p;
    }
    return 0;
}

void hide_and_unlink(Actor *p)
{
    p->render_flags &= 0x7fffffff;
    unlink_12c(p);
    p->prev_chain = 0;
    p->next_chain = 0;
}

unsigned long eng1_296e0(Actor *p)
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
            d = eng1_2a330(q, x, z) - y;
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
            if (!eng1_2a220(q, &b))
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

unsigned long eng1_29870(Actor *p)
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
            d = eng1_2a330(q, x, z) - y;
            if (d < 0) {
                d = ~d + 1 >> 16;
                if ((unsigned short)d < above) {
                    above = (unsigned short)d;
                    g_support_actor_6fed8 = q;
                }
            }
        } else {
            if (!eng1_2a220(q, &b))
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

unsigned long eng1_299d0(Actor *p)
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
        if (q == p || (q->flags & 0x80000000))
            continue;
        if (q->flags & 0x200000) {
            if (!(q->flags & 0x40000) &&
                (q->x > x || q->prev_x <= x || q->z <= z || q->prev_z >= z))
                continue;
            d = eng1_2a330(q, x, z) - y;
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
            if (!eng1_2a220(q, &b))
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

long eng1_29b70(Actor *q, long x, long z)
{
    Bounds3D b;
    long y = 0x7fff;
    long left;

    if (q->flags & 0x200000) {
        if ((q->flags & 0x40000) ||
            (q->x <= x && q->prev_x > x && q->z >= z && q->prev_z < x) || x == -1)
            y = eng1_2a330(q, x, z);
    } else if (extent_producer_30e40(q, &b, 0)) {
        if (x == -1 || ((left = q->x + (b.min_x << 16)) <= x && (b.max_x << 16) + left >= x))
            y = q->y + ((unsigned long)b.min_y << 16);
    }
    return y;
}

int eng1_29c20(Actor *p)
{
    long x = p->x;
    long z;
    int limit;
    Actor *s;
    long y;
    long r;

    limit = abs((short)((x - p->prev_x) >> 16));
    if (limit < 4)
        limit += 4;
    else
        limit += 2;
    z = p->z;
    limit += abs((short)((z - p->prev_z) >> 16));
    if (!(p->flags & 0x80000)) {
        s = p->prev_scene;
        if (s) {
            while (s->prev_scene && !(s->flags & 0x40000))
                s = s->prev_scene;
            y = eng1_29b70(s, x, z);
            if (y != 0x7fff) {
                p->y = y;
                return 1;
            }
            unlink_0e4(p);
        }
        r = eng1_296e0(p);
        if (r != 0x7fff7fff) {
            if ((short)(r >> 16) <= limit) {
                p->y -= r & 0xffff0000;
            } else if ((short)r <= limit) {
                p->y = (p->y & 0xffff0000) + (r << 16);
                insert_scene(p, 0);
                return 1;
            }
        }
    }
    r = grid_scan_20b50(p->x >> 16, p->y >> 16, p->z >> 16, p->frame_group);
    if (r != 0x7fff7fff) {
        if ((short)(r >> 16) <= limit) {
            p->y = (p->y & 0xffff0000) - (r & 0xffff0000);
            return 1;
        }
        if ((short)r <= limit) {
            p->y += r << 16;
            return 1;
        }
    }
    return 0;
}

int eng1_29da0(Actor *p)
{
    int limit;
    long r;

    limit = abs((short)((p->x - p->prev_x) >> 16));
    if (limit < 4)
        limit = 4;
    limit += 8;
    r = grid_scan_20b50(p->x >> 16, p->y >> 16, p->z >> 16, p->frame_group);
    if (r != 0x7fff7fff && (short)(r >> 16) <= limit) {
        p->y = (p->y & 0xffff0000) - (r & 0xffff0000);
        return 1;
    }
    return 0;
}

int eng1_29e20(Actor *p)
{
    short dx, dz, dy;
    long r;

    dy = abs((short)((p->y - p->prev_y) >> 16));
    dx = abs((short)((p->x - p->prev_x) >> 16));
    dz = abs((short)((p->z - p->prev_z) >> 16));
    if (!(p->flags & 0x80000)) {
        if (p->prev_scene)
            unlink_0e4(p);
        r = eng1_29870(p);
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
        if ((short)(r >> 16) <= (dy + dx + dz) * 2 + 16) {
            p->y -= r & 0xffff0000;
            return 1;
        }
    }
    return 0;
}

int eng1_29f50(Actor *p)
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
        if ((short)(r >> 16) <= dz + dy + dx + 8) {
            p->y -= r & 0xffff0000;
            return 1;
        }
    }
    return 0;
}

long eng1_2a010(long x, long y, long z, int group)
{
    unsigned short above = 0x7fff, below = 0x7fff;
    Actor *q;
    long d;
    long lo;
    int i;
    Bounds3D b;
    long low, a, g;

    for (i = 0; i < g_actor_worklist_714c0[0]; ) {
        q = (Actor *)g_actor_worklist_714c0[++i];
        if (!eng1_2a220(q, &b))
            continue;
        if ((lo = q->z + (b.min_z << 16)) > z || (b.max_z << 16) + lo < z)
            continue;
        if ((lo = q->x + (b.min_x << 16)) > x || (b.max_x << 16) + lo < x)
            continue;
        d = (b.min_y << 16) + q->y - y;
        if (d < 0) {
            d = ~d + 1 >> 16;
            if ((unsigned short)d < above)
                above = (unsigned short)d;
        } else {
            d >>= 16;
            if ((unsigned short)d < below)
                below = (unsigned short)d;
        }
    }
    a = ((unsigned long)above << 16) | below;
    g = grid_scan_20b50(x >> 16, y >> 16, z >> 16, group);
    low = a & 0xffff;
    if (low >= (g & 0xffff))
        low = g & 0xffff;
    if ((a & 0xffff0000) < (g & 0xffff0000))
        a &= 0xffff0000;
    else
        a = g & 0xffff0000;
    return low | a;
}

unsigned long dispatch_local_2a170(long x, long y, long z, int step)
{
    Actor *q;
    int i;
    Bounds3D b;

    g_eng1_spatial_match_actor_6fedc = 0;
    for (i = 0; i < g_actor_worklist_71b20[0]; ) {
        q = (Actor *)g_actor_worklist_71b20[++i];
        if (actor_bounds_2a880(q, &b, 0) && point_in_box_2a810(x, y, z, &b)) {
            g_eng1_spatial_match_actor_6fedc = q;
            return q->unknown_0d8;
        }
    }
    return grid_query_20d30(x >> 16, y >> 16, z >> 16, step) & 0xffff;
}

int eng1_2a220(Actor *p, Bounds3D *b)
{
    if (!(p->shape.min_x | p->shape.max_x))
        return extent_producer_30e40(p, b, 1);
    b->min_x = p->shape.min_x;
    b->max_x = p->shape.max_x;
    b->min_y = p->shape.min_y;
    b->max_y = p->shape.max_y;
    b->min_z = p->shape.min_z;
    b->max_z = p->shape.max_z;
    return 1;
}

void eng1_native_code_target_002a290(Actor *p)
{
    long r = grid_scan_20b50(p->x >> 16, p->y >> 16, p->z >> 16, p->frame_group);
    p->state_025 |= 1;
    if ((unsigned short)r < 0x140) {
        if ((g_engine_interface.context->level == 2 || g_engine_interface.context->level == 8 ||
             g_engine_interface.context->level == 10) && (p->render_flags & 0x10000000)) {
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

long eng1_2a330(Actor *p, long x, long z)
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

int eng1_2a3d0(Actor *q, Actor *p)
{
    Bounds3D a, b;

    if (!actor_bounds_2a880(p, &a, 0) || !actor_bounds_2a880(q, &b, 0))
        return 0;
    return !((b.max_x < a.min_x) | (a.max_x < b.min_x) |
             (b.max_y < a.min_y) | (a.max_y < b.min_y));
}

int eng1_2a460(Bounds3D *box, Actor *q)
{
    Bounds3D a;

    if (actor_bounds_2a880(q, &a, 0))
        return !((a.max_y < box->min_y) | (box->max_x < a.min_x) |
                 (box->max_y < a.min_y) | (a.max_x < box->min_x));
    return 0;
}

int planar_bounds_overlap(Bounds3D *a, Bounds3D *b)
{
    return !((a->max_y < b->min_y) | (b->max_y < a->min_y) |
             (a->max_x < b->min_x) | (b->max_x < a->min_x));
}

int eng1_2a570(Bounds3D *box, Actor *p)
{
    unsigned short x = (unsigned long)p->x >> 16;
    unsigned short y = (unsigned long)p->y >> 16;
    unsigned short z = (unsigned long)p->z >> 16;
    Bounds3D r;

    if (p->render_flags & 0x10) {
        r.max_x = x - p->shape.min_x;
        r.min_x = x - p->shape.max_x;
    } else {
        r.min_x = x + p->shape.min_x;
        r.max_x = x + p->shape.max_x;
    }
    if (p->render_flags & 0x20) {
        r.max_y = y - p->shape.min_x;
        r.min_y = y - p->shape.max_x;
    } else {
        r.min_y = y + p->shape.min_x;
        r.max_y = y + p->shape.max_x;
    }
    return !(((short)(p->shape.max_z + z) < box->min_z) | (box->max_z < (short)(p->shape.min_z + z)) |
             (r.max_y < box->min_y) | (box->max_y < r.min_y) |
             (box->max_x < r.min_x) | (r.max_x < box->min_x));
}

int eng1_2a660(Actor *q, Actor *p)
{
    Bounds3D a, b;

    if (!actor_bounds_2a880(p, &a, 0) || !actor_bounds_2a880(q, &b, 0))
        return 0;
    return !((b.max_z < a.min_z) | (a.max_z < b.min_z) |
             (b.max_x < a.min_x) | (a.max_x < b.min_x) |
             (b.max_y < a.min_y) | (a.max_y < b.min_y));
}

}
