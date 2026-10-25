#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "../src/shared/engine_interface.h"
#include "../src/shared/resource_record.h"

int __cdecl runtime_control(void);
EngineInterface g_engine_interface;
ActorResource g_actor_resources[81];
short g_control_threshold;
unsigned long g_control_forwarded_value;
unsigned long g_control_global_71ec0;
unsigned char g_control_argument_6eef8[1];
unsigned char g_control_argument_6eefc[1];
unsigned char g_control_argument_6ef00[1];

enum {
    EV_DESTROY_1 = 101, EV_DESTROY_3 = 103, EV_CANCEL = 200,
    EV_CALLBACK = 201, EV_RECLAIM_1 = 301, EV_TWOARG = 401,
    EV_FREE = 402, EV_CLEANUP = 403, EV_HELPER = 404, EV_WORKER = 405,
    EV_UPDATE = 406, EV_REG_A = 407, EV_REG_B = 408, EV_RESET = 409,
    EV_CLEAN_ZERO = 410, EV_INIT = 411, EV_SHUTDOWN = 412,
    EV_POLL = 413, EV_RESOURCE_TEST = 414, EV_BUSY = 415, EV_ZEROARG = 416,
    EV_REPLACEMENT_CALLBACK = 417
};

static ResourceCallbackContext context_storage;
static ResourceCallbackContext replacement_context;
static int events[128];
static int event_count;
static int checks;
static int failures;
static int mode_for_callback;
static int mutate_selector_in_callback;
static int mutate_context_on_cancel;
static int busy_case_active;
static int busy_count;
static jmp_buf terminal_loop_escape;

