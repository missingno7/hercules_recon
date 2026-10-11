/* Observed audio priority transition; genuine pinned Windows SDK calls. */
#include <windows.h>
extern "C" {
extern DWORD g_exe_615bd0;
extern int g_exe_615bd4;
extern BOOL g_exe_615bd8;
extern HANDLE g_exe_615bbc;
void __cdecl audio_native_035d0(BYTE enabled)
{
    if (enabled && !g_exe_615bd8) {
        g_exe_615bd0 = GetPriorityClass(GetCurrentProcess());
        g_exe_615bd4 = GetThreadPriority(GetCurrentThread());
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
        if (g_exe_615bbc != 0)
            SetThreadPriority(g_exe_615bbc, THREAD_PRIORITY_LOWEST);
        g_exe_615bd8 = TRUE;
    }
    if (!enabled && g_exe_615bd8) {
        SetPriorityClass(GetCurrentProcess(), g_exe_615bd0);
        SetThreadPriority(GetCurrentThread(), g_exe_615bd4);
        g_exe_615bd8 = FALSE;
    }
}
}
