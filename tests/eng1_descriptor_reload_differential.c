#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "../src/shared/engine_interface.h"
#include "../src/shared/resource_record.h"
#include "../src/eng1/resource_runtime_data.h"

#define NATIVE_ROWS_RVA 0x5cee0UL
#define NATIVE_INTERFACE_RVA 0x70f20UL
#define NATIVE_ACTIVE_RECORD_RVA 0x72864UL
#define NATIVE_LOADING_ID_RVA 0x4d7f8UL
#define NATIVE_TOTAL_BYTES_RVA 0x6ee6cUL
#define NATIVE_START_TICK_RVA 0x6ee70UL
#define NATIVE_OUTER_RVA 0x33e0UL
#define RESOURCE_COUNT 81UL
#define BUFFER_ROWS 4UL

extern EngineInterface g_engine_interface;
extern ActorResource g_actor_resources[81];
extern ActorResource *g_active_resource_record;
extern unsigned long g_reported_total_bytes;
extern unsigned long g_resource_start_tick;
extern void __cdecl eng1_target_033e0(const unsigned char *,
                                      const unsigned char *);
extern void __cdecl dispatch_engine_2c2d0(void *);

ResourceCallbackContext *g_resource_context;

static HMODULE g_oracle;
static ActorResource *g_native_rows;
static unsigned long g_active_side;
static unsigned long g_checks;
static unsigned long g_failures;
static unsigned long g_guard_reads;
static unsigned long g_identity_guards;
static unsigned long g_unexpected_calls;
static const unsigned char *g_allowed_first;
static const unsigned char *g_allowed_second;
static unsigned long g_first_extent;
static unsigned long g_second_extent;
static unsigned long g_file_length_calls[2];
static unsigned long g_cancel_calls[2];
static unsigned long g_compact_calls[2];
static unsigned long g_event_count[2];
static unsigned long g_event_overflow;

static ActorReference g_allocation_buffers[81][BUFFER_ROWS];
static ActorReference g_preallocated_buffer[BUFFER_ROWS];
static ResourceCallbackContext g_native_context;
static ResourceCallbackContext g_source_context;
static const char g_fixture_path[] = "P:\\SRC1\\FIXTURE\\RESOURCE";
static const char g_fixture_suffix[] = "\\ANIMPSX.BIN";

typedef struct CallbackEvent {
    char operation;
    unsigned long resource_id;
    unsigned long bytes;
    unsigned long flags;
    void *destination;
    char path[64];
    char suffix[32];
} CallbackEvent;

static CallbackEvent g_events[2][64];

typedef struct Snapshot {
    ActorResource rows[81];
    ResourceCallbackContext context;
    unsigned char buffers[81][BUFFER_ROWS * sizeof(ActorReference)];
    unsigned char preallocated[BUFFER_ROWS * sizeof(ActorReference)];
    CallbackEvent events[64];
    unsigned long event_count;
    unsigned long file_length_calls;
    unsigned long cancel_calls;
    unsigned long compact_calls;
    long active_row;
    int loading_resource;
    unsigned long total_bytes;
    unsigned long start_tick;
} Snapshot;

static void check_case(const char *case_name, const char *label, int ok)
{
    ++g_checks;
    if (!ok) {
        ++g_failures;
        printf("FAIL %s: %s\n", case_name, label);
    }
}

static void append_event(char operation, unsigned long resource_id,
                         unsigned long bytes, unsigned long flags,
                         void *destination, const char *path,
                         const char *suffix)
{
    CallbackEvent *event;
    unsigned long index;

    if (g_active_side > 1UL) {
        ++g_event_overflow;
        return;
    }
    index = g_event_count[g_active_side];
    if (index >= 64UL) {
        ++g_event_overflow;
        return;
    }
    event = &g_events[g_active_side][index];
    memset(event, 0, sizeof(*event));
    event->operation = operation;
    event->resource_id = resource_id;
    event->bytes = bytes;
    event->flags = flags;
    event->destination = destination;
    if (path != 0)
        strncpy(event->path, path, sizeof(event->path) - 1UL);
    if (suffix != 0)
        strncpy(event->suffix, suffix, sizeof(event->suffix) - 1UL);
    ++g_event_count[g_active_side];
}

static ActorResource *current_rows(void)
{
    return g_active_side == 0UL ? g_native_rows : g_actor_resources;
}

