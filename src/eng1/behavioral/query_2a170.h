#ifndef ENG1_QUERY_2A170_H
#define ENG1_QUERY_2A170_H

typedef unsigned long query_2a170_u32;
typedef unsigned short query_2a170_u16;
typedef struct Query2a170Bounds Query2a170Bounds;
struct Query2a170Bounds {
    short mn[3];
    short mx[3];
};

extern volatile query_2a170_u32 g_eng1_spatial_match_actor_6fedc;
query_2a170_u32 __cdecl dispatch_local_2a170(
    query_2a170_u32 x, query_2a170_u32 y,
    query_2a170_u32 z, query_2a170_u32 step);

int __cdecl point_in_box_2a810(unsigned long, unsigned long, unsigned long, const Query2a170Bounds *);
#endif
