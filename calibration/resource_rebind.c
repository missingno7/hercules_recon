#include "engine_interface.h"

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned long u32;

extern u8 *g_resource_root;
extern u8 g_root_count_70ed0;
extern u8 g_root_count_70ed1;
extern u8 g_root_count_70ed2;
extern u8 g_root_count_70ed3;

extern u8 g_packed_byte_70cf0;
extern u8 g_packed_byte_70cf1;
extern u8 g_packed_byte_70cf2;
extern u8 g_active_category_70cfe;
extern u8 g_mode_table_value_70d00;
extern u8 g_active_row_70d01;
extern u16 g_selected_width_70d12;
extern s16 g_element_width_70d14;

extern u8 *g_header_pointer_70d28;
extern u8 *g_header_pointer_70d2c;
extern u8 *g_header_pointer_70d30;
extern u8 *g_header_pointer_70d34;
extern u8 *g_header_pointer_70d38;
extern u8 *g_header_pointer_70d3c;
extern u8 *g_header_pointer_70d40;
extern u8 *g_header_pointer_70d44;
extern u8 *g_header_pointer_70d4c;
extern u8 *g_header_pointer_70d50;
extern u8 *g_header_pointer_70d54;
extern u8 *g_header_pointer_70d58;
extern u8 *g_header_pointer_70d6c;
extern u8 *g_header_pointer_70d48;

extern u8 g_clear_byte_70ecc;
extern u8 g_header_byte_70ecd;
extern u8 g_group_count_70ece;
extern u8 g_group_flag_70ecf;
extern u8 *g_root_group_rows_70d5c[];
extern u16 g_group_reset_count_70dac;
extern u32 g_reset_dwords_70d70[];
extern u16 g_reset_words_70dae[];
extern u16 g_group_cumulative_70e4c[];
extern u8 *g_rebuilt_row_pointers_70dcc[];
extern u16 g_rebuilt_row_offsets_70e0c[];
extern u8 *g_allocated_row_slots_71e40[];
extern u8 g_mode_lookup_5df0b[];

void __cdecl resource_select_group(int selector);

#define RELATIVE_FIELD(field) \
    ((field) + ((*(u32 *)(field) >> 2) * 4UL))

void __cdecl resource_rebuild_blob(void)
{
    u8 *root;
    u8 *cursor;
    u16 *header_words;
    u16 word_08;
    u16 word_0a;
    u16 word_0c;
    u16 word_0e;
    u16 word_10;
    u16 word_12;
    u32 packed;
    int i;

    root = g_resource_root;
    header_words = (u16 *)(root + 8);
    word_08 = header_words[0];
    word_0a = header_words[1];
    word_0c = header_words[2];
    word_0e = header_words[3];
    word_10 = header_words[4];
    word_12 = header_words[5];

    g_root_count_70ed0 = (u8)word_08;
    g_root_count_70ed1 = (u8)word_10;
    g_root_count_70ed2 = (u8)word_12;
    g_root_count_70ed3 = (u8)word_0a;
    g_header_byte_70ecd = (u8)word_0e;
    g_clear_byte_70ecc = 0;

    cursor = root + 0x26;
    g_header_pointer_70d28 = RELATIVE_FIELD(cursor);
    cursor += 4;
    g_header_pointer_70d30 = RELATIVE_FIELD(cursor);
    cursor += 4;

    packed = *(u32 *)cursor;
    g_packed_byte_70cf0 = (u8)packed;
    g_packed_byte_70cf1 = (u8)(packed >> 8);
    g_packed_byte_70cf2 = (u8)(packed >> 16);
    cursor += 4;

    for (i = 0; i < (int)word_08; ++i) {
        g_root_group_rows_70d5c[i] = RELATIVE_FIELD(cursor);
        cursor += 4;
    }

    g_header_pointer_70d58 = RELATIVE_FIELD(cursor);
    cursor += 4;
    g_header_pointer_70d54 = RELATIVE_FIELD(cursor);
    cursor += 4;
    g_header_pointer_70d50 = RELATIVE_FIELD(cursor);
    cursor += 4;
    g_header_pointer_70d44 = RELATIVE_FIELD(cursor);
    cursor += 4;

    g_header_pointer_70d34 = cursor;
    g_header_pointer_70d2c = cursor + (long)word_0c * 4L - 4L;
    g_header_pointer_70d38 = cursor + ((long)word_0e + word_0c) * 4L;
    g_header_pointer_70d3c = cursor + ((long)word_10 + word_0e + word_0c) * 4L;
    g_header_pointer_70d4c = cursor + ((long)word_12 + word_10 + word_0e + word_0c) * 4L;
    g_header_pointer_70d40 = cursor + ((long)word_0a + word_10 + word_12 + word_0e + word_0c) * 4L;

    g_mode_table_value_70d00 = g_mode_lookup_5df0b[
        (int)(s8)g_engine_interface.context_004->unknown_000[0] * 28];
    resource_select_group((int)(s8)g_active_category_70cfe);
}

void __cdecl resource_select_group(int selector)
{
    int count;
    int i;
    u8 *row;
    s16 relative_end;
    u32 total;
    u8 group_count;

    count = (int)(s8)g_root_count_70ed0;
    if (selector > count)
        return;

    row = g_root_group_rows_70d5c[selector];
    group_count = row[0];
    g_group_count_70ece = group_count;
    g_group_flag_70ecf = row[2];
    g_selected_width_70d12 = *(u16 *)(row + 4);
    g_element_width_70d14 = (s16)*(u16 *)(row + 6);
    g_group_reset_count_70dac = 0;

    relative_end = *(s16 *)(row + 0x0a);
    g_header_pointer_70d6c = row + 0x0a + relative_end;

    total = 0;
    for (i = 0; i < (int)(s8)group_count; ++i) {
        total += *(u32 *)(row + 0x0c + i * 4);
        g_group_cumulative_70e4c[i] = (u16)total;
    }

    g_header_pointer_70d48 = row + (int)(s8)group_count * 4 + 0x0c;
    for (i = 0; i < 15; ++i) {
        g_reset_dwords_70d70[i] = 0;
        g_reset_words_70dae[i] = 0;
    }
}

void __cdecl resource_rebuild_group_rows(void)
{
    int category;
    int row_number;
    int count;
    int column_step;
    int i;
    u16 base_offset;
    s16 width;
    s16 row_offset;
    u8 *block;

    category = (int)(s8)g_active_category_70cfe;
    row_number = (int)(s8)g_active_row_70d01;
    block = g_allocated_row_slots_71e40[category * 8 + row_number];
    base_offset = *(u16 *)block;
    count = (int)(s8)g_group_count_70ece;
    if (count <= 0)
        return;

    column_step = (count + 2) & ~1;
    width = g_element_width_70d14;
    for (i = 0; i < count; ++i) {
        row_offset = *(s16 *)(block + 2 + i * 2);
        g_rebuilt_row_offsets_70e0c[i] = (u16)row_offset;
        g_rebuilt_row_offsets_70e0c[16 + i] = base_offset;
        g_rebuilt_row_pointers_70dcc[i] = block + column_step * 2;
        column_step += (long)width * (long)row_offset * 2L;
    }
}
