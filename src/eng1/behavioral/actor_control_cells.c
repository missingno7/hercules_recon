#include "actor_control_cells.h"

/* Same shared pointer formerly defined in runtime_storage.c. Normal setup
 * supplies the actor address; there is no fabricated default pointee. */
unsigned char *g_collision_context;

/* Four-byte countdown gate; all five native writer bodies remain incomplete. */
unsigned long g_eng1_setup_dword_6efa4;
