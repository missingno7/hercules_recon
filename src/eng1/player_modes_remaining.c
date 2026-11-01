/* ENG1 player-mode entries 18a10..19c20, recovered from the original PC code.
 * The setup dispatcher calls these with no physical arguments. Data and helper
 * names are provisional; opaque image arrays are external dependencies, not
 * fabricated payloads. Live pointer reads preserve the original reloads. */
#include <stddef.h>
#include "../shared/engine_interface.h"
#include "setup_shared_state.h"

typedef unsigned char ModeByte;
typedef unsigned short ModeWord;
typedef unsigned long ModeDword;
typedef struct ModeActor ModeActor;
struct ModeActor {
    ModeDword x, y, z;
    ModeByte pad_00c[0x30 - 0x0c];
    short ticks, phase;
    ModeWord sprite;
    ModeByte pad_036[2];
    ModeWord profile;
    ModeByte pad_03a[0x4c - 0x3a];
    ModeDword parameter, flags, render_flags;
    ModeByte pad_058[0x64 - 0x58];
    void *image;
    ModeByte pad_068[0x94 - 0x68];
    ModeByte direction_x, direction_y, direction_z, kind_097;
    ModeByte pad_098[0xa8 - 0x98];
    ModeDword velocity_x, target_x, rate_x, limit_x;
    ModeDword velocity_y, target_y, rate_y, limit_y;
    ModeByte pad_0c8[0xe4 - 0xc8];
    ModeActor *next;
    ModeByte pad_0e8[0x108 - 0xe8];
    ModeDword saved_velocity;
};
typedef struct ModeContext {
    ModeByte pad_000[0x5c];
    /* Native BYTE tests at 5d/5f select the high halves of these WORDs.
     * Matches the original-backed neighboring player-mode context view. */
    volatile ModeWord held;
    ModeWord pressed;
} ModeContext;
typedef char mode_pointer32[(sizeof(void *) == 4) ? 1 : -1];
typedef char mode_actor_image64[(offsetof(ModeActor, image) == 0x64) ? 1 : -1];
typedef char mode_actor_direction94[(offsetof(ModeActor, direction_x) == 0x94) ? 1 : -1];
typedef char mode_actor_next_e4[(offsetof(ModeActor, next) == 0xe4) ? 1 : -1];
typedef char mode_actor_saved108[(offsetof(ModeActor, saved_velocity) == 0x108) ? 1 : -1];
typedef char mode_context_held5c[(offsetof(ModeContext, held) == 0x5c) ? 1 : -1];
typedef char mode_context_pressed5e[(offsetof(ModeContext, pressed) == 0x5e) ? 1 : -1];

extern void *g_normal_actor_pool;
extern unsigned short g_eng1_6ef70;
extern unsigned short g_eng1_word_5cf54;
extern unsigned char g_eng1_image_546b0[], g_eng1_image_54c30[];
extern unsigned char g_eng1_image_54b50[], g_eng1_image_53e80[];
extern unsigned char g_eng1_image_53e58[];
extern unsigned long __cdecl dispatch_local_15d00(void);
extern void __cdecl dispatch_local_16790(void);
extern void __cdecl unlink_0e4(void *);
extern int __cdecl eng1_29c20(void *);
extern int __cdecl eng1_29e20(void *);
extern void __cdecl title_04ea0(int);
extern void __cdecl eng1_17340(void);
extern void __cdecl eng1_176e0(void);
extern int __cdecl eng1_17ff0(void);
extern int __cdecl eng1_17ba0(void);
extern int __cdecl eng1_16fa0(void);
extern int __cdecl eng1_16990(void);
extern int __cdecl eng1_17d00(void);
extern int __cdecl eng1_17a20(void);
extern int __cdecl eng1_16e30(void);
extern void __cdecl eng1_19d70(void);
extern void __cdecl eng1_19aa0(void);
extern void __cdecl eng1_19a10(void);
extern ModeDword __cdecl dispatch_local_30010(long, long, long, unsigned short);
extern ModeDword __cdecl grid_scan_20b50(long, long, long, int);

