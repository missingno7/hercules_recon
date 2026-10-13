#include <windows.h>
#include <stdio.h>

#include "presentation_probe.h"

typedef HRESULT (WINAPI *DirectDrawCreateFn)(void *guid, void **out_object,
                                              void *outer);

typedef struct SyntheticBackendTrace {
    DWORD calls;
    void *guid;
    void **out_object;
    void *outer;
    HRESULT result;
    void *returned_object;
} SyntheticBackendTrace;
typedef SyntheticBackendTrace *(WINAPI *GetBackendTraceFn)(void);
typedef PresentationProbeV1 *(WINAPI *GetProbeFn)(void);
typedef PresentationForwardInfoV1 *(WINAPI *GetForwardInfoFn)(void);
typedef renderer_u32 (WINAPI *BuildCloneFn)(const renderer_u32 *, renderer_u32 *);
typedef BOOL (WINAPI *SetForwarderFn)(const char *);
typedef int (__cdecl *FakeChainInitFn)(void *, renderer_u32);
typedef void (__cdecl *FakeSubmitFn)(void *);

static void *g_seen_first;
static renderer_u32 g_seen_count;
static renderer_u32 g_fake_submit_calls;
static void *g_seen_submit;

static int __cdecl fake_chain_init(void *first, renderer_u32 count)
{
    g_seen_first = first;
    g_seen_count = count;
    return 0x13572468;
}

static void __cdecl fake_submit(void *first)
{
    g_fake_submit_calls += 1UL;
    g_seen_submit = first;
}

static int fail(const char *message)
{
    printf("FAIL=%s\n", message);
    return 1;
}

