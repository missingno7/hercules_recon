/* Working observed four-byte cells only; original aggregate/TU/layout unproved.
 * The sound buffer and thread objects remain owned by their actual API creators. */
#include <windows.h>
#include <dsound.h>

typedef char sdk_buffer_pointer_4[(sizeof(IDirectSoundBuffer *)==4)?1:-1];
typedef char sdk_counter_4[(sizeof(unsigned long)==4)?1:-1];
typedef char sdk_handle_4[(sizeof(HANDLE)==4)?1:-1];
typedef char sdk_priority_class_4[(sizeof(DWORD)==4)?1:-1];
typedef char sdk_thread_priority_4[(sizeof(int)==4)?1:-1];
typedef char sdk_bool_4[(sizeof(BOOL)==4)?1:-1];

extern "C" {
IDirectSoundBuffer *g_exe_615b9c = 0;
unsigned long g_exe_615ba8 = 0;
HANDLE g_exe_615bbc = 0;
DWORD g_exe_615bd0 = 0;
int g_exe_615bd4 = 0;
BOOL g_exe_615bd8 = 0;
}
