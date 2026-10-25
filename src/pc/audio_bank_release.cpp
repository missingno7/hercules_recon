#include <windows.h>
#include <dsound.h>

extern "C" {
extern unsigned char g_audio_cleanup_iteration_flag;
extern IDirectSoundBuffer *g_exe_799908[];

void __cdecl cleanup_native_f710(unsigned int index)
{
    IDirectSoundBuffer **slot;
    unsigned int remaining;

    if (index >= 1200)
        return;
    if (g_audio_cleanup_iteration_flag == 0 && index == 824)
        return;

    slot = g_exe_799908 + index * 4;
    remaining = 4;
    do {
        if (*slot != 0) {
            (*slot)->Stop();
            (*slot)->Release();
            *slot = 0;
        }
        ++slot;
    } while (--remaining != 0);
}
}
