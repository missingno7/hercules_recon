#include "hercules_cif.h"

static int cif_view_valid(const HerculesCifView *view)
{
    return view != 0 && view->palette_rgb != 0 &&
           view->blend_tables != 0 && view->shade_table != 0 &&
           view->clut_rows != 0 &&
           view->consumed_bytes == HERCULES_CIF_REQUIRED_BYTES;
}

HerculesCifStatus hercules_cif_view(
    const renderer_u8 *source, size_t source_length, HerculesCifView *view)
{
    HerculesCifView result;

    if (source == 0 || view == 0)
        return HERCULES_CIF_INVALID_ARGUMENT;
    if (source_length < (size_t)HERCULES_CIF_REQUIRED_BYTES)
        return HERCULES_CIF_TRUNCATED;

    result.palette_rgb = source + HERCULES_CIF_PALETTE_OFFSET;
    result.blend_tables = source + HERCULES_CIF_BLEND_OFFSET;
    result.shade_table = source + HERCULES_CIF_SHADE_OFFSET;
    result.clut_rows = source + HERCULES_CIF_CLUT_OFFSET;
    result.consumed_bytes = HERCULES_CIF_REQUIRED_BYTES;
    *view = result;
    return HERCULES_CIF_OK;
}

HerculesCifStatus hercules_cif_palette_entry(
    const HerculesCifView *view, renderer_u32 index,
    HerculesCifPaletteEntry *entry)
{
    renderer_u32 offset;
    HerculesCifPaletteEntry result;

    if (entry == 0 || !cif_view_valid(view) ||
        index >= HERCULES_CIF_PALETTE_ENTRIES)
        return HERCULES_CIF_INVALID_ARGUMENT;

    offset = index * HERCULES_CIF_PALETTE_RGB_BYTES;
    if (index == 0UL || index == HERCULES_CIF_PALETTE_ENTRIES - 1UL) {
        result.red = (renderer_u8)index;
        result.green = (renderer_u8)index;
        result.blue = (renderer_u8)index;
    } else {
        result.red = view->palette_rgb[offset];
        result.green = view->palette_rgb[offset + 1UL];
        result.blue = view->palette_rgb[offset + 2UL];
    }
    result.flags = HERCULES_CIF_PALETTE_ENTRY_FLAGS;
    *entry = result;
    return HERCULES_CIF_OK;
}

HerculesCifStatus hercules_cif_table_span(
    const HerculesCifView *view, renderer_u32 ordinal, HerculesCifSpan *span)
{
    HerculesCifSpan result;

    if (span == 0 || !cif_view_valid(view))
        return HERCULES_CIF_INVALID_ARGUMENT;

    if (ordinal == (renderer_u32)HERCULES_CIF_TABLE_BLEND) {
        result.address = HERCULES_CIF_BLEND_ADDRESS;
        result.bytes = view->blend_tables;
        result.length = HERCULES_CIF_BLEND_BYTES;
    } else if (ordinal == (renderer_u32)HERCULES_CIF_TABLE_SHADE) {
        result.address = HERCULES_CIF_SHADE_ADDRESS;
        result.bytes = view->shade_table;
        result.length = HERCULES_CIF_SHADE_BYTES;
    } else if (ordinal == (renderer_u32)HERCULES_CIF_TABLE_CLUT) {
        result.address = HERCULES_CIF_CLUT_ADDRESS;
        result.bytes = view->clut_rows;
        result.length = HERCULES_CIF_CLUT_BYTES;
    } else {
        return HERCULES_CIF_INVALID_ARGUMENT;
    }

    *span = result;
    return HERCULES_CIF_OK;
}
