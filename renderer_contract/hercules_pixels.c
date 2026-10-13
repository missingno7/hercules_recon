#include "hercules_pixels.h"

static int herc_pixel_table_valid(const HercPixelTableReader *table,
                                 renderer_u32 minimum_length)
{
    if (table == 0 || table->read_byte == 0)
        return 0;
    if (table->length < minimum_length)
        return 0;
    return 1;
}

static int herc_pixel_destination_writer_valid(
    const HercPixelDestinationAccess *destination)
{
    return destination != 0 && destination->write_byte != 0;
}

HercPixelStepResult herc_pixel_c882_step(
    renderer_u8 source_texel, const HercPixelTableReader *shade_row,
    const HercPixelTableReader *blend_tables, renderer_u32 blend_table_count,
    renderer_u32 selector, const HercPixelDestinationAccess *destination)
{
    renderer_u8 mapped_color;
    renderer_u8 old_destination;
    renderer_u8 blended_color;
    renderer_u32 blend_index;
    const HercPixelTableReader *blend_table;

    if (!herc_pixel_destination_writer_valid(destination))
        return HERC_PIXEL_INVALID;
    if (!herc_pixel_table_valid(shade_row, HERC_PIXEL_COLOR_ROW_BYTES))
        return HERC_PIXEL_INVALID;
    if (blend_tables == 0 ||
        blend_table_count != HERC_PIXEL_BLEND_SELECTORS ||
        selector >= HERC_PIXEL_BLEND_SELECTORS)
        return HERC_PIXEL_INVALID;
    blend_table = &blend_tables[selector];
    if (!herc_pixel_table_valid(blend_table, HERC_PIXEL_BLEND_BYTES))
        return HERC_PIXEL_INVALID;

    if (source_texel == 0)
        return HERC_PIXEL_NO_WRITE;
    if (destination->read_byte == 0)
        return HERC_PIXEL_INVALID;

    if (!shade_row->read_byte(shade_row->context, (renderer_u32)source_texel,
                              &mapped_color))
        return HERC_PIXEL_INVALID;
    if (!destination->read_byte(destination->context, &old_destination))
        return HERC_PIXEL_INVALID;

    blend_index = ((renderer_u32)mapped_color << 8) |
                  (renderer_u32)old_destination;
    if (!blend_table->read_byte(blend_table->context, blend_index,
                                &blended_color))
        return HERC_PIXEL_INVALID;
    if (!destination->write_byte(destination->context, blended_color))
        return HERC_PIXEL_INVALID;

    return HERC_PIXEL_WROTE;
}

HercPixelStepResult herc_pixel_cc50_step(
    renderer_u8 source_texel, const HercPixelTableReader *palette_row,
    const HercPixelDestinationAccess *destination)
{
    renderer_u8 palette_color;

    if (!herc_pixel_destination_writer_valid(destination))
        return HERC_PIXEL_INVALID;
    if (!herc_pixel_table_valid(palette_row, HERC_PIXEL_COLOR_ROW_BYTES))
        return HERC_PIXEL_INVALID;
    if (source_texel == 0)
        return HERC_PIXEL_NO_WRITE;

    if (!palette_row->read_byte(palette_row->context,
                                (renderer_u32)source_texel, &palette_color))
        return HERC_PIXEL_INVALID;
    if (!destination->write_byte(destination->context, palette_color))
        return HERC_PIXEL_INVALID;

    return HERC_PIXEL_WROTE;
}

HercPixelStepResult herc_pixel_c2e0_fill_byte(
    renderer_u8 fill_byte, const HercPixelDestinationAccess *destination)
{
    if (!herc_pixel_destination_writer_valid(destination))
        return HERC_PIXEL_INVALID;
    if (!destination->write_byte(destination->context, fill_byte))
        return HERC_PIXEL_INVALID;
    return HERC_PIXEL_WROTE;
}

static int pixel_proposal_destination_writer_valid(
    const HercPixelDestinationAccess *destination)
{
    return destination != 0 && destination->write_byte != 0;
}

HercPixelStepResult herc_pixel_c8f0_step(
    renderer_u8 source_texel, const HercPixelTableReader *blend_table,
    const HercPixelDestinationAccess *destination)
{
    renderer_u8 old_destination;
    renderer_u8 blended_color;
    renderer_u32 blend_index;

    if (!pixel_proposal_destination_writer_valid(destination))
        return HERC_PIXEL_INVALID;
    if (blend_table == 0 || blend_table->read_byte == 0 ||
        blend_table->length < HERC_PIXEL_BLEND_BYTES)
        return HERC_PIXEL_INVALID;
    if (source_texel == 0)
        return HERC_PIXEL_NO_WRITE;
    if (destination->read_byte == 0)
        return HERC_PIXEL_INVALID;

    if (!destination->read_byte(destination->context, &old_destination))
        return HERC_PIXEL_INVALID;
    blend_index = ((renderer_u32)source_texel << 8) |
                  (renderer_u32)old_destination;
    if (!blend_table->read_byte(blend_table->context, blend_index,
                                &blended_color))
        return HERC_PIXEL_INVALID;
    if (!destination->write_byte(destination->context, blended_color))
        return HERC_PIXEL_INVALID;

    return HERC_PIXEL_WROTE;
}

HercPixelStepResult herc_pixel_cab0_step(
    renderer_u8 source_texel,
    const HercPixelDestinationAccess *destination)
{
    if (!pixel_proposal_destination_writer_valid(destination))
        return HERC_PIXEL_INVALID;
    if (source_texel == 0)
        return HERC_PIXEL_NO_WRITE;
    if (!destination->write_byte(destination->context, source_texel))
        return HERC_PIXEL_INVALID;
    return HERC_PIXEL_WROTE;
}
