/*
 * Ordinary-C reconstruction of ENG1.DLL RVA 0x20d30.
 *
 * The native routine is cdecl and takes signed whole-coordinate x/y/z plus
 * an ignored DWORD step. Its caller uses only the returned low WORD. The
 * first-word lane has a measured source prefix through index 6, then aliases
 * the existing mutable sequence owner for indices 7..13. The fourth-word lane
 * aliases that owner for indices 6..12. Higher indices remain explicitly
 * incomplete until their source owners and caller domain are proven.
 */
#include "incomplete.h"
#include "resource_runtime_data.h"
#include "grid_query_20d30.h"

typedef unsigned char grid_u8;
typedef unsigned short grid_u16;
typedef unsigned long grid_u32;
typedef long grid_s32;

typedef char grid_20d30_requires_32bit_pointer[(sizeof(void *) == 4) ? 1 : -1];
typedef char grid_20d30_requires_32bit_ulong[(sizeof(grid_u32) == 4) ? 1 : -1];

/* Existing mutable owner; do not copy its overlapping bytes into this file. */
extern grid_u8 g_actor_group_sequence_5e138[0x70];

static grid_u32 grid_20d30_sar8(grid_u32 bits)
{
    grid_u32 shifted;

    shifted = bits >> 8;
    if ((bits & 0x80000000UL) != 0)
        shifted |= 0xff000000UL;
    return shifted;
}

static void grid_20d30_unavailable(grid_u32 rva, const char *reason)
{
    eng1_incomplete_behavior(rva, reason);
    for (;;)
        ;
}

static grid_u32 grid_20d30_sequence_word(grid_u32 offset)
{
    grid_u8 *p;
    p = g_actor_group_sequence_5e138 + offset;
    return *(volatile grid_u32 *)(void *)p;
}

static grid_u32 grid_20d30_first_coefficient(grid_u32 index)
{
    if (index <= 6UL)
        return ((volatile grid_u32 *)g_eng1_grid_query_coefficient_prefix_5e0d0)[index * 4UL];
    if (index <= 13UL)
        return grid_20d30_sequence_word(index * 16UL - 104UL);
    grid_20d30_unavailable(0x5e0d0UL + index * 16UL,
        "0x20d30 first coefficient index has no source-owned value/view");
    return 0UL;
}

static grid_u32 grid_20d30_fourth_coefficient(grid_u32 index)
{
    if (index <= 5UL)
        return ((volatile grid_u32 *)g_eng1_grid_query_coefficient_prefix_5e0d0)[index * 4UL + 3UL];
    if (index <= 12UL)
        return grid_20d30_sequence_word(index * 16UL - 92UL);
    grid_20d30_unavailable(0x5e0dcUL + index * 16UL,
        "0x20d30 fourth coefficient index has no source-owned value/view");
    return 0UL;
}

