#include "setup_shared_state.h"
#include "forest_entry_data.h"

extern int __cdecl initialize_command_table(unsigned long bytes,
                                             unsigned int opcode,
                                             unsigned long count);

/* ENG1 RVA 0x3ca10. The callee's return is deliberately ignored; native
 * overwrites EAX with -2 before storing the final two WORD cells. */
void __cdecl eng1_native_code_target_003ca10(void)
{
    *(volatile unsigned short *)&g_eng1_setup_word_718e0 = 0x0360;
    *(unsigned short *volatile *)(void *)&g_actor_pipeline_storage_71440.byte_cells[0x2d4] =
        g_eng1_sound_words_66d68;
    (void)initialize_command_table(0x14UL, 0x3781U, 0x0eUL);
    *(volatile short *)&g_camera_adjust_y_70d04 = (short)0xfffeU;
    *(volatile unsigned short *)&g_eng1_setup_word_7183c = (short)0xfffeU;
}
