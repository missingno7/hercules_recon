#include "setup_shared_state.h"
#include "../../../calibration/engine_interface.h"

extern EngineInterface g_engine_interface;
extern void *g_normal_actor_pool;

/* Native 0x11fc0 is a no-argument cdecl leaf. Keep the live context load
 * between the first two WORD stores and the later writes. The six-DWORD clear
 * ends at 0x72158; the following WORD is a separate owner. */
void __cdecl dispatch_local_11fc0(void)
{
    unsigned char *context;
    unsigned long i;

    g_eng1_setup_word_70822 = 0x00b9;
    g_eng1_setup_word_7219e = 0x00b9;
    context = (unsigned char *)(void *)g_engine_interface.context_004;
    g_eng1_setup_word_7082e = 0;
    g_eng1_setup_word_70820 = 0x007d;
    g_eng1_setup_word_7219c = 0x007d;
    g_eng1_setup_word_721a2 = 0;
    g_eng1_setup_word_721a4 = 0;
    g_eng1_setup_word_72194 = 0;
    g_eng1_setup_word_721a6 = 0x0064;
    g_eng1_setup_word_721a0 = 0;
    g_eng1_setup_dword_6ef5c = 0;
    g_eng1_setup_dword_72188 = 0;
    *(unsigned short *)(void *)(context + 0x22) = 0;
    for (i = 0; i != 6; ++i)
        g_eng1_setup_clear_72140[i] = 0;
}

/* Native 0x15670 is a no-argument cdecl leaf. The normal-pool pointer is
 * captured before the first write. The collision context is loaded once for
 * the +0x54 DWORD update and reloaded for the later +0x4a WORD store. */
void __cdecl dispatch_local_15670(void)
{
    unsigned char *normal_pool;
    unsigned char *collision;
    unsigned long flags;

    normal_pool = (unsigned char *)g_normal_actor_pool;
    g_eng1_setup_word_6ef94 = 0;
    normal_pool[0xec] = 0;
    collision = *(unsigned char * volatile *)(void *)&g_collision_context;
    flags = *(unsigned long *)(void *)(collision + 0x54);
    flags &= 0xfffffffbUL;
    *(unsigned long *)(void *)(collision + 0x54) = flags;
    collision = *(unsigned char * volatile *)(void *)&g_collision_context;
    *(unsigned short *)(void *)(collision + 0x4a) = 0x00a0;
}
