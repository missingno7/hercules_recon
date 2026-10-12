#ifndef ENG1_FOREST_ENTRY_0E3E0_H
#define ENG1_FOREST_ENTRY_0E3E0_H

typedef char eng1_forest_entry_pointer_must_be_32_bits[
    (sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_forest_entry_dword_must_be_32_bits[
    (sizeof(unsigned long) == 4) ? 1 : -1];

void __cdecl dispatch_local_00e3e0(void);

#endif
