#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/shared/engine_interface.h"
#include "../src/shared/resource_record.h"

#define NATIVE_ROW_RVA 0x5cee0UL
#define NATIVE_INTERFACE_RVA 0x70f20UL
#define NATIVE_ENTRY_RVA 0x3910UL

extern EngineInterface g_engine_interface;
extern ActorResource g_actor_resources[81];
ResourceCallbackContext *g_resource_context;
extern void __cdecl eng1_target_03910(const unsigned char *,
                                                const unsigned char *);
extern int __cdecl engine_file_cancel(void);
extern void __cdecl dispatch_engine_2c2d0(void *);

void __cdecl host_cancel_load(void)
{
    (void)engine_file_cancel();
}

void __cdecl release_resource_buffer(void *buffer)
{
    dispatch_engine_2c2d0(buffer);
}

static unsigned long checks;
static unsigned long failures;
static unsigned long active_side;
static unsigned long guard_calls;
static ResourceCallbackContext native_context;
static ResourceCallbackContext source_context;
static unsigned long fake_buffers[81][2];
static char events[2][32];
static unsigned long event_lengths[2];
static unsigned long cancel_counts[2];
static unsigned long free_counts[2];
static void *free_arguments[2][16];

static void check_case(const char *case_name, const char *label, int ok)
{
    ++checks;
    if (!ok) {
        ++failures;
        printf("FAIL %s: %s\n", case_name, label);
    }
}

static void append_event(char value)
{
    if (active_side < 2 && event_lengths[active_side] < sizeof(events[0]))
        events[active_side][event_lengths[active_side]++] = value;
}

static void __cdecl callback_slot_spy(void)
{
}

static int __cdecl cancel_spy(void)
{
    if (active_side < 2)
        ++cancel_counts[active_side];
    append_event('C');
    return 0x5a;
}

static void __cdecl free_spy(void *buffer)
{
    if (active_side < 2) {
        if (free_counts[active_side] < 16)
            free_arguments[active_side][free_counts[active_side]] = buffer;
        ++free_counts[active_side];
    }
    append_event('F');
}

/* Valid local lists are the only inputs to this finite differential. */
void eng1_guard_initial_read(const void *base, unsigned long index,
                             unsigned long stride, unsigned long width,
                             const char *site)
{
    (void)base;
    (void)index;
    (void)stride;
    (void)width;
    (void)site;
    ++guard_calls;
}

typedef struct Snapshot {
    ActorResource rows[81];
    ResourceCallbackContext context;
    char events[32];
    unsigned long event_length;
    unsigned long cancel_count;
    unsigned long free_count;
    void *free_args[16];
} Snapshot;

typedef struct CaseSeed {
    unsigned short state[81];
    unsigned short entry_count[81];
    unsigned char has_buffer[81];
} CaseSeed;

static void configure_rows(ActorResource *rows, const CaseSeed *seed)
{
    unsigned long i;

    memset(rows, 0, 81UL * sizeof(rows[0]));
    for (i = 0; i < 81UL; ++i) {
        rows[i].residency_state = seed->state[i];
        rows[i].entry_count = seed->entry_count[i];
        if (seed->has_buffer[i])
            rows[i].references = (ActorReference *)&fake_buffers[i][0];
    }
}

static void configure_native_interface(HMODULE oracle,
                                       ResourceCallbackContext *context)
{
    EngineInterface *interface_table = (EngineInterface *)
        ((unsigned char *)oracle + NATIVE_INTERFACE_RVA);
    memset(interface_table, 0, sizeof(*interface_table));
    interface_table->context_004 = context;
    interface_table->cancel_load_024 = cancel_spy;
    interface_table->free_allocation_040 = free_spy;
}

static void configure_source_interface(ResourceCallbackContext *context)
{
    memset(&g_engine_interface, 0, sizeof(g_engine_interface));
    g_engine_interface.context_004 = context;
    g_engine_interface.cancel_load_024 = cancel_spy;
    g_engine_interface.free_allocation_040 = free_spy;
    g_resource_context = context;
}