static int resource_id_for_output_slot(void **slot)
{
    ActorResource *rows;
    unsigned long index;

    rows = current_rows();
    for (index = 0; index < RESOURCE_COUNT; ++index) {
        if (slot == (void **)&rows[index].references)
            return (int)index;
    }
    return -1;
}

static int resource_id_for_load_destination(void *destination)
{
    ActorResource *rows;
    unsigned long index;
    unsigned long prefix_bytes;
    void *expected;

    rows = current_rows();
    for (index = 0; index < RESOURCE_COUNT; ++index) {
        prefix_bytes = ((unsigned long)rows[index].entry_count + 1UL) * 4UL;
        expected = (unsigned char *)&g_allocation_buffers[index][0] + prefix_bytes;
        if (destination == expected)
            return (int)index;
    }
    return -1;
}

static void __cdecl callback_marker(void)
{
}

static void *__cdecl allocate_spy(void **output_slot, unsigned long bytes,
                                  unsigned long flags)
{
    int resource_id;
    void *result;

    resource_id = resource_id_for_output_slot(output_slot);
    if (resource_id < 0) {
        ++g_unexpected_calls;
        return 0;
    }
    result = &g_allocation_buffers[resource_id][0];
    append_event('A', (unsigned long)resource_id, bytes, flags, result, 0, 0);
    *output_slot = result;
    return result;
}

static unsigned long __cdecl file_length_spy(const char *path,
                                              const char *suffix)
{
    if (g_active_side < 2UL)
        ++g_file_length_calls[g_active_side];
    append_event('F', 0xffffffffUL, 0UL, 0UL, 0, path, suffix);
    return 17UL;
}

static unsigned long __cdecl load_sync_spy(const char *path,
                                            const char *suffix,
                                            void *destination,
                                            unsigned long bytes)
{
    int resource_id;

    resource_id = resource_id_for_load_destination(destination);
    if (resource_id < 0) {
        ++g_unexpected_calls;
        append_event('L', 0xffffffffUL, bytes, 0UL, destination, path, suffix);
        return 0UL;
    }
    append_event('L', (unsigned long)resource_id, bytes, 0UL,
                 destination, path, suffix);
    return 1UL;
}

static int __cdecl load_async_spy(const char *path, const char *suffix,
                                  int sector_offset, void *destination,
                                  unsigned long bytes)
{
    (void)path;
    (void)suffix;
    (void)sector_offset;
    (void)destination;
    (void)bytes;
    ++g_unexpected_calls;
    return 0;
}

static int __cdecl cancel_spy(void)
{
    if (g_active_side < 2UL)
        ++g_cancel_calls[g_active_side];
    append_event('C', 0xffffffffUL, 0UL, 0UL, 0, 0, 0);
    return 0;
}

static unsigned long __cdecl compact_spy(int pool_id)
{
    (void)pool_id;
    if (g_active_side < 2UL)
        ++g_compact_calls[g_active_side];
    return 1UL;
}

static void __cdecl free_spy(void *pointer)
{
    append_event('D', 0xffffffffUL, 0UL, 0UL, pointer, 0, 0);
}

/* The only list addresses accepted by the finite guard spy are test-owned
 * arrays. The terminator itself is readable; every later byte is rejected. */
static unsigned long expected_extent(const unsigned char *base)
{
    if (base == g_allowed_first)
        return g_first_extent;
    if (base == g_allowed_second)
        return g_second_extent;
    return 0xffffffffUL;
}

void eng1_guard_initial_read(const void *base, unsigned long index,
                             unsigned long stride, unsigned long width,
                             const char *site)
{
    const unsigned char *bytes;
    unsigned long extent;

    (void)site;
    bytes = (const unsigned char *)base;
    extent = expected_extent(bytes);
    if (extent == 0xffffffffUL || stride != 1UL || width != 1UL ||
        index >= extent) {
        ++g_unexpected_calls;
        return;
    }
    ++g_guard_reads;
}

void eng1_guard_unreadable_identity(const void *base, const char *site)
{
    ++g_identity_guards;
    eng1_guard_initial_read(base, 0UL, 1UL, 1UL, site);
}

/* Unreached branches are traps, so a successful fixture cannot silently
 * substitute effects for the actual provider/callback sources. */
void __cdecl resource_reclaim(int reason)
{
    (void)reason;
    ++g_unexpected_calls;
}

void __cdecl engine_diagnostic(const char *format, ...)
{
    (void)format;
    ++g_unexpected_calls;
}

