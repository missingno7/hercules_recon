#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/eng1/interpreters.h"
#include "../src/eng1/actor_control_tables.h"
#include "../src/eng1/read_guard.h"
#include "../src/eng1/incomplete.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

typedef struct SourceActorList SourceActorList;
struct SourceActorList {
    volatile long count;
    Actor *items[5];
};

typedef struct SourceEvent SourceEvent;
struct SourceEvent {
    char kind;
    Actor *actor;
    unsigned long argument;
};

static Actor g_parent;
static Actor g_children[5];
static SourceActorList g_list;
static void *g_frames[16];
static unsigned long g_frame_tokens[16];
static SourceEvent g_events[32];
static unsigned int g_event_count;
static unsigned int g_41000_calls;
static unsigned int g_producer_calls;
static unsigned int g_mutation_applied;
static unsigned int g_unexpected_calls;
static unsigned short g_rows_before[18][6];

extern void __cdecl eng1_native_code_target_03ca80(Actor *actor);
extern void __cdecl eng1_source_actual_0041000(Actor *child,
                                                unsigned long category);

static void fail(const char *message)
{
    printf("FAIL source-only 3ca80: %s\n", message);
    exit(1);
}

static void record_event(char kind, Actor *actor, unsigned long argument)
{
    if (g_event_count >= sizeof(g_events) / sizeof(g_events[0]))
        fail("event trace overflow");
    g_events[g_event_count].kind = kind;
    g_events[g_event_count].actor = actor;
    g_events[g_event_count].argument = argument;
    ++g_event_count;
}

static volatile u8 *actor_byte(Actor *actor, unsigned int offset)
{
    return (volatile u8 *)actor + offset;
}

static u32 actor_dword(Actor *actor, unsigned int offset)
{
    return *(volatile u32 *)(void *)((volatile u8 *)actor + offset);
}

/* Test-only list producer, deliberately not a 2b130 implementation. */
int __cdecl eng1_native_code_target_002b130(Actor *actor)
{
    ++g_producer_calls;
    if (actor != &g_parent)
        fail("controlled producer received the wrong parent");
    *actor_byte(actor, 0x21U) = 1U;
    *(volatile u32 *)(void *)((volatile u8 *)actor + 0x120U) =
        (u32)(void *)&g_list;
    g_list.count = 1L;
    g_list.items[0] = &g_children[0];
    record_event('P', actor, 1UL);
    return 1;
}

/* This wrapper delegates to the actual 0x41000 C provider, then injects
 * controlled post-call mutations to discriminate the caller's reloads. */
void __cdecl eng1_native_code_target_0041000(Actor *child,
                                              unsigned long category)
{
    ++g_41000_calls;
    record_event('C', child, category);
    eng1_source_actual_0041000(child, category);
    if (g_41000_calls == 1U) {
        *actor_byte(&g_parent, 0x21U) = 2U;
        g_children[0].kind = 0x30U;
        g_list.items[1] = &g_children[1];
        g_list.items[2] = &g_children[2];
        g_list.items[3] = &g_children[3];
        g_list.items[4] = &g_children[4];
        g_list.count = 5L;
        g_mutation_applied = 1U;
    }
}

/* Keep the real shared select_frame provider while tracing the guarded API
 * boundary used by the reconstructed callback and 0x41000 provider. */
void __cdecl eng1_guarded_select_frame(Actor *actor, int index)
{
    if (actor == 0 || actor->frames != g_frames || index < 0 || index >= 16)
        fail("frame selection escaped the controlled 16-slot frame table");
    record_event('F', actor, (unsigned long)index);
    select_frame(actor, index);
}

int __cdecl remove_actor(Actor *actor)
{
    (void)actor;
    ++g_unexpected_calls;
    fail("successful producer path unexpectedly called remove_actor");
    return 0;
}

void eng1_incomplete_behavior(unsigned long rva, const char *reason)
{
    printf("FAIL incomplete guard rva=%08lx: %s\n", rva, reason);
    exit(1);
}

static void initialize_child(Actor *child, unsigned short kind,
                             unsigned long y, unsigned long flags,
                             unsigned short initial_frame_word)
{
    memset(child, 0, sizeof(*child));
    child->kind = kind;
    child->y = (long)y;
    child->flags = flags;
    child->unknown_02a = initial_frame_word;
    child->frames = g_frames;
    child->frame_index = 6;
    child->frame_ticks = 7;
    child->frame_state = 8;
    child->image_or_program = g_frames[6];
}

static void expect_event(unsigned int index, char kind, Actor *actor,
                         unsigned long argument)
{
    if (index >= g_event_count || g_events[index].kind != kind ||
        g_events[index].actor != actor ||
        g_events[index].argument != argument)
        fail("ordered source callback/frame event differs from prediction");
}

static void check_frame_result(Actor *actor, short index)
{
    if (actor->frame_index != index ||
        actor->image_or_program != g_frames[index] ||
        actor->frame_state != -1 || actor->frame_ticks != 0)
        fail("actual select_frame provider result differs");
}

