#include "host_restart_dialog.h"
extern "C" {
extern BYTE g_registry_config[72];
int cleanup_native_4b50(int);
}
typedef char restart_dialog_size[(sizeof(CHostRestartDialog)==92)?1:-1];
CHostRestartDialog::CHostRestartDialog(CWnd* parent) : CDialog(0xb5,parent) {}
BEGIN_MESSAGE_MAP(CHostRestartDialog,CDialog)
    ON_BN_CLICKED(0x470,OnRva_0000F080)
    ON_BN_CLICKED(0x46c,OnRva_0000F000)
    ON_BN_CLICKED(0x46b,OnRva_0000EFE0)
    ON_BN_CLICKED(0x46e,OnRva_0000F040)
    ON_BN_CLICKED(0x46f,OnRva_0000F060)
    ON_BN_CLICKED(0x46d,OnRva_0000F020)
    ON_BN_CLICKED(0x471,OnRva_0000F0A0)
    ON_BN_CLICKED(0x472,OnRva_0000F0C0)
    ON_BN_CLICKED(0x46a,OnRva_0000EFC0)
    ON_BN_CLICKED(0x473,OnRva_0000F0E0)
END_MESSAGE_MAP()
#define RESTART_HANDLER(name,level) void CHostRestartDialog::name() { cleanup_native_4b50(level); EndDialog(0); }
RESTART_HANDLER(OnRva_0000EFC0,1)
RESTART_HANDLER(OnRva_0000EFE0,2)
RESTART_HANDLER(OnRva_0000F000,3)
RESTART_HANDLER(OnRva_0000F020,5)
RESTART_HANDLER(OnRva_0000F040,6)
RESTART_HANDLER(OnRva_0000F060,7)
RESTART_HANDLER(OnRva_0000F080,8)
RESTART_HANDLER(OnRva_0000F0A0,9)
RESTART_HANDLER(OnRva_0000F0C0,10)
RESTART_HANDLER(OnRva_0000F0E0,11)
#undef RESTART_HANDLER
BOOL CHostRestartDialog::OnInitDialog()
{
    CDialog::OnInitDialog();
    SetWindowText("Restart Level");
    GetDlgItem(0x46a)->SetWindowText("Training");
    GetDlgItem(0x46b)->SetWindowText("Gauntlet");
    GetDlgItem(0x46c)->SetWindowText("Forest");
    GetDlgItem(0x46d)->SetWindowText("Thebes");
    GetDlgItem(0x46e)->SetWindowText("Hydra");
    GetDlgItem(0x46f)->SetWindowText("Medusa");
    GetDlgItem(0x470)->SetWindowText("Cyclops");
    GetDlgItem(0x471)->SetWindowText("Titans");
    GetDlgItem(0x472)->SetWindowText("Torment");
    GetDlgItem(0x473)->SetWindowText("Vortex");
    GetDlgItem(IDCANCEL)->SetWindowText("CANCEL");
#define ENABLE_LEVEL(id,level) GetDlgItem(id)->EnableWindow(g_registry_config[4+(*(DWORD*)(g_registry_config+12)&0xff)] >= level);
    ENABLE_LEVEL(0x46a,1)
    ENABLE_LEVEL(0x46b,2)
    ENABLE_LEVEL(0x46c,3)
    ENABLE_LEVEL(0x46d,5)
    ENABLE_LEVEL(0x46e,6)
    ENABLE_LEVEL(0x46f,7)
    ENABLE_LEVEL(0x470,8)
    ENABLE_LEVEL(0x471,9)
    ENABLE_LEVEL(0x472,10)
    ENABLE_LEVEL(0x473,11)
#undef ENABLE_LEVEL
    return TRUE;
}
