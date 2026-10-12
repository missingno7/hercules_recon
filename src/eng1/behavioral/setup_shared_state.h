#ifndef ENG1_SETUP_SHARED_STATE_H
#define ENG1_SETUP_SHARED_STATE_H

/* Physical no-argument cdecl dispatch at native1555b. The48-cell object is
 * a measured prefix through the store to slot47, never a full-capacity claim. */
typedef void (__cdecl *Eng1SetupHandler)(void);
typedef char eng1_setup_pointer32[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_setup_ulong32[(sizeof(unsigned long) == 4) ? 1 : -1];
extern Eng1SetupHandler g_eng1_setup_handlers_72040[48];

/* Missing entries: declarations only. Integrator must supply real functions
 * or the centralized explicit-incomplete provider, never original addresses. */
extern void __cdecl eng1_native_code_target_0016840(void);
extern void __cdecl eng1_native_code_target_0016b40(void);
extern void __cdecl eng1_native_code_target_00171b0(void);
extern void __cdecl eng1_native_code_target_0016da0(void);
extern void __cdecl eng1_native_code_target_0016f40(void);
extern void __cdecl eng1_native_code_target_0017400(void);
extern void __cdecl eng1_native_code_target_00177a0(void);
extern void __cdecl eng1_native_code_target_00178f0(void);
extern void __cdecl dispatch_local_19440(void);
extern void __cdecl eng1_native_code_target_0017a90(void);
extern void __cdecl eng1_native_code_target_0018240(void);
extern void __cdecl eng1_native_code_target_0019260(void);
extern void __cdecl eng1_native_code_target_0017cd0(void);
extern void __cdecl eng1_native_code_target_0018a10(void);
extern void __cdecl eng1_native_code_target_0018ed0(void);
extern void __cdecl eng1_native_code_target_00193f0(void);
extern void __cdecl eng1_native_code_target_0018ba0(void);
extern void __cdecl eng1_native_code_target_0019140(void);
extern void __cdecl eng1_native_code_target_0019290(void);
extern void __cdecl eng1_native_code_target_00191e0(void);
extern void __cdecl eng1_native_code_target_0019480(void);
extern void __cdecl eng1_native_code_target_0017eb0(void);
extern void __cdecl eng1_native_code_target_0019c20(void);
extern void __cdecl eng1_native_code_target_00379e0(void);
extern void __cdecl eng1_native_code_target_0017c30(void);
extern void __cdecl eng1_native_code_target_0017c70(void);
extern void __cdecl eng1_native_code_target_00182c0(void);
extern void __cdecl eng1_native_code_target_0018550(void);
extern void __cdecl eng1_native_code_target_0018770(void);
extern void __cdecl eng1_native_code_target_0018830(void);
extern void __cdecl eng1_native_code_target_0018c30(void);

extern unsigned long g_eng1_setup_dword_72008;
extern unsigned char g_eng1_setup_byte_72010, g_eng1_setup_byte_72011;
extern unsigned short g_eng1_setup_word_72012;
extern unsigned long g_eng1_setup_dword_72014, g_eng1_setup_dword_7201c;
extern unsigned char *g_eng1_setup_pointer_72018, *g_eng1_setup_pointer_72020;
extern unsigned long g_eng1_setup_dword_72114;
extern unsigned long g_eng1_setup_dword_72118, g_eng1_setup_dword_7211c;
extern unsigned long g_eng1_setup_dword_72120, g_eng1_setup_dword_7216c;
extern unsigned long g_eng1_setup_dword_72180;
extern unsigned short g_eng1_setup_word_72158, g_eng1_setup_word_7216a;
extern unsigned char g_eng1_setup_byte_7215c;
extern unsigned char *g_eng1_setup_pointer_72164;
extern unsigned short g_eng1_setup_word_6ef74, g_eng1_setup_word_6ef78;
extern unsigned short g_eng1_setup_word_6ef7c, g_eng1_setup_word_6ef80;
extern unsigned short g_eng1_setup_word_6ef84;
extern unsigned short g_eng1_setup_word_6ef8c, g_eng1_setup_word_6ef90;
extern unsigned short g_eng1_setup_word_6ef94, g_eng1_setup_word_6ef98;
/* Native15d00 reads all four bytes at6ef9c, then writes only the low WORD.
 * One owner preserves the two upper bytes; next6efa0 starts at exclusive end.
 * This union supplies physical views, not an original declaration claim. */
typedef union Eng1SetupCell6ef9c {
    unsigned long dword;
    unsigned short words[2];
} Eng1SetupCell6ef9c;
typedef char eng1_setup_cell_6ef9c_is_four[(sizeof(Eng1SetupCell6ef9c) == 4) ? 1 : -1];
extern Eng1SetupCell6ef9c g_eng1_setup_cell_6ef9c;
#define g_eng1_setup_word_6ef9c (g_eng1_setup_cell_6ef9c.words[0])
#define g_eng1_setup_dword_6ef9c (g_eng1_setup_cell_6ef9c.dword)
extern unsigned long g_eng1_setup_dword_6ef88, g_eng1_setup_dword_6efa0;
extern unsigned long g_eng1_setup_dword_6efa8, g_eng1_setup_dword_6efac;
extern unsigned char g_eng1_setup_byte_6efb4;

/* Writable native .data. Only the nine measured four-byte rows0..8 are supplied.
 * Consumers read the low byte with stride4. Other indices need an explicit
 * incomplete-prefix guard; signed indexing supplies no capacity proof. */
extern unsigned char g_eng1_setup_categories_55278[9][4];

/* Adjacent setup leaf11fc0: exact six-DWORD clear prefix, independent cells. */
extern unsigned long g_eng1_setup_clear_72140[6];
extern unsigned long g_eng1_setup_dword_6ef5c, g_eng1_setup_dword_72188;
extern unsigned short g_eng1_setup_word_72194, g_eng1_setup_word_7219c;
extern unsigned short g_eng1_setup_word_7219e, g_eng1_setup_word_721a0;
extern unsigned short g_eng1_setup_word_721a2, g_eng1_setup_word_721a4;
extern unsigned short g_eng1_setup_word_721a6;
/* Existing viewport_state_data.c28-byte prefix, never three new owners. */
extern unsigned char g_cold_start_state_bytes_70820[0x1c];
#define g_eng1_setup_word_70820 (*(unsigned short *)(void *)(g_cold_start_state_bytes_70820 + 0))
#define g_eng1_setup_word_70822 (*(unsigned short *)(void *)(g_cold_start_state_bytes_70820 + 2))
#define g_eng1_setup_word_7082e (*(unsigned short *)(void *)(g_cold_start_state_bytes_70820 + 14))

/* Reuse all established providers below; this header owns no duplicate cells. */
extern unsigned char *g_collision_context;
extern unsigned long eng1_sim_state_72104, eng1_sim_state_72174;
#define g_eng1_setup_dword_72104 eng1_sim_state_72104
#define g_eng1_setup_dword_72174 eng1_sim_state_72174
#include "resource_runtime_data.h"
#define g_eng1_setup_byte_70cfe g_active_category_70cfe
extern unsigned long g_resource_transition_ticks_6efb0;
extern unsigned long eng1_sim_state_72110, eng1_sim_state_72170;
extern unsigned short eng1_sim_state_72108, eng1_sim_state_6feb4;
extern long g_camera_x_70ca0, g_camera_y_70ca4, g_camera_z_70ca8;
extern long g_camera_center_x_70cdc, g_camera_center_y_70ce0, g_camera_focal_70ce4;
extern unsigned char g_camera_iterations_70cfd;
extern short g_camera_adjust_x_70d02, g_camera_adjust_y_70d04;
extern short g_camera_depth_low_70d0c, g_camera_depth_high_70d0e;
extern unsigned short g_camera_flags_70d10;
extern long g_camera_z_base_70d1c;

#include "actor_pipeline_storage.h"
#define g_eng1_setup_dword_717b4 (g_actor_pipeline_storage_71440.dword_cells[0x374 / 4])
#define g_eng1_setup_word_7183c (g_actor_pipeline_storage_71440.word_cells[0x3fc / 2])
#define g_eng1_setup_word_71848 (g_actor_pipeline_storage_71440.word_cells[0x408 / 2])
#define g_eng1_setup_word_718e0 (g_actor_pipeline_storage_71440.word_cells[0x4a0 / 2])
#define g_eng1_setup_word_71a12 (g_actor_pipeline_storage_71440.word_cells[0x5d2 / 2])
#define g_eng1_setup_word_71a14 (g_actor_pipeline_storage_71440.word_cells[0x5d4 / 2])

#endif
