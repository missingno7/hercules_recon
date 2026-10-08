#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "../calibration/host_file_service.c"

ResourceCallbackContext *g_resource_context;
/* Synthetic fixture storage only; this does not claim the shipped capacity. */
char g_resource_filename[512];
const char g_resource_filename_format[] = "%s%s";
const char g_resource_loading_format[] = "Loading %s @ 0x%x,size %d ";
const char g_resource_async_open_failure_format[] = "Unable to open %s (%d)\n";
const char g_resource_sync_open_failure_format[] = "Unable to open %s \n";
const char g_resource_sync_loaded_format[] = "- Loaded \n";
const char g_resource_close_failure_format[] = "- Failed \n";
unsigned long g_resource_prepare_scalar;

#define MAX_CALLS 32
static ResourceCallbackContext contexts[8];
static unsigned char data_a[65536], data_b[65536];
static int checks, failures;
static char events[256];
static int event_count;
static int empty_calls;
static ResourceCallbackContext *empty_redirect;
static int open_results[MAX_CALLS], open_count, open_index;
static ResourceCallbackContext *open_redirects[MAX_CALLS];
static char opened_paths[MAX_CALLS][80];
static int open_modes[MAX_CALLS], open_flags[MAX_CALLS];
static int close_results[MAX_CALLS], close_count, close_index;
static ResourceCallbackContext *close_redirects[MAX_CALLS];
static int closed_handles[MAX_CALLS];
static long seek_results[MAX_CALLS], seek_offsets[MAX_CALLS];
static int seek_origins[MAX_CALLS], seek_handles[MAX_CALLS], seek_count, seek_index;
static ResourceCallbackContext *seek_redirects[MAX_CALLS];
static size_t read_results[MAX_CALLS], read_sizes[MAX_CALLS];
static int read_handles[MAX_CALLS], read_count, read_index;
static void *read_destinations[MAX_CALLS];
static ResourceCallbackContext *read_redirects[MAX_CALLS];
static int diagnostic_count;
static const char *last_diagnostic_format;
static char diagnostic_filename[80];
static int diagnostic_integer;
static void *diagnostic_pointer;
static int loading_diagnostic_integer, loading_diagnostic_calls;
static void *loading_diagnostic_pointer;
static ResourceCallbackContext *loading_redirect, *close_diag_redirect;
static const char *loading_filename_rewrite;
static int failed_callback_calls, failed_callback_argument, failed_status_seen;
static int complete_callback_calls, complete_callback_argument;
static unsigned short complete_mode_seen;
static unsigned long complete_countdown_seen;
static ResourceCallbackContext *callback_redirect;

static void record_event(char value)
{
    if (event_count < (int)sizeof(events) - 1) {
        events[event_count++] = value;
        events[event_count] = '\0';
    }
}

#define expect(label, condition) do { \
    ++checks; \
    if (!(condition)) { ++failures; printf("FAIL: %s\n", label); } \
} while (0)

