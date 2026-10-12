#ifndef ENG1_ACTOR_CONTROL_CELLS_H
#define ENG1_ACTOR_CONTROL_CELLS_H

/* Independent native loader-zero cells. Grouping does not imply adjacency,
 * historical translation-unit ownership, or a safe collision-pointer lifetime. */
typedef char eng1_control_cells_pointer32[sizeof(void *) == 4 ? 1 : -1];
typedef char eng1_control_cells_dword32[sizeof(unsigned long) == 4 ? 1 : -1];
extern unsigned char *g_collision_context;          /* native 0x7217c */
extern unsigned long g_eng1_setup_dword_6efa4;     /* native 0x6efa4 */

#endif
