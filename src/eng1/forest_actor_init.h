#ifndef ENG1_FOREST_ACTOR_INIT_H
#define ENG1_FOREST_ACTOR_INIT_H
#include <stddef.h>
typedef struct Eng1ForestRecord20 {
    long x, y, z;
    unsigned short flags, state, kind, untouched_012;
} Eng1ForestRecord20;
typedef struct Eng1ForestPlacement8 {
    unsigned short x, y, z, spatial_index;
} Eng1ForestPlacement8;
typedef char eng1_forest_record_layout[
    (sizeof(Eng1ForestRecord20)==20 && offsetof(Eng1ForestRecord20,flags)==12 &&
     offsetof(Eng1ForestRecord20,kind)==16 && sizeof(Eng1ForestPlacement8)==8)?1:-1];
void __cdecl eng1_call_26040(void);
void __cdecl eng1_call_26480(void);
#endif
