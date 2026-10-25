#include "color_transition_views.h"
#include "actor_pipeline_storage.h"
#include "../shared/engine_interface.h"
#include "incomplete.h"

extern unsigned int __cdecl dispatch_engine_2c460(int limit);
extern void __cdecl play_sound(unsigned short effect, void *actor);

/* ENG1:1ebf0. Services may change the actor, transition state and context.
 * Preserve their order and the late reloads. Bounded behavior recovery only;
 * historical source spelling, original capacities and byte identity are unknown. */
void __cdecl eng1_native_code_target_01ebf0(void *object)
{
    short *selection;
    unsigned int value;
    long count;
    unsigned char *row;
    ResourceCallbackContext *context;
    if (g_eng1_color_transition_state_6f4a8.previous_selection == 0xffffffffUL) {
        if (dispatch_engine_2c460((int)g_eng1_color_transition_state_6f4a8.current_value) == 10)
            play_sound(0x19c, 0);
        return;
    }
    selection = (short *)((unsigned char *)object + 0x32);
    if (*selection == -69) {
        *selection = 9;
        value = dispatch_engine_2c460(2);
        if (value == 0) play_sound(0x18b, 0);
        else if (value == 1) play_sound(0x190, 0);
    } else {
        if (dispatch_engine_2c460((int)g_eng1_color_transition_state_6f4a8.current_value) == 0) {
            *selection = 4;
            value = dispatch_engine_2c460(2);
            if (value == 0) play_sound(0x18b, 0);
            else if (value == 1) play_sound(0x190, 0);
        }
        if (dispatch_engine_2c460((int)(g_eng1_color_transition_state_6f4a8.current_value * 2UL)) == 10)
            play_sound(0x19c, 0);
    }
    if (*selection < 0) return;
    /* Ten observed rows and the existing shared owner, not an original maximum.
     * Check after services: native callbacks may change the actor and count. */
    if (*selection >= 10)
        eng1_incomplete_behavior(0x1ebf0, "color row outside measured ten-row prefix");
    context = g_engine_interface.context_004;
    row = g_eng1_color_list_rows_58a20[*selection];
    count = (long)g_actor_pipeline_storage_71440.dword_cells[0x6a0 / 4];
    if (count < 0 || count >= 135)
        eng1_incomplete_behavior(0x1ebf0, "color append outside measured shared-list prefix");
    g_actor_pipeline_storage_71440.dword_cells[0x6a0 / 4] = count + 1;
    g_actor_pipeline_storage_71440.dword_cells[0x6a4 / 4 + count] =
        (unsigned long)row;
    if (context->tick_038 & 1) --*selection;
}
