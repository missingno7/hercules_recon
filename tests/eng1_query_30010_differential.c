#include <windows.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/initial_spatial_roots.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef u32 (__cdecl *Query30010)(u32, u32, u32, u32);

/* Isolated fixture definition for the existing frame_data.c pointer cell. */
u32 g_configuration_dword_713bc;
extern u32 __cdecl dispatch_local_30010(u32, u32, u32, u32);

static HMODULE oracle;
static unsigned int cases, checks, failures;
static u32 query_index;
static u32 query_args[4];
static unsigned int query_calls;
static int query_changes_table;
static u32 query_replacement_table;
static jmp_buf incomplete_target;
static int incomplete_armed;
static u32 incomplete_rva;

static u8 *native_cell(u32 rva)
{ return (u8 *)oracle + rva; }

static void check(int condition, const char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

u32 __cdecl eng1_native_code_target_002a170(u32 x, u32 y, u32 z, u32 plane)
{
    ++query_calls;
    query_args[0] = x;
    query_args[1] = y;
    query_args[2] = z;
    query_args[3] = plane;
    if (query_changes_table)
        g_configuration_dword_713bc = query_replacement_table;
    return query_index;
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

static void compare_native_collision_prefix(u32 rva, const u16 *source,
                                            unsigned int words)
{
    unsigned int i;
    for (i = 0; i < words; ++i)
        check(source[i] == *(u16 *)(void *)native_cell(rva + i * 2UL),
              "owned collision-map source WORD matches native prefix");
}

/* Keep the real 0x2a170 call safe at its native zero-index path.  Nonzero
 * result behavior is tested separately with an explicitly controlled source
 * dependency spy; it is not represented as a native differential. */
static u32 native_zero_case(u32 mask)
{
    Query30010 native_fn = (Query30010)(void *)native_cell(0x30010UL);

    *(u32 *)(void *)native_cell(0x71b20UL) = 0UL;
    *(u8 *)(void *)native_cell(0x70d01UL) = 0xffU;
    *(u32 *)(void *)native_cell(0x713bcUL) =
        (u32)(void *)native_cell(0x4f1e0UL);
    return native_fn(0UL, 0UL, 0UL, mask);
}

/* Keep the queried index zero while varying the table WORD independently.
 * This would fail an implementation returning index&mask instead of WORD&mask. */
static void native_table_word_case(u16 word, u32 mask, u32 expected)
{
    u16 native_saved = *(u16 *)(void *)native_cell(0x4f1e0UL);
    u16 source_saved = g_eng1_collision_map_04f1e0[0];
    u32 native_result, source_result;
    *(u16 *)(void *)native_cell(0x4f1e0UL) = word;
    g_eng1_collision_map_04f1e0[0] = word;
    native_result = native_zero_case(mask);
    g_configuration_dword_713bc = (u32)(void *)g_eng1_collision_map_04f1e0;
    query_index = 0; query_changes_table = 0;
    source_result = dispatch_local_30010(0UL, 0UL, 0UL, mask);
    check(native_result == source_result && source_result == expected,
          "native table WORD and low mask determine return independently of index");
    *(u16 *)(void *)native_cell(0x4f1e0UL) = native_saved;
    g_eng1_collision_map_04f1e0[0] = source_saved;
    ++cases;
}

static void source_query_case(u32 index, const u16 *table, u32 mask,
                              u32 expected, const char *label)
{
    u32 x = 0x12345678UL, y = 0x87654321UL, z = 0xabcdef01UL;
    u32 result;
    g_configuration_dword_713bc = (u32)(void *)table;
    query_index = index;
    query_calls = 0;
    query_changes_table = 0;
    result = dispatch_local_30010(x, y, z, mask);
    check(result == expected, label);
    check(query_calls == 1 && query_args[0] == x && query_args[1] == y &&
          query_args[2] == z && query_args[3] == 0,
          "forwards x/y/z and literal zero to 0x2a170 dependency");
    ++cases;
}

static void source_live_table_reload_case(void)
{
    u32 result;
    u16 saved = g_eng1_collision_map_04f1e0[25];
    g_eng1_collision_map_04f1e0[25] = 0x1234U;
    g_configuration_dword_713bc =
        (u32)(void *)g_eng1_collision_map_0616f8;
    query_index = 25UL;
    query_calls = 0;
    query_changes_table = 1;
    query_replacement_table = (u32)(void *)g_eng1_collision_map_04f1e0;
    result = dispatch_local_30010(0x06400000UL, 0UL, 0UL, 0xffffUL);
    query_changes_table = 0;
    check(result == 0x1234UL && query_calls == 1,
          "reloads 0x713bc after the dependency changes the selected table");
    g_eng1_collision_map_04f1e0[25] = saved;
    ++cases;
}

static void guard_cases(void)
{
    u16 unknown_table[4] = { 0, 0, 0, 0 };
    int jumped;

    g_configuration_dword_713bc =
        (u32)(void *)g_eng1_collision_map_04f1e0;
    query_index = 32UL;
    incomplete_armed = 1;
    jumped = setjmp(incomplete_target);
    if (jumped == 0) {
        (void)dispatch_local_30010(0UL, 0UL, 0UL, 1UL);
        check(0, "measured collision-map overrun stops before the read");
    } else {
        check(incomplete_rva == 0x4f1e0UL,
              "known-prefix overrun reports the source owner RVA");
    }

    g_configuration_dword_713bc = (u32)(void *)unknown_table;
    query_index = 0UL;
    jumped = setjmp(incomplete_target);
    if (jumped == 0) {
        (void)dispatch_local_30010(0UL, 0UL, 0UL, 1UL);
        check(0, "unowned collision-map pointer stops before the read");
    } else {
        check(incomplete_rva == 0x713bcUL,
              "unowned pointer reports the selecting pointer-cell RVA");
    }
    incomplete_armed = 0;
    ++cases;
}

int main(int argc, char **argv)
{
    u32 native_result;
    unsigned int data_checks, native_checks, native_cases, spy_checks, spy_cases;
    if (argc != 2)
        return 2;
    oracle = LoadLibraryA(argv[1]);
    if (oracle == 0) {
        printf("LoadLibrary failed: %lu\n", (u32)GetLastError());
        return 3;
    }

    compare_native_collision_prefix(0x4f1e0UL,
        g_eng1_collision_map_04f1e0, 32U);
    compare_native_collision_prefix(0x616f8UL,
        g_eng1_collision_map_0616f8, 32U);

    data_checks = checks;
    native_result = native_zero_case(0xffffffffUL);
    g_configuration_dword_713bc =
        (u32)(void *)g_eng1_collision_map_04f1e0;
    query_index = 0UL;
    query_calls = 0;
    query_changes_table = 0;
    check(native_result == dispatch_local_30010(0UL, 0UL, 0UL, 0xffffffffUL),
          "native and candidate agree on the controlled zero-index path");
    ++cases;
    native_table_word_case(0x1234U, 0x00f0UL, 0x0030UL);
    native_table_word_case(0x9234U, 0xffffffffUL, 0UL);
    native_table_word_case(0x1234U, 0xabcd00f0UL, 0x0030UL);

    native_checks = checks - data_checks;
    native_cases = cases;
    source_query_case(7UL, g_eng1_collision_map_04f1e0, 7UL, 7UL,
        "low-bit collision entry intersects the incoming mask");
    source_query_case(7UL, g_eng1_collision_map_04f1e0, 8UL, 0UL,
        "nonmatching low-bit mask returns zero");
    source_query_case(7UL, g_eng1_collision_map_04f1e0, 0xabcd0007UL, 7UL,
        "upper WORD of incoming mask is ignored");
    source_query_case(25UL, g_eng1_collision_map_0616f8, 0xffffUL, 0UL,
        "table WORD bit 15 suppresses the index result");
    source_live_table_reload_case();
    spy_checks = checks - data_checks - native_checks;
    spy_cases = cases - native_cases;
    guard_cases();

    FreeLibrary(oracle);
    printf("query_native_cases=%u checks=%u failures=%u data_word_checks=%u\n", native_cases, native_checks, failures, data_checks);
    printf("query_source_spy_cases=%u checks=%u failures=%u\n", spy_cases, spy_checks, failures);
    printf("query_source_guard_cases=%u failures=%u\n", checks-data_checks-native_checks-spy_checks, failures);
    return failures == 0 ? 0 : 1;
}
