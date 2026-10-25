/* ENG1 RVA 2cd30. Ordinary-C timer/selector update and actor callbacks.
 * Plausible and instruction-audited; selected source spies only. Unknown
 * 2d3f0/2d440 endpoints remain real unresolved source dependencies. */
#include <stddef.h>
#include "actor_control_update.h"
#include "incomplete.h"
#include "../shared/engine_interface.h"
#include "actor_pipeline_storage.h"
#include "actor_control_state.h"
#include "actor_control_cells.h"
#include "initial_read_guards.h"

typedef unsigned char call_u8;
typedef unsigned short call_u16;
typedef unsigned long call_u32;

typedef int (__cdecl *Eng1Call2cd30HostDispatch)(call_u32);

/* One contiguous 80-byte view preserves the measured physical offsets.
 * Runtime domains are narrower: class 0 reads base 0x5c3b8 only for selectors
 * 1..5, while selector 12 maps to class 2 and reads from base 0x5c3d0.
 * Indices 0..39 correspond to WORDs at [0x5c3b8,0x5c408). */


/* Inline jump table 0x2d1d4 and byte selector map 0x2d1e4, interpreted as
 * four native handler destinations. */
static const call_u8 eng1_call_2cd30_selector_handlers[12] = {
    0U, 0U, 0U, 0U, 0U, 3U, 3U, 1U, 3U, 3U, 3U, 2U
};

/* Native 0x2c4c0 is a one-DWORD cdecl thunk to EngineInterface+0xb0,
 * opaque_090[8]. Keep the actual host owner and live function-pointer read. */
int __cdecl eng1_native_code_target_002c4c0(call_u32 value)
{
    return ((Eng1Call2cd30HostDispatch)g_engine_interface.opaque_090[8])(value);
}


/* These are unresolved native source/data dependencies. They intentionally
 * have no local default implementation. */
extern int __cdecl eng1_native_dep_002d3f0(Actor *actor,
                                          call_u32 selector);
extern void __cdecl eng1_native_dep_002d440(Actor *actor);
extern call_u32 eng1_sim_state_72110;
extern void __cdecl eng1_native_code_target_002a290(Actor *actor);
extern call_u32 __cdecl eng1_interpret_signed_pairs(Actor *actor);

static volatile call_u32 *eng1_call_2cd30_collision_word(void)
{
    unsigned char *context;

    context = *(unsigned char * volatile *)(void *)&g_collision_context;
    if (context == NULL)
        eng1_incomplete_behavior(0x7217cUL,
            "2cd30 collision pointer has no initialized DWORD pointee");
    return (volatile call_u32 *)(void *)context;
}

static volatile call_u16 *eng1_call_2cd30_word(
    volatile Eng1ActorControlState70ed8 *state, unsigned int byte_offset)
{
    return (volatile call_u16 *)(void *)&state->bytes[byte_offset];
}

static volatile call_u32 *eng1_call_2cd30_actor_dword(
    Actor *actor, unsigned int byte_offset)
{
    return (volatile call_u32 *)(void *)((volatile call_u8 *)actor +
                                         byte_offset);
}

static volatile long *eng1_call_2cd30_actor_long(
    Actor *actor, unsigned int byte_offset)
{
    return (volatile long *)(void *)((volatile call_u8 *)actor +
                                     byte_offset);
}

static volatile call_u16 *eng1_call_2cd30_actor_word(
    Actor *actor, unsigned int byte_offset)
{
    return (volatile call_u16 *)(void *)((volatile call_u8 *)actor +
                                         byte_offset);
}

static volatile call_u8 *eng1_call_2cd30_actor_byte(
    Actor *actor, unsigned int byte_offset)
{
    return (volatile call_u8 *)actor + byte_offset;
}