#define CHECK(expr) do { ++checks; if (!(expr)) { ++failures; \
    printf("FAIL line %d: %s\n", __LINE__, #expr); } } while (0)

static void record(int event)
{
    if (event_count < (int)(sizeof(events) / sizeof(events[0])))
        events[event_count++] = event;
}

static void reset_case(unsigned char selector)
{
    memset(&context_storage, 0, sizeof(context_storage));
    memset(&replacement_context, 0, sizeof(replacement_context));
    memset(g_actor_resources, 0, sizeof(g_actor_resources));
    memset(events, 0, sizeof(events));
    event_count = 0;
    busy_case_active = 0;
    busy_count = 0;
    mode_for_callback = 0;
    mutate_selector_in_callback = 0;
    mutate_context_on_cancel = 0;
    g_engine_interface.context_004 = &context_storage;
    context_storage.unknown_000[3] = selector;
    g_control_threshold = 0;
    g_control_forwarded_value = 0x13579bdfUL;
    g_control_global_71ec0 = 0x2468ace0UL;
}

static int event_is(int index, int event)
{
    return index < event_count && events[index] == event;
}

static void __cdecl load_failed_callback(void)
{
    record(EV_CALLBACK);
    if (mode_for_callback) {
        context_storage.mode_09c = 1;
        context_storage.unknown_000[3] = 0;
        mode_for_callback = 0;
    }
    if (mutate_selector_in_callback)
        context_storage.unknown_000[3] = 33;
}

static void __cdecl replacement_load_failed_callback(void)
{
    record(EV_REPLACEMENT_CALLBACK);
}

void __cdecl resource_destroy(int id)
{
    record(id == 3 ? EV_DESTROY_3 : EV_DESTROY_1);
}

void __cdecl host_cancel(void)
{
    record(EV_CANCEL);
    if (mutate_context_on_cancel) {
        g_engine_interface.context_004 = &replacement_context;
        mutate_context_on_cancel = 0;
    }
}
void __cdecl resource_reclaim(int wait_mode) { record(EV_RECLAIM_1 + wait_mode - 1); }

void __cdecl host_two_argument(void *first, unsigned long second)
{
    record(EV_TWOARG);
    CHECK(first == g_control_argument_6eef8);
    CHECK(second == g_control_forwarded_value);
}

void __cdecl host_free_allocation(void *allocation)
{
    record(EV_FREE);
    CHECK(allocation == g_engine_interface.context_004->unknown_0e4);
}

void __cdecl resource_cleanup(void) { record(EV_CLEANUP); }
void __cdecl resource_helper_646c(void) { record(EV_HELPER); }
void __cdecl host_worker(void) { record(EV_WORKER); }

void __cdecl host_two_argument_update(void *first, unsigned long second)
{
    record(EV_UPDATE);
    CHECK(first == g_control_argument_6eefc || first == g_control_argument_6ef00);
    CHECK(second == g_control_forwarded_value);
}

EngineNoArgCallback __cdecl host_register_callback_a(EngineNoArgCallback callback)
{
    record(EV_REG_A);
    CHECK(callback == 0);
    return 0;
}

EngineNoArgCallback __cdecl host_register_callback_b(EngineNoArgCallback callback)
{
    record(EV_REG_B);
    CHECK(callback == 0);
    return 0;
}

void __cdecl host_reset_resource(unsigned long value)
{
    record(EV_RESET);
    CHECK(value == 0);
}

void __cdecl host_clean(unsigned long value)
{
    record(EV_CLEAN_ZERO);
    CHECK(value == 0);
}

void __cdecl engine_init(void) { record(EV_INIT); }
void __cdecl engine_shutdown(void) { record(EV_SHUTDOWN); }
void __cdecl host_poll(void) { record(EV_POLL); }
void __cdecl resource_test(void) { record(EV_RESOURCE_TEST); }

void __cdecl host_zero_argument(void) { record(EV_ZEROARG); }

void __cdecl host_busy_callback(void)
{
    record(EV_BUSY);
    if (!busy_case_active) return;
    ++busy_count;
    if (busy_count == 1) {
        context_storage.suppress_auto_close_0e0 = 1;
    } else if (busy_count == 2) {
        context_storage.unknown_0e4 = 0;
    } else {
        longjmp(terminal_loop_escape, 1);
    }
}

static void test_case_zero_and_threshold(void)
{
    reset_case(0);
    context_storage.unknown_000[5] = 7;
    CHECK(runtime_control() == 0);
    CHECK(event_count == 0 && context_storage.unknown_000[5] == 7);

    reset_case(5);
    context_storage.unknown_000[5] = 7;
    g_control_threshold = 127;
    CHECK(runtime_control() == 0);
    CHECK(context_storage.unknown_000[5] == 7 && context_storage.unknown_000[3] == 5);

    reset_case(5);
    context_storage.unknown_000[5] = 7;
    g_control_threshold = 128;
    CHECK(runtime_control() == 0);
    CHECK(context_storage.unknown_000[5] == 0 && context_storage.unknown_000[3] == 0);

    reset_case(5);
    context_storage.unknown_000[5] = 7;
    g_control_threshold = -1;
    CHECK(runtime_control() == 0);
    CHECK(context_storage.unknown_000[5] == 7 && context_storage.unknown_000[3] == 5);
}

static void test_case_one_two(void)
{
    reset_case(2);
    context_storage.suppress_auto_close_0e0 = 1;
    context_storage.unknown_0e4 = (void *)0x1234;
    CHECK(runtime_control() == 0);
    CHECK(event_count == 0 && context_storage.unknown_000[3] == 0);

    reset_case(1);
    context_storage.mode_09c = 1;
    context_storage.load_failed = load_failed_callback;
    mutate_selector_in_callback = 1;
    CHECK(runtime_control() == 0);
    CHECK(event_count == 6);
    CHECK(event_is(0, EV_DESTROY_3));
    CHECK(event_is(1, EV_DESTROY_1));
    CHECK(event_is(2, EV_CANCEL));
    CHECK(event_is(3, EV_CALLBACK));
    CHECK(event_is(4, EV_RECLAIM_1));
    CHECK(event_is(5, EV_TWOARG));
    CHECK(g_actor_resources[3].residency_state == 6);
    CHECK(g_actor_resources[1].residency_state == 6);
    CHECK(context_storage.unknown_000[5] == 1 && context_storage.blocked_0df == 1);
    CHECK(context_storage.unknown_000[3] == 0);
}

static void test_case_eighteen_nineteen_fallthrough(void)
{
    int selector;
    for (selector = 18; selector <= 19; ++selector) {
        reset_case((unsigned char)selector);
        context_storage.load_failed = load_failed_callback;
        context_storage.unknown_0e4 = (void *)0x5678;
        CHECK(runtime_control() == 33);
        CHECK(event_count == 11);
        CHECK(event_is(0, EV_FREE));
        CHECK(event_is(1, EV_CLEANUP));
        CHECK(event_is(2, EV_HELPER));
        CHECK(event_is(3, EV_WORKER));
        CHECK(event_is(4, EV_UPDATE));
        CHECK(event_is(5, EV_REG_A));
        CHECK(event_is(6, EV_REG_B));
        CHECK(event_is(7, EV_RESET));
        CHECK(event_is(8, EV_CLEAN_ZERO));
        CHECK(event_is(9, EV_INIT));
        CHECK(event_is(10, EV_SHUTDOWN));
        CHECK(context_storage.unknown_000[0] == 0xff);
        CHECK(context_storage.unknown_128 == 0);
        CHECK(g_control_global_71ec0 == 0);
    }
}

static void append_expected(int *expected, int *count, int event)
{
    expected[*count] = event;
    ++*count;
}

static void test_mode_matrix(void)
{
    int selectors[5] = { 1, 2, 18, 19, 33 };
    int modes[3] = { 0, 1, 2 };
    int expected[20];
    int expected_count;
    int selector_index;
    int mode_index;
    int selector;
    int mode;
    int i;

    for (selector_index = 0; selector_index < 5; ++selector_index) {
        for (mode_index = 0; mode_index < 3; ++mode_index) {
            selector = selectors[selector_index];
            mode = modes[mode_index];
            reset_case((unsigned char)selector);
            context_storage.mode_09c = (unsigned short)mode;
            context_storage.load_failed = load_failed_callback;
            if (selector == 18 || selector == 19)
                context_storage.unknown_0e4 = (void *)0x5678;

            expected_count = 0;
            if (selector == 1 || selector == 2) {
                append_expected(expected, &expected_count, EV_DESTROY_3);
                append_expected(expected, &expected_count, EV_DESTROY_1);
                if (mode == 1) {
                    append_expected(expected, &expected_count, EV_CANCEL);
                    append_expected(expected, &expected_count, EV_CALLBACK);
                }
                append_expected(expected, &expected_count, EV_RECLAIM_1);
                append_expected(expected, &expected_count, EV_TWOARG);
                CHECK(runtime_control() == 0);
            } else if (selector == 18 || selector == 19) {
                if (mode == 1) {
                    append_expected(expected, &expected_count, EV_CANCEL);
                    append_expected(expected, &expected_count, EV_CALLBACK);
                }
                append_expected(expected, &expected_count, EV_FREE);
                append_expected(expected, &expected_count, EV_CLEANUP);
                append_expected(expected, &expected_count, EV_HELPER);
                append_expected(expected, &expected_count, EV_WORKER);
                append_expected(expected, &expected_count, EV_UPDATE);
                if (mode == 1) {
                    append_expected(expected, &expected_count, EV_CANCEL);
                    append_expected(expected, &expected_count, EV_CALLBACK);
                }
                append_expected(expected, &expected_count, EV_REG_A);
                append_expected(expected, &expected_count, EV_REG_B);
                append_expected(expected, &expected_count, EV_RESET);
                append_expected(expected, &expected_count, EV_CLEAN_ZERO);
                append_expected(expected, &expected_count, EV_INIT);
                append_expected(expected, &expected_count, EV_SHUTDOWN);
                CHECK(runtime_control() == 33);
                CHECK(context_storage.unknown_000[0] == 0xff);
            } else {
                if (mode == 1) {
                    append_expected(expected, &expected_count, EV_CANCEL);
                    append_expected(expected, &expected_count, EV_CALLBACK);
                }
                append_expected(expected, &expected_count, EV_REG_A);
                append_expected(expected, &expected_count, EV_REG_B);
                append_expected(expected, &expected_count, EV_RESET);
                append_expected(expected, &expected_count, EV_CLEAN_ZERO);
                append_expected(expected, &expected_count, EV_INIT);
                append_expected(expected, &expected_count, EV_SHUTDOWN);
                CHECK(runtime_control() == 33);
            }

            CHECK(event_count == expected_count);
            for (i = 0; i < expected_count && i < event_count; ++i)
                CHECK(events[i] == expected[i]);
            if (selector == 18 || selector == 19 || selector == 33) {
                CHECK(context_storage.unknown_128 == 0);
                CHECK(g_control_global_71ec0 == 0);
            }
        }
    }
}

static void test_cancel_reloads_mutated_context_and_callback(void)
{
    int expected[13] = {
        EV_CANCEL, EV_REPLACEMENT_CALLBACK, EV_FREE, EV_CLEANUP,
        EV_HELPER, EV_WORKER, EV_UPDATE, EV_REG_A, EV_REG_B, EV_RESET,
        EV_CLEAN_ZERO, EV_INIT, EV_SHUTDOWN
    };
    int i;

    reset_case(18);
    context_storage.mode_09c = 1;
    context_storage.load_failed = load_failed_callback;
    context_storage.unknown_128 = 0x5a;
    context_storage.unknown_0e4 = (void *)0x5678;
    replacement_context.mode_09c = 0;
    replacement_context.load_failed = replacement_load_failed_callback;
    replacement_context.unknown_128 = 0x6b;
    replacement_context.unknown_0e4 = (void *)0x9876;
    mutate_context_on_cancel = 1;

    CHECK(runtime_control() == 33);
    CHECK(event_count == 13);
    for (i = 0; i < 13 && i < event_count; ++i)
        CHECK(events[i] == expected[i]);
    CHECK(g_engine_interface.context_004 == &replacement_context);
    CHECK(replacement_context.unknown_000[0] == 0xff);
    CHECK(replacement_context.unknown_128 == 0);
    CHECK(context_storage.unknown_128 == 0x5a);
    CHECK(g_control_global_71ec0 == 0);
}

static void test_case_thirty_three(void)
{
    reset_case(33);
    CHECK(runtime_control() == 33);
    CHECK(event_count == 6);
    CHECK(event_is(0, EV_REG_A));
    CHECK(event_is(1, EV_REG_B));
    CHECK(event_is(2, EV_RESET));
    CHECK(event_is(3, EV_CLEAN_ZERO));
    CHECK(event_is(4, EV_INIT));
    CHECK(event_is(5, EV_SHUTDOWN));
}

static void test_default_live_poll_and_terminal_loop(void)
{
    reset_case(3);
    context_storage.unknown_000[6] = 3;
    context_storage.unknown_12c = 0x1234;
    context_storage.unknown_0e4 = (void *)0x9990;
    busy_case_active = 1;

    if (setjmp(terminal_loop_escape) == 0) {
        (void)runtime_control();
        CHECK(0);
    }

    CHECK(busy_count == 3);
    CHECK(context_storage.unknown_000[3] == 19);
    CHECK(context_storage.unknown_12c == 0);
    CHECK(event_count == 13);
    CHECK(event_is(0, EV_POLL));
    CHECK(event_is(1, EV_CLEAN_ZERO));
    CHECK(event_is(2, EV_RESOURCE_TEST));
    CHECK(event_is(3, EV_BUSY));
    CHECK(event_is(4, EV_POLL));
    CHECK(event_is(5, EV_CLEAN_ZERO));
    CHECK(event_is(6, EV_RESOURCE_TEST));
    CHECK(event_is(7, EV_BUSY));
    CHECK(event_is(8, EV_CLEANUP));
    CHECK(event_is(9, EV_WORKER));
    CHECK(event_is(10, EV_CLEAN_ZERO));
    CHECK(event_is(11, EV_UPDATE));
    CHECK(event_is(12, EV_BUSY));
}

int main(void)
{
    test_case_zero_and_threshold();
    test_case_one_two();
    test_case_eighteen_nineteen_fallthrough();
    test_case_thirty_three();
    test_mode_matrix();
    test_cancel_reloads_mutated_context_and_callback();
    test_default_live_poll_and_terminal_loop();
    printf("engine control fixture: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}
