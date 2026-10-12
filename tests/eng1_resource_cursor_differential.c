#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../calibration/resource_rebind.c"

EngineInterface g_engine_interface;
u8 *g_resource_root;
u8 g_root_count_70ed0, g_root_count_70ed1, g_root_count_70ed2, g_root_count_70ed3;
u8 g_packed_byte_70cf0, g_packed_byte_70cf1, g_packed_byte_70cf2;
u8 g_active_category_70cfe, g_mode_table_value_70d00, g_active_row_70d01;
u16 g_selected_width_70d12;
s16 g_element_width_70d14;
u8 *g_header_pointer_70d28, *g_header_pointer_70d2c, *g_header_pointer_70d30;
u8 *g_header_pointer_70d34, *g_header_pointer_70d38, *g_header_pointer_70d3c;
u8 *g_header_pointer_70d40, *g_header_pointer_70d44, *g_header_pointer_70d4c;
u8 *g_header_pointer_70d50, *g_header_pointer_70d54, *g_header_pointer_70d58;
u8 *g_header_pointer_70d6c, *g_header_pointer_70d48;
u8 g_clear_byte_70ecc, g_header_byte_70ecd, g_group_count_70ece, g_group_flag_70ecf;
u8 *g_root_group_rows_70d5c[4];
u16 g_group_reset_count_70dac;
u32 g_reset_dwords_70d70[15];
u16 g_reset_words_70dae[15];
u16 g_group_cumulative_70e4c[16];
u8 *g_rebuilt_row_pointers_70dcc[16];
u16 g_rebuilt_row_offsets_70e0c[32];
u8 *g_allocated_row_slots_71e40[32];
u8 g_mode_lookup_5df0b[256];

static int checks, failures;
static HMODULE oracle;
static u8 *source_blob, *native_blob;
static u8 *cell(unsigned long rva) { return (u8 *)oracle + rva; }
static void check(int ok, const char *label)
{ ++checks; if (!ok) { ++failures; printf("FAIL: %s\n", label); } }
static long offset_of(u8 *p, u8 *base) { return (long)(p - base); }
static void check_ptr(const char *label, unsigned long rva, u8 *source_ptr)
{
    u8 *native_ptr = *(u8 **)cell(rva);
    check(offset_of(source_ptr, source_blob) == offset_of(native_ptr, native_blob), label);
}

