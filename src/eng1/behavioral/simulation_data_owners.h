#ifndef ENG1_SIMULATION_DATA_OWNERS_H
#define ENG1_SIMULATION_DATA_OWNERS_H

/* Typed views for measured ENG1 simulation data prefixes.
 * No historical name, full capacity, or linked RVA placement is asserted. */
typedef unsigned char sim_data_u8;
typedef unsigned short sim_data_u16;
typedef unsigned long sim_data_u32;

typedef struct Eng1SimulationWordSlot {
    sim_data_u16 value;
    sim_data_u16 other;
} Eng1SimulationWordSlot;

typedef struct Eng1SimulationLookupOwner {
    void *pointer_prefix_537e0[8];
    sim_data_u32 value_view_53800[64];
} Eng1SimulationLookupOwner;

extern Eng1SimulationWordSlot eng1_native_data_target_0052b70[8];
extern sim_data_u8 g_eng1_simulation_data_bytes_53620[0x1c0];
extern Eng1SimulationLookupOwner g_eng1_simulation_lookup_owner_537e0;

/* The 0x53800 DWORD view is the same physical storage as owner+0x20. */
#define eng1_native_data_target_0053800 \
    (g_eng1_simulation_lookup_owner_537e0.value_view_53800)

/* For 0x537e0, entries 0..7 are pointer-valued; entries 8..63 share the
 * DWORD bytes beginning at 0x53800. Callers must enforce index < 64. */
#define ENG1_NATIVE_DATA_TARGET_00537E0_DWORD(index) \
    ((sim_data_u32)((index) < 8UL \
        ? (sim_data_u32)g_eng1_simulation_lookup_owner_537e0.pointer_prefix_537e0[(index)] \
        : g_eng1_simulation_lookup_owner_537e0.value_view_53800[(index) - 8UL]))

#endif
