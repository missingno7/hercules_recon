/* Bounded unpatched ENG1 execution against the private worklist source. */
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define main source_worklist_fixture_main
#include "eng1_worklist_factory_spies.c"
#undef main

static HMODULE g_native_module;
static union { unsigned long align; unsigned char bytes[0x134 * 2]; } g_native_normal;
static union { unsigned long align; unsigned char bytes[0x0ec * 2]; } g_native_middle;
static union { unsigned long align; unsigned char bytes[0x094 * 2]; } g_native_alternate;
static union { unsigned long align; unsigned char bytes[40]; } g_native_worklist;
static unsigned char g_source_final[0x268];
static unsigned char g_native_final[0x268];
static unsigned char g_source_callback_state[0x134];
static unsigned char g_native_callback_state[0x134];
static unsigned char g_source_record_final[20];
static unsigned char g_native_record_final[20];
static unsigned char g_source_callback_record[20];
static unsigned char g_native_callback_record[20];
static int g_active_family;
static int g_source_callback_order;
static int g_native_callback_order;
static void *g_source_callback_actor;
static void *g_native_callback_actor;
static unsigned long g_checks;
static unsigned long g_failures;
static int g_native_mode;

static unsigned char *native_cell(unsigned long rva)
{
    return (unsigned char *)g_native_module + rva;
}

static void check(int condition, const char *name)
{
    ++g_checks;
    if (!condition) {
        ++g_failures;
        printf("FAIL %s\n", name);
    }
}

static void __cdecl differential_action_probe(void *actor)
{
    unsigned long bytes = g_active_family == 0 ? 0x134UL :
                          g_active_family == 1 ? 0x0ecUL : 0x094UL;
    if (g_native_mode) {
        ++g_native_callback_order;
        g_native_callback_actor = actor;
        memcpy(g_native_callback_state, actor, bytes);
        memcpy(g_native_callback_record, callback_record, 20);
    } else {
        ++g_source_callback_order;
        g_source_callback_actor = actor;
        memcpy(g_source_callback_state, actor, bytes);
        memcpy(g_source_callback_record, callback_record, 20);
    }
    action_probe(actor);
}

static void seed_patterned(unsigned char *bytes, unsigned long size)
{
    memset(bytes, 0xa5, size);
}

static void store_native_pointer(unsigned long rva, const void *pointer)
{
    *(unsigned long *)native_cell(rva) = (unsigned long)pointer;
}

static unsigned short native_word(unsigned long rva)
{
    return *(unsigned short *)native_cell(rva);
}

static void set_native_word(unsigned long rva, unsigned short value)
{
    *(unsigned short *)native_cell(rva) = value;
}

static unsigned short source_cursor(int family)
{
    if (family == 0) return g_normal_cursor_alternate;
    if (family == 1) return g_middle_cursor_alternate;
    return g_alternate_cursor_alternate;
}

static unsigned long native_cursor_rva(int family)
{
    if (family == 0) return 0x713c4UL;
    if (family == 1) return 0x713b8UL;
    return 0x71430UL;
}

static unsigned long pool_stride(int family)
{
    return family == 0 ? 0x134UL : family == 1 ? 0x0ecUL : 0x094UL;
}

static unsigned char *source_pool(int family)
{
    if (family == 0) return normal_pool.bytes;
    if (family == 1) return middle_pool.bytes;
    return alternate_pool.bytes;
}

static unsigned char *native_pool(int family)
{
    if (family == 0) return g_native_normal.bytes;
    if (family == 1) return g_native_middle.bytes;
    return g_native_alternate.bytes;
}

static void seed_native_cells(int family)
{
    store_native_pointer(0x72860UL, g_native_normal.bytes);
    store_native_pointer(0x73494UL, g_native_middle.bytes);
    store_native_pointer(0x734e0UL, g_native_alternate.bytes);
    store_native_pointer(0x6fee8UL, g_native_worklist.bytes);
    *(unsigned long *)native_cell(0x6feecUL) = 1UL;

    store_native_pointer(0x71388UL, eng1_fixture_normal_assets);
    store_native_pointer(0x71370UL, eng1_fixture_normal_actions);
    store_native_pointer(0x71390UL, eng1_fixture_middle_assets);
    store_native_pointer(0x7137cUL, eng1_fixture_middle_actions);
    store_native_pointer(0x713a4UL, eng1_fixture_alternate_assets);
    store_native_pointer(0x7139cUL, eng1_fixture_alternate_actions);

    /* These are the exact cursor and word-bound cells used by the existing
     * allocators; only the selected family is made allocatable. */
    if (family == 0) {
        set_native_word(0x713c4UL, 0);
        set_native_word(0x713a8UL, 2);
        set_native_word(0x71378UL, 0);
    } else if (family == 1) {
        set_native_word(0x713b8UL, 0);
        set_native_word(0x713a0UL, 2);
        set_native_word(0x713aaUL, 0);
    } else {
        set_native_word(0x71430UL, 0);
        set_native_word(0x713b6UL, 2);
        set_native_word(0x7138cUL, 0);
    }
}

