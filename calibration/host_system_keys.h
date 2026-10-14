#ifndef HERCULES_HOST_SYSTEM_KEYS_H
#define HERCULES_HOST_SYSTEM_KEYS_H

#include <afxwin.h>

#ifdef _AFXDLL
#error This measured receiver uses the pinned static MFC ABI.
#endif

/* PC-backed CFrameWnd base, 188-byte extent and complete virtual-slot identities.
 * Message-map storage and unimplemented message handlers remain dependencies.
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

protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};

#endif
