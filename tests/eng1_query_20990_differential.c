#include <windows.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/query_20990.h"

typedef unsigned char u8;
typedef unsigned long u32;
typedef long s32;
typedef s32 (__cdecl *NativeQuery20990)(u32, u32, const u8 *);

extern u32 g_eng1_grid_query_coefficient_prefix_5e0d0[26];
extern u8 g_actor_group_sequence_5e138[0x70];

static HMODULE oracle;
static unsigned int native_cases;
static unsigned int checks;
static unsigned int failures;
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

static s32 run_native(u32 x, u32 y, const u8 *program)
{
    NativeQuery20990 target;
    target = (NativeQuery20990)(void *)native_cell(0x20990UL);
    return target(x, y, program);
}

static void native_case(const char *label, u32 x, u32 y,
                        const u8 *program, s32 expected)
{
    s32 native_result;
    s32 source_result;
    ++native_cases;
    native_result = run_native(x, y, program);
    source_result = query_decode_20990(x, y, program);
    check(native_result == source_result, label);
    check(native_result == expected, "native result matches derived value");
    check(source_result == expected, "source result matches derived value");
    printf("native: %s => %ld\n", label, source_result);
}

static int write_native_dword(u32 rva, u32 value)
{
    DWORD previous;
    u8 *cell;
    cell = native_cell(rva);
    if (!VirtualProtect(cell, 4U, PAGE_EXECUTE_READWRITE, &previous))
        return 0;
    write32(cell, value);
    FlushInstructionCache(GetCurrentProcess(), cell, 4U);
    if (!VirtualProtect(cell, 4U, previous, &previous))
        return 0;
    return 1;
}

static void write_source_dword(u8 *cell, u32 value)
{
    write32(cell, value);
}

static void test_source_only_controls(void)
{
    u8 wrapped_program[8];
    u8 negative_program[8];
    u8 outside_lane_program[8];
    s32 value;

    memset(wrapped_program, 0, sizeof(wrapped_program));
    wrapped_program[0] = 1U;
    wrapped_program[1] = 1U;
    value = query_decode_20990(0x7fffffffUL, 0UL, wrapped_program);
    check(value == 2L,
          "source-only 32-bit multiply/pointer-offset wrap returns doubled sample");

    memset(negative_program, 0, sizeof(negative_program));
    negative_program[0] = 0xffU;
    incomplete_armed = 1;
    if (setjmp(incomplete_target) == 0) {
        (void)query_decode_20990(0UL, 0UL, negative_program);
        incomplete_armed = 0;
        check(0, "negative first tag does not read beyond measured owners");
    } else {
        incomplete_armed = 0;
        ++guard_cases;
        check(incomplete_rva == 0x5e1c0UL,
              "negative first tag reports first unowned low-index coefficient");
    }

    memset(outside_lane_program, 0, sizeof(outside_lane_program));
    outside_lane_program[0] = 13U;
    outside_lane_program[1] = 2U;
    outside_lane_program[2] = 3U;
    incomplete_armed = 1;
    if (setjmp(incomplete_target) == 0) {
        (void)query_decode_20990(0UL, 0UL, outside_lane_program);
        incomplete_armed = 0;
        check(0, "index-13 third lane does not exceed shared-owner prefix");
    } else {
        incomplete_armed = 0;
        ++guard_cases;
        check(incomplete_rva == 0x5e1a8UL,
              "index-13 third lane fails at the measured-owner boundary");
    }
}

