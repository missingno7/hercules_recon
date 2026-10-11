/* In-memory differential for TITLE's password table checker (test only). */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NATIVE_CHECK_RVA 0x0cd30UL
#define NATIVE_TABLE_RVA 0x25a30UL
#define NATIVE_INTERFACE_RVA 0x2bb40UL
#define ROWS 42UL
#define FIELDS 6UL
#define TABLE_BYTES (ROWS * FIELDS * 4UL)
#define OBJECT_BYTES 0x134UL
#define CONTEXT_BYTES 0x40UL

typedef int (__cdecl *CheckFn)(void);

static unsigned long random_state = 0x51a7c0deUL;
static unsigned long random_word(void)
{
    random_state = random_state * 1664525UL + 1013904223UL;
    return random_state;
}

static void fill_bytes(unsigned char *p, unsigned long n)
{
    unsigned long i;
    for (i = 0; i < n; ++i) p[i] = (unsigned char)random_word();
}

static void set_word(unsigned char *object, unsigned short value)
{
    *(unsigned short *)(object + 0x34) = value;
}

static int compare_bytes(const char *name, unsigned long test,
                         const unsigned char *original,
                         const unsigned char *candidate, unsigned long count)
{
    unsigned long i;
    for (i = 0; i < count; ++i) {
        if (original[i] != candidate[i]) {
            printf("DIVERGENCE %s case=%lu byte=%lu original=%02x candidate=%02x\n",
                   name, test, i, (unsigned int)original[i],
                   (unsigned int)candidate[i]);
            return 0;
        }
    }
    return 1;
}

static void install_inputs(HMODULE module, unsigned long interface_rva,
                           const unsigned long pointer_rva[4],
                           unsigned char objects[4][OBJECT_BYTES],
                           unsigned char context[CONTEXT_BYTES])
{
    unsigned long i;
    unsigned char *base = (unsigned char *)module;
    *(unsigned long *)(base + interface_rva + 4) = (unsigned long)context;
    for (i = 0; i < 4; ++i)
        *(unsigned long *)(base + pointer_rva[i]) = (unsigned long)objects[i];
}

static int compare_post_state(const char *name, unsigned long test,
                              unsigned char original_objects[4][OBJECT_BYTES],
                              unsigned char candidate_objects[4][OBJECT_BYTES],
                              unsigned char original_context[CONTEXT_BYTES],
                              unsigned char candidate_context[CONTEXT_BYTES],
                              HMODULE original_module, HMODULE candidate_module,
                              const unsigned long original_flag_rva[3],
                              const unsigned long candidate_flag_rva[3])
{
    unsigned long i;
    for (i = 0; i < 4; ++i) {
        char label[40];
        sprintf(label, "%s_object%lu", name, i);
        if (!compare_bytes(label, test, original_objects[i],
                           candidate_objects[i], OBJECT_BYTES)) return 0;
    }
    if (!compare_bytes(name, test, original_context, candidate_context,
                       CONTEXT_BYTES)) return 0;
    for (i = 0; i < 3; ++i) {
        unsigned long a = *(unsigned long *)((unsigned char *)original_module +
                                              original_flag_rva[i]);
        unsigned long b = *(unsigned long *)((unsigned char *)candidate_module +
                                              candidate_flag_rva[i]);
        if (a != b) {
            printf("DIVERGENCE %s case=%lu flag%lu original=%08lx candidate=%08lx\n",
                   name, test, i, a, b);
            return 0;
        }
    }
    return 1;
}

