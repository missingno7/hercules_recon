#include <stdio.h>
#include <string.h>
#include "../calibration/resource_context.h"
#include "../calibration/host_callback_protocol_externs.h"

extern int __cdecl host_frame_close_state(void);
extern int __cdecl host_frame_notify_zero(void);
extern EngineNoArgCallback __cdecl host_set_callback_zero(EngineNoArgCallback callback);
extern EngineNoArgCallback __cdecl host_set_callback_one(EngineNoArgCallback callback);
extern unsigned long __cdecl host_disable_callback_gate(void);

ResourceCallbackContext *g_resource_context;
int g_public_file_count;
int g_public_file_handles[16];
const char g_open_capacity_error[] = "capacity";
const char g_open_mode_error[] = "mode";
const char g_seek_handle_error[] = "seek";
const char g_read_handle_error[] = "read";
const char g_archive_read_mode[] = "rb";
const char g_resource_close_failure_format[] = "- Failed \n";
const char g_resource_close_success_format[] = "- Closed \n";
HostCallbackStoragePrefix g_host_callback_prefix;

static int archive_close_result;
static int archive_close_calls;
static int archive_last_handle;
static int redirect_context_on_close;
static ResourceCallbackContext *redirect_context;
static int diagnostic_calls;
static const char *diagnostic_format;
static int diagnostic_archive_close_calls;
static ResourceCallbackContext *diagnostic_context;
static unsigned long diagnostic_ac;
static unsigned short diagnostic_mode;
static int callback_zero_a_count;
static int callback_zero_b_count;
static int callback_one_count;
static int callback_zero_a_replaces_itself;
static int checks;

static int expect(int condition, const char *description)
{
    ++checks;
    if (!condition) {
        printf("FAIL: %s\n", description);
        return 0;
    }
    return 1;
}

static void reset_observers(void)
{
    archive_close_result = 0;
    archive_close_calls = 0;
    archive_last_handle = -1;
    redirect_context_on_close = 0;
    redirect_context = 0;
    diagnostic_calls = 0;
    diagnostic_format = 0;
    diagnostic_archive_close_calls = -1;
    diagnostic_context = 0;
    diagnostic_ac = 0xffffffffUL;
    diagnostic_mode = 0xffff;
}

int __cdecl archive_close(int handle)
{
    ++archive_close_calls;
    archive_last_handle = handle;
    if (redirect_context_on_close) {
        redirect_context_on_close = 0;
        g_resource_context = redirect_context;
    }
    return archive_close_result;
}

void __cdecl archive_fatal(const char *message)
{
    printf("unexpected archive_fatal: %s\n", message);
}

int __cdecl archive_open(const char *path, const char *mode)
{
    (void)path;
    (void)mode;
    return -1;
}

long __cdecl archive_position(int handle)
{
    (void)handle;
    return 0;
}

int __cdecl archive_seek(int handle, long offset, int origin)
{
    (void)handle;
    (void)offset;
    (void)origin;
    return 0;
}

size_t __cdecl archive_read(void *destination, size_t size, size_t count, int handle)
{
    (void)destination;
    (void)size;
    (void)count;
    (void)handle;
    return 0;
}

void __cdecl host_diagnostic(const char *format, ...)
{
    ++diagnostic_calls;
    diagnostic_format = format;
    diagnostic_archive_close_calls = archive_close_calls;
    diagnostic_context = g_resource_context;
    if (g_resource_context != 0) {
        diagnostic_ac = g_resource_context->unknown_0ac;
        diagnostic_mode = g_resource_context->mode_09c;
    }
}

static void __cdecl callback_zero_b(void)
{
    ++callback_zero_b_count;
}

static void __cdecl callback_zero_a(void)
{
    ++callback_zero_a_count;
    if (callback_zero_a_replaces_itself) {
        callback_zero_a_replaces_itself = 0;
        host_set_callback_zero(callback_zero_b);
    }
}

static void __cdecl callback_one(void)
{
    ++callback_one_count;
}

#define EXPECT(test, description) do { if (!expect((test), (description))) return 1; } while (0)

