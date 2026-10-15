/* Measured writable ENG1 color storage, including initialized level-6 records. */
typedef unsigned char eng1_u8;
typedef unsigned long eng1_u32;
typedef char eng1_requires_32bit_ulong[(sizeof(eng1_u32) == 4) ? 1 : -1];

eng1_u32 g_mesh_colors_6f4e8[512];
eng1_u32 g_frame_global_70cf8;
eng1_u8 g_frame_global_70cfc;
eng1_u32 g_frame_global_73488;

/* Six-byte writable native records; no adjacency or aggregate claim. */
eng1_u8 g_level6_color_target_5a7b0[6]={0,0,0,128,128,128};
eng1_u8 g_level6_color_current_5a7b8[6]={0,0,0,128,128,128};