static void reset_fixture(void)
{
    memset(contexts, 0, sizeof(contexts));
    memset(data_a, 0, sizeof(data_a));
    memset(data_b, 0, sizeof(data_b));
    memset(events, 0, sizeof(events));
    memset(opened_paths, 0, sizeof(opened_paths));
    memset(open_results, 0, sizeof(open_results));
    memset(open_redirects, 0, sizeof(open_redirects));
    memset(open_modes, 0, sizeof(open_modes));
    memset(open_flags, 0, sizeof(open_flags));
    memset(close_results, 0, sizeof(close_results));
    memset(close_redirects, 0, sizeof(close_redirects));
    memset(closed_handles, 0, sizeof(closed_handles));
    memset(seek_results, 0, sizeof(seek_results));
    memset(seek_offsets, 0, sizeof(seek_offsets));
    memset(seek_origins, 0, sizeof(seek_origins));
    memset(seek_handles, 0, sizeof(seek_handles));
    memset(seek_redirects, 0, sizeof(seek_redirects));
    memset(read_results, 0, sizeof(read_results));
    memset(read_sizes, 0, sizeof(read_sizes));
    memset(read_handles, 0, sizeof(read_handles));
    memset(read_destinations, 0, sizeof(read_destinations));
    memset(read_redirects, 0, sizeof(read_redirects));
    memset(diagnostic_filename, 0, sizeof(diagnostic_filename));
    g_resource_context = &contexts[0];
    g_resource_filename[0] = '\0';
    g_resource_prepare_scalar = 0x1234UL;
    event_count = empty_calls = 0;
    open_count = open_index = close_count = close_index = 0;
    seek_count = seek_index = read_count = read_index = 0;
    diagnostic_count = diagnostic_integer = 0;
    diagnostic_pointer = 0;
    loading_diagnostic_integer = loading_diagnostic_calls = 0;
    loading_diagnostic_pointer = 0;
    last_diagnostic_format = 0;
    empty_redirect = loading_redirect = close_diag_redirect = 0;
    loading_filename_rewrite = 0;
    failed_callback_calls = failed_callback_argument = failed_status_seen = 0;
    complete_callback_calls = complete_callback_argument = 0;
    complete_mode_seen = 0;
    complete_countdown_seen = 0;
    callback_redirect = 0;
}

static void queue_open(int result, ResourceCallbackContext *redirect)
{
    open_results[open_count] = result;
    open_redirects[open_count] = redirect;
    ++open_count;
}

static void queue_close(int result, ResourceCallbackContext *redirect)
{
    close_results[close_count] = result;
    close_redirects[close_count] = redirect;
    ++close_count;
}

static void queue_seek(long result, ResourceCallbackContext *redirect)
{
    seek_results[seek_count] = result;
    seek_redirects[seek_count] = redirect;
    ++seek_count;
}

static void queue_read(size_t result, ResourceCallbackContext *redirect)
{
    read_results[read_count] = result;
    read_redirects[read_count] = redirect;
    ++read_count;
}

void __cdecl empty_host_callback(void)
{
    ++empty_calls;
    record_event('E');
    if (empty_redirect != 0) {
        g_resource_context = empty_redirect;
        empty_redirect = 0;
    }
}

void __cdecl host_diagnostic(const char *format, ...)
{
    va_list args;
    char *path;

    ++diagnostic_count;
    last_diagnostic_format = format;
    diagnostic_integer = 0;
    diagnostic_pointer = 0;
    if (format == g_resource_loading_format) {
        record_event('L');
        va_start(args, format);
        path = va_arg(args, char *);
        diagnostic_pointer = va_arg(args, void *);
        diagnostic_integer = va_arg(args, int);
        va_end(args);
        loading_diagnostic_pointer = diagnostic_pointer;
        loading_diagnostic_integer = diagnostic_integer;
        ++loading_diagnostic_calls;
        strncpy(diagnostic_filename, path, sizeof(diagnostic_filename) - 1);
        if (loading_filename_rewrite != 0) {
            strcpy(g_resource_filename, loading_filename_rewrite);
            loading_filename_rewrite = 0;
        }
        if (loading_redirect != 0) {
            g_resource_context = loading_redirect;
            loading_redirect = 0;
        }
    } else if (format == g_resource_sync_open_failure_format) {
        record_event('F');
        va_start(args, format);
        path = va_arg(args, char *);
        va_end(args);
        strncpy(diagnostic_filename, path, sizeof(diagnostic_filename) - 1);
    } else if (format == g_resource_async_open_failure_format) {
        record_event('A');
        va_start(args, format);
        path = va_arg(args, char *);
        diagnostic_integer = va_arg(args, int);
        va_end(args);
        strncpy(diagnostic_filename, path, sizeof(diagnostic_filename) - 1);
    } else if (format == g_resource_sync_loaded_format) {
        record_event('D');
    } else if (format == g_resource_close_failure_format) {
        record_event('X');
        if (close_diag_redirect != 0) {
            g_resource_context = close_diag_redirect;
            close_diag_redirect = 0;
        }
    }
}

