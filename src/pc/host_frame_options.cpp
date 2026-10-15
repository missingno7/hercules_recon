#include "../../calibration/host_system_keys.h"
#include "../../calibration/host_options_classes.h"
extern "C" {
extern DWORD g_exe_ba0230;
void __cdecl graphics_native_05a50(void);
void __cdecl cleanup_native_4b00(void);
void __cdecl cleanup_native_4ac0(void);
}
void CMainFrame::OnRva_00009620(void)
{
    COptionsSheet sheet("Options",0,0);
    ++g_exe_ba0230;
    graphics_native_05a50();
    cleanup_native_4b00();
    if (sheet.DoModal() == IDOK)
        cleanup_native_4ac0();
    --g_exe_ba0230;
}
