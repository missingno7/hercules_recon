/* Native MCI readiness protocol; pinned SDK and identified CRT routine. */
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
extern "C" {
extern BOOL g_exe_96e630;
int __cdecl cleanup_native_9c30(void)
{
    char response[256];
    if ((g_exe_96e630 ||
         (g_exe_96e630 = mciSendStringA("open cdaudio shareable wait", 0, 0, 0) == 0)) &&
        mciSendStringA("status cdaudio media present", response, sizeof response, 0) == 0 &&
        _stricmp(response, "true") == 0 &&
        mciSendStringA("status cdaudio ready", response, sizeof response, 0) == 0 &&
        _stricmp(response, "true") == 0)
        return 1;
    return 0;
}
}
