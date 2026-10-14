#include <stdio.h>
#include <stddef.h>

#include "hercules_cif.h"

static renderer_u8 fixture[HERCULES_CIF_REQUIRED_BYTES];
static renderer_u8 extended_fixture[HERCULES_CIF_REQUIRED_BYTES + 7UL];

static void fill_fixture(renderer_u8 *bytes, renderer_u32 length)
{
    renderer_u32 index;

    for (index = 0UL; index < length; ++index)
        bytes[index] = (renderer_u8)(index * 13UL + 5UL);
    for (index = 0UL; index < HERCULES_CIF_PALETTE_ENTRIES; ++index) {
        renderer_u32 offset = index * HERCULES_CIF_PALETTE_RGB_BYTES;
        bytes[offset] = (renderer_u8)(index + 3UL);
        bytes[offset + 1UL] = (renderer_u8)(255UL - index);
        bytes[offset + 2UL] = (renderer_u8)(index * 5UL + 1UL);
    }
}

static int same_view(const HerculesCifView *left,
                     const HerculesCifView *right)
{
    return left->palette_rgb == right->palette_rgb &&
           left->blend_tables == right->blend_tables &&
           left->shade_table == right->shade_table &&
           left->clut_rows == right->clut_rows &&
           left->consumed_bytes == right->consumed_bytes;
}

static int same_span(const HerculesCifSpan *left,
                     const HerculesCifSpan *right)
{
    return left->address == right->address && left->bytes == right->bytes &&
           left->length == right->length;
}

static int test_view_boundaries(void)
{
    HerculesCifView view;
    HerculesCifView before;
    HerculesCifView expected;

    fill_fixture(fixture, HERCULES_CIF_REQUIRED_BYTES);
    view.palette_rgb = fixture + 11UL;
    view.blend_tables = fixture + 12UL;
    view.shade_table = fixture + 13UL;
    view.clut_rows = fixture + 14UL;
    view.consumed_bytes = 17UL;
    before = view;
    if (hercules_cif_view(fixture,
            (size_t)(HERCULES_CIF_REQUIRED_BYTES - 1UL), &view) !=
            HERCULES_CIF_TRUNCATED || !same_view(&view, &before))
        return 0;

    if (hercules_cif_view(fixture,
            (size_t)HERCULES_CIF_REQUIRED_BYTES, &view) != HERCULES_CIF_OK)
        return 0;
    expected.palette_rgb = fixture + HERCULES_CIF_PALETTE_OFFSET;
    expected.blend_tables = fixture + HERCULES_CIF_BLEND_OFFSET;
    expected.shade_table = fixture + HERCULES_CIF_SHADE_OFFSET;
    expected.clut_rows = fixture + HERCULES_CIF_CLUT_OFFSET;
    expected.consumed_bytes = HERCULES_CIF_REQUIRED_BYTES;
    if (!same_view(&view, &expected))
        return 0;

    fill_fixture(extended_fixture,
                 (renderer_u32)sizeof(extended_fixture));
    if (hercules_cif_view(extended_fixture, sizeof(extended_fixture),
                          &view) != HERCULES_CIF_OK ||
        view.palette_rgb != extended_fixture + HERCULES_CIF_PALETTE_OFFSET ||
        view.blend_tables != extended_fixture + HERCULES_CIF_BLEND_OFFSET ||
        view.shade_table != extended_fixture + HERCULES_CIF_SHADE_OFFSET ||
        view.clut_rows != extended_fixture + HERCULES_CIF_CLUT_OFFSET ||
        view.consumed_bytes != HERCULES_CIF_REQUIRED_BYTES)
        return 0;

    before = view;
    if (hercules_cif_view(0, sizeof(extended_fixture), &view) !=
            HERCULES_CIF_INVALID_ARGUMENT || !same_view(&view, &before))
        return 0;
    if (hercules_cif_view(extended_fixture, sizeof(extended_fixture), 0) !=
        HERCULES_CIF_INVALID_ARGUMENT)
        return 0;
    return 1;
}