void __cdecl host_cancel_load(void)
{
    ++g_unexpected_calls;
}

void __cdecl release_resource_buffer(void *buffer)
{
    dispatch_engine_2c2d0(buffer);
}

typedef void (__cdecl *NativeEntry)(const unsigned char *,
                                    const unsigned char *);

static unsigned long find_extent(const unsigned char *list)
{
    unsigned long index;
    if (list == 0)
        return 0UL;
    for (index = 0; index < 8UL; ++index) {
        if (list[index] == 0xffU)
            return index + 1UL;
    }
    return 0UL;
}

static void initialize_rows(ActorResource *rows)
{
    static const unsigned short selected_ids[] = { 1U, 2U, 3U, 5U, 6U, 7U };
    unsigned long index;
    unsigned long i;

    memset(rows, 0, 81UL * sizeof(rows[0]));
    for (index = 0; index < sizeof(selected_ids) / sizeof(selected_ids[0]); ++index) {
        i = selected_ids[index];
        rows[i].path = (void *)g_fixture_path;
        rows[i].entry_count = 2U;
        rows[i].unknown_006 = 0x1122U;
        rows[i].file_length = 17UL;
        rows[i].residency_state = 1U;
        rows[i].unknown_012 = (unsigned short)(0x4400U + i);
    }
    rows[7].references = &g_preallocated_buffer[0];
    rows[7].residency_state = 3U;
    rows[7].unknown_012 = 0x7788U;
}

static void reset_observers(void)
{
    memset(g_event_count, 0, sizeof(g_event_count));
    memset(g_file_length_calls, 0, sizeof(g_file_length_calls));
    memset(g_cancel_calls, 0, sizeof(g_cancel_calls));
    memset(g_compact_calls, 0, sizeof(g_compact_calls));
    g_event_overflow = 0UL;
    g_guard_reads = 0UL;
    g_identity_guards = 0UL;
    g_unexpected_calls = 0UL;
}

static void prepare_context(ResourceCallbackContext *context)
{
    memset(context, 0x5a, sizeof(*context));
    context->tick_038 = 0x13579bdfUL;
    context->mode_09c = 0;
    context->blocked_0df = 0;
    context->load_complete = callback_marker;
    context->load_failed = callback_marker;
}

static void prepare_native_interface(void)
{
    EngineInterface *table;

    table = (EngineInterface *)((unsigned char *)g_oracle +
                                NATIVE_INTERFACE_RVA);
    memset(table, 0, sizeof(*table));
    table->context_004 = &g_native_context;
    table->load_async_020 = load_async_spy;
    table->cancel_load_024 = cancel_spy;
    table->compact_pool_034 = compact_spy;
    table->free_allocation_040 = free_spy;
    table->file_length_048 = file_length_spy;
    table->load_sync_068 = load_sync_spy;
    table->allocate_078 = allocate_spy;
}

static void prepare_source_interface(void)
{
    memset(&g_engine_interface, 0, sizeof(g_engine_interface));
    g_engine_interface.context_004 = &g_source_context;
    g_engine_interface.load_async_020 = load_async_spy;
    g_engine_interface.cancel_load_024 = cancel_spy;
    g_engine_interface.compact_pool_034 = compact_spy;
    g_engine_interface.free_allocation_040 = free_spy;
    g_engine_interface.file_length_048 = file_length_spy;
    g_engine_interface.load_sync_068 = load_sync_spy;
    g_engine_interface.allocate_078 = allocate_spy;
    g_resource_context = &g_source_context;
}

static long active_row_index(ActorResource *active, ActorResource *rows)
{
    if (active == 0)
        return -1L;
    if (active >= rows && active < rows + RESOURCE_COUNT)
        return (long)(active - rows);
    return -2L;
}

