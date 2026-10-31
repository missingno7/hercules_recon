/* ENG1 scene-state functions at 0x1abf0 and 0x1ac10 (0x1ae00, the next native function of
 * this gap, is not reconstructed to equality yet and is left out).
 * Their native behavior covers level setup, camera/color state and a staged
 * actor sequence. The adjacent sound and alternate-pool files do not own this
 * group. Translation-unit ownership remains unproved; this is a new unit.
 */
#include "setup_shared_state.h"
#include "actor_pipeline_storage_views.h"
#include "initial_spatial_roots.h"
#include "../shared/engine_interface.h"
#include <stdlib.h>
#include <stddef.h>

typedef struct SceneActor SceneActor;
struct SceneActor {
    long x, y, z;
    unsigned char unknown_00c[0x22 - 0x0c];
    unsigned char category, mode;
    unsigned char unknown_024[0x2e - 0x24];
    unsigned short kind;
    unsigned char unknown_030[4];
    unsigned short sprite;
    unsigned short entry_count;
    unsigned short profile;
    unsigned short scale;
    unsigned char unknown_03c[0x10c - 0x3c];
    long sort_value_10c;
};
typedef struct SceneCallback12 {
    void (__cdecl *initialize)(SceneActor *);
    void *update;
    void *draw;
} SceneCallback12;
typedef struct SceneCallback8 {
    void (__cdecl *initialize)(SceneActor *);
    void *update;
} SceneCallback8;
typedef char scene_actor_sort_offset[(offsetof(SceneActor, sort_value_10c)==0x10c)?1:-1];
typedef char scene_callback12_size[(sizeof(SceneCallback12)==12)?1:-1];
typedef char scene_callback8_size[(sizeof(SceneCallback8)==8)?1:-1];

/* Address-based aliases for the scene cells touched by these callbacks. */
extern long g_scene_state_6f3e0;       /* ENG1 RVA 0x6f3e0 */
extern long g_scene_wait_6f458;        /* ENG1 RVA 0x6f458 */
extern long g_scene_countdown_6f45c;   /* ENG1 RVA 0x6f45c */
extern long g_scene_flag_6f460;        /* ENG1 RVA 0x6f460 */
extern long g_scene_flag_6f464;        /* ENG1 RVA 0x6f464 */
extern long g_scene_flag_6f468;        /* ENG1 RVA 0x6f468 */
extern unsigned char *g_scene_current_palette_6f3c8; /* ENG1 RVA 0x6f3c8 */
extern unsigned char *g_scene_palette_6f3cc; /* ENG1 RVA 0x6f3cc */
extern long g_scene_cell_6f3d0;        /* ENG1 RVA 0x6f3d0 */
extern long g_scene_cell_6f3d4;        /* ENG1 RVA 0x6f3d4 */
extern long g_scene_cell_6f3e4;        /* ENG1 RVA 0x6f3e4 */
extern long g_scene_cell_6f3f8;        /* ENG1 RVA 0x6f3f8 */
extern long g_scene_cell_6f444;        /* ENG1 RVA 0x6f444 */
extern long g_scene_cell_6f448;        /* ENG1 RVA 0x6f448 */
extern long g_scene_cell_6f44c;        /* ENG1 RVA 0x6f44c */
extern long g_scene_cell_6f450;        /* ENG1 RVA 0x6f450 */
extern long g_scene_cell_6f454;        /* ENG1 RVA 0x6f454 */
extern long g_scene_cell_6efb8;        /* ENG1 RVA 0x6efb8 */
extern SceneActor *g_scene_actor_6efbc; /* ENG1 RVA 0x6efbc */
extern long g_scene_cell_6ffbc;        /* ENG1 RVA 0x6ffbc */
extern long g_scene_cell_6f180;        /* ENG1 RVA 0x6f180 */
extern void *g_scene_state_data_6f3f4; /* ENG1 RVA 0x6f3f4 */

extern unsigned char g_level6_color_target_5a7b0[6];
extern unsigned char g_level6_color_current_5a7b8[6];
extern unsigned char g_level6_color_mode0_56048[];
extern unsigned char g_level6_color_mode1_560e0[];
extern unsigned char g_level6_color_mode2_56168[];
extern SceneActor *g_normal_actor_pool;
extern SceneCallback12 *g_configuration_dword_71370;
extern SceneCallback8 *g_configuration_dword_7139c;
extern void __cdecl play_sound(int, SceneActor *);
extern SceneActor *__cdecl spawn_normal(int, int, int, int);
extern SceneActor *__cdecl spawn_kind(long, long, long, int);

