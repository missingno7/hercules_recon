#include "../calibration/resource_provider.c"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

ActorResource g_actor_resources[81];
ActorResource *g_active_resource_record;
int g_loading_resource;
unsigned long g_reported_total_bytes;
unsigned long g_resource_start_tick;
ResourceCallbackContext *g_resource_context;
const char g_resource_suffix[] = "\\ANIMPSX.BIN";
const char g_provider_bad_id_format[] = "Illegal Dbase %d \n";

static ResourceCallbackContext context_primary;
static ResourceCallbackContext context_secondary;
static ActorReference allocation_rows[16];
static char path_text[8][24];
static ActorReference **allocation_slots[4];
static unsigned long allocation_sizes[4];
static unsigned long allocation_flags[4];
static int allocation_answers[4];
static int allocation_calls;
static int allocation_redirect_active;
static ActorResource *allocation_active_target;
static int allocation_redirect_context;
static ResourceCallbackContext *allocation_context_target;
static int mutate_primary_tick;
static unsigned long primary_tick_after_mutation;
static int length_calls;
static unsigned long length_answer;
static int length_redirect_active;
static ActorResource *length_active_target;
static const char *length_path_seen;
static const char *length_suffix_seen;
static int reclaim_calls;
static int reclaim_reason;
static int async_calls;
static int async_answer;
static const char *async_path_seen;
static const char *async_suffix_seen;
static int async_offset_seen;
static void *async_destination_seen;
static unsigned long async_bytes_seen;
static int sync_calls;
static unsigned long sync_answer;
static const char *sync_path_seen;
static const char *sync_suffix_seen;
static void *sync_destination_seen;
static unsigned long sync_bytes_seen;
static int complete_calls;
static int complete_id_seen;
static int cancel_calls;
static int cancel_id_seen;
static int diagnostic_calls;
static const char *diagnostic_format_seen;
static int diagnostic_id_seen;
static int checks;
static int failures;

static void expect(int condition)
{
    ++checks;
    if (!condition)
        ++failures;
}

static void __cdecl marker_callback(void)
{
}

static void reset_fixture(void)
{
    memset(g_actor_resources, 0, sizeof(g_actor_resources));
    memset(&context_primary, 0, sizeof(context_primary));
    memset(&context_secondary, 0, sizeof(context_secondary));
    memset(allocation_rows, 0x5a, sizeof(allocation_rows));
    memset(allocation_slots, 0, sizeof(allocation_slots));
    memset(allocation_sizes, 0, sizeof(allocation_sizes));
    memset(allocation_flags, 0, sizeof(allocation_flags));
    memset(allocation_answers, 0, sizeof(allocation_answers));
    allocation_calls = 0;
    allocation_redirect_active = 0;
    allocation_active_target = 0;
    allocation_redirect_context = 0;
    allocation_context_target = 0;
    mutate_primary_tick = 0;
    primary_tick_after_mutation = 0;
    length_calls = 0;
    length_answer = 0;
    length_redirect_active = 0;
    length_active_target = 0;
    length_path_seen = 0;
    length_suffix_seen = 0;
    reclaim_calls = 0;
    reclaim_reason = -1;
    async_calls = 0;
    async_answer = 0;
    async_path_seen = 0;
    async_suffix_seen = 0;
    async_offset_seen = -1;
    async_destination_seen = 0;
    async_bytes_seen = 0;
    sync_calls = 0;
    sync_answer = 0;
    sync_path_seen = 0;
    sync_suffix_seen = 0;
    sync_destination_seen = 0;
    sync_bytes_seen = 0;
    complete_calls = 0;
    complete_id_seen = -1;
    cancel_calls = 0;
    cancel_id_seen = -1;
    diagnostic_calls = 0;
    diagnostic_format_seen = 0;
    diagnostic_id_seen = -1;
    g_active_resource_record = 0;
    g_loading_resource = -1;
    g_reported_total_bytes = 0;
    g_resource_start_tick = 0;
    g_resource_context = &context_primary;
    context_primary.tick_038 = 0x13572468UL;
    context_primary.load_complete = marker_callback;
    context_primary.load_failed = marker_callback;
    context_secondary.load_complete = marker_callback;
    context_secondary.load_failed = marker_callback;
}

