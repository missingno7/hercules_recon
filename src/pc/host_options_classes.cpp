#include "host_options_classes.h"
#include <stddef.h>
/* Coherent recovered class region; original TU grouping is not asserted. */

IMPLEMENT_DYNCREATE(CKeyboardPage, CPropertyPage)

CKeyboardPage::CKeyboardPage() : CPropertyPage(0x8e)
{
}

CKeyboardPage::~CKeyboardPage()
{
}

void CKeyboardPage::DoDataExchange(CDataExchange* dx)
{
    CPropertyPage::DoDataExchange(dx);
    DDX_Control(dx,0x416,m_control_0416);
    DDX_Control(dx,0x424,m_control_0424);
    DDX_Control(dx,0x40b,m_control_040b);
    DDX_Control(dx,0x40c,m_control_040c);
    DDX_Control(dx,0x40e,m_control_040e);
    DDX_Control(dx,0x40f,m_control_040f);
    DDX_Control(dx,0x410,m_control_0410);
    DDX_Control(dx,0x411,m_control_0411);
    DDX_Control(dx,0x412,m_control_0412);
    DDX_Control(dx,0x413,m_control_0413);
}

BEGIN_MESSAGE_MAP(CKeyboardPage, CPropertyPage)
    ON_BN_CLICKED(0x424, OnRva_000092C0)
    ON_BN_CLICKED(0x44b, OnReset)
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(CJoystickPage, CPropertyPage)

CJoystickPage::CJoystickPage() : CPropertyPage(0x8f)
{
}

CJoystickPage::~CJoystickPage()
{
}

void CJoystickPage::DoDataExchange(CDataExchange* dx)
{
    CPropertyPage::DoDataExchange(dx);
    DDX_Control(dx,0x3f0,m_control_03f0);
    DDX_Control(dx,0x3ef,m_control_03ef);
    DDX_Control(dx,0x3ea,m_control_03ea);
    DDX_Control(dx,0x3eb,m_control_03eb);
    DDX_Control(dx,0x3ec,m_control_03ec);
    DDX_Control(dx,0x3ed,m_control_03ed);
    DDX_Control(dx,0x431,m_control_0431);
}

BEGIN_MESSAGE_MAP(CJoystickPage, CPropertyPage)
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_LBN_SELCHANGE(0x431, OnRva_00008850)
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(CSoundPage, CPropertyPage)

CSoundPage::CSoundPage() : CPropertyPage(0xb2)
{
}

CSoundPage::~CSoundPage()
{
}

void CSoundPage::DoDataExchange(CDataExchange* dx)
{
    CPropertyPage::DoDataExchange(dx);
    DDX_Control(dx,0x45f,m_control_045f);
    DDX_Control(dx,0x45d,m_control_045d);
    DDX_Control(dx,0x45e,m_control_045e);
}

BEGIN_MESSAGE_MAP(CSoundPage, CPropertyPage)
    ON_BN_CLICKED(0x45d, OnRva_00010390)
    ON_BN_CLICKED(0x45e, OnRva_000103C0)
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(CGamePlayPage, CPropertyPage)

CGamePlayPage::CGamePlayPage() : CPropertyPage(0xb3)
{
}

CGamePlayPage::~CGamePlayPage()
{
}

void CGamePlayPage::DoDataExchange(CDataExchange* dx)
{
    CPropertyPage::DoDataExchange(dx);
    DDX_Control(dx,0x469,m_control_0469);
    DDX_Control(dx,0x467,m_control_0467);
    DDX_Control(dx,0x466,m_control_0466);
    DDX_Control(dx,0x472,m_control_0472);
}

BEGIN_MESSAGE_MAP(CGamePlayPage, CPropertyPage)
    ON_BN_CLICKED(0x472, OnRva_000062D0)
    ON_BN_CLICKED(0x466, OnRva_00006330)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(COptionsSheet, CPropertySheet)

COptionsSheet::COptionsSheet(LPCTSTR caption, CWnd* parent, UINT selected)
    : CPropertySheet(caption,parent,selected)
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    AddPage(&m_keyboard);
    AddPage(&m_joystick);
    AddPage(&m_sound);
    AddPage(&m_gameplay);
}

COptionsSheet::~COptionsSheet()
{
}

BEGIN_MESSAGE_MAP(COptionsSheet, CPropertySheet)
END_MESSAGE_MAP()

