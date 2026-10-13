#include <windows.h>
#include <stdio.h>
#include <string.h>

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
typedef renderer_u32 (WINAPI *PublishTableFn)(const renderer_u32 *,
                                               renderer_u32 *,
                                               renderer_u32 *);
typedef BOOL (WINAPI *SetForwarderFn)(const char *);
/* Physical x86 return capture is test-only; public slot 89 remains void. */
typedef renderer_u32 (__cdecl *RawChainInitFn)(void *, renderer_u32);
typedef renderer_u32 (__cdecl *RawSubmitFn)(void *);
typedef struct AbiResult {
    renderer_u32 raw_eax;
    renderer_u32 esp_before;
    renderer_u32 esp_after_call;
    renderer_u32 preserved_mask;
} AbiResult;

#define FAKE_NESTED_RAW_EAX 0xa89f00d1UL
#define FAKE_DIRECT_RAW_EAX 0x89abcdefUL

static renderer_u32 *g_published_pointer_cell;
static LONG g_fake_submit_calls;
static LONG g_fake_chain_calls;
static LONG g_last_submit;
static LONG g_nested_submit_once;
static LONG g_nested_submit_result;
static LONG g_worker_results[4];
static renderer_u32 g_synthetic_source[ENGINE_TABLE_WORD_COUNT];
static renderer_u32 g_synthetic_clone[ENGINE_TABLE_WORD_COUNT];
static renderer_u32 g_synthetic_pointer_cell;

/* These helpers are diagnostic call probes, not reconstructed game code.
 * They capture raw EAX, then verify Cdecl cleanup and nonvolatile registers. */