#define MODE_ACTOR() ((ModeActor *)(*(void * volatile *)(void *)&g_normal_actor_pool))
#define MODE_COLLISION() ((ModeActor *)(*(unsigned char * volatile *)(void *)&g_collision_context))
#define MODE_CONTEXT() ((ModeContext *)(*(ResourceCallbackContext * volatile *)(void *)&g_engine_interface.context_004))
#define MODE_SOURCE() ((ModeActor *)(*(volatile ModeDword *)&g_eng1_setup_dword_72118))

void __cdecl eng1_native_code_target_0018a10(void)
{
    if (!dispatch_local_15d00())
        return;
    eng1_sim_state_72108 = 0x13;
    MODE_ACTOR()->image = g_eng1_image_546b0;
    MODE_ACTOR()->ticks = -1;
    MODE_ACTOR()->phase = -1;
    dispatch_local_15d00();
    g_eng1_6ef70 = 0x13;
    MODE_ACTOR()->direction_y = 0xff;
    MODE_ACTOR()->velocity_y = 0xc0000;
    MODE_ACTOR()->rate_y = 0x4000;
    MODE_ACTOR()->limit_y = 0x10000;
    MODE_ACTOR()->target_y = 0xf0000;
    MODE_ACTOR()->target_x = 0x40000;
    MODE_ACTOR()->velocity_x = 0x40000;
    MODE_ACTOR()->saved_velocity = 0x40000;
    MODE_ACTOR()->rate_x = 0x8000;
    g_eng1_setup_dword_7211c = 0;
    unlink_0e4(MODE_ACTOR());
    {
        ModeActor *actor = MODE_ACTOR();
        actor->saved_velocity = actor->velocity_x;
    }
}

void __cdecl eng1_native_code_target_0018ba0(void)
{
    if (dispatch_local_15d00())
        dispatch_local_16790();
}

void __cdecl eng1_native_code_target_0018c30(void)
{
    if (dispatch_local_15d00()) {
        g_eng1_setup_dword_7201c = 0;
        dispatch_local_16790();
    }
}

void __cdecl eng1_native_code_target_0018ed0(void)
{
    MODE_ACTOR()->parameter = 3;
    if (!eng1_29c20(MODE_ACTOR())) {
        eng1_17340();
        return;
    }
    if (g_eng1_setup_word_6ef84 == 0) {
        if (eng1_17ff0() || eng1_17ba0() || eng1_16fa0() || eng1_16990() ||
            eng1_17d00() || eng1_17a20() || eng1_16e30())
            return;
    }
    if (dispatch_local_15d00()) {
        eng1_sim_state_6feb4 = 0;
        dispatch_local_16790();
    }
}

void __cdecl eng1_native_code_target_0019140(void)
{
    ModeActor *actor = MODE_ACTOR();
    eng1_sim_state_6feb4 = 0;
    eng1_29c20(actor);
    dispatch_local_15d00();
}

void __cdecl eng1_native_code_target_00191e0(void)
{
    ModeActor *actor = MODE_ACTOR();
    if (actor->image == g_eng1_image_54c30) {
        if (actor->phase > 8) {
            title_04ea0(1);
            actor = MODE_ACTOR();
        }
        if (g_eng1_word_5cf54 == 3) {
            eng1_sim_state_72108 = 0x19;
            actor->image = g_eng1_image_54b50;
            MODE_ACTOR()->ticks = -1;
            MODE_ACTOR()->phase = -1;
            dispatch_local_15d00();
            return;
        }
    }
    dispatch_local_15d00();
}

void __cdecl eng1_native_code_target_0019260(void)
{
    ModeActor *source = MODE_SOURCE();
    ModeActor *actor = MODE_ACTOR();
    actor->x = source->x;
    source = MODE_SOURCE();
    actor = MODE_ACTOR();
    actor->y = source->y;
    MODE_ACTOR()->sprite = 0;
}

