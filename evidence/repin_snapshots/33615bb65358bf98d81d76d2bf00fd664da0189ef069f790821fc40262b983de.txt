#include "host_options_classes.h"
extern "C" {
extern BYTE g_registry_config[72];
extern DWORD g_config_default_values_rva_36158[9];
extern LPCSTR g_keyboard_labels_rva_36cf0[256];
extern DWORD g_keyboard_working_rva_56e5f4[9];
}

BOOL CKeyboardPage::OnRva_00008DD0(HWND window, SHORT key, BOOL clear_conflict)
{
    key &= 0xff;
    WORD* slot;
    WORD* previous_slot = 0;
    HWND previous_window = 0;
    if (g_keyboard_labels_rva_36cf0[key] == (LPCSTR)-1)
        return FALSE;
    if (m_control_040b.m_hWnd == window) slot = (WORD*)&g_keyboard_working_rva_56e5f4[2];
    else if (m_control_040c.m_hWnd == window) slot = (WORD*)&g_keyboard_working_rva_56e5f4[8];
    else if (m_control_040e.m_hWnd == window) slot = (WORD*)&g_keyboard_working_rva_56e5f4[0];
    else if (m_control_040f.m_hWnd == window) slot = (WORD*)&g_keyboard_working_rva_56e5f4[3];
    else if (m_control_0410.m_hWnd == window) slot = (WORD*)&g_keyboard_working_rva_56e5f4[6];
    else if (m_control_0411.m_hWnd == window) slot = (WORD*)&g_keyboard_working_rva_56e5f4[1];
    else if (m_control_0412.m_hWnd == window) slot = (WORD*)&g_keyboard_working_rva_56e5f4[7];
    else if (m_control_0413.m_hWnd == window) slot = (WORD*)&g_keyboard_working_rva_56e5f4[5];
    else if (m_control_0416.m_hWnd == window) slot = (WORD*)&g_keyboard_working_rva_56e5f4[4];
    else return FALSE;
    if (clear_conflict) {
#define CONFLICT(index,member) \
        if (slot != (WORD*)&g_keyboard_working_rva_56e5f4[index] && \
            *(SHORT*)&g_keyboard_working_rva_56e5f4[index] == key) { \
            previous_window = member.m_hWnd; \
            previous_slot = (WORD*)&g_keyboard_working_rva_56e5f4[index]; \
        }
        CONFLICT(2,m_control_040b)
        CONFLICT(8,m_control_040c)
        CONFLICT(0,m_control_040e)
        CONFLICT(3,m_control_040f)
        CONFLICT(6,m_control_0410)
        CONFLICT(1,m_control_0411)
        CONFLICT(7,m_control_0412)
        CONFLICT(5,m_control_0413)
        CONFLICT(4,m_control_0416)
#undef CONFLICT
        if (previous_window) {
            ::SetWindowText(previous_window,"None");
            *previous_slot = 0;
        }
    }
    *slot = key;
    LPCSTR label = g_keyboard_labels_rva_36cf0[key];
    if (label == 0) {
        UINT character = MapVirtualKey(key,2);
        if (!(character & 0x80) && character != 0) {
            char text[2];
            text[0] = (char)character;
            text[1] = 0;
            ::SetWindowText(window,text);
        } else ::SetWindowText(window,"None");
    } else ::SetWindowText(window,label);
    return TRUE;
}

