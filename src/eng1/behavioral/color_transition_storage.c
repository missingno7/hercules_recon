#include "color_transition_views.h"

/* Three separate measured prefixes. No original adjacency/placement claim.
 * The pointer at 0x71ec8 remains owned by frame_data.c. */
Eng1ColorTransitionPair8 g_eng1_color_transition_pairs_589b8[8] = {
    { 100UL, 100UL },
    { 90UL, 125UL },
    { 80UL, 150UL },
    { 70UL, 175UL },
    { 60UL, 200UL },
    { 50UL, 250UL },
    { 50UL, 250UL },
    { 50UL, 250UL }
};
Eng1ColorTransitionState32 g_eng1_color_transition_state_6f4a8;
color_u32 g_eng1_color_transition_timer_71ecc;
