#ifndef HERCULES_STATE_PROPOSAL_H
#define HERCULES_STATE_PROPOSAL_H

#include <stddef.h>
#include "hercules_types.h"
#include "hercules_packets.h"
#include "hercules_state_wire.h"

/* Private, pure wire-decode proposal. No native pointers or global writes. */
typedef enum HerculesStateStatus {
    HERCULES_STATE_OK = 0,
    HERCULES_STATE_INVALID_ARGUMENT = 1,
    HERCULES_STATE_TRUNCATED = 2,
    HERCULES_STATE_WRONG_TAG = 3
} HerculesStateStatus;

typedef struct HerculesE1StateUpdate {
    renderer_u32 page_index;
    renderer_u32 table_selector;
    renderer_u32 alternate_bank;
} HerculesE1StateUpdate;

typedef struct HerculesF3ClipBounds {
    renderer_i32 left;
    renderer_i32 top;
    renderer_i32 right;
    renderer_i32 bottom;
} HerculesF3ClipBounds;

typedef struct HerculesCommand01Move {
    renderer_i32 destination_x;
    renderer_i32 destination_y;
    renderer_i32 source_x;
    renderer_i32 source_y;
    renderer_i32 width;
    renderer_i32 height;
} HerculesCommand01Move;

/* E1 minimum is AC60's 8-byte prefix. The extension DWORD at +8 is ignored. */
#ifdef __cplusplus
extern "C" {
#endif

HerculesStateStatus hercules_state_decode_e1(
    const renderer_u8 *wire, size_t length, HerculesE1StateUpdate *out);

/* F3 consumes its complete 12-byte inline payload. */
HerculesStateStatus hercules_state_decode_f3(
    const renderer_u8 *wire, size_t length, HerculesF3ClipBounds *out);

/* 0x01 includes the 8-byte rectangle reached by A8F0 at packet+0x10. */
HerculesStateStatus hercules_state_decode_command01(
    const renderer_u8 *wire, size_t length, HerculesCommand01Move *out);

#ifdef __cplusplus
}
#endif

#endif
