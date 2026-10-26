/* Exact-size loader-zero scalar owners for the ENG1 cells with no existing
 * canonical source definition. Types match the existing caller declarations. */
typedef char c4_eng1_pointer_width[(sizeof(void *) == 4) ? 1 : -1];
typedef char c4_eng1_long_width[(sizeof(long) == 4) ? 1 : -1];

void *g_72874;
long g_player_flag_707f4;
void *g_player_palette_707e0;