void __cdecl eng1_native_code_target_0019290(void)
{
    ModeActor *node;
    ModeDword left, right;
    eng1_19d70();
    if (eng1_29e20(MODE_COLLISION())) {
        if (eng1_sim_state_72110) {
            eng1_176e0();
            return;
        }
        node = MODE_COLLISION()->next;
        if (node) {
            while (node->next)
                node = node->next;
            if (node->kind_097 == 8) {
                eng1_sim_state_72108 = 0x12;
                MODE_ACTOR()->image = g_eng1_image_53e80;
                goto selected_image;
            }
        }
        eng1_sim_state_72108 = 0x12;
        MODE_ACTOR()->image = g_eng1_image_53e58;
selected_image:
        MODE_ACTOR()->ticks = -1;
        MODE_ACTOR()->phase = -1;
        dispatch_local_15d00();
        g_eng1_6ef70 = 0;
        MODE_ACTOR()->velocity_x = 0;
        MODE_ACTOR()->saved_velocity = 0;
        MODE_ACTOR()->rate_x = 0;
        return;
    }
    dispatch_local_15d00();
    {
        ModeActor *actor = MODE_ACTOR();
        left = dispatch_local_30010((long)(actor->x - 0x1c0000UL),
            (long)(actor->y - 0x10000UL), (long)actor->z, 1);
    }
    {
        ModeActor *actor = MODE_ACTOR();
        right = dispatch_local_30010((long)(actor->x + 0x1c0000UL),
            (long)(actor->y - 0x10000UL), (long)actor->z, 1);
    }
    if (!left || !right) {
        if (left)
            MODE_ACTOR()->x += 0x20000UL;
        if (right)
            MODE_ACTOR()->x -= 0x20000UL;
    }
}

void __cdecl eng1_native_code_target_00193f0(void)
{
    ModeActor *actor = MODE_ACTOR();
    g_eng1_setup_word_6ef80 = 0x32;
    eng1_29c20(actor);
    if (!dispatch_local_15d00())
        return;
    actor = MODE_ACTOR();
    if (actor->image == g_eng1_image_53e80) {
        eng1_17340();
        g_eng1_6ef70 = 0x12;
    } else {
        dispatch_local_16790();
    }
}

void __cdecl eng1_native_code_target_0019480(void)
{
    ModeContext *context = MODE_CONTEXT();
    ModeActor *actor;
    if ((short)g_eng1_setup_word_72158 < 0) {
        if (context->pressed & 0x300) {
            eng1_17340();
            return;
        }
    } else if ((short)g_eng1_setup_word_72158 > 5) {
        if (!(context->held & 0x300)) {
            actor = MODE_COLLISION();
            if (grid_scan_20b50((long)actor->x >> 16, (long)actor->y >> 16,
                    (long)actor->z >> 16, actor->profile) & 0xffffUL)
                eng1_17340();
            else
                dispatch_local_16790();
            return;
        }
    } else {
        if ((short)g_eng1_setup_word_72158++ < 6) {
            if (!(context->held & 0x300))
                g_eng1_setup_word_72158 = 0xfff8;
        } else {
            g_eng1_setup_word_72158 = 6;
        }
    }
    if (context->held & 0x10) {
        actor = MODE_ACTOR();
        if (actor->direction_y == 0)
            actor->velocity_y = 0;
        eng1_19aa0();
        MODE_ACTOR()->direction_y = 1;
    } else if (context->held & 0x40) {
        actor = MODE_ACTOR();
        if (actor->direction_y != 0)
            actor->velocity_y = 0;
        eng1_19aa0();
        MODE_ACTOR()->direction_y = 0;
    } else {
        MODE_ACTOR()->velocity_y = 0;
    }
    context = MODE_CONTEXT();
    if (context->held & 0x80) {
        actor = MODE_ACTOR();
        if (actor->direction_x == 0)
            actor->velocity_x = 0;
        eng1_19a10();
        MODE_ACTOR()->direction_x = 0xff;
        actor = MODE_COLLISION();
        actor->render_flags = (actor->render_flags & ~0x10UL) | (actor->direction_x & 0x10UL);
    } else if (context->held & 0x20) {
        actor = MODE_ACTOR();
        if ((actor->direction_x & 0x10) == 0x10)
            actor->velocity_x = 0;
        eng1_19a10();
        MODE_ACTOR()->direction_x = 0;
        actor = MODE_COLLISION();
        actor->render_flags = (actor->render_flags & ~0x10UL) | (actor->direction_x & 0x10UL);
    } else {
        MODE_ACTOR()->velocity_x = 0;
    }
    if (MODE_CONTEXT()->held & 0x1000)
        MODE_COLLISION()->z += 0x40000UL;
    if (MODE_CONTEXT()->held & 0x4000)
        MODE_COLLISION()->z -= 0x40000UL;
}

