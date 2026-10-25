#include <windows.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/resource_runtime_data.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef long s32;
typedef u32 (__cdecl *GridQuery20d30)(s32, s32, s32, u32);
typedef void (__cdecl *RebuildRows20d30)(void);

extern u32 __cdecl grid_query_20d30(s32, s32, s32, u32);
extern void __cdecl resource_rebuild_group_rows(void);
extern u8 g_actor_group_sequence_5e138[0x70];
extern u32 g_eng1_grid_query_coefficient_prefix_5e0d0[26];
/* Link-only cell for the uncalled resource_rebuild_blob fixture branch. */
u8 g_mode_lookup_5df0b[4096];

static HMODULE oracle;
static unsigned int checks;
static unsigned int failures;
static unsigned int native_cases;
static unsigned int guard_cases;
static jmp_buf incomplete_target;
static int incomplete_armed;
static u32 incomplete_rva;
static u8 row_block[144];
static u8 header_block[128];
static u8 sequence_snapshot[112];
static u8 source_sequence_snapshot[112];
static void *source_pointer_snapshot[2];
static u16 source_offset_snapshot[32];

static LONG WINAPI report_exception(EXCEPTION_POINTERS *info)
{
    u32 *stack = (u32 *)(unsigned long)info->ContextRecord->Esp;
    printf("unhandled exception=%08lx address=%p\n",
           info->ExceptionRecord->ExceptionCode,
           info->ExceptionRecord->ExceptionAddress);
    printf("  eax=%08lx edx=%08lx esp=%08lx effective=%08lx\n",
           info->ContextRecord->Eax, info->ContextRecord->Edx,
           info->ContextRecord->Esp,
           info->ContextRecord->Edx + info->ContextRecord->Eax);
    printf("  slots+10=%08lx +14=%08lx +1c=%08lx +20=%08lx +24=%08lx +28=%08lx\n",
           stack[4], stack[5], stack[7], stack[8], stack[9], stack[10]);
    return EXCEPTION_EXECUTE_HANDLER;
}

static u8 *native_cell(u32 rva)
{
    return (u8 *)oracle + rva;
}

static u32 read32(u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) |
           ((u32)p[3] << 24);
}

static void write16(u8 *p, u16 value)
{
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
}

static void write32(u8 *p, u32 value)
{
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
    p[2] = (u8)(value >> 16);
    p[3] = (u8)(value >> 24);
}

