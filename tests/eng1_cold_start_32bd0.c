#include <string.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/shared/engine_interface.h"
#include "../src/eng1/cold_start_frame_data.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned long u32;
typedef long s32;

extern u8 g_cold_start_state_bytes_70820[0x1c];
extern u16 g_stpanel1_upload_rectangle_words[4];
extern int g_context_sequence_index;
extern u8 g_frame_color_state_prefix_6ffc8[0x0c];
extern u32 g_frame_global_73484;
extern u8 *g_dispatch_frame_current_71ec4;
extern void __cdecl dispatch_local_32bd0(void);

enum { EVENT_33620 = 1, EVENT_33420, EVENT_2C60, EVENT_1F40, EVENT_2C980 };
enum { MAX_EVENTS = 256 };

typedef struct FixtureEvent {
    int kind;
    s32 args[7];
    u16 rectangle[4];
    u32 mode;
    u32 key;
    void *pointer;
} FixtureEvent;

static FixtureEvent g_events[MAX_EVENTS];
static int g_event_count;
static u8 g_test_context[0x1400];
static u8 g_test_frame[0x1400];
static u32 g_test_cursor[16];
static int g_failures;
static jmp_buf g_incomplete_jump;
static int g_incomplete_armed;
static int g_incomplete_calls;
static unsigned long g_incomplete_rva;
static const char *g_incomplete_reason;

void __cdecl eng1_incomplete_behavior(unsigned long rva, const char *reason)
{
    ++g_incomplete_calls;
    g_incomplete_rva = rva;
    g_incomplete_reason = reason;
    if (g_incomplete_armed)
        longjmp(g_incomplete_jump, 1);
    fprintf(stderr, "unexpected incomplete behavior at 0x%08lx: %s\n",
            rva, reason != 0 ? reason : "(no reason)");
    exit(1);
}

static void event_start(int kind)
{
    if (g_event_count < MAX_EVENTS) {
        memset(&g_events[g_event_count], 0, sizeof(g_events[g_event_count]));
        g_events[g_event_count].kind = kind;
        ++g_event_count;
    } else {
        ++g_failures;
    }
}

static void event_args(int kind, s32 a, s32 b, s32 c, s32 d,
                       s32 e, s32 f, s32 g)
{
    int at = g_event_count;
    event_start(kind);
    if (at < MAX_EVENTS) {
        g_events[at].args[0] = a;
        g_events[at].args[1] = b;
        g_events[at].args[2] = c;
        g_events[at].args[3] = d;
        g_events[at].args[4] = e;
        g_events[at].args[5] = f;
        g_events[at].args[6] = g;
    }
}

void __cdecl dispatch_local_33620(s32 a, s32 b, s32 c, s32 d,
                                   s32 e, s32 f, s32 g)
{
    event_args(EVENT_33620, a, b, c, d, e, f, g);
}

void __cdecl dispatch_local_33420(s32 a, s32 b, s32 c, s32 d,
                                   s32 e, s32 f, s32 g)
{
    event_args(EVENT_33420, a, b, c, d, e, f, g);
}

void __cdecl dispatch_local_2c60(const u16 rectangle[4], u32 mode, u32 key)
{
    int at = g_event_count;
    event_start(EVENT_2C60);
    if (at < MAX_EVENTS) {
        memcpy(g_events[at].rectangle, rectangle, sizeof(g_events[at].rectangle));
        g_events[at].mode = mode;
        g_events[at].key = key;
    }
}

void __cdecl dispatch_local_1f40(void)
{
    event_start(EVENT_1F40);
}

static void __cdecl fixture_engine_2c980(void *first, u32 second, u32 third,
                                          u32 fourth, u32 fifth)
{
    int at = g_event_count;
    event_start(EVENT_2C980);
    if (at < MAX_EVENTS) {
        g_events[at].pointer = first;
        g_events[at].args[0] = (s32)second;
        g_events[at].args[1] = (s32)third;
        g_events[at].args[2] = (s32)fourth;
        g_events[at].args[3] = (s32)fifth;
    }
}

