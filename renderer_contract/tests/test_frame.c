#include "hercules_frame.h"
#include "hercules_frame.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#if defined(_MSC_VER)
#define FRAME_TEST_CDECL __cdecl
#else
#define FRAME_TEST_CDECL
#endif

#if defined(FRAME_TEST_ACCEPTED_WRITER)
RENDERER_ASSERT_LIVE_X86_POINTER_WIDTH(frame_test_requires_x86_pointers);
extern int FRAME_TEST_CDECL frame_chain_write(void *first,
                                               unsigned long raw_count);
#endif

#define TEST_PACKET_COUNT ((renderer_u32)4u)
#define TEST_WALK_CAPACITY (ENG3_FRAME_ORDER_TABLE_COUNT + TEST_PACKET_COUNT + 2u)

typedef struct FrameFixtureMemory {
    Eng1FramePairViewDraft eng1_frames;
    Eng3FramePairViewDraft eng3_frames;
    TitleBufferPairViewDraft title_buffers;
    renderer_raw32 packet_links[TEST_PACKET_COUNT];
} FrameFixtureMemory;

typedef struct FrameTestState {
    FrameFixtureMemory memory;
    renderer_u8 *writable_bytes;
    FrameRaw32Region region;
    renderer_raw32 visits[TEST_WALK_CAPACITY];
    FrameVisitGuard guard;
    renderer_u32 check_count;
    renderer_u32 failure_count;
} FrameTestState;

typedef struct FrameResolverProbe {
    FrameRaw32Region *region;
    renderer_raw32 forbidden_address;
    renderer_u32 forbidden_queries;
} FrameResolverProbe;

static int expect(FrameTestState *state, int condition, const char *name)
{
    ++state->check_count;
    if (!condition) {
        ++state->failure_count;
        printf("FAIL %s\n", name);
        return 0;
    }
    printf("PASS %s\n", name);
    return 1;
}

static int test_store32(FrameTestState *state,
                        renderer_raw32 address,
                        renderer_raw32 value)
{
    renderer_u32 offset;
    if (address < state->region.base ||
        address - state->region.base > state->region.size ||
        4u > state->region.size - (address - state->region.base))
        return 0;
    offset = address - state->region.base;
    state->writable_bytes[offset + 0] = (renderer_u8)value;
    state->writable_bytes[offset + 1] = (renderer_u8)(value >> 8);
    state->writable_bytes[offset + 2] = (renderer_u8)(value >> 16);
    state->writable_bytes[offset + 3] = (renderer_u8)(value >> 24);
    return 1;
}

static int test_read32(FrameTestState *state,
                       renderer_raw32 address,
                       renderer_raw32 *value)
{
    const renderer_u8 *resolved;
    if (!frame_raw32_region_resolve(&state->region, address, 4u, &resolved))
        return 0;
    *value = (renderer_raw32)resolved[0] |
             ((renderer_raw32)resolved[1] << 8) |
             ((renderer_raw32)resolved[2] << 16) |
             ((renderer_raw32)resolved[3] << 24);
    return 1;
}

static int test_probe_resolver(void *context,
                               renderer_raw32 address,
                               renderer_u32 byte_count,
                               const renderer_u8 **resolved_bytes)
{
    FrameResolverProbe *probe = (FrameResolverProbe *)context;
    if (probe == 0 || probe->region == 0)
        return 0;
    if (address == probe->forbidden_address)
        ++probe->forbidden_queries;
    return frame_raw32_region_resolve(probe->region, address, byte_count,
                                      resolved_bytes);
}

static int seed_forward_slots(FrameTestState *state,
                              renderer_raw32 first,
                              renderer_u32 count)
{
    renderer_u32 i;
    renderer_raw32 value;
    if (count == 0)
        return 0;
    for (i = 0; i < count; ++i) {
        value = (i + 1u < count) ? first + (i + 1u) * 4u : 0;
        if (!test_store32(state, first + i * 4u, value))
            return 0;
    }
    return 1;
}

