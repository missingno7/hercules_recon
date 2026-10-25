/* Ordinary-C reconstruction of ENG1.DLL RVA 0x2d9b0. */
#include "actor_control_update.h"

typedef unsigned char Eng1D9b0Byte;
typedef unsigned long Eng1D9b0Dword;

typedef char eng1_d9b0_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_d9b0_dword_is_32[(sizeof(Eng1D9b0Dword) == 4) ? 1 : -1];

void __cdecl eng1_native_code_target_02d9b0(Actor *actor)
{
    volatile Eng1D9b0Byte *bytes;
    Eng1D9b0Dword original_x;
    Eng1D9b0Byte classifier_result;

    bytes = (volatile Eng1D9b0Byte *)(void *)actor;
    original_x = *(volatile Eng1D9b0Dword *)(void *)(bytes + 0x00);
    *(volatile Eng1D9b0Dword *)(void *)(bytes + 0x04) = 0x03200000UL;
    *(volatile Eng1D9b0Dword *)(void *)(bytes + 0x08) = 0x01000000UL;
    classifier_result = eng1_native_code_target_0041fd0(original_x);
    bytes[0x21] = classifier_result;
}
