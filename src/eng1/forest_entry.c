/*
 * Behavioral source candidate for the forest-selected ENG1 entry at RVA
 * 0x0e3e0.  The two cells and the initialized word table remain external
 * data owners; their complete extents and semantic names are not established
 * by this function.
 */
#include "forest_entry_data.h"

void __cdecl dispatch_local_00e3e0(void)
{
    eng1_native_cell_06ef10 = 0UL;
    eng1_native_ptr_071714 = eng1_native_words_04dce8;
}