grid_u32 __cdecl grid_query_20d30(
    grid_s32 x, grid_s32 y, grid_s32 z, grid_u32 step)
{
    grid_u32 z_low;
    grid_u32 layer;
    grid_u32 group_limit;
    grid_u16 base_offset;
    grid_u16 x_extent;
    grid_u16 x_word;
    grid_u32 x_adjusted;
    grid_u32 x_column;
    grid_u32 y_adjusted;
    grid_u32 y_column;
    grid_u16 width_word;
    grid_s32 width_signed;
    grid_u32 cell_index;
    grid_u32 cell_address;
    grid_u32 cell_value;
    grid_u32 record_type;
    grid_u32 header_address;
    grid_u32 record_address;
    grid_u32 relative_address;
    grid_u32 packed_address;
    grid_u32 packed_word;
    grid_u32 x_index;
    grid_u32 y_index;
    grid_u32 z_index;
    grid_u32 local_x;
    grid_u32 local_y;
    grid_u32 local_z;
    grid_u32 coefficient;
    grid_u32 factor;
    grid_u32 term;
    grid_u32 offset_bits;
    grid_u32 sample_address;
    grid_u8 sample;
    grid_u16 result_word;

    (void)step;

    if (g_active_row_70d01 == (signed char)-1)
        return 0UL;

    z_low = (grid_u32)z & 0xffffUL;
    layer = (z_low + 0x40UL) >> 7;
    if (g_group_count_70ece <= 0)
        grid_20d30_unavailable(0x70eceUL,
            "0x20d30 native layer clamp requires a positive signed group count");

    group_limit = (grid_u32)(unsigned char)g_group_count_70ece - 1UL;
    if (layer > group_limit)
        layer = group_limit;
    if (layer >= 16UL)
        grid_20d30_unavailable(0x70dccUL,
            "0x20d30 row-pointer index exceeds the measured sixteen-pointer prefix");

    base_offset = g_rebuilt_row_offsets_70e0c[16UL + layer];
    x_word = (grid_u16)(base_offset * (grid_u16)0xffc0U);
    x_adjusted = ((grid_u32)x + (grid_u32)x_word) & 0xffffUL;
    x_column = x_adjusted >> 6;
    x_extent = g_rebuilt_row_offsets_70e0c[layer];
    if (x_column >= (grid_u32)x_extent)
        return 0UL;

    y_adjusted = ((grid_u32)y + 0x20UL) & 0xffffUL;
    y_column = y_adjusted >> 6;
    width_word = (grid_u16)g_element_width_70d14;
    if (y_column >= (grid_u32)width_word)
        return 0UL;
    width_signed = (grid_s32)g_element_width_70d14;
    if (width_signed <= 0)
        grid_20d30_unavailable(0x70d14UL,
            "0x20d30 signed row stride is outside the measured positive-width domain");

    cell_index = x_column * (grid_u32)width_signed + y_column;
    if (g_rebuilt_row_pointers_70dcc[layer] == (grid_u8 *)0)
        grid_20d30_unavailable(0x70dccUL,
            "0x20d30 selected row pointer has no reconstructed owner");
    cell_address = (grid_u32)(unsigned long)g_rebuilt_row_pointers_70dcc[layer]
                 + cell_index * 4UL;
    cell_value = *(volatile grid_u32 *)(unsigned long)cell_address;
    record_type = (cell_value >> 24) & 0x7fUL;
    if (record_type == 0UL)
        return 0UL;

    if (g_header_pointer_70d40 == (grid_u8 *)0)
        grid_20d30_unavailable(0x70d40UL,
            "0x20d30 nonzero grid record has no reconstructed header pointer");
    header_address = (grid_u32)(unsigned long)g_header_pointer_70d40;
    record_address = header_address + record_type * 4UL - 4UL;
    relative_address = *(volatile grid_u32 *)(unsigned long)record_address;
    packed_address = record_address + relative_address;
    packed_word = *(volatile grid_u16 *)(unsigned long)packed_address;
    sample_address = packed_address + 2UL;

    x_index = packed_word & 0x0fUL;
    y_index = (packed_word >> 4) & 0x0fUL;
    z_index = (packed_word >> 8) & 0xffUL;
    offset_bits = 0UL;
    if (x_index != 0UL) {
        local_x = x_adjusted & 0x3fUL;
        coefficient = grid_20d30_first_coefficient(x_index);
        offset_bits += grid_20d30_sar8(coefficient * local_x);
    }

    if (y_index != 0UL) {
        local_y = y_adjusted & 0x3fUL;
        coefficient = grid_20d30_first_coefficient(y_index);
        factor = grid_20d30_fourth_coefficient(x_index);
        offset_bits += grid_20d30_sar8(coefficient * local_y) * factor;
    }

    if (z_index != 0UL) {
        local_z = ((z_low + 0x3fUL) >> 1) & 0x3fUL;
        coefficient = grid_20d30_first_coefficient(z_index);
        term = grid_20d30_sar8(coefficient * local_z);
        factor = grid_20d30_fourth_coefficient(x_index);
        term *= factor;
        factor = grid_20d30_fourth_coefficient(y_index);
        term *= factor;
        offset_bits += term;
    }

    sample_address += offset_bits;
    sample = *(volatile grid_u8 *)(unsigned long)sample_address;
    if ((sample & 0x80U) != 0)
        result_word = (grid_u16)(0xff00U | (grid_u16)sample);
    else
        result_word = (grid_u16)sample;
    return (grid_u32)result_word;
}
