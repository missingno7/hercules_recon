#include "host_system_keys.h"
extern "C" BYTE g_host_frame_maximized_7a022c;
/* Original PC resize helper/callers; names and TU ownership remain provisional. */
typedef char frame_receiver_is_188[(sizeof(CMainFrame)==188)?1:-1];

void CMainFrame::OnRva_000099A0(int width, int height)
{
    RECT window, work;
    if (g_host_frame_maximized_7a022c != 0) {
        ShowWindow(SW_RESTORE);
        g_host_frame_maximized_7a022c = 0;
    }
    GetWindowRect(&window);
    SystemParametersInfo(SPI_GETWORKAREA,0,&work,0);
    int y = window.top < work.top ? work.top : window.top;
    int x = window.left < work.left ? work.left : window.left;
    SetWindowPos(0,x,y,
        width + 2*(GetSystemMetrics(SM_CXEDGE)+GetSystemMetrics(SM_CXDLGFRAME)),
        height + GetSystemMetrics(SM_CYMENU) + 2*(GetSystemMetrics(SM_CYEDGE)+GetSystemMetrics(SM_CYDLGFRAME)) + GetSystemMetrics(SM_CYCAPTION),
        SWP_NOZORDER);
}

void CMainFrame::OnRva_00009A60(void) { OnRva_000099A0(320,240); }
void CMainFrame::OnRva_00009A70(void) { OnRva_000099A0(480,360); }
void CMainFrame::OnRva_00009A80(void) { OnRva_000099A0(640,480); }
