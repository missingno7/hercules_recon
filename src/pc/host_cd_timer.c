#include <windows.h>
#include <mmsystem.h>

extern DWORD g_exe_96e61c;
extern DWORD g_exe_96e620;
extern DWORD g_exe_96e624;
extern DWORD g_exe_96e62c;
extern void __cdecl host_cd_play_9e60(DWORD start, DWORD end);

void __cdecl host_timing_service_409cd0(void)
{
    DWORD start, end;
    if (g_exe_96e61c != 0 && g_exe_96e620 != 0) {
        start = ((g_exe_96e61c & 255) +
                 ((g_exe_96e61c >> 8) & 255) * 60) * 1000;
        end = ((g_exe_96e620 & 255) +
               ((g_exe_96e620 >> 8) & 255) * 60) * 1000;
        if (timeGetTime() - g_exe_96e624 >= end - start) {
            g_exe_96e62c = 0;
            host_cd_play_9e60(g_exe_96e61c, g_exe_96e620);
        }
    }
}
