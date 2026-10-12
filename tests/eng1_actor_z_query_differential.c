#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/behavioral/interpreters.h"
#include "../src/eng1/behavioral/grid_query_20b50.h"
#include "../src/eng1/behavioral/resource_runtime_data.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef void (__cdecl *NativeEf10)(Actor *);
typedef u32 (__cdecl *Native20b50)(long, long, long, u32);

extern void __cdecl eng1_native_code_target_000ef10(Actor *actor);

static HMODULE oracle;
static u8 row_cells[64];
static u8 header_bytes[96];
static unsigned int checks;
static unsigned int failures;
static unsigned int cases_run;

static void set_native_byte(u32 rva, u8 value);
static void set_native_word(u32 rva, u16 value);

typedef struct NativeSavedCells NativeSavedCells;
struct NativeSavedCells {
    u8 active_row;
    u8 group_count;
    u8 width[2];
    u8 header_pointer[4];
    u8 row_pointer[4];
    u8 extent[2];
    u8 base_offset[2];
};

typedef struct SourceSavedCells SourceSavedCells;
struct SourceSavedCells {
    signed char active_row;
    signed char group_count;
    short width;
    unsigned char *header_pointer;
    unsigned char *row_pointer;
    unsigned short extent;
    unsigned short base_offset;
};

