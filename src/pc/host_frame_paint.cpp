#include "../../calibration/host_system_keys.h"
extern "C" {
extern BYTE g_fatal_suppress_dialog;
extern BYTE g_exe_43a590;
void __cdecl graphics_native_06620(void);
}
typedef char frame_receiver_is_188[(sizeof(CMainFrame)==188)?1:-1];
typedef char paint_dc_is_84[(sizeof(CPaintDC)==84)?1:-1];
typedef char paint_struct_is_64[(sizeof(PAINTSTRUCT)==64)?1:-1];

/* Normal-path reconstruction. Current VC5 C-call context elides the original
 * inactive outer EH record; this is an explicit matching gap, not byte equality.
 * See evidence/host_frame_paint_recovery.json. */
void CMainFrame::OnRva_00009740(void)
{
    CPaintDC dc(this);
    if (g_fatal_suppress_dialog == 0) {
        g_exe_43a590 = 1;
        graphics_native_06620();
    }
}