void __cdecl eng1_native_code_target_0019c20(void)
{
    MODE_COLLISION()->sprite = 0;
}

/* 0x19d70..0x19f50: vertical braking, stop transition, and floor clamp. */
extern ModeActor *g_native_pointer_7217c;
extern unsigned char g_eng1_image_53e28[];
extern int __cdecl eng1_29da0(void *);
extern int __cdecl eng1_19970(int);
extern void __cdecl eng1_170c0(void);
extern void __cdecl eng1_19d30(void);

void __cdecl eng1_19d70(void)
{
    ModeActor *actor;

    actor = MODE_ACTOR();
    if (actor->direction_y != 0) {
        eng1_29da0(g_native_pointer_7217c);
        actor = MODE_ACTOR();
        actor->velocity_y -= actor->limit_y;
        actor = MODE_ACTOR();
        if ((long)actor->velocity_y < 0) {
            actor->velocity_y = 0;
            MODE_ACTOR()->direction_y = 0;
            MODE_ACTOR()->target_y = 0xd0000;
            MODE_ACTOR()->rate_y = 0x6000;
            if (eng1_sim_state_72108 == 0x16)
                return;
            if (eng1_sim_state_72108 == 4 &&
                (MODE_CONTEXT()->held & 0x40)) {
                actor = MODE_ACTOR();
                eng1_sim_state_72108 = 0x18;
                actor->image = g_eng1_image_53e28;
                MODE_ACTOR()->ticks = -1;
                MODE_ACTOR()->phase = -1;
                dispatch_local_15d00();
                MODE_ACTOR()->rate_y = 0x10000;
                MODE_ACTOR()->target_y = 0x200000;
                g_native_pointer_7217c->render_flags &= 0xff7fffffUL;
                return;
            }
            eng1_sim_state_72108 = 7;
            return;
        }
        if (eng1_19970(0x10))
            return;
        actor = MODE_ACTOR();
        actor->y -= actor->velocity_y;
        return eng1_19d30();
    }

    actor->velocity_y += actor->rate_y;
    actor = MODE_ACTOR();
    if ((long)actor->velocity_y > (long)actor->target_y) {
        actor->velocity_y = actor->target_y;
        actor = MODE_ACTOR();
    }
    actor->y += actor->velocity_y;

    if ((long)MODE_ACTOR()->y > (long)((unsigned long)*(unsigned short *)
            (g_root_group_rows_70d5c[(int)(signed char)g_active_category_70cfe] + 6) << 22)) {
        if (eng1_sim_state_72108 != 0x16)
            eng1_170c0();
        MODE_ACTOR()->y = (unsigned long)*(unsigned short *)
            (g_root_group_rows_70d5c[(int)(signed char)g_active_category_70cfe] + 6) << 22;
        MODE_ACTOR()->velocity_y = 0x80000;
        return;
    }

    return eng1_19d30();
}

/* Additional native player-mode views and dependencies used only by the
 * recovered entries below. */
typedef struct ModeLinkActor {
    ModeByte pad_000[0x2e];
    ModeWord kind;
    ModeByte pad_030[0x9a - 0x30];
    ModeWord state_09a;
} ModeLinkActor;
typedef struct ModeControlContext {
    ModeByte pad_000[0x5c];
    volatile ModeWord held_05c;
    ModeWord pressed_05e;
    ModeByte pad_060[0x6c - 0x60];
    ModeWord mask_06c;
} ModeControlContext;
typedef char mode_link_kind_2e[(offsetof(ModeLinkActor, kind) == 0x2e) ? 1 : -1];
typedef char mode_link_state_9a[(offsetof(ModeLinkActor, state_09a) == 0x9a) ? 1 : -1];
typedef char mode_control_mask_6c[(offsetof(ModeControlContext, mask_06c) == 0x6c) ? 1 : -1];