static void snapshot_native(Snapshot *out)
{
    ActorResource *active;
    int *loading;
    unsigned long *total;
    unsigned long *tick;

    memset(out, 0, sizeof(*out));
    memcpy(out->rows, g_native_rows, sizeof(out->rows));
    memcpy(&out->context, &g_native_context, sizeof(out->context));
    memcpy(out->buffers, g_allocation_buffers, sizeof(out->buffers));
    memcpy(out->preallocated, g_preallocated_buffer, sizeof(out->preallocated));
    memcpy(out->events, g_events[0], sizeof(out->events));
    out->event_count = g_event_count[0];
    out->file_length_calls = g_file_length_calls[0];
    out->cancel_calls = g_cancel_calls[0];
    out->compact_calls = g_compact_calls[0];
    active = *(ActorResource **)((unsigned char *)g_oracle +
                                  NATIVE_ACTIVE_RECORD_RVA);
    out->active_row = active_row_index(active, g_native_rows);
    loading = (int *)((unsigned char *)g_oracle + NATIVE_LOADING_ID_RVA);
    total = (unsigned long *)((unsigned char *)g_oracle + NATIVE_TOTAL_BYTES_RVA);
    tick = (unsigned long *)((unsigned char *)g_oracle + NATIVE_START_TICK_RVA);
    out->loading_resource = *loading;
    out->total_bytes = *total;
    out->start_tick = *tick;
}

static void snapshot_source(Snapshot *out)
{
    memset(out, 0, sizeof(*out));
    memcpy(out->rows, g_actor_resources, sizeof(out->rows));
    memcpy(&out->context, &g_source_context, sizeof(out->context));
    memcpy(out->buffers, g_allocation_buffers, sizeof(out->buffers));
    memcpy(out->preallocated, g_preallocated_buffer, sizeof(out->preallocated));
    memcpy(out->events, g_events[1], sizeof(out->events));
    out->event_count = g_event_count[1];
    out->file_length_calls = g_file_length_calls[1];
    out->cancel_calls = g_cancel_calls[1];
    out->compact_calls = g_compact_calls[1];
    out->active_row = active_row_index(g_active_resource_record, g_actor_resources);
    out->loading_resource = g_loading_resource;
    out->total_bytes = g_reported_total_bytes;
    out->start_tick = g_resource_start_tick;
}

static int same_event(const CallbackEvent *left, const CallbackEvent *right)
{
    return left->operation == right->operation &&
           left->resource_id == right->resource_id &&
           left->bytes == right->bytes && left->flags == right->flags &&
           left->destination == right->destination &&
           strcmp(left->path, right->path) == 0 &&
           strcmp(left->suffix, right->suffix) == 0;
}

static void compare_snapshots(const char *name, const Snapshot *native,
                              const Snapshot *source)
{
    unsigned long index;
    unsigned long differing_rows;

    differing_rows = 0UL;
    for (index = 0; index < RESOURCE_COUNT; ++index) {
        if (memcmp(&native->rows[index], &source->rows[index],
                   sizeof(native->rows[index])) != 0) {
            ++differing_rows;
            printf("ROWDIFF %s %lu native(state=%u count=%u refs=%p aux=%u len=%lu) source(state=%u count=%u refs=%p aux=%u len=%lu)\n",
                   name, index,
                   native->rows[index].residency_state,
                   native->rows[index].entry_count,
                   native->rows[index].references,
                   native->rows[index].unknown_012,
                   native->rows[index].file_length,
                   source->rows[index].residency_state,
                   source->rows[index].entry_count,
                   source->rows[index].references,
                   source->rows[index].unknown_012,
                   source->rows[index].file_length);
        }
    }
    check_case(name, "all 81 full rows match", differing_rows == 0UL);
    check_case(name, "callback context bytes match",
               memcmp(&native->context, &source->context,
                      sizeof(native->context)) == 0);
    check_case(name, "provider reference buffers match",
               memcmp(native->buffers, source->buffers,
                      sizeof(native->buffers)) == 0);
    check_case(name, "preallocated reference buffer is unchanged",
               memcmp(native->preallocated, source->preallocated,
                      sizeof(native->preallocated)) == 0);
    check_case(name, "provider active row matches",
               native->active_row == source->active_row);
    check_case(name, "loading resource id matches",
               native->loading_resource == source->loading_resource);
    check_case(name, "reported total bytes match",
               native->total_bytes == source->total_bytes);
    check_case(name, "provider start tick matches",
               native->start_tick == source->start_tick);
    check_case(name, "file length callback counts match",
               native->file_length_calls == source->file_length_calls);
    check_case(name, "cancel callback counts match",
               native->cancel_calls == source->cancel_calls);
    check_case(name, "compact callback counts match",
               native->compact_calls == source->compact_calls);
    check_case(name, "host callback event count matches",
               native->event_count == source->event_count);
    if (native->event_count == source->event_count) {
        for (index = 0; index < native->event_count; ++index)
            check_case(name, "host callback arguments and order match",
                       same_event(&native->events[index],
                                  &source->events[index]));
    }
}

