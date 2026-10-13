#ifndef HERCULES_PRESENTATION_PROBE_H
#define HERCULES_PRESENTATION_PROBE_H

#include <windows.h>

#include "hercules_host_abi.h"

#define PRESENTATION_PROBE_MAGIC 0x50525031UL
#define PRESENTATION_TRACE_CAPACITY 1024UL

#define PRESENTATION_STATE_UNCHECKED 0UL
#define PRESENTATION_STATE_OTHER_IMAGE 1UL
#define PRESENTATION_STATE_PUBLISHED 2UL
#define PRESENTATION_STATE_LAYOUT_MISMATCH 0xE101UL
#define PRESENTATION_STATE_TABLE_MISMATCH 0xE102UL
#define PRESENTATION_STATE_CLONE_FAILED 0xE103UL

#define PRESENTATION_FORWARD_UNRESOLVED 0UL
#define PRESENTATION_FORWARD_READY 1UL
#define PRESENTATION_FORWARD_PATH_FAILED 2UL
#define PRESENTATION_FORWARD_LOAD_FAILED 3UL
#define PRESENTATION_FORWARD_SELF_MODULE 4UL
#define PRESENTATION_FORWARD_EXPORT_FAILED 5UL
#define PRESENTATION_FORWARD_SELF_EXPORT 6UL
#define PRESENTATION_FORWARD_PATH_UNAVAILABLE 7UL

#define PRESENTATION_EVENT_CHAIN_INIT 1UL
#define PRESENTATION_EVENT_SUBMIT 2UL
#define PRESENTATION_EVENT_DDRAW_ENTER 3UL
#define PRESENTATION_EVENT_DDRAW_RETURN 4UL
#define PRESENTATION_EVENT_CHAIN_INIT_RETURN 5UL
#define PRESENTATION_EVENT_SUBMIT_RETURN 6UL

#define PRESENTATION_EVENT_COPY_PUBLISHED 7UL

typedef struct PresentationTraceEventV1 {
    renderer_u32 sequence;
    volatile LONG committed_sequence;
    renderer_u32 kind;
    renderer_u32 thread_id;
    renderer_u32 active_depth;
    renderer_u32 arg0;
    renderer_u32 arg1;
    renderer_u32 arg2;
    renderer_u32 target;
} PresentationTraceEventV1;

typedef struct PresentationProbeV1 {
    renderer_u32 magic;
    renderer_u32 state;
    renderer_u32 process_attach_calls;
    renderer_u32 host_base;
    renderer_u32 source_pointer_cell;
    renderer_u32 source_pointer_before;
    renderer_u32 source_pointer_after;
    renderer_u32 clone_pointer;
    renderer_u32 original_chain_init;
    renderer_u32 original_submit;
    renderer_u32 directdrawcreate_calls;
    renderer_u32 real_export;
    renderer_u32 trace_count;
    renderer_u32 trace_dropped;
    renderer_u32 slot81_entered;
    renderer_u32 slot81_completed;
    renderer_u32 slot89_entered;
    renderer_u32 slot89_completed;
    renderer_u32 active_wrapper_calls;
    renderer_u32 max_active_wrapper_calls;
    PresentationTraceEventV1 trace[PRESENTATION_TRACE_CAPACITY];
} PresentationProbeV1;

typedef struct PresentationForwardInfoV1 {
    renderer_u32 magic;
    renderer_u32 state;
    renderer_u32 module;
    renderer_u32 export_address;
    renderer_u32 win32_error;
    renderer_u32 machine;
    renderer_u32 timestamp;
    renderer_u32 image_size;
    char requested_path[MAX_PATH];
    char module_path[MAX_PATH];
} PresentationForwardInfoV1;

#if defined(_MSC_VER)
#define PRESENTATION_CDECL __cdecl
#else
#define PRESENTATION_CDECL __attribute__((cdecl))
#endif

typedef int (PRESENTATION_CDECL *PresentationChainInitFn)(
    void *first_slot, renderer_u32 count);
typedef void (PRESENTATION_CDECL *PresentationSubmitFn)(void *first_slot);
typedef HRESULT (WINAPI *PresentationDirectDrawCreateFn)(
    void *guid, void **out_object, void *outer);

__declspec(dllexport) PresentationProbeV1 *WINAPI ProbeGetState(void);
__declspec(dllexport) PresentationForwardInfoV1 *WINAPI ProbeGetForwardInfo(void);
__declspec(dllexport) BOOL WINAPI ProbeResolveForwarder(void);
__declspec(dllexport) renderer_u32 WINAPI ProbeBuildSyntheticClone(
    const renderer_u32 *source, renderer_u32 *output);
__declspec(dllexport) renderer_u32 WINAPI ProbePublishSyntheticTable(
    const renderer_u32 *source, renderer_u32 *output,
    renderer_u32 *pointer_cell);
__declspec(dllexport) BOOL WINAPI ProbeSetSyntheticForwarderA(const char *path);

#endif