static int expected_code(int table[ROWS][FIELDS],
                         unsigned char objects[4][OBJECT_BYTES],
                         const unsigned short bases[4],
                         unsigned long flags[3],
                         unsigned char context[CONTEXT_BYTES])
{
    unsigned long row, key;
    int code = -1;
    for (row = 0; row < ROWS; ++row) {
        for (key = 0; key < 4; ++key) {
            unsigned short word = *(unsigned short *)(objects[key] + 0x34);
            if (table[row][key] != (int)word - bases[key]) break;
        }
        if (key == 4) {
            code = table[row][4];
            break;
        }
    }
    if (code == 100) {
        flags[0] = 1;
        return code;
    }
    if (code == 101 && flags[0] == 1) {
        *(unsigned long *)(context + 0x30) |= 0x80000000UL;
        *(unsigned short *)(context + 0x2e) = 0;
        return code;
    }
    if (code == 102) {
        flags[1] = 1;
        return code;
    }
    if (code == 103 && flags[1] == 1) {
        *(unsigned long *)(context + 0x30) |= 0x80000000UL;
        *(unsigned short *)(context + 0x2e) = 1;
        return code;
    }
    if (code == 104) {
        flags[2] = 1;
        return code;
    }
    if (code == 105 && flags[2] == 1) {
        *(unsigned long *)(context + 0x30) |= 0x80000008UL;
        context[6] = 3;
        return 1;
    }
    context[0x0e] = (unsigned char)table[row][5];
    context[0x12] = (unsigned char)row;
    return code;
}

