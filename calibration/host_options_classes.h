#ifndef HERCULES_HOST_OPTIONS_CLASSES_H
#define HERCULES_HOST_OPTIONS_CLASSES_H
#include <afxwin.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#ifdef _AFXDLL
#error This measured class region uses pinned static MFC.
#endif
/* Class names are backed by PC runtime metadata. Member names/access and original
 * translation units remain unproved. Uninitialized scalar members are deliberate. */
class CKeyboardPage : public CPropertyPage
{
public:
    CKeyboardPage();
    virtual ~CKeyboardPage();
    DECLARE_DYNCREATE(CKeyboardPage)
    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual BOOL PreTranslateMessage(MSG*);
    virtual void OnReset();
    afx_msg void OnRva_000092C0();
    afx_msg void OnRva_00009410();
protected:
    virtual void DoDataExchange(CDataExchange*);
public:
    CEdit m_control_0416;
    CButton m_control_0424;
    CEdit m_control_040b;
    CEdit m_control_040c;
    CEdit m_control_040e;
    CEdit m_control_040f;
    CEdit m_control_0410;
    CEdit m_control_0411;
    CEdit m_control_0412;
    CEdit m_control_0413;
    BOOL m_native_2e4;
};

class CJoystickPage : public CPropertyPage
{
public:
    CJoystickPage();
    virtual ~CJoystickPage();
    DECLARE_DYNCREATE(CJoystickPage)
    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT);
    afx_msg void OnRva_00008850();
protected:
    virtual void DoDataExchange(CDataExchange*);
public:
    CEdit m_control_03f0;
    CEdit m_control_03ef;
    CEdit m_control_03ea;
    CEdit m_control_03eb;
    CEdit m_control_03ec;
    CEdit m_control_03ed;
    CListBox m_control_0431;
};

class CSoundPage : public CPropertyPage
{
public:
    CSoundPage();
    virtual ~CSoundPage();
    DECLARE_DYNCREATE(CSoundPage)
    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnRva_00010390();
    afx_msg void OnRva_000103C0();
protected:
    virtual void DoDataExchange(CDataExchange*);
public:
    CSliderCtrl m_control_045f;
    CButton m_control_045d;
    CButton m_control_045e;
    BOOL m_sound_enabled;
    BOOL m_music_enabled;
};

class CGamePlayPage : public CPropertyPage
{
public:
    CGamePlayPage();
    virtual ~CGamePlayPage();
    DECLARE_DYNCREATE(CGamePlayPage)
    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnRva_000062D0();
    afx_msg void OnRva_00006330();
protected:
    virtual void DoDataExchange(CDataExchange*);
public:
    CStatic m_control_0469;
    CStatic m_control_0467;
    CButton m_control_0466;
    CButton m_control_0472;
    int m_difficulty;
    int m_lives;
};

class COptionsSheet : public CPropertySheet
{
public:
    COptionsSheet(LPCTSTR, CWnd* = 0, UINT = 0);
    virtual ~COptionsSheet();
    DECLARE_DYNAMIC(COptionsSheet)
    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    CKeyboardPage m_keyboard;
    CJoystickPage m_joystick;
    CSoundPage m_sound;
    CGamePlayPage m_gameplay;
};
#endif
