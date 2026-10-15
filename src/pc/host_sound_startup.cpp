/* Original PC sound startup hypothesis; source/prototype/TU identity unproved. */
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <math.h>
#include <string.h>
#include <stddef.h>
#include "../../calibration/host_sound_startup.h"
#include "../../calibration/host_warm_delivery.h"
#include "../../calibration/host_sound_bank.h"
#include "../../calibration/host_sound_channel_storage.h"

extern "C" {
extern IDirectSound *g_exe_79e440;
extern IDirectSoundBuffer *g_exe_79e444;
extern HWND g_fatal_owner_window;
extern void __cdecl audio_native_fdd0(unsigned short,unsigned short);
typedef char sound_primary_pcm_is_16[(sizeof(PCMWAVEFORMAT)==16)?1:-1];
typedef char sound_primary_descriptor_is_20[(sizeof(DSBUFFERDESC)==20)?1:-1];

DWORD __cdecl host_sound_primary_format_fd50(DWORD rate)
{
    DWORD previous = 0;
    if (g_exe_79e440 != 0 && g_exe_79e444 != 0) {
        previous = g_host_sound_primary_format_698ec8.wf.nSamplesPerSec;
        memset(&g_host_sound_primary_format_698ec8,0,sizeof(g_host_sound_primary_format_698ec8));
        g_host_sound_primary_format_698ec8.wf.wFormatTag = WAVE_FORMAT_PCM;
        g_host_sound_primary_format_698ec8.wf.nChannels = 2;
        g_host_sound_primary_format_698ec8.wf.nSamplesPerSec = rate;
        g_host_sound_primary_format_698ec8.wf.nAvgBytesPerSec = rate * 4;
        g_host_sound_primary_format_698ec8.wf.nBlockAlign = 4;
        g_host_sound_primary_format_698ec8.wBitsPerSample = 16;
        g_exe_79e444->SetFormat((WAVEFORMATEX *)&g_host_sound_primary_format_698ec8);
    }
    return previous;
}

void __cdecl host_service_fdf0(void)
{
    int index;
    double volume;
    DWORD configured;
    DSBUFFERDESC descriptor;
    g_host_sound_paused_79e44c = 0;
    g_host_sound_volume_table_690a18[0] = -10000;
    for (index = 1; index < 16384; ++index) {
        volume = 1650.0 * log10(index * (1.0 / 16384.0));
        if (volume < -10000.0)
            volume = -10000.0;
        else if (volume > 0.0)
            volume = 0.0;
        g_host_sound_volume_table_690a18[index] = (SHORT)(volume - 200.0);
    }
    configured = *(DWORD *)(g_registry_config + 16);
    audio_native_fdd0((WORD)configured,(WORD)configured);
    memset(g_exe_799908,0,sizeof(g_exe_799908));
    memset(g_host_sound_bank_counters_698ed8,0,sizeof(g_host_sound_bank_counters_698ed8));
    memset(g_host_audio_channel_ids_799848,-1,sizeof(g_host_audio_channel_ids_799848));
    memset(g_host_sound_channel_lanes_79e408,-1,sizeof(g_host_sound_channel_lanes_79e408));
    g_host_sound_next_slot_79e448 = 0;
    DirectSoundCreate(0,&g_exe_79e440,0);
    if (g_exe_79e440 != 0) {
        g_exe_79e440->SetCooperativeLevel(g_fatal_owner_window,DSSCL_EXCLUSIVE);
        memset(&descriptor,0,sizeof(descriptor));
        descriptor.dwSize = sizeof(descriptor);
        descriptor.dwFlags = DSBCAPS_PRIMARYBUFFER;
        g_exe_79e440->CreateSoundBuffer(&descriptor,&g_exe_79e444,0);
    }
    host_sound_primary_format_fd50(22050);
}
}
