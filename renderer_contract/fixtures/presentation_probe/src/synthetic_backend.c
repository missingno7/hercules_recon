#include <windows.h>

typedef struct SyntheticBackendTrace {
    DWORD calls;
    void *guid;
    void **out_object;
    void *outer;
    HRESULT result;
    void *returned_object;
} SyntheticBackendTrace;

static SyntheticBackendTrace g_trace;

__declspec(dllexport) SyntheticBackendTrace *WINAPI BackendGetTrace(void)
{
    return &g_trace;
}

HRESULT WINAPI DirectDrawCreate(void *guid, void **out_object, void *outer)
{
    g_trace.calls += 1UL;
    g_trace.guid = guid;
    g_trace.out_object = out_object;
    g_trace.outer = outer;
    if (g_trace.calls == 1UL) {
        g_trace.result = (HRESULT)0x88760007UL;
        g_trace.returned_object = (void *)0x56789000UL;
    } else {
        g_trace.result = (HRESULT)0x80004005UL;
        g_trace.returned_object = (void *)0x789ab000UL;
    }
    if (out_object != 0)
        *out_object = g_trace.returned_object;
    return g_trace.result;
}
