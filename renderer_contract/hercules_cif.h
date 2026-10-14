#ifndef HERCULES_CIF_H
#define HERCULES_CIF_H

#include <stddef.h>
#include "hercules_types.h"

/* MAINMENU.CIF resource payload layout observed through the original loader.
 * These are payload-relative offsets, while the addresses below name native
 * destinations. All lengths are exact byte counts. */
#define HERCULES_CIF_PALETTE_OFFSET       ((renderer_u32)0x00000000UL)
#define HERCULES_CIF_PALETTE_ENTRIES      ((renderer_u32)256UL)
#define HERCULES_CIF_PALETTE_RGB_BYTES    ((renderer_u32)3UL)
#define HERCULES_CIF_PALETTE_BYTES        ((renderer_u32)0x00000300UL)

#define HERCULES_CIF_BLEND_OFFSET         ((renderer_u32)0x00000300UL)
#define HERCULES_CIF_BLEND_BANKS          ((renderer_u32)4UL)
#define HERCULES_CIF_BLEND_BANK_BYTES     ((renderer_u32)0x00010000UL)
#define HERCULES_CIF_BLEND_BYTES          ((renderer_u32)0x00040000UL)

#define HERCULES_CIF_SHADE_OFFSET         ((renderer_u32)0x00040300UL)
#define HERCULES_CIF_SHADE_BYTES          ((renderer_u32)0x00010000UL)

#define HERCULES_CIF_CLUT_OFFSET          ((renderer_u32)0x00050300UL)
#define HERCULES_CIF_CLUT_ROWS            ((renderer_u32)31UL)
#define HERCULES_CIF_CLUT_ROW_BYTES       ((renderer_u32)256UL)
#define HERCULES_CIF_CLUT_BYTES           ((renderer_u32)0x00001f00UL)

#define HERCULES_CIF_REQUIRED_BYTES       ((renderer_u32)0x00052200UL)

#define HERCULES_CIF_BLEND_ADDRESS        ((renderer_raw32)0x0092e3d0UL)
#define HERCULES_CIF_SHADE_ADDRESS        ((renderer_raw32)0x0091c488UL)
#define HERCULES_CIF_CLUT_ADDRESS         ((renderer_raw32)0x0092c490UL)
#define HERCULES_CIF_CLUT_END_ADDRESS     ((renderer_raw32)0x0092e390UL)
#define HERCULES_CIF_PALETTE_ENTRY_FLAGS  ((renderer_u8)4U)

typedef struct HerculesCifView {
    const renderer_u8 *palette_rgb;
    const renderer_u8 *blend_tables;
    const renderer_u8 *shade_table;
    const renderer_u8 *clut_rows;
    renderer_u32 consumed_bytes;
} HerculesCifView;

typedef struct HerculesCifPaletteEntry {
    renderer_u8 red;
    renderer_u8 green;
    renderer_u8 blue;
    renderer_u8 flags;
} HerculesCifPaletteEntry;

typedef struct HerculesCifSpan {
    renderer_raw32 address;
    const renderer_u8 *bytes;
    renderer_u32 length;
} HerculesCifSpan;

typedef enum HerculesCifStatus {
    HERCULES_CIF_OK = 0,
    HERCULES_CIF_INVALID_ARGUMENT = 1,
    HERCULES_CIF_TRUNCATED = 2
} HerculesCifStatus;

typedef enum HerculesCifTableOrdinal {
    HERCULES_CIF_TABLE_BLEND = 0,
    HERCULES_CIF_TABLE_SHADE = 1,
    HERCULES_CIF_TABLE_CLUT = 2,
    HERCULES_CIF_TABLE_COUNT = 3
} HerculesCifTableOrdinal;

#ifdef __cplusplus
extern "C" {
#endif

/* Create a borrowed view over one complete CIF resource. Extra bytes are
 * accepted and left outside the view. On failure, view is unchanged. The
 * source remains owned by the caller and must outlive every returned pointer. */
HerculesCifStatus hercules_cif_view(
    const renderer_u8 *source, size_t source_length, HerculesCifView *view);

/* Convert one source RGB triplet to a native PALETTEENTRY-shaped value.
 * Native 0x6780 forces entries 0 and 255 to grayscale; all returned entries
 * carry the observed flags byte 4. The CIF's stored triplets are unchanged. */
HerculesCifStatus hercules_cif_palette_entry(
    const HerculesCifView *view, renderer_u32 index,
    HerculesCifPaletteEntry *entry);

/* Return the three resource-backed table writes in native call order:
 * blend block, shade block, then the 31-row CLUT block. The CLUT span ends
 * before row 32; this payload does not describe any preceding startup state
 * or the gate that selects a runtime CLUT path. On failure, span is unchanged. */
HerculesCifStatus hercules_cif_table_span(
    const HerculesCifView *view, renderer_u32 ordinal, HerculesCifSpan *span);

#ifdef __cplusplus
}
#endif

#endif
