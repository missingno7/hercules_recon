#ifndef ENG1_FOREST_ENTRY_DATA_H
#define ENG1_FOREST_ENTRY_DATA_H

#include "forest_entry.h"
#include "actor_pipeline_storage.h"

/* Four-byte, loader-zero cell observed at ENG1 RVA 0x6ef10. */
extern unsigned long eng1_native_cell_06ef10;

/* Typed interior view at owner RVA 0x71440 + 0x2d4 = 0x71714. The measured
 * actor-pipeline aggregate supplies the bytes; this lane defines no cell. */
#define eng1_native_ptr_071714 \
    (*(unsigned short **)(void *)&g_actor_pipeline_storage_71440.byte_cells[0x2d4])

/* Eight WORDs observed from 0x4dce8 through the next data boundary at 0x4dcf8.
 * This is a measured prefix, not a claim about the complete logical table. */
extern unsigned short eng1_native_words_04dce8[];

/* Separate bounded sound WORD prefix used by level1 initialization. */
extern unsigned short g_eng1_sound_words_66d68[8];

#endif