int main(int argc, char **argv)
{
    HMODULE probe_module;
    HMODULE backend_module;
    BuildCloneFn build_clone;
    SetForwarderFn set_forwarder;
    GetProbeFn get_probe;
    GetForwardInfoFn get_forward_info;
    GetBackendTraceFn get_backend_trace;
    DirectDrawCreateFn directdraw_create;
    PresentationProbeV1 *probe;
    PresentationForwardInfoV1 *forward_info;
    SyntheticBackendTrace *backend;
    renderer_u32 source[ENGINE_TABLE_WORD_COUNT];
    renderer_u32 clone[ENGINE_TABLE_WORD_COUNT];
    renderer_u32 index;
    renderer_u32 slot;
    renderer_u32 value;
    void *first;
    void *submit_first;
    HRESULT result;
    void *object;
    void *guid;
    void *outer;
    char backend_path[MAX_PATH];
    int saw_chain = 0;
    int saw_submit = 0;
    int saw_submit_return = 0;
    int saw_ddraw_enter = 0;
    int saw_ddraw_return = 0;

    if (argc != 2 && argc != 3)
        return fail("expected absolute synthetic backend path");
    if (!GetFullPathNameA(argv[1], sizeof(backend_path), backend_path, 0))
        return fail("could not resolve synthetic backend path");
    probe_module = LoadLibraryA("ddraw.dll");
    if (probe_module == 0)
        return fail("candidate ddraw.dll could not be loaded from app directory");
    get_probe = (GetProbeFn)GetProcAddress(probe_module, "ProbeGetState");
    get_forward_info = (GetForwardInfoFn)GetProcAddress(
        probe_module, "ProbeGetForwardInfo");
    directdraw_create = (DirectDrawCreateFn)GetProcAddress(
        probe_module, "DirectDrawCreate");
    build_clone = (BuildCloneFn)GetProcAddress(probe_module,
                                                "ProbeBuildSyntheticClone");
    set_forwarder = (SetForwarderFn)GetProcAddress(probe_module,
                                                    "ProbeSetSyntheticForwarderA");
    if (get_probe == 0 || get_forward_info == 0 || directdraw_create == 0 ||
        build_clone == 0 || set_forwarder == 0)
        return fail("candidate diagnostic exports missing");
    probe = get_probe();
    forward_info = get_forward_info();
    if (probe == 0 || probe->state != PRESENTATION_STATE_OTHER_IMAGE)
        return fail("candidate did not leave the synthetic image untouched");

    if (argc == 3) {
        char candidate_path[MAX_PATH];
        renderer_u32 expected_guard_state;
        const char *self_guard_name;
        const char *forward_path;

        if (lstrcmpA(argv[2], "--selfguard") == 0) {
            expected_guard_state = PRESENTATION_FORWARD_SELF_MODULE;
            self_guard_name = "SELF_MODULE_GUARD";
            if (GetModuleFileNameA(probe_module, candidate_path,
                                   sizeof(candidate_path)) == 0)
                return fail("could not read candidate module path");
            forward_path = candidate_path;
        } else if (lstrcmpA(argv[2], "--selfexport") == 0) {
            expected_guard_state = PRESENTATION_FORWARD_SELF_EXPORT;
            self_guard_name = "SELF_EXPORT_GUARD";
            forward_path = backend_path;
        } else {
            return fail("unexpected third argument");
        }
        if (!set_forwarder(forward_path))
            return fail("candidate self-forward path was rejected");
        object = (void *)0x6789a000UL;
        result = directdraw_create(0, &object, 0);
        if ((renderer_u32)result != 0x80004005UL ||
            object != (void *)0x6789a000UL ||
            forward_info->state != expected_guard_state ||
            GetModuleHandleA("synthetic_system_ddraw.dll") != 0)
            return fail("candidate self-forward was not safely rejected");
        printf("RESULT=PASS\n%s=PASS\nFORWARD_STATE=%lu\n",
               self_guard_name, forward_info->state);
        return 0;
    }

    for (index = 0; index < ENGINE_TABLE_WORD_COUNT; ++index)
        source[index] = 0xa0000000UL + index;
    source[0] = ENGINE_TABLE_SIGNATURE;
    source[HERCULES_SLOT_CHAIN_INIT] = (renderer_u32)fake_chain_init;
    source[HERCULES_SLOT_SUBMIT] = (renderer_u32)fake_submit;
    if (build_clone(source, clone) != ENGINE_TABLE_CLONE_OK)
        return fail("bounded 250-word clone failed");
    for (index = 0; index < ENGINE_TABLE_WORD_COUNT; ++index) {
        if (index == HERCULES_SLOT_CHAIN_INIT || index == HERCULES_SLOT_SUBMIT)
            continue;
        if (clone[index] != source[index])
            return fail("an unselected table word changed");
    }
    if (clone[0] != ENGINE_TABLE_SIGNATURE ||
        clone[HERCULES_SLOT_CHAIN_INIT] == source[HERCULES_SLOT_CHAIN_INIT] ||
        clone[HERCULES_SLOT_SUBMIT] == source[HERCULES_SLOT_SUBMIT])
        return fail("selected wrapper words were not installed");

    first = (void *)0x12345000UL;
    submit_first = (void *)0x23456000UL;
    slot = HERCULES_SLOT_CHAIN_INIT;
    value = clone[slot];
    result = ((PresentationChainInitFn)value)(first, 0x800UL);
    if (result != 0x13572468 || g_seen_first != first ||
        g_seen_count != 0x800UL)
        return fail("chain-init wrapper changed arguments or return");
    slot = HERCULES_SLOT_SUBMIT;
    value = clone[slot];
    ((PresentationSubmitFn)value)(submit_first);
    if (g_fake_submit_calls != 1UL || g_seen_submit != submit_first)
        return fail("submit wrapper did not forward the original call");

    if (!set_forwarder(backend_path))
        return fail("synthetic forwarding path was rejected");
    guid = (void *)0x34567000UL;
    outer = (void *)0x45678000UL;
    object = (void *)0x6789a000UL;
    if (GetModuleHandleA("synthetic_system_ddraw.dll") != 0)
        return fail("forwarding backend was loaded before export invocation");
    result = directdraw_create(guid, &object, outer);
    if ((renderer_u32)result != 0x88760007UL ||
        object != (void *)0x56789000UL ||
        GetModuleHandleA("synthetic_system_ddraw.dll") == 0)
        return fail("safe export call did not load and forward to backend");
    backend_module = GetModuleHandleA("synthetic_system_ddraw.dll");
    get_backend_trace = (GetBackendTraceFn)GetProcAddress(
        backend_module, "BackendGetTrace");
    if (get_backend_trace == 0)
        return fail("synthetic backend diagnostic export missing");
    backend = get_backend_trace();
    if (backend == 0 ||
        backend->guid != guid || backend->out_object != &object ||
        backend->outer != outer || backend->calls != 1UL)
        return fail("forwarder changed DirectDraw arguments, HRESULT, or object");

    object = (void *)0x89abc000UL;
    result = directdraw_create(0, &object, 0);
    if ((renderer_u32)result != 0x80004005UL ||
        object != (void *)0x789ab000UL ||
        backend->guid != 0 || backend->out_object != &object ||
        backend->outer != 0 || backend->calls != 2UL)
        return fail("failure HRESULT or output object did not pass through");

    for (index = 0; index < probe->trace_count &&
                        index < PRESENTATION_TRACE_CAPACITY; ++index) {
        PresentationTraceEventV1 *event = &probe->trace[index];
        if (event->kind == PRESENTATION_EVENT_CHAIN_INIT &&
            event->arg0 == (renderer_u32)first && event->arg1 == 0x800UL)
            saw_chain = 1;
        if (event->kind == PRESENTATION_EVENT_SUBMIT &&
            event->arg0 == (renderer_u32)submit_first)
            saw_submit = 1;
        if (event->kind == PRESENTATION_EVENT_SUBMIT_RETURN &&
            event->arg0 == (renderer_u32)submit_first)
            saw_submit_return = 1;
        if (event->kind == PRESENTATION_EVENT_DDRAW_ENTER &&
            event->arg0 == (renderer_u32)guid &&
            event->arg1 == (renderer_u32)&object &&
            event->arg2 == (renderer_u32)outer)
            saw_ddraw_enter = 1;
        if (event->kind == PRESENTATION_EVENT_DDRAW_RETURN &&
            event->arg0 == (renderer_u32)(HRESULT)0x80004005UL)
            saw_ddraw_return = 1;
    }
    if (!saw_chain || !saw_submit || !saw_submit_return ||
        !saw_ddraw_enter || !saw_ddraw_return)
        return fail("fixed trace omitted a callback or forwarding boundary");
    if (probe->trace_dropped != 0UL || probe->directdrawcreate_calls != 2UL ||
        probe->real_export == 0UL)
        return fail("diagnostic counters or forwarded export are inconsistent");

    printf("RESULT=PASS\n");
    printf("CLONE_WORDS=%lu\n", ENGINE_TABLE_WORD_COUNT);
    printf("FORWARD_CALLS=%lu\n", backend->calls);
    printf("TRACE_EVENTS=%lu\n", probe->trace_count);
    printf("TRACE_DROPPED=%lu\n", probe->trace_dropped);
    printf("SYNTHETIC_HOST_ATTACH_STATE=%lu\n", probe->state);
    return 0;
}