static int initialize_region(FrameTestState *state)
{
    memset(&state->memory, 0xa5, sizeof(state->memory));
    state->writable_bytes = (renderer_u8 *)&state->memory;
    state->region.size = (renderer_u32)sizeof(state->memory);
    state->region.bytes = state->writable_bytes;
#if defined(FRAME_TEST_ACCEPTED_WRITER)
    /* The accepted writer stores live x86 pointers as raw32 values. */
    state->region.base = (renderer_raw32)(unsigned long)&state->memory;
#else
    /* Portable tests map a synthetic raw32 range onto host-owned bytes. */
    state->region.base = (renderer_raw32)0x20000000u;
#endif
    if (state->region.size > (renderer_u32)0xffffffffu - state->region.base)
        return 0;
    frame_visit_guard_init(&state->guard, state->visits,
                           TEST_WALK_CAPACITY);
    return 1;
}

static int initialize_link_tables(FrameTestState *state,
                                  renderer_raw32 eng1_frame0,
                                  renderer_raw32 eng1_frame1,
                                  renderer_raw32 eng3_frame0,
                                  renderer_raw32 eng3_frame1,
                                  renderer_raw32 title_ot0,
                                  renderer_raw32 title_ot1)
{
#if defined(FRAME_TEST_ACCEPTED_WRITER)
    (void)eng1_frame0;
    (void)eng1_frame1;
    (void)eng3_frame0;
    (void)eng3_frame1;
    (void)title_ot0;
    (void)title_ot1;
    frame_chain_write(state->memory.eng1_frames.observed_views[0].chain_link_slots,
                      (unsigned long)ENG1_FRAME_LINK_COUNT);
    frame_chain_write(state->memory.eng1_frames.observed_views[1].chain_link_slots,
                      (unsigned long)ENG1_FRAME_LINK_COUNT);
    frame_chain_write(state->memory.eng3_frames.observed_views[0].order_table_slots,
                      (unsigned long)ENG3_FRAME_ORDER_TABLE_COUNT);
    frame_chain_write(state->memory.eng3_frames.observed_views[1].order_table_slots,
                      (unsigned long)ENG3_FRAME_ORDER_TABLE_COUNT);
    frame_chain_write(state->memory.title_buffers.observed_buffers[0].order_table_head_slots,
                      (unsigned long)TITLE_ORDER_TABLE_COUNT);
    frame_chain_write(state->memory.title_buffers.observed_buffers[1].order_table_head_slots,
                      (unsigned long)TITLE_ORDER_TABLE_COUNT);
    return 1;
#else
    return seed_forward_slots(state, eng1_frame0, ENG1_FRAME_LINK_COUNT) &&
           seed_forward_slots(state, eng1_frame1, ENG1_FRAME_LINK_COUNT) &&
           seed_forward_slots(state, eng3_frame0, ENG3_FRAME_ORDER_TABLE_COUNT) &&
           seed_forward_slots(state, eng3_frame1, ENG3_FRAME_ORDER_TABLE_COUNT) &&
           seed_forward_slots(state, title_ot0, TITLE_ORDER_TABLE_COUNT) &&
           seed_forward_slots(state, title_ot1, TITLE_ORDER_TABLE_COUNT);
#endif
}

