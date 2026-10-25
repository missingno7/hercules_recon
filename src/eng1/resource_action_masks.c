/* PC-backed gameplay resource dispatcher and eight-slot mask helpers.
 * Source ownership/TU and original declarations remain unproved. */
#include <stddef.h>
#include "../shared/engine_interface.h"
#include "../shared/resource_record.h"
#include "setup_shared_state.h"
#include "simulation_data_owners.h"

extern ActorResource g_actor_resources[81];
extern void __cdecl resource_reset(int resource_id);
extern void __cdecl resource_cancel(int resource_id);

/* ENG1 RVA 0x128a0: merge the incoming low byte into the shared pending mask,
 * then reset each currently pending slot unless its live context or record
 * already marks it as active/completed. The owner and capacities are the
 * measured eight-slot and 81-record views. */
void __cdecl eng1_resource_reset_mask_128a0(unsigned long action)
{
    /* Signed induction reproduces the observed JL. Original spelling/type
     * is not established; the supported scan is exactly slots zero to seven. */
    long slot;
    unsigned long bit;
    unsigned long resource_id;
    unsigned short *context_mask;

    g_eng1_setup_byte_72010 = (unsigned char)(
        g_eng1_setup_byte_72010 | (unsigned char)action);

    for (slot = 0; slot < 8L; ++slot) {
        bit = 1UL << slot;
        if ((g_eng1_setup_byte_72010 & bit) != 0) {
            context_mask = (unsigned short *)(
                (unsigned char *)g_engine_interface.context_004 + 0x1c);
            if ((*context_mask & bit) == 0) {
                resource_id = g_eng1_simulation_lookup_owner_537e0
                    .value_view_53800[slot];
                if (g_actor_resources[resource_id].residency_state != 3U)
                    resource_reset((int)resource_id);
            }
        }
    }
}

/* ENG1 RVA 0x12910: clear selected pending bits once, then cancel resources
 * selected by the cached incoming low byte. The table is reloaded per slot. */
void __cdecl eng1_resource_cancel_mask_12910(unsigned long action)
{
    unsigned long slot;
    unsigned long bit;
    unsigned long selected;
    unsigned long resource_id;

    selected = action & 0xffUL;
    g_eng1_setup_byte_72010 = (unsigned char)(
        g_eng1_setup_byte_72010 & (unsigned char)~selected);

    for (slot = 0; slot < 8UL; ++slot) {
        bit = 1UL << slot;
        if ((selected & bit) != 0) {
            resource_id = g_eng1_simulation_lookup_owner_537e0
                .value_view_53800[slot];
            resource_cancel((int)resource_id);
        }
    }
}

/* ENG1 RVA 0x28290: flag 0x400 selects the eight-slot mask family;
 * flag 0x800 selects cancellation on either path. */
void __cdecl eng1_native_code_target_028290(unsigned long action)
{
    if ((action & 0x400UL) != 0) {
        if ((action & 0x800UL) != 0)
            eng1_resource_cancel_mask_12910(action);
        else
            eng1_resource_reset_mask_128a0(action);
    } else if ((action & 0x800UL) != 0) {
        resource_cancel((int)(action & 0xffUL));
    } else {
        resource_reset((int)(action & 0xffUL));
    }
}
