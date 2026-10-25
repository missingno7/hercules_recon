#ifndef HERCULES_ENG1_CALLBACK_STORAGE_VIEWS_H
#define HERCULES_ENG1_CALLBACK_STORAGE_VIEWS_H
#include "actor_pipeline_storage_views.h"
/* Native callbacks use different widths in the same physical allocation. */
#define eng1_native_cell_0718e0 (g_actor_pipeline_storage_71440.word_cells[0x4a0 / 2])
#define eng1_native_cell_0716a8 (g_actor_pipeline_storage_71440.word_cells[0x268 / 2])
#define eng1_native_cell_071718 (g_actor_pipeline_storage_71440.dword_cells[0x2d8 / 4])
#define eng1_native_ptr_071714 \
    (*(unsigned short **)(void *)&g_actor_pipeline_storage_71440.dword_cells[0x2d4 / 4])
#endif
