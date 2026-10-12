#include "setup_shared_state.h"

/* Measured two-byte loader-zero input owner at native 0x70f0c.
 * Wider neighboring storage and dynamic writer coverage remain unproved. */
extern unsigned short g_eng1_input_word_70f0c;

/* ENG1 RVA 0x3ca50. Native loads WORD 0x70f0c into zeroed EAX, then stores
 * one of two WORD sentinels through the existing actor-pipeline owner. */
void __cdecl eng1_native_code_target_003ca50(void)
{
    unsigned long value = (unsigned long)*(volatile unsigned short *)&g_eng1_input_word_70f0c;

    if (value == 0x0cUL) {
        *(volatile unsigned short *)&g_eng1_setup_word_7183c = 0xffc0U;
    } else if (value <= 0x0eUL) {
        *(volatile unsigned short *)&g_eng1_setup_word_7183c = 0xfffeU;
    } else if (value <= 0x11UL) {
        *(volatile unsigned short *)&g_eng1_setup_word_7183c = 0xffc0U;
    } else {
        *(volatile unsigned short *)&g_eng1_setup_word_7183c = 0xfffeU;
    }
}
