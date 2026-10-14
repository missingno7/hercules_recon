#include "color_transition_views.h"

/* ENG1 callback pair44 initializer, RVA0x1eba0. */
void __cdecl eng1_native_code_target_01eba0(void *object)
{
    color_u8 *bytes;
    color_u32 current;
    color_u32 target;

    bytes = (color_u8 *)object;
    *(color_u32 *)(void *)(bytes + 0x54) = 0UL;
    *(color_u16 *)(void *)(bytes + 0x34) = 0U;
    *(color_u16 *)(void *)(bytes + 0x32) = 0xffbbU;

    g_eng1_color_transition_timer_71ecc = 0xffffffffUL;
    g_eng1_color_transition_state_6f4a8.selection = 0xffffffffUL;
    g_eng1_color_transition_state_6f4a8.previous_selection = 0xffffffffUL;

    current = g_eng1_color_transition_pairs_589b8[0].current;
    target = g_eng1_color_transition_pairs_589b8[0].target;
    g_eng1_color_transition_state_6f4a8.previous_current = target;
    g_eng1_color_transition_state_6f4a8.target_value = target;
    g_eng1_color_transition_state_6f4a8.previous_target = target;
    g_eng1_color_transition_state_6f4a8.interpolation = 0UL;
    g_eng1_color_transition_state_6f4a8.current_target = current;
    g_eng1_color_transition_state_6f4a8.current_value = current;
}