int __cdecl virtual_file_open(const char *path, int mode, int flags)
{
    int index, result;
    ResourceCallbackContext *redirect;

    index = open_index++;
    record_event('O');
    strncpy(opened_paths[index], path, sizeof(opened_paths[index]) - 1);
    open_modes[index] = mode;
    open_flags[index] = flags;
    result = index < open_count ? open_results[index] : 1;
    redirect = index < open_count ? open_redirects[index] : 0;
    if (redirect != 0)
        g_resource_context = redirect;
    return result;
}

long __cdecl virtual_file_seek(int handle, long offset, int origin)
{
    int index;
    ResourceCallbackContext *redirect;

    index = seek_index++;
    record_event('S');
    seek_handles[index] = handle;
    seek_offsets[index] = offset;
    seek_origins[index] = origin;
    redirect = index < seek_count ? seek_redirects[index] : 0;
    if (redirect != 0)
        g_resource_context = redirect;
    return index < seek_count ? seek_results[index] : 0L;
}

size_t __cdecl virtual_file_read(int handle, void *destination, size_t bytes)
{
    int index;
    ResourceCallbackContext *redirect;

    index = read_index++;
    record_event('R');
    read_handles[index] = handle;
    read_destinations[index] = destination;
    read_sizes[index] = bytes;
    redirect = index < read_count ? read_redirects[index] : 0;
    if (redirect != 0)
        g_resource_context = redirect;
    return index < read_count ? read_results[index] : 0;
}

int __cdecl virtual_file_close(int handle)
{
    int index, result;
    ResourceCallbackContext *redirect;

    index = close_index++;
    record_event('C');
    closed_handles[index] = handle;
    result = index < close_count ? close_results[index] : 0;
    redirect = index < close_count ? close_redirects[index] : 0;
    if (redirect != 0)
        g_resource_context = redirect;
    return result;
}

static void __cdecl failed_callback(int value)
{
    record_event('B');
    ++failed_callback_calls;
    failed_callback_argument = value;
    failed_status_seen = g_resource_context->transfer_status_0dc;
    if (callback_redirect != 0) {
        g_resource_context = callback_redirect;
        callback_redirect = 0;
    }
}

static void __cdecl complete_callback(int value)
{
    record_event('K');
    ++complete_callback_calls;
    complete_callback_argument = value;
    complete_mode_seen = g_resource_context->mode_09c;
    complete_countdown_seen = g_resource_context->countdown_0bc;
}

static void test_prepare_and_sync(void)
{
    unsigned long result;

    reset_fixture();
    contexts[0].prepare_state_1b00 = 0x123UL;
    contexts[0].mode_09c = 7;
    contexts[1].prepare_state_1b00 = 0x456UL;
    contexts[1].mode_09c = 9;
    empty_redirect = &contexts[1];
    host_file_prepare();
    expect("prepare calls accepted empty callback", empty_calls == 1 && events[0] == 'E');
    expect("prepare clears live extended field and separate scalar", contexts[1].prepare_state_1b00 == 0 && g_resource_prepare_scalar == 0);
    expect("prepare uses post-callback context and preserves mode", contexts[0].prepare_state_1b00 == 0x123UL && contexts[1].mode_09c == 9);

    reset_fixture();
    queue_open(11, &contexts[1]);
    queue_open(22, &contexts[4]);
    queue_close(1, 0);
    queue_close(0, &contexts[5]);
    contexts[2].virtual_handle_0b8 = 33;
    contexts[3].virtual_handle_0b8 = 44;
    loading_redirect = &contexts[2];
    loading_filename_rewrite = "REOPENED-NAME";
    queue_read(0, &contexts[3]);
    queue_read(1, 0);
    result = host_file_sync("ROOT", "/FILE", data_a, 3000UL);
    expect("sync returns original requested byte count after retry succeeds", result == 3000UL);
    expect("sync preserves format concatenation and retry uses live filename", strcmp(opened_paths[0], "ROOT/FILE") == 0 && strcmp(opened_paths[1], "REOPENED-NAME") == 0);
    expect("sync opens read-only through real adapter API", open_index == 2 && open_modes[0] == 0 && open_flags[0] == 0 && open_modes[1] == 0 && open_flags[1] == 0);
    expect("sync reloads live handle after diagnostic and read", read_index == 2 && read_handles[0] == 33 && read_handles[1] == 22 && closed_handles[0] == 44 && closed_handles[1] == 22);
    expect("sync captures destination and request despite short read", read_destinations[0] == data_a && read_destinations[1] == data_a && read_sizes[0] == 3000 && read_sizes[1] == 3000);
    expect("sync records captured loading arguments before log mutation", loading_diagnostic_calls == 2 && loading_diagnostic_pointer == data_a && loading_diagnostic_integer == 3000);
    expect("sync success status is written to post-close context before loaded log", contexts[5].transfer_status_0dc == 0 && last_diagnostic_format == g_resource_sync_loaded_format);
    expect("sync failure-close retry and completion order", strcmp(events, "EOLRCOLRCD") == 0);

    reset_fixture();
    queue_open(-1, 0);
    result = host_file_sync("BAD", ".DAT", data_a, 8);
    expect("sync initial open failure returns zero", result == 0);
    expect("sync failure logs filename and skips read/close", open_index == 1 && read_index == 0 && close_index == 0 && last_diagnostic_format == g_resource_sync_open_failure_format && strcmp(diagnostic_filename, "BAD.DAT") == 0);
}

