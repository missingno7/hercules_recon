/* Behavioral reconstruction of ENG1 RVA 0x12960. */
#include "simulation_data_owners.h"
#include "incomplete.h"

typedef char eng1_call_12960_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_call_12960_ulong_is_32[(sizeof(unsigned long) == 4) ? 1 : -1];

extern void *g_alternate_actor_pool;
extern void __cdecl initialize_alternate_actor(void *actor);

/* The native routine reads exactly eight DWORDs from this writable prefix.
 * The values are kept separate from the already-owned WORD table at 0x52b70. */
extern unsigned long g_eng1_call_12960_dword_52b50[8];

void __cdecl eng1_call_12960(void)
{
    unsigned char *pool;
    unsigned long offset;
    unsigned long index;

    offset = 0x128UL;
    index = 0;
    while (offset < 0x5c8UL) {
        volatile unsigned char *actor;
        unsigned long leading_dword;

        /* Native reloads the pool cell after each preceding initializer. */
        pool = (unsigned char *)*(void *volatile *)&g_alternate_actor_pool;
        if (!pool)
            eng1_incomplete_behavior(0x734e0UL,
                "0x12960 requires the alternate actor pool pointer");
        actor = pool + offset;
        initialize_alternate_actor((void *)actor);
        *(volatile unsigned long *)(void *)(actor + 0x50) = 6UL;
        *(volatile unsigned short *)(void *)(actor + 0x2e) = 0x200cU;
        *(volatile unsigned long *)(void *)(actor + 0x54) = 0xc0000000UL;
        *(volatile unsigned short *)(void *)(actor + 0x3e) = 4U;
        actor[0x23] = 6U;
        *(volatile unsigned short *)(void *)(actor + 0x34) =
            *(volatile unsigned short *)(void *)&eng1_native_data_target_0052b70[index].value;
        actor[0x22] = 2U;
        leading_dword = *(volatile unsigned long *)(void *)&g_eng1_call_12960_dword_52b50[index];
        offset += 0x94UL;
        ++index;
        *(volatile unsigned long *)(void *)(actor + 0x00) = leading_dword;
        *(volatile unsigned long *)(void *)(actor + 0x04) = 0x00500000UL;
        *(volatile unsigned long *)(void *)(actor + 0x08) = 0UL;
    }
}