static int test_password_checker(HMODULE original_module,
                                 HMODULE candidate_module,
                                 unsigned long candidate_fn_rva,
                                 unsigned long candidate_table_rva,
                                 unsigned long candidate_interface_rva,
                                 const unsigned long candidate_pointer_rva[4],
                                 const unsigned long candidate_flag_rva[3])
{
    const unsigned long original_pointer_rva[4] = {
        0x2a278UL, 0x2a27cUL, 0x2a280UL, 0x2a274UL
    };
    const unsigned long original_flag_rva[3] = {
        0x2a388UL, 0x2a268UL, 0x2a260UL
    };
    const unsigned short key_bases[4] = { 0x1e3, 0x121, 0x182, 0xc0 };
    int (*original_table)[FIELDS] =
        (int (*)[FIELDS])((unsigned char *)original_module + NATIVE_TABLE_RVA);
    int (*candidate_table)[FIELDS] =
        (int (*)[FIELDS])((unsigned char *)candidate_module + candidate_table_rva);
    unsigned char original_objects[4][OBJECT_BYTES];
    unsigned char candidate_objects[4][OBJECT_BYTES];
    unsigned char original_objects_before[4][OBJECT_BYTES];
    unsigned char original_context[CONTEXT_BYTES];
    unsigned char candidate_context[CONTEXT_BYTES];
    unsigned char expected_context[CONTEXT_BYTES];
    unsigned long expected_flags[3] = { 0, 0, 0 };
    unsigned long result_counts[106] = { 0 };
    CheckFn original = (CheckFn)((unsigned char *)original_module + NATIVE_CHECK_RVA);
    CheckFn candidate = (CheckFn)((unsigned char *)candidate_module + candidate_fn_rva);
    unsigned long i, j, test = 0;

    if (!compare_bytes("password_table", 0,
                       (const unsigned char *)original_table,
                       (const unsigned char *)candidate_table, TABLE_BYTES)) return 0;

    for (i = 0; i < 3; ++i) {
        *(unsigned long *)((unsigned char *)original_module + original_flag_rva[i]) = 0;
        *(unsigned long *)((unsigned char *)candidate_module + candidate_flag_rva[i]) = 0;
    }

    for (i = 0; i < ROWS; ++i) {
        int original_result, candidate_result, expected_result;
        ++test;
        fill_bytes((unsigned char *)original_objects, sizeof(original_objects));
        memcpy(candidate_objects, original_objects, sizeof(original_objects));
        fill_bytes(original_context, sizeof(original_context));
        memcpy(candidate_context, original_context, sizeof(original_context));
        for (j = 0; j < 4; ++j) {
            unsigned short a = (unsigned short)(key_bases[j] + original_table[i][j]);
            unsigned short b = (unsigned short)(key_bases[j] + candidate_table[i][j]);
            set_word(original_objects[j], a);
            set_word(candidate_objects[j], b);
        }
        install_inputs(original_module, NATIVE_INTERFACE_RVA,
                       original_pointer_rva, original_objects, original_context);
        install_inputs(candidate_module, candidate_interface_rva,
                       candidate_pointer_rva, candidate_objects, candidate_context);
        memcpy(original_objects_before, original_objects, sizeof(original_objects_before));
        memcpy(expected_context, original_context, sizeof(expected_context));
        expected_result = expected_code(original_table, original_objects,
                                        key_bases, expected_flags, expected_context);
        original_result = original();
        candidate_result = candidate();
        if (original_result != candidate_result || original_result != expected_result) {
            printf("DIVERGENCE password_valid_row case=%lu row=%lu original=%ld candidate=%ld expected=%ld\n",
                   test, i, (long)original_result, (long)candidate_result,
                   (long)expected_result);
            return 0;
        }
        ++result_counts[original_result];
        if (!compare_post_state("password_valid_row", test,
                                original_objects, candidate_objects,
                                original_context, candidate_context,
                                original_module, candidate_module,
                                original_flag_rva, candidate_flag_rva)) return 0;
        if (!compare_bytes("password_valid_object_immutability", test,
                           (const unsigned char *)original_objects_before,
                           (const unsigned char *)original_objects,
                           sizeof(original_objects_before)) ||
            !compare_bytes("password_valid_context_effect", test,
                           expected_context, original_context,
                           sizeof(expected_context))) return 0;
        for (j = 0; j < 3; ++j) {
            unsigned long original_flag = *(unsigned long *)((unsigned char *)original_module +
                                                              original_flag_rva[j]);
            unsigned long candidate_flag = *(unsigned long *)((unsigned char *)candidate_module +
                                                               candidate_flag_rva[j]);
            if (original_flag != expected_flags[j] || candidate_flag != expected_flags[j]) {
                printf("DIVERGENCE password_valid_special_flag case=%lu flag=%lu expected=%lu original=%lu candidate=%lu\n",
                       test, j, expected_flags[j], original_flag, candidate_flag);
                return 0;
            }
        }
    }

    for (i = 0; i < ROWS + 1; ++i) {
        int original_result, candidate_result;
        ++test;
        fill_bytes((unsigned char *)original_objects, sizeof(original_objects));
        memcpy(candidate_objects, original_objects, sizeof(original_objects));
        fill_bytes(original_context, sizeof(original_context));
        memcpy(candidate_context, original_context, sizeof(original_context));
        if (i < ROWS) {
            set_word(original_objects[0], key_bases[0]);
            set_word(candidate_objects[0], key_bases[0]);
            for (j = 1; j < 4; ++j) {
                set_word(original_objects[j], (unsigned short)(
                    key_bases[j] + original_table[i][j]));
                set_word(candidate_objects[j], (unsigned short)(
                    key_bases[j] + candidate_table[i][j]));
            }
        } else {
            for (j = 0; j < 4; ++j) {
                set_word(original_objects[j], 0xffff);
                set_word(candidate_objects[j], 0xffff);
            }
        }
        install_inputs(original_module, NATIVE_INTERFACE_RVA,
                       original_pointer_rva, original_objects, original_context);
        install_inputs(candidate_module, candidate_interface_rva,
                       candidate_pointer_rva, candidate_objects, candidate_context);
        memcpy(original_objects_before, original_objects, sizeof(original_objects_before));
        memcpy(expected_context, original_context, sizeof(expected_context));
        original_result = original();
        candidate_result = candidate();
        if (original_result != candidate_result || original_result != -1) {
            printf("DIVERGENCE password_invalid case=%lu row=%lu original=%ld candidate=%ld expected=-1\n",
                   test, i, (long)original_result, (long)candidate_result);
            return 0;
        }
        if (!compare_post_state("password_invalid", test,
                                original_objects, candidate_objects,
                                original_context, candidate_context,
                                original_module, candidate_module,
                                original_flag_rva, candidate_flag_rva)) return 0;
        if (!compare_bytes("password_invalid_object_immutability", test,
                           (const unsigned char *)original_objects_before,
                           (const unsigned char *)original_objects,
                           sizeof(original_objects_before)) ||
            !compare_bytes("password_invalid_context_unchanged", test,
                           expected_context, original_context,
                           sizeof(expected_context))) return 0;
        for (j = 0; j < 3; ++j) {
            unsigned long original_flag = *(unsigned long *)((unsigned char *)original_module +
                                                              original_flag_rva[j]);
            unsigned long candidate_flag = *(unsigned long *)((unsigned char *)candidate_module +
                                                               candidate_flag_rva[j]);
            if (original_flag != expected_flags[j] || candidate_flag != expected_flags[j]) {
                printf("DIVERGENCE password_invalid_special_flag case=%lu flag=%lu expected=%lu original=%lu candidate=%lu\n",
                       test, j, expected_flags[j], original_flag, candidate_flag);
                return 0;
            }
        }
    }

    printf("PASS password_checker valid_table_rows=%lu invalid_inputs=%lu table_bytes=%lu state=objects+context+three_special_flags\n",
           ROWS, ROWS + 1, TABLE_BYTES);
    printf("OBSERVED special_return_codes=100,101,102,103,104,105; invalid_first_key=0 and all_words=ffff rejected\n");
    printf("RETURN_COUNTS");
    for (i = 1; i <= 12; ++i)
        if (result_counts[i]) printf(" %lu:%lu", i, result_counts[i]);
    for (i = 100; i <= 105; ++i)
        if (result_counts[i]) printf(" %lu:%lu", i, result_counts[i]);
    printf("\n");
    return 1;
}

