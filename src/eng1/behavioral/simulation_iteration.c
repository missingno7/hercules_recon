/* Behavioral reconstruction for ENG1.DLL RVA 0x156a0.
 * The function stays RVA-labelled because its gameplay role and source unit
 * remain unproved. Its two state-update callees have separate identities.
 */
#include "../../../calibration/engine_interface.h"

typedef unsigned char eng1_sim_u8;
typedef unsigned short eng1_sim_u16;
typedef unsigned long eng1_sim_u32;

extern eng1_sim_u32 eng1_sim_state_72110;
extern eng1_sim_u32 eng1_sim_state_72170;
extern eng1_sim_u16 eng1_sim_state_72108;
extern eng1_sim_u16 eng1_sim_state_6feb4;
extern void *g_normal_actor_pool;

/* Separate unresolved behavioral dependencies; do not replace with no-ops. */
extern void __cdecl eng1_native_code_target_0015780(void); /* RVA 0x15780 */
extern void __cdecl eng1_native_code_target_0012620(void); /* RVA 0x12620 */

void __cdecl dispatch_local_156a0(void)
{
    ResourceCallbackContext *context;
    eng1_sim_u32 context_flags;
    eng1_sim_u32 quantized;
    eng1_sim_u8 *actor;

    context = g_engine_interface.context_004;
    context_flags = *(eng1_sim_u32 *)((eng1_sim_u8 *)context + 0x30);

    if ((((eng1_sim_u8 *)context)[0x65] & 0x04U) != 0 &&
        (context_flags & 0x00004000UL) != 0)
        eng1_sim_state_72110 = 1UL;

    if ((((eng1_sim_u8 *)context)[0x65] & 0x01U) != 0 &&
        (context_flags & 0x00004000UL) != 0) {
        eng1_sim_state_72110 = 1UL;
        context->unknown_000[0x0c] = 1U;
        context = g_engine_interface.context_004;
        if (((eng1_sim_u8 *)context)[0x10] == 0)
            ((eng1_sim_u8 *)context)[0x10] = 1U;
        context = g_engine_interface.context_004;
    }

    if ((((eng1_sim_u8 *)context)[0x65] & 0x02U) != 0 &&
        (context_flags & 0x00004000UL) != 0) {
        context->unknown_000[0x0c] = 1U;
        context = g_engine_interface.context_004;
        eng1_sim_state_72110 = 1UL;
        context->unknown_000[0x10] = 0U;
        context = g_engine_interface.context_004;
    }

    if ((((eng1_sim_u8 *)context)[0x65] & 0x08U) != 0 &&
        eng1_sim_state_72108 != 0x0019U &&
        (context_flags & 0x00002000UL) != 0)
        eng1_sim_state_72170 = 1UL;

    eng1_native_code_target_0015780();
    context = g_engine_interface.context_004;
    eng1_sim_state_6feb4 = 0U;

    /* Native reloads this pointer before clearing 0x6feb4. */
    context_flags = *(eng1_sim_u32 *)((eng1_sim_u8 *)context + 0x30);
    if ((context_flags & 0x00008000UL) != 0 &&
        (context->unknown_05c & 0xa000U) == 0xa000U) {
        actor = (eng1_sim_u8 *)g_normal_actor_pool;
        quantized = *(eng1_sim_u32 *)(actor + 8);
        quantized += 0x00400000UL;
        quantized &= 0xff800000UL;
        *(eng1_sim_u32 *)(actor + 8) = quantized;
    }

    eng1_native_code_target_0012620();
}
