#include "setup_shared_state.h"
#include "forest_entry_data.h"

extern int __cdecl initialize_command_table(unsigned long bytes,
                                             unsigned int opcode,
                                             unsigned long count);

/* ENG1 RVA 0x3ca10. The callee's return is deliberately ignored.  The
 * 0x7183c cell is the signed camera-Y limit read by camera smoothing as
 * (short)W(7183c); storing -2 to it as a short lets the compiler share one
 * EAX = -2 for both WORD stores, as native does. */
void __cdecl eng1_native_code_target_003ca10(void)
{
    g_eng1_setup_word_718e0 = 0x0360;
    eng1_native_ptr_071714 = g_eng1_sound_words_66d68;
    (void)initialize_command_table(0x14UL, 0x3781U, 0x0eUL);
    g_camera_adjust_y_70d04 = -2;
    *(short *)&g_eng1_setup_word_7183c = -2;
}