static void check(int condition, const char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

void __cdecl eng1_incomplete_behavior(u32 rva, const char *reason)
{
    (void)reason;
    incomplete_rva = rva;
    if (incomplete_armed)
        longjmp(incomplete_target, 1);
    printf("unexpected incomplete path at %08lx\n", rva);
    ExitProcess(0xe0484731UL);
}

static void set_native_word(u32 rva, u16 value)
{
    write16(native_cell(rva), value);
}

static void set_native_dword(u32 rva, u32 value)
{
    write32(native_cell(rva), value);
}

static void write_grid_cell(unsigned int layer, unsigned int x_column,
                            unsigned int y_column, u8 tag)
{
    unsigned int cell = x_column * 4U + y_column;
    u8 *p = row_block + 8U + layer * 64U + cell * 4U;
    write32(p, (u32)tag << 24);
}

static void set_header_record(u8 tag, u16 packed, u8 sample_base)
{
    u8 *entry;
    u32 relative;
    u8 *packed_word;
    unsigned int i;

    entry = header_block + ((unsigned int)tag - 1U) * 4U;
    relative = 16UL + ((u32)tag - 1UL) * 4UL;
    write32(entry, relative);
    packed_word = entry + relative;
    write16(packed_word, packed);
    for (i = 0; i < 48U; ++i)
        packed_word[2U + i] = (u8)(sample_base + i);
}

static void prepare_state(void)
{
    RebuildRows20d30 native_builder;
    unsigned int i;

    memset(row_block, 0, sizeof(row_block));
    memset(header_block, 0, sizeof(header_block));
    write16(row_block, 1U); /* base column offset; native truncates its product */
    write16(row_block + 2, 4U);
    write16(row_block + 4, 4U);

    g_active_category_70cfe = 0;
    g_active_row_70d01 = 0;
    g_group_count_70ece = 2;
    g_element_width_70d14 = 4;
    g_allocated_row_slots_71e40[0] = row_block;
    g_header_pointer_70d40 = header_block;
    native_cell(0x70cfeUL)[0] = 0;
    native_cell(0x70d01UL)[0] = 0;
    native_cell(0x70eceUL)[0] = 2;
    set_native_word(0x70d14UL, 4U);
    set_native_dword(0x71e40UL, (u32)(unsigned long)row_block);
    set_native_dword(0x70d40UL, (u32)(unsigned long)header_block);

    printf("fixture: invoke native row producer\n");
    native_builder = (RebuildRows20d30)(void *)native_cell(0x208f0UL);
    native_builder();
    printf("fixture: invoke source row producer\n");
    resource_rebuild_group_rows();

    check((u32)(unsigned long)g_rebuilt_row_pointers_70dcc[0] ==
              read32(native_cell(0x70dccUL)),
          "real source and native row producers agree on row pointer 0");
    check((u32)(unsigned long)g_rebuilt_row_pointers_70dcc[1] ==
              read32(native_cell(0x70dd0UL)),
          "real source and native row producers agree on row pointer 1");
    check(g_rebuilt_row_offsets_70e0c[0] ==
              (u16)(native_cell(0x70e0cUL)[0] |
                    ((u16)native_cell(0x70e0cUL)[1] << 8)) &&
          g_rebuilt_row_offsets_70e0c[1] ==
              (u16)(native_cell(0x70e0eUL)[0] |
                    ((u16)native_cell(0x70e0eUL)[1] << 8)) &&
          g_rebuilt_row_offsets_70e0c[16] ==
              (u16)(native_cell(0x70e2cUL)[0] |
                    ((u16)native_cell(0x70e2cUL)[1] << 8)) &&
          g_rebuilt_row_offsets_70e0c[17] ==
              (u16)(native_cell(0x70e2eUL)[0] |
                    ((u16)native_cell(0x70e2eUL)[1] << 8)),
          "real source and native row producers agree on both offset lanes");

    for (i = 0; i < 4U; ++i) {
        write_grid_cell(0U, i, 0U, 1U);
        write_grid_cell(1U, i, 0U, 2U);
    }
    set_header_record(1U, 0U, 0xf9U);
    set_header_record(2U, 0U, 0xe8U);
    set_header_record(3U, 0U, 0xd0U);
    memcpy(sequence_snapshot, native_cell(0x5e138UL), sizeof(sequence_snapshot));
    memcpy(source_sequence_snapshot, g_actor_group_sequence_5e138,
           sizeof(source_sequence_snapshot));
    check(memcmp(source_sequence_snapshot, sequence_snapshot,
                 sizeof(sequence_snapshot)) == 0,
          "source sequence owner initially matches native overlap bytes");
    memcpy(source_pointer_snapshot, g_rebuilt_row_pointers_70dcc,
           sizeof(source_pointer_snapshot));
    memcpy(source_offset_snapshot, g_rebuilt_row_offsets_70e0c,
           sizeof(source_offset_snapshot));
}

static void compare_case(const char *label, s32 x, s32 y, s32 z, u32 step,
                         unsigned int layer, unsigned int x_column,
                         unsigned int y_column, u8 tag, u16 packed,
                         u8 sample_base)
{
    GridQuery20d30 native_fn;
    u32 native_result;
    u32 source_result;

    write_grid_cell(layer, x_column, y_column, tag);
    if (tag != 0U)
        set_header_record(tag, packed, sample_base);
    native_fn = (GridQuery20d30)(void *)native_cell(0x20d30UL);
    printf("case: %s\n", label);
    native_result = native_fn(x, y, z, step);
    source_result = grid_query_20d30(x, y, z, step);
    printf("  native=%04lx source=%04lx\n", native_result & 0xffffUL,
           source_result & 0xffffUL);
    check((native_result & 0xffffUL) == (source_result & 0xffffUL), label);
    ++native_cases;
}

static void compare_case_with_sequence_override(
    const char *label, s32 x, s32 y, s32 z, u32 step,
    unsigned int layer, unsigned int x_column, unsigned int y_column,
    u8 tag, u16 packed, u8 sample_base, u32 native_rva,
    u32 source_sequence_offset)
{
    u8 native_saved[4];
    u8 source_saved[4];

    memcpy(native_saved, native_cell(native_rva), sizeof(native_saved));
    memcpy(source_saved, g_actor_group_sequence_5e138 + source_sequence_offset,
           sizeof(source_saved));
    write32(native_cell(native_rva), 0UL);
    write32(g_actor_group_sequence_5e138 + source_sequence_offset, 0UL);
    compare_case(label, x, y, z, step, layer, x_column, y_column,
                 tag, packed, sample_base);
    memcpy(native_cell(native_rva), native_saved, sizeof(native_saved));
    memcpy(g_actor_group_sequence_5e138 + source_sequence_offset, source_saved,
           sizeof(source_saved));
}

static void test_native_cases(void)
{
    u8 active_row;
    unsigned int i;
    int prefix_matches;

    prefix_matches = 1;
    for (i = 0; i < 26U; ++i) {
        if (g_eng1_grid_query_coefficient_prefix_5e0d0[i] !=
            read32(native_cell(0x5e0d0UL + (u32)i * 4UL)))
            prefix_matches = 0;
    }
    check(prefix_matches,
          "all 26 source prefix DWORDs match file-backed native RVA 0x5e0d0..0x5e138");

    active_row = native_cell(0x70d01UL)[0];
    native_cell(0x70d01UL)[0] = 0xffU;
    g_active_row_70d01 = (signed char)-1;
    compare_case("0xff active-row sentinel returns low-WORD zero",
                 0, 0, 0, 17U, 0U, 0U, 0U, 0U, 0U, 0U);
    native_cell(0x70d01UL)[0] = active_row;
    g_active_row_70d01 = 0;

    compare_case("x extent is an unsigned exclusive bound",
                 320, -32, 0, 0U, 0U, 0U, 0U, 1U, 0U, 0xf9U);
    compare_case("y extent is an unsigned exclusive bound",
                 64, 224, 0, 0U, 0U, 0U, 0U, 1U, 0U, 0xf9U);
    compare_case("zero grid tag returns zero",
                 64, -32, 0, 0U, 0U, 0U, 0U, 0U, 0U, 0U);
    compare_case("header sample byte sign extends into returned WORD",
                 64, -32, 0, 0U, 0U, 0U, 0U, 1U, 0U, 0xf9U);
    compare_case("packed x nibble contributes signed-byte displacement",
                 112, -32, 0, 0xdeadbeefUL, 0U, 0U, 0U, 1U, 0x0001U, 0x80U);
    compare_case("packed y nibble contributes second-lane displacement",
                 64, 16, 0, 0U, 0U, 0U, 0U, 1U, 0x0010U, 0x80U);
    compare_case("packed high byte uses z-local interpolation",
                 64, -32, 32, 0U, 0U, 0U, 0U, 1U, 0x0100U, 0x80U);
    compare_case("combined packed interpolation and step inertness",
                 112, 16, 32, 0U, 0U, 0U, 0U, 1U, 0x0111U, 0x80U);
    compare_case("changing only step preserves the combined interpolation result",
                 112, 16, 32, 0xdeadbeefUL, 0U, 0U, 0U, 1U, 0x0111U, 0x80U);
    compare_case("signed negative z wraps to WORD and clamps to row 1",
                 64, -32, -1, 0U, 1U, 0U, 0U, 2U, 0U, 0x90U);
    compare_case("type 3 record reads its own relative header lane",
                 64, -32, 65, 0U, 1U, 0U, 0U, 3U, 0x0700U, 0xf0U);
    compare_case_with_sequence_override(
        "first coefficient index 7 reads a live shared-sequence alias",
        65, -32, 0, 0U, 0U, 0U, 0U, 1U, 0x0007U, 0xc0U,
        0x5e140UL, 8UL);
    compare_case_with_sequence_override(
        "fourth coefficient index 6 reads a live shared-sequence alias",
        64, 16, 0, 0U, 0U, 0U, 0U, 1U, 0x0016U, 0xc0U,
        0x5e13cUL, 4UL);

    check(memcmp(sequence_snapshot, native_cell(0x5e138UL),
                 sizeof(sequence_snapshot)) == 0,
          "native queries never write the overlapping payload-sequence bytes");
    check(memcmp(source_sequence_snapshot, g_actor_group_sequence_5e138,
                 sizeof(source_sequence_snapshot)) == 0,
          "source queries never write the shared payload-sequence owner");
    check(memcmp(source_pointer_snapshot, g_rebuilt_row_pointers_70dcc,
                 sizeof(source_pointer_snapshot)) == 0 &&
          memcmp(source_offset_snapshot, g_rebuilt_row_offsets_70e0c,
                 sizeof(source_offset_snapshot)) == 0,
          "native and source queries leave reconstructed row pointers/offsets unchanged");
}

static void expect_source_incomplete(u32 expected_rva, const char *label)
{
    volatile int returned;

    returned = 1;
    incomplete_armed = 1;
    if (setjmp(incomplete_target) == 0) {
        grid_query_20d30(64, -32, 0, 0U);
        returned = 1;
    } else {
        returned = 0;
    }
    incomplete_armed = 0;
    check(!returned && incomplete_rva == expected_rva, label);
    ++guard_cases;
}

static void expect_source_incomplete_packed(u32 expected_rva, u16 packed,
                                            const char *label)
{
    u16 saved_packed;
    volatile int returned;

    saved_packed = (u16)(header_block[16] | ((u16)header_block[17] << 8));
    write16(header_block + 16, packed);
    returned = 1;
    incomplete_armed = 1;
    if (setjmp(incomplete_target) == 0) {
        grid_query_20d30(64, -32, 0, 0U);
        returned = 1;
    } else {
        returned = 0;
    }
    incomplete_armed = 0;
    write16(header_block + 16, saved_packed);
    check(!returned && incomplete_rva == expected_rva, label);
    ++guard_cases;
}

static void test_source_guards(void)
{
    signed char saved_count;
    u8 packed_saved[2];
    u8 *saved_row_pointer;
    u8 *saved_header;

    saved_count = g_group_count_70ece;
    g_group_count_70ece = 0;
    expect_source_incomplete(0x70eceUL,
        "nonpositive signed group count cannot silently return a grid result");
    g_group_count_70ece = saved_count;

    saved_row_pointer = g_rebuilt_row_pointers_70dcc[0];
    saved_header = g_header_pointer_70d40;
    packed_saved[0] = header_block[16];
    packed_saved[1] = header_block[17];

    expect_source_incomplete_packed(0x5e1b0UL, 0x0e00U,
        "unowned high-byte coefficient index fails explicitly");
    expect_source_incomplete_packed(0x5e1acUL, 0x001dU,
        "unowned fourth-lane factor index fails explicitly");

    g_rebuilt_row_pointers_70dcc[0] = (u8 *)0;
    expect_source_incomplete(0x70dccUL,
        "missing selected row pointer is explicit");
    g_rebuilt_row_pointers_70dcc[0] = saved_row_pointer;

    g_header_pointer_70d40 = (u8 *)0;
    expect_source_incomplete(0x70d40UL,
        "missing nonzero-tag header is explicit");
    g_header_pointer_70d40 = saved_header;
    write16(header_block + 16, (u16)(packed_saved[0] |
                                     ((u16)packed_saved[1] << 8)));
}

int main(int argc, char **argv)
{
    setvbuf(stdout, (char *)0, _IONBF, 0);
    SetUnhandledExceptionFilter(report_exception);
    if (argc != 2) {
        printf("usage: neutralfixture.exe ENG1.DLL\n");
        return 2;
    }
    oracle = LoadLibraryA(argv[1]);
    if (oracle == (HMODULE)0) {
        printf("LoadLibrary failed %08lx\n", GetLastError());
        return 3;
    }
    printf("fixture: mapped ENG1 at %p\n", oracle);
    if (native_cell(0x20d30UL)[0] != 0xa0U ||
        native_cell(0x208f0UL)[0] != 0x83U) {
        printf("unexpected mapped ENG1 code bytes\n");
        return 4;
    }

    prepare_state();
    test_native_cases();
    test_source_guards();
    check(memcmp(sequence_snapshot, native_cell(0x5e138UL),
                 sizeof(sequence_snapshot)) == 0,
          "overlap snapshot remains unchanged after source guards");
    printf("grid_query_native_cases=%u checks=%u failures=%u\n",
           native_cases, checks, failures);
    printf("grid_query_source_guard_cases=%u failures=%u\n",
           guard_cases, failures);
    FreeLibrary(oracle);
    return failures == 0U && native_cases == 14U && guard_cases == 5U ? 0 : 1;
}
