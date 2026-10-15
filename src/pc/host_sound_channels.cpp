// Original PC channel control hypothesis. Packed-channel tests use slots 0..23.
// Retain native pre-guard ID access and live reloads after COM callbacks.
#include <windows.h>
#include <dsound.h>
#include "../../calibration/host_warm_delivery.h"

#include "../../calibration/host_sound_channel_storage.h"

extern "C" {
extern void __cdecl cleanup_native_f710(unsigned int);
extern int __cdecl host_sound_bank_initialize_f3d0(unsigned int);

void __cdecl host_sound_bank_restore_f760(void)
{
    int bank;
    DWORD status;
    for (bank = 0; bank < 1200; ++bank) {
        if (g_exe_799908[bank * 4] != 0) {
            g_exe_799908[bank * 4]->GetStatus(&status);
            if (status == DSBSTATUS_BUFFERLOST) {
                g_exe_799908[bank * 4]->Restore();
                cleanup_native_f710(bank);
                host_sound_bank_initialize_f3d0(bank);
            }
        }
    }
}

void __cdecl audio_native_fa10(int value, int slot_index)
{
    DWORD id;
    unsigned int bank, lane;
    short slot;
    if (value == -1) {
        slot = (short)slot_index;
        id = g_host_audio_channel_ids_799848[slot];
        if ((id & 0xffff0000UL) == 0x03380000UL ||
            slot < 0 || slot > 24 || id == 0xffffffffUL)
            return;
        bank = id >> 16;
        if (g_exe_799908[bank * 4] == 0)
            return;
        g_exe_799908[bank * 4 + (id & 0xffffUL)]->Stop();
        id = g_host_audio_channel_ids_799848[slot];
        g_exe_799908[(id >> 16) * 4 + (id & 0xffffUL)]->SetCurrentPosition(0);
        g_host_audio_channel_ids_799848[slot] = 0xffffffffUL;
    } else {
        if ((unsigned int)value >= 1200 || value == 824)
            return;
        for (lane = 0; lane < 4; ++lane) {
            if (g_exe_799908[value * 4] != 0) {
                g_exe_799908[value * 4 + lane]->Stop();
                g_exe_799908[value * 4 + lane]->SetCurrentPosition(0);
            }
        }
    }
}

void __cdecl host_sound_pause_ffb0(void)
{
    int slot;
    DWORD id;
    if (g_host_sound_paused_79e44c == 0) {
        g_host_sound_paused_79e44c = 1;
        host_audio_poll_f820();
        for (slot = 0; slot < 24; ++slot) {
            id = g_host_audio_channel_ids_799848[slot];
            if (id != 0xffffffffUL && g_exe_799908[(id >> 16) * 4] != 0)
                g_exe_799908[(id >> 16) * 4 + (id & 0xffffUL)]->Stop();
        }
    }
}

void __cdecl host_sound_resume_10030(void)
{
    int slot;
    DWORD id;
    if (g_host_sound_paused_79e44c != 0) {
        g_host_sound_paused_79e44c = 0;
        host_sound_bank_restore_f760();
        for (slot = 0; slot < 24; ++slot) {
            id = g_host_audio_channel_ids_799848[slot];
            if (id != 0xffffffffUL && g_exe_799908[(id >> 16) * 4] != 0) {
                g_exe_799908[(id >> 16) * 4 + (id & 0xffffUL)]->SetCurrentPosition(
                    g_host_audio_channel_cursors_7998a8[slot]);
                if (g_registry_config[14] != 0) {
                    id = g_host_audio_channel_ids_799848[slot];
                    g_exe_799908[(id >> 16) * 4 + (id & 0xffffUL)]->Play(
                        0, 0, g_host_sound_loop_flags_698a18[id >> 16] != 0);
                }
            }
        }
    }
}
}