static int invoke_native(const unsigned char *first,
                         const unsigned char *second)
{
    NativeEntry entry;
    entry = (NativeEntry)((unsigned char *)g_oracle + NATIVE_OUTER_RVA);
    entry(first, second);
    return 1;
}

static void reset_provider_state(void)
{
    ActorResource **active;
    int *loading;
    unsigned long *total;
    unsigned long *tick;

    active = (ActorResource **)((unsigned char *)g_oracle +
                                 NATIVE_ACTIVE_RECORD_RVA);
    loading = (int *)((unsigned char *)g_oracle + NATIVE_LOADING_ID_RVA);
    total = (unsigned long *)((unsigned char *)g_oracle + NATIVE_TOTAL_BYTES_RVA);
    tick = (unsigned long *)((unsigned char *)g_oracle + NATIVE_START_TICK_RVA);
    *active = 0;
    *loading = -1;
    *total = 0xdeadbeefUL;
    *tick = 0xabcdef01UL;
    g_active_resource_record = 0;
    g_loading_resource = -1;
    g_reported_total_bytes = 0xdeadbeefUL;
    g_resource_start_tick = 0xabcdef01UL;
}

static void configure_list_guards(const unsigned char *first,
                                  const unsigned char *second)
{
    g_allowed_first = first;
    g_allowed_second = second;
    g_first_extent = find_extent(first);
    g_second_extent = find_extent(second);
}

static void reset_buffers(void)
{
    memset(g_allocation_buffers, 0xa5, sizeof(g_allocation_buffers));
    memset(g_preallocated_buffer, 0x6d, sizeof(g_preallocated_buffer));
}

static int run_native(const unsigned char *first, const unsigned char *second,
                      Snapshot *out)
{
    unsigned long *total;
    unsigned long *tick;
    int *loading;

    reset_observers();
    reset_buffers();
    prepare_context(&g_native_context);
    memset(g_native_rows, 0, 81UL * sizeof(g_native_rows[0]));
    initialize_rows(g_native_rows);
    prepare_native_interface();
    reset_provider_state();
    total = (unsigned long *)((unsigned char *)g_oracle + NATIVE_TOTAL_BYTES_RVA);
    tick = (unsigned long *)((unsigned char *)g_oracle + NATIVE_START_TICK_RVA);
    loading = (int *)((unsigned char *)g_oracle + NATIVE_LOADING_ID_RVA);
    *total = 0xdeadbeefUL;
    *tick = 0xabcdef01UL;
    *loading = -1;
    g_active_side = 0UL;
    if (!invoke_native(first, second))
        return 0;
    snapshot_native(out);
    return 1;
}

static void run_source(const unsigned char *first, const unsigned char *second,
                       Snapshot *out)
{
    reset_observers();
    reset_buffers();
    prepare_context(&g_source_context);
    initialize_rows(g_actor_resources);
    prepare_source_interface();
    reset_provider_state();
    g_active_side = 1UL;
    eng1_target_033e0(first, second);
    snapshot_source(out);
}

static void run_case(const char *name, const unsigned char *first,
                     const unsigned char *second)
{
    Snapshot native_snapshot;
    Snapshot source_snapshot;
    int native_ok;

    configure_list_guards(first, second);
    native_ok = run_native(first, second, &native_snapshot);
    check_case(name, "unpatched native entry completed", native_ok);
    run_source(first, second, &source_snapshot);
    check_case(name, "fixture-only guard saw exactly the readable prefix",
               g_unexpected_calls == 0UL && g_guard_reads != 0UL);
    check_case(name, "resource pool reclaim/diagnostic/cancel trap stayed unused",
               g_unexpected_calls == 0UL);
    check_case(name, "observer event buffer did not overflow",
               g_event_overflow == 0UL);
    compare_snapshots(name, &native_snapshot, &source_snapshot);
}

