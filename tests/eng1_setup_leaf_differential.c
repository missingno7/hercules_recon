#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/shared/engine_interface.h"
#include "../src/eng1/setup_shared_state.h"

extern void *g_normal_actor_pool;
void __cdecl dispatch_local_11fc0(void);
void __cdecl dispatch_local_15670(void);

typedef void (__cdecl *NoArgFunction)(void);
static HMODULE oracle;
static int cases, checks, failures;

static unsigned char *cell(unsigned long rva)
{
    return (unsigned char *)oracle + rva;
}

static void check(int ok, const char *why)
{
    ++checks;
    if (!ok) {
        ++failures;
        printf("FAIL %s\n", why);
    }
}

static void fill_pattern(unsigned char *p, unsigned long n, unsigned char seed)
{
    unsigned long i;
    for (i = 0; i != n; ++i)
        p[i] = (unsigned char)(seed + i * 13UL + (i >> 2));
}

static void test_011fc0(void)
{
    unsigned char native_context[0x100];
    unsigned char source_context[0x100];
    unsigned char native_nearby[0x20];
    unsigned char expected_nearby[0x20];
    unsigned short *native_words;
    unsigned long *native_dwords;
    NoArgFunction native_function;
    unsigned short *expected_words;

    fill_pattern(cell(0x70820), 0x1c, 0x35);
    fill_pattern((unsigned char *)g_cold_start_state_bytes_70820, 0x1c, 0x35);
    fill_pattern(cell(0x72190), 0x20, 0x53);
    memcpy(native_nearby, cell(0x72190), sizeof(native_nearby));
    fill_pattern(cell(0x72140), 24, 0x91);
    fill_pattern((unsigned char *)g_eng1_setup_clear_72140, 24, 0x91);
    *(unsigned short *)(void *)cell(0x72158) = 0x6bd2;
    g_eng1_setup_word_72158 = 0x6bd2;
    *(unsigned long *)(void *)cell(0x6ef5c) = 0x76543210UL;
    *(unsigned long *)(void *)cell(0x72188) = 0x89abcdefUL;
    g_eng1_setup_dword_6ef5c = 0x76543210UL;
    g_eng1_setup_dword_72188 = 0x89abcdefUL;

    fill_pattern(native_context, sizeof(native_context), 0xc7);
    fill_pattern(source_context, sizeof(source_context), 0xc7);
    *(void **)(void *)cell(0x70f24) = native_context;
    g_engine_interface.context_004 = (ResourceCallbackContext *)(void *)source_context;

    native_function = (NoArgFunction)(void *)cell(0x11fc0);
    native_function();
    dispatch_local_11fc0();

    check(!memcmp(cell(0x70820), g_cold_start_state_bytes_70820, 0x1c),
          "0x11fc0 cold-state WORD aliases and untouched prefix bytes");
    check(!memcmp(cell(0x72140), g_eng1_setup_clear_72140, 24),
          "0x11fc0 exact six-DWORD clear prefix");
    native_words = (unsigned short *)(void *)cell(0x72190);
    check(native_words[2] == g_eng1_setup_word_72194, "0x72194 WORD");
    check(native_words[6] == g_eng1_setup_word_7219c, "0x7219c WORD");
    check(native_words[7] == g_eng1_setup_word_7219e, "0x7219e WORD");
    check(native_words[8] == g_eng1_setup_word_721a0, "0x721a0 WORD");
    check(native_words[9] == g_eng1_setup_word_721a2, "0x721a2 WORD");
    check(native_words[10] == g_eng1_setup_word_721a4, "0x721a4 WORD");
    check(native_words[11] == g_eng1_setup_word_721a6, "0x721a6 WORD");
    memcpy(expected_nearby, native_nearby, sizeof(expected_nearby));
    expected_words = (unsigned short *)(void *)expected_nearby;
    expected_words[2] = 0;
    expected_words[6] = 0x007d;
    expected_words[7] = 0x00b9;
    expected_words[8] = 0;
    expected_words[9] = 0;
    expected_words[10] = 0;
    expected_words[11] = 0x0064;
    check(!memcmp(expected_nearby, cell(0x72190), sizeof(expected_nearby)),
          "0x72190 neighborhood preserves every untouched adjacent byte");
    check(*(unsigned short *)(void *)cell(0x72158) == g_eng1_setup_word_72158,
          "0x72140 six-DWORD clear stops before the separate 0x72158 WORD");
    native_dwords = (unsigned long *)(void *)cell(0x6ef5c);
    check(native_dwords[0] == g_eng1_setup_dword_6ef5c, "0x6ef5c DWORD");
    check(*(unsigned long *)(void *)cell(0x72188) == g_eng1_setup_dword_72188,
          "0x72188 DWORD");
    check(!memcmp(native_context, source_context, sizeof(native_context)),
          "captured context +0x22 store and surrounding bytes");
    ++cases;
}

