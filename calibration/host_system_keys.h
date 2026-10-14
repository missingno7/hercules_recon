#ifndef HERCULES_HOST_SYSTEM_KEYS_H
#define HERCULES_HOST_SYSTEM_KEYS_H

#include <afxwin.h>

#ifdef _AFXDLL
#error This measured receiver uses the pinned static MFC ABI.
#endif

/* PC-backed CFrameWnd base, 188-byte extent and complete virtual-slot identities.
 * Mapped callback signatures are measured; missing handler bodies remain dependencies.
 * Member access spelling and original translation-unit ownership are unproved. */
class CMainFrame : public CFrameWnd
{
public:
    CMainFrame();
    virtual ~CMainFrame();
    DECLARE_DYNCREATE(CMainFrame)
    DECLARE_MESSAGE_MAP()

public:
    void OnSysKeyDown(UINT key, UINT repeatCount, UINT flags);
    void OnSysKeyUp(UINT key, UINT repeatCount, UINT flags);

    /* Provisional nonvirtual callbacks: address/signature labels only. */
    afx_msg void OnRva_00009620(void);
    afx_msg void OnRva_00009820(BOOL, HTASK);
    afx_msg BOOL OnRva_000097A0(void);
    afx_msg void OnRva_000097C0(CWnd*);
    afx_msg void OnRva_000097F0(void);
    afx_msg void OnRva_00009840(void);
    afx_msg void OnRva_00009870(UINT, int, int);
    afx_msg void OnRva_00009740(void);
    afx_msg BOOL OnRva_00009900(CDC*);
    afx_msg void OnRva_00009910(UINT, LPARAM);
    afx_msg void OnRva_00009950(UINT, CPoint);
    afx_msg void OnRva_00009990(void);
    afx_msg void OnRva_00009A60(void);
    afx_msg void OnRva_00009A90(CCmdUI*);
    afx_msg BOOL OnRva_00009BC0(CWnd*, UINT, UINT);
    afx_msg void OnRva_00009970(CMenu*);
    afx_msg void OnRva_00009A70(void);
    afx_msg void OnRva_00009A80(void);
    afx_msg void OnRva_00009B40(UINT, UINT, UINT);
    afx_msg void OnRva_000096C0(void);
    afx_msg void OnRva_00009BF0(CCmdUI*);
    afx_msg void OnRva_00009C10(CCmdUI*);

protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};

#endif
