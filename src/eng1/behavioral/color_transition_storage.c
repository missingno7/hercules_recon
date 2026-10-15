#include "color_transition_views.h"

/* Separate measured prefixes. No original adjacency/placement claim.
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

/* Native data58a20..58a5c; the following four bytes are not owned here. */
unsigned char g_eng1_color_list_rows_58a20[10][6] = {
    { 0, 0, 0, 96, 96, 96 },
    { 0, 0, 0, 48, 48, 48 },
    { 0, 0, 0, 32, 32, 32 },
    { 0, 0, 0, 32, 32, 32 },
    { 0, 0, 0, 255, 255, 255 },
    { 0, 0, 0, 32, 32, 32 },
    { 0, 0, 0, 255, 255, 255 },
    { 0, 0, 0, 123, 128, 128 },
    { 0, 0, 0, 32, 32, 32 },
    { 0, 0, 0, 255, 255, 255 }
};
