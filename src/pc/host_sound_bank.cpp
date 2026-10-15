/* Private native-derived working views; no original type/TU identity claim. */
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <stdio.h>
#include <stddef.h>

#include "../../calibration/host_sound_bank.h"
#include "../../calibration/host_sound_channel_storage.h"
#include "../../calibration/host_sound_decoder_tables.h"

extern "C" {
typedef char header_prefix_is_16[(sizeof(HostSoundHeaderPrefix)==16)?1:-1];
typedef char pcm_is_16[(sizeof(PCMWAVEFORMAT)==16)?1:-1];
typedef char descriptor_is_20[(sizeof(DSBUFFERDESC)==20)?1:-1];
typedef char pcm_bits_are_at_14[(offsetof(PCMWAVEFORMAT,wBitsPerSample)==14)?1:-1];

extern IDirectSound *g_exe_79e440;
extern IDirectSoundBuffer *g_exe_799908[4800];
extern char g_archive_read_mode[];
int __cdecl archive_open(const char *,const char *);
int __cdecl archive_close(int);
size_t __cdecl archive_read(void *,size_t,size_t,int);
void __cdecl archive_fatal(const char *);
void __cdecl cleanup_native_f710(unsigned int);

void __cdecl host_sound_bank_initialize_f3d0(unsigned int index)
{
    void *block1=0;
    void *block2=0;
    DWORD bytes1=0;
    DWORD bytes2=0;
    char filename[256];
    int file;
    IDirectSoundBuffer **slot;
    if(index>=1200)
        return;
    slot=g_exe_799908+index*4;
    if(*slot!=0)
        return;
    sprintf(filename,g_host_sound_filename_format,index&0xffff);
    file=archive_open(filename,g_archive_read_mode);
    archive_read(&g_host_sound_header_699838,16,1,file);
    if(g_host_sound_header_699838.magic!=0x03465345)
        archive_fatal(g_host_sound_create_error);

    PCMWAVEFORMAT pcm;
    memset(&pcm,0,sizeof pcm);
    pcm.wf.wFormatTag=WAVE_FORMAT_PCM;
    pcm.wf.nChannels=1;
    pcm.wf.nSamplesPerSec=g_host_sound_header_699838.rate_word&0xffff;
    pcm.wBitsPerSample=(g_host_sound_header_699838.flags&0xff00)?8:16;
    pcm.wf.nBlockAlign=pcm.wBitsPerSample>>3;
    pcm.wf.nAvgBytesPerSec=pcm.wf.nBlockAlign*pcm.wf.nSamplesPerSec;
    DSBUFFERDESC desc;
    memset(&desc,0,sizeof desc);
    desc.dwFlags=(index<1000?DSBCAPS_STATIC:DSBCAPS_LOCSOFTWARE)|
        DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLPAN|DSBCAPS_CTRLVOLUME;
    desc.dwSize=sizeof(desc);
    desc.dwBufferBytes=g_host_sound_header_699838.sample_bytes/((g_host_sound_header_699838.flags&0xff00)?2:1);
    /* Genuine PCM prefix passed through the SDK's extended-format parameter. */
    desc.lpwfxFormat=(WAVEFORMATEX *)&pcm;
    if(g_exe_79e440!=0)
        g_exe_79e440->CreateSoundBuffer(&desc,slot,0);
    if(*slot!=0) {
        int state;
        (*slot)->SetCurrentPosition(0);
        (*slot)->SetFrequency(0);
        (*slot)->SetPan(0);
        (*slot)->SetVolume(0);
        /* Actual observed offset; no declared scratch capacity is supplied. */
        BYTE *compressed=(BYTE *)g_host_sound_scratch_699848+0xfffff-(g_host_sound_header_699838.sample_bytes>>2);
        archive_read(compressed,g_host_sound_header_699838.sample_bytes>>2,1,file);
        (*slot)->Lock(0,g_host_sound_header_699838.sample_bytes/((g_host_sound_header_699838.flags&0xff00)?2:1),
                      &block1,&bytes1,&block2,&bytes2,0);
        if(block1!=0 && bytes1!=0) {
            state=0;
            host_sound_decode_2410((const signed char *)compressed,g_host_sound_scratch_699848,(int)g_host_sound_header_699838.sample_bytes,&state);
            if(g_host_sound_header_699838.flags&0xff00) {
                for(state=0;state<bytes1;++state)
                    ((BYTE *)block1)[state]=(BYTE)(127-(g_host_sound_scratch_699848[state]>>8));
            } else {
                for(state=0;state<(bytes1>>1);++state)
                    ((short *)block1)[state]=g_host_sound_scratch_699848[state];
            }
            (*slot)->Unlock(block1,bytes1,block2,bytes2);
        }
        for(state=1;state<4;++state) {
            g_exe_79e440->DuplicateSoundBuffer(*slot,g_exe_799908+index*4+state);
            if(g_exe_799908[index*4+state]==0) {
                cleanup_native_f710(index);
                break;
            }
        }
    }
    g_host_sound_bank_counters_698ed8[index]=0;
    g_host_sound_loop_flags_698a18[index]=(BYTE)g_host_sound_header_699838.flags;
    archive_close(file);
}
}
