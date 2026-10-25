#include "engine_interface.h"

/* Names are private semantic labels; native source and ownership are unknown. */
extern int g_context_sequence_phase;
extern int g_context_sequence_index;
extern unsigned short *g_context_sequence_words;

void __cdecl context_sequence_step(void)
{
    unsigned int previous_word;

    if (g_engine_interface.context_004->unknown_000[4] != 0)
        return;

    switch (g_context_sequence_phase) {
    case 0:
        break;
    case 2:
        if (g_context_sequence_index < 0x374) {
            if (g_engine_interface.context_004->unknown_05c != 0) {
                g_engine_interface.context_004->unknown_000[3] = 0x13;
            }

            g_engine_interface.context_004->unknown_05c =
                g_context_sequence_words[g_context_sequence_index];
            g_context_sequence_index = g_context_sequence_index + 1;

            previous_word = g_engine_interface.context_004->unknown_05c;
            g_engine_interface.context_004->unknown_05c =
                g_context_sequence_words[g_context_sequence_index];
            g_engine_interface.context_004->unknown_05e =
                (unsigned short)((previous_word ^
                                  g_engine_interface.context_004->unknown_05c) &
                                 g_engine_interface.context_004->unknown_05c);
            g_engine_interface.context_004->unknown_060 = 0;
        } else {
            g_engine_interface.context_004->unknown_000[3] = 0x13;
        }
    }

}