static void test_async_and_length(void)
{
    static const unsigned long requests[] = {
        0x00000000UL, 0x00000001UL, 0x000007ffUL, 0x00000800UL,
        0x00000801UL, 0x07fff800UL, 0x07fff801UL, 0x7ffff7ffUL,
        0x7ffff800UL, 0x7ffff801UL, 0x80000000UL, 0xfffff800UL,
        0xfffff801UL, 0xffffffffUL
    };
    static const short expected[] = {0,1,1,1,2,-1,0,-1,-1,0,1,0,0,0};
    int i, result;

    reset_fixture();
    contexts[0].mode_09c = 1;
    result = host_file_async("A", "B", 1, data_a, 2048);
    expect("async busy guard returns one without prepare or I/O", result == 1 && empty_calls == 0 && open_index == 0 && seek_index == 0 && event_count == 0);

    reset_fixture();
    contexts[0].mode_09c = 3;
    contexts[0].destination_cursor_a4 = data_b + 8;
    contexts[0].total_sectors_0a0 = 19;
    contexts[0].remaining_sectors_09e = -5;
    contexts[0].countdown_0bc = 27;
    contexts[0].transfer_status_0dc = 0x123;
    queue_open(-1, 0);
    result = host_file_async("BAD", ".BIN", 99, data_a, 0x1234UL);
    expect("async open error returns three and keeps service state", result == 3 && contexts[0].mode_09c == 3 && contexts[0].destination_cursor_a4 == data_b + 8 && contexts[0].total_sectors_0a0 == 19 && contexts[0].remaining_sectors_09e == -5 && contexts[0].countdown_0bc == 27 && contexts[0].transfer_status_0dc == 0x123);
    expect("async open error records handle and formats byte request", contexts[0].virtual_handle_0b8 == -1 && seek_index == 0 && last_diagnostic_format == g_resource_async_open_failure_format && diagnostic_integer == 0x1234);

    for (i = 0; i < (int)(sizeof(requests) / sizeof(requests[0])); ++i) {
        reset_fixture();
        queue_open(9, 0);
        result = host_file_async("P", "S", 0, data_a, requests[i]);
        expect("async signed wrapped rounding and low-word store", result == 2 && contexts[0].mode_09c == 1 && contexts[0].destination_cursor_a4 == data_a && contexts[0].total_sectors_0a0 == expected[i] && contexts[0].remaining_sectors_09e == expected[i] && contexts[0].countdown_0bc == 0 && seek_index == 0);
    }

    reset_fixture();
    queue_open(7, &contexts[1]);
    contexts[1].mode_09c = 5;
    contexts[2].mode_09c = 8;
    queue_seek(-4, &contexts[2]);
    result = host_file_async("P", "S", -1, data_a, 1);
    expect("async nonzero negative sector offset shifts as wrapped 32-bit value", seek_index == 1 && seek_handles[0] == 7 && seek_offsets[0] == -2048L && seek_origins[0] == 0);
    expect("async reloads context after seek before cursor and state stores", result == 2 && contexts[1].mode_09c == 5 && contexts[1].destination_cursor_a4 == 0 && contexts[2].destination_cursor_a4 == data_a && contexts[2].mode_09c == 1 && contexts[2].total_sectors_0a0 == 1 && contexts[2].remaining_sectors_09e == 1);

    reset_fixture();
    queue_open(7, &contexts[1]);
    queue_open(8, &contexts[4]);
    contexts[2].virtual_handle_0b8 = 41;
    contexts[3].virtual_handle_0b8 = 51;
    queue_seek(123L, &contexts[2]);
    queue_seek(456L, 0);
    queue_close(1, &contexts[3]);
    queue_close(0, &contexts[5]);
    result = (int)resource_file_length("DIR", "FILE");
    expect("length returns final seek length after close failure/reopen", result == 456 && open_index == 2 && seek_index == 2 && close_index == 2);
    expect("length reloads handle after seek and retries with live reopened handle", seek_handles[0] == 7 && closed_handles[0] == 41 && opened_paths[1][0] != '\0' && seek_handles[1] == 8 && closed_handles[1] == 8);
    expect("length does not prepare or emit diagnostics", empty_calls == 0 && diagnostic_count == 0);
}

