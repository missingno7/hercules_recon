#include "hercules_textures.h"

int herc_tex_rect_supported(const HERC_TEX_RECT16 *rect)
{
    long right;
    long bottom;
    if (rect == 0)
        return 0;
    if (rect->x < 0 || rect->y < 0 || rect->width < 0 || rect->height < 0)
        return 0;
    if (rect->x >= (renderer_i16)HERC_TEX_UPLOAD_X_WORDS ||
        rect->y >= (renderer_i16)HERC_TEX_UPLOAD_Y_ROWS)
        return 0;
    right = (long)rect->x + (long)rect->width;
    bottom = (long)rect->y + (long)rect->height;
    return right <= (long)HERC_TEX_UPLOAD_X_WORDS &&
           bottom <= (long)HERC_TEX_UPLOAD_Y_ROWS;
}

int herc_tex_upload_byte_offset(
    renderer_u32 x_word, renderer_u32 y_row, renderer_u32 byte_in_word,
    renderer_u32 *offset_from_map_base)
{
    renderer_u32 row;
    renderer_u32 byte_x;
    if (offset_from_map_base == 0 || x_word >= HERC_TEX_UPLOAD_X_WORDS ||
        y_row >= HERC_TEX_UPLOAD_Y_ROWS || byte_in_word > 1U)
        return 0;
    row = (y_row < 256U) ? y_row : (0x800U + (y_row & 0xffU));
    byte_x = (x_word << 1) + byte_in_word;
    *offset_from_map_base = (row << 8) + ((byte_x & 0x700U) << 8) +
                            (byte_x & 0xffU);
    return 1;
}

int herc_tex_sample_byte_offset(
    renderer_u16 tpage, renderer_u8 u, renderer_u8 v,
    renderer_u32 *offset_from_map_base)
{
    renderer_u32 base_offset;
    if (offset_from_map_base == 0)
        return 0;
    if ((tpage & 0x0180U) == 0x0180U)
        base_offset = 0x00100000U;
    else
        base_offset = ((renderer_u32)(tpage & 0x001fU) >> 1) << 16;
    *offset_from_map_base = base_offset + ((renderer_u32)v << 8) +
                            (renderer_u32)u;
    return 1;
}

renderer_u32 herc_tex_clut_row(renderer_u16 clut_word)
{
    long row;
    row = ((long)clut_word >> 6) - 0x1e0L;
    if (row < 0L || row > 30L)
        return 0U;
    return (renderer_u32)row;
}

renderer_u32 herc_tex_clut_byte_offset(renderer_u16 clut_word,
                                       renderer_u8 texel)
{
    return (herc_tex_clut_row(clut_word) << 8) + (renderer_u32)texel;
}

int herc_tex_cc50_active_clut_writes(renderer_u8 texel)
{
    return texel != 0U;
}

renderer_u32 herc_tex_blend_table_index(renderer_u16 tpage)
{
    return ((renderer_u32)tpage >> 5) & 3U;
}

int herc_tex_c882_ca51_cbf4_writes(renderer_u8 texel)
{
    return texel != 0U;
}

renderer_u32 herc_tex_a640_return_mode0(
    renderer_u32 arg2, renderer_u32 arg4, renderer_u32 arg5)
{
    renderer_u32 result;
    renderer_u32 low;
    low = (((arg4 & 3U) << 2) | (arg5 & 3U)) << 3;
    result = low | (arg2 & 0x200U);
    result |= (((arg4 >> 2) & 0xf0U) | (arg2 & 0x100U)) >> 4;
    return result;
}
