/* Private data providers observed as writable loader-zero storage in ENG1. */
typedef unsigned char eng1_u8;
typedef unsigned long eng1_u32;
typedef char eng1_requires_32bit_ulong[(sizeof(eng1_u32) == 4) ? 1 : -1];

eng1_u32 g_mesh_colors_6f4e8[512];
eng1_u32 g_frame_global_70cf8;
eng1_u8 g_frame_global_70cfc;
eng1_u32 g_frame_global_73488;
