/* Ordinary-C candidate for ENG1.DLL RVA 0x20b50.
 *
 * This is the bounded grid scan called by the actor update and spatial
 * callbacks. The query borrows the existing resource runtime owners; it does
 * not define another copy of their row tables or header pointers. The stream
 * decoder at RVA 0x20990 remains a separately owned dependency.
 */
#include "grid_query_20b50.h"
#include "incomplete.h"
#include "resource_runtime_data.h"

typedef unsigned char query_u8;
typedef unsigned short query_u16;
typedef unsigned long query_u32;
typedef short query_s16;
typedef long query_s32;

typedef char query_20b50_requires_32bit_pointer[
    (sizeof(void *) == 4) ? 1 : -1];
typedef char query_20b50_requires_32bit_ulong[
    (sizeof(query_u32) == 4) ? 1 : -1];

#include "query_20990.h"

static void query_20b50_unsupported(query_u32 rva, const char *reason)
{
    eng1_incomplete_behavior(rva, reason);
    for (;;)
        ;
}

static int query_20b50_sign_clear(query_u32 bits)
{
    return (bits & 0x80000000UL) == 0UL;
}

/* Returns zero for an empty cell/relative record. A nonzero tag requires a
 * non-null source-owned header pointer. Its allocation extent is not yet
 * measured, so no arbitrary length or tag capacity is asserted here. */
static int query_20b50_decode_cell(
    query_u32 cell_address,
    query_u32 x_fraction,
    query_u32 z_residual,
    query_u32 *sample_bits)
{
    query_u32 cell;
    query_u32 tag;
    query_u32 header_address;
    query_u32 entry_address;
    query_u32 program_address;
    query_s16 relative;
    query_u32 helper_x;

    cell = *(volatile query_u32 *)(unsigned long)cell_address;
    tag = (cell >> 1) & 0x3ffUL;
    if (tag == 0UL)
        return 0;

    if (g_header_pointer_70d34 == (unsigned char *)0)
        query_20b50_unsupported(0x70d34UL,
            "0x20b50 nonzero cell tag has no reconstructed header pointer");

    header_address = (query_u32)(unsigned long)g_header_pointer_70d34;
    entry_address = header_address + tag * 4UL - 2UL;
    relative = *(volatile query_s16 *)(unsigned long)entry_address;
    if (relative == 0)
        return 0;

    program_address = entry_address + (query_u32)(query_s32)relative;
    if ((cell & 1UL) != 0UL)
        helper_x = 0x3fUL - x_fraction;
    else
        helper_x = x_fraction;

    *sample_bits = (query_u32)query_decode_20990(
        helper_x, z_residual,
        (const unsigned char *)(unsigned long)program_address);
    return 1;
}

static query_u32 query_20b50_pack(query_u32 high, query_u32 low)
{
    return (high << 16) + low;
}