static void set_resource(int id, unsigned short count, unsigned long length,
                         unsigned short state, unsigned short auxiliary,
                         ActorReference *references)
{
    sprintf(path_text[id % 8], "resource-%d", id);
    g_actor_resources[id].path = path_text[id % 8];
    g_actor_resources[id].entry_count = count;
    g_actor_resources[id].unknown_006 = 0x6161U;
    g_actor_resources[id].file_length = length;
    g_actor_resources[id].references = references;
    g_actor_resources[id].residency_state = state;
    g_actor_resources[id].unknown_012 = auxiliary;
}

void __cdecl engine_diagnostic(const char *format, ...)
{
    va_list args;
    ++diagnostic_calls;
    diagnostic_format_seen = format;
    va_start(args, format);
    diagnostic_id_seen = va_arg(args, int);
    va_end(args);
}

unsigned long __cdecl resource_file_length(const char *path, const char *suffix)
{
    ++length_calls;
    length_path_seen = path;
    length_suffix_seen = suffix;
    if (length_redirect_active)
        g_active_resource_record = length_active_target;
    return length_answer;
}

ActorReference *__cdecl resource_allocate(
    ActorReference **output_slot, unsigned long bytes, unsigned long flags)
{
    int call = allocation_calls++;
    if (call < 4) {
        allocation_slots[call] = output_slot;
        allocation_sizes[call] = bytes;
        allocation_flags[call] = flags;
    }
    if (allocation_redirect_active)
        g_active_resource_record = allocation_active_target;
    if (allocation_redirect_context)
        g_resource_context = allocation_context_target;
    if (mutate_primary_tick)
        context_primary.tick_038 = primary_tick_after_mutation;
    if (call < 4 && allocation_answers[call]) {
        *output_slot = &allocation_rows[0];
        return &allocation_rows[0];
    }
    return 0;
}

void __cdecl resource_reclaim(int reason)
{
    ++reclaim_calls;
    reclaim_reason = reason;
}

int __cdecl resource_load_async(const char *path, const char *suffix,
                                int sector_offset, void *destination,
                                unsigned long bytes)
{
    ++async_calls;
    async_path_seen = path;
    async_suffix_seen = suffix;
    async_offset_seen = sector_offset;
    async_destination_seen = destination;
    async_bytes_seen = bytes;
    return async_answer;
}

unsigned long __cdecl resource_load_sync(const char *path, const char *suffix,
                                         void *destination, unsigned long bytes)
{
    ++sync_calls;
    sync_path_seen = path;
    sync_suffix_seen = suffix;
    sync_destination_seen = destination;
    sync_bytes_seen = bytes;
    return sync_answer;
}

void __cdecl resource_cancel(int resource_id)
{
    ++cancel_calls;
    cancel_id_seen = resource_id;
}

void __cdecl resource_load_complete(void)
{
    ++complete_calls;
    complete_id_seen = g_loading_resource;
}

void __cdecl resource_load_failed(void)
{
}

