/* Ordinary-C behavioral reconstruction of ENG1 entries 0x2d3f0 and
 * 0x2d440.  The six-byte record view and touched Actor offsets are measured
 * from the native instructions.  This file does not claim an original C
 * structure or function name, and it contains no original-binary dispatch. */
#include <stddef.h>
#include <stdio.h>
#include "actor_control_tail.h"
#include "actor_control_cells.h"
#include "actor_control_state.h"
#include "group_layouts.h"
#include "incomplete.h"
#include "initial_read_guards.h"
#include "read_guard.h"
#include "actor_control_tables.h"

typedef unsigned char actor_tail_u8;
typedef unsigned short actor_tail_u16;
typedef unsigned long actor_tail_u32;

typedef struct Eng1ActorControlRecord Eng1ActorControlRecord;
struct Eng1ActorControlRecord {
    short opcode;
    short x_or_secondary;
    short y_or_selector;
};

typedef char eng1_actor_control_record_is_six_bytes[
    (sizeof(Eng1ActorControlRecord) == 6) ? 1 : -1];
typedef char eng1_actor_tail_pointer_is_32_bits[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_actor_tail_long_is_32_bits[(sizeof(long) == 4) ? 1 : -1];
typedef char eng1_actor_tail_record_base_is_118[
    (offsetof(Actor, initial_steps) == 0x118 &&
     offsetof(Actor, unknown_11c) == 0x11c &&
     offsetof(Actor, action_callback_index) == 0x124) ? 1 : -1];

static actor_tail_u32 eng1_actor_tail_fixed16(short value)
{
    return ((actor_tail_u32)(long)value) << 16;
}

static long eng1_actor_tail_signed_bits(actor_tail_u32 value)
{
    return (long)value;
}

/* Reproduce the 32-bit SUB/CDQ/XOR/SUB absolute-value sequence without
 * signed-overflow expressions in C.  INT32_MIN remains negative, as on x86. */
static actor_tail_u32 eng1_actor_tail_abs_difference(long left, long right)
{
    actor_tail_u32 difference;

    difference = (actor_tail_u32)left - (actor_tail_u32)right;
    if (eng1_actor_tail_signed_bits(difference) < 0)
        difference = 0UL - difference;
    return difference;
}

static actor_tail_u32 eng1_actor_tail_z_limit(short value, int increment)
{
    actor_tail_u32 scaled;

    scaled = (actor_tail_u32)(long)value;
    if (increment) ++scaled;
    return scaled << 22;
}

/* The native 0x70e6c base is the bounds_by_row member at +0x20 in the
 * measured 0x70e4c ResourceGroupState owner. Signed classes -5..-1 alias
 * earlier cumulative WORDs in that same physical owner; use byte offsets so
 * those aliases remain explicit and guard the complete measured interval. */
static short *eng1_actor_tail_bounds_row(Actor *actor)
{
    long row_index;
    long byte_offset;
    actor_tail_u32 requested_rva;
    char reason[128];

    row_index = (long)(signed char)
        ((actor_tail_u32)actor->z >> 24);
    if (row_index < -5L || row_index >= 16L) {
        requested_rva = (actor_tail_u32)(0x70e6cL + row_index * 6L);
        sprintf(reason,
                "measured 0x70e4c owner: bounds RVA 0x70e6c row index %ld requests RVA 0x%08lx outside its 128-byte prefix",
                row_index, requested_rva);
        eng1_incomplete_behavior(0x70e6cUL, reason);
        return 0;
    }
    byte_offset = 0x20L + row_index * 6L;
    return (short *)((unsigned char *)(void *)&g_resource_group_state_70e4c +
                     byte_offset);
}

static actor_tail_u32 eng1_actor_tail_collision_root(void)
{
    return *(actor_tail_u32 *)(void *)g_collision_context;
}

/* RVA 0x6567c is a byte-offset view into the existing 0x65678 row owner:
 * column 2 of each measured 12-byte row. Only rows 0..17 are measured. Do not
 * read beyond that prefix unless later evidence extends the shared owner. */
static short *eng1_actor_tail_transition_row(long index)
{
    actor_tail_u32 address;
    actor_tail_u32 unsigned_index;
    actor_tail_u32 requested_rva;
    actor_tail_u32 magnitude;
    char reason[160];

    if (index < 0) {
        magnitude = 0UL - (actor_tail_u32)index;
        if (magnitude > 0x6567cUL / 12UL) {
            sprintf(reason,
                    "measured 0x65678 owner: base RVA 0x6567c index %ld; requested RVA underflows",
                    index);
            eng1_incomplete_behavior(0x6567cUL, reason);
            return 0;
        }
        requested_rva = 0x6567cUL - magnitude * 12UL;
        sprintf(reason,
                "measured 0x65678 owner: base RVA 0x6567c index %ld requests RVA 0x%08lx outside 18 measured rows",
                index, requested_rva);
        eng1_incomplete_behavior(0x6567cUL, reason);
        return 0;
    }

    unsigned_index = (actor_tail_u32)index;
    if (unsigned_index > (0xffffffffUL - 0x6567cUL) / 12UL) {
        sprintf(reason,
                "measured 0x65678 owner: base RVA 0x6567c index %ld; requested RVA arithmetic overflows",
                index);
        eng1_incomplete_behavior(0x6567cUL, reason);
        return 0;
    }
    requested_rva = 0x6567cUL + unsigned_index * 12UL;
    if (unsigned_index >= 18UL) {
        sprintf(reason,
                "measured 0x65678 owner: base RVA 0x6567c index %ld requests RVA 0x%08lx outside 18 measured rows",
                index, requested_rva);
        eng1_incomplete_behavior(0x6567cUL, reason);
        return 0;
    }

    address = (actor_tail_u32)(unsigned long)
        &g_eng1_classifier_rows_65678[0][2];
    if (unsigned_index > (0xffffffffUL - address) / 12UL) {
        sprintf(reason,
                "measured 0x65678 owner: base RVA 0x6567c index %ld requests RVA 0x%08lx; host address arithmetic overflows",
                index, requested_rva);
        eng1_incomplete_behavior(0x6567cUL, reason);
        return 0;
    }
    address += unsigned_index * 12UL;
    return (short *)(unsigned long)address;
}

static void eng1_actor_tail_store_direction_from_target(Actor *actor,
                                                         long target_x)
{
    if (actor->x < target_x)
        actor->direction_x = 0;
    else
        actor->direction_x = 0xff;
}

static void eng1_actor_tail_increment_transition(Actor *actor)
{
    actor->unknown_11c = (short)((actor_tail_u16)actor->unknown_11c + 1U);
}

int __cdecl eng1_native_dep_002d3f0(Actor *actor, actor_tail_u32 selector)
{
    Eng1ActorControlRecord *record;
    actor_tail_u32 row_index;

    record = (Eng1ActorControlRecord *)actor->initial_steps;
    row_index = 0;
    for (;;) {
        short opcode;

        eng1_guard_program_read(actor->initial_steps, (long)row_index, 6U, 2U,
                                "0x2d3f0 registered control record");
        eng1_guard_initial_read(actor->initial_steps, row_index, 6UL, 2UL,
                                "0x2d3f0 six-byte control record");
        opcode = record->opcode;
        if (opcode == 20) {
            eng1_guard_program_read(actor->initial_steps, (long)row_index,
                                    6U, 6U, "0x2d3f0 selector WORD");
            eng1_guard_initial_read(actor->initial_steps, row_index,
                                    6UL, 6UL, "0x2d3f0 selector WORD");
            if (selector == (actor_tail_u32)(long)record->y_or_selector) {
                g_eng1_actor_control_state_70ed8.words[9] = (actor_tail_u16)selector;
                actor->unknown_11c = (short)row_index;
                return 1;
            }
        }
        if (opcode == 0)
            return 0;
        ++row_index;
        ++record;
    }
}

static void eng1_actor_tail_case_two(Actor *actor,
                                     Eng1ActorControlRecord *record,
                                     actor_tail_u32 requested_x)
{
    Eng1ActorControlState70ed8 *state;
    actor_tail_u32 target_x_bits;
    actor_tail_u32 target_y_bits;
    actor_tail_u32 distance;
    long target_x;
    long lower_x;
    long upper_x;
    short *bounds;
    actor_tail_u8 orientation;

    state = &g_eng1_actor_control_state_70ed8;
    target_x_bits = requested_x;
    target_y_bits = eng1_actor_tail_fixed16(record->y_or_selector);

    if (record->y_or_selector == 1 && state->words[23] != 0)
        target_x_bits = (actor_tail_u32)state->words[23] << 16;
    if (record->y_or_selector == 2 && state->words[3] != 0)
        target_x_bits = (actor_tail_u32)state->words[3] << 16;
    target_x = eng1_actor_tail_signed_bits(target_x_bits);

    if (actor->unknown_02a == 1) {
        distance = eng1_actor_tail_abs_difference(actor->x, target_x);
        if (eng1_actor_tail_signed_bits(distance) < 0x00420000L) {
            eng1_guarded_select_frame(actor, 8);
            return;
        }

        orientation = *((actor_tail_u8 *)actor + 0x1f) & 3U;
        if (orientation == 1) {
            bounds = eng1_actor_tail_bounds_row(actor);
            if (bounds == 0)
                return;
            lower_x = eng1_actor_tail_signed_bits(
                eng1_actor_tail_z_limit(bounds[0], 0) - 0x00400000UL);
            upper_x = eng1_actor_tail_signed_bits(
                eng1_actor_tail_z_limit(bounds[1], 1));

            if (target_x > lower_x && actor->x < lower_x)
                actor->x = lower_x;
            if (target_x < upper_x && actor->x > upper_x)
                actor->x = upper_x;
        }
        eng1_actor_tail_store_direction_from_target(actor, target_x);
        return;
    }

    if (actor->unknown_02a == 0x15) {
        distance = eng1_actor_tail_abs_difference(actor->x, target_x);
        if (eng1_actor_tail_signed_bits(distance) > 0x00420000L)
            eng1_guarded_select_frame(actor, 7);
        return;
    }

    if (actor->unknown_02a == 0x16) {
        distance = eng1_actor_tail_abs_difference(actor->x, target_x);
        if (eng1_actor_tail_signed_bits(distance) >= 0x00100000L) {
            eng1_actor_tail_increment_transition(actor);
            return;
        }
        eng1_actor_tail_increment_transition(actor);
        actor->x = target_x;
        return;
    }

    eng1_guarded_select_frame(actor, 6);
    eng1_actor_tail_store_direction_from_target(actor, target_x);
    (void)target_y_bits;
}

static void eng1_actor_tail_handle_state_zero_or_eleven(Actor *actor,
                                                         int reverse)
{
    Eng1ActorControlState70ed8 *state;
    actor_tail_u32 control_index;
    short *row;
    unsigned short fallback;
    int frame;

    state = &g_eng1_actor_control_state_70ed8;
    control_index = (actor_tail_u32)state->words[13];
        row = eng1_actor_tail_transition_row((long)control_index);
    if (row == 0)
        return;
    fallback = (unsigned short)row[0];

    if (!reverse) {
        if (fallback < 8U) {
            if (state->words[25] == 0)
                return;
            frame = 0x22;
        } else if ((unsigned short)row[3] == 0) {
            frame = 0x22;
        } else {
            actor->unknown_02a = 4;
            actor->direction_x = 0;
            state->words[25] = 0;
            return;
        }
    } else {
        if (fallback < 8U) {
            if (state->words[25] == 0)
                return;
            frame = 0x31;
        } else if ((unsigned short)row[3] == 0) {
            frame = 0x31;
        } else {
            actor->unknown_02a = 4;
            actor->direction_x = 0;
            state->words[25] = 0;
            return;
        }
    }

    eng1_guarded_select_frame(actor, frame);
    actor->direction_x = 0;
    state->words[25] = 0;
}

static void eng1_actor_tail_case_five(Actor *actor)
{
    Eng1ActorControlState70ed8 *state;
    Actor *shared_actor;

    state = &g_eng1_actor_control_state_70ed8;
    if (actor->unknown_02a == 0 || actor->unknown_02a == 0x0b) {
        eng1_actor_tail_handle_state_zero_or_eleven(actor, 0);
        return;
    }

    if (actor->unknown_02a == 5) {
        shared_actor = (Actor *)(unsigned long)state->dwords[8];
        if (shared_actor == 0 || shared_actor->unknown_02a != 0)
            return;
        shared_actor->unknown_02a = 3;
        play_sound((unsigned short)0x4049U, shared_actor);
        return;
    }

    if (actor->unknown_02a == 9) {
        shared_actor = (Actor *)(unsigned long)state->dwords[8];
        if (shared_actor->unknown_02a == 9)
            eng1_actor_tail_increment_transition(actor);
    }
}

static void eng1_actor_tail_case_six(Actor *actor,
                                     actor_tail_u32 requested_x)
{
    Eng1ActorControlState70ed8 *state;
    Actor *shared_actor;

    state = &g_eng1_actor_control_state_70ed8;
    if (actor->unknown_02a == 0 || actor->unknown_02a == 0x0b) {
        eng1_actor_tail_handle_state_zero_or_eleven(actor, 1);
        return;
    }

    if (actor->unknown_02a == 5) {
        shared_actor = (Actor *)(unsigned long)state->dwords[8];
        if (shared_actor == 0 || shared_actor->unknown_02a != 0)
            return;
        shared_actor->unknown_02a = 1;
        play_sound((unsigned short)0x404aU, shared_actor);
        return;
    }

    if (actor->unknown_02a == 9) {
        if (eng1_actor_tail_signed_bits(
                eng1_actor_tail_collision_root()) <=
            eng1_actor_tail_signed_bits(requested_x))
            return;
        if (state->dwords[8] == 0)
            return;
        eng1_actor_tail_increment_transition(actor);
    }
}

static void eng1_actor_tail_case_eleven(Actor *actor,
                                        actor_tail_u32 requested_x,
                                        actor_tail_u32 requested_y)
{
    actor_tail_u32 root_x_bits;
    actor_tail_u32 difference_bits;
    actor_tail_u32 distance;
    actor_tail_u32 lower_bits;
    actor_tail_u32 upper_bits;
    long root_x;
    long actor_x_before_bounds;
    long difference;
    long lower_x;
    long upper_x;
    short *bounds;
    actor_tail_u8 orientation;

    root_x_bits = eng1_actor_tail_collision_root();
    root_x = eng1_actor_tail_signed_bits(root_x_bits);

    if (actor->x < eng1_actor_tail_signed_bits(requested_x))
        actor->x = eng1_actor_tail_signed_bits(requested_x);
    if (actor->x > eng1_actor_tail_signed_bits(requested_y))
        actor->x = eng1_actor_tail_signed_bits(requested_y);

    /* Native caches actor.x - root.x after the record's [x,y] clamp, but
     * before the later row-bounds clamp.  Keep this value for the state tests. */
    actor_x_before_bounds = actor->x;
    difference_bits = (actor_tail_u32)actor_x_before_bounds - root_x_bits;
    difference = eng1_actor_tail_signed_bits(difference_bits);

    orientation = *((actor_tail_u8 *)actor + 0x1f) & 3U;

    if (orientation == 1) {
        bounds = eng1_actor_tail_bounds_row(actor);
        if (bounds == 0)
            return;
        lower_bits = eng1_actor_tail_z_limit(bounds[0], 0) -
                     0x00800000UL;
        upper_bits = eng1_actor_tail_z_limit(bounds[1], 0) +
                     0x00800000UL;
        lower_x = eng1_actor_tail_signed_bits(lower_bits);
        upper_x = eng1_actor_tail_signed_bits(upper_bits);

        if (root_x > lower_x && actor->x < lower_x)
            actor->x = lower_x;
        if (root_x < upper_x && actor->x > upper_x)
            actor->x = upper_x;
        if (actor->x < root_x)
            actor->direction_x = 0;
        else
            actor->direction_x = 0xff;
    }

    if (actor->unknown_02a == 1) {
        if (actor->direction_x != 0) {
            if (difference >= (long)0xffe80000UL)
                return;
        } else if (difference <= 0x00180000L) {
            return;
        }
        eng1_guarded_select_frame(actor, 8);
        return;
    }

    if (actor->unknown_02a == 0) {
        if (actor->direction_x != 0) {
            if (difference <= 0)
                return;
        } else if (difference >= 0) {
            return;
        }
        distance = eng1_actor_tail_abs_difference(actor_x_before_bounds,
                                                   root_x);
        if (eng1_actor_tail_signed_bits(distance) <= 0x00600000L)
            return;
        eng1_guarded_select_frame(actor, 7);
    }
}

void __cdecl eng1_native_dep_002d440(Actor *actor)
{
    Eng1ActorControlRecord *record;
    Eng1ActorControlState70ed8 *state;
    actor_tail_u32 record_index;
    actor_tail_u32 target_x_bits;
    actor_tail_u32 target_y_bits;
    actor_tail_u32 distance;
    short opcode;
    short x;
    short y;
    short *row;

    if (actor->unknown_11c < 0)
        actor->unknown_11c = 0;
    record_index = (actor_tail_u32)(actor_tail_u16)actor->unknown_11c;
    eng1_guard_program_read(actor->initial_steps, (long)record_index, 6U, 6U,
                            "0x2d440 registered control record");
    eng1_guard_initial_read(actor->initial_steps, record_index, 6UL, 6UL,
                            "0x2d440 six-byte control record");
    record = (Eng1ActorControlRecord *)actor->initial_steps + record_index;
    opcode = record->opcode;
    x = record->x_or_secondary;
    y = record->y_or_selector;
    target_x_bits = eng1_actor_tail_fixed16(x);
    target_y_bits = eng1_actor_tail_fixed16(y);
    state = &g_eng1_actor_control_state_70ed8;

    switch (opcode) {
    case 0:
        return;
    case 1:
        eng1_actor_tail_increment_transition(actor);
        actor->x = eng1_actor_tail_signed_bits(target_x_bits);
        actor->y = eng1_actor_tail_signed_bits(target_y_bits);
        return;
    case 2:
        eng1_actor_tail_case_two(actor, record, target_x_bits);
        return;
    case 3:
        actor->action_callback_index = (short)x;
        state->words[9] = (actor_tail_u16)y;
        eng1_actor_tail_increment_transition(actor);
        return;
    case 4:
        if (state->dwords[5] != 0) {
            actor_tail_u16 *first;
            first = (actor_tail_u16 *)(unsigned long)state->dwords[5];
            if (first[0x124 / 2] == first[0x126 / 2])
                eng1_actor_tail_increment_transition(actor);
        }
        return;
    case 5:
        eng1_actor_tail_case_five(actor);
        return;
    case 6:
        eng1_actor_tail_case_six(actor, target_x_bits);
        return;
    case 7:
        eng1_guarded_select_frame(actor, 0x2e);
        eng1_actor_tail_increment_transition(actor);
        return;
    case 8:
        eng1_actor_tail_increment_transition(actor);
        actor->z = eng1_actor_tail_signed_bits(target_x_bits);
        return;
    case 9:
        if (eng1_actor_tail_signed_bits(
                eng1_actor_tail_collision_root()) >
            eng1_actor_tail_signed_bits(target_x_bits)) {
            actor->unknown_02a = 0;
            eng1_actor_tail_increment_transition(actor);
        } else {
            actor->unknown_02a = 0x0a;
        }
        return;
    case 10:
        eng1_actor_tail_increment_transition(actor);
        return;
    case 11:
        eng1_actor_tail_case_eleven(actor, target_x_bits, target_y_bits);
        return;
    case 12:
        row = eng1_actor_tail_transition_row((long)y);
        if (row == 0)
            return;
        if ((unsigned short)row[0] >= 4U) {
            actor->unknown_11c = (short)((short)x + actor->unknown_11c);
        }
        eng1_actor_tail_increment_transition(actor);
        return;
    case 13:
        eng1_guarded_select_frame(actor, 0);
        eng1_actor_tail_increment_transition(actor);
        return;
    case 14:
        return;
    default:
        eng1_actor_tail_increment_transition(actor);
        return;
    }
}