static void reset_observers(void)
{
    memset(events, 0, sizeof(events));
    memset(event_lengths, 0, sizeof(event_lengths));
    memset(cancel_counts, 0, sizeof(cancel_counts));
    memset(free_counts, 0, sizeof(free_counts));
    memset(free_arguments, 0, sizeof(free_arguments));
    guard_calls = 0;
}

static void snapshot_native(Snapshot *out, ActorResource *native_rows)
{
    memcpy(out->rows, native_rows, sizeof(out->rows));
    memcpy(&out->context, &native_context, sizeof(out->context));
    memcpy(out->events, events[0], sizeof(out->events));
    out->event_length = event_lengths[0];
    out->cancel_count = cancel_counts[0];
    out->free_count = free_counts[0];
    memcpy(out->free_args, free_arguments[0], sizeof(out->free_args));
}

static void snapshot_source(Snapshot *out)
{
    memcpy(out->rows, g_actor_resources, sizeof(out->rows));
    memcpy(&out->context, &source_context, sizeof(out->context));
    memcpy(out->events, events[1], sizeof(out->events));
    out->event_length = event_lengths[1];
    out->cancel_count = cancel_counts[1];
    out->free_count = free_counts[1];
    memcpy(out->free_args, free_arguments[1], sizeof(out->free_args));
}

static void compare_snapshots(const char *name, const Snapshot *native,
                              const Snapshot *source)
{
    unsigned long i;
    unsigned long differing_rows;

    differing_rows = 0;
    for (i = 0; i < 81UL; ++i) {
        if (memcmp(&native->rows[i], &source->rows[i],
                   sizeof(native->rows[i])) != 0) {
            ++differing_rows;
            printf("ROWDIFF %s %lu native(state=%u count=%u refs=%p aux=%u path=%p len=%lu) source(state=%u count=%u refs=%p aux=%u path=%p len=%lu)\n",
                   name, i,
                   native->rows[i].residency_state,
                   native->rows[i].entry_count,
                   native->rows[i].references,
                   native->rows[i].unknown_012,
                   native->rows[i].path,
                   native->rows[i].file_length,
                   source->rows[i].residency_state,
                   source->rows[i].entry_count,
                   source->rows[i].references,
                   source->rows[i].unknown_012,
                   source->rows[i].path,
                   source->rows[i].file_length);
        }
    }
    check_case(name, "all 81 resource rows match, including state/count/buffer/auxiliary fields",
               differing_rows == 0);
    check_case(name, "callback context bytes match",
               memcmp(&native->context, &source->context,
                      sizeof(native->context)) == 0);
    check_case(name, "callback event order matches",
               native->event_length == source->event_length &&
               memcmp(native->events, source->events,
                      native->event_length) == 0);
    if (native->event_length != source->event_length ||
        memcmp(native->events, source->events,
               native->event_length < source->event_length ?
               native->event_length : source->event_length) != 0)
        printf("EVENTDIFF %s native=%.*s source=%.*s\n", name,
               (int)native->event_length, native->events,
               (int)source->event_length, source->events);
    check_case(name, "host cancellation counts match",
               native->cancel_count == source->cancel_count);
    check_case(name, "host free counts match",
               native->free_count == source->free_count);
    if (native->cancel_count != source->cancel_count ||
        native->free_count != source->free_count)
        printf("CALLBACKDIFF %s native(cancel=%lu free=%lu) source(cancel=%lu free=%lu)\n",
               name, native->cancel_count, native->free_count,
               source->cancel_count, source->free_count);
    for (i = 0; i < native->free_count && i < 16UL; ++i)
        check_case(name, "host free argument matches native order",
                   native->free_args[i] == source->free_args[i]);
}

