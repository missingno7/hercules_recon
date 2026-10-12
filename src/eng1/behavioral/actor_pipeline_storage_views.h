#ifndef ENG1_ACTOR_PIPELINE_STORAGE_VIEWS_H
#define ENG1_ACTOR_PIPELINE_STORAGE_VIEWS_H
#include "actor_pipeline_storage.h"

/* Include after the old address-labelled extern declarations. The macros
 * below rebind function references to one aggregate storage owner. */
#define g_pipeline_actor_slots_71440 \
    ((unsigned char *)&g_actor_pipeline_storage_71440.byte_cells[0x000])
#define g_actor_worklist_71460 \
    ((unsigned long *)&g_actor_pipeline_storage_71440.dword_cells[0x020 / 4])
#define g_actor_worklist_714c0 \
    ((unsigned long *)&g_actor_pipeline_storage_71440.dword_cells[0x080 / 4])
#define g_actor_worklist_71580 \
    ((unsigned long *)&g_actor_pipeline_storage_71440.dword_cells[0x140 / 4])
#define g_actor_worklist_716c0 \
    ((unsigned long *)&g_actor_pipeline_storage_71440.dword_cells[0x280 / 4])
#define g_actor_worklist_71760 \
    ((unsigned long *)&g_actor_pipeline_storage_71440.dword_cells[0x320 / 4])
#define g_actor_worklist_717c0 \
    ((unsigned long *)&g_actor_pipeline_storage_71440.dword_cells[0x380 / 4])
#define g_actor_worklist_71900 \
    ((unsigned long *)&g_actor_pipeline_storage_71440.dword_cells[0x4c0 / 4])
#define g_resource_state_snapshot_719c0 \
    ((unsigned char *)&g_actor_pipeline_storage_71440.byte_cells[0x580])
#define g_actor_worklist_71a20 \
    ((unsigned long *)&g_actor_pipeline_storage_71440.dword_cells[0x5e0 / 4])
#define g_actor_worklist_71b20 \
    ((unsigned long *)&g_actor_pipeline_storage_71440.dword_cells[0x6e0 / 4])
#define g_actor_worklist_71be0 \
    ((unsigned long *)&g_actor_pipeline_storage_71440.dword_cells[0x7a0 / 4])
#define g_actor_worklist_71ce0 \
    ((unsigned long *)&g_actor_pipeline_storage_71440.dword_cells[0x8a0 / 4])

#define g_pipeline_dword_71680 \
    (g_actor_pipeline_storage_71440.dword_cells[0x240 / 4])
#define g_pipeline_dword_716a8 \
    (g_actor_pipeline_storage_71440.dword_cells[0x268 / 4])
#define g_pipeline_dword_71718 \
    (g_actor_pipeline_storage_71440.dword_cells[0x2d8 / 4])
#define g_pipeline_dword_71840 \
    (g_actor_pipeline_storage_71440.dword_cells[0x400 / 4])
#define g_pipeline_dword_71844 \
    (g_actor_pipeline_storage_71440.dword_cells[0x404 / 4])
#define g_configuration_selected_71860 \
    (g_actor_pipeline_storage_71440.byte_cells[0x420])
#define g_pipeline_dword_71a18 \
    (g_actor_pipeline_storage_71440.dword_cells[0x5d8 / 4])

#define g_pipeline_word_71720 \
    (g_actor_pipeline_storage_71440.word_cells[0x2e0 / 2])
#define g_pipeline_word_71740 \
    (g_actor_pipeline_storage_71440.word_cells[0x300 / 2])
#define g_pipeline_word_71bc4 \
    (g_actor_pipeline_storage_71440.word_cells[0x784 / 2])
#define g_pipeline_word_71c5c \
    (g_actor_pipeline_storage_71440.word_cells[0x81c / 2])
#define g_pipeline_word_71c60 \
    (g_actor_pipeline_storage_71440.word_cells[0x820 / 2])
#define g_pipeline_word_71ca0 \
    (g_actor_pipeline_storage_71440.word_cells[0x860 / 2])
#define g_pipeline_word_71cc0 \
    (g_actor_pipeline_storage_71440.word_cells[0x880 / 2])

/* These two names bind to established camera storage, never duplicate cells. */
#define g_pipeline_view_origin_70ca0 g_camera_x_70ca0
#define g_pipeline_view_origin_70cdc g_camera_center_x_70cdc

#endif
