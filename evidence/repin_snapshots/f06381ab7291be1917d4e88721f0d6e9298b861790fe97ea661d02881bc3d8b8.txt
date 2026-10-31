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
