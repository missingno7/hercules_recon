#include "host_system_keys.h"
#include "host_startup_storage.h"

extern "C" {
extern BYTE g_exe_92c488;
extern int g_exe_91c44c;
}

void CMainFrame::OnRva_00009A90(CCmdUI *command)
{
    command->Enable((signed char)g_exe_92c488);
}

void CMainFrame::OnRva_00009BF0(CCmdUI *command)
{
    command->Enable(g_startup_apply_guard_rva_51c450);
}

void CMainFrame::OnRva_00009C10(CCmdUI *command)
{
    command->Enable(!g_exe_91c44c);
}