static int check(int condition, int line)
{
    if (!condition)
        return line;
    return 0;
}

static void reset_fixture(void)
{
    memset(&g_engine_interface, 0, sizeof(g_engine_interface));
    memset(g_test_context, 0, sizeof(g_test_context));
    memset(g_test_frame, 0, sizeof(g_test_frame));
    memset(g_test_cursor, 0, sizeof(g_test_cursor));
    memset(g_cold_start_state_bytes_70820, 0, 0x1c);
    memset(g_frame_color_state_prefix_6ffc8, 0, 0x0c);
    g_context_sequence_index = 0;
    g_engine_interface.context_004 =
        (ResourceCallbackContext *)(void *)g_test_context;
    g_engine_interface.dispatch_2c980_2ac = fixture_engine_2c980;
    g_dispatch_frame_current_71ec4 = g_test_frame;
    g_frame_global_73484 = (u32)(void *)g_test_cursor;
    *(u32 *)(void *)(g_test_frame + 0x1374) = 0x11223344UL;
    g_event_count = 0;
    g_failures = 0;
    g_incomplete_armed = 0;
    g_incomplete_calls = 0;
    g_incomplete_rva = 0;
    g_incomplete_reason = 0;
}

static int test_animation_and_frame_link(void)
{
    int result;
    int index;
    FixtureEvent *first;
    FixtureEvent *second;
    u32 cursor;

    reset_fixture();
    g_test_context[4] = 1;
    g_test_context[6] = 3;
    g_cold_start_state_bytes_70820[0x18] = 0xd0;
    g_cold_start_state_bytes_70820[0x1a] = 0xd0;
    cursor = g_frame_global_73484;

    dispatch_local_32bd0();

    result = check(g_event_count >= 4, __LINE__);
    if (result) return result;
    first = &g_events[0];
    second = &g_events[1];
    result = check(first->kind == EVENT_33620 &&
                   first->args[0] == 0x18 && first->args[1] == 0xd0 &&
                   first->args[2] == 0xf0 && first->args[3] == 0x28 &&
                   first->args[4] == 0x10 && first->args[5] == 0x10 &&
                   first->args[6] == 3, __LINE__);
    if (result) return result;
    result = check(second->kind == EVENT_33620 &&
                   second->args[0] == 0x25 && second->args[1] == 0xd0 &&
                   second->args[2] == 0xf0 && second->args[3] == 0x38 &&
                   second->args[4] == 0x10 && second->args[5] == 0x10 &&
                   second->args[6] == 3, __LINE__);
    if (result) return result;
    result = check(g_context_sequence_index == -1 &&
                   g_test_context[3] == 0x13, __LINE__);
    if (result) return result;
    result = check(g_events[2].kind == EVENT_33620 &&
                   g_events[2].args[0] == 0xec &&
                   g_events[2].args[1] == 0x14 &&
                   g_events[2].args[2] == 0x88 &&
                   g_events[2].args[3] == 0 &&
                   g_events[2].args[4] == 0x40 &&
                   g_events[2].args[5] == 0x1f &&
                   g_events[2].args[6] == 0, __LINE__);
    if (result) return result;
    result = check(g_events[8].kind == EVENT_33620 &&
                   g_events[8].args[0] == 0x5c &&
                   g_events[8].args[1] == 0x1a &&
                   g_events[8].args[2] == 0x18 &&
                   g_events[8].args[3] == 0x28 &&
                   g_events[8].args[4] == 0x40 &&
                   g_events[8].args[5] == 0x0d &&
                   g_events[8].args[6] == 0, __LINE__);
    if (result) return result;
    result = check(g_events[15].kind == EVENT_33620 &&
                   g_events[15].args[0] == 0x6a &&
                   g_events[15].args[1] == 0x1d &&
                   g_events[15].args[2] == 0 &&
                   g_events[15].args[3] == 0x30 &&
                   g_events[15].args[4] == 4 &&
                   g_events[15].args[5] == 6 &&
                   g_events[15].args[6] == 1, __LINE__);
    if (result) return result;
    index = 0;
    while (index < g_event_count && g_events[index].kind != EVENT_1F40)
        ++index;
    result = check(index == 16, __LINE__);
    if (result) return result;
    result = check(g_events[g_event_count - 2].kind == EVENT_1F40 &&
                   g_events[g_event_count - 1].kind == EVENT_2C980,
                   __LINE__);
    if (result) return result;
    result = check(g_events[g_event_count - 1].pointer == (void *)cursor &&
                   g_events[g_event_count - 1].args[0] == 0 &&
                   g_events[g_event_count - 1].args[1] == 0 &&
                   g_events[g_event_count - 1].args[2] == 0x180 &&
                   g_events[g_event_count - 1].args[3] == 0, __LINE__);
    if (result) return result;
    result = check(*(u32 *)(void *)cursor == 0x11223344UL &&
                   *(u32 *)(void *)(g_test_frame + 0x1374) == cursor &&
                   g_frame_global_73484 == cursor + 0x0c, __LINE__);
    if (result) return result;
    return check(g_failures == 0, __LINE__);
}

