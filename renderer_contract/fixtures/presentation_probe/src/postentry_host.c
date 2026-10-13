#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "postentry_shared.h"

typedef renderer_u32 (WINAPI *PublishTableFn)(const renderer_u32 *,
                                               renderer_u32 *,
                                               renderer_u32 *);
typedef PresentationProbeV1 *(WINAPI *GetProbeFn)(void);
typedef void (__cdecl *EngineRunFn)(SyntheticEngineInvocationV1 *);

__declspec(dllexport) SyntheticHostStateV1 SyntheticHostState = { 0 };

static int __cdecl host_chain_init(void *first_slot, renderer_u32 count)
{
    SyntheticHostState.callback81_calls += 1UL;
    SyntheticHostState.callback81_first = (renderer_u32)first_slot;
    SyntheticHostState.callback81_count = count;
    SyntheticHostState.callback81_return = 0x11223344UL;
    return (int)SyntheticHostState.callback81_return;
}

/* The public table signature is void; this synthetic target gives physical
 * x86 EAX a distinctive value so the proxy's private transparent type can
 * record and relay it. */
static renderer_u32 __cdecl host_submit(void *first_slot)
{
    SyntheticHostState.callback89_calls += 1UL;
    SyntheticHostState.callback89_first = (renderer_u32)first_slot;
    SyntheticHostState.callback89_raw_eax = 0x55667788UL;
    return SyntheticHostState.callback89_raw_eax;
}

static DWORD WINAPI engine_worker(LPVOID argument)
{
    SyntheticEngineInvocationV1 *invocation =
        (SyntheticEngineInvocationV1 *)argument;
    HMODULE engine_module;
    EngineRunFn engine_run;
    while (InterlockedCompareExchange(
               (PVOID *)&SyntheticHostState.worker_thread_id,
               (PVOID)0, (PVOID)0) == 0)
        Sleep(0UL);
    engine_module = (HMODULE)SyntheticHostState.engine_base;
    engine_run = engine_module == 0 ? 0 :
        (EngineRunFn)GetProcAddress(engine_module, "SyntheticEngineRun");
    if (engine_run == 0)
        return 0xe002UL;
    engine_run(invocation);
    SyntheticHostState.worker_return = 0x600d0001UL;
    return SyntheticHostState.worker_return;
}

int main(int argc, char **argv)
{
    HMODULE proxy_module;
    HMODULE engine_module;
    PublishTableFn publish_table;
    GetProbeFn get_probe;
    PresentationProbeV1 *probe;
    SyntheticEngineInvocationV1 invocation;
    HANDLE worker;
    DWORD worker_id = 0UL;
    renderer_u32 index;
    int invalid_signature;
    int warm_copy;
    int timeout_after_copy;

    memset(&SyntheticHostState, 0, sizeof(SyntheticHostState));
    SyntheticHostState.magic = POSTENTRY_HOST_MAGIC;
    SyntheticHostState.state = POSTENTRY_HOST_STATE_INITIAL;
    if (argc != 4) {
        SyntheticHostState.state = POSTENTRY_HOST_STATE_FAILURE;
        return 10;
    }
    invalid_signature = lstrcmpiA(argv[3], "invalidsig") == 0;
    warm_copy = lstrcmpiA(argv[3], "coldwarm") == 0;
    timeout_after_copy = lstrcmpiA(argv[3], "timeout") == 0;

    for (index = 0; index < ENGINE_TABLE_WORD_COUNT; ++index)
        SyntheticHostState.source_table[index] = 0xa3000000UL + index;
    SyntheticHostState.source_table[0] = ENGINE_TABLE_SIGNATURE;
    SyntheticHostState.source_table[HERCULES_SLOT_CHAIN_INIT] =
        (renderer_u32)host_chain_init;
    SyntheticHostState.source_table[HERCULES_SLOT_SUBMIT] =
        (renderer_u32)host_submit;
    if (invalid_signature)
        SyntheticHostState.source_table[0] ^= 1UL;

    SyntheticHostState.pointer_cell =
        (renderer_u32)SyntheticHostState.source_table;
    proxy_module = LoadLibraryA(argv[1]);
    if (proxy_module == 0) {
        SyntheticHostState.state = POSTENTRY_HOST_STATE_FAILURE;
        return 11;
    }
    SyntheticHostState.candidate_base = (renderer_u32)proxy_module;
    publish_table = (PublishTableFn)GetProcAddress(
        proxy_module, "ProbePublishSyntheticTable");
    get_probe = (GetProbeFn)GetProcAddress(proxy_module, "ProbeGetState");
    if (publish_table == 0 || get_probe == 0) {
        SyntheticHostState.state = POSTENTRY_HOST_STATE_FAILURE;
        return 12;
    }
    probe = get_probe();
    SyntheticHostState.probe_state_pointer = (renderer_u32)probe;
    SyntheticHostState.state = POSTENTRY_HOST_STATE_PROXY_READY;
    SyntheticHostState.publish_status = publish_table(
        SyntheticHostState.source_table,
        SyntheticHostState.clone_table,
        &SyntheticHostState.pointer_cell);
    if (SyntheticHostState.publish_status != ENGINE_TABLE_CLONE_OK) {
        SyntheticHostState.state =
            SyntheticHostState.publish_status ==
                ENGINE_TABLE_CLONE_BAD_SIGNATURE ?
            POSTENTRY_HOST_STATE_BAD_SIGNATURE : POSTENTRY_HOST_STATE_FAILURE;
        printf("HOST_PUBLISH_STATUS=%lu\n",
               SyntheticHostState.publish_status);
        fflush(stdout);
        return invalid_signature ? 0x51UL : 13;
    }
    if (SyntheticHostState.pointer_cell !=
        (renderer_u32)SyntheticHostState.clone_table) {
        SyntheticHostState.state = POSTENTRY_HOST_STATE_FAILURE;
        return 14;
    }
    SyntheticHostState.state = POSTENTRY_HOST_STATE_TABLE_PUBLISHED;

    engine_module = LoadLibraryA(argv[2]);
    if (engine_module == 0) {
        SyntheticHostState.state = POSTENTRY_HOST_STATE_FAILURE;
        return 15;
    }
    SyntheticHostState.engine_base = (renderer_u32)engine_module;
    SyntheticHostState.state = POSTENTRY_HOST_STATE_ENGINE_LOADED;
    invocation.table = (const renderer_u32 *)
        SyntheticHostState.pointer_cell;
    invocation.warm_copy = warm_copy ? 1UL : 0UL;
    invocation.hang_after_copy = timeout_after_copy ? 1UL : 0UL;
    worker = CreateThread(0, 0, engine_worker, &invocation, 0, &worker_id);
    if (worker == 0) {
        SyntheticHostState.state = POSTENTRY_HOST_STATE_FAILURE;
        return 16;
    }
    InterlockedExchange(
        (LONG *)&SyntheticHostState.worker_thread_id, (LONG)worker_id);
    WaitForSingleObject(worker, INFINITE);
    CloseHandle(worker);
    SyntheticHostState.state = POSTENTRY_HOST_STATE_RENDER_DONE;
    printf("HOST_RENDER_RETURNED=1\n");
    fflush(stdout);
    return 0;
}
