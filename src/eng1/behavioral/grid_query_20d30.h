#ifndef ENG1_GRID_QUERY_20D30_H
#define ENG1_GRID_QUERY_20D30_H

typedef unsigned long eng1_grid_query_u32;
typedef long eng1_grid_query_s32;

#if defined(_MSC_VER)
#define ENG1_GRID_QUERY_CDECL __cdecl
#else
#define ENG1_GRID_QUERY_CDECL
#endif

/* Measured non-overlapping prefix [RVA 0x5e0d0, 0x5e138), 26 DWORDs. */
extern eng1_grid_query_u32
    g_eng1_grid_query_coefficient_prefix_5e0d0[26];

eng1_grid_query_u32 ENG1_GRID_QUERY_CDECL grid_query_20d30(
    eng1_grid_query_s32 x, eng1_grid_query_s32 y,
    eng1_grid_query_s32 z, eng1_grid_query_u32 step);

#endif
