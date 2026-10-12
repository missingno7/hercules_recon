/* Private ordinary-C reconstruction of ENG1.DLL RVA 0x3e220. */
#include "interpreters.h"
#include "read_guard.h"
typedef char attachment_callback_word_is_two[(sizeof(unsigned short)==2)?1:-1];
typedef char attachment_callback_dword_is_four[(sizeof(unsigned long)==4)?1:-1];

extern void *__cdecl attach_actor(void *actor, int kind);

void __cdecl eng1_native_code_target_03e220(Actor *actor)
{
    volatile unsigned char *bytes = (volatile unsigned char *)actor;
    unsigned short saved_state;

    eng1_guarded_advance_frame(actor);
    eng1_guarded_select_frame(actor, 13);

    /* Native reads this WORD after both frame helpers have returned. */
    saved_state = *(volatile unsigned short *)(void *)(bytes + 0x2a);

    *(volatile unsigned long *)(void *)(bytes + 0x50) = 0x00000a02UL;
    *(volatile unsigned short *)(void *)(bytes + 0x124) = 0;
    *(volatile unsigned short *)(void *)(bytes + 0xee) = 3;
    *(volatile unsigned long *)(void *)(bytes + 0xdc) =
        ((unsigned long)saved_state) << 16;
    *(volatile unsigned short *)(void *)(bytes + 0x2a) = 0;

    (void)attach_actor(actor, 0x2022);
}
