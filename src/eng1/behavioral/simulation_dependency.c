/* Behavioral ENG1 simulation helpers.
 * Reconstructs native entries 0x12620 and 0x12790, using the existing strict
 * 0x29260 reset_frame_state provider and measured simulation data.
 */
#include <stddef.h>
#include "../../../calibration/engine_interface.h"
#include "../../../calibration/resource_record.h"
#include "interpreters.h"
#include "simulation_data_owners.h"
#include "incomplete.h"

typedef unsigned char sim_u8;
typedef unsigned short sim_u16;
typedef unsigned long sim_u32;

typedef char simulation_u32_is_four[(sizeof(sim_u32) == 4) ? 1 : -1];

extern void *g_alternate_actor_pool;
extern ActorResource g_actor_resources[];
extern sim_u32 eng1_sim_state_72174;
extern sim_u32 eng1_sim_state_72104;
extern sim_u16 eng1_sim_state_6ef40;

extern void __cdecl resource_reset(int resource_id);
extern void __cdecl resource_cancel(int resource_id);
extern void __cdecl reset_frame_state(Actor *actor);
/* Existing source providers with the measured native physical argument
 * widths: play_sound receives a WORD effect id and actor pointer. */

/* Measured source prefixes, not an inferred native capacity. Reject an
 * unsupported read at its original dependency identity without success. */
static sim_u32 simulation_resource_for_row(sim_u32 index)
{
    sim_u32 resource_id;
    if (index >= 64UL) {
        eng1_incomplete_behavior(0x53800UL,
            "simulation resource view exceeds measured64-DWORD prefix");
        return 0;
    }
    resource_id = eng1_native_data_target_0053800[index];
    if (resource_id >= 81UL) {
        eng1_incomplete_behavior(0x5cee0UL,
            "simulation resource id exceeds the measured81-row owner");
        return 0;
    }
    return resource_id;
}

/* Native 0x12790. The caller supplies an alternate row and an index. The
 * second argument is a DWORD at the physical call boundary; the normal parent
 * passes 0..7. Its return register is ignored by the only reviewed caller. */
sim_u32 __cdecl dispatch_local_12790(void *actor, sim_u32 index)
{
    sim_u8 *bytes;
    sim_u16 row_timer;
    sim_u32 target_position;
    sim_u32 current_position;
    sim_u32 resource_index;
    sim_u32 flags;
    sim_u32 mask;
    sim_u32 i;
    unsigned short *context_mask;

    bytes = (sim_u8 *)actor;
    row_timer = *(sim_u16 *)(bytes + 0x2a);
    if (row_timer != 0) {
        row_timer = (sim_u16)(row_timer - 1U);
        *(sim_u16 *)(bytes + 0x2a) = row_timer;
        if (row_timer == 0)
            play_sound((unsigned short)13, 0);
    } else {
        target_position = *(sim_u32 *)(bytes + 0x4c);
        i = 6;
        do {
            current_position = *(sim_u32 *)(bytes + 4);
            if ((long)current_position < (long)target_position) {
                current_position += 0x00010000UL;
                *(sim_u32 *)(bytes + 4) = current_position;
            }

            current_position = *(sim_u32 *)(bytes + 4);
            if ((long)current_position > (long)target_position) {
                current_position += 0xffff0000UL;
                if ((long)current_position <= 0x0000ffffL)
                    current_position = 0;
                *(sim_u32 *)(bytes + 4) = current_position;
            }
            --i;
        } while (i != 0);
    }

    if (index == eng1_sim_state_72104) {
        /* The only measured caller is 0x12620, which passes rows 0..7.
         * This is inside both the 64-DWORD view and the 81 resource rows. */
        resource_index = simulation_resource_for_row(index);
        if (g_actor_resources[resource_index].residency_state != 3) {
            resource_reset((int)resource_index);
            reset_frame_state((Actor *)actor);
        }

        (void)eng1_interpret_word_stream((Actor *)actor);
        if (*(sim_u8 *)(bytes + 0x21) == 0) {
            if (*(sim_u8 *)(bytes + 0x22) == 2) {
                reset_frame_state((Actor *)actor);
                resource_index = simulation_resource_for_row(index);
                resource_cancel((int)resource_index);
            }
        } else {
            --*(sim_u8 *)(bytes + 0x21);
        }
    }

    /* Native reloads the context pointer after all intervening helper calls. */
    context_mask = (unsigned short *)(
        (sim_u8 *)g_engine_interface.context_004 + 0x1c);
    mask = 1UL << (index & 31UL);
    flags = *(sim_u32 *)(bytes + 0x54);
    if (((sim_u32)*context_mask & mask) != 0)
        flags = (flags & 0xffffff00UL) | ((flags & 0xffUL) & 0xf8UL);
    else
        flags |= 7UL;
    *(sim_u32 *)(bytes + 0x54) = flags;

    current_position = *(sim_u32 *)(bytes + 4);
    flags = *(sim_u32 *)(bytes + 0x54);
    if (current_position != 0)
        flags |= 0x80000000UL;
    else
        flags &= 0x7fffffffUL;
    *(sim_u32 *)(bytes + 0x54) = flags;
    return flags;
}