int main(void)
{
    FrameTestState state;
    renderer_raw32 frame0, frame1, eng3_frame0, eng3_frame1;
    renderer_raw32 title_ot0, title_ot1;
    renderer_raw32 packet0, packet1, packet2, packet3;
    renderer_raw32 bucket, old_head = 0, terminator;
    renderer_raw32 invalid_last;
    FrameResolverProbe resolver_probe;
    const renderer_u8 *mapped_tail;
    FrameWalkStatus status;
    int ok = 1;

    memset(&state, 0, sizeof(state));
    memset(&resolver_probe, 0, sizeof(resolver_probe));
    ok &= expect(&state, initialize_region(&state),
                 "fixed-width raw32 region maps without host pointer truncation");
    if (state.region.bytes == 0) {
        printf("checks=%u failures=%u\n", (unsigned int)state.check_count,
               (unsigned int)(state.failure_count + 1u));
        return 1;
    }

    frame0 = state.region.base + (renderer_u32)offsetof(FrameFixtureMemory, eng1_frames) +
             ENG1_FRAME_LINK_OFFSET;
    frame1 = frame0 + ENG1_FRAME_VIEW_STRIDE;
    eng3_frame0 = state.region.base +
                  (renderer_u32)offsetof(FrameFixtureMemory, eng3_frames) +
                  ENG3_FRAME_ORDER_TABLE_OFFSET;
    eng3_frame1 = eng3_frame0 + ENG3_FRAME_VIEW_STRIDE;
    title_ot0 = state.region.base +
                (renderer_u32)offsetof(FrameFixtureMemory, title_buffers) +
                TITLE_ORDER_TABLE_OFFSET;
    title_ot1 = title_ot0 + TITLE_BUFFER_STRIDE;
    packet0 = state.region.base +
              (renderer_u32)offsetof(FrameFixtureMemory, packet_links);
    packet1 = packet0 + 4u;
    packet2 = packet0 + 8u;
    packet3 = packet0 + 12u;

    ok &= expect(&state, initialize_link_tables(&state, frame0, frame1,
                                                eng3_frame0, eng3_frame1,
                                                title_ot0, title_ot1),
                 "both module-specific frame/table pairs initialize");
    ok &= expect(&state, test_store32(&state, packet2, 0),
                 "NULL chain head seeded");
    status = frame_walk_raw32_links(frame_raw32_region_resolve, &state.region,
                                    0, &state.guard, &terminator);
    ok &= expect(&state, status == FRAME_WALK_OK && state.guard.count == 0 &&
                 terminator == 0,
                 "empty raw32 head terminates at NULL without visits");

    ok &= expect(&state, test_store32(&state, packet2, packet3) &&
                 test_store32(&state, packet3, 0),
                 "terminal-NULL two-link chain seeded");
    status = frame_walk_raw32_links(frame_raw32_region_resolve, &state.region,
                                    packet2, &state.guard, &terminator);
    ok &= expect(&state, status == FRAME_WALK_OK && state.guard.count == 2 &&
                 state.visits[0] == packet2 && state.visits[1] == packet3 &&
                 terminator == 0,
                 "bounded raw32 walker accepts final NULL link");

    invalid_last = state.region.base + state.region.size + 4u;
    ok &= expect(&state, test_store32(&state, packet2, packet3) &&
                 test_store32(&state, packet3, invalid_last),
                 "invalid last link seeded outside mapped range");
    status = frame_walk_raw32_links(frame_raw32_region_resolve, &state.region,
                                    packet2, &state.guard, &terminator);
    ok &= expect(&state, status == FRAME_WALK_UNMAPPED_ADDRESS &&
                 state.guard.count == 2,
                 "bounded raw32 walker rejects invalid last link");

    ok &= expect(&state, test_store32(&state, packet2, packet3) &&
                 test_store32(&state, packet3, packet2),
                 "cycle links seeded");
    status = frame_walk_raw32_links(frame_raw32_region_resolve, &state.region,
                                    packet2, &state.guard, &terminator);
    ok &= expect(&state, status == FRAME_WALK_CYCLE,
                 "raw32 walker rejects cycle through visit guard");
    frame_visit_guard_init(&state.guard, state.visits, 1u);
    status = frame_walk_raw32_links(frame_raw32_region_resolve, &state.region,
                                    packet2, &state.guard, &terminator);
    ok &= expect(&state, status == FRAME_WALK_VISIT_LIMIT,
                 "raw32 walker applies caller visit budget");
    frame_visit_guard_init(&state.guard, state.visits, TEST_WALK_CAPACITY);

    ok &= expect(&state, frame1 - frame0 == ENG1_FRAME_VIEW_STRIDE &&
                 title_ot1 - title_ot0 == TITLE_BUFFER_STRIDE,
                 "ENG1 and TITLE retain distinct observed strides");
    status = frame_walk_raw32_links(frame_raw32_region_resolve, &state.region,
                                    frame0, &state.guard, &terminator);
    ok &= expect(&state, status == FRAME_WALK_OK &&
                 state.guard.count == ENG1_FRAME_LINK_COUNT && terminator == 0,
                 "first ENG1 view has 0x500 links and terminal NULL");
    status = frame_walk_raw32_links(frame_raw32_region_resolve, &state.region,
                                    frame1, &state.guard, &terminator);
    ok &= expect(&state, status == FRAME_WALK_OK &&
                 state.guard.count == ENG1_FRAME_LINK_COUNT && terminator == 0,
                 "second ENG1 view has 0x500 links and terminal NULL");
    ok &= expect(&state, eng3_frame1 - eng3_frame0 ==
                 ENG3_FRAME_VIEW_STRIDE &&
                 ENG3_FRAME_PAIR_REQUEST == 2u * ENG3_FRAME_VIEW_STRIDE,
                 "ENG3 retains its independent two-view stride/request");
    status = frame_walk_raw32_links(frame_raw32_region_resolve, &state.region,
                                    eng3_frame0, &state.guard, &terminator);
    ok &= expect(&state, status == FRAME_WALK_OK &&
                 state.guard.count == ENG3_FRAME_ORDER_TABLE_COUNT &&
                 terminator == 0,
                 "first ENG3 view has 0x800 chain slots and terminal NULL");
    status = frame_walk_raw32_links(frame_raw32_region_resolve, &state.region,
                                    eng3_frame1, &state.guard, &terminator);
    ok &= expect(&state, status == FRAME_WALK_OK &&
                 state.guard.count == ENG3_FRAME_ORDER_TABLE_COUNT &&
                 terminator == 0,
                 "second ENG3 view has 0x800 chain slots and terminal NULL");
    status = frame_walk_ac60_order_table(frame_raw32_region_resolve,
                                         &state.region, eng3_frame0,
                                         ENG3_FRAME_ORDER_TABLE_COUNT,
                                         &state.guard);
    ok &= expect(&state, status == FRAME_WALK_OK && state.guard.count == 0,
                 "AC60 model traverses an empty ENG3 0x800-slot table to NULL");
    status = frame_walk_raw32_links(frame_raw32_region_resolve, &state.region,
                                    title_ot0, &state.guard, &terminator);
    ok &= expect(&state, status == FRAME_WALK_OK &&
                 state.guard.count == TITLE_ORDER_TABLE_COUNT && terminator == 0,
                 "first TITLE table has 0x500 heads and terminal NULL");
    status = frame_walk_raw32_links(frame_raw32_region_resolve, &state.region,
                                    title_ot1, &state.guard, &terminator);
    ok &= expect(&state, status == FRAME_WALK_OK &&
                 state.guard.count == TITLE_ORDER_TABLE_COUNT && terminator == 0,
                 "second TITLE table has 0x500 heads and terminal NULL");

    bucket = TITLE_ORDER_TABLE_COUNT - 2u - 7u;
    ok &= expect(&state, bucket == 0x4f7u,
                 "TITLE source depth input 7 selects OT index 0x4f7");
    ok &= expect(&state, test_read32(&state, title_ot0 + bucket * 4u,
                                     &old_head),
                 "selected bucket head resolves");
    ok &= expect(&state, test_store32(&state, packet0, old_head) &&
                 test_store32(&state, title_ot0 + bucket * 4u, packet0),
                 "first primitive prepends ahead of existing sentinel");
    ok &= expect(&state, test_read32(&state, title_ot0 + bucket * 4u,
                                     &old_head),
                 "bucket head reloads before second prepend");
    ok &= expect(&state, test_store32(&state, packet1, old_head) &&
                 test_store32(&state, title_ot0 + bucket * 4u, packet1),
                 "second primitive prepends in LIFO order");
    status = frame_walk_ac60_order_table(frame_raw32_region_resolve,
                                         &state.region, title_ot0,
                                         TITLE_ORDER_TABLE_COUNT,
                                         &state.guard);
    ok &= expect(&state, status == FRAME_WALK_OK && state.guard.count == 2 &&
                 state.visits[0] == packet1 && state.visits[1] == packet0,
                 "AC60 model skips sentinels and visits newest first");

    ok &= expect(&state, seed_forward_slots(&state, title_ot0,
                                            TITLE_ORDER_TABLE_COUNT),
                 "TITLE OT restored before invalid-tail case");
    status = frame_walk_ac60_order_table(frame_raw32_region_resolve,
                                         &state.region, title_ot0,
                                         TITLE_ORDER_TABLE_COUNT,
                                         &state.guard);
    ok &= expect(&state, status == FRAME_WALK_OK && state.guard.count == 0,
                 "empty AC60 table advances through terminal NULL");
    ok &= expect(&state, test_store32(&state,
                 title_ot0 + (TITLE_ORDER_TABLE_COUNT - 1u) * 4u,
                 invalid_last),
                 "invalid final OT head seeded");
    status = frame_walk_ac60_order_table(frame_raw32_region_resolve,
                                         &state.region, title_ot0,
                                         TITLE_ORDER_TABLE_COUNT,
                                         &state.guard);
    ok &= expect(&state, status == FRAME_WALK_UNMAPPED_ADDRESS,
                 "AC60 model rejects invalid final primitive link");

    ok &= expect(&state, seed_forward_slots(&state, title_ot0,
                                            TITLE_ORDER_TABLE_COUNT),
                 "TITLE OT restored before one-past sentinel case");
    ok &= expect(&state, test_store32(&state,
                 title_ot0 + (TITLE_ORDER_TABLE_COUNT - 1u) * 4u,
                 title_ot0 + TITLE_ORDER_TABLE_COUNT * 4u),
                 "one-past final OT sentinel seeded in mapped tail bytes");
    resolver_probe.region = &state.region;
    resolver_probe.forbidden_address =
        title_ot0 + TITLE_ORDER_TABLE_COUNT * 4u;
    ok &= expect(&state, frame_raw32_region_resolve(
                 &state.region, resolver_probe.forbidden_address, 4u,
                 &mapped_tail),
                 "malformed one-past address is deliberately still mapped");
    status = frame_walk_ac60_order_table(test_probe_resolver,
                                         &resolver_probe, title_ot0,
                                         TITLE_ORDER_TABLE_COUNT,
                                         &state.guard);
    ok &= expect(&state, status == FRAME_WALK_RANGE_ERROR &&
                 resolver_probe.forbidden_queries == 0,
                 "AC60 decoder rejects one-past sentinel before resolver lookup");

    frame_visit_guard_init(&state.guard, state.visits, 8u);
    ok &= expect(&state, seed_forward_slots(&state, title_ot0,
                                            TITLE_ORDER_TABLE_COUNT),
                 "empty OT restored before sentinel-budget case");
    status = frame_walk_ac60_order_table(frame_raw32_region_resolve,
                                         &state.region, title_ot0,
                                         TITLE_ORDER_TABLE_COUNT,
                                         &state.guard);
    ok &= expect(&state, status == FRAME_WALK_VISIT_LIMIT,
                 "AC60 model bounds empty-bucket sentinel advances");

    printf("checks=%u failures=%u\n", (unsigned int)state.check_count,
           (unsigned int)state.failure_count);
    return (ok && state.failure_count == 0) ? 0 : 1;
}
