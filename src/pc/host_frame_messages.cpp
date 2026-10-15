#include "../../calibration/host_system_keys.h"
extern "C" {
extern signed char g_exe_ba0238;
extern BYTE g_exe_ba0228;
extern DWORD g_exe_ba0230;
/* Existing fullscreen state owner; historical name also serves fatal dialogs. */
extern BYTE g_fatal_suppress_dialog;
extern int g_exe_ba0220;
extern int g_exe_ba0224;
extern BYTE g_host_frame_maximized_7a022c;
void __cdecl graphics_native_05a50(void);
unsigned short __cdecl cleanup_native_6ec0(int);
}
typedef char frame_receiver_is_188[(sizeof(CMainFrame)==188)?1:-1];
typedef char frame_point_is_8[(sizeof(CPoint)==8)?1:-1];

/* Original PC callbacks. Address labels and TU ownership remain provisional. */
void CMainFrame::OnRva_00009820(BOOL active, HTASK task)
{
    Default();
    g_exe_ba0238 = (active != 0) + 1;
    graphics_native_05a50();
}

void CMainFrame::OnRva_00009870(UINT type, int width, int height)
{
    CFrameWnd::OnSize(type,width,height);
    if (g_fatal_suppress_dialog == 0) {
        RECT client;
        g_host_frame_maximized_7a022c = type == SIZE_MAXIMIZED;
        ::GetClientRect(m_hWnd,&client);
        g_exe_ba0220 = client.right - client.left;
        g_exe_ba0224 = client.bottom - client.top;
        g_exe_ba0228 = g_exe_ba0220 < 1 || g_exe_ba0224 < 1;
    }
}

void CMainFrame::OnRva_00009910(UINT command, LPARAM parameter)
{
    if ((command & 0xfff0) != SC_SCREENSAVE) {
        ++g_exe_ba0230;
        graphics_native_05a50();
        CFrameWnd::OnSysCommand(command,parameter);
    }
}

void CMainFrame::OnRva_00009950(UINT hit, CPoint point)
{
    ++g_exe_ba0230;
    graphics_native_05a50();
    Default();
}

void CMainFrame::OnRva_00009970(CMenu *menu)
{
    ++g_exe_ba0230;
    graphics_native_05a50();
    Default();
}

void CMainFrame::OnRva_00009840(void)
{
    cleanup_native_6ec0(0);
    g_exe_ba0238 = 1;
    WinHelp(0,HELP_INDEX);
}
