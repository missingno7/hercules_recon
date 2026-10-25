#ifndef HERCULES_HOST_SOUND_BANK_H
#define HERCULES_HOST_SOUND_BANK_H
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct HostSoundHeaderPrefix {
    DWORD magic;
    DWORD sample_bytes;
    DWORD rate_word;
    DWORD flags;
} HostSoundHeaderPrefix;
extern HostSoundHeaderPrefix g_host_sound_header_699838;
extern WORD g_host_sound_bank_counters_698ed8[1200];
/* Definition supplies only the measured accessed prefix; original total capacity
 * and source element type remain unproved. */
extern short g_host_sound_scratch_699848[];
extern char g_host_sound_filename_format[];
extern char g_host_sound_create_error[];
void __cdecl host_sound_bank_initialize_f3d0(unsigned int index);
#ifdef __cplusplus
}
#endif
#endif