static void test_cached_sync_success(void)
{
    reset_fixture();
    set_resource(2, 2U, 4090UL, 0U, 0xa202U, 0);
    allocation_answers[0] = 1;
    sync_answer = 4090UL;
    allocation_rows[3][0] = 0x7373;
    allocation_rows[3][1] = 0x7474;

    expect(resource_provider(2, 0) == 0);
    expect(length_calls == 0);
    expect(allocation_calls == 1);
    expect(allocation_slots[0] == &g_actor_resources[2].references);
    expect(allocation_sizes[0] == 0x2000UL);
    expect(allocation_flags[0] == 0x20UL);
    expect(g_actor_resources[2].references == &allocation_rows[0]);
    expect(allocation_rows[0][0] == 0 && allocation_rows[0][1] == 0);
    expect(allocation_rows[1][0] == 0 && allocation_rows[1][1] == 0);
    expect(allocation_rows[2][0] == 0 && allocation_rows[2][1] == 0);
    expect(allocation_rows[3][0] == 0x7373 && allocation_rows[3][1] == 0x7474);
    expect(g_actor_resources[2].residency_state == 2U);
    expect(g_actor_resources[2].unknown_012 == 1U);
    expect(g_loading_resource == 2);
    expect(g_reported_total_bytes == 4102UL);
    expect(g_resource_start_tick == context_primary.tick_038);
    expect(sync_calls == 1 && sync_path_seen == g_actor_resources[2].path);
    expect(sync_suffix_seen == g_resource_suffix);
    expect(sync_destination_seen == (unsigned char *)&allocation_rows[0] + 12);
    expect(sync_bytes_seen == 4090UL);
    expect(complete_calls == 1 && complete_id_seen == 2);
    expect(cancel_calls == 0 && async_calls == 0);
}

static void test_uncached_async_live_rereads(void)
{
    reset_fixture();
    set_resource(3, 1U, 0UL, 0U, 0x3030U, 0);
    set_resource(4, 4U, 99UL, 0U, 0x4040U, 0);
    set_resource(5, 1U, 88UL, 0U, 0x5050U, &allocation_rows[8]);
    context_primary.tick_038 = 0x11112222UL;
    context_secondary.tick_038 = 0x33334444UL;
    length_answer = 0x800UL;
    length_redirect_active = 1;
    length_active_target = &g_actor_resources[4];
    allocation_answers[0] = 1;
    allocation_redirect_active = 1;
    allocation_active_target = &g_actor_resources[5];
    allocation_redirect_context = 1;
    allocation_context_target = &context_secondary;
    mutate_primary_tick = 1;
    primary_tick_after_mutation = 0x55667788UL;
    async_answer = 3;
    allocation_rows[2][0] = 0x1212;
    allocation_rows[2][1] = 0x3434;

    expect(resource_provider(3, 1) == -1);
    expect(length_calls == 1);
    expect(length_path_seen == g_actor_resources[3].path);
    expect(length_suffix_seen == g_resource_suffix);
    expect(g_actor_resources[3].file_length == 0UL);
    expect(g_actor_resources[4].file_length == 0x800UL);
    expect(allocation_calls == 1);
    expect(allocation_slots[0] == &g_actor_resources[3].references);
    expect(allocation_sizes[0] == 0x1800UL);
    expect(allocation_flags[0] == 0x20UL);
    expect(g_actor_resources[3].references == &allocation_rows[0]);
    expect(g_actor_resources[3].residency_state == 2U);
    expect(g_actor_resources[3].unknown_012 == 1U);
    expect(g_reported_total_bytes == 2056UL);
    expect(g_loading_resource == 3);
    expect(g_resource_start_tick == context_secondary.tick_038);
    expect(context_primary.load_complete == marker_callback);
    expect(context_primary.load_failed == marker_callback);
    expect(context_secondary.load_complete == resource_load_complete);
    expect(context_secondary.load_failed == resource_load_failed);
    expect(allocation_rows[0][0] == 0 && allocation_rows[0][1] == 0);
    expect(allocation_rows[1][0] == 0 && allocation_rows[1][1] == 0);
    expect(allocation_rows[2][0] == 0x1212 && allocation_rows[2][1] == 0x3434);
    expect(async_calls == 1 && async_path_seen == g_actor_resources[5].path);
    expect(async_suffix_seen == g_resource_suffix && async_offset_seen == 0);
    expect(async_destination_seen == (unsigned char *)g_actor_resources[5].references + 8);
    expect(async_bytes_seen == 0x800UL);
    expect(complete_calls == 0 && cancel_calls == 0 && sync_calls == 0);
}

