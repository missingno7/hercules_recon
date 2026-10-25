/* Private ordinary-C reconstruction of ENG1.DLL RVA 0xefa0. */
#include "interpreters.h"

typedef unsigned char Eng1Efa0Byte;
typedef unsigned short Eng1Efa0Word;
typedef unsigned long Eng1Efa0Dword;

typedef char eng1_efa0_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_efa0_dword_is_32[(sizeof(Eng1Efa0Dword) == 4) ? 1 : -1];

extern void __cdecl reset_frame_state(Actor *actor);

void __cdecl eng1_native_code_target_00efa0(Actor *actor)
{
    volatile Eng1Efa0Byte *bytes;
    Eng1Efa0Dword old_render_flags;

    bytes = (volatile Eng1Efa0Byte *)(void *)actor;
    old_render_flags = *(volatile Eng1Efa0Dword *)(void *)(bytes + 0x54);
    *(volatile Eng1Efa0Dword *)(void *)(bytes + 0x50) = 0x00010000UL;
    bytes[0x23] = 5U;
    *(volatile Eng1Efa0Dword *)(void *)(bytes + 0x54) =
        old_render_flags | 0x00000604UL;
    *(volatile Eng1Efa0Word *)(void *)(bytes + 0x4a) = 0;

    reset_frame_state(actor);
    (void)eng1_interpret_word_stream(actor);
}