extern ModeActor *g_frame_owner_72860;
extern long g_eng1_7210c;
extern short g_pipeline_word_71bc4, g_pipeline_word_71c5c;
extern unsigned long g_frame_global_70c54;
extern unsigned char g_eng1_image_544c0[];
extern unsigned char g_eng1_image_549f0[], g_eng1_image_54aa0[], g_eng1_image_54930[];
extern unsigned char g_eng1_setup_program_54558[];
extern void __cdecl resource_reset(int);
extern void __cdecl dispatch_engine_2c4d0(void);
extern unsigned int __cdecl random_limit(int);
extern long __cdecl eng1_2a010(long, long, long, int);

int __cdecl eng1_18b10(void)
{
    int kind;

    if (g_eng1_7210c == 0)
        return 0;
    kind = ((ModeLinkActor *)(void *)g_eng1_7210c)->kind;
    if (kind != 2 && (kind <= 3 || kind > 5))
        return 0;

    ((ModeLinkActor *)(void *)g_eng1_7210c)->state_09a = 1;
    kind = ((ModeLinkActor *)(void *)g_eng1_7210c)->kind;
    g_eng1_setup_word_6ef98 = 0;
    g_eng1_setup_word_6ef9c = 0;
    g_eng1_setup_dword_7201c = kind;
    eng1_sim_state_72108 = 0x14;
    g_frame_owner_72860->image = g_eng1_image_544c0;
    g_frame_owner_72860->ticks = -1;
    g_frame_owner_72860->phase = -1;
    dispatch_local_15d00();
    return 1;
}

int __cdecl eng1_18bb0(void)
{
    ModeControlContext *context;
    ModeActor *owner;

    context = (ModeControlContext *)(void *)MODE_CONTEXT();
    if (context->held_05c & context->mask_06c) {
        if (context->held_05c & 0x10) {
            if (g_eng1_setup_dword_7201c == 4)
                return 0;
            owner = g_frame_owner_72860;
            eng1_sim_state_72108 = 0x2f;
            owner->image = g_eng1_setup_program_54558 + 0x90;
        } else {
            owner = g_frame_owner_72860;
            eng1_sim_state_72108 = 0x2f;
            owner->image = g_eng1_setup_program_54558;
        }
        g_frame_owner_72860->ticks = -1;
        g_frame_owner_72860->phase = -1;
        dispatch_local_15d00();
        return 1;
    }
    return 0;
}

