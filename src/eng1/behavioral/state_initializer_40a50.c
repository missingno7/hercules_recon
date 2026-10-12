/* Ordinary-C reconstruction of ENG1.DLL RVA 0x40a50. */
#include "interpreters.h"

typedef unsigned char Eng1A50Byte;
typedef unsigned short Eng1A50Word;
typedef unsigned long Eng1A50Dword;

typedef char eng1_a50_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_a50_dword_is_32[(sizeof(Eng1A50Dword) == 4) ? 1 : -1];
typedef char eng1_a50_word_is_16[(sizeof(Eng1A50Word) == 2) ? 1 : -1];

void __cdecl eng1_native_code_target_040a50(Actor *actor)
{
    volatile Eng1A50Byte *bytes;
    Eng1A50Dword render_flags;
    Eng1A50Word control_word;
    Eng1A50Word step;
    Eng1A50Dword magnitude;

    bytes = (volatile Eng1A50Byte *)(void *)actor;
    render_flags = *(volatile Eng1A50Dword *)(void *)(bytes + 0x54);
    *(volatile Eng1A50Dword *)(void *)(bytes + 0x54) =
        render_flags | 0x10004000UL;

    control_word = *(volatile Eng1A50Word *)(void *)(bytes + 0x2a);
    magnitude = (Eng1A50Dword)(control_word & 0x001fU);
    step = (control_word & 0x0800U) != 0U ? 0x0300U : 0x0100U;

    if ((control_word & 0x0100U) != 0U) {
        *(volatile Eng1A50Word *)(void *)(bytes + 0x74) = step;
        *(volatile Eng1A50Dword *)(void *)(bytes + 0xd8) = magnitude;
    }
    if ((control_word & 0x0200U) != 0U)
        *(volatile Eng1A50Dword *)(void *)(bytes + 0xdc) = magnitude;
    if ((control_word & 0x0400U) != 0U) {
        *(volatile Eng1A50Word *)(void *)(bytes + 0x70) = step;
        *(volatile Eng1A50Dword *)(void *)(bytes + 0xdc) = magnitude;
    }
}
