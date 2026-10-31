/*
 * ENG1 loader-zero level runtime cells in the zero tail of .data: the effect level
 * read by the level frame (src/eng1/level_frame_01e50.c) and the sound-effect buffer
 * pointer of src/eng1/level_actors_0c730.c. Both are accessed whole only. This file is
 * their single data-only definition; names keep the provisional address labels.
 */
long g_effect_level_6ee64;
void *g_sfx_buffer_6ef34;