static int test_sequence_index_wrap_edges(void)
{
    static const s32 initial_indices[2] = {
        (s32)0x80000000UL, 0x7fffffffL
    };
    static const s32 expected_after[2] = {
        0x7fffffffL, 0x7ffffffeL
    };
    int case_index;
    int result;
    s32 expected_first_y;
    s32 expected_second_y;

    for (case_index = 0; case_index < 2; ++case_index) {
        reset_fixture();
        g_test_context[3] = 0x71;
        g_test_context[4] = 1;
        g_test_context[6] = 3;
        g_context_sequence_index = initial_indices[case_index];

        /* Both 0x80000000+30 and 0x7fffffff+30 wrap to values whose signed
         * /30 quotient is -71582787. Native 0x32c04..0x32c0c implements that
         * signed division; the later /10 yields quotient -7158278 and
         * remainder -7. */
        expected_first_y = -114532408L;
        expected_second_y = -72L;

        dispatch_local_32bd0();

        result = check(g_event_count >= 2 &&
                       g_events[0].kind == EVENT_33620 &&
                       g_events[0].args[0] == 0x18 &&
                       g_events[0].args[3] == expected_first_y &&
                       g_events[1].kind == EVENT_33620 &&
                       g_events[1].args[0] == 0x25 &&
                       g_events[1].args[3] == expected_second_y,
                       __LINE__);
        if (result) return result;
        result = check(g_context_sequence_index ==
                           expected_after[case_index] &&
                       g_test_context[3] == 0x71, __LINE__);
        if (result) return result;
    }
    return 0;
}

