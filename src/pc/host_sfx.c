/* Original PC SFX loading source hypothesis; source names/TU unproved. */
#include <string.h>
#include "host_sfx.h"
#include "host_file_callbacks.h"
extern void __cdecl host_diagnostic(const char *,...);
extern void __cdecl empty_host_callback();
extern int __cdecl host_zero_callback();
extern short __cdecl return_true(int);
extern void *__cdecl host_allocate(void **,MusicU32,MusicU32);
extern void *__cdecl shrink_relocate(void *,MusicU32,MusicU32,MusicU32);
#define SFX_CONTEXT ((MusicU8 *)g_host_engine_interface.context_004)
#define SFX_DWORD(off) (*(MusicU32 *)(SFX_CONTEXT+(off)))
#define SFX_WORD(off) (*(unsigned short *)(SFX_CONTEXT+(off)))
#define SFX_BYTE(off) (*(MusicU8 *)(SFX_CONTEXT+(off)))
#define SFX_POINTER(off) (*(MusicU8 **)(SFX_CONTEXT+(off)))

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

int __cdecl host_sfx_load_1330(unsigned int bank,unsigned int slot)
{
    MusicU32 bytes,aligned,slot_offset,header_offset;
    MusicU8 *base;
    long delta_words;
    int program;
    HostSfxProgramAttributes attributes;
    HostSfxToneAttributes tone;
    bank &= 0xff;
    slot &= 0xff;
    host_diagnostic(g_host_sfx_load_text,bank,slot);
    SFX_WORD(0x2b0+slot*2)=(unsigned short)bank;
    bytes=resource_file_length(g_host_music_paths_rva_35178[bank].path,g_host_sfx_filename);
    aligned=bytes&0xfffff800UL;
    slot_offset=0x13c+slot*4;
    if (host_allocate((void **)(SFX_CONTEXT+slot_offset),aligned+0x1000UL,0x10)==0)
        return -1;
    host_file_sync(g_host_music_paths_rva_35178[bank].path,g_host_sfx_filename,
                   SFX_POINTER(slot_offset),bytes);
    base=SFX_POINTER(slot_offset);
    header_offset=0x150+slot*4;
    SFX_POINTER(0x164+slot*4)=base+((MusicU32 *)base)[0];
    SFX_POINTER(header_offset)=base+((MusicU32 *)base)[1];
    empty_host_callback();
    if (host_music_bank_register_c040(SFX_POINTER(header_offset),(short)slot)<0) {
        host_diagnostic(g_host_music_open_error);
        return -1;
    }
    if (host_music_bank_preload_c090(SFX_POINTER(0x164+slot*4),(short)slot)<0) {
        host_diagnostic(g_host_music_download_error);
        return -1;
    }
    (void)return_true(1);
    SFX_DWORD(0x178+slot*4)=(MusicU32)host_zero_callback((unsigned short)slot);
    delta_words=((long)((MusicU32)SFX_POINTER(header_offset)-(MusicU32)SFX_POINTER(0x164+slot*4)))>>2;
    SFX_POINTER(0x18c+slot*4)=(MusicU8 *)(SFX_DWORD(0x178+slot*4)+((MusicU32)delta_words<<2));
    (void)shrink_relocate(SFX_POINTER(slot_offset),
        (MusicU32)base-(MusicU32)SFX_POINTER(header_offset)+aligned+0x1000UL,0,1);
    for (program=0;program<127;++program) {
        (void)host_sfx_program_attributes_be60((short)slot,(short)program,&attributes);
        if (attributes.sample_006!=0) {
            if (SFX_BYTE(0x2f0+(long)attributes.sample_006*3)!=0xff) {
                host_diagnostic(g_host_sfx_warning_begin);
                host_diagnostic(g_host_sfx_warning_detail);
                host_diagnostic(g_host_sfx_warning_program,program,(int)attributes.sample_006);
                host_diagnostic(g_host_sfx_warning_end);
            }
            SFX_BYTE(0x2f0+(long)attributes.sample_006*3)=(MusicU8)slot;
            SFX_BYTE(0x2f1+(long)attributes.sample_006*3)=(MusicU8)program;
            (void)host_sfx_tone_attributes_bec0((short)slot,(short)program,0,&tone);
            SFX_BYTE(0x2f2+(long)attributes.sample_006*3)=tone.byte_002;
        }
    }
    return 0;
}
