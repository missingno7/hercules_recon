/* Ordinary-C behavioral reconstruction of the bounded ENG1 0x12c00 body and
 * its complete local helper at 0x12d10. It deliberately stops at measured
 * owners and uses explicit fail-fast dependencies outside those limits. */
#include <stddef.h>
#include "interpreters.h"
#include "resource_runtime_data.h"
#include "simulation_data_owners.h"
#include "action_spawn_27d40.h"
#include "../shared/engine_interface.h"
#include "incomplete.h"

extern void __cdecl reset_frame_state(Actor *actor);
extern Actor *__cdecl action_spawn_27d40(Actor *actor, unsigned long action);
extern int __cdecl release_actor(Actor *actor);
extern void __cdecl eng1_native_code_target_002a290(Actor *actor);
/* Native 0x12d10 pushes the actor pointer to this unresolved callee. */
extern void __cdecl eng1_native_code_target_000ef10(Actor *actor);
extern unsigned short g_eng1_config_word_pool_533c8[282];

void __cdecl eng1_native_code_target_012d10(Actor *actor)
{
    volatile unsigned char *context =
        (volatile unsigned char *)g_engine_interface.context_004;
    volatile unsigned short *kind =
        (volatile unsigned short *)((volatile unsigned char *)actor + 0x2e);

    if (context[0] == 9U)
        return;

    switch (*kind) {
    case 0x2002U:
    case 0x2012U:
    case 0x2013U:
    case 0x201aU:
    case 0x201bU:
    case 0x201cU:
    case 0x201dU:
        eng1_native_code_target_002a290(actor);
        break;
    default:
        break;
    }

    /* Native 0x12d3f calls 0x2a290 and falls through to 0x12d48. */
    eng1_native_code_target_000ef10(actor);
}

static void __cdecl eng1_candidate_unsupported_prefix(const char *reason)
{
    eng1_incomplete_behavior(0x12c00UL, reason);
}

void __cdecl eng1_native_code_target_012c00(Actor *actor)
{
    volatile unsigned char *bytes = (volatile unsigned char *)actor;
    volatile unsigned short *kind =
        (volatile unsigned short *)(bytes + 0x2e);
    volatile unsigned short *control =
        (volatile unsigned short *)(bytes + 0x4a);
    volatile unsigned long *render_flags =
        (volatile unsigned long *)(bytes + 0x54);
    unsigned long x, y, sum, offset, owner_address, data_address;
    unsigned long lookup_value;
    unsigned long saved_render_flags;
    unsigned short snapshot_kind, group;
    unsigned char *owner;
    Actor *child;

    reset_frame_state(actor);
    y = *(volatile unsigned long *)(bytes + 0x04);
    x = *(volatile unsigned long *)(bytes + 0x00);
    sum = x + y;
    *(volatile unsigned short *)(bytes + 0x32) =
        (unsigned short)((sum >> 17) & 7UL);

    if (*kind == 0x2012U &&
        ((volatile unsigned char *)g_engine_interface.context_004)[0x0c] == 9U) {
        *kind = 0x2013U;
        *(void * volatile *)(void *)(bytes + 0x64) =
            (void *)&g_eng1_config_word_pool_533c8[196];
    }
    if (*kind == 0x2002U &&
        *(volatile unsigned short *)((volatile unsigned char *)
            g_engine_interface.context_004 + 0x1e) == 0x00ffU) {
        *kind = 0x2013U;
        *(void * volatile *)(void *)(bytes + 0x64) =
            (void *)&g_eng1_config_word_pool_533c8[196];
    }

    /* Native keeps this WORD in DI across action_spawn_27d40. */
    snapshot_kind = *kind;
    if (snapshot_kind == 0x2015U || snapshot_kind == 0x2016U ||
        snapshot_kind == 0x2017U) {
        child = action_spawn_27d40(actor, 0xa007UL);
        if (child != NULL) {
            *(volatile unsigned short *)((volatile unsigned char *)child + 0x4a)
                = 0x0080U;
            *(volatile unsigned short *)((volatile unsigned char *)child + 0x34)
                = g_eng1_config_word_pool_533c8[134];
        }
        saved_render_flags = *render_flags;
        *control = 0x00a0U;
        *render_flags = saved_render_flags | 0x04000605UL;
    }

    if (snapshot_kind == 0x200bU) {
        offset = *(volatile unsigned long *)(bytes + 0x58);
        if (offset == 0UL) {
            (void)release_actor(actor);
            return;
        }

        group = *(volatile unsigned short *)(bytes + 0x38);
        if (group >= 1U)
            eng1_candidate_unsupported_prefix(
                "0x12c00 frame-group index exceeds measured 0x6fee8 row prefix");
        owner = (unsigned char *)g_kind_info[group].owner;
        if (owner == NULL)
            eng1_candidate_unsupported_prefix(
                "0x12c00 kind-info owner pointer is null");

        owner_address = (unsigned long)owner;
        data_address = (unsigned long)g_eng1_simulation_data_bytes_53620;
        if (owner_address < data_address ||
            owner_address - data_address > 0x1c0UL - 0x0eUL ||
            offset > 0x1c0UL - (owner_address - data_address) - 0x0eUL)
            eng1_candidate_unsupported_prefix(
                "0x12c00 relative word read exceeds measured 0x53620 byte owner");

        *(volatile unsigned short *)(bytes + 0x2a) =
            *(volatile unsigned short *)(owner + offset + 0x0c) & 0x003fU;
        lookup_value = ENG1_NATIVE_DATA_TARGET_00537E0_DWORD(
            *(volatile unsigned short *)(bytes + 0x2a));
        *(volatile unsigned short *)(bytes + 0x32) = 0U;
        *(void * volatile *)(void *)(bytes + 0x64) = (void *)lookup_value;
        eng1_native_code_target_012d10(actor);
        return;
    }

    eng1_native_code_target_012d10(actor);
}
