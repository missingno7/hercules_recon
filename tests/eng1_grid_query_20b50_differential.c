#include <windows.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/grid_query_20b50.h"
#include "../src/eng1/resource_runtime_data.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef long s32;
typedef u32 (__cdecl *Native20b50)(s32, s32, s32, u32);
typedef s32 (__cdecl *Native20990)(u32, u32, const u8 *);

static HMODULE oracle;
static u8 row_cells[32];
static u8 header_bytes[96];
static unsigned int checks;
static unsigned int failures;
static unsigned int native_cases;
static unsigned int guard_cases;
static jmp_buf incomplete_target;
static int incomplete_armed;
static u32 incomplete_rva;

static u8 *native_cell(u32 rva)
{
    return (u8 *)oracle + rva;
}

static u32 read32(const u8 *p)
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

/* Explicitly test-only call-through. Production source keeps 0x20990 typed
 * and separately owned; this fixture invokes the read-only oracle body. */


static void set_native_byte(u32 rva, u8 value)
{
    native_cell(rva)[0] = value;
}

static void set_native_word(u32 rva, u16 value)
{
    write16(native_cell(rva), value);
}

static void set_native_dword(u32 rva, u32 value)
{
    write32(native_cell(rva), value);
}

static void set_cell(unsigned int index, u32 value)
{
    write32(row_cells + index * 4U, value);
}

static void set_tagged_program(u32 tag, const u8 *program, unsigned int size)
{
    u8 *entry;
    u8 *target;
    u32 entry_offset;
    u32 target_offset;
    u16 relative;

    entry_offset = tag * 4UL - 2UL;
    target_offset = 32UL + tag * 8UL;
    entry = header_bytes + entry_offset;
    target = header_bytes + target_offset;
    relative = (u16)(target - entry);
    write16(entry, relative);
    memcpy(target, program, size);
}

static void prepare_state(void)
{
    unsigned int i;

    memset(row_cells, 0, sizeof(row_cells));
    memset(header_bytes, 0, sizeof(header_bytes));
    g_active_row_70d01 = 0;
    g_group_count_70ece = 1;
    g_element_width_70d14 = 4;
    g_header_pointer_70d34 = header_bytes;
    g_rebuilt_row_pointers_70dcc[0] = row_cells;
    for (i = 0; i < 32U; ++i)
        g_rebuilt_row_offsets_70e0c[i] = 0;
    g_rebuilt_row_offsets_70e0c[0] = 1;
    g_rebuilt_row_offsets_70e0c[16] = 0;

    set_native_byte(0x70d01UL, 0U);
    set_native_byte(0x70eceUL, 1U);
    set_native_word(0x70d14UL, 4U);
    set_native_dword(0x70d34UL, (u32)(unsigned long)header_bytes);
    set_native_dword(0x70dccUL, (u32)(unsigned long)row_cells);
    set_native_word(0x70e0cUL, 1U);
    set_native_word(0x70e2cUL, 0U);
}

static void compare_case(const char *label, s32 x, s32 y, s32 z, u32 step,
                         u32 expected)
{
    Native20b50 target;
    u32 native_result;
    u32 source_result;

    target = (Native20b50)(void *)native_cell(0x20b50UL);
    native_result = target(x, y, z, step);
    source_result = grid_scan_20b50(x, y, z, step);
    ++native_cases;
    check(native_result == source_result, label);
    check(native_result == expected, "native output matches derived value");
    printf("case: %s native=%08lx source=%08lx\n",
           label, native_result, source_result);
}

static void test_source_guard(void)
{
    u8 saved;
    volatile int returned;

    saved = g_group_count_70ece;
    g_group_count_70ece = 0;
    returned = 1;
    incomplete_armed = 1;
    if (setjmp(incomplete_target) == 0) {
        (void)grid_scan_20b50(0, 96, 0, 0);
        returned = 1;
    } else {
        returned = 0;
    }
    incomplete_armed = 0;
    g_group_count_70ece = (signed char)saved;
    check(!returned && incomplete_rva == 0x70eceUL,
          "nonpositive group count fails explicitly in source");
    ++guard_cases;
}

int main(int argc, char **argv)
{
    u8 program[3];
    u8 active;

    setvbuf(stdout, (char *)0, _IONBF, 0);
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
    if (native_cell(0x20990UL)[0] != 0x8bU) {
        printf("unexpected native identity bytes\n");
        return 4;
    }

    prepare_state();
    active = native_cell(0x70d01UL)[0];
    native_cell(0x70d01UL)[0] = 0xffU;
    g_active_row_70d01 = (signed char)-1;
    compare_case("active-row sentinel", 0, 96, 0, 0xdeadbeefUL,
                 0x7fff7fffUL);
    native_cell(0x70d01UL)[0] = active;
    g_active_row_70d01 = 0;

    set_cell(1U, 2U);
    program[0] = 0U;
    program[1] = 0U;
    program[2] = 0U;
    set_tagged_program(1UL, program, sizeof(program));
    compare_case("positive forward hit uses y-adjusted reverse seed",
                 0, 96, 0, 17U, 0UL);

    memset(row_cells, 0, sizeof(row_cells));
    program[0] = 0U;
    program[1] = 1U;
    program[2] = 0U;
    set_tagged_program(1UL, program, sizeof(program));
    set_cell(4U, 2U);
    compare_case("forward exhaustion reads the one-past-width cell",
                 0, 287, 0, 0U, 0x00017fffUL);

    memset(row_cells, 0, sizeof(row_cells));
    compare_case("x extent exclusive bound", 64, 96, 0, 0U,
                 0x7fff7fffUL);

    { unsigned int native_checks=checks;
    test_source_guard();
    printf("grid_query_20b50_native_cases=%u checks=%u failures=%u\n",
           native_cases, native_checks, failures);
    printf("grid_query_20b50_source_guard_cases=%u checks=%u failures=%u\n",
           guard_cases, checks-native_checks, failures);
    }
    FreeLibrary(oracle);
    return failures == 0U && native_cases == 4U && guard_cases == 1U ? 0 : 1;
}
