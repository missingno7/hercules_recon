#include "host_system_keys.h"

extern "C" {
extern BYTE g_exe_96e618;
extern BYTE g_exe_92c488;
extern signed char g_exe_ba0238;
extern BYTE g_exe_ba0228;
extern DWORD g_exe_ba0230;
extern DWORD g_host_input_pending_rva_7a0234;
extern BYTE g_fatal_suppress_dialog;
unsigned short __cdecl cleanup_native_6ec0(int value);
}

void CMainFrame::OnSysKeyDown(UINT key, UINT repeatCount, UINT flags)
{
    if (g_exe_96e618 != 0) {
        return;
    }

    if (g_fatal_suppress_dialog != 0) {
        g_exe_96e618 = 1;
        cleanup_native_6ec0(0);
        g_host_input_pending_rva_7a0234 = 1;
        return;
    }

    if (g_exe_92c488 == 0 || key != VK_RETURN || g_exe_ba0238 < 2 ||
        g_exe_ba0228 != 0 || g_exe_ba0230 != 0) {
        Default();
        return;
    }

    g_exe_96e618 = 1;
    cleanup_native_6ec0(1);
    g_host_input_pending_rva_7a0234 = 1;
}

void CMainFrame::OnSysKeyUp(UINT key, UINT repeatCount, UINT flags)
{
    if (g_exe_96e618 != 0) {
        g_exe_96e618 = 0;
        return;
    }

    Default();
}
