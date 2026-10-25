#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/interpreters.h"
#include "../src/eng1/actor_control_tables.h"

extern void __cdecl eng1_native_code_target_0041000(Actor *child,
                                           unsigned long category);

typedef void (__cdecl *NativeEntry)(Actor *, unsigned long);

typedef struct TestCase TestCase;
struct TestCase {
    const char *name;
    unsigned long category;
    unsigned short column2;
    unsigned short column3;
    unsigned short expected_frame_word;
    unsigned short expected_frame_index;
};

static HMODULE g_oracle;
static unsigned int g_checks;
static unsigned int g_failures;
static unsigned int g_cases;
static unsigned short g_saved_native_rows[18][6];
static unsigned short g_saved_source_rows[18][6];
static unsigned long g_frame_tokens[16];
static void *g_frames[16];

static unsigned char *native_cell(unsigned long rva)
{
    return (unsigned char *)g_oracle + rva;
}

static void write_native_word(unsigned long rva, unsigned short value)
{
    memcpy(native_cell(rva), &value, sizeof(value));
}

static void check(int condition, const char *label)
{
    ++g_checks;
    if (!condition) {
        ++g_failures;
        printf("FAIL %s\n", label);
    }
}

static void prepare_actor(Actor *actor)
{
    unsigned int i;
    unsigned long copied_dc;
    unsigned long copied_e0;
    long child_y;

    memset(actor, 0xa5, sizeof(*actor));
    actor->x = 0x12345678L;
    actor->y = 0x10203040L;
    actor->z = 0x55667788L;
    actor->kind = 0x2fU;
    actor->unknown_02a = 0x7766U;
    actor->frame_ticks = 0x1357;
    actor->frame_state = 0x2468;
    actor->frames = g_frames;
    actor->frame_index = 6;
    actor->image_or_program = (void *)0x12345678UL;

    /* Match the caller's pre-helper stores for a kind-0x2f list entry. */
    child_y = actor->y;
    copied_dc = 0x00000040UL;
    copied_e0 = 0x000000a0UL;
    memcpy((unsigned char *)(void *)actor + 0xd8, &child_y, 4);
    memcpy((unsigned char *)(void *)actor + 0xdc, &copied_dc, 4);
    memcpy((unsigned char *)(void *)actor + 0xe0, &copied_e0, 4);

    for (i = 0; i < 16U; ++i) {
        g_frame_tokens[i] = 0x41000000UL + i * 0x100UL;
        g_frames[i] = &g_frame_tokens[i];
    }
}

static void set_row(const TestCase *test_case)
{
    unsigned long row_rva;

    row_rva = 0x65678UL + test_case->category * 12UL;
    g_eng1_classifier_rows_65678[test_case->category][2] =
        test_case->column2;
    g_eng1_classifier_rows_65678[test_case->category][3] =
        test_case->column3;
    write_native_word(row_rva + 4UL, test_case->column2);
    write_native_word(row_rva + 6UL, test_case->column3);
}

