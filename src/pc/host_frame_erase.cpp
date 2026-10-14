#include "../../calibration/host_system_keys.h"

/* Original WM_ERASEBKGND map target at EXE RVA 0x9900. */
BOOL CMainFrame::OnRva_00009900(CDC*)
{
    return TRUE;
}