__declspec(naked) static void __cdecl abi_invoke_submit(
    RawSubmitFn callback, void *first, AbiResult *result)
{
    __asm {
        push ebp
        mov ebp, esp
        push ebx
        push esi
        push edi
        sub esp, 12
        mov eax, dword ptr [ebp+16]
        mov dword ptr [esp], eax
        mov dword ptr [esp+4], esp
        mov dword ptr [esp+8], ebp
        mov ebx, 0x81b0b0b0
        mov esi, 0x89c0c0c0
        mov edi, 0x89d0d0d0
        push dword ptr [ebp+12]
        call dword ptr [ebp+8]
        mov edx, dword ptr [esp+4]
        mov dword ptr [edx], eax
        mov ecx, dword ptr [esp+8]
        mov dword ptr [edx+4], ecx
        mov ecx, esp
        mov dword ptr [edx+8], ecx
        xor eax, eax
        cmp ebx, 0x81b0b0b0
        jne submit_esi_check
        or eax, 1
    submit_esi_check:
        cmp esi, 0x89c0c0c0
        jne submit_edi_check
        or eax, 2
    submit_edi_check:
        cmp edi, 0x89d0d0d0
        jne submit_ebp_check
        or eax, 4
    submit_ebp_check:
        cmp ebp, dword ptr [esp+12]
        jne submit_esp_check
        or eax, 8
    submit_esp_check:
        mov ecx, dword ptr [esp+8]
        sub ecx, 4
        cmp esp, ecx
        jne submit_save_mask
        or eax, 16
    submit_save_mask:
        mov edx, dword ptr [esp+4]
        mov dword ptr [edx+12], eax
        mov esp, dword ptr [esp+8]
        add esp, 12
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}

__declspec(naked) static void __cdecl abi_invoke_chain(
    RawChainInitFn callback, void *first, renderer_u32 count,
    AbiResult *result)
{
    __asm {
        push ebp
        mov ebp, esp
        push ebx
        push esi
        push edi
        sub esp, 12
        mov eax, dword ptr [ebp+20]
        mov dword ptr [esp], eax
        mov dword ptr [esp+4], esp
        mov dword ptr [esp+8], ebp
        mov ebx, 0x81b0b0b0
        mov esi, 0x89c0c0c0
        mov edi, 0x89d0d0d0
        push dword ptr [ebp+16]
        push dword ptr [ebp+12]
        call dword ptr [ebp+8]
        mov edx, dword ptr [esp+8]
        mov dword ptr [edx], eax
        mov ecx, dword ptr [esp+12]
        mov dword ptr [edx+4], ecx
        mov ecx, esp
        mov dword ptr [edx+8], ecx
        xor eax, eax
        cmp ebx, 0x81b0b0b0
        jne chain_esi_check
        or eax, 1
    chain_esi_check:
        cmp esi, 0x89c0c0c0
        jne chain_edi_check
        or eax, 2
    chain_edi_check:
        cmp edi, 0x89d0d0d0
        jne chain_ebp_check
        or eax, 4
    chain_ebp_check:
        cmp ebp, dword ptr [esp+16]
        jne chain_esp_check
        or eax, 8
    chain_esp_check:
        mov ecx, dword ptr [esp+12]
        sub ecx, 8
        cmp esp, ecx
        jne chain_save_mask
        or eax, 16
    chain_save_mask:
        mov edx, dword ptr [esp+8]
        mov dword ptr [edx+12], eax
        mov esp, dword ptr [esp+12]
        add esp, 12
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}

static int __cdecl fake_chain_init(void *first, renderer_u32 count)
{
    LONG calls = InterlockedIncrement((LONG *)&g_fake_chain_calls);
    (void)first;
    (void)count;
    if (calls == 1L && g_published_pointer_cell != 0) {
        renderer_u32 *table = (renderer_u32 *)*g_published_pointer_cell;
        RawSubmitFn nested = (RawSubmitFn)table[HERCULES_SLOT_SUBMIT];
        if (InterlockedCompareExchange((PVOID *)&g_nested_submit_once,
                                       (PVOID)1, (PVOID)0) == 0)
            InterlockedExchange((LONG *)&g_nested_submit_result,
                (LONG)nested((void *)0x0badf00dUL));
    }
    return 0x13572468;
}

static renderer_u32 __cdecl fake_submit(void *first)
{
    renderer_u32 value = (renderer_u32)first;
    InterlockedIncrement((LONG *)&g_fake_submit_calls);
    InterlockedExchange((LONG *)&g_last_submit, (LONG)first);
    if (value == 0x0badf00dUL)
        return FAKE_NESTED_RAW_EAX;
    if (value == 0x23456000UL)
        return FAKE_DIRECT_RAW_EAX;
    if (value >= 0x70000000UL && value < 0x70000004UL)
        return 0x90000000UL | (value & 0xffUL);
    return 0xfedcba98UL;
}

static DWORD WINAPI submit_worker(LPVOID argument)
{
    renderer_u32 *table = (renderer_u32 *)*g_published_pointer_cell;
    RawSubmitFn submit = (RawSubmitFn)table[HERCULES_SLOT_SUBMIT];
    renderer_u32 index = (renderer_u32)argument - 0x70000000UL;
    renderer_u32 result = submit(argument);
    if (index < 4UL)
        InterlockedExchange((LONG *)&g_worker_results[index], (LONG)result);
    return 0UL;
}

static int fail(const char *message)
{
    printf("FAIL=%s\n", message);
    return 1;
}

static int verify_completed_trace(PresentationProbeV1 *probe,
                                  void *first, renderer_u32 count,
                                  void *submit_first, void *guid,
                                  void **out_object, void *outer)
{
    renderer_u32 index;
    renderer_u32 chain_enter = 0UL;
    renderer_u32 chain_return = 0UL;
    renderer_u32 nested_enter = 0UL;
    renderer_u32 nested_return = 0UL;
    renderer_u32 submit_enter = 0UL;
    renderer_u32 submit_return = 0UL;
    renderer_u32 draw_success_enter = 0UL;
    renderer_u32 draw_success_return = 0UL;
    renderer_u32 draw_failure_enter = 0UL;
    renderer_u32 draw_failure_return = 0UL;
    renderer_u32 worker_args_seen = 0UL;
    renderer_u32 worker_returns_seen = 0UL;

    for (index = 0; index < probe->trace_count; ++index) {
        PresentationTraceEventV1 *event = &probe->trace[index];
        if (event->sequence != index + 1UL ||
            event->committed_sequence != (LONG)event->sequence ||
            event->thread_id == 0UL)
            return 0;
        if (event->kind == PRESENTATION_EVENT_CHAIN_INIT &&
            event->arg0 == (renderer_u32)first && event->arg1 == count &&
            event->target != 0UL) {
            chain_enter = event->sequence;
        } else if (event->kind == PRESENTATION_EVENT_CHAIN_INIT_RETURN &&
                   event->arg0 == (renderer_u32)first &&
                   event->arg1 == count &&
                   event->arg2 == 0x13572468UL) {
            chain_return = event->sequence;
        } else if (event->kind == PRESENTATION_EVENT_SUBMIT &&
                   event->arg0 == 0x0badf00dUL) {
            nested_enter = event->sequence;
        } else if (event->kind == PRESENTATION_EVENT_SUBMIT_RETURN &&
                   event->arg0 == 0x0badf00dUL) {
            if (event->arg1 != FAKE_NESTED_RAW_EAX)
                return 0;
            nested_return = event->sequence;
        } else if (event->kind == PRESENTATION_EVENT_SUBMIT &&
                   event->arg0 == (renderer_u32)submit_first) {
            submit_enter = event->sequence;
        } else if (event->kind == PRESENTATION_EVENT_SUBMIT_RETURN &&
                   event->arg0 == (renderer_u32)submit_first) {
            if (event->arg1 != FAKE_DIRECT_RAW_EAX)
                return 0;
            submit_return = event->sequence;
        } else if (event->kind == PRESENTATION_EVENT_DDRAW_ENTER &&
                   event->arg0 == (renderer_u32)guid &&
                   event->arg1 == (renderer_u32)out_object &&
                   event->arg2 == (renderer_u32)outer) {
            draw_success_enter = event->sequence;
        } else if (event->kind == PRESENTATION_EVENT_DDRAW_RETURN &&
                   event->arg0 == 0x88760007UL &&
                   event->arg1 == (renderer_u32)out_object) {
            draw_success_return = event->sequence;
        } else if (event->kind == PRESENTATION_EVENT_DDRAW_ENTER &&
                   event->arg0 == 0UL &&
                   event->arg1 == (renderer_u32)out_object &&
                   event->arg2 == 0UL) {
            draw_failure_enter = event->sequence;
        } else if (event->kind == PRESENTATION_EVENT_DDRAW_RETURN &&
                   event->arg0 == 0x80004005UL &&
                   event->arg1 == (renderer_u32)out_object) {
            draw_failure_return = event->sequence;
        }

        if (event->kind == PRESENTATION_EVENT_SUBMIT) {
            if (event->arg0 == 0x0badf00dUL ||
                event->arg0 == (renderer_u32)submit_first)
                continue;
            if (event->arg0 >= 0x70000000UL &&
                event->arg0 < 0x70000004UL)
                worker_args_seen |= 1UL << (event->arg0 - 0x70000000UL);
        } else if (event->kind == PRESENTATION_EVENT_SUBMIT_RETURN &&
                   event->arg0 >= 0x70000000UL &&
                   event->arg0 < 0x70000004UL) {
            renderer_u32 worker_index = event->arg0 - 0x70000000UL;
            if (event->arg1 != (0x90000000UL | worker_index))
                return 0;
            worker_returns_seen |= 1UL << worker_index;
        }
    }
    return chain_enter != 0UL && nested_enter > chain_enter &&
           nested_return > nested_enter && chain_return > nested_return &&
           submit_enter != 0UL && submit_return > submit_enter &&
           draw_success_enter != 0UL &&
           draw_success_return > draw_success_enter &&
           draw_failure_enter > draw_success_return &&
           draw_failure_return > draw_failure_enter &&
           worker_args_seen == 0x0fUL && worker_returns_seen == 0x0fUL;
}

int main(int argc, char **argv)
{
    HMODULE probe_module;
    HMODULE backend_module;
    PublishTableFn publish_table;
    SetForwarderFn set_forwarder;
    GetProbeFn get_probe;
    GetForwardInfoFn get_forward_info;
    GetBackendTraceFn get_backend_trace;
    DirectDrawCreateFn directdraw_create;
    PresentationProbeV1 *probe;
    PresentationForwardInfoV1 *forward_info;
    SyntheticBackendTrace *backend;
    renderer_u32 *source = g_synthetic_source;
    renderer_u32 *clone = g_synthetic_clone;
    renderer_u32 index;
    renderer_u32 *published_table;
    void *threads[4];
    void *first;
    void *submit_first;
    HRESULT result;
    AbiResult chain_abi;
    AbiResult submit_abi;
    void *object;
    void *guid;
    void *outer;
    char backend_path[MAX_PATH];
    BOOL hold_after_pass = FALSE;

    if (argc != 2 && argc != 3)
        return fail("expected backend path and optional --selfguard, --selfexport, or --hold-after-pass");
    if (!GetFullPathNameA(argv[1], sizeof(backend_path), backend_path, 0))
        return fail("could not resolve synthetic backend path");
    if (argc == 3 && lstrcmpA(argv[2], "--hold-after-pass") == 0)
        hold_after_pass = TRUE;

    probe_module = LoadLibraryA("ddraw.dll");
    if (probe_module == 0)
        return fail("candidate ddraw.dll could not be loaded from app directory");
    get_probe = (GetProbeFn)GetProcAddress(probe_module, "ProbeGetState");
    get_forward_info = (GetForwardInfoFn)GetProcAddress(
        probe_module, "ProbeGetForwardInfo");
    directdraw_create = (DirectDrawCreateFn)GetProcAddress(
        probe_module, "DirectDrawCreate");
    publish_table = (PublishTableFn)GetProcAddress(
        probe_module, "ProbePublishSyntheticTable");
    set_forwarder = (SetForwarderFn)GetProcAddress(
        probe_module, "ProbeSetSyntheticForwarderA");
    if (get_probe == 0 || get_forward_info == 0 || directdraw_create == 0 ||
        publish_table == 0 || set_forwarder == 0)
        return fail("candidate diagnostic exports missing");
    probe = get_probe();
    forward_info = get_forward_info();
    if (probe == 0 || probe->state != PRESENTATION_STATE_OTHER_IMAGE)
        return fail("candidate did not leave the synthetic image untouched in DllMain");

    if (argc == 3 && !hold_after_pass) {
        renderer_u32 expected_guard_state;
        const char *guard_name;
        const char *forward_path;

        if (lstrcmpA(argv[2], "--selfguard") == 0) {
            expected_guard_state = PRESENTATION_FORWARD_SELF_MODULE;
            guard_name = "SELF_MODULE_GUARD";
            if (GetModuleFileNameA(probe_module, backend_path,
                                   sizeof(backend_path)) == 0)
                return fail("could not read candidate module path");
            forward_path = backend_path;
        } else if (lstrcmpA(argv[2], "--selfexport") == 0) {
            expected_guard_state = PRESENTATION_FORWARD_SELF_EXPORT;
            guard_name = "SELF_EXPORT_GUARD";
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
               guard_name, forward_info->state);
        return 0;
    }

    for (index = 0; index < ENGINE_TABLE_WORD_COUNT; ++index)
        source[index] = 0xa0000000UL + index;
    source[0] = ENGINE_TABLE_SIGNATURE;
    source[HERCULES_SLOT_CHAIN_INIT] = (renderer_u32)fake_chain_init;
    source[HERCULES_SLOT_SUBMIT] = (renderer_u32)fake_submit;
    g_synthetic_pointer_cell = (renderer_u32)source;
    if (publish_table(source, clone, &g_synthetic_pointer_cell) !=
            ENGINE_TABLE_CLONE_OK ||
        g_synthetic_pointer_cell != (renderer_u32)clone ||
        probe->source_pointer_before != (renderer_u32)source ||
        probe->source_pointer_after != (renderer_u32)clone ||
        probe->clone_pointer != (renderer_u32)clone)
        return fail("source table clone was not published through the synthetic host pointer cell");
    if (probe->source_pointer_cell != (renderer_u32)&g_synthetic_pointer_cell)
        return fail("synthetic pointer cell identity was not recorded");
    published_table = (renderer_u32 *)g_synthetic_pointer_cell;
    for (index = 0; index < ENGINE_TABLE_WORD_COUNT; ++index) {
        if (index == HERCULES_SLOT_CHAIN_INIT || index == HERCULES_SLOT_SUBMIT)
            continue;
        if (published_table[index] != source[index])
            return fail("an unselected table word changed during clone publication");
    }
    if (memcmp(source, (renderer_u32 *)probe->source_pointer_before,
               sizeof(g_synthetic_source)) != 0 ||
        published_table[0] != ENGINE_TABLE_SIGNATURE ||
        published_table[HERCULES_SLOT_CHAIN_INIT] == source[HERCULES_SLOT_CHAIN_INIT] ||
        published_table[HERCULES_SLOT_SUBMIT] == source[HERCULES_SLOT_SUBMIT])
        return fail("source table changed or selected wrappers were not installed");

    g_published_pointer_cell = &g_synthetic_pointer_cell;
    first = (void *)0x12345000UL;
    submit_first = (void *)0x23456000UL;
    abi_invoke_chain((RawChainInitFn)published_table[
        HERCULES_SLOT_CHAIN_INIT], first, 0x800UL, &chain_abi);
    if (chain_abi.raw_eax != 0x13572468UL ||
        chain_abi.preserved_mask != 0x1fUL ||
        chain_abi.esp_before - chain_abi.esp_after_call != 8UL ||
        g_fake_chain_calls != 1L || g_nested_submit_result !=
            (LONG)FAKE_NESTED_RAW_EAX ||
        probe->slot81_entered != 1UL ||
        probe->slot81_completed != 1UL) {
        printf("DIAG_CHAIN_EAX=%08lx MASK=%08lx ESP_BEFORE=%08lx ESP_AFTER=%08lx CALLS=%ld NESTED=%08lx ENTERED=%lu COMPLETED=%lu\n",
               chain_abi.raw_eax, chain_abi.preserved_mask,
               chain_abi.esp_before, chain_abi.esp_after_call,
               g_fake_chain_calls, (renderer_u32)g_nested_submit_result,
               probe->slot81_entered, probe->slot81_completed);
        return fail("slot 81 raw EAX, Cdecl stack, saved registers, or nested slot 89 forwarding failed");
    }
    abi_invoke_submit((RawSubmitFn)published_table[
        HERCULES_SLOT_SUBMIT], submit_first, &submit_abi);
    if (submit_abi.raw_eax != FAKE_DIRECT_RAW_EAX ||
        submit_abi.preserved_mask != 0x1fUL ||
        submit_abi.esp_before - submit_abi.esp_after_call != 4UL ||
        g_fake_submit_calls != 2L || g_last_submit != (LONG)submit_first ||
        probe->slot89_entered != 2UL || probe->slot89_completed != 2UL)
        return fail("slot 89 raw EAX, Cdecl stack, saved registers, or forwarding failed");

    for (index = 0; index < 4UL; ++index) {
        threads[index] = CreateThread(0, 0, submit_worker,
            (LPVOID)(0x70000000UL + index), 0, 0);
        if (threads[index] == 0)
            return fail("synthetic concurrent callback thread creation failed");
    }
    if (WaitForMultipleObjects(4, (HANDLE *)threads, TRUE, 5000UL) !=
        WAIT_OBJECT_0)
        return fail("synthetic concurrent callbacks did not complete in time");
    for (index = 0; index < 4UL; ++index)
        CloseHandle((HANDLE)threads[index]);
    if (g_fake_submit_calls != 6L || probe->slot89_entered != 6UL ||
        probe->slot89_completed != 6UL ||
        probe->active_wrapper_calls != 0UL ||
        probe->max_active_wrapper_calls < 2UL)
        return fail("thread-safe callback counts or reentrant depth were inconsistent");
    for (index = 0; index < 4UL; ++index) {
        if ((renderer_u32)g_worker_results[index] !=
            (0x90000000UL | index))
            return fail("concurrent slot 89 raw EAX was not forwarded per thread");
    }

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
    if (backend == 0 || backend->guid != guid ||
        backend->out_object != &object || backend->outer != outer ||
        backend->calls != 1UL)
        return fail("forwarder changed DirectDraw arguments, HRESULT, or object");

    object = (void *)0x89abc000UL;
    result = directdraw_create(0, &object, 0);
    if ((renderer_u32)result != 0x80004005UL ||
        object != (void *)0x789ab000UL || backend->guid != 0 ||
        backend->out_object != &object || backend->outer != 0 ||
        backend->calls != 2UL)
        return fail("failure HRESULT or output object did not pass through");

    if (probe->trace_dropped != 0UL ||
        probe->directdrawcreate_calls != 2UL || probe->real_export == 0UL ||
        probe->trace_count != 18UL ||
        !verify_completed_trace(probe, first, 0x800UL, submit_first,
                                guid, &object, outer))
        return fail("published callback/forwarding trace was incomplete or corrupt");
    if (backend->calls != 2UL)
        return fail("synthetic backend call counter mismatch");

    printf("RESULT=PASS\n");
    printf("COPY_PUBLISHED=1\n");
    printf("CLONE_WORDS=%lu\n", ENGINE_TABLE_WORD_COUNT);
    printf("SOURCE_TABLE_UNCHANGED=1\n");
    printf("SLOT81_ENTERED=%lu\n", probe->slot81_entered);
    printf("SLOT81_COMPLETED=%lu\n", probe->slot81_completed);
    printf("SLOT89_ENTERED=%lu\n", probe->slot89_entered);
    printf("SLOT89_COMPLETED=%lu\n", probe->slot89_completed);
    printf("SLOT81_RAW_EAX=%08lx\n", chain_abi.raw_eax);
    printf("SLOT81_ABI_MASK=%08lx\n", chain_abi.preserved_mask);
    printf("SLOT89_RAW_EAX=%08lx\n", submit_abi.raw_eax);
    printf("SLOT89_ABI_MASK=%08lx\n", submit_abi.preserved_mask);
    printf("NESTED_SLOT89_RAW_EAX=%08lx\n",
           (renderer_u32)g_nested_submit_result);
    printf("WORKER_RAW_EAX=90000000,90000001,90000002,90000003\n");
    printf("REENTRANT_MAX_DEPTH=%lu\n", probe->max_active_wrapper_calls);
    printf("FORWARD_CALLS=%lu\n", backend->calls);
    printf("TRACE_EVENTS=%lu\n", probe->trace_count);
    printf("TRACE_DROPPED=%lu\n", probe->trace_dropped);
    printf("SYNTHETIC_HOST_ATTACH_STATE=%lu\n", probe->state);
    fflush(stdout);
    if (hold_after_pass) {
        printf("ABNORMAL_READY=1\n");
        fflush(stdout);
        Sleep(INFINITE);
    }
    return 0;
}
