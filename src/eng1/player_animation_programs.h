#ifndef ENG1_PLAYER_ANIMATION_PROGRAMS_H
#define ENG1_PLAYER_ANIMATION_PROGRAMS_H
#include "flags_word_programs.h"

/* Fixed-stride signed pairs and duration-headed WORD loops. These are complete
 * programs for the separately reviewed restart-only consumer profiles, not
 * original C declaration or general animation-capacity claims. */
typedef struct Eng1PlayerAnimationPair {
    short command;
    short ticks;
} Eng1PlayerAnimationPair;
typedef char eng1_player_animation_pair4[sizeof(Eng1PlayerAnimationPair)==4?1:-1];
extern Eng1PlayerAnimationPair g_player_program_633e8[9];
extern Eng1ProgramWord g_player_program_634b0[19];
extern Eng1ProgramWord g_player_program_634e0[9];
#endif
