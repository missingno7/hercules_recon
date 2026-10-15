#ifndef HERCULES_SHIM_H
#define HERCULES_SHIM_H

/* Experimental x86 host-interface shim (not reconstruction source).
 *
 * The shim publishes a clone of the 250-DWORD host->engine table in which an
 * allowlisted set of slots points at a generic cdecl relay. The relay observes
 * the call, forwards the original stack window and ECX/EDX unchanged to the
 * original host callback, and returns the callback's EAX/EDX unchanged.
 * A backend receives backend-neutral events built from the shared
 * renderer_contract decoders. Backend code runs behind a fault firewall:
 * a backend fault disables the backend and never reaches the host.
 *
 * Only forward-only and observe modes exist. Replacing AC60's raster/present
 * work is deliberately unimplemented; see README.md for the side effects a
 * replacement must still perform. */

#include "hercules_host_abi.h"
#include "hercules_packets.h"
#include "hercules_staging.h"

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error "hercules_shim is an MSVC x86 experiment (naked relay and SEH)"
#endif

#ifdef HERCULES_SHIM_BUILD
#define HS_API __declspec(dllexport)
#else
#define HS_API __declspec(dllimport)
#endif
#define HS_CALL __cdecl

#define HS_ABI_VERSION 1u
#define HS_RELAY_STACK_WORDS 8u

#ifdef __cplusplus
extern "C" {
#endif

typedef enum HsMode {
    HS_MODE_FORWARD_ONLY = 0, /* relay only: no events, no pre-walk */
    HS_MODE_OBSERVE = 1       /* relay + events + slot-89 pre-walk */
} HsMode;

typedef enum HsInstallStatus {
    HS_INSTALLED = 0,
    HS_INVALID_ARGUMENT = 1,
    HS_SOURCE_POINTER_MISMATCH = 2,
    HS_BAD_SIGNATURE = 3,
    HS_NULL_CALLBACK = 4,
    HS_CLONE_REJECTED = 5,
    HS_ALREADY_INSTALLED = 6,
    HS_ABI_MISMATCH = 7
} HsInstallStatus;

typedef enum HsPrewalkSkip {
    HS_PREWALK_DONE = 0,
    HS_PREWALK_SKIP_MODE = 1,           /* forward-only or backend disabled */
    HS_PREWALK_SKIP_PRIMARY_CALLBACK = 2,/* BD80 primary target non-null: AC60 may run foreign code */
    HS_PREWALK_SKIP_UNKNOWN_EXTENT = 3, /* no slot-81 extent recorded for this order table */
    HS_PREWALK_SKIP_NESTED = 4,
    HS_PREWALK_SKIP_WALK_FAULT = 5,     /* the shim's read faulted; original runs unchanged */
    HS_PREWALK_SKIP_WALK_ERROR = 6      /* walker status != OK (cycle, visit limit, ...) */
} HsPrewalkSkip;

typedef struct HsServiceEvent {
    renderer_u32 sequence;
    renderer_u32 thread_id;
    renderer_u32 slot;
    renderer_u32 phase;          /* 0 enter, 1 return */
    renderer_u32 stack_words[HS_RELAY_STACK_WORDS]; /* raw window; arity is the callee's */
    renderer_u32 raw_eax;        /* return phase only */
    renderer_u32 raw_edx;        /* return phase only */
    renderer_u32 target;         /* original host callback word */
} HsServiceEvent;

typedef struct HsFrameBegin {
    renderer_u32 frame;
    renderer_u32 thread_id;
    renderer_raw32 first_slot;
    renderer_u32 slot_count;     /* from the slot-81 call that linked this table */
} HsFrameBegin;

typedef struct HsPrimitive {
    renderer_u32 frame;
    renderer_u32 index;
    renderer_raw32 node;
    RendererPacketStatus decode_status;
    RendererDecodedPacket decoded;
    RendererAc60ApplyStatus apply_status;
    RendererAc60Event staged;
    RendererAc60StateView state_after; /* shim-owned model, not the host cells */
} HsPrimitive;

typedef struct HsFrameEnd {
    renderer_u32 frame;
    renderer_u32 primitives;
    renderer_u32 prewalk;        /* HsPrewalkSkip */
    renderer_u32 consumption_consistent; /* post-call re-walk equals pre-walk */
} HsFrameEnd;

/* All callbacks are synchronous, on the host thread that called the slot,
 * inside the original call bracket. Records are borrowed for the call only. */
typedef struct HsBackendV1 {
    renderer_u32 struct_size;
    renderer_u32 abi_version;
    void *context;
    void (HS_CALL *service)(void *context, const HsServiceEvent *event);
    void (HS_CALL *frame_begin)(void *context, const HsFrameBegin *frame);
    void (HS_CALL *primitive)(void *context, const HsPrimitive *primitive);
    void (HS_CALL *frame_end)(void *context, const HsFrameEnd *frame);
} HsBackendV1;

typedef struct HsConfig {
    renderer_u32 struct_size;
    renderer_u32 mode;           /* HsMode */
    /* Address of the BD80 primary callback cell (host A8E780 on the pinned
     * EXE). NULL means unknown, which disables the pre-walk. */
    const volatile renderer_u32 *primary_callback_cell;
    renderer_u32 visit_capacity; /* pre-walk visit guard; 0 selects 0x4000 */
} HsConfig;

typedef struct HsStats {
    renderer_u32 struct_size;
    renderer_u32 service_calls;
    renderer_u32 frames;
    renderer_u32 primitives;
    renderer_u32 backend_faults;
    renderer_u32 last_fault_code;
    renderer_u32 backend_disabled;
    renderer_u32 prewalk_skips[7];
    renderer_u32 consumption_mismatches;
    renderer_u32 fpu_restores;   /* backend changed x87 CW or MXCSR */
    renderer_u32 abandoned_brackets; /* slot-89 original unwound by an exception */
} HsStats;

/* Slots the shim relays (subset of the renderer_contract clone allowlist). */
HS_API renderer_u32 HS_CALL hs_relayed_slots(const renderer_u32 **slots);

HS_API HsInstallStatus HS_CALL hs_install(const renderer_u32 *source_words,
                                          renderer_u32 *host_pointer_cell,
                                          const HsConfig *config,
                                          const HsBackendV1 *backend);

/* Swap the backend and mode between frames (never inside a slot-89 bracket).
 * The published table and relay stay fixed, so gameplay-visible calls are
 * identical for every backend and for none. Re-enables a disabled backend. */
HS_API HsInstallStatus HS_CALL hs_select_backend(const HsBackendV1 *backend,
                                                 renderer_u32 mode);

HS_API void HS_CALL hs_get_stats(HsStats *stats);

/* Test/diagnostic hook: the relay stub address published for one slot. */
HS_API renderer_u32 HS_CALL hs_relay_address(renderer_u32 slot);

#ifdef __cplusplus
}
#endif

#endif
