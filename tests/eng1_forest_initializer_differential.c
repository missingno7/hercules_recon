#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../calibration/engine_interface.h"
#include "../src/eng1/behavioral/setup_shared_state.h"
#include "../src/eng1/behavioral/forest_entry_data.h"

#define SLOT_COUNT 20
#define RECORD_WORDS 10
#define PIPELINE_BYTES 0x8c0
#define NATIVE_BASE 0x10000000UL

extern void __cdecl eng1_native_code_target_003ca10(void);
extern unsigned short *g_command_slot_storage_71320[SLOT_COUNT];
extern int __cdecl initialize_command_table(unsigned long, unsigned int,
                                            unsigned long);

typedef struct Event {
    unsigned long slot_index;
    unsigned long bytes;
    unsigned long flags;
    unsigned short word_718e0;
    unsigned short word_70d04;
    unsigned short word_7183c;
    int pointer_matches;
} Event;

typedef struct RunLog {
    Event events[SLOT_COUNT + 2];
    unsigned long calls;
    unsigned long successes;
    unsigned long fail_at;
} RunLog;
typedef void *(__cdecl *AllocatorCallback)(void **, unsigned long,
                                           unsigned long);

static HMODULE g_oracle;
static RunLog g_native_log, g_source_log;
static unsigned short g_native_records[SLOT_COUNT][RECORD_WORDS];
static unsigned short g_source_records[SLOT_COUNT][RECORD_WORDS];
static unsigned short g_native_preseed[RECORD_WORDS];
static unsigned short g_source_preseed[RECORD_WORDS];
static int (__cdecl *g_native_entry)(void);
static unsigned char g_saved_pipeline[PIPELINE_BYTES];
static unsigned char g_saved_slots[SLOT_COUNT * sizeof(void *)];
static unsigned char g_saved_camera_adjust_y[sizeof(unsigned short)];
static unsigned char g_saved_allocator_cell[sizeof(void *)];
static unsigned long g_checks;
static unsigned long g_failures;

#define CHECK(c, m) do { ++g_checks; if (!(c)) { \
    ++g_failures; printf("FAIL line %d: %s\n", __LINE__, m); return 1; \
} } while (0)

static unsigned char *native_cell(unsigned long rva)
{
    return (unsigned char *)g_oracle + rva;
}

static void snapshot_native_cells(void)
{
    memcpy(g_saved_pipeline, native_cell(0x71440), sizeof(g_saved_pipeline));
    memcpy(g_saved_slots, native_cell(0x71320), sizeof(g_saved_slots));
    memcpy(g_saved_camera_adjust_y, native_cell(0x70d04),
           sizeof(g_saved_camera_adjust_y));
    memcpy(g_saved_allocator_cell, native_cell(0x70f98),
           sizeof(g_saved_allocator_cell));
}

static void restore_native_cells(void)
{
    memcpy(native_cell(0x71440), g_saved_pipeline, sizeof(g_saved_pipeline));
    memcpy(native_cell(0x71320), g_saved_slots, sizeof(g_saved_slots));
    memcpy(native_cell(0x70d04), g_saved_camera_adjust_y,
           sizeof(g_saved_camera_adjust_y));
    memcpy(native_cell(0x70f98), g_saved_allocator_cell,
           sizeof(g_saved_allocator_cell));
}

static void seed_records(unsigned short records[SLOT_COUNT][RECORD_WORDS],
                         unsigned short preseed[RECORD_WORDS])
{
    unsigned long i, j;
    for (i = 0; i < SLOT_COUNT; ++i)
        for (j = 0; j < RECORD_WORDS; ++j)
            records[i][j] = (unsigned short)(0xa500U + i * 16U + j);
    for (j = 0; j < RECORD_WORDS; ++j)
        preseed[j] = (unsigned short)(0x5100U + j);
}

