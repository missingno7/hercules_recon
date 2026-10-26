/* HERCULES.EXE application object unit 0x75d0..0x7aa0 (lane h4 candidate).
 * CWinApp-derived class: message map (0x42e170: ON_COMMAND 0x800e about box plus the
 * CWinApp OnFileNew/OnFileOpen entries), constructor, compiler-generated destructors,
 * the global instance's dynamic initializer/atexit pair, InitInstance (0x7670),
 * ExitInstance, the about handler and an SEH-guarded OnIdle.
 * Class/member names are provisional (no runtime-class record exists); TU unproved. */
#include "host_system_keys.h"
#include <stdio.h>

extern "C" {
extern HWND g_fatal_owner_window;
extern signed char g_exe_ba0238;
extern BYTE g_exe_ba0228;
extern DWORD g_exe_ba0230;
extern HINSTANCE g_exe_ba01f4;
extern LPCTSTR g_exe_ba01f8;
extern HMENU g_exe_ba0204;

extern int __cdecl host_registry_read_path(void);
extern void __cdecl host_startup_5e10(void);
extern void __cdecl graphics_native_05a50(void);
extern void __cdecl cleanup_native_6bb0(void);
extern void __cdecl host_timing_dispatch_5f20(void);
extern void __cdecl archive_fatal(const char *message);
}

class CHerculesApp : public CWinApp
{
public:
    CHerculesApp();
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    virtual BOOL OnIdle(LONG lCount);
    afx_msg void OnAppAbout();
    DECLARE_MESSAGE_MAP()

public:
    HANDLE m_hInstanceMapping;
};

typedef char app_mapping_offset_c0[(offsetof(CHerculesApp, m_hInstanceMapping) == 0xc0) ? 1 : -1];

BEGIN_MESSAGE_MAP(CHerculesApp, CWinApp)
    ON_COMMAND(0x800e, OnAppAbout)
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

CHerculesApp::CHerculesApp()
{
}

CHerculesApp theApp;

BOOL CHerculesApp::InitInstance()
{
    char path[1024];
    char drive[1024];

    Enable3dControlsStatic();
    m_hInstanceMapping = CreateFileMappingA((HANDLE)-1, NULL, PAGE_READONLY, 0, 0x20, "Hercules");
    if (GetLastError() != 0)
        return FALSE;
    if (!host_registry_read_path()) {
        MessageBoxA(NULL, "Hercules is not installed.\nRun SETUP from the CD.", "Hercules Notice",
                    MB_ICONHAND);
        return FALSE;
    }
    GetModuleFileNameA(AfxGetInstanceHandle(), path, sizeof(path));
    _splitpath(path, drive, NULL, NULL, NULL);
    if (GetDriveTypeA(strcat(drive, "\\")) != DRIVE_FIXED) {
        MessageBoxA(NULL, "Cannot start Hercules from here.\nRun the installed copy.", "Hercules Notice",
                    MB_ICONHAND);
        return FALSE;
    }

    CMainFrame *pFrame = new CMainFrame;
    if (!pFrame->LoadFrame(0x80))
        return FALSE;
    m_pMainWnd = pFrame;
    g_fatal_owner_window = m_pMainWnd->m_hWnd;
    g_exe_ba01f4 = m_hInstance;
    g_exe_ba01f8 = m_pszAppName;
    g_exe_ba0204 = GetMenu(g_fatal_owner_window);
    ModifyMenuA(g_exe_ba0204, 0, MF_BYPOSITION, 0, "&Game");
    ModifyMenuA(g_exe_ba0204, 0x8006, MF_BYCOMMAND, 0x8006, "&Options\tF2");
    ModifyMenuA(g_exe_ba0204, 0x8065, MF_BYCOMMAND, 0x8065, "&Restart Level\tF3");
    ModifyMenuA(g_exe_ba0204, 0xe141, MF_BYCOMMAND, 0xe141, "E&xit");
    ModifyMenuA(g_exe_ba0204, 1, MF_BYPOSITION, 1, "&Display");
    ModifyMenuA(g_exe_ba0204, 0x8020, MF_BYCOMMAND, 0x8020, "320x240 Fullscreen\tAlt+Enter");
    ModifyMenuA(g_exe_ba0204, 0x8021, MF_BYCOMMAND, 0x8021, "320x240 Window\t(Fastest)");
    ModifyMenuA(g_exe_ba0204, 0x8064, MF_BYCOMMAND, 0x8064, "480x360 Window\t(Average)");
    ModifyMenuA(g_exe_ba0204, 0x8063, MF_BYCOMMAND, 0x8063, "640x480 Window\t(Slowest)");
    ModifyMenuA(g_exe_ba0204, 2, MF_BYPOSITION, 2, "&Help");
    ModifyMenuA(g_exe_ba0204, 0xe146, MF_BYCOMMAND, 0xe146, "Help &Topics\tF1");
    ModifyMenuA(g_exe_ba0204, 0x800e, MF_BYCOMMAND, 0x800e, "&About Hercules");
    m_pMainWnd->ShowWindow(m_nCmdShow);
    m_pMainWnd->UpdateWindow();
    host_startup_5e10();
    return TRUE;
}

int CHerculesApp::ExitInstance()
{
    if (m_hInstanceMapping)
        CloseHandle(m_hInstanceMapping);
    return CWinApp::ExitInstance();
}

void CHerculesApp::OnAppAbout()
{
    MessageBoxA(g_fatal_owner_window, "\nHERCULES\n\nCopyright (c) Disney Interactive 1997   \n\n",
                "About Hercules..", MB_ICONINFORMATION);
}

BOOL CHerculesApp::OnIdle(LONG lCount)
{
    LPEXCEPTION_POINTERS exception;
    char message[1024];

    __try {
        g_exe_ba0230 = 0;
        graphics_native_05a50();
        cleanup_native_6bb0();
        if (g_exe_ba0238 && !g_exe_ba0228) {
            host_timing_dispatch_5f20();
            if (g_exe_ba0238 == 1)
                g_exe_ba0238 = 0;
            cleanup_native_6bb0();
            graphics_native_05a50();
        }
    } __except (exception = GetExceptionInformation()) {
        sprintf(message, "Exception %08lX at Address %08lX",
                exception->ExceptionRecord->ExceptionCode,
                exception->ExceptionRecord->ExceptionAddress);
        archive_fatal(message);
    }
    if (!g_exe_ba0238 || g_exe_ba0228)
        WaitMessage();
    return CWinApp::OnIdle(0);
}
