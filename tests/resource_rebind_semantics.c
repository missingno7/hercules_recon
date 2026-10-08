#include <stdio.h>
#include <string.h>
#include "../calibration/resource_rebind.c"

EngineInterface g_engine_interface;
u8 *g_resource_root;
u8 g_root_count_70ed0;
u8 g_root_count_70ed1;
u8 g_root_count_70ed2;
u8 g_root_count_70ed3;
u8 g_packed_byte_70cf0;
u8 g_packed_byte_70cf1;
u8 g_packed_byte_70cf2;
u8 g_active_category_70cfe;
u8 g_mode_table_value_70d00;
u8 g_active_row_70d01;
u16 g_selected_width_70d12;
s16 g_element_width_70d14;
u8 *g_header_pointer_70d28;
u8 *g_header_pointer_70d2c;
u8 *g_header_pointer_70d30;
u8 *g_header_pointer_70d34;
u8 *g_header_pointer_70d38;
u8 *g_header_pointer_70d3c;
u8 *g_header_pointer_70d40;
u8 *g_header_pointer_70d44;
u8 *g_header_pointer_70d4c;
u8 *g_header_pointer_70d50;
u8 *g_header_pointer_70d54;
u8 *g_header_pointer_70d58;
u8 *g_header_pointer_70d6c;
u8 *g_header_pointer_70d48;
u8 g_clear_byte_70ecc;
u8 g_header_byte_70ecd;
u8 g_group_count_70ece;
u8 g_group_flag_70ecf;
u8 *g_root_group_rows_70d5c[4];
u16 g_group_reset_count_70dac;
u32 g_reset_dwords_70d70[15];
u16 g_reset_words_70dae[15];
u16 g_group_cumulative_70e4c[16];
u8 *g_rebuilt_row_pointers_70dcc[16];
u16 g_rebuilt_row_offsets_70e0c[32];
u8 *g_allocated_row_slots_71e40[32];
u8 g_mode_lookup_5df0b[256];

static ResourceCallbackContext context;
static u8 blob[0x400];
static u8 row_block[128];
static unsigned long checks;
static unsigned long failures;

static void check(const char *label, int passed)
{
    ++checks;
    if (!passed) {
        ++failures;
        printf("FAIL: %s\n", label);
    }
}

static void relative_field(u8 *field, u8 *target)
{
    *(u32 *)field = (u32)(target - field);
}

static void make_root_row(u8 *row, u8 groups, u8 flag,
                          u16 selected_width, s16 element_width,
                          s16 data_bytes, u32 a, u32 b, u32 c)
{
    row[0] = groups;
    row[2] = flag;
    *(u16 *)(row + 4) = selected_width;
    *(u16 *)(row + 6) = (u16)element_width;
    *(s16 *)(row + 0x0a) = data_bytes;
    *(u32 *)(row + 0x0c) = a;
    *(u32 *)(row + 0x10) = b;
    *(u32 *)(row + 0x14) = c;
}

