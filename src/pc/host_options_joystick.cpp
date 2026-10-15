#include "../../calibration/host_options_classes.h"
#include <mmsystem.h>
#include <stdio.h>
extern "C" {
extern BYTE g_registry_config[72];
extern BYTE g_host_input_joy_available_rva_79e8a0[16];
extern JOYCAPSA g_host_input_joy_caps_rva_79e8b0[16];
extern DWORD g_joystick_working_rva_56e5d8[7];
extern char g_resource_filename[];
DWORD host_input_sample_7c50(int);
void cleanup_native_7aa0(void);
}
#define SELECTED (*(int*)&g_joystick_working_rva_56e5d8[0])
#define SWORD g_joystick_working_rva_56e5d8[4]
#define JUMP g_joystick_working_rva_56e5d8[2]
#define PUNCH g_joystick_working_rva_56e5d8[5]
#define POWER g_joystick_working_rva_56e5d8[6]
#define WEAPON g_joystick_working_rva_56e5d8[1]
#define PAUSE g_joystick_working_rva_56e5d8[3]

void CJoystickPage::OnRva_000083F0(CEdit* edit, DWORD buttons, BOOL active)
{
    char text[32];
    buttons &= 0x0fffffff;
    if ((!active && buttons == 0) || (GetAsyncKeyState(VK_SPACE) & 0x8000)) {
        buttons = 0;
        edit->SetWindowText("None");
    } else {
        if (buttons == 0) return;
        int count = 0;
        for (int bit = 0; bit < 28; ++bit)
            if ((buttons >> bit) & 1) ++count;
        if (count > 3) return;
        edit->SetWindowText(g_resource_filename);
        for (int button = 0; button < 28; ++button) {
            if (buttons & (1UL << button)) {
                sprintf(text," %d",button+1);
                edit->ReplaceSel(text);
            }
        }
    }
    if (edit == &m_control_03ea) SWORD = buttons;
    else if (edit == &m_control_03eb) JUMP = buttons;
    else if (edit == &m_control_03ec) PUNCH = buttons;
    else if (edit == &m_control_03ed) POWER = buttons;
    else if (edit == &m_control_03ef) WEAPON = buttons;
    else if (edit == &m_control_03f0) PAUSE = buttons;
}

void CJoystickPage::OnTimer(UINT event)
{
    CPropertyPage::OnTimer(event);
    if (event == 0xfaf87c4e) {
        CWnd* focus = GetFocus();
        if (focus == &m_control_03ea || focus == &m_control_03eb ||
            focus == &m_control_03ec || focus == &m_control_03ed ||
            focus == &m_control_03ef || focus == &m_control_03f0) {
            DWORD buttons = host_input_sample_7c50(SELECTED);
            OnRva_000083F0((CEdit*)focus,buttons,TRUE);
        }
    }
}

BOOL CJoystickPage::OnInitDialog()
{
    char text[256];
    CPropertyPage::OnInitDialog();
    SetWindowText("GamePad");
    GetDlgItem(0x46b)->SetWindowText("Hold SPACE to clear a button selection.");
    GetDlgItem(0x46c)->SetWindowText("Sword");
    GetDlgItem(0x46d)->SetWindowText("Jump");
    GetDlgItem(0x46e)->SetWindowText("Punch");
    GetDlgItem(0x46f)->SetWindowText("Power Punch");
    GetDlgItem(0x470)->SetWindowText("Weapon");
    GetDlgItem(0x471)->SetWindowText("Pause");
    SELECTED = *(SHORT*)(g_registry_config+0x16);
    m_control_0431.ResetContent();
    int none = m_control_0431.InsertString(-1,"None");
    m_control_0431.SetItemData(none,(DWORD)-1);
    m_control_0431.SetCurSel(0);
    for (int device = 0; device < 16; ++device) {
        if (g_host_input_joy_available_rva_79e8a0[device]) {
            sprintf(text,"Joy %d, %d buttons",device+1,g_host_input_joy_caps_rva_79e8b0[device].wNumButtons);
            int item = m_control_0431.InsertString(-1,text);
            m_control_0431.SetItemData(item,device);
            if (SELECTED == device) m_control_0431.SetCurSel(item);
        }
    }
    OnRva_000083F0(&m_control_03ea,*(DWORD*)(g_registry_config+0x30),FALSE);
    OnRva_000083F0(&m_control_03eb,*(DWORD*)(g_registry_config+0x34),FALSE);
    OnRva_000083F0(&m_control_03ec,*(DWORD*)(g_registry_config+0x38),FALSE);
    OnRva_000083F0(&m_control_03ed,*(DWORD*)(g_registry_config+0x3c),FALSE);
    OnRva_000083F0(&m_control_03ef,*(DWORD*)(g_registry_config+0x40),FALSE);
    OnRva_000083F0(&m_control_03f0,*(DWORD*)(g_registry_config+0x44),FALSE);
    SetTimer(0xfaf87c4e,200,0);
    return TRUE;
}

void CJoystickPage::OnOK()
{
    *(WORD*)(g_registry_config+0x16) = (WORD)SELECTED;
    *(DWORD*)(g_registry_config+0x30) = SWORD;
    *(DWORD*)(g_registry_config+0x34) = JUMP;
    *(DWORD*)(g_registry_config+0x38) = PUNCH;
    *(DWORD*)(g_registry_config+0x3c) = POWER;
    *(DWORD*)(g_registry_config+0x40) = WEAPON;
    *(DWORD*)(g_registry_config+0x44) = PAUSE;
    cleanup_native_7aa0();
    CPropertyPage::OnOK();
}

void CJoystickPage::OnDestroy()
{
    CPropertyPage::OnDestroy();
    KillTimer(0xfaf87c4e);
}

void CJoystickPage::OnRva_00008850()
{
    SELECTED = (int)m_control_0431.GetItemData(m_control_0431.GetCurSel());
    SWORD = JUMP = PUNCH = POWER = WEAPON = PAUSE = 0;
    switch (g_host_input_joy_caps_rva_79e8b0[SELECTED].wNumButtons) {
    case 10:
        SWORD = 1; JUMP = 2; PUNCH = 4; POWER = 0x10; WEAPON = 0x80; PAUSE = 0x100;
        break;
    default:
    case 6: PAUSE = 0x20;
    case 5: WEAPON = 0x10;
    case 4: PUNCH = 8;
    case 3: JUMP = 4;
    case 2: POWER = 2;
    case 1: SWORD = 1;
    case 0: break;
    }
    OnRva_000083F0(&m_control_03ea,SWORD,FALSE);
    OnRva_000083F0(&m_control_03eb,JUMP,FALSE);
    OnRva_000083F0(&m_control_03ec,PUNCH,FALSE);
    OnRva_000083F0(&m_control_03ed,POWER,FALSE);
    OnRva_000083F0(&m_control_03ef,WEAPON,FALSE);
    OnRva_000083F0(&m_control_03f0,PAUSE,FALSE);
}
