#ifndef HERCULES_HOST_SFX_H
#define HERCULES_HOST_SFX_H
#include "host_music.h"
#include <stddef.h>
/* Physical output views only; original PSX names and declarations unproved. */
typedef struct HostSfxProgramAttributes {
    MusicU8 unknown_000[6];
    short sample_006;
    MusicU8 unknown_008[8];
} HostSfxProgramAttributes;
typedef struct HostSfxToneAttributes {
    MusicU8 unknown_000[2];
    MusicU8 byte_002;
    MusicU8 unknown_003[29];
} HostSfxToneAttributes;
typedef char sfx_program_is_16[(sizeof(HostSfxProgramAttributes)==16)?1:-1];
typedef char sfx_tone_is_32[(sizeof(HostSfxToneAttributes)==32)?1:-1];
typedef char sfx_program_sample_at6[(offsetof(HostSfxProgramAttributes,sample_006)==6)?1:-1];
typedef char sfx_tone_byte_at2[(offsetof(HostSfxToneAttributes,byte_002)==2)?1:-1];
#ifdef __cplusplus
extern "C" {
#endif
extern char g_host_sfx_load_text[],g_host_sfx_filename[];
extern char g_host_sfx_warning_begin[],g_host_sfx_warning_detail[];
extern char g_host_sfx_warning_program[],g_host_sfx_warning_end[];
short __cdecl host_sfx_program_attributes_be60(short,short,HostSfxProgramAttributes *);
short __cdecl host_sfx_tone_attributes_bec0(short,short,short,HostSfxToneAttributes *);
int __cdecl host_sfx_load_1330(unsigned int,unsigned int);
#ifdef __cplusplus
}
#endif
#endif