typedef char options_base_page_140[(sizeof(CPropertyPage)==140)?1:-1];
typedef char options_base_sheet_136[(sizeof(CPropertySheet)==136)?1:-1];
typedef char options_control_60[(sizeof(CWnd)==60)?1:-1];
typedef char options_map_entry_24[(sizeof(AFX_MSGMAP_ENTRY)==24)?1:-1];
typedef char size_CKeyboardPage[(sizeof(CKeyboardPage)==744)?1:-1];
typedef char offset_CKeyboardPage_0416[(offsetof(CKeyboardPage,m_control_0416)==0x8c)?1:-1];
typedef char offset_CKeyboardPage_0424[(offsetof(CKeyboardPage,m_control_0424)==0xc8)?1:-1];
typedef char offset_CKeyboardPage_040b[(offsetof(CKeyboardPage,m_control_040b)==0x104)?1:-1];
typedef char offset_CKeyboardPage_040c[(offsetof(CKeyboardPage,m_control_040c)==0x140)?1:-1];
typedef char offset_CKeyboardPage_040e[(offsetof(CKeyboardPage,m_control_040e)==0x17c)?1:-1];
typedef char offset_CKeyboardPage_040f[(offsetof(CKeyboardPage,m_control_040f)==0x1b8)?1:-1];
typedef char offset_CKeyboardPage_0410[(offsetof(CKeyboardPage,m_control_0410)==0x1f4)?1:-1];
typedef char offset_CKeyboardPage_0411[(offsetof(CKeyboardPage,m_control_0411)==0x230)?1:-1];
typedef char offset_CKeyboardPage_0412[(offsetof(CKeyboardPage,m_control_0412)==0x26c)?1:-1];
typedef char offset_CKeyboardPage_0413[(offsetof(CKeyboardPage,m_control_0413)==0x2a8)?1:-1];
typedef char size_CJoystickPage[(sizeof(CJoystickPage)==560)?1:-1];
typedef char offset_CJoystickPage_03f0[(offsetof(CJoystickPage,m_control_03f0)==0x8c)?1:-1];
typedef char offset_CJoystickPage_03ef[(offsetof(CJoystickPage,m_control_03ef)==0xc8)?1:-1];
typedef char offset_CJoystickPage_03ea[(offsetof(CJoystickPage,m_control_03ea)==0x104)?1:-1];
typedef char offset_CJoystickPage_03eb[(offsetof(CJoystickPage,m_control_03eb)==0x140)?1:-1];
typedef char offset_CJoystickPage_03ec[(offsetof(CJoystickPage,m_control_03ec)==0x17c)?1:-1];
typedef char offset_CJoystickPage_03ed[(offsetof(CJoystickPage,m_control_03ed)==0x1b8)?1:-1];
typedef char offset_CJoystickPage_0431[(offsetof(CJoystickPage,m_control_0431)==0x1f4)?1:-1];
typedef char size_CSoundPage[(sizeof(CSoundPage)==328)?1:-1];
typedef char offset_CSoundPage_045f[(offsetof(CSoundPage,m_control_045f)==0x8c)?1:-1];
typedef char offset_CSoundPage_045d[(offsetof(CSoundPage,m_control_045d)==0xc8)?1:-1];
typedef char offset_CSoundPage_045e[(offsetof(CSoundPage,m_control_045e)==0x104)?1:-1];
typedef char size_CGamePlayPage[(sizeof(CGamePlayPage)==388)?1:-1];
typedef char offset_CGamePlayPage_0469[(offsetof(CGamePlayPage,m_control_0469)==0x8c)?1:-1];
typedef char offset_CGamePlayPage_0467[(offsetof(CGamePlayPage,m_control_0467)==0xc8)?1:-1];
typedef char offset_CGamePlayPage_0466[(offsetof(CGamePlayPage,m_control_0466)==0x104)?1:-1];
typedef char offset_CGamePlayPage_0472[(offsetof(CGamePlayPage,m_control_0472)==0x140)?1:-1];
typedef char options_sheet_2156[(sizeof(COptionsSheet)==2156)?1:-1];
typedef char sheet_keyboard_offset[(offsetof(COptionsSheet,m_keyboard)==0x88)?1:-1];
typedef char sheet_joystick_offset[(offsetof(COptionsSheet,m_joystick)==0x370)?1:-1];
typedef char sheet_sound_offset[(offsetof(COptionsSheet,m_sound)==0x5a0)?1:-1];
typedef char sheet_gameplay_offset[(offsetof(COptionsSheet,m_gameplay)==0x6e8)?1:-1];