static int test_transition_cache_and_blend(void)
{
    int result;
    int saw_first_pair;
    int saw_second_pair;
    int index;
    int first_33420;
    u32 cursor;

    reset_fixture();
    g_test_context[4] = 0;
    *(s16 *)(void *)(g_test_context + 0x1e) = 0;
    *(s16 *)(void *)(g_test_context + 0x20) = 6;
    *(s16 *)(void *)(g_test_context + 0x22) = 4;
    *(s16 *)(void *)(g_test_context + 0x24) = 1;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x00) = 0;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x02) = 2;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x04) = 1;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x06) = 3;
    g_cold_start_state_bytes_70820[8] = 2;
    g_cold_start_state_bytes_70820[9] = 2;
    g_cold_start_state_bytes_70820[0x0a] = 3;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x0c) = 0x100;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x10) = 10;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x12) = 12;
    g_cold_start_state_bytes_70820[0x14] = 2;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x16) = 64;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x18) = 0xd0;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x1a) = 0xd0;
    g_frame_color_state_prefix_6ffc8[10] = 0xa5;
    g_frame_color_state_prefix_6ffc8[11] = 0xa5;
    cursor = g_frame_global_73484;

    dispatch_local_32bd0();

    result = check(*(s16 *)(void *)(g_test_context + 0x22) == 2 &&
                   *(s16 *)(void *)(g_test_context + 0x20) == 4 &&
                   *(s16 *)(void *)(g_cold_start_state_bytes_70820 + 0x10) == 12,
                   __LINE__);
    if (result) return result;
    result = check(*(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x04) == 0 &&
                   *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x06) == 2,
                   __LINE__);
    if (result) return result;
    result = check(*(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x16) == 64 &&
                   g_frame_global_6ffd0 == 64 &&
                   g_frame_global_6ffc8 == 0x60606060UL &&
                   g_frame_global_6ffcc == 0x20202020UL &&
                   g_frame_color_state_prefix_6ffc8[10] == 0xa5 &&
                   g_frame_color_state_prefix_6ffc8[11] == 0xa5, __LINE__);
    if (result) return result;

    saw_first_pair = 0;
    saw_second_pair = 0;
    for (index = 0; index < g_event_count; ++index) {
        if (g_events[index].kind == EVENT_2C60 &&
            g_events[index].mode == 0 && g_events[index].key == 0x238 &&
            g_events[index].rectangle[0] == 36 &&
            g_events[index].rectangle[1] == 0x238 &&
            g_events[index].rectangle[2] == 12 &&
            g_events[index].rectangle[3] == 24)
            saw_first_pair = 1;
        if (g_events[index].kind == EVENT_2C60 &&
            g_events[index].mode == 0x0c && g_events[index].key == 0x238 &&
            g_events[index].rectangle[0] == 0 &&
            g_events[index].rectangle[1] == 0x328 &&
            g_events[index].rectangle[2] == 12 &&
            g_events[index].rectangle[3] == 24)
            saw_second_pair = 1;
    }
    result = check(saw_first_pair && saw_second_pair, __LINE__);
    if (result) return result;
    first_33420 = 0;
    while (first_33420 < g_event_count &&
           g_events[first_33420].kind != EVENT_33420)
        ++first_33420;
    result = check(first_33420 + 6 < g_event_count, __LINE__);
    if (result) return result;
    result = check(g_events[first_33420].args[0] == 188 &&
                   g_events[first_33420].args[1] == 208 &&
                   g_events[first_33420].args[2] == 160 &&
                   g_events[first_33420].args[3] == 80 &&
                   g_events[first_33420].args[4] == 36 &&
                   g_events[first_33420].args[5] == 16 &&
                   g_events[first_33420].args[6] == 0, __LINE__);
    if (result) return result;
    result = check(g_events[first_33420 + 1].args[0] == 256 &&
                   g_events[first_33420 + 1].args[1] == 208 &&
                   g_events[first_33420 + 1].args[2] == 200 &&
                   g_events[first_33420 + 1].args[3] == 80,
                   __LINE__);
    if (result) return result;
    result = check(g_events[first_33420 + 2].args[0] == 208 &&
                   g_events[first_33420 + 2].args[1] == 211 &&
                   g_events[first_33420 + 2].args[2] == 24 &&
                   g_events[first_33420 + 2].args[3] == 40 &&
                   g_events[first_33420 + 2].args[4] == 64 &&
                   g_events[first_33420 + 2].args[5] == 13 &&
                   g_events[first_33420 + 2].args[6] == 0, __LINE__);
    if (result) return result;
    result = check(g_events[first_33420 + 3].args[0] == 272 &&
                   g_events[first_33420 + 3].args[1] == 211 &&
                   g_events[first_33420 + 4].args[0] == 208 &&
                   g_events[first_33420 + 4].args[1] == 214 &&
                   g_events[first_33420 + 4].args[4] == 3 &&
                   g_events[first_33420 + 5].args[0] == 211 &&
                   g_events[first_33420 + 5].args[1] == 214 &&
                   g_events[first_33420 + 5].args[2] == 3 &&
                   g_events[first_33420 + 5].args[4] == 21,
                   __LINE__);
    if (result) return result;
    result = check(g_events[first_33420 + 6].args[0] == 232 &&
                   g_events[first_33420 + 6].args[1] == 214 &&
                   g_events[first_33420 + 6].args[3] == 0x30 &&
                   g_events[first_33420 + 6].args[4] == 24,
                   __LINE__);
    if (result) return result;
    result = check(g_events[g_event_count - 2].kind == EVENT_1F40 &&
                   g_events[g_event_count - 1].kind == EVENT_2C980 &&
                   g_events[g_event_count - 1].pointer == (void *)cursor,
                   __LINE__);
    if (result) return result;
    return check(g_failures == 0, __LINE__);
}

