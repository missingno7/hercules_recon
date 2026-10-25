/* Actual custom-image scalar fields and genuine overlapping mask/palette owner. */
#include "initial_custom_roots.h"
extern void __cdecl eng1_native_code_target_034b40(void *,int);
extern void __cdecl eng1_native_code_target_034bb0(void *,int);
Eng1CustomMaskPalette56 g_eng1_custom_mask_palette_05ebb8 = {
    4UL, { 0xaaaabbbbUL, 0x88889999UL, 0x66667777UL, 0x44445555UL, 0x00000000UL, 0xc1cb0000UL, 0xb989bdaaUL, 0xb547b968UL, 0xb125b126UL, 0xa8e4ad04UL, 0xa4c2a8e3UL, 0xa0a1a0a2UL }, 0x9c809c81UL
};
Eng1CustomImage36 g_eng1_custom_image_05ebf0 = {
    96U, 96U, 8U, 2U, 3U, 1U, 0, 0, 0, 32U, 64U, 96U, { 0U, 0U, 0U },
    eng1_native_code_target_034b40, eng1_native_code_target_034bb0,
    &g_eng1_custom_mask_palette_05ebb8.columns,
    (unsigned short *)&g_eng1_custom_mask_palette_05ebb8.mask_cells[5]
};
/* Duration and first opcode WORD only; no terminal or continued capacity. */
