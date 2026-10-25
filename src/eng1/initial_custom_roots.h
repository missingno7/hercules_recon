#ifndef ENG1_INITIAL_CUSTOM_ROOTS_H
#define ENG1_INITIAL_CUSTOM_ROOTS_H
#include <stddef.h>
typedef void (__cdecl *Eng1CustomPointCallback)(void *, int);
typedef struct Eng1CustomMaskPalette56 { unsigned long columns; unsigned long mask_cells[12]; unsigned long palette_tail; } Eng1CustomMaskPalette56;
typedef struct Eng1CustomImage36 { unsigned short points, visible_points; unsigned char point_mode, divisor, mask_rows, image_mode; short x,y,z; unsigned char shade,red,green; unsigned char reserved_011[3]; Eng1CustomPointCallback initialize_point, update_point; unsigned long *mask; unsigned short *palette; } Eng1CustomImage36;
typedef char eng1_custom_sizes[(sizeof(void*)==4 && sizeof(unsigned long)==4 && sizeof(Eng1CustomMaskPalette56)==56 && sizeof(Eng1CustomImage36)==36 && offsetof(Eng1CustomImage36,initialize_point)==20 && offsetof(Eng1CustomImage36,palette)==32)?1:-1];
extern Eng1CustomMaskPalette56 g_eng1_custom_mask_palette_05ebb8;
extern Eng1CustomImage36 g_eng1_custom_image_05ebf0;
extern unsigned short g_eng1_zero_word_program_06ef18[2];
#endif
