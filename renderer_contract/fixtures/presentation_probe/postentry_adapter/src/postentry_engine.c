#include <windows.h>

#include "postentry_shared.h"

typedef int (__cdecl *EngineChainInitFn)(void *, renderer_u32);
typedef void (__cdecl *EngineSubmitFn)(void *);

__declspec(dllexport) SyntheticEngineStateV1 SyntheticEngineState = { 0 };

void __cdecl SyntheticEngineAfterCopy(void)
{
    SyntheticEngineState.after_copy_hits += 1UL;
}

void __cdecl SyntheticEngineAfterChainReturn(void)
{
}

void __cdecl SyntheticEngineAfterSubmitReturn(void)
{
}

void __cdecl SyntheticEngineAfterWarmCopy(void)
{
    SyntheticEngineState.after_warm_copy_hits += 1UL;
}

void __cdecl SyntheticEngineAfterRender(void)
{
    SyntheticEngineState.after_render_hits += 1UL;
}

static void copy_table(const renderer_u32 *source)
{
    renderer_u32 index;
    for (index = 0; index < ENGINE_TABLE_WORD_COUNT; ++index)
        SyntheticEngineState.local_table[index] = source[index];
    SyntheticEngineState.copy_count += 1UL;
}

void __cdecl SyntheticEngineRun(SyntheticEngineInvocationV1 *invocation)
{
    EngineChainInitFn chain_init;
    EngineSubmitFn submit;

    if (invocation == 0 || invocation->table == 0)
        return;
    SyntheticEngineState.magic = POSTENTRY_ENGINE_MAGIC;
    copy_table(invocation->table);
    SyntheticEngineAfterCopy();
    if (invocation->warm_copy) {
        copy_table(invocation->table);
        SyntheticEngineAfterWarmCopy();
    }
    if (invocation->hang_after_copy)
        Sleep(INFINITE);

    chain_init = (EngineChainInitFn)SyntheticEngineState.local_table[
        HERCULES_SLOT_CHAIN_INIT];
    submit = (EngineSubmitFn)SyntheticEngineState.local_table[
        HERCULES_SLOT_SUBMIT];
    if (chain_init != 0) {
        SyntheticEngineState.slot81_result =
            (renderer_u32)chain_init((void *)0x81810001UL, 0x81000001UL);
        SyntheticEngineAfterChainReturn();
    }
    if (submit != 0) {
        submit((void *)0x89890001UL);
        SyntheticEngineAfterSubmitReturn();
    }
    SyntheticEngineState.callback_thread_id = GetCurrentThreadId();
    SyntheticEngineState.render_complete = 1UL;
    SyntheticEngineAfterRender();
}
