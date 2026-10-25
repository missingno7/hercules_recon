#ifndef ENG1_PLAYER_STATE_CELLS_H
#define ENG1_PLAYER_STATE_CELLS_H

/* Complete loader-zero cells established by native writers and direct access
 * widths. Names are reconstruction labels. Grouping implies neither an original
 * translation unit nor a contiguous aggregate. Actor pointee extents/lifetimes
 * remain separate evidence questions. */
struct Actor;
typedef char eng1_player_cell_pointer32[sizeof(struct Actor *) == 4 ? 1 : -1];
typedef char eng1_player_cell_signed32[sizeof(long) == 4 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
extern struct Actor *g_player_glow_707dc;
extern struct Actor *g_burst_origin_707e4;
extern long g_player_cruise_707e8;
extern long g_player_flag_707f8;
extern struct Actor *g_player_trail_707fc;
#ifdef __cplusplus
}
#endif

#endif
