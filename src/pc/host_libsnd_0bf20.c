/* HERCULES.EXE sound-library shim 0xbe60..0xc2d0 (lane h2 candidate): PSX libsnd-style
 * VAB/voice/sequence entries.  0xbe60/0xbec0 are copied from src/pc/host_sfx.c and
 * 0xc040/0xc090 from src/pc/host_music.c (recorded neighbours) to test one contiguous unit.
 * Names provisional. */
#include <string.h>
#include "host_sfx.h"
#include "../shared/engine_interface.h"

extern void __cdecl host_sound_bank_initialize_f3d0(unsigned int);
extern int __cdecl host_0f940(int);
extern void __cdecl cleanup_native_f710(int);
extern int __cdecl host_0f8d0(int);
extern void __cdecl cleanup_native_ff40(int);
extern void __cdecl audio_native_fa10(int, short);
extern void __cdecl host_0fbf0(int, short, short, short);
extern void __cdecl host_cd_play_9e60(unsigned long, unsigned long);
extern void __cdecl cleanup_native_9e10(void);

extern long g_host_voice_left[24];      /* 0xa8e7d0 */
extern long g_host_voice_right[24];     /* 0xa8e830 */
extern unsigned long *g_host_sequences[31]; /* 0xa8e890 */
extern EngineNoArgCallback g_host_secondary_callback_68e90c;
extern unsigned long g_host_secondary_delivery_68e910;
extern short g_host_master_left;        /* 0x43a480 */
extern short g_host_master_right;       /* 0x43a484 */

short host_0c160(short voice, short left, short right);

short __cdecl host_sfx_program_attributes_be60(short bank,short program,HostSfxProgramAttributes *out)
{
    if (g_host_music_bank_headers_rva_68e790[bank]!=0 && program>=0 && program<=127) {
        memset(out,0,sizeof(*out));
        out->sample_006=*(short *)((MusicU8 *)g_host_music_bank_headers_rva_68e790[bank]+0x44+program*2);
        return 0;
    }
    return -1;
}

short __cdecl host_sfx_tone_attributes_bec0(short bank,short program,short ignored,HostSfxToneAttributes *out)
{
    if (g_host_music_bank_headers_rva_68e790[bank]!=0 && program>=0 && program<=127) {
        memset(out,0,sizeof(*out));
        out->byte_002=*((MusicU8 *)g_host_music_bank_headers_rva_68e790[bank]+0x144+program);
        return 0;
    }
    return -1;
}

short __cdecl host_0bf20(short bank, short program, short tone, short note, short fine,
                         short left, short right)
{
    MusicU8 *header;
    int volume, l, r, voice;
    header = (MusicU8 *)g_host_music_bank_headers_rva_68e790[bank];
    if (header == 0 || program < 0 || program > 127)
        return -1;
    volume = (signed char)header[0x144 + program];
    l = left * volume;
    l = l * l / 16384;
    l = l * 129 / 128;
    r = right * volume;
    r = r * r / 16384;
    r = r * 129 / 128;
    voice = host_0f940(*(short *)(header + 0x44 + program * 2));
    host_0c160((short)voice, (short)l, (short)r);
    return (short)voice;
}

void __cdecl host_0bfe0(short bank)
{
    int offset;
    if (bank < 0 || bank >= 16 || g_host_music_bank_headers_rva_68e790[bank] == 0)
        return;
    for (offset = 0x44; offset < 0x144; offset += 2) {
        short sample = *(short *)((MusicU8 *)g_host_music_bank_headers_rva_68e790[bank] + offset);
        if (sample != 0)
            cleanup_native_f710(sample);
    }
    g_host_music_bank_headers_rva_68e790[bank] = 0;
}

short __cdecl host_music_bank_register_c040(void *header,short slot)
{
    if (slot < -1 || slot >= 16)
        return -1;
    if (slot == -1) {
        for (slot=0;slot<16;++slot)
            if (g_host_music_bank_headers_rva_68e790[slot]==0)
                break;
        if (slot>=16)
            return -1;
    }
    g_host_music_bank_headers_rva_68e790[slot]=header;
    return slot;
}

short __cdecl host_music_bank_preload_c090(void *ignored,short slot)
{
    int offset;
    if (g_host_music_bank_headers_rva_68e790[slot]!=0) {
        for (offset=0x44;offset<0x144;offset+=2) {
            short sample=*(short *)((MusicU8 *)g_host_music_bank_headers_rva_68e790[slot]+offset);
            if (sample!=0)
                host_sound_bank_initialize_f3d0((unsigned int)(int)sample);
        }
    }
    return slot;
}

void __cdecl host_0c0e0(short ignored, long *out)
{
    *out = 0;
}

int __cdecl host_0c0f0(int value)
{
    return host_0f8d0(value);
}

void __cdecl host_0c100(void)
{
    memset(g_host_music_bank_headers_rva_68e790, 0, sizeof(g_host_music_bank_headers_rva_68e790));
    memset(g_host_sequences, 0, sizeof(g_host_sequences));
    g_host_master_left = 0x7f;
    g_host_master_right = 0x7f;
    cleanup_native_ff40(0);
}

short __cdecl host_0c140(short voice)
{
    audio_native_fa10(-1, voice);
    return 0;
}

short __cdecl host_0c160(short voice, short left, short right)
{
    g_host_voice_left[voice] = (g_host_master_left * left) >> 7;
    g_host_voice_right[voice] = (g_host_master_right * right) >> 7;
    host_0fbf0(-1, voice, (short)g_host_voice_left[voice], (short)g_host_voice_right[voice]);
    return 0;
}

short __cdecl host_0c1c0(short voice, short *left, short *right)
{
    if (left != 0)
        *left = (short)g_host_voice_left[voice];
    if (right != 0)
        *right = (short)g_host_voice_right[voice];
    return 0;
}

void __cdecl host_0c200(short left, short right)
{
    g_host_master_left = left;
    g_host_master_right = right;
}

void __cdecl host_0c220(int mode)
{
    g_host_secondary_delivery_68e910 = mode == 5;
}

int __cdecl host_0c250(unsigned long *sequence)
{
    int i;
    for (i = 0; i < 31; i++)
        if (g_host_sequences[i] == 0)
            break;
    g_host_sequences[i] = sequence;
    return i + 1;
}

void __cdecl host_0c280(short id)
{
    --id;
    if (g_host_sequences[id] != 0)
        host_cd_play_9e60(g_host_sequences[id][0] >> 8, g_host_sequences[id][1] >> 8);
}

void __cdecl host_0c2b0(void)
{
    cleanup_native_9e10();
}

EngineNoArgCallback __cdecl host_0c2d0(EngineNoArgCallback callback)
{
    EngineNoArgCallback previous = g_host_secondary_callback_68e90c;
    g_host_secondary_callback_68e90c = callback;
    return previous;
}