void __cdecl eng1_18f60(void)
{
    ModeByte *context;
    ModeByte counter;
    long step;
    int negative;
    int positive;
    unsigned long hit;

    context = (ModeByte *)(void *)MODE_CONTEXT();
    counter = context[0x0d];
    counter++;
    context[0x0d] = counter;
    context = (ModeByte *)(void *)MODE_CONTEXT();
    counter = context[0x0c];
    if (counter != 0) {
        counter--;
        context[0x0c] = counter;
        context = (ModeByte *)(void *)MODE_CONTEXT();
    }
    if (context[0x30] & 8) {
        counter = context[0x0c];
        counter++;
        context[0x0c] = counter;
        context = (ModeByte *)(void *)MODE_CONTEXT();
    }
    counter = context[0x0c];
    if (counter == 0) {
        context[3] = 1;
        context = (ModeByte *)(void *)MODE_CONTEXT();
    }
    g_frame_global_70c54 = 1;
    context[5] = 1;
    context = (ModeByte *)(void *)MODE_CONTEXT();
    context[1] = context[2];
    dispatch_engine_2c4d0();
    eng1_29c20(g_frame_owner_72860);

    positive = 0;
    negative = 0;
    for (step = 0x100000L; step < 0x410000L; step += 0x100000L) {
        if (dispatch_local_30010(g_frame_owner_72860->x + step,
                                 g_frame_owner_72860->y - 0x20000UL,
                                 g_frame_owner_72860->z, 1))
            positive++;
        hit = (unsigned short)eng1_2a010(g_frame_owner_72860->x + step,
                                         g_frame_owner_72860->y,
                                         g_frame_owner_72860->z, 0);
        if (hit != 0)
            positive++;
        if (dispatch_local_30010(g_frame_owner_72860->x - step,
                                 g_frame_owner_72860->y - 0x20000UL,
                                 g_frame_owner_72860->z, 1))
            negative++;
        hit = (unsigned short)eng1_2a010(g_frame_owner_72860->x - step,
                                         g_frame_owner_72860->y,
                                         g_frame_owner_72860->z, 0);
        if (hit != 0)
            negative++;
    }

    eng1_sim_state_72108 = 0x16;
    g_frame_owner_72860->ticks = -1;
    g_frame_owner_72860->phase = -1;
    if (negative != 0 && positive != 0) {
        g_frame_owner_72860->image = g_eng1_image_549f0;
        dispatch_local_15d00();
        return;
    }
    positive += negative;
    if (positive == 0 && random_limit(100) < 0x32)
        negative = 1;

    if (negative != 0) {
        if (g_frame_owner_72860->direction_x != 0) {
            g_frame_owner_72860->image = g_eng1_image_54930;
            dispatch_local_15d00();
            return;
        }
        g_frame_owner_72860->image = g_eng1_image_54aa0;
        dispatch_local_15d00();
        return;
    }
    if (g_frame_owner_72860->direction_x != 0) {
        g_frame_owner_72860->image = g_eng1_image_54aa0;
        dispatch_local_15d00();
        return;
    }
    g_frame_owner_72860->image = g_eng1_image_54930;
    dispatch_local_15d00();
}

void __cdecl eng1_19160(void)
{
    ModeByte *context;

    if (g_eng1_word_5cf54 != 3) {
        resource_reset(5);
        {
            ModeActor *owner;
            owner = g_frame_owner_72860;
            eng1_sim_state_72108 = 0x19;
            owner->image = g_eng1_image_54c30;
        }
    } else {
        {
            ModeActor *owner;
            owner = g_frame_owner_72860;
            eng1_sim_state_72108 = 0x19;
            owner->image = g_eng1_image_54b50;
        }
    }
    g_frame_owner_72860->ticks = -1;
    g_frame_owner_72860->phase = -1;
    dispatch_local_15d00();
    context = (ModeByte *)(void *)MODE_CONTEXT();
    g_frame_global_70c54 = 1;
    context[1] = context[2];
}

void __cdecl eng1_19240(void)
{
    if (eng1_sim_state_72170 == 0)
        eng1_sim_state_72170 = 1;
}

int __cdecl eng1_19cf0(void)
{
    ModeActor *actor;
    long z;

    actor = g_frame_owner_72860;
    z = (long)actor->z >> 16;
    if (z < g_pipeline_word_71c5c) {
        actor->z = (unsigned long)((long)g_pipeline_word_71c5c << 16);
        return 1;
    }
    if (z > g_pipeline_word_71bc4) {
        actor->z = (unsigned long)((long)g_pipeline_word_71bc4 << 16);
        return 1;
    }
    return 0;
}

/* Native ENG1 0x19650 probes three positions on the selected horizontal side.
 * The direction offset is captured once; actor coordinates are reread from
 * the live normal-actor pointer between the spatial queries. */
int __cdecl eng1_19650(void)
{
    ModeActor *actor;
    long horizontal_offset;

    actor = MODE_ACTOR();
    horizontal_offset = actor->direction_x
        ? -0x1c0000L : 0x1c0000L;
    if (dispatch_local_30010(
            (long)(actor->x + horizontal_offset),
            (long)(actor->y - 0x200000UL), (long)actor->z, 1))
        return 0;

    actor = MODE_ACTOR();
    if (dispatch_local_30010(
            (long)(actor->x + horizontal_offset),
            (long)(actor->y - 0x100000UL), (long)actor->z, 1))
        return 0;

    actor = MODE_ACTOR();
    return dispatch_local_30010((long)(actor->x + horizontal_offset),
                                (long)actor->y, (long)actor->z, 1) == 0;
}

extern volatile unsigned long g_eng1_spatial_match_actor_6fedc;
extern long g_eng1_7210c;

