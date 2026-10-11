#include <windows.h>
#include <mmsystem.h>

extern DWORD g_exe_96e62c;
extern DWORD g_exe_96e628;
extern char audio_stop_command[];
extern int __cdecl cleanup_native_9c30(void);

void __cdecl cleanup_native_9d60(void)
{
    if (g_exe_96e62c != 0 && cleanup_native_9c30() != 0)
        mciSendStringA(audio_stop_command, 0, 0, 0);

    if (g_exe_96e628 == 0)
        g_exe_96e628 = timeGetTime();

    g_exe_96e62c = 0;
}