static void seed_source_cells(int family)
{
    g_normal_actor_pool = (ActorNormal *)normal_pool.bytes;
    g_middle_actor_pool = (ActorMiddle *)middle_pool.bytes;
    g_alternate_actor_pool = (ActorCore *)alternate_pool.bytes;
    g_kind_info[0].owner = worklist_row.bytes;
    g_kind_info[0].count = 1;
    g_normal_cursor_alternate = 0;
    g_normal_limit_wrap = family == 0 ? 2 : 2;
    g_normal_limit_wrap_start = 0;
    g_middle_cursor_alternate = 0;
    g_middle_wrap_start_primary = 2;
    g_middle_wrap_start_alternate = 0;
    g_alternate_cursor_alternate = 0;
    g_alternate_limit_wrap = 2;
    g_alternate_limit_wrap_start = 0;
}

static void configure_case(int family, int fail_allocation)
{
    unsigned char *source_bytes;
    unsigned char *native_bytes;
    unsigned long stride = pool_stride(family);

    reset_common();
    seed_source_cells(family);
    memset(g_native_normal.bytes, 0, sizeof(g_native_normal.bytes));
    memset(g_native_middle.bytes, 0, sizeof(g_native_middle.bytes));
    memset(g_native_alternate.bytes, 0, sizeof(g_native_alternate.bytes));
    memset(g_native_worklist.bytes, 0, sizeof(g_native_worklist.bytes));
    source_bytes = source_pool(family);
    native_bytes = native_pool(family);
    seed_patterned(source_bytes, stride * 2UL);
    seed_patterned(native_bytes, stride * 2UL);
    memset(worklist_row.bytes, 0xa5, sizeof(worklist_row.bytes));
    memset(g_native_worklist.bytes, 0xa5, sizeof(g_native_worklist.bytes));
    *(unsigned short *)(source_bytes + 0x2e) = 1;
    *(unsigned short *)(native_bytes + 0x2e) = 1;
    *(unsigned short *)(source_bytes + stride + 0x2e) = 0;
    *(unsigned short *)(native_bytes + stride + 0x2e) = 0;
    if (fail_allocation) {
        *(unsigned short *)(source_bytes + 0x2e) = 1;
        *(unsigned short *)(native_bytes + 0x2e) = 1;
        if (family == 0) {
            g_normal_limit_wrap = 1;
            set_native_word(0x713a8UL, 1);
        } else if (family == 1) {
            g_middle_wrap_start_primary = 1;
            set_native_word(0x713a0UL, 1);
        } else {
            g_alternate_limit_wrap = 1;
            set_native_word(0x713b6UL, 1);
        }
    }
    seed_native_cells(family);
    /* seed_native_cells installs valid bounds; reapply the failure bound. */
    if (fail_allocation) {
        if (family == 0) set_native_word(0x713a8UL, 1);
        else if (family == 1) set_native_word(0x713a0UL, 1);
        else set_native_word(0x713b6UL, 1);
    }
    callback_record = (Eng1ForestRecord20 *)(worklist_row.bytes + 20);
    callback_count = 0;
    callback_failed = 0;
    callback_actor = 0;
    callback_record = (Eng1ForestRecord20 *)(worklist_row.bytes + 20);
    g_source_callback_order = 0;
    g_native_callback_order = 0;
    g_source_callback_actor = 0;
    g_native_callback_actor = 0;
    memset(g_source_callback_state, 0, sizeof(g_source_callback_state));
    memset(g_native_callback_state, 0, sizeof(g_native_callback_state));
    memset(g_source_callback_record, 0, sizeof(g_source_callback_record));
    memset(g_native_callback_record, 0, sizeof(g_native_callback_record));
}