query_u32 __cdecl grid_scan_20b50(
    query_s32 x,
    query_s32 y,
    query_s32 z,
    query_u32 step)
{
    query_u32 rounded_z;
    query_u32 layer;
    query_u32 layer_limit;
    query_u16 base_offset;
    query_u16 x_extent;
    query_u16 width_word;
    query_s32 width_signed;
    query_u32 x_adjusted;
    query_u32 x_cell;
    query_u32 y_adjusted;
    query_u32 y_cell;
    query_u32 x_fraction;
    query_u32 z_fraction;
    query_u32 z_residual;
    query_u32 row_address;
    query_u32 cell_index;
    query_u32 cell_address;
    query_u32 scan_index;
    query_u32 scan_coordinate;
    query_u32 low_result;
    query_u32 sample_bits;
    query_u32 difference;
    query_u32 backward_coordinate;
    query_u32 combined;
    int decoder_called;

    (void)step;

    if (g_active_row_70d01 == (signed char)-1)
        return 0x7fff7fffUL;

    rounded_z = ((query_u32)z + 0x3fUL) & 0xffffUL;
    layer = rounded_z >> 7;
    if (g_group_count_70ece <= 0)
        query_20b50_unsupported(0x70eceUL,
            "0x20b50 signed group-count clamp has no positive measured domain");

    layer_limit = (query_u32)(query_u8)g_group_count_70ece - 1UL;
    if (layer > layer_limit)
        layer = layer_limit;
    if (layer >= 16UL)
        query_20b50_unsupported(0x70dccUL,
            "0x20b50 selected row-pointer index exceeds the measured 16-slot prefix");

    base_offset = g_rebuilt_row_offsets_70e0c[16UL + layer];
    x_adjusted = ((query_u32)x +
        (query_u32)base_offset * 0xffc0UL) & 0xffffUL;
    x_cell = x_adjusted >> 6;
    x_extent = g_rebuilt_row_offsets_70e0c[layer];
    if (x_cell >= (query_u32)x_extent)
        return 0x7fff7fffUL;

    y_adjusted = ((query_u32)y + 0xffe0UL) & 0xffffUL;
    y_cell = y_adjusted >> 6;
    width_word = (query_u16)g_element_width_70d14;
    if (y_cell >= (query_u32)width_word)
        return 0x7fff7fffUL;

    width_signed = (query_s32)g_element_width_70d14;
    if (width_signed <= 0)
        query_20b50_unsupported(0x70d14UL,
            "0x20b50 signed row width is outside the positive measured domain");

    if (g_rebuilt_row_pointers_70dcc[layer] == (unsigned char *)0)
        query_20b50_unsupported(0x70dccUL,
            "0x20b50 selected grid row has no reconstructed row-pointer owner");

    row_address = (query_u32)(unsigned long)g_rebuilt_row_pointers_70dcc[layer];
    cell_index = x_cell * (query_u32)width_signed + y_cell;
    cell_address = row_address + cell_index * 4UL;
    x_fraction = x_adjusted & 0x3fUL;
    z_fraction = (rounded_z >> 1) & 0x3fUL;
    z_residual = 0x3fUL - z_fraction;
    low_result = 0x7fffUL;
    scan_index = y_cell;
    scan_coordinate = scan_index << 6;

    for (;;) {
        decoder_called = query_20b50_decode_cell(cell_address, x_fraction,
                                                  z_residual, &sample_bits);
        if (decoder_called && sample_bits != 0xffffffffUL) {
            difference = scan_coordinate - sample_bits - y_adjusted;
            if (query_20b50_sign_clear(difference)) {
                low_result = difference;
                break;
            }
        }

        /* The native loop increments the cell pointer before testing width.
         * If it exhausts the forward scan, the second scan dereferences this
         * resulting one-past-width slot first; keep that observable order. */
        cell_address += 4UL;
        ++scan_index;
        scan_coordinate += 0x40UL;
        if (decoder_called &&
            (sample_bits == 0xffffffffUL ||
             !query_20b50_sign_clear(difference))) {
            width_word = (query_u16)g_element_width_70d14;
            width_signed = (query_s16)width_word;
        }
        if ((query_s32)scan_index >= width_signed)
            break;
    }

    /* 0x20c99 restores y_adjusted, regardless of whether the forward scan
     * found a sample. The reverse scan starts from y_adjusted - cursor. */
    backward_coordinate = y_adjusted - scan_coordinate;

    for (;;) {
        decoder_called = query_20b50_decode_cell(cell_address, x_fraction,
                                                  z_residual, &sample_bits);
        if (decoder_called && sample_bits != 0xffffffffUL) {
            combined = sample_bits + backward_coordinate;
            if (query_20b50_sign_clear(combined))
                return query_20b50_pack(combined, low_result);
        }

        cell_address -= 4UL;
        --scan_index;
        backward_coordinate += 0x40UL;
        if (!query_20b50_sign_clear(scan_index))
            return 0x7fff0000UL + low_result;
    }
}
