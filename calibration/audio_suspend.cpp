/* Native display-transition dependency; actual DirectSound buffer physical view. */
#include <windows.h>
#include <dsound.h>

extern "C" {
extern unsigned long g_exe_615ba8;
extern IDirectSoundBuffer *g_exe_615b9c;
extern void __cdecl audio_native_035d0(int);

void __cdecl graphics_native_03770(void)
{
    ++g_exe_615ba8;
    if (g_exe_615b9c != 0)
        g_exe_615b9c->Stop();
    audio_native_035d0(0);
}
}
