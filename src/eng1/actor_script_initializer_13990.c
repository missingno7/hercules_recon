/* Ordinary-C reconstruction of ENG1.DLL RVA 0x13990.
 * Actor field views and data dependencies remain separate source owners. */
#include "../shared/engine_interface.h"
#include "interpreters.h"

typedef unsigned char Eng1Call13990Byte;
typedef unsigned short Eng1Call13990Word;
typedef unsigned long Eng1Call13990Dword;

typedef char eng1_call_13990_dword_is_32[(sizeof(Eng1Call13990Dword) == 4) ? 1 : -1];
typedef char eng1_call_13990_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_call_13990_kind_at_2e[(offsetof(Actor, kind) == 0x2e) ? 1 : -1];
typedef char eng1_call_13990_interpolation_at_4c[
    (offsetof(Actor, interpolation_position) == 0x4c) ? 1 : -1];
typedef char eng1_call_13990_flags_at_50[(offsetof(Actor, flags) == 0x50) ? 1 : -1];
typedef char eng1_call_13990_program_at_64[
    (offsetof(Actor, image_or_program) == 0x64) ? 1 : -1];

extern Eng1Call13990Word g_eng1_config_word_prefix_53370[44];
extern Eng1Call13990Word g_eng1_config_word_pool_533c8[282];
extern void __cdecl reset_frame_state(Actor *actor);
extern void __cdecl eng1_native_code_target_002a290(Actor *actor);

void __cdecl eng1_native_code_target_013990(Actor *actor)
{
    volatile Actor *live_actor = actor;
    ResourceCallbackContext * volatile context_owner;
    volatile Eng1Call13990Byte *context_bytes;
    Eng1Call13990Word kind;
    Eng1Call13990Byte selector;

    kind = live_actor->kind;
    live_actor->flags = 0x00000802UL;

    /* Native 0x10070f24 is the same copied EngineInterface.context_004 cell. */
    context_owner = *(ResourceCallbackContext * volatile *)(void *)
        &g_engine_interface.context_004;
    context_bytes = (volatile Eng1Call13990Byte *)(void *)context_owner;
    selector = (Eng1Call13990Byte)(((Eng1Call13990Dword)kind + 6UL) & 0x0fUL);

    if (selector == context_bytes[2])
        live_actor->image_or_program =
            (void *)&g_eng1_config_word_prefix_53370[0];
    else
        live_actor->image_or_program =
            (void *)&g_eng1_config_word_pool_533c8[0];

    live_actor->interpolation_position = 1L;
    reset_frame_state(actor);
    (void)eng1_interpret_word_stream(actor);
    eng1_native_code_target_002a290(actor);
}
