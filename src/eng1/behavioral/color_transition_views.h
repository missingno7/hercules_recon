#ifndef HERCULES_ENG1_COLOR_TRANSITION_VIEWS_H
#define HERCULES_ENG1_COLOR_TRANSITION_VIEWS_H

/* Measured views, not original declarations or translation-unit ownership. */
typedef unsigned char color_u8;
typedef unsigned short color_u16;
typedef unsigned long color_u32;

typedef struct Eng1ColorTransitionPair8 {
    color_u32 current;
    color_u32 target;
} Eng1ColorTransitionPair8;

typedef struct Eng1ColorTransitionState32 {
    color_u32 previous_selection;
    color_u32 previous_current;
    color_u32 previous_target;
    color_u32 current_target;
    color_u32 interpolation;
    color_u32 selection;
    color_u32 current_value;
    color_u32 target_value;
} Eng1ColorTransitionState32;

typedef char color_pair_extent_is_measured[
    (sizeof(Eng1ColorTransitionPair8) == 8) ? 1 : -1];
typedef char color_state_extent_is_measured[
    (sizeof(Eng1ColorTransitionState32) == 32) ? 1 : -1];

extern Eng1ColorTransitionPair8 g_eng1_color_transition_pairs_589b8[8];
extern Eng1ColorTransitionState32 g_eng1_color_transition_state_6f4a8;
extern color_u32 g_eng1_color_transition_timer_71ecc;
/* Ten six-byte rows reached by the observed 9/4/decrement producer states. */
extern unsigned char g_eng1_color_list_rows_58a20[10][6];
extern void __cdecl eng1_native_code_target_01ebf0(void *object);
extern void __cdecl eng1_native_code_target_01eba0(void *object);

#endif