static void test_015670(int alias_case)
{
    unsigned char native_pool[0x200];
    unsigned char source_pool[0x200];
    unsigned char native_collision[0x100];
    unsigned char source_collision[0x100];
    unsigned char *native_collision_view;
    unsigned char *source_collision_view;
    NoArgFunction native_function;
    unsigned long before_flags;

    fill_pattern(native_pool, sizeof(native_pool), (unsigned char)(0x21 + alias_case));
    memcpy(source_pool, native_pool, sizeof(native_pool));
    fill_pattern(native_collision, sizeof(native_collision), (unsigned char)(0x62 + alias_case));
    memcpy(source_collision, native_collision, sizeof(native_collision));
    if (alias_case) {
        native_collision_view = native_pool + 0xa2;
        source_collision_view = source_pool + 0xa2;
    } else {
        native_collision_view = native_collision;
        source_collision_view = source_collision;
    }

    *(void **)(void *)cell(0x72860) = native_pool;
    *(void **)(void *)cell(0x7217c) = native_collision_view;
    *(unsigned short *)(void *)cell(0x6ef94) = 0x79b3;
    g_normal_actor_pool = source_pool;
    g_collision_context = source_collision_view;
    g_eng1_setup_word_6ef94 = 0x79b3;
    before_flags = *(unsigned long *)(void *)(native_collision_view + 0x54);

    native_function = (NoArgFunction)(void *)cell(0x15670);
    native_function();
    dispatch_local_15670();

    check(!memcmp(native_pool, source_pool, sizeof(native_pool)),
          alias_case ? "0x15670 overlapping pool/context writes preserve native order" :
                       "0x15670 normal-pool +0xec byte");
    if (!alias_case)
        check(!memcmp(native_collision, source_collision, sizeof(native_collision)),
              "0x15670 collision DWORD and WORD effects");
    check(*(unsigned short *)(void *)cell(0x6ef94) == g_eng1_setup_word_6ef94 &&
          g_eng1_setup_word_6ef94 == 0, "0x6ef94 WORD clear");
    check((*(unsigned long *)(void *)(native_collision_view + 0x54) & 4UL) == 0,
          "0x15670 clears only DWORD flag bit 2");
    check(*(unsigned long *)(void *)(native_collision_view + 0x54) == (before_flags & 0xfffffffbUL),
          "0x15670 preserves remaining collision DWORD bits");
    check(*(unsigned short *)(void *)(native_collision_view + 0x4a) == 0x00a0,
          "0x15670 final +0x4a WORD store");
    if (alias_case)
        check(native_pool[0xec] == 0xa0,
              "later +0x4a WORD store overwrites the earlier aliased pool byte");
    ++cases;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    oracle = LoadLibraryExA(argv[1], NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!oracle) {
        printf("LoadLibraryExA failed %lu\n", GetLastError());
        return 3;
    }
    test_011fc0();
    test_015670(0);
    test_015670(1);
    printf("setup_leaf_cases=%d checks=%d failures=%d\n", cases, checks, failures);
    FreeLibrary(oracle);
    return failures ? 1 : 0;
}