static void test_retry_and_sync_failure(void)
{
    reset_fixture();
    set_resource(6, 1U, 100UL, 0U, 0x6060U, 0);
    allocation_answers[0] = 0;
    allocation_answers[1] = 1;
    sync_answer = 0;

    expect(resource_provider(6, 0) == 0);
    expect(allocation_calls == 2 && reclaim_calls == 1 && reclaim_reason == 0);
    expect(allocation_slots[0] == &g_actor_resources[6].references);
    expect(allocation_slots[1] == allocation_slots[0]);
    expect(allocation_sizes[0] == 0x1000UL && allocation_sizes[1] == 0x1000UL);
    expect(allocation_flags[0] == 0x20UL && allocation_flags[1] == 0x20UL);
    expect(sync_calls == 1 && sync_path_seen == g_actor_resources[6].path);
    expect(sync_destination_seen == (unsigned char *)&allocation_rows[0] + 8);
    expect(sync_bytes_seen == 100UL);
    expect(cancel_calls == 1 && cancel_id_seen == 6);
    expect(complete_calls == 0);
}

static void test_allocation_failures(void)
{
    reset_fixture();
    set_resource(7, 0U, 100UL, 7U, 0x7171U, 0);
    g_loading_resource = 71;
    g_reported_total_bytes = 0xababababUL;
    g_resource_start_tick = 0xcdcdcdcdUL;

    expect(resource_provider(7, 0) == 1);
    expect(allocation_calls == 2 && reclaim_calls == 1);
    expect(g_actor_resources[7].residency_state == 0U);
    expect(g_actor_resources[7].unknown_012 == 0x7171U);
    expect(g_loading_resource == 71);
    expect(g_reported_total_bytes == 0xababababUL);
    expect(g_resource_start_tick == 0xcdcdcdcdUL);
    expect(sync_calls == 0 && async_calls == 0);

    reset_fixture();
    set_resource(8, 0U, 100UL, 4U, 0x8181U, 0);
    expect(resource_provider(8, 1) == 1);
    expect(allocation_calls == 2 && reclaim_calls == 1);
    expect(g_actor_resources[8].residency_state == 4U);
    expect(g_actor_resources[8].unknown_012 == 0x8181U);
    expect(g_loading_resource == -1);
    expect(sync_calls == 0 && async_calls == 0);
}

static void test_early_guards(void)
{
    reset_fixture();
    expect(resource_provider(81, 0) == 1);
    expect(diagnostic_calls == 1);
    expect(diagnostic_format_seen == g_provider_bad_id_format);
    expect(diagnostic_id_seen == 81);
    expect(length_calls == 0 && allocation_calls == 0);

    reset_fixture();
    g_active_resource_record = &g_actor_resources[10];
    context_primary.mode_09c = 1U;
    expect(resource_provider(10, 0) == 0);
    expect(g_active_resource_record == &g_actor_resources[10]);
    expect(allocation_calls == 0 && length_calls == 0);

    reset_fixture();
    g_active_resource_record = &g_actor_resources[11];
    context_primary.blocked_0df = 1U;
    expect(resource_provider(11, 0) == 0);
    expect(g_active_resource_record == &g_actor_resources[11]);

    reset_fixture();
    set_resource(12, 0U, 1UL, 3U, 0x1212U, 0);
    g_active_resource_record = &g_actor_resources[13];
    expect(resource_provider(12, 0) == 0);
    expect(g_active_resource_record == &g_actor_resources[13]);
    expect(allocation_calls == 0 && length_calls == 0);
}

int main(void)
{
    test_cached_sync_success();
    test_uncached_async_live_rereads();
    test_retry_and_sync_failure();
    test_allocation_failures();
    test_early_guards();
    printf("checks=%d failures=%d\n", checks, failures);
    return failures != 0;
}
