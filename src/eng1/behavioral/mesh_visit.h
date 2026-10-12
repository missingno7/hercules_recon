#ifndef ENG1_PRIVATE_MESH_VISIT_H
#define ENG1_PRIVATE_MESH_VISIT_H
#include "../../../calibration/engine_dispatch_externs.h"
/* Shared measured caller/callee prefix. Historical type/TU remain unknown. */
typedef struct Eng1MeshVisit {
    u8 *header_000, *header_004, *child_008;
    short projected_00c[2];
    short depth_010;
    u8 unknown_012[3];
    u8 column_counter_015;
    u8 column_stop_016;
    u8 row_stop_017;
    long row_018;
    long column_01c;
    u32 packed_020;
    u8 winding_024, variant_025, shift_026, limit_027;
    u8 mode_028, side_029;
    u16 cell_02a, flags_02c;
    u8 unknown_02e[6];
    long tick_034, layer_y_038, layer_z_03c, cached_040, cached_044;
    long row_local_048, column_local_04c, anchor_row_050;
    long transform_054[8];
} Eng1MeshVisit;
typedef char mesh_visit_offsets[
    (offsetof(Eng1MeshVisit,depth_010)==0x10 &&
     offsetof(Eng1MeshVisit,column_counter_015)==0x15 &&
     offsetof(Eng1MeshVisit,packed_020)==0x20 &&
     offsetof(Eng1MeshVisit,tick_034)==0x34 &&
     offsetof(Eng1MeshVisit,transform_054)==0x54 &&
     sizeof(Eng1MeshVisit)==0x74) ? 1 : -1];
#endif