static void record_event(RunLog *log, unsigned long slot_index,
                         unsigned long bytes, unsigned long flags,
                         unsigned short word_718e0,
                         unsigned short word_70d04,
                         unsigned short word_7183c, int pointer_matches)
{
    Event *event;
    if (log->calls >= SLOT_COUNT + 2)
        return;
    event = &log->events[log->calls];
    event->slot_index = slot_index;
    event->bytes = bytes;
    event->flags = flags;
    event->word_718e0 = word_718e0;
    event->word_70d04 = word_70d04;
    event->word_7183c = word_7183c;
    event->pointer_matches = pointer_matches;
}

static void mutate_native_at_callback(unsigned long call_number)
{
    if (call_number == 5) {
        *(unsigned short *)(void *)native_cell(0x718e0) = 0x2468;
        *(unsigned short *)(void *)native_cell(0x70d04) = 0x1122;
        *(unsigned short *)(void *)native_cell(0x7183c) = 0x3344;
    }
}

static void mutate_source_at_callback(unsigned long call_number)
{
    if (call_number == 5) {
        g_eng1_setup_word_718e0 = 0x2468;
        g_camera_adjust_y_70d04 = 0x1122;
        g_eng1_setup_word_7183c = 0x3344;
    }
}

static void *__cdecl native_allocator_spy(void **output_slot,
                                           unsigned long bytes,
                                           unsigned long flags)
{
    unsigned long call_number = g_native_log.calls + 1;
    unsigned long slot_index = SLOT_COUNT;
    void **native_slots = (void **)(void *)native_cell(0x71320);
    unsigned long i;
    unsigned short *result;

    for (i = 0; i < SLOT_COUNT; ++i)
        if (output_slot == &native_slots[i])
            slot_index = i;
    record_event(&g_native_log, slot_index, bytes, flags,
                 *(unsigned short *)(void *)native_cell(0x718e0),
                 *(unsigned short *)(void *)native_cell(0x70d04),
                 *(unsigned short *)(void *)native_cell(0x7183c),
                 *(void **)(void *)native_cell(0x71714) ==
                     (void *)(void *)native_cell(0x66d68));
    ++g_native_log.calls;
    mutate_native_at_callback(call_number);
    if (call_number == g_native_log.fail_at)
        return 0;
    if (slot_index == SLOT_COUNT || g_native_log.successes >= SLOT_COUNT)
        return 0;

    result = g_native_records[g_native_log.successes++];
    *output_slot = result;
    return result;
}

static void *__cdecl source_allocator_spy(void **output_slot,
                                           unsigned long bytes,
                                           unsigned long flags)
{
    unsigned long call_number = g_source_log.calls + 1;
    unsigned long slot_index = SLOT_COUNT;
    unsigned long i;
    unsigned short *result;

    for (i = 0; i < SLOT_COUNT; ++i)
        if (output_slot == (void **)&g_command_slot_storage_71320[i])
            slot_index = i;
    record_event(&g_source_log, slot_index, bytes, flags,
                 g_eng1_setup_word_718e0,
                 (unsigned short)g_camera_adjust_y_70d04,
                 g_eng1_setup_word_7183c,
                 *(void **)(void *)&g_actor_pipeline_storage_71440.byte_cells[0x2d4] ==
                     (void *)g_eng1_sound_words_66d68);
    ++g_source_log.calls;
    mutate_source_at_callback(call_number);
    if (call_number == g_source_log.fail_at)
        return 0;
    if (slot_index == SLOT_COUNT || g_source_log.successes >= SLOT_COUNT)
        return 0;

    result = g_source_records[g_source_log.successes++];
    *output_slot = result;
    return result;
}

