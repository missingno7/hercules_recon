/* Ordinary-C behavioral candidate for ENG1.DLL RVA 0x2a290.
 * The target is a one-actor update leaf.  This is a bounded field view, not
 * an original aggregate or translation-unit claim.  The real 0x20b50 query
 * remains a separate external dependency. */
#include <stddef.h>
#include "../../../calibration/engine_interface.h"

typedef unsigned char actor_u8;
typedef unsigned short actor_u16;
typedef unsigned long actor_u32;

typedef struct ActorUpdateView ActorUpdateView;
struct ActorUpdateView {
    actor_u32 x_000;
    actor_u32 y_004;
    actor_u32 z_008;
    actor_u8 unknown_00c[0x25 - 0x0c];
    actor_u8 update_state_025;
    actor_u8 unknown_026[0x38 - 0x26];
    actor_u16 query_parameter_038;
    actor_u8 unknown_03a[0x54 - 0x3a];
    actor_u32 flags_054;
};

typedef char actor_update_view_offsets_are_observed[
    (offsetof(ActorUpdateView, x_000) == 0x00 &&
     offsetof(ActorUpdateView, y_004) == 0x04 &&
     offsetof(ActorUpdateView, z_008) == 0x08 &&
     offsetof(ActorUpdateView, update_state_025) == 0x25 &&
     offsetof(ActorUpdateView, query_parameter_038) == 0x38 &&
     offsetof(ActorUpdateView, flags_054) == 0x54) ? 1 : -1];
typedef char actor_update_ulong_is_32_bits[(sizeof(actor_u32) == 4) ? 1 : -1];
typedef char actor_update_pointer_is_32_bits[(sizeof(void *) == 4) ? 1 : -1];

/* This helper is an unresolved native dependency in production source.  The
 * bounded fixture supplies a test-only call-through spy for it. */
extern long __cdecl eng1_native_dep_020b50(long x, long y, long z,
                                          int parameter);

static long actor_update_signed_high_word(actor_u32 value)
{
    return (long)(short)(actor_u16)(value >> 16);
}

void __cdecl eng1_native_code_target_002a290(ActorUpdateView *actor)
{
    actor_u32 query_result;
    actor_u8 context_mode;

    query_result = (actor_u32)eng1_native_dep_020b50(
        actor_update_signed_high_word(actor->x_000),
        actor_update_signed_high_word(actor->y_004),
        actor_update_signed_high_word(actor->z_008),
        (int)actor->query_parameter_038);

    actor->update_state_025 |= 1U;

    /* Native compares AX, then uses all of EAX in the selected arithmetic. */
    if ((actor_u16)query_result >= 0x0140U) {
        if ((query_result & 0xffff0000UL) >= 0x01000000UL)
            return;

        actor->y_004 -= query_result & 0xffff0000UL;
        actor->update_state_025 &= 0xfeU;
        return;
    }

    context_mode = *(actor_u8 *)(void *)g_engine_interface.context_004;
    if ((context_mode == 2U || context_mode == 8U || context_mode == 10U) &&
        (actor->flags_054 & 0x10000000UL) != 0) {
        actor->y_004 += (query_result - 0x20UL) << 16;
        return;
    }

    actor->y_004 += query_result << 16;
    actor->update_state_025 &= 0xfeU;
}
