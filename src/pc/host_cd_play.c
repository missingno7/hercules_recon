#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

extern DWORD g_exe_96e61c;
extern DWORD g_exe_96e620;
extern DWORD g_exe_96e624;
extern DWORD g_exe_96e62c;
extern int g_exe_91c46c;
extern unsigned char g_registry_config[72];
extern void __cdecl cleanup_native_9e10(void);
extern int __cdecl cleanup_native_9c30(void);

void __cdecl host_cd_play_9e60(DWORD start, DWORD end)
{
    char command[256];
    DWORD from, to;
    int attempt;

    cleanup_native_9e10();
    g_exe_96e61c = start;
    g_exe_96e620 = end;
    for (attempt = 0; attempt < 2; ++attempt) {
        if (g_registry_config[13] != 1 || cleanup_native_9c30() == 0 ||
            g_exe_96e62c != 0)
            break;
        mciSendStringA("set cdaudio time format tmsf", 0, 0, 0);
        from = MCI_MAKE_TMSF((BYTE)(g_exe_96e61c >> 16),
                            (BYTE)(g_exe_96e61c >> 8),
                            (BYTE)g_exe_96e61c, 0);
        to = MCI_MAKE_TMSF((BYTE)(g_exe_96e61c >> 16),
                          (BYTE)(g_exe_96e620 >> 8),
                          (BYTE)g_exe_96e620, 0);
        sprintf(command, "play cdaudio from %d to %d", from, to);
        g_exe_96e62c = mciSendStringA(command, 0, 0, 0) == 0;
        if (g_exe_96e62c == 0)
            Sleep(100);
    }
    g_exe_96e624 = timeGetTime();
    g_exe_91c46c = 1;
}