static int reset_case(unsigned long fail_at)
{
    unsigned long i;
    memset(&g_native_log, 0, sizeof(g_native_log));
    memset(&g_source_log, 0, sizeof(g_source_log));
    g_native_log.fail_at = fail_at;
    g_source_log.fail_at = fail_at;
    seed_records(g_native_records, g_native_preseed);
    seed_records(g_source_records, g_source_preseed);
    memset(g_command_slot_storage_71320, 0,
           sizeof(g_command_slot_storage_71320));
    memset(native_cell(0x71320), 0, SLOT_COUNT * sizeof(void *));
    g_command_slot_storage_71320[3] = g_source_preseed;
    *(unsigned short **)(void *)native_cell(0x71320 + 3 * 4) =
        g_native_preseed;

    for (i = 0; i < PIPELINE_BYTES; ++i)
        g_actor_pipeline_storage_71440.byte_cells[i] =
            (unsigned char)(0x31U + i * 7U);
    g_eng1_setup_word_718e0 = 0x1357;
    g_eng1_setup_word_7183c = 0x468a;
    *(unsigned long *)(void *)&g_actor_pipeline_storage_71440.byte_cells[0x2d4] =
        0x87654321UL;
    memcpy(native_cell(0x71440), &g_actor_pipeline_storage_71440,
           PIPELINE_BYTES);
    g_camera_adjust_y_70d04 = 0x369c;
    *(unsigned short *)(void *)native_cell(0x70d04) = 0x369c;
    return 0;
}

static unsigned long normalized_slot(void *value, int native)
{
    unsigned long i;
    unsigned short (*records)[RECORD_WORDS] =
        native ? g_native_records : g_source_records;
    unsigned short *preseed = native ? g_native_preseed : g_source_preseed;
    if (value == preseed)
        return 0xffffffffUL;
    for (i = 0; i < SLOT_COUNT; ++i)
        if (value == records[i])
            return i;
    return 0xfffffffeUL;
}

static int compare_results(const char *name)
{
    unsigned long i;
    unsigned long expected_calls = g_native_log.fail_at ?
        g_native_log.fail_at : 14UL;
    unsigned long expected_successes = g_native_log.fail_at ?
        g_native_log.fail_at - 1UL : 14UL;
    unsigned char *native_pipeline = native_cell(0x71440);
    unsigned short *native_slots = (unsigned short *)(void *)native_cell(0x71320);
    CHECK(g_native_log.calls == g_source_log.calls,
          "native/source allocator call counts differ");
    CHECK(g_native_log.calls == expected_calls,
          "unexpected allocator call count for success/failure control");
    CHECK(g_native_log.successes == g_source_log.successes,
          "native/source successful allocation counts differ");
    CHECK(g_native_log.successes == expected_successes,
          "unexpected successful allocation count for failure control");
    for (i = 0; i < g_native_log.calls; ++i) {
        Event *a = &g_native_log.events[i];
        Event *b = &g_source_log.events[i];
        unsigned long expected_slot = i < 3UL ? i : i + 1UL;
        CHECK(a->slot_index == expected_slot,
              "native allocator did not take the first free command slot");
        CHECK(a->slot_index == b->slot_index, "allocator slot ordering differs");
        CHECK(a->bytes == b->bytes && a->flags == b->flags,
              "allocator byte/flag arguments differ");
        CHECK(a->bytes == 0x14UL && a->flags == 0UL,
              "native allocator arguments differ from the 0x14/0 request");
        CHECK(a->word_718e0 == b->word_718e0 &&
              a->word_70d04 == b->word_70d04 &&
              a->word_7183c == b->word_7183c,
              "callback-visible state or callback order differs");
        CHECK(a->pointer_matches && b->pointer_matches,
              "pointer cell was not installed before allocation callbacks");
    }
    CHECK(g_native_log.events[0].word_718e0 == 0x0360 &&
          g_native_log.events[0].word_70d04 == 0x369c &&
          g_native_log.events[0].word_7183c == 0x468a,
          "callback did not observe initial stores before the final WORD writes");
    CHECK(g_native_log.events[5].word_718e0 == 0x2468 &&
          g_native_log.events[5].word_70d04 == 0x1122 &&
          g_native_log.events[5].word_7183c == 0x3344,
          "later callback did not observe live mutations from callback 5");
    CHECK(*(unsigned short *)(void *)native_cell(0x718e0) ==
          g_eng1_setup_word_718e0, "WORD 0x718e0 differs");
    CHECK(*(unsigned short *)(void *)native_cell(0x70d04) ==
          (unsigned short)g_camera_adjust_y_70d04, "WORD 0x70d04 differs");
    CHECK(*(unsigned short *)(void *)native_cell(0x7183c) ==
          g_eng1_setup_word_7183c, "WORD 0x7183c differs");
    CHECK(*(unsigned short *)(void *)native_cell(0x718e0) == 0x2468 &&
          *(unsigned short *)(void *)native_cell(0x70d04) == 0xfffe &&
          *(unsigned short *)(void *)native_cell(0x7183c) == 0xfffe,
          "callback effects and final -2 WORD stores have wrong order");
    for (i = 0; i < PIPELINE_BYTES; ++i) {
        if (i >= 0x2d4 && i < 0x2d8)
            continue;
        CHECK(native_pipeline[i] ==
              g_actor_pipeline_storage_71440.byte_cells[i],
              "shared actor-pipeline bytes differ");
    }
    for (i = 0; i < SLOT_COUNT; ++i) {
        void *native_value = *(void **)(void *)(native_slots + i * 2);
        void *source_value = g_command_slot_storage_71320[i];
        CHECK(normalized_slot(native_value, 1) ==
              normalized_slot(source_value, 0), "command slot contents differ");
    }
    CHECK(memcmp(g_native_records, g_source_records,
                 sizeof(g_native_records)) == 0,
          "allocated command record contents differ");
    CHECK(memcmp(native_cell(0x66d68), g_eng1_sound_words_66d68,
                 sizeof(g_eng1_sound_words_66d68)) == 0,
          "bounded native/source WORD prefix differs");
    printf("case=%s callbacks=%lu successful=%lu final=%04x,%04x,%04x\n",
           name, g_native_log.calls, g_native_log.successes,
           *(unsigned short *)(void *)native_cell(0x718e0),
           *(unsigned short *)(void *)native_cell(0x70d04),
           *(unsigned short *)(void *)native_cell(0x7183c));
    return 0;
}