int main(int argc, char **argv)
{
    HMODULE original_module, candidate_module;
    unsigned long candidate_fn_rva, candidate_table_rva, candidate_interface_rva;
    unsigned long candidate_pointer_rva[4], candidate_flag_rva[3];
    unsigned long i;

    if (argc != 13) {
        printf("usage: password_diff.exe <original.dll> <candidate.dll> <fn-rva> <table-rva> <interface-rva> <ptr-rva0..3> <flag-rva0..2>\n");
        return 2;
    }
    candidate_fn_rva = strtoul(argv[3], NULL, 0);
    candidate_table_rva = strtoul(argv[4], NULL, 0);
    candidate_interface_rva = strtoul(argv[5], NULL, 0);
    for (i = 0; i < 4; ++i) candidate_pointer_rva[i] = strtoul(argv[6 + i], NULL, 0);
    for (i = 0; i < 3; ++i) candidate_flag_rva[i] = strtoul(argv[10 + i], NULL, 0);

    original_module = LoadLibraryA(argv[1]);
    if (original_module == NULL) {
        printf("HARNESS ERROR original LoadLibraryA error=%08lx path=%s\n",
               (unsigned long)GetLastError(), argv[1]);
        return 3;
    }
    candidate_module = LoadLibraryA(argv[2]);
    if (candidate_module == NULL) {
        printf("HARNESS ERROR candidate LoadLibraryA error=%08lx path=%s\n",
               (unsigned long)GetLastError(), argv[2]);
        FreeLibrary(original_module);
        return 4;
    }
    if (original_module == candidate_module) {
        printf("HARNESS ERROR loader returned same module handle\n");
        FreeLibrary(candidate_module);
        FreeLibrary(original_module);
        return 5;
    }
    printf("loaded original=%p candidate=%p handles_distinct=1\n",
           (void *)original_module, (void *)candidate_module);
    if (!test_password_checker(original_module, candidate_module,
                               candidate_fn_rva, candidate_table_rva,
                               candidate_interface_rva, candidate_pointer_rva,
                               candidate_flag_rva)) {
        FreeLibrary(candidate_module);
        FreeLibrary(original_module);
        return 1;
    }
    FreeLibrary(candidate_module);
    FreeLibrary(original_module);
    return 0;
}
