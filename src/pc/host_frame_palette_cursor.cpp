#include "host_system_keys.h"
extern "C" {
extern BYTE g_fatal_suppress_dialog;
UINT __cdecl graphics_native_06700(int);
}
/* Original PC callback address labels; historical names/TU remain unproved. */
typedef char frame_receiver_is_188[(sizeof(CMainFrame)==188)?1:-1];

BOOL CMainFrame::OnRva_000097A0(void)
{
    return graphics_native_06700(GetCurrentMessage()->message);
}

void CMainFrame::OnRva_000097C0(CWnd *focus)
{
    if (focus != this && !IsChild(focus))
        OnRva_000097A0();
}

BOOL CMainFrame::OnRva_00009BC0(CWnd *window, UINT hit, UINT message)
{
    if (g_fatal_suppress_dialog != 0) {
        ::SetCursor(0);
        return TRUE;
    }
    return CFrameWnd::OnSetCursor(window,hit,message);
}