static void make_case_record(int family, int native_side)
{
    unsigned char *row = native_side ? g_native_worklist.bytes : worklist_row.bytes;
    Eng1ForestRecord20 *record = (Eng1ForestRecord20 *)(row + 20);
    unsigned short kind = family == 0 ? 2U : family == 1 ? 0x4003U : 0x2005U;
    record->kind = kind;
    record->flags = family == 0 ? 0x1045U : family == 1 ? 0x0045U : 0x0101U;
    record->state = family == 0 ? 0x1234U : family == 1 ? 0xbeefU : 0x9876U;
    record->x = family == 0 ? 0x00120000L : family == 1 ? 0x00210000L : 0x00310000L;
    record->y = family == 0 ? 0x00030000L : family == 1 ? 0x00070000L : 0x00090000L;
    record->z = family == 0 ? 1L : family == 1 ? 0x00010001L : 0x000b0000L;
}

static void prepare_table_case(int family)
{
    unsigned short kind = family == 0 ? 2U : family == 1 ? 0x4003U : 0x2005U;
    unsigned long index = family == 0 ? kind : (unsigned long)(kind & 0x0fffU);
    if (family == 0) {
        set32(eng1_fixture_normal_assets + index * 16UL, 0x11112222UL);
        set32(eng1_fixture_normal_assets + index * 16UL + 4UL, 0x33334444UL);
        set32(eng1_fixture_normal_assets + index * 16UL + 8UL, 0x55556666UL);
        eng1_fixture_normal_assets[index * 16UL + 12UL] = 0x5aU;
        eng1_fixture_normal_actions[index * 3UL] = differential_action_probe;
        expect(kind, 0x1234U, 0x5aU, 0x11112222UL, 0x33334444UL,
               0x55556666UL, 0xffU, 1, 0x80000010UL, 1, 1, 1, 1);
    } else if (family == 1) {
        set32(eng1_fixture_middle_assets + index * 8UL, 0x22223333UL);
        eng1_fixture_middle_assets[index * 8UL + 6UL] = 0x33U;
        eng1_fixture_middle_actions[index * 2UL] = differential_action_probe;
        expect(kind, 0, 0x33U, 0x22223333UL, 0, 0,
               0, 1, 0x80000000UL, 1, 0, 0, 1);
    } else {
        set32(eng1_fixture_alternate_assets + index * 8UL, 0x44445555UL);
        eng1_fixture_alternate_assets[index * 8UL + 6UL] = 0x77U;
        eng1_fixture_alternate_actions[index * 2UL] = differential_action_probe;
        expect(kind, 0, 0x77U, 0x44445555UL, 0, 0,
               0, 0, 0x80000000UL, 0, 0, 0, 0);
    }
}

static void *__cdecl invoke_native(int family, void *record)
{
    if (family == 0)
        return ((void *(__cdecl *)(void *, unsigned long))native_cell(0x270c0UL))(record, 0);
    if (family == 1)
        return ((void *(__cdecl *)(void *, unsigned long))native_cell(0x27720UL))(record, 0);
    return ((void *(__cdecl *)(void *, unsigned long))native_cell(0x27c40UL))(record, 0);
}

static void *__cdecl invoke_source(int family, void *record)
{
    if (family == 0) return eng1_call_270c0((Eng1ForestRecord20 *)record, 0);
    if (family == 1) return eng1_call_27720((Eng1ForestRecord20 *)record, 0);
    return eng1_call_27c40((Eng1ForestRecord20 *)record, 0);
}

