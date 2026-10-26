/* Native ENG1 frame callbacks. ABI, reference identities and proof limits:
 * evidence/frame_callbacks_recovery_2026_10_26.json. Actor stays opaque so
 * declarations follow the selected providers instead of a stale region header. */
typedef struct Actor Actor;
typedef char frame_callback_uint_is_32[(sizeof(unsigned int) == 4) ? 1 : -1];
typedef char frame_callback_word_is_16[(sizeof(unsigned short) == 2) ? 1 : -1];
typedef char frame_callback_pointer_is_32[(sizeof(Actor *) == 4) ? 1 : -1];

extern void __cdecl reset_frame_state(Actor *actor);
extern int __cdecl eng1_interpret_word_stream(Actor *actor);
extern int __cdecl remove_actor(Actor *actor);
extern int __cdecl release_actor(Actor *actor);
extern int __cdecl eng1_2a9e0(Actor *actor, int margin);

void __cdecl eng1_native_code_target_014fb0(Actor *actor)
{
    *(unsigned int *)((unsigned char *)actor + 0x50) = 0x802;
    *(unsigned char *)((unsigned char *)actor + 0x22) = 5;
    *(unsigned int *)((unsigned char *)actor + 0x54) |= 5;
    *(unsigned short *)((unsigned char *)actor + 0x4a) = 0x80;
    *(unsigned int *)((unsigned char *)actor + 0x04) += 0xc0000;
    reset_frame_state(actor);
    eng1_interpret_word_stream(actor);
}

void __cdecl eng1_native_code_target_015000(Actor *actor)
{
    if (eng1_2a9e0(actor, 0x140))
        remove_actor(actor);
    eng1_interpret_word_stream(actor);
}

void __cdecl eng1_native_code_target_015030(Actor *actor)
{
    *(unsigned int *)((unsigned char *)actor + 0x50) = 0x802;
    *(unsigned char *)((unsigned char *)actor + 0x22) = 2;
    reset_frame_state(actor);
    eng1_interpret_word_stream(actor);
}

void __cdecl eng1_native_code_target_015060(Actor *actor)
{
    if (eng1_2a9e0(actor, 0x140))
        release_actor(actor);
    eng1_interpret_word_stream(actor);
}
