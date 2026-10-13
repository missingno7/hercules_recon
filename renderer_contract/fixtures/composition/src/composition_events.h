#ifndef HERCULES_COMPOSITION_EVENTS_H
#define HERCULES_COMPOSITION_EVENTS_H

typedef unsigned char composition_u8;
typedef unsigned short composition_u16;
typedef unsigned int composition_u32;
typedef char composition_assert_u8[(sizeof(composition_u8) == 1) ? 1 : -1];
typedef char composition_assert_u16[(sizeof(composition_u16) == 2) ? 1 : -1];
typedef char composition_assert_u32[(sizeof(composition_u32) == 4) ? 1 : -1];

typedef enum CompositionTraceStatus {
    COMPOSITION_TRACE_OK = 0,
    COMPOSITION_TRACE_INVALID = 1,
    COMPOSITION_TRACE_ORDER_ERROR = 2,
    COMPOSITION_TRACE_OVERFLOW = 3,
    COMPOSITION_TRACE_INCOMPLETE_UNKNOWN = 4
} CompositionTraceStatus;

typedef enum CompositionEventKind {
    COMPOSITION_EVENT_FRAME_ENTRY = 1,
    COMPOSITION_EVENT_GEOMETRY_COMMAND = 2,
    COMPOSITION_EVENT_SCREEN_SPACE_GRAPHIC = 3,
    COMPOSITION_EVENT_STATE_COMMAND = 4,
    COMPOSITION_EVENT_TEXTURE_MUTATION = 5,
    COMPOSITION_EVENT_PALETTE_LUT_MUTATION = 6,
    COMPOSITION_EVENT_SURFACE_CLEAR = 7,
    COMPOSITION_EVENT_SURFACE_LOCK = 8,
    COMPOSITION_EVENT_VRAM_MOVE_REQUEST = 9,
    COMPOSITION_EVENT_CALLBACK_ENTRY = 10,
    COMPOSITION_EVENT_CALLBACK_SERVICE = 11,
    COMPOSITION_EVENT_LIVE_LINK_RELOAD = 12,
    COMPOSITION_EVENT_PACKET_CONSUMED = 13,
    COMPOSITION_EVENT_SURFACE_REMAP_REQUEST = 14,
    COMPOSITION_EVENT_SURFACE_UNLOCK = 15,
    COMPOSITION_EVENT_DIRTY_REQUEST = 16,
    COMPOSITION_EVENT_PRESENT_REQUEST = 17,
    COMPOSITION_EVENT_MOVIE_FRAME_COPY = 18,
    COMPOSITION_EVENT_DESKTOP_DC_TEXT = 19,
    COMPOSITION_EVENT_FINAL_OUTPUT = 20,
    COMPOSITION_EVENT_OUTPUT_FLUSH = 21,
    COMPOSITION_EVENT_UNKNOWN_WRITER = 255
} CompositionEventKind;

typedef enum CompositionEventOrigin {
    COMPOSITION_ORIGIN_ORIGINAL_INSTRUCTION = 1,
    COMPOSITION_ORIGIN_CONTROLLED_ENDPOINT = 2,
    COMPOSITION_ORIGIN_STATIC_ORDER = 3,
    COMPOSITION_ORIGIN_TOOL_BOOKKEEPING = 4,
    COMPOSITION_ORIGIN_UNRESOLVED = 255
} CompositionEventOrigin;

typedef enum CompositionEventDomain {
    COMPOSITION_DOMAIN_NONE = 0,
    COMPOSITION_DOMAIN_ORDER_TABLE = 1,
    COMPOSITION_DOMAIN_GEOMETRY = 2,
    COMPOSITION_DOMAIN_SCREEN_SPACE = 3,
    COMPOSITION_DOMAIN_TEXTURE = 4,
    COMPOSITION_DOMAIN_PALETTE_LUT = 5,
    COMPOSITION_DOMAIN_CPU_SURFACE = 6,
    COMPOSITION_DOMAIN_DESKTOP_DC = 7,
    COMPOSITION_DOMAIN_DISPLAY = 8,
    COMPOSITION_DOMAIN_EXTERNAL = 9
} CompositionEventDomain;

typedef enum CompositionSemanticOwner {
    COMPOSITION_OWNER_UNCLASSIFIED = 0,
    COMPOSITION_OWNER_SOURCE_ASSOCIATED = 1
} CompositionSemanticOwner;

/* Flag carried by source-reconstructed events emitted by the bounded C model.
 * It distinguishes them from instruction observations in the native trace. */
#define COMPOSITION_EVENT_FLAG_C_RECONSTRUCTION 0x01U

/* Serialized fields are fixed-width values. raw_token is always an address
 * token from the x86 target, never a host pointer. This record contains no
 * pointers; only the collector below points at caller-owned storage. */
typedef struct CompositionEvent {
    composition_u32 scope_id;
    composition_u32 local_order;
    composition_u32 source_order;
    composition_u32 source_rva;
    composition_u32 raw_token;
    composition_u32 argument[4];
    composition_u16 kind;
    composition_u8 origin;
    composition_u8 domain;
    composition_u8 semantic_owner;
    composition_u8 flags;
} CompositionEvent;

typedef struct CompositionTrace {
    CompositionEvent *events;
    composition_u32 capacity;
    composition_u32 count;
    CompositionTraceStatus status;
} CompositionTrace;

void composition_trace_init(CompositionTrace *trace,
                            CompositionEvent *storage,
                            composition_u32 capacity);

/* Appends in per-scope local_order. Unknown-writer records are retained, then
 * poison the trace status so callers cannot mistake partial evidence for a
 * complete successful trace. */
CompositionTraceStatus composition_trace_append(
    CompositionTrace *trace, const CompositionEvent *event);

/* Checks the specific connected state-only AC60 -> A8F0/BD80 -> DIB-present
 * subset projected from test_original_composition_frame.py. This is not a
 * general frame-completeness or cross-route serialization claim. */
CompositionTraceStatus composition_validate_connected_frame(
    const CompositionEvent *events, composition_u32 count,
    composition_u32 scope_id);

#endif
