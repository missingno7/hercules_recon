#include <stdio.h>
#include <string.h>
#include "../calibration/engine_file_bridge.c"

EngineInterface g_engine_interface;
static ResourceCallbackContext callback_context;
static char destination[8];
static char payload[16];
static const char prefix_text[] = "prefix";
static const char suffix_text[] = "suffix";
static unsigned long checks;
static unsigned long failures;
static int async_calls;
static int cancel_calls;
static int compact_calls;
static int length_first_calls;
static int length_second_calls;
static int sync_calls;
static int allocate_calls;
static int forwarding_ok;
static void **seen_output_slot;
static unsigned long seen_bytes;
static unsigned long seen_flags;

static void check(const char *label, int passed)
{
    ++checks;
    if (!passed) {
        ++failures;
        printf("FAIL: %s\n", label);
    }
}

static int __cdecl async_callback(const char *prefix, const char *suffix,
                                 int sector_offset, void *buffer,
                                 unsigned long bytes)
{
    ++async_calls;
    forwarding_ok = prefix == prefix_text && suffix == suffix_text &&
                    sector_offset == -7 && buffer == destination &&
                    bytes == 0x87654321UL;
    return -1234567;
}

static int __cdecl cancel_callback(void)
{
    ++cancel_calls;
    return -31;
}

static unsigned long __cdecl compact_callback(int pool_id)
{
    ++compact_calls;
    forwarding_ok = forwarding_ok && pool_id == -0x12345;
    return 0x89abcdefUL;
}

static unsigned long __cdecl length_second(const char *prefix, const char *suffix)
{
    ++length_second_calls;
    forwarding_ok = forwarding_ok && prefix == prefix_text && suffix == suffix_text;
    return 0xfedcba98UL;
}

static unsigned long __cdecl length_first(const char *prefix, const char *suffix)
{
    ++length_first_calls;
    forwarding_ok = prefix == prefix_text && suffix == suffix_text;
    g_engine_interface.file_length_048 = length_second;
    return 0x76543210UL;
}

static unsigned long __cdecl sync_callback(const char *prefix, const char *suffix,
                                           void *buffer, unsigned long bytes)
{
    ++sync_calls;
    forwarding_ok = forwarding_ok && prefix == prefix_text &&
                    suffix == suffix_text && buffer == destination &&
                    bytes == 0xa7654321UL;
    return 0xdeadbeefUL;
}

static void *__cdecl allocate_callback(void **output_slot, unsigned long bytes,
                                       unsigned long flags)
{
    ++allocate_calls;
    seen_output_slot = output_slot;
    seen_bytes = bytes;
    seen_flags = flags;
    *output_slot = payload;
    return payload;
}

int main(void)
{
    void *slot = 0;
    int result;

    g_engine_interface.context_004 = &callback_context;
    g_engine_interface.load_async_020 = async_callback;
    g_engine_interface.cancel_load_024 = cancel_callback;
    g_engine_interface.compact_pool_034 = compact_callback;
    g_engine_interface.file_length_048 = length_first;
    g_engine_interface.load_sync_068 = sync_callback;
    g_engine_interface.allocate_078 = allocate_callback;

    check("native view has 250 words and context at slot one",
          sizeof(EngineInterface) == 1000 &&
          (char *)&g_engine_interface.context_004 - (char *)&g_engine_interface == 4 &&
          g_engine_interface.context_004 == &callback_context);

    result = engine_file_load_async(prefix_text, suffix_text, -7,
                                    destination, 0x87654321UL);
    check("async forwards five arguments and return EAX", async_calls == 1 &&
          forwarding_ok && result == -1234567);

    result = engine_file_cancel();
    check("cancel forwards zero arguments and return EAX", cancel_calls == 1 &&
          result == -31);

    check("compact forwards signed pool id and return EAX",
          engine_pool_compact(-0x12345) == 0x89abcdefUL && compact_calls == 1 &&
          forwarding_ok);

    check("length uses the current table member and returns first result",
          engine_file_length(prefix_text, suffix_text) == 0x76543210UL &&
          length_first_calls == 1);
    check("next length call observes callback's live table mutation",
          engine_file_length(prefix_text, suffix_text) == 0xfedcba98UL &&
          length_second_calls == 1 && forwarding_ok);

    check("sync forwards four arguments and return EAX",
          engine_file_load_sync(prefix_text, suffix_text, destination,
                                0xa7654321UL) == 0xdeadbeefUL &&
          sync_calls == 1 && forwarding_ok);

    check("allocation forwards output slot, size and flags; returns pointer",
          engine_pool_allocate(&slot, 0x76543210UL, 0x87654321UL) == payload &&
          slot == payload && seen_output_slot == &slot &&
          seen_bytes == 0x76543210UL && seen_flags == 0x87654321UL &&
          allocate_calls == 1);

    printf("engine file bridge fixture: %lu checks, %lu failures\n", checks, failures);
    return failures ? 1 : 0;
}
