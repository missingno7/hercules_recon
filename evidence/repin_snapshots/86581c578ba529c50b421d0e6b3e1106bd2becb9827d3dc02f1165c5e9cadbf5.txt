/* ENG1.DLL 0x1aba0: select and dispatch one of three sound IDs.
 * The native entry has no arguments. It reads the low byte of the existing
 * setup WORD, then uses either a null owner or the normal actor-pool root.
 * Original names and translation-unit ownership remain unproved.
 */
#include "setup_shared_state.h"

extern void *g_normal_actor_pool;
extern unsigned int __cdecl dispatch_engine_2c460(int limit);
extern void __cdecl play_sound(unsigned short id, void *owner);

void __cdecl eng1_1aba0(void)
{
    unsigned int roll;

    if ((*(unsigned char *)(void *)&g_eng1_setup_word_72012 & 0x18) != 0) {
        play_sound(0x27a, (void *)0);
        return;
    }

    roll = dispatch_engine_2c460(0x8000);
    if (roll < 0x4000)
        play_sound(0x24, g_normal_actor_pool);
    else
        play_sound(0x25, g_normal_actor_pool);
}