static void test_cancel_and_progress(void)
{
    reset_fixture();
    contexts[0].mode_09c = 1;
    contexts[0].virtual_handle_0b8 = 44;
    contexts[0].unknown_0ac = 7;
    queue_close(4, 0);
    host_cancel_load();
    expect("cancel close failure preserves mode and opaque field", contexts[0].mode_09c == 1 && contexts[0].unknown_0ac == 7 && last_diagnostic_format == g_resource_close_failure_format);

    reset_fixture();
    contexts[0].mode_09c = 1;
    contexts[0].virtual_handle_0b8 = 44;
    contexts[0].unknown_0ac = 7;
    contexts[1].mode_09c = 8;
    contexts[1].unknown_0ac = 9;
    queue_close(0, &contexts[1]);
    host_cancel_load();
    expect("cancel success reloads live context after close", contexts[0].mode_09c == 1 && contexts[0].unknown_0ac == 7 && contexts[1].mode_09c == 0 && contexts[1].unknown_0ac == 0);

    reset_fixture();
    contexts[0].mode_09c = 4;
    contexts[0].countdown_0bc = 0x80000000UL;
    host_file_progress();
    expect("progress countdown_0bc comparison/decrement is unsigned", contexts[0].countdown_0bc == 0x7fffffffUL && contexts[0].mode_09c == 4 && read_index == 0);

    reset_fixture();
    contexts[0].mode_09c = 1;
    contexts[0].countdown_0bc = 1;
    contexts[0].remaining_sectors_09e = 6;
    host_file_progress();
    expect("countdown_0bc reaching zero clears mode before transfer branch", contexts[0].countdown_0bc == 0 && contexts[0].mode_09c == 0 && contexts[0].remaining_sectors_09e == 6 && read_index == 0);

    reset_fixture();
    contexts[0].mode_09c = 1;
    contexts[0].transfer_result_0d8 = -1;
    contexts[0].load_failed = failed_callback;
    contexts[1].mode_09c = 9;
    contexts[1].transfer_result_0d8 = 77;
    callback_redirect = &contexts[1];
    host_file_progress();
    expect("failure callback receives zero after status store", failed_callback_calls == 1 && failed_callback_argument == 0 && failed_status_seen == 0x100 && contexts[0].transfer_status_0dc == 0x100);
    expect("failure cleanup applies to context live after callback", contexts[0].mode_09c == 1 && contexts[0].transfer_result_0d8 == -1 && contexts[1].mode_09c == 0 && contexts[1].transfer_result_0d8 == 0);

    reset_fixture();
    contexts[0].mode_09c = 1;
    contexts[0].virtual_handle_0b8 = 55;
    contexts[0].destination_cursor_a4 = data_a;
    contexts[0].remaining_sectors_09e = 6;
    contexts[0].suppress_auto_close_0e0 = 1;
    contexts[0].countdown_0bc = 7;
    contexts[0].load_complete = complete_callback;
    queue_read(0, 0);
    host_file_progress();
    expect("progress chooses five sectors and ignores short read result", read_index == 1 && read_handles[0] == 55 && read_destinations[0] == data_a && read_sizes[0] == 10240 && contexts[0].remaining_sectors_09e == 1 && contexts[0].destination_cursor_a4 == data_a + 10240 && complete_callback_calls == 0);
    queue_read(0, 0);
    host_file_progress();
    expect("final one-sector request advances by requested bytes and calls completion with zero", read_index == 2 && read_sizes[1] == 2048 && contexts[0].destination_cursor_a4 == data_a + 12288 && complete_callback_calls == 1 && complete_callback_argument == 0);
    expect("suppress-auto-close preserves live mode while both progress calls decrement countdown_0bc", contexts[0].mode_09c == 1 && contexts[0].countdown_0bc == 5 && close_index == 0 && contexts[0].transfer_status_0dc == 0);

    reset_fixture();
    contexts[0].mode_09c = 1;
    contexts[0].virtual_handle_0b8 = 33;
    contexts[0].destination_cursor_a4 = data_a + 8192;
    contexts[0].remaining_sectors_09e = -2;
    contexts[0].suppress_auto_close_0e0 = 1;
    contexts[0].load_complete = complete_callback;
    queue_read(0, 0);
    host_file_progress();
    expect("negative physical sector count is not clamped", read_index == 1 && read_handles[0] == 33 && read_sizes[0] == 0xfffff000UL && contexts[0].remaining_sectors_09e == 0);
    expect("negative read still advances live cursor by requested signed-byte delta", contexts[0].destination_cursor_a4 == data_a + 4096 && complete_callback_calls == 1 && contexts[0].transfer_status_0dc == 0);

    reset_fixture();
    contexts[0].mode_09c = 1;
    contexts[0].virtual_handle_0b8 = 61;
    contexts[0].remaining_sectors_09e = 0;
    contexts[0].suppress_auto_close_0e0 = 0;
    contexts[0].unknown_0ac = 3;
    contexts[1].unknown_0ac = 9;
    contexts[1].load_complete = complete_callback;
    queue_close(1, 0);
    close_diag_redirect = &contexts[1];
    host_file_progress();
    expect("completion stores status before attempted auto-close", contexts[0].transfer_status_0dc == 0 && closed_handles[0] == 61 && contexts[0].unknown_0ac == 3);
    expect("close failure does not block mode/countdown_0bc then completion callback", contexts[1].mode_09c == 4 && contexts[1].countdown_0bc == 12 && contexts[1].unknown_0ac == 9 && complete_callback_calls == 1 && complete_callback_argument == 0 && complete_mode_seen == 4 && complete_countdown_seen == 12);

    reset_fixture();
    contexts[0].mode_09c = 1;
    contexts[0].remaining_sectors_09e = 0;
    contexts[0].suppress_auto_close_0e0 = 1;
    contexts[0].countdown_0bc = 5;
    contexts[0].load_complete = complete_callback;
    host_file_progress();
    host_file_progress();
    expect("suppressed completion can repeat without cleanup", complete_callback_calls == 2 && contexts[0].mode_09c == 1 && contexts[0].countdown_0bc == 3 && close_index == 0);
}

int main(void)
{
    test_prepare_and_sync();
    test_async_and_length();
    test_cancel_and_progress();
    printf("checks=%d failures=%d\n", checks, failures);
    return failures != 0;
}
