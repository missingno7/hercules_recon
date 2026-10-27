/* Original PC music/control source hypothesis; original declarations/TU unproved. */
#include "host_music.h"
#include "host_file_callbacks.h"
extern void __cdecl host_diagnostic(const char *,...);
/* Historical C callsite views; common original ignored-argument declarations unproved. */
extern void __cdecl empty_host_callback();
typedef void (__cdecl *HostThreeWordCallback)(unsigned short, unsigned short, unsigned short);
extern int __cdecl host_zero_callback();
extern short __cdecl return_true(int);
extern void *__cdecl host_allocate(void **,MusicU32,MusicU32);
extern void *__cdecl shrink_relocate(void *,MusicU32,MusicU32,MusicU32);
extern void __cdecl host_sound_bank_initialize_f3d0(unsigned int);

/* Non-owning access to measured context bytes; no array extent/owner is invented. */
#define MUSIC_CONTEXT ((MusicU8 *)g_host_engine_interface.context_004)
#define MUSIC_DWORD(off) (*(MusicU32 *)(MUSIC_CONTEXT+(off)))
#define MUSIC_WORD(off) (*(unsigned short *)(MUSIC_CONTEXT+(off)))
#define MUSIC_POINTER(off) (*(MusicU8 **)(MUSIC_CONTEXT+(off)))

void __cdecl host_music_apply_1000(void)
{
    MUSIC_DWORD(0x1b14)=((*(volatile MusicU32 *)(MUSIC_CONTEXT+0x1b0c))*MUSIC_DWORD(0x1b04))>>8;
    MUSIC_DWORD(0x1b18)=(MUSIC_DWORD(0x1b10)*MUSIC_DWORD(0x1b08))>>8;
    if (MUSIC_DWORD(0x1afc)!=0xffffffffUL &&
        (MusicU32)MUSIC_WORD(0x1b1c)!=MUSIC_DWORD(0x1b14)) {
        ((HostThreeWordCallback)empty_host_callback)(MUSIC_WORD(0x1afc),MUSIC_WORD(0x1b14),MUSIC_WORD(0x1b14));
        MUSIC_WORD(0x1b1c)=MUSIC_WORD(0x1b14);
    }
}
void __cdecl host_music_signal_1090(void)
{
    if (MUSIC_DWORD(0x1afc)!=0xffffffffUL)
        ((void (__cdecl *)(unsigned short))empty_host_callback)(MUSIC_WORD(0x1afc));
}
void __cdecl host_music_set_bases_10b0(MusicU32 music,MusicU32 effects)
{
    MUSIC_DWORD(0x1b04)=music;
    MUSIC_DWORD(0x1b08)=effects;
    host_music_apply_1000();
}
void __cdecl host_music_initialize_10e0(void)
{
    host_diagnostic(g_host_music_init_text);
    MUSIC_DWORD(0x1b0c)=256;
    MUSIC_DWORD(0x1b10)=256;
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
int __cdecl host_music_load_1110(unsigned int bank,unsigned int slot)
{
    MusicU32 bytes,aligned,slot_offset,header_offset;
    MusicU8 *base;
    long delta_words;
    bank &= 0xff;
    slot &= 0xff;
    host_diagnostic(g_host_music_load_text,bank,slot);
    MUSIC_WORD(0x2b0+slot*2)=(unsigned short)bank;
    bytes=resource_file_length(g_host_music_paths_rva_35178[bank].path,g_host_music_filename);
    aligned=bytes&0xfffff800UL;
    slot_offset=0x13c+slot*4;
    if (host_allocate((void **)(MUSIC_CONTEXT+slot_offset),aligned+0x1000UL,0x10)==0)
        return -1;
    host_file_sync(g_host_music_paths_rva_35178[bank].path,g_host_music_filename,
                   MUSIC_POINTER(slot_offset),bytes);
    base=MUSIC_POINTER(slot_offset);
    header_offset=0x150+slot*4;
    MUSIC_POINTER(0x164+slot*4)=base+((MusicU32 *)base)[0];
    MUSIC_POINTER(header_offset)=base+((MusicU32 *)base)[1];
    MUSIC_POINTER(0x24c+slot*4)=base+((MusicU32 *)base)[2];
    MUSIC_POINTER(0x260+slot*4)=base+((MusicU32 *)base)[3];
    MUSIC_POINTER(0x274+slot*4)=base+((MusicU32 *)base)[4];
    MUSIC_POINTER(0x288+slot*4)=base+((MusicU32 *)base)[5];
    MUSIC_POINTER(0x29c+slot*4)=base+((MusicU32 *)base)[6];
    empty_host_callback();
    if (host_music_bank_register_c040(MUSIC_POINTER(header_offset),(short)slot)<0) {
        host_diagnostic(g_host_music_open_error);
        return -1;
    }
    if (host_music_bank_preload_c090(MUSIC_POINTER(0x164+slot*4),(short)slot)<0) {
        host_diagnostic(g_host_music_download_error);
        return -1;
    }
    (void)return_true(1);
    MUSIC_DWORD(0x178+slot*4)=(MusicU32)host_zero_callback((unsigned short)slot);
    delta_words=((long)((MusicU32)MUSIC_POINTER(header_offset)-(MusicU32)MUSIC_POINTER(0x164+slot*4)))>>2;
    MUSIC_POINTER(0x18c+slot*4)=(MusicU8 *)(MUSIC_DWORD(0x178+slot*4)+((MusicU32)delta_words<<2));
    (void)shrink_relocate(MUSIC_POINTER(slot_offset),
        (MusicU32)base-(MusicU32)MUSIC_POINTER(header_offset)+aligned+0x1000UL,0,1);
    return 0;
}