static void successful_case(int family, const char *name)
{
    unsigned long bytes = pool_stride(family);
    unsigned short native_cursor_after;
    unsigned short source_cursor_after;
    void *source_result;
    void *native_result;
    unsigned char *source_actor = source_pool(family);
    unsigned char *native_actor = native_pool(family);

    configure_case(family, 0);
    make_case_record(family, 0);
    make_case_record(family, 1);
    prepare_table_case(family);

    g_native_mode = 0;
    g_active_family = family;
    source_result = invoke_source(family, worklist_row.bytes + 20);
    memcpy(g_source_final, source_actor, bytes * 2UL);
    memcpy(g_source_record_final, worklist_row.bytes + 20, 20);
    source_cursor_after = source_cursor(family);

    g_native_mode = 1;
    callback_count = 0;
    callback_failed = 0;
    callback_record = (Eng1ForestRecord20 *)(g_native_worklist.bytes + 20);
    seed_native_cells(family);
    /* Native callback table uses the same finite fixture row as source. */
    native_result = invoke_native(family, g_native_worklist.bytes + 20);
    memcpy(g_native_final, native_actor, bytes * 2UL);
    memcpy(g_native_record_final, g_native_worklist.bytes + 20, 20);
    native_cursor_after = native_word(native_cursor_rva(family));
    g_native_mode = 0;

    check((unsigned char *)source_result - source_actor == (long)bytes,
          "source actor return normalized to pool slot 1");
    check((unsigned char *)native_result - native_actor == (long)bytes,
          "native actor return normalized to pool slot 1");
    check(g_source_callback_order == 1 && g_native_callback_order == 1,
          "one action callback follows each factory invocation");
    check(g_source_callback_actor == source_result &&
          g_native_callback_actor == native_result,
          "callback receives the selected actor on both sides");
    check(!callback_failed && callback_count == 1,
          "callback observes completed kind, owner-relative binding and asset fields");
    check(source_cursor_after == 1 && native_cursor_after == 1,
          "allocator cursor advances to the selected slot on both paths");
    check(memcmp(g_source_final, g_native_final, bytes * 2UL) == 0,
          "entire two-slot pool region including initialized actor and untouched gaps matches");
    check(memcmp(g_source_callback_state, g_native_callback_state, bytes) == 0,
          "full actor state at callback boundary matches");
    check(memcmp(g_source_record_final, g_native_record_final, 20) == 0,
          "complete 20-byte worklist record mutation matches");
    check(memcmp(g_source_record_final, g_source_callback_record, 20) == 0 &&
          memcmp(g_native_record_final, g_native_callback_record, 20) == 0,
          "callback sees the final complete worklist record mutation");
    check(memcmp(g_source_final + bytes, g_source_callback_state, bytes) == 0 &&
          memcmp(g_native_final + bytes, g_native_callback_state, bytes) == 0,
          "factory makes no actor writes after action callback");
    printf("PASS %s\n", name);
}

static void allocation_failure_case(int family, const char *name)
{
    unsigned long bytes = pool_stride(family);
    unsigned char source_before[0x268];
    unsigned char native_before[0x268];
    unsigned char source_record_before[20];
    unsigned char native_record_before[20];
    unsigned long source_size = bytes * 2UL;
    void *source_result;
    void *native_result;

    configure_case(family, 1);
    make_case_record(family, 0);
    make_case_record(family, 1);
    prepare_table_case(family);
    memcpy(source_before, source_pool(family), source_size);
    memcpy(native_before, native_pool(family), source_size);
    memcpy(source_record_before, worklist_row.bytes + 20, 20);
    memcpy(native_record_before, g_native_worklist.bytes + 20, 20);

    g_native_mode = 0;
    g_active_family = family;
    source_result = invoke_source(family, worklist_row.bytes + 20);
    g_native_mode = 1;
    callback_record = (Eng1ForestRecord20 *)(g_native_worklist.bytes + 20);
    callback_count = 0;
    callback_failed = 0;
    seed_native_cells(family);
    /* Reapply the failing source-equivalent bound after ordinary cell setup. */
    if (family == 0) set_native_word(0x713a8UL, 1);
    else if (family == 1) set_native_word(0x713a0UL, 1);
    else set_native_word(0x713b6UL, 1);
    native_result = invoke_native(family, g_native_worklist.bytes + 20);
    g_native_mode = 0;

    check(source_result == 0 && native_result == 0,
          "exhausted pool returns null on source and native paths");
    check(g_source_callback_order == 0 && g_native_callback_order == 0 &&
          callback_count == 0,
          "allocation failure skips action callbacks");
    check(memcmp(source_before, source_pool(family), source_size) == 0 &&
          memcmp(native_before, native_pool(family), source_size) == 0,
          "allocation failure preserves both full pool regions");
    check(memcmp(source_record_before, worklist_row.bytes + 20, 20) == 0 &&
          memcmp(native_record_before, g_native_worklist.bytes + 20, 20) == 0,
          "allocation failure preserves all record bytes");
    check(source_cursor(family) == 0 &&
          native_word(native_cursor_rva(family)) == 0,
          "failed allocation leaves both allocator cursor WORDs unchanged");
    printf("PASS %s\n", name);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("usage: worklist_native_diff <path-to-ENG1.DLL>\n");
        return 2;
    }
    g_native_module = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (g_native_module == 0) {
        printf("LoadLibraryExA error=%lu\n", GetLastError());
        return 3;
    }
    successful_case(0, "270c0 normal");
    successful_case(1, "27720 middle");
    successful_case(2, "27c40 alternate");
    allocation_failure_case(0, "270c0 allocation failure");
    allocation_failure_case(1, "27720 allocation failure");
    allocation_failure_case(2, "27c40 allocation failure");
    printf("worklist_native_diff cases=6 checks=%lu failures=%lu\n",
           g_checks, g_failures);
    FreeLibrary(g_native_module);
    return g_failures ? 1 : 0;
}
