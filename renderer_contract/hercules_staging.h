#ifndef RENDERER_AC60_STAGING_PROPOSAL_H
#define RENDERER_AC60_STAGING_PROPOSAL_H

#include <stddef.h>
#include "hercules_packets.h"
#include "hercules_state.h"

/* Minimal named view of the AC60 staging cells observed from the PC binary.
 * Coordinates are raw 32-bit sign-extended native words. Polygon UV words are
 * unsigned bytes shifted left 16 bits; sprite UV cells retain raw byte values.
 * Slot numbering describes these
 * PC workspace lanes only; it makes no PSX or GPU vertex-order claim. */
typedef struct RendererAc60WorkspaceView {
    renderer_u32 clip_left;             /* A909B0 */
    renderer_u32 clip_top;              /* A909B4 */
    renderer_u32 native_x_slot0;        /* A909B8 */
    renderer_u32 native_x_slot1;        /* A909BC */
    renderer_u32 native_x_slot2;        /* A909C0 */
    renderer_u32 native_x_slot3;        /* A909C4 */
    renderer_u32 native_y_slot0;        /* A909C8 */
    renderer_u32 native_y_slot1;        /* A909CC */
    renderer_u32 native_y_slot2;        /* A909D0 */
    renderer_u32 native_y_slot3;        /* A909D4 */
    renderer_u32 native_u_slot0;        /* A909D8 */
    renderer_u32 native_u_slot1;        /* A909DC */
    renderer_u32 native_u_slot2;        /* A909E0 */
    renderer_u32 native_u_slot3;        /* A909E4 */
    renderer_u32 native_v_slot0;        /* A909E8 */
    renderer_u32 native_v_slot1;        /* A909EC */
    renderer_u32 native_v_slot2;        /* A909F0 */
    renderer_u32 native_v_slot3;        /* A909F4; FT3 U2 overwrite observed */
    renderer_u32 vertex_count;          /* A909F8 */
    renderer_raw32 texture_page_address;/* A909FC; raw 32-bit address */
    renderer_u32 color0_red;            /* A90A00; zero-extended R byte */
    renderer_u32 clut_word;             /* A90A04; native DWORD store */
    renderer_u16 sprite_width;          /* A90A0C; native WORD store */
    renderer_u16 sprite_height;         /* A90A10; native WORD store */
} RendererAc60WorkspaceView;

typedef struct RendererAc60PageCells {
    renderer_u32 page_selector;         /* A8E6F4 */
    renderer_u32 page_mode;             /* A8E6F8 */
    renderer_u32 alternate_bank;        /* A8E6FC */
} RendererAc60PageCells;

typedef struct RendererAc60StateView {
    RendererAc60WorkspaceView workspace;
    RendererAc60PageCells page;
    HerculesF3ClipBounds clip_bounds;   /* right/bottom cells outside A909 range */
    renderer_u8 raster_suppression_flag; /* BA01FC; native byte read gates 0x20 and 0x60 groups */
    renderer_u32 dispatch_sequence;
} RendererAc60StateView;

typedef enum RendererAc60EventKind {
    RENDERER_AC60_EVENT_NONE = 0,
    RENDERER_AC60_EVENT_FT3 = 1,
    RENDERER_AC60_EVENT_FT4 = 2,
    RENDERER_AC60_EVENT_GT4 = 3,
    RENDERER_AC60_EVENT_VARIABLE_SPRITE = 4,
    RENDERER_AC60_EVENT_SPRITE_8 = 5,
    RENDERER_AC60_EVENT_SPRITE_16 = 6,
    RENDERER_AC60_EVENT_E1_STATE = 7,
    RENDERER_AC60_EVENT_F3_CLIP = 8,
    RENDERER_AC60_EVENT_DEFERRED_COPY = 9,
    RENDERER_AC60_EVENT_OT_GATE = 10,
    RENDERER_AC60_EVENT_IGNORED_RASTER = 11,
    RENDERER_AC60_EVENT_RASTER_SUPPRESSED = 12
} RendererAc60EventKind;

typedef enum RendererAc60ApplyStatus {
    RENDERER_AC60_APPLY_OK = 0,
    RENDERER_AC60_APPLY_DEFERRED = 1,
    RENDERER_AC60_APPLY_UNSUPPORTED = 2,
    RENDERER_AC60_APPLY_UNRECOGNIZED = 3,
    RENDERER_AC60_APPLY_INVALID_ARGUMENT = 4,
    RENDERER_AC60_APPLY_TRUNCATED = 5,
    RENDERER_AC60_APPLY_RANGE_ERROR = 6
} RendererAc60ApplyStatus;

typedef struct RendererAc60Event {
    RendererAc60EventKind kind;
    RendererPacketKind packet_kind;
    renderer_u8 command;
    renderer_u8 semitransparent; /* command bit 1, separate from page cells */
    HerculesCommand01Move deferred_copy;
} RendererAc60Event;

#ifdef __cplusplus
extern "C" {
#endif

/* Apply one packet to a caller-owned state view. The 0x01 command produces a
 * deferred descriptor only; this helper never reads or writes VRAM bytes. */
RendererAc60ApplyStatus renderer_ac60_apply_packet(
    const renderer_u8 *wire, size_t length,
    RendererAc60StateView *state, RendererAc60Event *event);

#ifdef __cplusplus
}
#endif

#endif
