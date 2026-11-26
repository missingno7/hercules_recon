/*
 * ENG1 loader-zero boss cells in the zero tail of .data, used only by the boss unit
 * (src/eng1/boss_unit_24090.c). Callers access them whole (direct [disp32] word and
 * dword accesses). This file is their single data-only definition; Actor stays incomplete.
 * Names keep the provisional address labels of src/eng1/actor_region.h.
 */
typedef struct Actor Actor;

unsigned short g_boss_script_index_71e24;
Actor *g_boss_71e28;
