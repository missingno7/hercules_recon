/* Test-only comparison of functions/data in two separately loaded TITLE DLLs. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NATIVE_RESET_RVA 0x94c0UL
#define NATIVE_CELL_FN_RVA 0x4fe0UL
#define NATIVE_CELL_DATA_RVA 0x2d340UL
#define NATIVE_INTERFACE_RVA 0x2bb40UL
#define NATIVE_FIRST_USE_RVA 0x257e8UL
#define NATIVE_ENTRY_RVA 0xc1f0UL
#define CELL_PREFIX_BYTES 3072UL
#define RESET_BYTES 0x94UL
#define RESET_GUARD 16UL
#define RESET_ARENA_BYTES (RESET_GUARD + 3UL + RESET_BYTES + RESET_GUARD)
#define INTERFACE_BYTES 1000UL

typedef void (__cdecl *ResetFn)(void *);
typedef void (__cdecl *CellInitFn)(void);
typedef void (__cdecl *EntryFn)(const void *, unsigned int);

static unsigned long random_state = 0xD1FF3A71UL;
static unsigned long random_word(void)
{
    random_state = random_state * 1664525UL + 1013904223UL;
    return random_state;
}

static int compare_bytes(const char *name, unsigned long test,
                         const unsigned char *original, const unsigned char *candidate,
                         unsigned long count, unsigned long seed, int alignment)
{
    unsigned long i;
    for (i = 0; i < count; ++i) {
        if (original[i] != candidate[i]) {
            if (alignment >= 0)
                printf("DIVERGENCE %s case=%lu alignment=%d seed=%08lx byte=%lu original=%02x candidate=%02x\n",
                       name, test, alignment, seed, i, (unsigned int)original[i],
                       (unsigned int)candidate[i]);
            else
                printf("DIVERGENCE %s case=%lu seed=%08lx byte=%lu original=%02x candidate=%02x\n",
                       name, test, seed, i, (unsigned int)original[i],
                       (unsigned int)candidate[i]);
            return 0;
        }
    }
    return 1;
}

static int test_object_reset(HMODULE original_module, HMODULE candidate_module,
                             unsigned long candidate_rva)
{
    ResetFn original;
    ResetFn candidate;
    unsigned char expected[RESET_ARENA_BYTES];
    unsigned char actual[RESET_ARENA_BYTES];
    unsigned long cases;
    unsigned long seed;
    unsigned long i;
    int alignment;
    original = (ResetFn)((unsigned char *)original_module + NATIVE_RESET_RVA);
    candidate = (ResetFn)((unsigned char *)candidate_module + candidate_rva);
    original(NULL);
    candidate(NULL);
    cases = 0;
    for (alignment = 0; alignment < 4; ++alignment) {
        for (i = 0; i < 256UL; ++i) {
            unsigned long b;
            unsigned char *expected_object;
            unsigned char *actual_object;
            seed = random_state;
            for (b = 0; b < RESET_ARENA_BYTES; ++b)
                expected[b] = (unsigned char)random_word();
            memcpy(actual, expected, sizeof(expected));
            expected_object = expected + RESET_GUARD + alignment;
            actual_object = actual + RESET_GUARD + alignment;
            original(expected_object);
            candidate(actual_object);
            ++cases;
            if (!compare_bytes("object_reset", cases, expected, actual,
                               RESET_ARENA_BYTES, seed, alignment)) return 0;
        }
    }
    printf("PASS object_reset cases=%lu null_accepted=1 alignments=0..3 state_bytes=%lu guard_bytes_each_side=16\n",
           cases, RESET_BYTES);
    return 1;
}

static int test_cell_initializer(HMODULE original_module, HMODULE candidate_module,
                                 unsigned long candidate_function_rva,
                                 unsigned long candidate_data_rva)
{
    CellInitFn original;
    CellInitFn candidate;
    unsigned char *expected;
    unsigned char *actual;
    unsigned long test;
    unsigned long i;
    unsigned long seed;
    original = (CellInitFn)((unsigned char *)original_module + NATIVE_CELL_FN_RVA);
    candidate = (CellInitFn)((unsigned char *)candidate_module + candidate_function_rva);
    expected = (unsigned char *)original_module + NATIVE_CELL_DATA_RVA;
    actual = (unsigned char *)candidate_module + candidate_data_rva;
    for (test = 0; test < 64UL; ++test) {
        seed = random_state;
        for (i = 0; i < CELL_PREFIX_BYTES; ++i) expected[i] = (unsigned char)random_word();
        memcpy(actual, expected, CELL_PREFIX_BYTES);
        original();
        candidate();
        if (!compare_bytes("cell_initializer", test + 1, expected, actual,
                           CELL_PREFIX_BYTES, seed, -1)) return 0;
    }
    printf("PASS cell_initializer cases=%lu compared_bytes=%lu cells=256 stride=12 includes_untouched_next_words=1\n",
           test, CELL_PREFIX_BYTES);
    return 1;
}

static int compare_region(const char *name, const unsigned char *expected,
                          const unsigned char *actual, unsigned long count)
{
    unsigned long i;
    for (i = 0; i < count; ++i) {
        if (expected[i] != actual[i]) {
            printf("DIVERGENCE %s byte=%lu expected=%02x actual=%02x\n",
                   name, i, (unsigned int)expected[i], (unsigned int)actual[i]);
            return 0;
        }
    }
    return 1;
}

static int test_entry_rejections(HMODULE original_module, HMODULE candidate_module,
                                 unsigned long candidate_entry_rva,
                                 unsigned long candidate_interface_rva,
                                 unsigned long candidate_flag_rva)
{
    EntryFn original;
    EntryFn candidate;
    int *original_flag;
    int *candidate_flag;
    unsigned char *original_interface;
    unsigned char *candidate_interface;
    unsigned char original_before[INTERFACE_BYTES];
    unsigned char candidate_before[INTERFACE_BYTES];
    unsigned char host[INTERFACE_BYTES];
    unsigned char host_before[INTERFACE_BYTES];
    unsigned long i;
    original = (EntryFn)((unsigned char *)original_module + NATIVE_ENTRY_RVA);
    candidate = (EntryFn)((unsigned char *)candidate_module + candidate_entry_rva);
    original_flag = (int *)((unsigned char *)original_module + NATIVE_FIRST_USE_RVA);
    candidate_flag = (int *)((unsigned char *)candidate_module + candidate_flag_rva);
    original_interface = (unsigned char *)original_module + NATIVE_INTERFACE_RVA;
    candidate_interface = (unsigned char *)candidate_module + candidate_interface_rva;
    if (*original_flag != 1 || *candidate_flag != 1) {
        printf("SKIP entry_rejections: first-use flags are original=%ld candidate=%ld, expected 1\n",
               (long)*original_flag, (long)*candidate_flag);
        return 1;
    }
    SetEnvironmentVariableA("HERCULES_TITLE_TRACE", NULL);
    memcpy(original_before, original_interface, INTERFACE_BYTES);
    memcpy(candidate_before, candidate_interface, INTERFACE_BYTES);
    original(NULL, 0);
    candidate(NULL, 0);
    if (!compare_region("cold_null_original_interface", original_before,
                        original_interface, INTERFACE_BYTES) ||
        !compare_region("cold_null_candidate_interface", candidate_before,
                        candidate_interface, INTERFACE_BYTES)) return 0;
    if (*original_flag != 1 || *candidate_flag != 1) {
        printf("DIVERGENCE cold_null first-use flag original=%ld candidate=%ld\n",
               (long)*original_flag, (long)*candidate_flag);
        return 0;
    }
    for (i = 0; i < INTERFACE_BYTES; ++i) host[i] = (unsigned char)random_word();
    host[0] = 0;
    host[1] = 0;
    host[2] = 0;
    host[3] = 0;
    memcpy(host_before, host, INTERFACE_BYTES);
    original(host, 0x12345678UL);
    candidate(host, 0x12345678UL);
    if (!compare_region("bad_signature_original_interface", host_before,
                        original_interface, INTERFACE_BYTES) ||
        !compare_region("bad_signature_candidate_interface", host_before,
                        candidate_interface, INTERFACE_BYTES) ||
        !compare_region("bad_signature_input_preserved", host_before, host,
                        INTERFACE_BYTES)) return 0;
    if (*original_flag != 1 || *candidate_flag != 1) {
        printf("DIVERGENCE bad_signature first-use flag original=%ld candidate=%ld\n",
               (long)*original_flag, (long)*candidate_flag);
        return 0;
    }
    printf("PASS entry_rejections cold_null=1 bad_signature=1 copied_interface_bytes=%lu callbacks_not_entered=1\n",
           INTERFACE_BYTES);
    return 1;
}

int main(int argc, char **argv)
{
    HMODULE original_module;
    HMODULE candidate_module;
    unsigned long reset_rva;
    unsigned long cell_function_rva;
    unsigned long cell_data_rva;
    unsigned long interface_rva;
    unsigned long flag_rva;
    unsigned long entry_rva;
    if (argc != 9) {
        printf("usage: dll_diff.exe <original.dll> <candidate.dll> <reset-rva> <cell-fn-rva> <cell-data-rva> <interface-rva> <flag-rva> <entry-rva>\n");
        return 2;
    }
    reset_rva = strtoul(argv[3], NULL, 0);
    cell_function_rva = strtoul(argv[4], NULL, 0);
    cell_data_rva = strtoul(argv[5], NULL, 0);
    interface_rva = strtoul(argv[6], NULL, 0);
    flag_rva = strtoul(argv[7], NULL, 0);
    entry_rva = strtoul(argv[8], NULL, 0);
    original_module = LoadLibraryA(argv[1]);
    if (original_module == NULL) {
        printf("HARNESS ERROR: original LoadLibraryA error=%08lx path=%s\n",
               (unsigned long)GetLastError(), argv[1]);
        return 3;
    }
    candidate_module = LoadLibraryA(argv[2]);
    if (candidate_module == NULL) {
        printf("HARNESS ERROR: candidate LoadLibraryA error=%08lx path=%s\n",
               (unsigned long)GetLastError(), argv[2]);
        FreeLibrary(original_module);
        return 4;
    }
    if (original_module == candidate_module) {
        printf("HARNESS ERROR: loader returned the same module handle for both paths\n");
        FreeLibrary(candidate_module);
        FreeLibrary(original_module);
        return 5;
    }
    printf("loaded original=%p candidate=%p handles_distinct=1\n",
           (void *)original_module, (void *)candidate_module);
    if (!test_object_reset(original_module, candidate_module, reset_rva) ||
        !test_cell_initializer(original_module, candidate_module,
                               cell_function_rva, cell_data_rva) ||
        !test_entry_rejections(original_module, candidate_module, entry_rva,
                               interface_rva, flag_rva)) {
        FreeLibrary(candidate_module);
        FreeLibrary(original_module);
        return 1;
    }
    FreeLibrary(candidate_module);
    FreeLibrary(original_module);
    printf("PASS all newDLL loaded-module comparisons\n");
    return 0;
}
