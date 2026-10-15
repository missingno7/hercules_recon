#include "../../calibration/host_system_keys.h"
#include "../../calibration/host_restart_dialog.h"
#include "../../calibration/display_mode_contract.h"
extern "C" {
extern DWORD g_exe_ba0230;
extern int g_exe_91c44c;
extern DWORD g_startup_apply_guard_rva_51c450;
void graphics_native_05a50(void);
void fatal_shutdown_chain(void);
void cleanup_native_6bb0(void);
}
void CMainFrame::OnRva_000096C0()
{
    ++g_exe_ba0230;
    graphics_native_05a50();
    CHostRestartDialog dialog;
    dialog.DoModal();
    --g_exe_ba0230;
}
void CMainFrame::OnRva_000097F0()
{
    if (g_exe_91c44c == 1) {
        g_exe_91c44c = -1;
        return;
    }
    fatal_shutdown_chain();
    CFrameWnd::OnClose();
}
void CMainFrame::OnRva_00009990()
{
    cleanup_native_6ec0(1);
    cleanup_native_6bb0();
}
void CMainFrame::OnRva_00009B40(UINT key, UINT repeat, UINT flags)
{
    if (g_exe_91c44c == 0) {
        switch (key) {
        case VK_F2: {
            unsigned short previous = cleanup_native_6ec0(0);
            cleanup_native_6bb0();
            OnRva_00009620();
            cleanup_native_6ec0(previous);
            return;
        }
        case VK_F3: if (g_startup_apply_guard_rva_51c450 != 0) {
            unsigned short previous = cleanup_native_6ec0(0);
            cleanup_native_6bb0();
            OnRva_000096C0();
            cleanup_native_6ec0(previous);
            return;
        }
        }
    }
    Default();
}