extern int __cdecl initialize_command_table(unsigned long, unsigned int,
                                             unsigned long);
extern void __cdecl eng1_native_code_target_001b9a0(void);
extern void __cdecl eng1_native_code_target_001c3f0(int);
extern void *__cdecl title_17ad0(int, int, int, int, int);
extern void __cdecl eng1_color_grid_update_5a60(int, int, int, int, int, int);

extern void __cdecl eng1_native_code_target_001bab0(void *);
extern void __cdecl eng1_native_code_target_001e140(void *);
extern void __cdecl eng1_native_code_target_001d6c0(SceneActor *);
extern void __cdecl eng1_native_code_target_0019240(void);
extern void __cdecl eng1_native_code_target_001b1d0(int);
extern void __cdecl eng1_native_code_target_002c4c0(int);
extern void __cdecl eng1_empty_22180(void);

void __cdecl eng1_native_code_target_01abf0(void)
{
    (void)title_17ad0(0, 0, 0, 0x2c, 0);
}

void __cdecl eng1_native_code_target_001ac10(void)
{
    signed char mode;
    SceneActor *actor;

    g_scene_countdown_6f45c = 0;
    g_scene_wait_6f458 = 0;
    g_scene_cell_6f450 = 0;
    g_scene_cell_6f3d0 = 0;
    g_scene_cell_6f3e4 = 0;
    g_scene_state_6f3e0 = 0;
    g_scene_cell_6f454 = 0;
    g_scene_cell_6f448 = 0;
    g_scene_cell_6f444 = 0;
    g_scene_cell_6f3f8 = 0;
    g_scene_cell_6efb8 = 8;
    g_scene_cell_6f3d4 = 0;
    g_scene_cell_6ffbc = 0;
    g_camera_center_y_70ce0 = 0xa0;
    g_camera_adjust_y_70d04 = (short)0xffa0U;
    g_camera_adjust_x_70d02 = 0x20;
    g_camera_z_base_70d1c = 0;
    g_camera_iterations_70cfd = 2;
    g_camera_flags_70d10 = 0x1f;
    g_camera_depth_low_70d0c = 0;
    g_camera_depth_high_70d0e = 0xe0;
    g_scene_cell_6f44c = 0;

    (void)initialize_command_table(0x14UL, 0x3781U, 0x0eUL);
    eng1_native_code_target_001b9a0();
    (void)title_17ad0(0, 0, 0, 0x202c, 0);
    (void)title_17ad0(0, 0x03e00000, 0x02800000, 0x202a, 0);
    eng1_native_code_target_001c3f0(0);
    g_level6_color_pending_6fea0 = (unsigned char *)0;

    g_level6_color_current_5a7b8[0] = 0;
    g_level6_color_target_5a7b0[0] = 0;
    g_level6_color_current_5a7b8[2] = 0;
    g_level6_color_target_5a7b0[2] = 0;
    g_level6_color_current_5a7b8[1] = 0;
    g_level6_color_target_5a7b0[1] = 0;
    g_level6_color_current_5a7b8[3] = 0x80;
    g_level6_color_target_5a7b0[3] = 0x80;
    g_level6_color_current_5a7b8[5] = 0x80;
    g_level6_color_target_5a7b0[5] = 0x80;
    g_level6_color_current_5a7b8[4] = 0x80;
    g_level6_color_target_5a7b0[4] = 0x80;

    (void)eng1_color_grid_update_5a60(0, 0, 0, 0x80, 0x80, 0x80);
    play_sound(0x180, (SceneActor *)0);
    play_sound(0x193, (SceneActor *)0);
    g_eng1_setup_dword_72188 = 5;

    mode = (signed char)g_engine_interface.context_004->unknown_000[0x0e];
    switch (mode) {
    case 0:
        g_scene_palette_6f3cc = g_level6_color_mode0_56048;
        break;
    case 1:
        g_scene_palette_6f3cc = g_level6_color_mode1_560e0;
        break;
    case 2:
        g_scene_palette_6f3cc = g_level6_color_mode2_56168;
        break;
    }
    g_scene_current_palette_6f3c8 = g_scene_palette_6f3cc;

    actor = spawn_normal(0, 0, 0, 0x34);
    if (actor != 0)
        g_configuration_dword_71370[0x34].initialize(actor);
    actor = spawn_normal(0, 0, 0, 0x35);
    if (actor != 0)
        g_configuration_dword_71370[0x35].initialize(actor);
}