int __cdecl eng1_197b0(void)
{
    if (dispatch_local_30010(
            (long)(((ModeActor *)g_normal_actor_pool)->x -
                (g_eng1_setup_dword_7201c
                    ? 0x380000L : 0x1c0000L)),
            (long)(((ModeActor *)g_normal_actor_pool)->y - 0x10000UL),
            (long)g_native_pointer_7217c->z, 1)) {
        if (g_eng1_setup_dword_7201c == 0) {
            ModeDword match = g_eng1_spatial_match_actor_6fedc;
            if (match != 0)
                g_eng1_7210c = (long)match;
        }
        return 0;
    }
    return 1;
}

int __cdecl eng1_19820(void)
{
    if (dispatch_local_30010(
            (long)(((ModeActor *)g_normal_actor_pool)->x -
                (g_eng1_setup_dword_7201c
                    ? 0x400000L : 0x240000L)),
            (long)(((ModeActor *)g_normal_actor_pool)->y - 0x10000UL),
            (long)g_native_pointer_7217c->z, 1)) {
        if (g_eng1_setup_dword_7201c == 0) {
            ModeDword match = g_eng1_spatial_match_actor_6fedc;
            if (match != 0)
                g_eng1_7210c = (long)match;
        }
        return 0;
    }
    return 1;
}

int __cdecl eng1_19890(void)
{
    long horizontal_offset;

    horizontal_offset = g_eng1_setup_dword_7201c
        ? 0x380000L : 0x1c0000L;
    horizontal_offset += ((ModeActor *)g_normal_actor_pool)->x;
    if (dispatch_local_30010(
            horizontal_offset,
            (long)(((ModeActor *)g_normal_actor_pool)->y - 0x80000UL),
            (long)g_native_pointer_7217c->z, 1)) {
        if (g_eng1_setup_dword_7201c == 0) {
            ModeDword match = g_eng1_spatial_match_actor_6fedc;
            if (match != 0)
                g_eng1_7210c = (long)match;
        }
        return 0;
    }
    return 1;
}

int __cdecl eng1_19900(void)
{
    long horizontal_offset;

    horizontal_offset = g_eng1_setup_dword_7201c
        ? 0x400000L : 0x240000L;
    horizontal_offset += ((ModeActor *)g_normal_actor_pool)->x;
    if (dispatch_local_30010(
            horizontal_offset,
            (long)(((ModeActor *)g_normal_actor_pool)->y - 0x80000UL),
            (long)g_native_pointer_7217c->z, 1)) {
        if (g_eng1_setup_dword_7201c == 0) {
            ModeDword match = g_eng1_spatial_match_actor_6fedc;
            if (match != 0)
                g_eng1_7210c = (long)match;
        }
        return 0;
    }
    return 1;
}

typedef struct PlayerModeBounds {
    short min_x, min_y, min_z;
    short max_x, max_y, max_z;
} PlayerModeBounds;

extern long g_actor_worklist_71a20[];
extern int __cdecl actor_bounds_2a880(
    void *, PlayerModeBounds *, unsigned long);
extern int __cdecl point_in_box_2a810(
    long, long, long, PlayerModeBounds *);

int __cdecl eng1_19970(int vertical_offset)
{
    PlayerModeBounds bounds;
    ModeActor *actor;
    long index;
    ModeByte level;

    level = ((ModeByte *)(void *)MODE_CONTEXT())[0];
    if (level != 1 && level != 5 && level != 3)
        return 0;
    if (!actor_bounds_2a880(g_native_pointer_7217c, &bounds, 1UL))
        return 0;
    index = 0;
    if (g_actor_worklist_71a20[0] <= 0)
        return 0;

    while (index < g_actor_worklist_71a20[0]) {
        actor = (ModeActor *)(unsigned long)
            g_actor_worklist_71a20[index + 1];
        if (point_in_box_2a810((long)actor->x,
                (long)(actor->y +
                    ((ModeDword)(unsigned long)vertical_offset << 16)),
                (long)actor->z, &bounds))
            return index + 1;
        ++index;
    }
    return 0;
}
