#ifndef ENG1_GEOMETRY_ASSOCIATION_OBSERVER_H
#define ENG1_GEOMETRY_ASSOCIATION_OBSERVER_H

#include "hercules_types.h"

/* Tool-side snapshots at original producer instruction boundaries. These
 * records are not an ENG1 callback ABI and do not describe an installed hook. */
#define ENG1_ASSOC_GT4_FACE_WORD_READ_RVA ((renderer_u32)0x2e0f1u)
#define ENG1_ASSOC_FT4_FACE_WORD_READ_RVA ((renderer_u32)0x2e27bu)
#define ENG1_ASSOC_GT4_PACKET_COMMIT_RVA  ((renderer_u32)0x2c790u)
#define ENG1_ASSOC_FT4_PACKET_COMMIT_RVA  ((renderer_u32)0x2c770u)

#define ENG1_ASSOC_FACE_RECORD_STRIDE    ((renderer_u32)8u)
#define ENG1_ASSOC_FACE_INDEX_COUNT      ((renderer_u32)4u)
#define ENG1_ASSOC_FACE_INDEX_BITS       ((renderer_u32)6u)
#define ENG1_ASSOC_FACE_INDEX_MASK       ((renderer_u32)0x3fu)
#define ENG1_ASSOC_POINT_SOURCE_STRIDE   ((renderer_u32)4u)
#define ENG1_ASSOC_POINT_STAGE_OFFSET    ((renderer_u32)0x40u)
#define ENG1_ASSOC_POINT_STAGE_STRIDE    ((renderer_u32)8u)

/* Captured immediately before the face-word load. The raw word is decoded
 * only for the four low six-bit slots verified by the bounded producer run. */
struct eng1_assoc_face_word_snapshot {
    renderer_u32 producer_rva;
    renderer_u32 face_word_address;       /* [EBP+8] address at read boundary */
    renderer_u32 raw_face_word;
    renderer_u32 esi_packet_cursor_candidate;
    renderer_u32 edi_point_stage_base;
};

/* Captured at the route-specific wrapper entry, after surviving culling. */
struct eng1_assoc_packet_commit_snapshot {
    renderer_u32 wrapper_rva;
    renderer_u32 packet_pointer_argument; /* [ESP+4] */
    renderer_u32 esi_packet_cursor;
};

typedef char eng1_assoc_face_word_snapshot_is_20[
    (sizeof(struct eng1_assoc_face_word_snapshot) == 20) ? 1 : -1];
typedef char eng1_assoc_packet_commit_snapshot_is_12[
    (sizeof(struct eng1_assoc_packet_commit_snapshot) == 12) ? 1 : -1];

#endif
