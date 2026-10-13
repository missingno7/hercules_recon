#ifndef HERCULES_POSTENTRY_SHARED_H
#define HERCULES_POSTENTRY_SHARED_H

#include <windows.h>

#include "presentation_probe.h"

#define POSTENTRY_HOST_MAGIC 0x504f5354UL
#define POSTENTRY_ENGINE_MAGIC 0x50454e47UL

#define POSTENTRY_HOST_STATE_INITIAL 0UL
#define POSTENTRY_HOST_STATE_PROXY_READY 1UL
#define POSTENTRY_HOST_STATE_TABLE_PUBLISHED 2UL
#define POSTENTRY_HOST_STATE_ENGINE_LOADED 3UL
#define POSTENTRY_HOST_STATE_RENDER_DONE 4UL
#define POSTENTRY_HOST_STATE_BAD_SIGNATURE 0xe001UL
#define POSTENTRY_HOST_STATE_FAILURE 0xe0ffUL

typedef struct SyntheticHostStateV1 {
    renderer_u32 magic;
    renderer_u32 state;
    renderer_u32 publish_status;
    renderer_u32 pointer_cell;
    renderer_u32 probe_state_pointer;
    renderer_u32 candidate_base;
    renderer_u32 engine_base;
    renderer_u32 worker_thread_id;
    renderer_u32 callback81_calls;
    renderer_u32 callback89_calls;
    renderer_u32 callback81_first;
    renderer_u32 callback81_count;
    renderer_u32 callback81_return;
    renderer_u32 callback89_first;
    renderer_u32 callback89_raw_eax;
    renderer_u32 worker_return;
    renderer_u32 source_table[ENGINE_TABLE_WORD_COUNT];
    renderer_u32 clone_table[ENGINE_TABLE_WORD_COUNT];
} SyntheticHostStateV1;

typedef struct SyntheticEngineStateV1 {
    renderer_u32 magic;
    renderer_u32 copy_count;
    renderer_u32 render_complete;
    renderer_u32 after_copy_hits;
    renderer_u32 after_warm_copy_hits;
    renderer_u32 after_render_hits;
    renderer_u32 slot81_result;
    renderer_u32 callback_thread_id;
    renderer_u32 local_table[ENGINE_TABLE_WORD_COUNT];
} SyntheticEngineStateV1;

typedef struct SyntheticEngineInvocationV1 {
    const renderer_u32 *table;
    renderer_u32 warm_copy;
    renderer_u32 hang_after_copy;
} SyntheticEngineInvocationV1;

#endif