BOOL CKeyboardPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    SetWindowText("Keyboard");
    GetDlgItem(0x44b)->SetWindowText("Reset Keys");
    GetDlgItem(0x472)->SetWindowText("Up");
    GetDlgItem(0x473)->SetWindowText("Down");
    GetDlgItem(0x474)->SetWindowText("Left");
    GetDlgItem(0x475)->SetWindowText("Right");
    GetDlgItem(0x476)->SetWindowText("Sword");
    GetDlgItem(0x477)->SetWindowText("Jump");
    GetDlgItem(0x478)->SetWindowText("Punch");
    GetDlgItem(0x479)->SetWindowText("Power Punch");
    GetDlgItem(0x47a)->SetWindowText("Weapon");
    OnRva_00008DD0(m_control_040b.m_hWnd,*(SHORT*)(g_registry_config+0x1c),FALSE);
    OnRva_00008DD0(m_control_040c.m_hWnd,*(SHORT*)(g_registry_config+0x1e),FALSE);
    OnRva_00008DD0(m_control_040e.m_hWnd,*(SHORT*)(g_registry_config+0x20),FALSE);
    OnRva_00008DD0(m_control_040f.m_hWnd,*(SHORT*)(g_registry_config+0x22),FALSE);
    OnRva_00008DD0(m_control_0410.m_hWnd,*(SHORT*)(g_registry_config+0x24),FALSE);
    OnRva_00008DD0(m_control_0411.m_hWnd,*(SHORT*)(g_registry_config+0x26),FALSE);
    OnRva_00008DD0(m_control_0412.m_hWnd,*(SHORT*)(g_registry_config+0x28),FALSE);
    OnRva_00008DD0(m_control_0413.m_hWnd,*(SHORT*)(g_registry_config+0x2a),FALSE);
    OnRva_00008DD0(m_control_0416.m_hWnd,*(SHORT*)(g_registry_config+0x2c),FALSE);
    m_keyboard_enabled = g_registry_config[0x14] == 0;
    OnRva_000092C0();
    return TRUE;
}

void CKeyboardPage::OnOK()
{
    *(WORD*)(g_registry_config+0x1c) = (WORD)g_keyboard_working_rva_56e5f4[2];
    *(WORD*)(g_registry_config+0x1e) = (WORD)g_keyboard_working_rva_56e5f4[8];
    *(WORD*)(g_registry_config+0x20) = (WORD)g_keyboard_working_rva_56e5f4[0];
    *(WORD*)(g_registry_config+0x22) = (WORD)g_keyboard_working_rva_56e5f4[3];
    *(WORD*)(g_registry_config+0x24) = (WORD)g_keyboard_working_rva_56e5f4[6];
    *(WORD*)(g_registry_config+0x26) = (WORD)g_keyboard_working_rva_56e5f4[1];
    *(WORD*)(g_registry_config+0x28) = (WORD)g_keyboard_working_rva_56e5f4[7];
    *(WORD*)(g_registry_config+0x2a) = (WORD)g_keyboard_working_rva_56e5f4[5];
    *(WORD*)(g_registry_config+0x2c) = (WORD)g_keyboard_working_rva_56e5f4[4];
    g_registry_config[0x14] = (BYTE)m_keyboard_enabled;
    CPropertyPage::OnOK();
}

void CKeyboardPage::OnRva_000092C0()
{
    m_keyboard_enabled = !m_keyboard_enabled;
    m_control_0424.SetWindowText(m_keyboard_enabled ? "Keys are ON" : "Keys are OFF");
}

BOOL CKeyboardPage::PreTranslateMessage(MSG* message)
{
    if (message->message == WM_KEYDOWN && OnRva_00008DD0(message->hwnd,(SHORT)message->wParam,TRUE))
        return TRUE;
    return CPropertyPage::PreTranslateMessage(message);
}

void CKeyboardPage::OnReset()
{
    OnRva_00008DD0(m_control_040b.m_hWnd,*(SHORT*)&g_config_default_values_rva_36158[0],TRUE);
    OnRva_00008DD0(m_control_040c.m_hWnd,*(SHORT*)&g_config_default_values_rva_36158[1],TRUE);
    OnRva_00008DD0(m_control_040e.m_hWnd,*(SHORT*)&g_config_default_values_rva_36158[2],TRUE);
    OnRva_00008DD0(m_control_040f.m_hWnd,*(SHORT*)&g_config_default_values_rva_36158[3],TRUE);
    OnRva_00008DD0(m_control_0410.m_hWnd,*(SHORT*)&g_config_default_values_rva_36158[4],TRUE);
    OnRva_00008DD0(m_control_0411.m_hWnd,*(SHORT*)&g_config_default_values_rva_36158[5],TRUE);
    OnRva_00008DD0(m_control_0412.m_hWnd,*(SHORT*)&g_config_default_values_rva_36158[6],TRUE);
    OnRva_00008DD0(m_control_0413.m_hWnd,*(SHORT*)&g_config_default_values_rva_36158[7],TRUE);
    OnRva_00008DD0(m_control_0416.m_hWnd,*(SHORT*)&g_config_default_values_rva_36158[8],TRUE);
}
