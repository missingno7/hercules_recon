typedef char movie_ulong_is_4[(sizeof(unsigned long) == 4) ? 1 : -1];
typedef char movie_uchar_is_1[(sizeof(unsigned char) == 1) ? 1 : -1];

unsigned long g_exe_4a3810 = 0;
unsigned long g_exe_4a3814 = 0;
unsigned char g_exe_48f810[81920] = {0};
unsigned long g_exe_47f750[47] = {0};

typedef char movie_width_cell_is_4[(sizeof(g_exe_4a3810) == 4) ? 1 : -1];
typedef char movie_height_cell_is_4[(sizeof(g_exe_4a3814) == 4) ? 1 : -1];
typedef char movie_history_prefix_is_81920[(sizeof(g_exe_48f810) == 81920) ? 1 : -1];
typedef char movie_noise_prefix_is_188[(sizeof(g_exe_47f750) == 188) ? 1 : -1];