static int run_case(const char *name, unsigned long fail_at)
{
    int native_eax;
    int result;
    CHECK(reset_case(fail_at) == 0, "case reset failed");
    *(AllocatorCallback *)(void *)native_cell(0x70f98) =
        native_allocator_spy;
    g_engine_interface.allocate_078 = source_allocator_spy;
    native_eax = g_native_entry();
    ++g_checks;
    if (native_eax != -2) {
        ++g_failures;
        restore_native_cells();
        printf("FAIL line %d: native raw EAX after stores differs from -2\n",
               __LINE__);
        return 1;
    }
    eng1_native_code_target_003ca10();
    result = compare_results(name);
    restore_native_cells();
    return result;
}

int main(int argc, char **argv)
{
    unsigned char *base;
    int cases = 0;
    if (argc != 2)
        return 2;
    g_oracle = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (!g_oracle) {
        printf("LoadLibraryExA failed %lu\n", GetLastError());
        return 3;
    }
    base = (unsigned char *)g_oracle;
    if ((unsigned long)base != NATIVE_BASE) {
        printf("unexpected image base %08lx\n", (unsigned long)base);
        FreeLibrary(g_oracle);
        return 4;
    }
    snapshot_native_cells();
    g_native_entry = (int (__cdecl *)(void))(void *)native_cell(0x3ca10);
    if (run_case("all-14", 0)) {
        printf("eng1_call_3ca10 cases=%d checks=%lu failures=%lu\n",
               cases, g_checks, g_failures);
        FreeLibrary(g_oracle);
        return 1;
    }
    ++cases;
    if (run_case("fail-at-6", 6)) {
        printf("eng1_call_3ca10 cases=%d checks=%lu failures=%lu\n",
               cases, g_checks, g_failures);
        FreeLibrary(g_oracle);
        return 1;
    }
    ++cases;
    printf("eng1_call_3ca10 cases=%d checks=%lu failures=%lu\n",
           cases, g_checks, g_failures);
    FreeLibrary(g_oracle);
    return g_failures ? 1 : 0;
}
