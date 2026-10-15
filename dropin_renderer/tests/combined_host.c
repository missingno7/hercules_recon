/* Source-built host only. Reuses the existing postentry_engine.c cold-copy
 * engine. No original executable or engine is loaded by this fixture. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dropin_renderer/host_bootstrap.h"
#include "postentry_shared.h"

static renderer_u32 __cdecl host_chain(void *first, renderer_u32 count);
static renderer_u32 __cdecl host_submit(void *first);
#ifndef COMBINED_UNUSED_RESULT
#define COMBINED_UNUSED_RESULT 0x22110000
#endif
#ifndef COMBINED_TABLE_SIGNATURE
#define COMBINED_TABLE_SIGNATURE ENGINE_TABLE_SIGNATURE
#endif
static renderer_u32 __cdecl host_unused(void) { return COMBINED_UNUSED_RESULT; }

__declspec(dllexport) renderer_u32 CombinedTable[250] = {
#include "synthetic_table.h"
};
__declspec(dllexport) renderer_u32 CombinedPointer = (renderer_u32)CombinedTable;
__declspec(dllexport) renderer_u32 CombinedPrimary;
static renderer_u32 g_early[250];
static renderer_u32 g_log[8], g_log_n, g_events, g_errors;
static int g_backend_fault;

static void __cdecl early_copy(void)
{
    memcpy(g_early, (const void *)CombinedPointer, sizeof(g_early));
}
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) void (__cdecl *CombinedEarlyCopy)(void) = early_copy;

#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); ++g_errors; } } while (0)

static renderer_u32 __cdecl host_chain(void *first, renderer_u32 count)
{
    g_log[g_log_n++] = (renderer_u32)first;
    g_log[g_log_n++] = count;
    SetLastError(0x81818181);
    return 0x11223344;
}
static renderer_u32 __cdecl host_submit(void *first)
{
    g_log[g_log_n++] = (renderer_u32)first;
    g_log[g_log_n++] = GetLastError();
    return 0x55667788;
}
static void __cdecl observe(void *context, const HsServiceEvent *event)
{
    (void)context;
    ++g_events;
    CHECK(event->slot == 81 || event->slot == 89);
    SetLastError(0xbad);
    if (g_backend_fault)
        RaiseException(0xe0001234, 0, 0, 0);
}

int main(int argc, char **argv)
{
    HMODULE engine;
    void (__cdecl *run)(SyntheticEngineInvocationV1 *);
    SyntheticEngineStateV1 *state;
    SyntheticEngineInvocationV1 invocation;
    HsBackendV1 backend = {sizeof(HsBackendV1), HS_ABI_VERSION, 0, observe, 0, 0, 0};
    renderer_u32 baseline[8], source[250], i, pass;
    const renderer_u32 *slots;
    renderer_u32 count = hs_relayed_slots(&slots);
    HsBootstrapStatus expected;
    if (argc != 3) return 2;
    expected = (HsBootstrapStatus)atoi(argv[2]);
    CHECK(hs_bootstrap_status() == expected);
    memcpy(source, CombinedTable, sizeof(source));
    for (i = 0; i < 250; ++i) {
        renderer_u32 j, want = source[i];
        if (expected == HS_BOOTSTRAP_INSTALLED)
            for (j = 0; j < count; ++j)
                if (i == slots[j]) want = hs_relay_address(i);
        CHECK(g_early[i] == want);
    }
    CHECK((CombinedPointer != (renderer_u32)CombinedTable) ==
          (expected == HS_BOOTSTRAP_INSTALLED));
    /* The engine uses the already copied prefix, even after the publisher's
     * cell changes. Backend switching must leave this cold copy unchanged. */
    CombinedPointer = (renderer_u32)CombinedTable;
    engine = LoadLibraryA(argv[1]);
    CHECK(engine != 0);
    if (!engine) return 3;
    run = (void (__cdecl *)(SyntheticEngineInvocationV1 *))GetProcAddress(engine, "SyntheticEngineRun");
    state = (SyntheticEngineStateV1 *)GetProcAddress(engine, "SyntheticEngineState");
    CHECK(run && state);
    if (!run || !state) return 4;
    invocation.table = g_early;
    invocation.warm_copy = 0;
    invocation.hang_after_copy = 0;
    for (pass = 0; pass < 3; ++pass) {
        g_log_n = g_events = 0;
        g_backend_fault = pass == 2;
        CHECK(hs_select_backend(pass ? &backend : 0,
              pass ? HS_MODE_OBSERVE : HS_MODE_FORWARD_ONLY) == HS_INSTALLED);
        SetLastError(0x12345678);
        run(&invocation);
        CHECK(g_log_n == 4);
        CHECK(state->slot81_result == 0x11223344);
        CHECK(state->render_complete == 1 && state->copy_count == pass + 1);
        CHECK(memcmp(state->local_table, g_early, sizeof(g_early)) == 0);
        CHECK(memcmp(source, CombinedTable, sizeof(source)) == 0);
        if (pass == 0) memcpy(baseline, g_log, sizeof(baseline));
        else CHECK(memcmp(baseline, g_log, 4 * sizeof(renderer_u32)) == 0);
        CHECK(g_events == (expected != HS_BOOTSTRAP_INSTALLED || pass == 0 ? 0u : pass == 1 ? 4u : 1u));
    }
    printf("COMBINED_HOST status=%u cold_copy=250 modes=3 errors=%u\n", expected, g_errors);
    /* The statically imported combined DLL lives until process exit. */
    return g_errors ? 1 : 0;
}