static int test_data_provider_values(void)
{
    const u16 expected[4] = { 0, 0x0200, 0x0080, 0x0200 };
    return check(memcmp(g_stpanel1_upload_rectangle_words, expected,
                        sizeof(expected)) == 0, __LINE__);
}

static int test_measured_coordinate_prefix_guard(void)
{
    int result;
    int index;
    int saw_row_191;
    int pair_count;
    int stopped;

    reset_fixture();
    g_test_context[4] = 1;
    *(u16 *)(void *)(g_test_context + 0x20) = 0x0100;
    *(u16 *)(void *)(g_test_context + 0x22) = 0;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x00) = 0x00bf;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x02) = 0x00c0;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x04) = 0;
    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x06) = 0;

    g_incomplete_armed = 1;
    stopped = setjmp(g_incomplete_jump);
    if (stopped == 0)
        dispatch_local_32bd0();
    g_incomplete_armed = 0;

    saw_row_191 = 0;
    pair_count = 0;
    for (index = 0; index < g_event_count; ++index) {
        if (g_events[index].kind == EVENT_2C60) {
            ++pair_count;
            if (g_events[index].rectangle[0] == 108 &&
                g_events[index].rectangle[1] == 760 &&
                g_events[index].rectangle[2] == 12 &&
                g_events[index].rectangle[3] == 24 &&
                g_events[index].mode == 0 &&
                g_events[index].key == 0x238)
                saw_row_191 = 1;
        }
    }
    result = check(stopped != 0 && pair_count == 1 && saw_row_191, __LINE__);
    if (result) return result;
    result = check(g_incomplete_calls == 1 && g_incomplete_rva == 0x32bd0UL &&
                   g_incomplete_reason != 0 &&
                   strstr(g_incomplete_reason, "192-pair prefix") != 0,
                   __LINE__);
    if (result) return result;
    result = check(*(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x04) ==
                       0x00bf &&
                   *(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x06) ==
                       0, __LINE__);
    if (result) return result;
    return check(g_failures == 0, __LINE__);
}

static int test_coordinate_prefix_edges(void)
{
    return check(sizeof(g_frame_coordinate_prefix_5dd80) == 0x180 &&
                 g_frame_coordinate_prefix_5dd80[0] == 3 &&
                 g_frame_coordinate_prefix_5dd80[1] == 0 &&
                 g_frame_coordinate_prefix_5dd80[4] == 0 &&
                 g_frame_coordinate_prefix_5dd80[5] == 10 &&
                 g_frame_coordinate_prefix_5dd80[0x17e] == 9 &&
                 g_frame_coordinate_prefix_5dd80[0x17f] == 8, __LINE__);
}

int main(void)
{
    int result;
    result = test_animation_and_frame_link();
    if (result) return result;
    result = test_sequence_index_wrap_edges();
    if (result) return result;
    result = test_transition_cache_and_blend();
    if (result) return result;
    result = test_measured_coordinate_prefix_guard();
    if (result) return result;
    result = test_coordinate_prefix_edges();
    if (result) return result;
    return test_data_provider_values();
}
