#ifndef RENDERER_FRAME_CONTRACT_DRAFT_H
#define RENDERER_FRAME_CONTRACT_DRAFT_H

/*
 * Separate measured PC views. These module-specific byte ranges are not a
 * claim that ENG1, ENG3 and TITLE share one original native aggregate or type.
 */
#include "hercules_types.h"

#define ENG1_FRAME_LINK_OFFSET       ((renderer_u32)0x88u)
#define ENG1_FRAME_LINK_COUNT        ((renderer_u32)0x500u)
#define ENG1_FRAME_VIEW_STRIDE       ((renderer_u32)0x1488u)
#define ENG1_FRAME_PAIR_REQUEST      ((renderer_u32)0x2910u)

#define ENG3_FRAME_ORDER_TABLE_OFFSET ((renderer_u32)0x88u)
#define ENG3_FRAME_ORDER_TABLE_COUNT  ((renderer_u32)0x800u)
#define ENG3_FRAME_VIEW_STRIDE        ((renderer_u32)0x2088u)
#define ENG3_FRAME_PAIR_REQUEST       ((renderer_u32)0x4110u)

#define TITLE_ORDER_TABLE_OFFSET     ((renderer_u32)0x70u)
#define TITLE_ORDER_TABLE_COUNT      ((renderer_u32)0x500u)
#define TITLE_BUFFER_STRIDE          ((renderer_u32)0x1478u)
#define TITLE_BUFFER_PAIR_BYTES      ((renderer_u32)0x28f0u)

typedef struct Eng1FrameViewDraft {
    renderer_u8 opaque_prefix_000_087[ENG1_FRAME_LINK_OFFSET];
    renderer_raw32 chain_link_slots[ENG1_FRAME_LINK_COUNT];
} Eng1FrameViewDraft;

typedef struct Eng1FramePairViewDraft {
    Eng1FrameViewDraft observed_views[2];
} Eng1FramePairViewDraft;

typedef struct Eng3FrameViewDraft {
    renderer_u8 opaque_prefix_000_087[ENG3_FRAME_ORDER_TABLE_OFFSET];
    renderer_raw32 order_table_slots[ENG3_FRAME_ORDER_TABLE_COUNT];
} Eng3FrameViewDraft;

typedef struct Eng3FramePairViewDraft {
    Eng3FrameViewDraft observed_views[2];
} Eng3FramePairViewDraft;

typedef struct TitleBufferViewDraft {
    renderer_u8 opaque_prefix_000_06f[TITLE_ORDER_TABLE_OFFSET];
    renderer_raw32 order_table_head_slots[TITLE_ORDER_TABLE_COUNT];
    renderer_u8 opaque_tail_1470_1477[8];
} TitleBufferViewDraft;

typedef struct TitleBufferPairViewDraft {
    TitleBufferViewDraft observed_buffers[2];
} TitleBufferPairViewDraft;

/* These fixed-width checks hold on 32-bit and 64-bit hosts. */
typedef char eng1_frame_view_size_check[
    (sizeof(Eng1FrameViewDraft) == ENG1_FRAME_VIEW_STRIDE) ? 1 : -1];
typedef char eng1_frame_pair_size_check[
    (sizeof(Eng1FramePairViewDraft) == ENG1_FRAME_PAIR_REQUEST) ? 1 : -1];
typedef char eng3_frame_view_size_check[
    (sizeof(Eng3FrameViewDraft) == ENG3_FRAME_VIEW_STRIDE) ? 1 : -1];
typedef char eng3_frame_pair_size_check[
    (sizeof(Eng3FramePairViewDraft) == ENG3_FRAME_PAIR_REQUEST) ? 1 : -1];
typedef char title_buffer_view_size_check[
    (sizeof(TitleBufferViewDraft) == TITLE_BUFFER_STRIDE) ? 1 : -1];
typedef char title_buffer_pair_size_check[
    (sizeof(TitleBufferPairViewDraft) == TITLE_BUFFER_PAIR_BYTES) ? 1 : -1];

#endif

#ifndef RENDERER_FRAME_CHAIN_WALKER_H
#define RENDERER_FRAME_CHAIN_WALKER_H

#include "hercules_types.h"

/* A resolver maps raw32 tokens to readable host bytes. It must not cast a
 * token to a native pointer. The walker's own visit guard bounds decoding. */
typedef int (*FrameRaw32Resolver)(void *context,
                                  renderer_raw32 address,
                                  renderer_u32 byte_count,
                                  const renderer_u8 **resolved_bytes);

typedef struct FrameVisitGuard {
    renderer_raw32 *visited_addresses;
    renderer_u32 capacity;
    renderer_u32 count;
} FrameVisitGuard;

typedef enum FrameWalkStatus {
    FRAME_WALK_OK = 0,
    FRAME_WALK_INVALID_ARGUMENT = 1,
    FRAME_WALK_UNMAPPED_ADDRESS = 2,
    FRAME_WALK_CYCLE = 3,
    FRAME_WALK_VISIT_LIMIT = 4,
    FRAME_WALK_ADDRESS_OVERFLOW = 5,
    FRAME_WALK_RANGE_ERROR = 6
} FrameWalkStatus;

/* Optional flat mapped region resolver useful for bounded tools and tests. */
typedef struct FrameRaw32Region {
    renderer_raw32 base;
    renderer_u32 size;
    const renderer_u8 *bytes;
} FrameRaw32Region;

#ifdef __cplusplus
extern "C" {
#endif

void frame_visit_guard_init(FrameVisitGuard *guard,
                            renderer_raw32 *visited_addresses,
                            renderer_u32 capacity);

int frame_raw32_region_resolve(void *context,
                               renderer_raw32 address,
                               renderer_u32 byte_count,
                               const renderer_u8 **resolved_bytes);

/* Follow first-DWORD links until raw32 NULL. The terminator is returned. */
FrameWalkStatus frame_walk_raw32_links(FrameRaw32Resolver resolver,
                                       void *resolver_context,
                                       renderer_raw32 head,
                                       FrameVisitGuard *guard,
                                       renderer_raw32 *terminator);

/* Model AC60's ascending order-table cursor and next-slot sentinel. The
 * explicit slot_count is a decoder boundary, not a count used by AC60 itself.
 * Visited addresses contain primitive nodes only; NULL is the native end
 * condition. Guard capacity bounds visits and sentinel-advance steps. */
FrameWalkStatus frame_walk_ac60_order_table(FrameRaw32Resolver resolver,
                                            void *resolver_context,
                                            renderer_raw32 order_table_base,
                                            renderer_u32 slot_count,
                                            FrameVisitGuard *guard);

#ifdef __cplusplus
}
#endif

#endif
