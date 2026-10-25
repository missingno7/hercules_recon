#include "host_system_keys.h"

/* Explicit static MFC map data. GetMessageMap is defined by host_window_class.cpp. */
AFX_DATADEF const AFX_MSGMAP CMainFrame::messageMap =
    { &CFrameWnd::messageMap, &CMainFrame::_messageEntries[0] };

#define HERCULES_WINDOW_ENTRY(message, signature, member, member_type) \
    { message, 0, 0, 0, signature, \
      (AFX_PMSG)(AFX_PMSGW)(member_type)&CMainFrame::member }

const AFX_MSGMAP_ENTRY CMainFrame::_messageEntries[] =
{
    ON_WM_CREATE()
    ON_COMMAND(0x8006, OnRva_00009620)
    HERCULES_WINDOW_ENTRY(WM_ACTIVATEAPP, AfxSig_vbh, OnRva_00009820,
        void (AFX_MSG_CALL CWnd::*)(BOOL, HTASK)),
    HERCULES_WINDOW_ENTRY(WM_QUERYNEWPALETTE, AfxSig_bv, OnRva_000097A0,
        BOOL (AFX_MSG_CALL CWnd::*)(void)),
    HERCULES_WINDOW_ENTRY(WM_PALETTECHANGED, AfxSig_vW, OnRva_000097C0,
        void (AFX_MSG_CALL CWnd::*)(CWnd*)),
    HERCULES_WINDOW_ENTRY(WM_CLOSE, AfxSig_vv, OnRva_000097F0,
        void (AFX_MSG_CALL CWnd::*)(void)),
    ON_COMMAND(0xE146, OnRva_00009840)
    HERCULES_WINDOW_ENTRY(WM_SIZE, AfxSig_vwii, OnRva_00009870,
        void (AFX_MSG_CALL CWnd::*)(UINT, int, int)),
    HERCULES_WINDOW_ENTRY(WM_PAINT, AfxSig_vv, OnRva_00009740,
        void (AFX_MSG_CALL CWnd::*)(void)),
    HERCULES_WINDOW_ENTRY(WM_ERASEBKGND, AfxSig_bD, OnRva_00009900,
        BOOL (AFX_MSG_CALL CWnd::*)(CDC*)),
    HERCULES_WINDOW_ENTRY(WM_SYSCOMMAND, AfxSig_vwl, OnRva_00009910,
        void (AFX_MSG_CALL CWnd::*)(UINT, LPARAM)),
    HERCULES_WINDOW_ENTRY(WM_NCLBUTTONDOWN, AfxSig_vwp, OnRva_00009950,
        void (AFX_MSG_CALL CWnd::*)(UINT, CPoint)),
    HERCULES_WINDOW_ENTRY(WM_NCMBUTTONDOWN, AfxSig_vwp, OnRva_00009950,
        void (AFX_MSG_CALL CWnd::*)(UINT, CPoint)),
    HERCULES_WINDOW_ENTRY(WM_NCRBUTTONDOWN, AfxSig_vwp, OnRva_00009950,
        void (AFX_MSG_CALL CWnd::*)(UINT, CPoint)),
    ON_COMMAND(0x8020, OnRva_00009990)
    ON_COMMAND(0x8021, OnRva_00009A60)
    ON_UPDATE_COMMAND_UI(0x8020, OnRva_00009A90)
    HERCULES_WINDOW_ENTRY(WM_SETCURSOR, AfxSig_bWww, OnRva_00009BC0,
        BOOL (AFX_MSG_CALL CWnd::*)(CWnd*, UINT, UINT)),
    ON_WM_SYSKEYDOWN()
    ON_WM_SYSKEYUP()
    HERCULES_WINDOW_ENTRY(WM_INITMENU, AfxSig_vM, OnRva_00009970,
        void (AFX_MSG_CALL CWnd::*)(CMenu*)),
    ON_COMMAND(0x8064, OnRva_00009A70)
    ON_COMMAND(0x8063, OnRva_00009A80)
    HERCULES_WINDOW_ENTRY(WM_KEYDOWN, AfxSig_vwww, OnRva_00009B40,
        void (AFX_MSG_CALL CWnd::*)(UINT, UINT, UINT)),
    ON_COMMAND(0x8065, OnRva_000096C0)
    ON_UPDATE_COMMAND_UI(0x8065, OnRva_00009BF0)
    ON_UPDATE_COMMAND_UI(0x8006, OnRva_00009C10)
    ON_COMMAND(0xE143, OnHelpFinder)
    ON_COMMAND(0xE146, OnHelp)
    ON_COMMAND(0xE147, OnHelpFinder)
    { 0, 0, 0, 0, AfxSig_end, (AFX_PMSG)0 }
};
