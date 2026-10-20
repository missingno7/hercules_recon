// Lane h3: HERCULES.EXE sound voice helpers 0xf7c0..0xfd4f (original PC DirectSound voice layer).
// Storage names reuse the project's existing providers; element types follow the loads observed
// here (signed 16-bit counters/lanes/gains; zero-extended volume table entries).
#include <windows.h>
#include <dsound.h>

extern "C" {
extern IDirectSoundBuffer *g_exe_799908[4800];
extern DWORD g_host_audio_channel_ids_799848[24];
extern BYTE g_registry_config[72];
extern BYTE g_host_sound_loop_flags_698a18[1200];
extern short g_host_sound_bank_counters_698ed8[1200];
extern short g_host_sound_channel_lanes_79e408[24];
extern int g_host_sound_next_slot_79e448;
extern short g_exe_79e438;
extern short g_exe_79e43c;
extern WORD g_host_sound_volume_table_690a18[16384];

/* 0 = bank idle, 1 = a lane plays once, 2 = a lane loops. */
int __cdecl host_0f7c0(unsigned int bank)
{
    int lane;
    DWORD status;
    if (bank < 1200 && g_exe_799908[bank * 4] != 0) {
        for (lane = 0; lane < 4; ++lane) {
            g_exe_799908[bank * 4 + lane]->GetStatus(&status);
            if (status & DSBSTATUS_PLAYING)
                return (status & DSBSTATUS_LOOPING) ? 2 : 1;
        }
    }
    return 0;
}

void __cdecl host_0f8d0(BYTE *active)
{
    int voice;
    for (voice = 0; voice < 24; ++voice)
        active[voice] = g_host_audio_channel_ids_799848[voice] != 0xffffffff;
}

/* Round-robin search for a free voice after the last one handed out. */
short __cdecl host_0f900(void)
{
    short voice;
    for (voice = g_host_sound_next_slot_79e448 + 1; ; ++voice) {
        if (voice >= 24)
            voice = 0;
        if (g_host_audio_channel_ids_799848[voice] == 0xffffffff)
            return g_host_sound_next_slot_79e448 = voice;
        if (voice == (short)g_host_sound_next_slot_79e448)
            return -1;
    }
}

short __cdecl host_0f940(unsigned int bank)
{
    short voice;
    int lane;
    if (bank < 1200 && g_exe_799908[bank * 4] != 0 &&
        (g_registry_config[14] != 0 || g_host_sound_loop_flags_698a18[bank] != 0)) {
        voice = host_0f900();
        if (voice != -1) {
            lane = g_host_sound_bank_counters_698ed8[bank];
            g_host_audio_channel_ids_799848[voice] = (lane & 0xffff) | (bank << 16);
            g_exe_799908[lane + bank * 4]->SetVolume(-10000);
            if (g_registry_config[14] != 0)
                g_host_sound_channel_lanes_79e408[voice] = g_host_sound_bank_counters_698ed8[bank];
            g_host_sound_bank_counters_698ed8[bank] = (g_host_sound_bank_counters_698ed8[bank] + 1) % 4;
        }
        return voice;
    }
    return -1;
}

/* Packs (DirectSound pan << 16) | DirectSound volume for 15-bit signed left/right levels. */
long __cdecl host_0fb10(short left, short right)
{
    double loud;
    double diff;
    double pan;
    left <<= 1;
    right <<= 1;
    if (left < 0)
        left = -left;
    if (right < 0)
        right = -right;
    left = ((left >> 1) * g_exe_79e438 >> 14) & 0x3fff;
    right = ((right >> 1) * g_exe_79e43c >> 14) & 0x3fff;
    if (left != 0 && right != 0) {
        loud = left > right ? left : right;
        diff = right - left;
        pan = diff * diff * 10000.0;
        pan = pan / (loud * loud);
        if (pan > 10000.0)
            pan = 10000.0;
    } else {
        pan = 10000.0;
    }
    if (left > right)
        pan = -pan;
    return ((long)pan << 16) | g_host_sound_volume_table_690a18[left > right ? left : right];
}

/* bank == -1 addresses one playing voice (and starts its deferred lane); otherwise all lanes of bank. */
void __cdecl host_0fbf0(int bank, short voice, short left, short right)
{
    long level;
    int lane;
    if (bank == -1) {
        if (voice < 0 || voice > 24)
            return;
        if (g_host_audio_channel_ids_799848[voice] == 0xffffffff)
            return;
        bank = g_host_audio_channel_ids_799848[voice] >> 16;
        if (g_exe_799908[bank * 4] == 0)
            return;
        level = host_0fb10(left, right);
        g_exe_799908[bank * 4 + (g_host_audio_channel_ids_799848[voice] & 0xffff)]->SetVolume((level << 16) >> 16);
        g_exe_799908[bank * 4 + (g_host_audio_channel_ids_799848[voice] & 0xffff)]->SetPan(level >> 16);
        if (g_registry_config[14] != 0 && g_host_sound_channel_lanes_79e408[voice] != -1) {
            g_exe_799908[bank * 4 + g_host_sound_channel_lanes_79e408[voice]]->Play(
                0, 0, g_host_sound_loop_flags_698a18[bank] != 0);
            g_host_sound_channel_lanes_79e408[voice] = -1;
        }
    } else if ((unsigned int)bank < 1200 && g_exe_799908[bank * 4] != 0) {
        level = host_0fb10(left, right);
        for (lane = 0; lane < 4; ++lane) {
            g_exe_799908[bank * 4 + lane]->SetVolume((level << 16) >> 16);
            g_exe_799908[bank * 4 + lane]->SetPan(level >> 16);
        }
    }
}
}
