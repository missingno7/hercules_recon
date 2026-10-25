#include "host_system_keys.h"

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

const AFX_MSGMAP* CMainFrame::GetMessageMap() const
{
    return &CMainFrame::messageMap;
}

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME);

    int nClientWidth = ::GetSystemMetrics(SM_CXEDGE);
    nClientWidth += ::GetSystemMetrics(SM_CXDLGFRAME);
    cs.cx = nClientWidth * 2 + 320;

    int nClientHeight = ::GetSystemMetrics(SM_CYEDGE);
    nClientHeight += ::GetSystemMetrics(SM_CYDLGFRAME);
    nClientHeight *= 2;
    nClientHeight += 240;
    nClientHeight += ::GetSystemMetrics(SM_CYMENU);
    nClientHeight += ::GetSystemMetrics(SM_CYCAPTION);
    cs.cy = nClientHeight;

    RECT rcWorkArea;
    ::SystemParametersInfoA(SPI_GETWORKAREA, 0, &rcWorkArea, 0);

    cs.x = rcWorkArea.right - rcWorkArea.left;
    cs.y = rcWorkArea.bottom - rcWorkArea.top;
    if (cs.cx >= cs.x)
    {
        cs.cx = cs.x;
        cs.x = 0;
    }
    else
    {
        cs.x = (cs.x - cs.cx) / 2 + rcWorkArea.left;
    }

    if (cs.cy >= cs.y)
    {
        cs.cy = cs.y;
        cs.y = 0;
    }
    else
    {
        cs.y = (cs.y - cs.cy) / 2 + rcWorkArea.top;
    }

    /* Retail performs three calls. Only the middle result supplies the resource
     * handle; the discarded calls are observable MFC boundaries. Their original
     * source spelling is unknown. Keep the calls, without altering MFC code. */
    (void)AfxGetModuleState();
    HINSTANCE hResourceInstance = AfxGetResourceHandle();
    (void)AfxGetModuleState();
    HCURSOR hCursor = ::LoadCursorA(NULL, MAKEINTRESOURCEA(IDC_ARROW));
    HICON hIcon = ::LoadIconA(hResourceInstance, MAKEINTRESOURCEA(0x80));
    cs.lpszClass = AfxRegisterWndClass(
        CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_BYTEALIGNCLIENT,
        hCursor, NULL, hIcon);

    return CFrameWnd::PreCreateWindow(cs);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
    return 0;
}
