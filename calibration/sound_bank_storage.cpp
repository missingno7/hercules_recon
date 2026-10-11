/* Observed pointer-bank prefix and real SDK cells. Original full-object extent,
 * aggregate/TU ownership and placement remain unproved. No COM target is supplied. */
#include <windows.h>
#include <dsound.h>
typedef char sdk_buffer_pointer_4[(sizeof(IDirectSoundBuffer *)==4)?1:-1];
typedef char sdk_device_pointer_4[(sizeof(IDirectSound *)==4)?1:-1];
typedef char sdk_flag_1[(sizeof(BYTE)==1)?1:-1];

extern "C" {
/* Native 4800-DWORD clear and 1200 bounded four-slot rows support this prefix. */
IDirectSoundBuffer *g_exe_799908[4800] = {0};
IDirectSound *g_exe_79e440 = 0;
IDirectSoundBuffer *g_exe_79e444 = 0;
BYTE g_audio_cleanup_iteration_flag = 0;
}
