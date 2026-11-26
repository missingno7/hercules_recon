/* Independent four-byte PC ENG1 loader-zero owners. Native setup writes the
 * glow/trail actor pointers, the burst initializer stores its actor argument,
 * and native signed arithmetic consumes cruise/flag DWORDs. No pointee storage,
 * runtime initialization policy or actor capacity is supplied by this unit.
 * See evidence/player_cells_references_2026_10_25.json. */
#include "player_state_cells.h"

struct Actor *g_player_glow_707dc;
struct Actor *g_burst_origin_707e4;
long g_player_cruise_707e8;
long g_player_flag_707f8;
struct Actor *g_player_trail_707fc;