int main(void)
{
    int i;

    memset(blob, 0, sizeof(blob));
    memset(row_block, 0, sizeof(row_block));
    memset(&context, 0, sizeof(context));
    context.unknown_000[0] = 1;
    g_engine_interface.context_004 = &context;
    g_resource_root = blob;
    g_active_category_70cfe = 0;
    g_active_row_70d01 = 0;
    g_mode_lookup_5df0b[28] = 0xa5;

    *(u16 *)(blob + 8) = 2;
    *(u16 *)(blob + 0x0a) = 1;
    *(u16 *)(blob + 0x0c) = 2;
    *(u16 *)(blob + 0x0e) = 3;
    *(u16 *)(blob + 0x10) = 4;
    *(u16 *)(blob + 0x12) = 5;
    relative_field(blob + 0x26, blob + 0x126);
    relative_field(blob + 0x2a, blob + 0x0aa);
    *(u32 *)(blob + 0x2e) = 0x00776655UL;
    relative_field(blob + 0x32, blob + 0x102);
    relative_field(blob + 0x36, blob + 0x182);
    relative_field(blob + 0x3a, blob + 0x07a);
    relative_field(blob + 0x3e, blob + 0x082);
    relative_field(blob + 0x42, blob + 0x08a);
    relative_field(blob + 0x46, blob + 0x092);
    make_root_row(blob + 0x102, 3, 0x44, 0x1234, 6, 12, 5, 7, 9);
    make_root_row(blob + 0x182, 2, 0x55, 0x2345, 4, 8, 3, 6, 0);
    make_root_row(blob + 0x1c2, 1, 0x66, 0x3456, 2, 4, 11, 0, 0);

    for (i = 0; i < 15; ++i) {
        g_reset_dwords_70d70[i] = 0x12345678UL;
        g_reset_words_70dae[i] = 0x5678;
    }

    resource_rebuild_blob();
    check("root parser stores measured low-byte header fields",
          g_root_count_70ed0 == 2 && g_root_count_70ed1 == 4 &&
          g_root_count_70ed2 == 5 && g_root_count_70ed3 == 1 &&
          g_header_byte_70ecd == 3 && g_clear_byte_70ecc == 0);
    check("root-relative fields resolve from each encoded field",
          g_header_pointer_70d28 == blob + 0x126 &&
          g_header_pointer_70d30 == blob + 0x0aa &&
          g_header_pointer_70d58 == blob + 0x07a &&
          g_header_pointer_70d54 == blob + 0x082 &&
          g_header_pointer_70d50 == blob + 0x08a &&
          g_header_pointer_70d44 == blob + 0x092);
    check("root cursor and descriptor bytes use their distinct offsets",
          g_header_pointer_70d34 == blob + 0x04a &&
          g_header_pointer_70d2c == blob + 0x04e &&
          g_header_pointer_70d38 == blob + 0x05e &&
          g_header_pointer_70d3c == blob + 0x06e &&
          g_header_pointer_70d4c == blob + 0x082 &&
          g_header_pointer_70d40 == blob + 0x086 &&
          g_packed_byte_70cf0 == 0x55 && g_packed_byte_70cf1 == 0x66 &&
          g_packed_byte_70cf2 == 0x77 && g_mode_table_value_70d00 == 0xa5);
    check("root entry table points into the movable blob and row setup runs",
          g_root_group_rows_70d5c[0] == blob + 0x102 &&
          g_root_group_rows_70d5c[1] == blob + 0x182 &&
          g_group_count_70ece == 3 && g_group_flag_70ecf == 0x44 &&
          g_selected_width_70d12 == 0x1234 && g_element_width_70d14 == 6);
    check("selected row establishes cumulative lengths and exact reset prefix",
          g_group_reset_count_70dac == 0 &&
          g_group_cumulative_70e4c[0] == 5 &&
          g_group_cumulative_70e4c[1] == 12 &&
          g_group_cumulative_70e4c[2] == 21 &&
          g_header_pointer_70d6c == blob + 0x118 &&
          g_header_pointer_70d48 == blob + 0x11a);
    for (i = 0; i < 15; ++i) {
        if (g_reset_dwords_70d70[i] != 0 || g_reset_words_70dae[i] != 0)
            failures++;
    }

    *(u16 *)(row_block + 0) = 10;
    *(s16 *)(row_block + 2) = 1;
    *(s16 *)(row_block + 4) = 3;
    *(s16 *)(row_block + 6) = 4;
    g_active_category_70cfe = 1;
    g_active_row_70d01 = 2;
    g_allocated_row_slots_71e40[10] = row_block;
    g_resource_root = 0;
    resource_rebuild_group_rows();
    check("row rebuild loads separate allocated owner slot after root moves",
          g_rebuilt_row_offsets_70e0c[0] == 1 &&
          g_rebuilt_row_offsets_70e0c[1] == 3 &&
          g_rebuilt_row_offsets_70e0c[2] == 4 &&
          g_rebuilt_row_offsets_70e0c[16] == 10 &&
          g_rebuilt_row_offsets_70e0c[17] == 10 &&
          g_rebuilt_row_offsets_70e0c[18] == 10 &&
          g_rebuilt_row_pointers_70dcc[0] == row_block + 8 &&
          g_rebuilt_row_pointers_70dcc[1] == row_block + 32 &&
          g_rebuilt_row_pointers_70dcc[2] == row_block + 104);

    g_root_group_rows_70d5c[2] = blob + 0x1c2;
    resource_select_group(2);
    check("selector accepts the signed upper boundary row table slot",
          g_group_count_70ece == 1 && g_group_flag_70ecf == 0x66 &&
          g_selected_width_70d12 == 0x3456 &&
          g_group_cumulative_70e4c[0] == 11);
    resource_select_group(3);
    check("selector above signed upper boundary leaves current row state",
          g_group_count_70ece == 1 && g_group_flag_70ecf == 0x66 &&
          g_selected_width_70d12 == 0x3456);

    printf("resource rebind fixture: %lu checks, %lu failures\n", checks, failures);
    return failures ? 1 : 0;
}