int main(void)
{
    unsigned int i;
    unsigned int source_checks = 0;
    const char expected_kinds[7] = { 'P', 'C', 'F', 'F', 'C', 'F', 'F' };
    Actor *expected_actors[7];
    unsigned long expected_args[7] = { 1UL, 1UL, 2UL, 12UL, 2UL, 1UL, 12UL };

    memset(&g_parent, 0, sizeof(g_parent));
    g_parent.x = 0x00030000L;
    g_parent.y = 0x00900000L;
    g_parent.category = 0x5aU;
    g_parent.kind = 0U;
    for (i = 0; i < 16U; ++i) {
        g_frame_tokens[i] = 0xface0000UL + i;
        g_frames[i] = &g_frame_tokens[i];
    }
    initialize_child(&g_children[0], 0x2fU, 0x00110000UL, 0x00000200UL, 0x4141U);
    initialize_child(&g_children[1], 0x2fU, 0x00220000UL, 0x00000200UL, 0x4242U);
    initialize_child(&g_children[2], 0x30U, 0x00330000UL, 0x00000200UL, 0x4343U);
    initialize_child(&g_children[3], 0x30U, 0x00440000UL, 0x00000200UL, 0x4444U);
    initialize_child(&g_children[4], 0x30U, 0x00550000UL, 0x00000200UL, 0x5555U);
    g_list.count = 0L;
    for (i = 0; i < 5U; ++i)
        g_list.items[i] = 0;

    memcpy(g_rows_before, g_eng1_classifier_rows_65678, sizeof(g_rows_before));
    /* Controlled source-data discriminators: producer chooses row 1 for the
     * cached mask; actual 0x41000 reads columns 2 and 3 for classes 1 and 2. */
    g_eng1_classifier_rows_65678[1][1] = 5U;
    g_eng1_classifier_rows_65678[1][2] = 3U;
    g_eng1_classifier_rows_65678[1][3] = 0x1234U;
    g_eng1_classifier_rows_65678[2][1] = 0U;
    g_eng1_classifier_rows_65678[2][2] = 2U;
    g_eng1_classifier_rows_65678[2][3] = 0x2345U;

    eng1_native_code_target_03ca80(&g_parent);

    expected_actors[0] = &g_parent;
    expected_actors[1] = &g_children[0];
    expected_actors[2] = &g_children[0];
    expected_actors[3] = &g_children[0];
    expected_actors[4] = &g_children[1];
    expected_actors[5] = &g_children[1];
    expected_actors[6] = &g_children[3];
    if (g_event_count != 7U || g_producer_calls != 1U || g_41000_calls != 2U ||
        g_mutation_applied != 1U || g_unexpected_calls != 0U)
        fail("source-only callback counts differ");
    ++source_checks;
    for (i = 0; i < 7U; ++i) {
        expect_event(i, expected_kinds[i], expected_actors[i], expected_args[i]);
        ++source_checks;
    }
    if (g_parent.x != 0x00010000L || *actor_byte(&g_parent, 0x21U) != 2U ||
        g_parent.category != 0x5aU || g_list.count != 5L)
        fail("parent coordinate/class byte/live-count postconditions differ");
    ++source_checks;

    if (g_children[0].kind != 0x30U ||
        g_children[0].y != (long)0x12340000UL ||
        g_children[0].unknown_02a != 9U ||
        (g_children[0].flags & 0x200U) != 0U)
        fail("first child provider mutation/kind reload/mask-set result differs");
    check_frame_result(&g_children[0], 12);
    source_checks += 2;

    if (g_children[1].kind != 0x2fU ||
        g_children[1].y != (long)0x23450000UL ||
        g_children[1].unknown_02a != 2U)
        fail("second child did not use reloaded class 2 column 2/3 values");
    check_frame_result(&g_children[1], 1);
    source_checks += 2;

    if (g_children[2].unknown_02a != 0x4343U ||
        (g_children[2].flags & 0x200U) == 0U ||
        g_children[2].frame_index != 6)
        fail("first post-non-0x30 mask-clear child unexpectedly selected a frame");
    ++source_checks;
    if (g_children[3].unknown_02a != 9U ||
        (g_children[3].flags & 0x200U) != 0U)
        fail("following kind-0x30 mask-set child did not select/clear");
    check_frame_result(&g_children[3], 12);
    source_checks += 2;
    if (g_children[4].unknown_02a != 0x5555U ||
        (g_children[4].flags & 0x200U) == 0U ||
        g_children[4].frame_index != 6)
        fail("trailing kind-0x30 mask-clear child unexpectedly selected a frame");
    ++source_checks;

    if (actor_dword(&g_children[0], 0xd8U) != 0x00110000UL ||
        actor_dword(&g_children[0], 0xdcU) != 0x40UL ||
        actor_dword(&g_children[0], 0xe0U) != 0xa0UL ||
        actor_dword(&g_children[1], 0xd8U) != 0x00220000UL ||
        actor_dword(&g_children[1], 0xdcU) != 0x40UL ||
        actor_dword(&g_children[1], 0xe0U) != 0xa0UL)
        fail("kind-0x2f child d8/dc/e0 setup differs");
    ++source_checks;

    if (memcmp(g_eng1_classifier_rows_65678, g_rows_before,
               sizeof(g_rows_before)) == 0)
        fail("test classifier changes unexpectedly disappeared before restore");
    memcpy(g_eng1_classifier_rows_65678, g_rows_before, sizeof(g_rows_before));
    if (memcmp(g_eng1_classifier_rows_65678, g_rows_before,
               sizeof(g_rows_before)) != 0)
        fail("test classifier owner was not restored");
    ++source_checks;

    printf("PASS source_only=1 native_oracle=not_called cases=1 checks=%u events=%u producer=controlled_2b130_callback_wrapper=actual_41000_plus_postcall_mutation frame_provider=src/shared/frame_state.c\n",
           source_checks, g_event_count);
    return 0;
}