static int invoke_native(HMODULE oracle, const unsigned char *first,
                         const unsigned char *second)
{
    void (__cdecl *entry)(const unsigned char *, const unsigned char *);
    entry = (void (__cdecl *)(const unsigned char *, const unsigned char *))
        ((unsigned char *)oracle + NATIVE_ENTRY_RVA);
    __try {
        entry(first, second);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
    return 1;
}

static void run_case(HMODULE oracle, const char *name,
                     const unsigned char *first,
                     const unsigned char *second,
                     const CaseSeed *seed)
{
    ActorResource *native_rows = (ActorResource *)
        ((unsigned char *)oracle + NATIVE_ROW_RVA);
    Snapshot native_snapshot, source_snapshot;
    int native_ok;

    reset_observers();
    memset(&native_context, 0x5a, sizeof(native_context));
    native_context.load_complete = callback_slot_spy;
    native_context.load_failed = callback_slot_spy;
    memset(native_rows, 0, 81UL * sizeof(native_rows[0]));
    configure_rows(native_rows, seed);
    configure_native_interface(oracle, &native_context);
    active_side = 0;
    native_ok = invoke_native(oracle, first, second);
    check_case(name, "unpatched native entry completed with configured host interface",
               native_ok);
    snapshot_native(&native_snapshot, native_rows);

    reset_observers();
    memset(&source_context, 0x5a, sizeof(source_context));
    source_context.load_complete = callback_slot_spy;
    source_context.load_failed = callback_slot_spy;
    configure_rows(g_actor_resources, seed);
    configure_source_interface(&source_context);
    active_side = 1;
    eng1_target_03910(first, second);
    check_case(name, "valid descriptor reads passed through source guard",
               guard_calls != 0 || (first == 0 && second == 0));
    snapshot_source(&source_snapshot);
    compare_snapshots(name, &native_snapshot, &source_snapshot);
}

static void seed_comprehensive(CaseSeed *seed)
{
    memset(seed, 0, sizeof(*seed));
    seed->state[0] = 1;
    seed->state[1] = 2;
    seed->state[2] = 3;
    seed->state[3] = 1;
    seed->state[4] = 3;
    seed->state[5] = 1;
    seed->state[6] = 1;
    seed->state[7] = 3;
    seed->state[8] = 1;
    seed->state[80] = 1;
    seed->entry_count[1] = 4;
    seed->entry_count[2] = 4;
    seed->entry_count[7] = 4;
    seed->has_buffer[7] = 1;
    seed->entry_count[8] = 4;
    seed->has_buffer[8] = 1;
}

static void assert_comprehensive_outputs(void)
{
    check_case("comprehensive", "row zero is canceled but outside cleanup sweep",
               g_actor_resources[0].residency_state == 4);
    check_case("comprehensive", "first-list tail match suppresses row 3",
               g_actor_resources[3].residency_state == 1);
    check_case("comprehensive", "second-list first-byte match suppresses row 4",
               g_actor_resources[4].residency_state == 3);
    check_case("comprehensive", "first-list first-byte match suppresses row 5",
               g_actor_resources[5].residency_state == 1);
    check_case("comprehensive", "second-list tail match suppresses row 6",
               g_actor_resources[6].residency_state == 1);
    check_case("comprehensive", "row 1 and 2 are canceled then actually cleaned",
               g_actor_resources[1].residency_state == 0 &&
               g_actor_resources[2].residency_state == 0);
    check_case("comprehensive", "count-four cleanup destroys row 7",
               g_actor_resources[7].residency_state == 0 &&
               g_actor_resources[7].entry_count == 4 &&
               g_actor_resources[7].references == 0 &&
               g_actor_resources[7].unknown_012 == 0);
    check_case("comprehensive", "count-four cleanup destroys row 8",
               g_actor_resources[8].residency_state == 0 &&
               g_actor_resources[8].entry_count == 4 &&
               g_actor_resources[8].references == 0);
    check_case("comprehensive", "row 80 is matched at a tail byte and remains active",
               g_actor_resources[80].residency_state == 1);
    check_case("comprehensive", "callback order is cancel then two real cleanup frees",
               event_lengths[1] == 3 && memcmp(events[1], "CFF", 3) == 0);
    check_case("comprehensive", "free callbacks receive cleanup rows 7 and 8 in order",
               free_counts[1] == 2 &&
               free_arguments[1][0] == &fake_buffers[7][0] &&
               free_arguments[1][1] == &fake_buffers[8][0]);
    check_case("comprehensive", "state-two cancellation clears both context callbacks",
               source_context.load_complete == 0 && source_context.load_failed == 0);
}

int main(int argc, char **argv)
{
    static const unsigned char first_set[] = { 5U, 3U, 0xffU };
    static const unsigned char second_set[] = { 4U, 6U, 80U, 0xffU };
    static const unsigned char empty_set[] = { 0xffU };
    static const unsigned char nonmatching_tail[] = { 1U, 2U, 0xffU };
    CaseSeed seed;
    HMODULE oracle;

    if (argc != 2) {
        printf("usage: native_diff ENG1.DLL\n");
        return 2;
    }
    oracle = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (oracle == 0) {
        printf("LoadLibraryEx failed: %lu\n", GetLastError());
        return 3;
    }

    seed_comprehensive(&seed);
    run_case(oracle, "comprehensive", first_set, second_set, &seed);
    assert_comprehensive_outputs();

    memset(&seed, 0, sizeof(seed));
    seed.state[0] = 2;
    seed.state[80] = 1;
    seed.entry_count[80] = 4;
    run_case(oracle, "null-and-sentinel", 0, empty_set, &seed);
    check_case("null-and-sentinel", "row zero remains canceled; row 80 cleaned",
               g_actor_resources[0].residency_state == 4 &&
               g_actor_resources[80].residency_state == 0);
    check_case("null-and-sentinel", "state-two host callback still runs once",
               cancel_counts[1] == 1 && event_lengths[1] == 1 &&
               events[1][0] == 'C');

    memset(&seed, 0, sizeof(seed));
    seed.state[3] = 3;
    seed.state[80] = 1;
    seed.entry_count[3] = 4;
    seed.entry_count[80] = 4;
    seed.has_buffer[3] = 1;
    seed.has_buffer[80] = 1;
    run_case(oracle, "nonmatching-tail", nonmatching_tail, empty_set, &seed);
    check_case("nonmatching-tail", "row absent from both sets is canceled",
               g_actor_resources[3].residency_state == 0 &&
               g_actor_resources[80].residency_state == 0);

    memset(&seed, 0, sizeof(seed));
    seed.state[1] = 4;
    seed.entry_count[1] = 7;
    seed.has_buffer[1] = 1;
    seed.state[2] = 7;
    seed.entry_count[2] = 4;
    seed.has_buffer[2] = 1;
    seed.state[3] = 4;
    seed.entry_count[3] = 0;
    seed.has_buffer[3] = 1;
    seed.state[4] = 7;
    seed.entry_count[4] = 0;
    seed.has_buffer[4] = 1;
    run_case(oracle, "state-count-independent-cleanup", empty_set, empty_set,
             &seed);
    check_case("state-count-independent-cleanup",
               "state 4 rows are destroyed while unrelated counts 7 and 0 remain",
               g_actor_resources[1].residency_state == 0 &&
               g_actor_resources[1].entry_count == 7 &&
               g_actor_resources[3].residency_state == 0 &&
               g_actor_resources[3].entry_count == 0);
    check_case("state-count-independent-cleanup",
               "state 7 rows become state 6 while unrelated counts 4 and 0 remain",
               g_actor_resources[2].residency_state == 6 &&
               g_actor_resources[2].entry_count == 4 &&
               g_actor_resources[4].residency_state == 6 &&
               g_actor_resources[4].entry_count == 0);
    check_case("state-count-independent-cleanup",
               "four cleanup frees preserve ascending row order",
               event_lengths[1] == 4 && memcmp(events[1], "FFFF", 4) == 0 &&
               free_counts[1] == 4 &&
               free_arguments[1][0] == &fake_buffers[1][0] &&
               free_arguments[1][1] == &fake_buffers[2][0] &&
               free_arguments[1][2] == &fake_buffers[3][0] &&
               free_arguments[1][3] == &fake_buffers[4][0]);

    printf("eng1_03910_native_diff checks=%lu failures=%lu\n",
           checks, failures);
    FreeLibrary(oracle);
    return failures == 0 ? 0 : 1;
}