static u8 *native_cell(u32 rva)
{
    return (u8 *)oracle + rva;
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

static void save_native_cells(NativeSavedCells *saved)
{
    memcpy(&saved->active_row, native_cell(0x70d01UL), 1);
    memcpy(&saved->group_count, native_cell(0x70eceUL), 1);
    memcpy(saved->width, native_cell(0x70d14UL), 2);
    memcpy(saved->header_pointer, native_cell(0x70d34UL), 4);
    memcpy(saved->row_pointer, native_cell(0x70dccUL), 4);
    memcpy(saved->extent, native_cell(0x70e0cUL), 2);
    memcpy(saved->base_offset, native_cell(0x70e2cUL), 2);
}

static void restore_native_cells(const NativeSavedCells *saved)
{
    memcpy(native_cell(0x70d01UL), &saved->active_row, 1);
    memcpy(native_cell(0x70eceUL), &saved->group_count, 1);
    memcpy(native_cell(0x70d14UL), saved->width, 2);
    memcpy(native_cell(0x70d34UL), saved->header_pointer, 4);
    memcpy(native_cell(0x70dccUL), saved->row_pointer, 4);
    memcpy(native_cell(0x70e0cUL), saved->extent, 2);
    memcpy(native_cell(0x70e2cUL), saved->base_offset, 2);
}

static void save_source_cells(SourceSavedCells *saved)
{
    saved->active_row = g_active_row_70d01;
    saved->group_count = g_group_count_70ece;
    saved->width = g_element_width_70d14;
    saved->header_pointer = g_header_pointer_70d34;
    saved->row_pointer = g_rebuilt_row_pointers_70dcc[0];
    saved->extent = g_rebuilt_row_offsets_70e0c[0];
    saved->base_offset = g_rebuilt_row_offsets_70e0c[16];
}

static void restore_source_cells(const SourceSavedCells *saved)
{
    g_active_row_70d01 = saved->active_row;
    g_group_count_70ece = saved->group_count;
    g_element_width_70d14 = saved->width;
    g_header_pointer_70d34 = saved->header_pointer;
    g_rebuilt_row_pointers_70dcc[0] = saved->row_pointer;
    g_rebuilt_row_offsets_70e0c[0] = saved->extent;
    g_rebuilt_row_offsets_70e0c[16] = saved->base_offset;
}

static void check(int condition, const char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

void __cdecl eng1_incomplete_behavior(unsigned long rva, const char *reason)
{
    printf("unexpected source guard at RVA %08lx: %s\n", rva, reason);
    ExitProcess(0xe0484731UL);
}

static void prepare_grid(int sentinel, unsigned int hit_index)
{
    unsigned char program[3];
    unsigned short relative;
    u32 row_address;
    u32 header_address;

    memset(row_cells, 0, sizeof(row_cells));
    memset(header_bytes, 0, sizeof(header_bytes));
    if (hit_index != 0xffffffffUL) {
        program[0] = 0;
        program[1] = 0;
        program[2] = 0;
        relative = (unsigned short)(40UL - 2UL);
        write16(header_bytes + 2, relative);
        memcpy(header_bytes + 40, program, sizeof(program));
        write32(row_cells + hit_index * 4UL, 2UL);
    }

    g_active_row_70d01 = sentinel ? (signed char)-1 : 0;
    g_group_count_70ece = 1;
    g_element_width_70d14 = 8;
    g_header_pointer_70d34 = header_bytes;
    g_rebuilt_row_pointers_70dcc[0] = row_cells;
    g_rebuilt_row_offsets_70e0c[0] = 1;
    g_rebuilt_row_offsets_70e0c[16] = 0;

    set_native_byte(0x70d01UL, sentinel ? 0xffU : 0U);
    set_native_byte(0x70eceUL, 1U);
    set_native_word(0x70d14UL, 8U);
    row_address = (u32)(unsigned long)row_cells;
    header_address = (u32)(unsigned long)header_bytes;
    write32(native_cell(0x70d34UL), header_address);
    write32(native_cell(0x70dccUL), row_address);
    write16(native_cell(0x70e0cUL), 1U);
    write16(native_cell(0x70e2cUL), 0U);
}

static void set_native_byte(u32 rva, u8 value)
{
    native_cell(rva)[0] = value;
}

static void set_native_word(u32 rva, u16 value)
{
    write16(native_cell(rva), value);
}

static void init_actor(Actor *actor, long y_tile, long z,
                       unsigned long flags)
{
    u8 *bytes;

    memset(actor, 0xa5, sizeof(*actor));
    actor->x = 0;
    actor->y = y_tile * 0x10000L;
    actor->z = z;
    actor->frame_group = (short)0x9234U;
    bytes = (u8 *)actor;
    bytes[0x25] = 0xf1U;
    write16(bytes + 0x28, 0xbeefU);
    write32(bytes + 0x54, (u32)flags);
}

static void compare_case(const char *label, long y_tile, long z,
                         int sentinel, unsigned int hit_index,
                         unsigned long initial_flags, u8 expected_state,
                         u16 expected_word28, unsigned long expected_flags)
{
    Actor native_actor;
    Actor source_actor;
    NativeEf10 target;
    Native20b50 query_target;
    u32 native_query_result;
    u32 source_query_result;

    prepare_grid(sentinel, hit_index);
    init_actor(&native_actor, y_tile, z, initial_flags);
    init_actor(&source_actor, y_tile, z, initial_flags);
    query_target = (Native20b50)(void *)native_cell(0x20b50UL);
    native_query_result = query_target(0L, y_tile, z >> 16, 0x9234UL);
    source_query_result = grid_scan_20b50(0L, y_tile, z >> 16, 0x9234UL);
    target = (NativeEf10)(void *)native_cell(0x0ef10UL);
    target(&native_actor);
    eng1_native_code_target_000ef10(&source_actor);

    ++cases_run;
    check(native_query_result == source_query_result,
          "0x20b50 dependency result agrees");
    printf("observed: native state=%02x word=%04x flags=%08lx; source state=%02x word=%04x flags=%08lx\n",
           ((u8 *)&native_actor)[0x25],
           *(u16 *)((u8 *)&native_actor + 0x28),
           *(u32 *)((u8 *)&native_actor + 0x54),
           ((u8 *)&source_actor)[0x25],
           *(u16 *)((u8 *)&source_actor + 0x28),
           *(u32 *)((u8 *)&source_actor + 0x54));
    printf("query: native=%08lx source=%08lx\n",
           native_query_result, source_query_result);
    check(memcmp(&native_actor, &source_actor, sizeof(native_actor)) == 0,
          label);
    check(((u8 *)&source_actor)[0x25] == expected_state,
          "actor +0x25 expected state");
    check(*(u16 *)((u8 *)&source_actor + 0x28) == expected_word28,
          "actor +0x28 expected WORD");
    check(*(u32 *)((u8 *)&source_actor + 0x54) == expected_flags,
          "actor +0x54 expected DWORD");
    printf("case: %s\n", label);
}

int main(int argc, char **argv)
{
    NativeSavedCells saved_native;
    SourceSavedCells saved_source;

    setvbuf(stdout, (char *)0, _IONBF, 0);
    if (argc != 2) {
        printf("usage: ef10fixture.exe ENG1.DLL\n");
        return 2;
    }
    oracle = LoadLibraryA(argv[1]);
    if (oracle == (HMODULE)0) {
        printf("LoadLibrary failed %08lx\n", GetLastError());
        return 3;
    }
    if (native_cell(0x0ef10UL)[0] != 0x56U) {
        printf("unexpected 0xef10 oracle bytes\n");
        FreeLibrary(oracle);
        return 4;
    }

    save_native_cells(&saved_native);
    save_source_cells(&saved_source);

    compare_case("boundary z=-0x100000 enters query path", 96L,
                 -0x100000L, 0, 1U, 0xa1b2c3a5UL,
                 0xf1U, 0U, 0xa1b2c3adUL);
    compare_case("one below cutoff takes short flag update", 96L,
                 -0x100001L, 0, 0xffffffffUL, 0xa1b2c3adUL,
                 0xf3U, 0xbeefU, 0xa1b2c3a5UL);
    compare_case("grid result 0x100 is inclusive success", 96L,
                 -0x100000L, 0, 5U, 0xa1b2c3a5UL,
                 0xf1U, 0x100U, 0xa1b2c3adUL);
    compare_case("grid result 0x101 clears bit 3 and preserves WORD", 159L,
                 -0x100000L, 0, 6U, 0xa1b2c3adUL,
                 0xf1U, 0xbeefU, 0xa1b2c3a5UL);
    compare_case("grid sentinel sets actor state and clears bit 3", 96L,
                 -0x100000L, 1, 0xffffffffUL, 0xa1b2c3adUL,
                 0xf3U, 0xbeefU, 0xa1b2c3a5UL);

    restore_source_cells(&saved_source);
    restore_native_cells(&saved_native);
    FreeLibrary(oracle);
    printf("eng1_call_ef10_native_cases=%u checks=%u failures=%u\n",
           cases_run, checks, failures);
    return cases_run == 5U && checks == 25U && failures == 0U ? 0 : 1;
}