void __cdecl eng1_native_code_target_02cd30(Actor *actor)
{
    volatile Eng1ActorControlState70ed8 *state;
    volatile call_u16 *pipeline_word;
    call_u16 timer_f0e;
    call_u16 timer_f04;
    call_u16 timer_ef0;
    call_u16 delay_ee0;
    call_u16 parameter_f02;
    call_u16 parameter_ee8;
    call_u16 selector;
    call_u16 actor_state;
    call_u8 selector_class;
    call_u8 classified;
    long actor_x;
    unsigned int table_index;

    state = &g_eng1_actor_control_state_70ed8;
    pipeline_word = (volatile call_u16 *)(void *)
        &g_actor_pipeline_storage_71440.word_cells[6];

    timer_f0e = *eng1_call_2cd30_word(state, 0x36U);
    if (timer_f0e != 0U) {
        --*eng1_call_2cd30_word(state, 0x0cU);
        if (*eng1_call_2cd30_word(state, 0x0cU) == 0U) {
            --timer_f0e;
            *eng1_call_2cd30_word(state, 0x0cU) = 0x32U;
            *eng1_call_2cd30_word(state, 0x36U) = timer_f0e;
        }
    }

    timer_f04 = *eng1_call_2cd30_word(state, 0x2cU);
    if (timer_f04 != 0U) {
        --*eng1_call_2cd30_word(state, 0x00U);
        if (*eng1_call_2cd30_word(state, 0x00U) == 0U) {
            --timer_f04;
            *eng1_call_2cd30_word(state, 0x00U) = 0x32U;
            *eng1_call_2cd30_word(state, 0x2cU) = timer_f04;
        }
    }

    timer_ef0 = *eng1_call_2cd30_word(state, 0x18U);
    if (timer_ef0 != 0U) {
        --*eng1_call_2cd30_word(state, 0x02U);
        if (*eng1_call_2cd30_word(state, 0x02U) == 0U) {
            --timer_ef0;
            *eng1_call_2cd30_word(state, 0x02U) = 0x0cU;
            *eng1_call_2cd30_word(state, 0x18U) = timer_ef0;
        }
    }

    actor_state = *eng1_call_2cd30_actor_word(actor, 0x2aU);
    if (actor_state != 0x0bU && actor_state != 0U) {
        *pipeline_word = 0U;
    } else if ((timer_f04 | timer_ef0 | timer_f0e) == 0U) {
        ++*pipeline_word;
    }

    if (*eng1_call_2cd30_word(state, 0x04U) != 0U)
        --*eng1_call_2cd30_word(state, 0x04U);
    if (*eng1_call_2cd30_word(state, 0x0aU) != 0U)
        --*eng1_call_2cd30_word(state, 0x0aU);

    delay_ee0 = *eng1_call_2cd30_word(state, 0x08U);
    if (delay_ee0 != 0U) {
        --delay_ee0;
        *eng1_call_2cd30_word(state, 0x08U) = delay_ee0;
    }

    if (*eng1_call_2cd30_word(state, 0x30U) != 0U) {
        --*eng1_call_2cd30_word(state, 0x30U);
        goto actor_callback;
    }
    if (eng1_sim_state_72110 != 0UL)
        goto actor_callback;

    parameter_f02 = *eng1_call_2cd30_word(state, 0x2aU) & 3U;
    parameter_ee8 = *eng1_call_2cd30_word(state, 0x10U) & 3U;
    *eng1_call_2cd30_word(state, 0x2aU) = parameter_f02;
    *eng1_call_2cd30_word(state, 0x10U) = parameter_ee8;

    selector = *eng1_call_2cd30_word(state, 0x34U);
    if (selector >= 1U && selector <= 12U) {
        selector_class = eng1_call_2cd30_selector_handlers[selector - 1U];

        if (selector_class == 0U) {
            if (*eng1_call_2cd30_word(state, 0x36U) >
                g_eng1_control_lookup_words_5c3b8[selector]) {
                eng1_native_code_target_002c4c0(
                    g_eng1_control_lookup_words_5c3b8[12U + selector]);
                *eng1_call_2cd30_word(state, 0x36U) = 0U;
                *eng1_call_2cd30_word(state, 0x30U) = 0x00faU;
                goto actor_callback;
            }
            if (*eng1_call_2cd30_word(state, 0x2cU) > 1U) {
                if (*eng1_call_2cd30_word(state, 0x04U) != 0U)
                    goto actor_callback;
                eng1_native_code_target_002c4c0(0x1aU);
                *eng1_call_2cd30_word(state, 0x04U) = 0x0320U;
                *eng1_call_2cd30_word(state, 0x30U) = 0x0064U;
                *eng1_call_2cd30_word(state, 0x2cU) = 0U;
                goto actor_callback;
            }
            if (*eng1_call_2cd30_word(state, 0x18U) > 0x14U) {
                if (*eng1_call_2cd30_word(state, 0x0aU) != 0U)
                    goto actor_callback;
                table_index = (unsigned int)parameter_f02;
                eng1_native_code_target_002c4c0(
                    g_eng1_control_lookup_words_5c3b8[28U + table_index]);
                ++*eng1_call_2cd30_word(state, 0x2aU);
                *eng1_call_2cd30_word(state, 0x0aU) = 0x00f0U;
                *eng1_call_2cd30_word(state, 0x30U) = 0x00a0U;
                *eng1_call_2cd30_word(state, 0x18U) = 0U;
                goto actor_callback;
            }
            if (*pipeline_word <= 0x00faU)
                goto actor_callback;
            goto pipeline_transition;
        }

        if (selector_class == 1U) {
            if (*eng1_call_2cd30_word(state, 0x18U) > 0x0aU) {
                if (*eng1_call_2cd30_word(state, 0x0aU) != 0U)
                    goto actor_callback;
                table_index = (unsigned int)parameter_f02;
                eng1_native_code_target_002c4c0(
                    g_eng1_control_lookup_words_5c3b8[28U + table_index]);
                ++*eng1_call_2cd30_word(state, 0x2aU);
                *eng1_call_2cd30_word(state, 0x0aU) = 0x00f0U;
                *eng1_call_2cd30_word(state, 0x30U) = 0x00a0U;
                *eng1_call_2cd30_word(state, 0x18U) = 0U;
                goto actor_callback;
            }
            if (*pipeline_word <= 0x00faU)
                goto actor_callback;
            goto pipeline_transition;
        }

        if (selector_class == 2U) {
            if (*eng1_call_2cd30_word(state, 0x36U) > 6U) {
                eng1_native_code_target_002c4c0(
                    g_eng1_control_lookup_words_5c3b8[12U + selector]);
                *eng1_call_2cd30_word(state, 0x30U) = 0x00faU;
                *eng1_call_2cd30_word(state, 0x36U) = 0U;
                goto actor_callback;
            }
            if (*eng1_call_2cd30_word(state, 0x18U) > 0x0aU) {
                if (*eng1_call_2cd30_word(state, 0x0aU) != 0U)
                    goto actor_callback;
                table_index = (unsigned int)parameter_f02;
                eng1_native_code_target_002c4c0(
                    g_eng1_control_lookup_words_5c3b8[28U + table_index]);
                ++*eng1_call_2cd30_word(state, 0x2aU);
                *eng1_call_2cd30_word(state, 0x0aU) = 0x00f0U;
                *eng1_call_2cd30_word(state, 0x30U) = 0x00a0U;
                *eng1_call_2cd30_word(state, 0x18U) = 0U;
                goto actor_callback;
            }
            if (*pipeline_word <= 0x00faU)
                goto actor_callback;
            goto selector12_pipeline_transition;
        }
        /* Selector classes 3 are native no-op handler paths. */
    }
    goto actor_callback;

selector12_pipeline_transition:
    if (delay_ee0 != 0U)
        goto actor_callback;
    table_index = 36U + (unsigned int)parameter_ee8;
    eng1_native_code_target_002c4c0(
        g_eng1_control_lookup_words_5c3b8[table_index]);
    ++*eng1_call_2cd30_word(state, 0x10U);
    *eng1_call_2cd30_word(state, 0x08U) = 0x015eU;
    *eng1_call_2cd30_word(state, 0x30U) = 0x00a0U;
    *pipeline_word = 0U;
    goto actor_callback;

pipeline_transition:
    if (delay_ee0 != 0U)
        goto actor_callback;
    if (g_eng1_setup_dword_6efa4 != 0UL) {
        table_index = 36U + (unsigned int)parameter_ee8;
    } else {
        table_index = 32U + (unsigned int)parameter_ee8;
    }
    eng1_native_code_target_002c4c0(
        g_eng1_control_lookup_words_5c3b8[table_index]);
    ++*eng1_call_2cd30_word(state, 0x10U);
    *eng1_call_2cd30_word(state, 0x08U) = 0x015eU;
    *eng1_call_2cd30_word(state, 0x30U) = 0x00a0U;
    *pipeline_word = 0U;

actor_callback:
    if ((*eng1_call_2cd30_actor_byte(actor, 0x25U) & 1U) != 0U)
        eng1_native_code_target_002a290(actor);

    classified = eng1_native_code_target_0041fd0(
        *eng1_call_2cd30_collision_word());
    *eng1_call_2cd30_word(state, 0x34U) = (call_u16)classified;

    actor_x = *eng1_call_2cd30_actor_long(actor, 0x00U);
    classified = eng1_native_code_target_0041fd0((call_u32)actor_x);
    *eng1_call_2cd30_word(state, 0x24U) = 0U;
    *eng1_call_2cd30_word(state, 0x1aU) = (call_u16)classified;

    actor_x = *eng1_call_2cd30_actor_long(actor, 0x00U);
    if (actor_x < 0x04f80000L) {
        actor_x = 0x04f80000L;
        *eng1_call_2cd30_actor_long(actor, 0x00U) = actor_x;
    }

    selector = *eng1_call_2cd30_word(state, 0x34U);
    if (selector != *eng1_call_2cd30_word(state, 0x12U) &&
        selector != *eng1_call_2cd30_word(state, 0x1aU))
        eng1_native_dep_002d3f0(actor, (call_u32)selector);
    eng1_native_dep_002d440(actor);

    if (*eng1_call_2cd30_actor_word(actor, 0x2aU) == 0x0aU) {
        *eng1_call_2cd30_actor_dword(actor, 0x54U) &= 0x7fffffffUL;
        goto callback_end;
    }

    *eng1_call_2cd30_actor_dword(actor, 0x54U) |= 0x80000000UL;
    (void)eng1_interpret_signed_pairs(actor);
    eng1_guarded_apply_frame_motion_2d((void *)actor);
    *eng1_call_2cd30_actor_dword(actor, 0x54U) =
        (*eng1_call_2cd30_actor_dword(actor, 0x54U) & 0xffffffefUL) |
        (*eng1_call_2cd30_actor_byte(actor, 0x94U) & 0x10U);
    *eng1_call_2cd30_word(state, 0x0eU) =
        *eng1_call_2cd30_actor_word(actor, 0x4cU);

    if (*eng1_call_2cd30_actor_word(actor, 0x2aU) == 7U) {
        *eng1_call_2cd30_word(state, 0x24U) = 1U;
        *eng1_call_2cd30_word(state, 0x1cU) =
            *eng1_call_2cd30_actor_word(actor, 0x36U);
    } else if (*eng1_call_2cd30_word(state, 0x26U) == 2U) {
        *eng1_call_2cd30_word(state, 0x24U) = 2U;
        *eng1_call_2cd30_word(state, 0x0eU) = 0x10U;
        *eng1_call_2cd30_word(state, 0x1cU) = 0x66U;
    }

    selector = *eng1_call_2cd30_actor_word(actor, 0x2aU);
    if (selector == 2U || selector == 5U) {
        *eng1_call_2cd30_word(state, 0x28U) =
            *eng1_call_2cd30_actor_word(actor, 0x36U);
    } else {
        *eng1_call_2cd30_word(state, 0x28U) = 0U;
    }

callback_end:
    *eng1_call_2cd30_word(state, 0x26U) = 0U;
    *eng1_call_2cd30_word(state, 0x2eU) = 0U;
    *eng1_call_2cd30_word(state, 0x06U) = 0U;
}
