#ifndef HERCULES_HOST_SYSTEM_KEYS_H
#define HERCULES_HOST_SYSTEM_KEYS_H

#include <afxwin.h>

#ifdef _AFXDLL
#error This measured receiver uses the pinned static MFC ABI.
#endif

/* Linked runtime identity CMainFrame, CFrameWnd base and 188-byte extent are
 * PC-backed. These member declarations do not supply the remaining original
 * virtual overrides, message map, constructors or window creation behavior. */
class CMainFrame : public CFrameWnd
{
public:
    void OnSysKeyDown(UINT key, UINT repeatCount, UINT flags);
    void OnSysKeyUp(UINT key, UINT repeatCount, UINT flags);
};

#endif
