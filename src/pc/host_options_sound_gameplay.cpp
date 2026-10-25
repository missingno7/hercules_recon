#include "host_options_classes.h"
/* Original PC Options behavior; member spellings/TU ownership remain provisional. */
extern "C" {
extern BYTE g_registry_config[72];
extern DWORD g_startup_apply_guard_rva_51c450;
}

BOOL CSoundPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    SetWindowText("Sound");
    GetDlgItem(0x460)->SetWindowText("Sound Volume");
    m_sound_enabled = g_registry_config[0x0e] == 0;
    OnRva_00010390();
    m_music_enabled = g_registry_config[0x0d] == 0;
    OnRva_000103C0();
    m_control_045f.SetRange(0,0x3fff,TRUE);
    m_control_045f.SetPos(0x3fff - *(DWORD*)(g_registry_config+0x10));
    m_control_045f.SetTicFreq(0x800);
    return TRUE;
}

void CSoundPage::OnRva_00010390()
{
    m_sound_enabled = !m_sound_enabled;
    m_control_045d.SetWindowText(m_sound_enabled ? "Sounds are ON" : "Sounds are OFF");
}

void CSoundPage::OnRva_000103C0()
{
    m_music_enabled = !m_music_enabled;
    m_control_045e.SetWindowText(m_music_enabled ? "Music is ON" : "Music is OFF");
}

void CSoundPage::OnOK()
{
    g_registry_config[0x0e] = (BYTE)m_sound_enabled;
    g_registry_config[0x0d] = (BYTE)m_music_enabled;
    *(DWORD*)(g_registry_config+0x10) = 0x3fff - m_control_045f.GetPos();
    CPropertyPage::OnOK();
}

BOOL CGamePlayPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    SetWindowText("Game Play");
    GetDlgItem(0x467)->SetWindowText("These options are only available during the Title's main menu.");
    m_difficulty = (*(DWORD*)(g_registry_config+0x0c) & 0xff) - 1;
    OnRva_000062D0();
    m_lives = *(DWORD*)(g_registry_config+0x18) - 1;
    OnRva_00006330();
    m_control_0472.EnableWindow(g_startup_apply_guard_rva_51c450);
    m_control_0466.EnableWindow(g_startup_apply_guard_rva_51c450);
    m_control_0467.ShowWindow(g_startup_apply_guard_rva_51c450 ? SW_HIDE : SW_SHOWNA);
    m_control_0469.ShowWindow(g_startup_apply_guard_rva_51c450 ? SW_HIDE : SW_SHOWNA);
    return TRUE;
}

void CGamePlayPage::OnRva_000062D0()
{
    LPCTSTR labels[3] = {"Beginner Difficulty","Medium Difficulty","Herculean Difficulty"};
    if (++m_difficulty > 2)
        m_difficulty = 0;
    m_control_0472.SetWindowText(labels[m_difficulty]);
}

void CGamePlayPage::OnRva_00006330()
{
    LPCTSTR labels[3] = {"Two Lives","Three Lives","Four Lives"};
    if (++m_lives > 4)
        m_lives = 2;
    m_control_0466.SetWindowText(labels[m_lives-2]);
}

void CGamePlayPage::OnOK()
{
    g_registry_config[0x0c] = (BYTE)m_difficulty;
    *(DWORD*)(g_registry_config+0x18) = m_lives;
    CPropertyPage::OnOK();
}
