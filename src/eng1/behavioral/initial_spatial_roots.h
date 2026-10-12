#ifndef ENG1_INITIAL_SPATIAL_ROOTS_H
#define ENG1_INITIAL_SPATIAL_ROOTS_H
#include <stddef.h>
typedef struct Eng1SpatialType10 { short x, y, z; unsigned short flags, kind; } Eng1SpatialType10;
typedef char eng1_spatial_type_size[(sizeof(Eng1SpatialType10)==10 && offsetof(Eng1SpatialType10,kind)==8)?1:-1];
extern Eng1SpatialType10 g_eng1_spatial_types_068170[97];
extern Eng1SpatialType10 g_eng1_spatial_types_04ede8[101];
extern Eng1SpatialType10 g_eng1_spatial_types_061130[148];
extern Eng1SpatialType10 g_eng1_spatial_types_0587a0[13];
extern Eng1SpatialType10 g_eng1_spatial_types_06fe60[7];
extern Eng1SpatialType10 g_eng1_spatial_types_062bf8[82];
extern unsigned short g_eng1_collision_map_04f1e0[32];
extern unsigned short g_eng1_collision_map_058830[24];
extern unsigned short g_eng1_collision_map_059fd8[32];
extern unsigned short g_eng1_collision_map_0616f8[32];
extern unsigned short g_eng1_collision_map_0630a8[30];
extern unsigned short g_eng1_collision_map_068590[36];
extern unsigned short g_eng1_collision_map_06b118[32];
extern Eng1SpatialType10 g_eng1_spatial_identity_06ff90;
extern Eng1SpatialType10 g_eng1_spatial_identity_0702a8;
#endif
