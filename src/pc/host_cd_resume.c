#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>

extern DWORD g_exe_96e61c;
extern DWORD g_exe_96e620;
extern DWORD g_exe_96e624;
extern DWORD g_exe_96e628;
extern DWORD g_exe_96e62c;
extern unsigned char g_registry_config[72];
extern char audio_stop_command[];
extern int __cdecl cleanup_native_9c30(void);

void __cdecl host_cd_unpause_9db0(void)
{
    if (g_exe_96e62c != 0 && cleanup_native_9c30() != 0)
        mciSendStringA(audio_stop_command, 0, 0, 0);
    if (g_exe_96e628 != 0) {
        g_exe_96e624 += timeGetTime() - g_exe_96e628;
        g_exe_96e628 = 0;
    }
    g_exe_96e62c = 0;
}

void __cdecl host_cd_resume_9f60(void)
{
    char command[256];
    DWORD tick, from, to;
    int elapsed;
    /* Keep the SDK WORD minute and BYTE second packing, including overlap. */
    if (g_exe_96e62c == 0 && g_exe_96e61c != 0) {
        if (g_registry_config[13] == 1 && cleanup_native_9c30() != 0) {
            mciSendStringA("set cdaudio time format tmsf", 0, 0, 0);
            tick = g_exe_96e628;
            if (tick == 0)
                tick = timeGetTime();
            elapsed = abs((int)(tick - g_exe_96e624)) / 1000;
            from = MCI_MAKE_TMSF((BYTE)(g_exe_96e61c >> 16),
                (BYTE)(g_exe_96e61c >> 8) + elapsed / 60,
                (BYTE)g_exe_96e61c + elapsed % 60, 0);
            to = MCI_MAKE_TMSF((BYTE)(g_exe_96e61c >> 16),
                (BYTE)(g_exe_96e620 >> 8), (BYTE)g_exe_96e620, 0);
            sprintf(command, "play cdaudio from %d to %d", from, to);
            g_exe_96e62c = mciSendStringA(command, 0, 0, 0) == 0;
        }
        if (g_exe_96e628 != 0) {
            g_exe_96e624 += timeGetTime() - g_exe_96e628;
            g_exe_96e628 = 0;
        }
    }
}