static void assert_comprehensive(void)
{
    static const unsigned long expected_ids[] = { 1UL, 3UL, 5UL };
    unsigned long index;
    unsigned long event_index;
    unsigned long id;

    check_case("comprehensive", "duplicate id and populated row load once each",
               g_event_count[1] == 6UL);
    for (index = 0; index < sizeof(expected_ids) / sizeof(expected_ids[0]); ++index) {
        id = expected_ids[index];
        event_index = index * 2UL;
        check_case("comprehensive", "provider event order is ascending resource id",
                   g_events[1][event_index].operation == 'A' &&
                   g_events[1][event_index].resource_id == id &&
                   g_events[1][event_index + 1UL].operation == 'L' &&
                   g_events[1][event_index + 1UL].resource_id == id);
        check_case("comprehensive", "allocation request uses 4K rounded size and flags 0x20",
                   g_events[1][event_index].bytes == 0x1000UL &&
                   g_events[1][event_index].flags == 0x20UL);
        check_case("comprehensive", "sync load receives observed prefix and file length",
                   g_events[1][event_index + 1UL].bytes == 17UL &&
                   strcmp(g_events[1][event_index + 1UL].path,
                          g_fixture_path) == 0 &&
                   strcmp(g_events[1][event_index + 1UL].suffix,
                          g_fixture_suffix) == 0);
        check_case("comprehensive", "loaded row reaches resident state and aux 2",
                   g_actor_resources[id].residency_state == 3U &&
                   g_actor_resources[id].unknown_012 == 2U);
        check_case("comprehensive", "provider initializes entry pairs but preserves tail bytes",
                   ((unsigned char *)g_allocation_buffers[id])[0] == 0U &&
                   ((unsigned char *)g_allocation_buffers[id])[11] == 0U &&
                   ((unsigned char *)g_allocation_buffers[id])[12] == 0xa5U);
    }
    check_case("comprehensive", "preallocated row skips provider and preserves auxiliary word",
               g_actor_resources[7].references == &g_preallocated_buffer[0] &&
               g_actor_resources[7].unknown_012 == 0x7788U);
    check_case("comprehensive", "file length was cached in each selected row already and is not queried",
               g_file_length_calls[1] == 0UL);
    check_case("comprehensive", "every nonnull byte-list scan reaches its first terminator",
               g_identity_guards == 162UL &&
               g_guard_reads == 81UL * ((4UL + 1UL) + (3UL + 1UL)));
    check_case("comprehensive", "provider globals retain final row and real completion state",
               g_active_resource_record == &g_actor_resources[5] &&
               g_loading_resource == 5 && g_reported_total_bytes == 29UL &&
               g_resource_start_tick == 0x13579bdfUL);
}

static void assert_terminator_case(void)
{
    check_case("terminator", "match after first 0xff is ignored",
               g_actor_resources[2].references == 0 &&
               g_actor_resources[2].unknown_012 == 0x4402U);
    check_case("terminator", "byte before 0xff still loads normally",
               g_actor_resources[6].residency_state == 3U &&
               g_actor_resources[6].unknown_012 == 2U);
    check_case("terminator", "each outer pass reads only through the terminator",
               g_identity_guards == 81UL && g_guard_reads == 81UL * 3UL);
}

static void assert_empty_case(void)
{
    check_case("null-sentinel", "null first list and sentinel-only second list do not load",
               g_event_count[1] == 0UL && g_actor_resources[1].references == 0 &&
               g_actor_resources[1].residency_state == 1U);
    check_case("null-sentinel", "null list bypasses read guard and sentinel is checked once per id",
               g_identity_guards == 81UL && g_guard_reads == 162UL);
}

int main(int argc, char **argv)
{
    static const unsigned char comprehensive_first[] = { 7U, 5U, 1U, 0xffU };
    static const unsigned char comprehensive_second[] = { 1U, 3U, 0xffU };
    static const unsigned char after_terminator[] = { 6U, 0xffU, 2U, 0xffU };
    static const unsigned char sentinel_only[] = { 0xffU };
    Snapshot unused_snapshot;

    (void)unused_snapshot;
    if (argc != 2) {
        printf("usage: native_diff ENG1.DLL\n");
        return 2;
    }
    g_oracle = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (g_oracle == 0) {
        printf("LoadLibraryEx failed: %lu\n", GetLastError());
        return 3;
    }
    g_native_rows = (ActorResource *)((unsigned char *)g_oracle +
                                      NATIVE_ROWS_RVA);

    run_case("comprehensive", comprehensive_first, comprehensive_second);
    assert_comprehensive();
    run_case("terminator", after_terminator, 0);
    assert_terminator_case();
    run_case("null-sentinel", 0, sentinel_only);
    assert_empty_case();

    printf("eng1_033e0_native_diff checks=%lu failures=%lu\n",
           g_checks, g_failures);
    FreeLibrary(g_oracle);
    return g_failures == 0UL ? 0 : 1;
}