int main(int argc, char **argv)
{
    FILE *f; long length; u8 *native_context, *source_context;
    void (__cdecl *native_rebuild)(void);
    unsigned long i;
    if (argc != 3) return 2;
    oracle = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (!oracle) { printf("load_error=%lu\n", GetLastError()); return 3; }
    f = fopen(argv[2], "rb"); if (!f) return 4;
    if (fseek(f, 0, SEEK_END) != 0 || (length = ftell(f)) <= 0 || fseek(f, 0, SEEK_SET) != 0) return 5;
    source_blob = (u8 *)malloc((size_t)length); native_blob = (u8 *)malloc((size_t)length);
    source_context = (u8 *)calloc(1, 0x40); native_context = (u8 *)calloc(1, 0x40);
    if (!source_blob || !native_blob || !source_context || !native_context) return 6;
    if (fread(source_blob, 1, (size_t)length, f) != (size_t)length) return 7;
    fclose(f); memcpy(native_blob, source_blob, (size_t)length);

    memset(&g_engine_interface, 0, sizeof(g_engine_interface));
    g_engine_interface.context_004 = (ResourceCallbackContext *)source_context;
    g_resource_root = source_blob; g_active_category_70cfe = 0;
    g_mode_lookup_5df0b[0] = *cell(0x5df0b);

    *(void **)(cell(0x70f20) + 4) = native_context;
    *(u8 **)(cell(0x70ed4)) = native_blob;
    *(signed char *)cell(0x70cfe) = 0;
    native_rebuild = (void (__cdecl *)(void))cell(0x20670);
    native_rebuild();
    resource_rebuild_blob();

#define CHECK_BYTE(rva, value, label) check(*(u8 *)cell(rva) == (u8)(value), label)
#define CHECK_WORD(rva, value, label) check(*(u16 *)cell(rva) == (u16)(value), label)
    CHECK_BYTE(0x70ed0, g_root_count_70ed0, "root count byte");
    CHECK_BYTE(0x70ed1, g_root_count_70ed1, "root count byte +1");
    CHECK_BYTE(0x70ed2, g_root_count_70ed2, "root count byte +2");
    CHECK_BYTE(0x70ed3, g_root_count_70ed3, "root count byte +3");
    CHECK_BYTE(0x70cf0, g_packed_byte_70cf0, "packed header byte 0");
    CHECK_BYTE(0x70cf1, g_packed_byte_70cf1, "packed header byte 1");
    CHECK_BYTE(0x70cf2, g_packed_byte_70cf2, "packed header byte 2");
    CHECK_BYTE(0x70d00, g_mode_table_value_70d00, "mode lookup byte");
    CHECK_BYTE(0x70ecc, g_clear_byte_70ecc, "clear byte");
    CHECK_BYTE(0x70ecd, g_header_byte_70ecd, "header byte");
    CHECK_BYTE(0x70ece, g_group_count_70ece, "selected row group count");
    CHECK_BYTE(0x70ecf, g_group_flag_70ecf, "selected row flags");
    CHECK_WORD(0x70d12, g_selected_width_70d12, "selected width");
    CHECK_WORD(0x70d14, g_element_width_70d14, "element width");
    CHECK_WORD(0x70dac, g_group_reset_count_70dac, "group reset count");
    check(*(u16 *)cell(0x70e4c) == g_group_cumulative_70e4c[0], "group cumulative[0]");
    for (i = 0; i < 11; ++i)
        check(*(u16 *)(cell(0x70e4c) + i * 2) == g_group_cumulative_70e4c[i], "group cumulative selected prefix");
    check(!memcmp(cell(0x70d70), g_reset_dwords_70d70, sizeof(g_reset_dwords_70d70)), "15 reset dwords");
    check(!memcmp(cell(0x70dae), g_reset_words_70dae, sizeof(g_reset_words_70dae)), "15 reset words");
    check_ptr("root row pointer table field", 0x70d5c, g_root_group_rows_70d5c[0]);
    check_ptr("header pointer 70d28", 0x70d28, g_header_pointer_70d28);
    check_ptr("header pointer 70d30", 0x70d30, g_header_pointer_70d30);
    check_ptr("header pointer 70d34", 0x70d34, g_header_pointer_70d34);
    check_ptr("header pointer 70d58", 0x70d58, g_header_pointer_70d58);
    check_ptr("header pointer 70d54", 0x70d54, g_header_pointer_70d54);
    check_ptr("header pointer 70d50", 0x70d50, g_header_pointer_70d50);
    check_ptr("header pointer 70d44", 0x70d44, g_header_pointer_70d44);
    check_ptr("header pointer 70d2c", 0x70d2c, g_header_pointer_70d2c);
    check_ptr("header pointer 70d38", 0x70d38, g_header_pointer_70d38);
    check_ptr("header pointer 70d3c", 0x70d3c, g_header_pointer_70d3c);
    check_ptr("header pointer 70d4c", 0x70d4c, g_header_pointer_70d4c);
    check_ptr("header pointer 70d40", 0x70d40, g_header_pointer_70d40);
    check_ptr("group header pointer 70d6c", 0x70d6c, g_header_pointer_70d6c);
    check_ptr("group header pointer 70d48", 0x70d48, g_header_pointer_70d48);
    check(offset_of(g_root_group_rows_70d5c[0], source_blob) == 0x26cb0, "FOREST native row resolves at file +0x26cb0");
    check(g_group_count_70ece == 11, "FOREST selected row reports eleven groups");
    printf("resource_cursor_native_diff checks=%d failures=%d source_row=%lx native_row=%lx\n",
        checks, failures, (unsigned long)offset_of(g_root_group_rows_70d5c[0], source_blob),
        (unsigned long)offset_of(*(u8 **)cell(0x70d5c), native_blob));
    return failures ? 1 : 0;
}