static void run_case(NativeEntry native_entry, const TestCase *test_case)
{
    Actor native_actor;
    Actor source_actor;
    long expected_y;
    void *expected_image;

    set_row(test_case);
    prepare_actor(&native_actor);
    prepare_actor(&source_actor);

    printf("CALL native %s category=%lu row=%04x/%04x\n",
           test_case->name, test_case->category,
           test_case->column2, test_case->column3);
    __try {
        native_entry(&native_actor, test_case->category);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        ++g_failures;
        printf("FAIL native exception case=%s code=%08lx\n",
               test_case->name, GetExceptionCode());
        return;
    }

    printf("CALL source %s\n", test_case->name);
    eng1_native_code_target_0041000(&source_actor, test_case->category);
    ++g_cases;

    check(memcmp(&native_actor, &source_actor, sizeof(Actor)) == 0,
          "full Actor bytes match");

    expected_y = test_case->column3 != 0U ?
        (long)((unsigned long)test_case->column3 << 16) : 0x10203040L;
    check(native_actor.y == expected_y, "column 3 replacement of child.y");
    check(native_actor.unknown_02a == test_case->expected_frame_word,
          "normalized WORD at child+0x2a");
    check(native_actor.frame_index == test_case->expected_frame_index,
          "select_frame index");
    expected_image = g_frames[test_case->expected_frame_index];
    check(native_actor.image_or_program == expected_image,
          "select_frame image pointer");
    check((unsigned short)native_actor.frame_state == 0xffffU,
          "select_frame sets frame state to -1");
    check(native_actor.frame_ticks == 0,
          "select_frame clears ticks");
    check(native_actor.kind == 0x2fU,
          "callback leaves list kind WORD unchanged");
    {
        unsigned long d8;
        unsigned long dc;
        unsigned long e0;

        memcpy(&d8, (unsigned char *)(void *)&native_actor + 0xd8, 4);
        memcpy(&dc, (unsigned char *)(void *)&native_actor + 0xdc, 4);
        memcpy(&e0, (unsigned char *)(void *)&native_actor + 0xe0, 4);
        check(d8 == 0x10203040UL,
              "caller-provided child.y copy at child+0xd8 remains intact");
        check(dc == 0x40UL, "caller-provided child+0xdc remains intact");
        check(e0 == 0xa0UL, "caller-provided child+0xe0 remains intact");
    }
    printf("CASE %s y=%08lx frame-word=%04x frame-index=%d selected=%d\n",
           test_case->name, (unsigned long)native_actor.y,
           native_actor.unknown_02a, native_actor.frame_index,
           1);
}

int main(int argc, char **argv)
{
    static const TestCase cases[] = {
        { "zero-normalizes-to-one", 0UL, 0U, 0U, 1U, 0U },
        { "one-selects-zero", 1UL, 1U, 0x2345U, 1U, 0U },
        { "two-selects-one", 2UL, 2U, 0U, 2U, 1U },
        { "three-selects-two", 3UL, 3U, 0x8001U, 3U, 2U },
        { "seven-selects-two", 4UL, 7U, 0U, 7U, 2U },
        { "eight-normalizes-to-special", 5UL, 8U, 0x0abcU, 0x10U, 14U },
        { "sixteen-normalizes-to-special", 6UL, 16U, 0U, 0x10U, 14U },
        { "high-bit-word-normalizes", 7UL, 0x8000U, 0U, 0x10U, 14U },
        { "all-ones-word-normalizes", 8UL, 0xffffU, 0xffffU, 0x10U, 14U }
    };
    NativeEntry native_entry;
    unsigned int i;
    unsigned long base;
    int exit_code;

    exit_code = 1;
    setvbuf(stdout, 0, _IONBF, 0);
    if (argc != 2) {
        fprintf(stderr, "usage: eng1_call_41000.exe ENG1.DLL\n");
        return 2;
    }

    g_oracle = LoadLibraryA(argv[1]);
    if (g_oracle == 0) {
        fprintf(stderr, "LoadLibraryA failed: %lu\n", GetLastError());
        return 2;
    }
    base = (unsigned long)g_oracle;
    if (base != 0x10000000UL) {
        fprintf(stderr, "oracle relocated to %08lx\n", base);
        goto cleanup;
    }

    memcpy(g_saved_native_rows, native_cell(0x65678UL),
           sizeof(g_saved_native_rows));
    memcpy(g_saved_source_rows, g_eng1_classifier_rows_65678,
           sizeof(g_saved_source_rows));

    native_entry = (NativeEntry)(void *)native_cell(0x41000UL);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
        run_case(native_entry, &cases[i]);

    printf("RESULT cases=%u checks=%u failures=%u\n",
           g_cases, g_checks, g_failures);
    exit_code = (g_cases == 9U && g_failures == 0U) ? 0 : 1;

    memcpy(native_cell(0x65678UL), g_saved_native_rows,
           sizeof(g_saved_native_rows));
    memcpy(g_eng1_classifier_rows_65678, g_saved_source_rows,
           sizeof(g_saved_source_rows));

cleanup:
    if (g_oracle != 0 && !FreeLibrary(g_oracle)) {
        fprintf(stderr, "FreeLibrary failed: %lu\n", GetLastError());
        exit_code = 1;
    }
    return exit_code;
}
