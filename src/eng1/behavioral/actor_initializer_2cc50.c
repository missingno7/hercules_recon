/* Ordinary-C behavioral reconstruction of ENG1.DLL RVA 0x2cc50.
 * The callback table uses cdecl actor-pointer entries; this body has one
 * measured stack argument and its caller-facing return contract is void. */
#include "interpreters.h"
#include "actor_pipeline_storage.h"
#include "initial_motion_roots.h"
#include "actor_control_state.h"

extern Eng1InitialMotion2 g_eng1_initial_motion_05c3a0[6];
extern void __cdecl eng1_native_code_target_002a290(Actor *actor);

void __cdecl eng1_native_code_target_02cc50(Actor *actor)
{
    volatile Eng1ActorControlState70ed8 *state;
    volatile Actor *view;

    state = &g_eng1_actor_control_state_70ed8;
    view = actor;
    state->dwords[5] = 0UL;           /* DWORD 0x70eec */
    state->words[9] = 0U;             /* WORD 0x70eea */
    state->words[19] = 0U;            /* WORD 0x70efe */
    state->words[18] = 0U;            /* WORD 0x70efc */
    state->words[25] = 0U;            /* WORD 0x70f0a */
    *(volatile unsigned short *)(void *)
        &g_actor_pipeline_storage_71440.word_cells[6] = 0U; /* WORD 0x7144c */
    state->words[12] = 0U;            /* WORD 0x70ef0 */
    state->words[22] = 0U;            /* WORD 0x70f04 */
    state->words[27] = 0U;            /* WORD 0x70f0e */
    state->words[1] = 10U;            /* WORD 0x70eda */
    state->words[0] = 10U;            /* WORD 0x70ed8 */
    state->words[6] = 10U;            /* WORD 0x70ee4 */
    state->words[4] = 0U;             /* WORD 0x70ee0 */
    state->words[5] = 0U;             /* WORD 0x70ee2 */
    state->words[2] = 0U;             /* WORD 0x70edc */
    state->words[24] = 0U;            /* WORD 0x70f08 */
    state->words[14] = 0x66U;         /* WORD 0x70ef4 */
    state->words[20] = 0x83U;         /* WORD 0x70f00 */

    view->direction_x = 0U;            /* BYTE actor+0x94 */
    advance_frame(actor);              /* existing native 0x29270 provider */
    view->steps = g_eng1_initial_motion_05c3a0; /* pointer actor+0xf0 */
    view->step_index = 1L;             /* DWORD actor+0xf4 */
    view->active_step = (short)-1;     /* WORD actor+0x11e */
    view->action_callback_index = 0;    /* WORD actor+0x124 */
    eng1_native_code_target_002a290(actor);
    view->render_flags |= 8UL;         /* DWORD read/modify/write actor+0x54 */
}
