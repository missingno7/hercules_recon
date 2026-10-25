#ifndef HERCULES_HOST_SOUND_STARTUP_H
#define HERCULES_HOST_SOUND_STARTUP_H
#include <windows.h>
#include <mmsystem.h>
#ifdef __cplusplus
extern "C" {
#endif
extern SHORT g_host_sound_volume_table_690a18[16384];
extern SHORT g_host_sound_channel_lanes_79e408[24];
extern int g_host_sound_next_slot_79e448;
extern PCMWAVEFORMAT g_host_sound_primary_format_698ec8;
DWORD __cdecl host_sound_primary_format_fd50(DWORD rate);
void __cdecl host_service_fdf0(void);
#ifdef __cplusplus
}
#endif
#endif