int main(int argc, char **argv)
{
    u8 scalar_program[8];
    u8 row_program[8];
    u8 two_dimensional_program[32];
    u8 original_source_words[8];
    u8 original_native_words[8];
    u32 original_word0;
    u32 original_word1;

    setvbuf(stdout, (char *)0, _IONBF, 0);
    if (argc != 2) {
        printf("usage: queryfixture.exe ENG1.DLL\n");
        return 2;
    }
    oracle = LoadLibraryA(argv[1]);
    if (oracle == (HMODULE)0) {
        printf("LoadLibrary failed %08lx\n", GetLastError());
        return 3;
    }
    printf("fixture: mapped ENG1 at %p\n", oracle);
    if ((u32)(unsigned long)oracle != 0x10000000UL ||
        native_cell(0x20990UL)[0] != 0x8bU ||
        native_cell(0x5e138UL)[0] != 0x10U) {
        printf("unexpected mapped ENG1 base or native identity\n");
        return 4;
    }

    memset(scalar_program, 0, sizeof(scalar_program));
    scalar_program[0] = 0U;
    scalar_program[1] = 10U;
    scalar_program[2] = 30U;
    native_case("tag 0 scalar interpolation", 32UL, 0UL,
                scalar_program, 40L);
    scalar_program[1] = 0xffU;
    native_case("tag 0 negative sample sentinel", 32UL, 0UL,
                scalar_program, -1L);
    scalar_program[1] = 64U;
    native_case("tag 0 high sample sentinel", 32UL, 0UL,
                scalar_program, 0L);
    scalar_program[1] = 65U;
    native_case("tag 0 high sample 65 preserves doubled masked EAX", 32UL, 0UL,
                scalar_program, 2L);
    scalar_program[1] = 127U;
    native_case("tag 0 high sample 127 preserves doubled masked EAX", 32UL, 0UL,
                scalar_program, 126L);

    memset(row_program, 0, sizeof(row_program));
    row_program[0] = 1U;
    row_program[1] = 4U;
    row_program[2] = 20U;
    native_case("tag 1 one-dimensional interpolation", 16UL, 0UL,
                row_program, 24L);
    row_program[1] = 64U;
    native_case("tag 1 high sample 64 preserves doubled masked EAX", 0UL, 0UL,
                row_program, 0L);
    row_program[1] = 65U;
    native_case("tag 1 high sample 65 preserves doubled masked EAX", 0UL, 0UL,
                row_program, 2L);
    row_program[1] = 127U;
    native_case("tag 1 high sample 127 preserves doubled masked EAX", 0UL, 0UL,
                row_program, 126L);

    memset(two_dimensional_program, 0, sizeof(two_dimensional_program));
    two_dimensional_program[0] = 0x16U;
    two_dimensional_program[1] = 64U;
    native_case("tag 0x16 high sample 64 preserves doubled masked EAX", 0UL, 0UL,
                two_dimensional_program, 0L);
    two_dimensional_program[1] = 65U;
    native_case("tag 0x16 high sample 65 preserves doubled masked EAX", 0UL, 0UL,
                two_dimensional_program, 2L);
    two_dimensional_program[1] = 127U;
    native_case("tag 0x16 high sample 127 preserves doubled masked EAX", 0UL, 0UL,
                two_dimensional_program, 126L);
    two_dimensional_program[1] = 0U;
    two_dimensional_program[3] = 10U;
    two_dimensional_program[4] = 20U;
    two_dimensional_program[7] = 30U;
    two_dimensional_program[8] = 40U;
    two_dimensional_program[9] = 50U;
    original_word0 = read32(native_cell(0x5e138UL));
    original_word1 = read32(native_cell(0x5e13cUL));
    write32(original_native_words, original_word0);
    write32(original_native_words + 4, original_word1);
    write32(original_source_words, read32(g_actor_group_sequence_5e138));
    write32(original_source_words + 4, read32(g_actor_group_sequence_5e138 + 4));
    check(write_native_dword(0x5e138UL, 16UL) != 0,
          "native alias word0 temporarily writable");
    write_source_dword(g_actor_group_sequence_5e138, 16UL);
    check(write_native_dword(0x5e13cUL, 3UL) != 0,
          "native alias word1 temporarily writable");
    write_source_dword(g_actor_group_sequence_5e138 + 4, 3UL);
    native_case("tag 0x16 reads shared lane-2/lane-3 owner", 2UL, 16UL,
                two_dimensional_program, 32L);

    check(write_native_dword(0x5e138UL, 0UL) != 0,
          "native lane-2 alias mutation applied");
    write_source_dword(g_actor_group_sequence_5e138, 0UL);
    native_case("tag 0x16 observes a live lane-2 alias update", 2UL, 16UL,
                two_dimensional_program, 30L);

    check(write_native_dword(0x5e138UL, 16UL) != 0,
          "native lane-2 alias restored for stride control");
    write_source_dword(g_actor_group_sequence_5e138, 16UL);
    check(write_native_dword(0x5e13cUL, 4UL) != 0,
          "native lane-3 alias mutation applied");
    write_source_dword(g_actor_group_sequence_5e138 + 4, 4UL);
    native_case("tag 0x16 observes a live lane-3 stride update", 2UL, 16UL,
                two_dimensional_program, 37L);

    check(write_native_dword(0x5e138UL, read32(original_native_words)) != 0,
          "native lane-2 owner word restored");
    write_source_dword(g_actor_group_sequence_5e138,
                       read32(original_source_words));
    check(write_native_dword(0x5e13cUL, read32(original_native_words + 4)) != 0,
          "native lane-3 owner word restored");
    write_source_dword(g_actor_group_sequence_5e138 + 4,
                       read32(original_source_words + 4));
    check(memcmp(native_cell(0x5e138UL), original_native_words,
                 sizeof(original_native_words)) == 0,
          "native alias words restored exactly");
    check(memcmp(g_actor_group_sequence_5e138, original_source_words,
                 sizeof(original_source_words)) == 0,
          "source alias words restored exactly");

    { unsigned int native_checks=checks;
    test_source_only_controls();
    printf("query_20990_native_cases=%u checks=%u failures=%u\n",
           native_cases, native_checks, failures);
    printf("query_20990_source_control_cases=1 checks=1 failures=%u\nquery_20990_source_guard_cases=%u checks=2 failures=%u\n",
           failures,guard_cases,failures);
    }
    FreeLibrary(oracle);
    return failures == 0U && native_cases == 15U && guard_cases == 2U ? 0 : 1;
}