static int test_palette_entries(void)
{
    HerculesCifView view;
    HerculesCifPaletteEntry entry;
    HerculesCifPaletteEntry before;

    fill_fixture(fixture, HERCULES_CIF_REQUIRED_BYTES);
    if (hercules_cif_view(fixture, sizeof(fixture), &view) !=
        HERCULES_CIF_OK)
        return 0;

    if (hercules_cif_palette_entry(&view, 0UL, &entry) != HERCULES_CIF_OK ||
        entry.red != 0U || entry.green != 0U || entry.blue != 0U ||
        entry.flags != HERCULES_CIF_PALETTE_ENTRY_FLAGS)
        return 0;
    if (hercules_cif_palette_entry(&view,
            HERCULES_CIF_PALETTE_ENTRIES - 1UL, &entry) !=
            HERCULES_CIF_OK || entry.red != 255U || entry.green != 255U ||
        entry.blue != 255U || entry.flags != HERCULES_CIF_PALETTE_ENTRY_FLAGS)
        return 0;

    if (hercules_cif_palette_entry(&view, 17UL, &entry) != HERCULES_CIF_OK ||
        entry.red != 20U || entry.green != 238U || entry.blue != 86U ||
        entry.flags != HERCULES_CIF_PALETTE_ENTRY_FLAGS)
        return 0;

    entry.red = 91U;
    entry.green = 92U;
    entry.blue = 93U;
    entry.flags = 94U;
    before = entry;
    if (hercules_cif_palette_entry(&view, HERCULES_CIF_PALETTE_ENTRIES,
                                   &entry) != HERCULES_CIF_INVALID_ARGUMENT ||
        entry.red != before.red || entry.green != before.green ||
        entry.blue != before.blue || entry.flags != before.flags)
        return 0;
    if (hercules_cif_palette_entry(&view, 1UL, 0) !=
        HERCULES_CIF_INVALID_ARGUMENT)
        return 0;
    return 1;
}

static int test_ordered_table_spans(void)
{
    HerculesCifView view;
    HerculesCifSpan span;
    HerculesCifSpan before;

    fill_fixture(fixture, HERCULES_CIF_REQUIRED_BYTES);
    if (hercules_cif_view(fixture, sizeof(fixture), &view) !=
        HERCULES_CIF_OK)
        return 0;

    if (hercules_cif_table_span(&view, HERCULES_CIF_TABLE_BLEND, &span) !=
            HERCULES_CIF_OK || span.address != HERCULES_CIF_BLEND_ADDRESS ||
        span.bytes != view.blend_tables ||
        span.length != HERCULES_CIF_BLEND_BYTES)
        return 0;
    if (hercules_cif_table_span(&view, HERCULES_CIF_TABLE_SHADE, &span) !=
            HERCULES_CIF_OK || span.address != HERCULES_CIF_SHADE_ADDRESS ||
        span.bytes != view.shade_table ||
        span.length != HERCULES_CIF_SHADE_BYTES)
        return 0;
    if (hercules_cif_table_span(&view, HERCULES_CIF_TABLE_CLUT, &span) !=
            HERCULES_CIF_OK || span.address != HERCULES_CIF_CLUT_ADDRESS ||
        span.bytes != view.clut_rows || span.length != HERCULES_CIF_CLUT_BYTES ||
        span.address + span.length != HERCULES_CIF_CLUT_END_ADDRESS)
        return 0;

    span.address = (renderer_raw32)0xabcdefUL;
    span.bytes = fixture;
    span.length = 123UL;
    before = span;
    if (hercules_cif_table_span(&view, HERCULES_CIF_TABLE_COUNT, &span) !=
            HERCULES_CIF_INVALID_ARGUMENT || !same_span(&span, &before))
        return 0;
    if (hercules_cif_table_span(&view, HERCULES_CIF_TABLE_BLEND, 0) !=
        HERCULES_CIF_INVALID_ARGUMENT)
        return 0;
    return 1;
}

int main(void)
{
    if (!test_view_boundaries()) {
        fprintf(stderr, "CIF resource view bounds failed\n");
        return 1;
    }
    if (!test_palette_entries()) {
        fprintf(stderr, "CIF palette entry conversion failed\n");
        return 1;
    }
    if (!test_ordered_table_spans()) {
        fprintf(stderr, "CIF table span order or bounds failed\n");
        return 1;
    }
    puts("CIF contract: borrowed payload bounds, palette endpoints, and ordered spans passed");
    return 0;
}
