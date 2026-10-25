#ifndef ENG1_ACTOR_CONTROL_STATE_H
#define ENG1_ACTOR_CONTROL_STATE_H

/* One backing view for the measured loader-zero interval [70ed8,70f10).
 * WORD and DWORD accesses share these bytes. Original type and wider logical
 * extent are unknown; this does not identify an original C structure. */
typedef union Eng1ActorControlState70ed8 {
    unsigned char bytes[0x38];
    unsigned short words[28];
    unsigned long dwords[14];
} Eng1ActorControlState70ed8;
typedef char eng1_actor_control_state_size_is_56[
    sizeof(Eng1ActorControlState70ed8) == 0x38 ? 1 : -1];
typedef char eng1_actor_control_word_size_is_2[sizeof(unsigned short) == 2 ? 1 : -1];
typedef char eng1_actor_control_dword_size_is_4[sizeof(unsigned long) == 4 ? 1 : -1];
extern Eng1ActorControlState70ed8 g_eng1_actor_control_state_70ed8;

/* Previously recovered standalone WORD, now the same exact interior view. */
#define g_eng1_input_word_70f0c (g_eng1_actor_control_state_70ed8.words[26])

#endif