int main(void)
{
    ResourceCallbackContext failure_context;
    ResourceCallbackContext entry_context;
    ResourceCallbackContext replacement_context;
    int result;

    memset(&failure_context, 0, sizeof(failure_context));
    memset(&entry_context, 0, sizeof(entry_context));
    memset(&replacement_context, 0, sizeof(replacement_context));
    memset(g_public_file_handles, 0, sizeof(g_public_file_handles));
    memset(&g_host_callback_prefix, 0, sizeof(g_host_callback_prefix));

    reset_observers();
    failure_context.virtual_handle_0b8 = 1;
    failure_context.unknown_0ac = 0x11223344UL;
    failure_context.mode_09c = 3;
    g_resource_context = &failure_context;
    g_public_file_count = 1;
    g_public_file_handles[0] = 41;
    archive_close_result = -7;
    result = host_frame_close_state();
    EXPECT(result == 0, "nonzero close result maps to return 0");
    EXPECT(archive_close_calls == 1 && archive_last_handle == 41,
           "real virtual close forwards the stored public handle to archive_close");
    EXPECT(g_public_file_count == 0 && g_public_file_handles[0] == 0,
           "real virtual close updates its actual public-handle table even on archive error");
    EXPECT(failure_context.unknown_0ac == 0x11223344UL && failure_context.mode_09c == 3,
           "close failure leaves both context fields untouched");
    EXPECT(diagnostic_calls == 1 && diagnostic_format == g_resource_close_failure_format,
           "close failure reports the observed failure string once");
    EXPECT(diagnostic_archive_close_calls == 1 && diagnostic_context == &failure_context &&
           diagnostic_ac == 0x11223344UL && diagnostic_mode == 3,
           "failure diagnostic occurs after close and before any context clear");

    reset_observers();
    entry_context.virtual_handle_0b8 = 1;
    entry_context.unknown_0ac = 0x11111111UL;
    entry_context.mode_09c = 5;
    replacement_context.unknown_0ac = 0x22222222UL;
    replacement_context.mode_09c = 6;
    g_resource_context = &entry_context;
    g_public_file_count = 1;
    g_public_file_handles[0] = 73;
    redirect_context = &replacement_context;
    redirect_context_on_close = 1;
    result = host_frame_close_state();
    EXPECT(result == 1, "zero close result maps to return 1");
    EXPECT(archive_close_calls == 1 && archive_last_handle == 73,
           "successful close calls the archive boundary once with stored handle");
    EXPECT(g_resource_context == &replacement_context,
           "fixture archive boundary replaced the live context during close");
    EXPECT(entry_context.unknown_0ac == 0x11111111UL && entry_context.mode_09c == 5,
           "post-close stores do not use a cached entry context");
    EXPECT(replacement_context.unknown_0ac == 0 && replacement_context.mode_09c == 0,
           "success clears DWORD +0xac before WORD +0x9c in the current context");
    EXPECT(diagnostic_calls == 1 && diagnostic_format == g_resource_close_success_format,
           "success reports the observed closed string once");
    EXPECT(diagnostic_archive_close_calls == 1 && diagnostic_context == &replacement_context &&
           diagnostic_ac == 0 && diagnostic_mode == 0,
           "success diagnostic follows both ordered context clears");
    EXPECT(g_public_file_count == 0 && g_public_file_handles[0] == 0,
           "real virtual close releases its public slot on success");

    reset_observers();
    failure_context.virtual_handle_0b8 = 2;
    failure_context.unknown_0ac = 0xabcdef01UL;
    failure_context.mode_09c = 9;
    g_resource_context = &failure_context;
    g_public_file_count = 1;
    g_public_file_handles[0] = 41;
    result = host_frame_close_state();
    EXPECT(result == 0 && archive_close_calls == 0,
           "invalid virtual handle returns through the close-error path without archive call");
    EXPECT(failure_context.unknown_0ac == 0xabcdef01UL && failure_context.mode_09c == 9 &&
           diagnostic_format == g_resource_close_failure_format,
           "invalid-handle error also preserves state and reports failure");

    memset(&g_host_callback_prefix, 0, sizeof(g_host_callback_prefix));
    callback_zero_a_count = 0;
    callback_zero_b_count = 0;
    callback_one_count = 0;
    callback_zero_a_replaces_itself = 0;
    host_set_callback_zero(callback_zero_a);
    host_set_callback_one(callback_one);
    g_host_callback_prefix.delivery_gate_008 = 0;
    result = host_frame_notify_zero();
    EXPECT(result == 0 && callback_zero_a_count == 0 && callback_one_count == 0,
           "channel-zero notification returns 0 and gate zero blocks callback delivery");
    g_host_callback_prefix.delivery_gate_008 = 1;
    result = host_frame_notify_zero();
    EXPECT(result == 0 && callback_zero_a_count == 1 && callback_one_count == 0,
           "channel-zero notification invokes only channel zero despite channel-one registration");
    callback_zero_a_replaces_itself = 1;
    result = host_frame_notify_zero();
    EXPECT(result == 0 && callback_zero_a_count == 2 && callback_zero_b_count == 0,
           "active channel-zero invocation uses the callback captured at entry");
    result = host_frame_notify_zero();
    EXPECT(result == 0 && callback_zero_b_count == 1 && callback_one_count == 0,
           "later channel-zero notification observes callback replacement");
    host_disable_callback_gate();
    result = host_frame_notify_zero();
    EXPECT(result == 0 && callback_zero_b_count == 1,
           "disabled shared gate prevents later channel-zero callback delivery");

    printf("host frame protocol fixture: %d checks, 0 failures\n", checks);
    return 0;
}