/* Native 0x12620. Mode selection is a DWORD unsigned switch; the compiler's
 * generated switch table is ordinary codegen. The native five-entry table
 * starts at 0x1277c after the RET, padding, and hotpatch sequence. */
void __cdecl dispatch_local_12620(void)
{
    sim_u32 mode;
    sim_u32 index;
    sim_u32 offset;
    sim_u8 *pool;
    sim_u8 *actor;

    mode = eng1_sim_state_72174;
    switch (mode) {
    case 0:
        offset = 0x128UL;
        while (offset < 0x5c8UL) {
            pool = (sim_u8 *)g_alternate_actor_pool;
            *(sim_u32 *)(pool + offset + 4) = 0;
            offset += 0x94UL;
        }
        return;

    case 1:
        eng1_sim_state_72174 = mode + 1UL;
        eng1_sim_state_6ef40 = 0x8cU;
        index = 0;
        offset = 0x128UL;
        while (offset < 0x5c8UL) {
            pool = (sim_u8 *)g_alternate_actor_pool;
            actor = pool + offset;
            *(sim_u32 *)(actor + 4) = 0;
            *(sim_u16 *)(actor + 0x2a) = (sim_u16)(index * 4UL);
            *(sim_u32 *)(actor + 0x4c) = 0x00460000UL;
            if (index == eng1_sim_state_72104) {
                *(sim_u8 *)(actor + 0x21) = 0x3cU;
                *(sim_u32 *)(actor + 0x64) =
                    ENG1_NATIVE_DATA_TARGET_00537E0_DWORD(index);
                reset_frame_state((Actor *)actor);
            } else {
                *(sim_u8 *)(actor + 0x22) = 2U;
                *(sim_u16 *)(actor + 0x34) =
                    eng1_native_data_target_0052b70[index].value;
            }
            offset += 0x94UL;
            ++index;
        }
        break;

    case 2:
        eng1_sim_state_6ef40 =
            (sim_u16)(eng1_sim_state_6ef40 - 1U);
        if ((short)eng1_sim_state_6ef40 <= 0)
            eng1_sim_state_72174 = mode + 1UL;
        break;

    case 3:
        eng1_sim_state_72174 = mode + 1UL;
        eng1_sim_state_6ef40 = 0x64U;
        index = 0;
        offset = 0x128UL;
        while (offset < 0x5c8UL) {
            pool = (sim_u8 *)g_alternate_actor_pool;
            actor = pool + offset;
            *(sim_u16 *)(actor + 0x2a) = (sim_u16)(index * 4UL);
            *(sim_u32 *)(actor + 0x4c) = 0;
            offset += 0x94UL;
            ++index;
        }
        break;

    case 4:
        eng1_sim_state_6ef40 =
            (sim_u16)(eng1_sim_state_6ef40 - 1U);
        if ((short)eng1_sim_state_6ef40 <= 0)
            eng1_sim_state_72174 = 0;
        break;

    default:
        break;
    }

    index = 0;
    offset = 0x128UL;
    while (offset < 0x5c8UL) {
        /* The original reloads the shared pool pointer for each callback. */
        pool = (sim_u8 *)g_alternate_actor_pool;
        actor = pool + offset;
        (void)dispatch_local_12790(actor, index);
        offset += 0x94UL;
        ++index;
    }
}
