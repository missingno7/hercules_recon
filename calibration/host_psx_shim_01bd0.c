/* HERCULES.EXE PSX-shim continuation 0x1bd0..0x1f60 (lane h1 candidate):
 * display/VSync setup, XA speech playback and pad-trigger test.  Names and TU unproved. */
#include "engine_interface.h"

extern EngineInterface g_host_engine_interface;
extern void __cdecl empty_host_callback(void);
extern int __cdecl host_zero_callback();
extern unsigned long __cdecl host_sequence_next(int);
extern EngineNoArgCallback __cdecl host_set_callback_one(EngineNoArgCallback);
extern void __cdecl host_pair_frame_initialize(void *, void *);
extern int __cdecl frame_chain_write(void *, unsigned long);
extern void __cdecl audio_native_fa10(int, int);
extern void __cdecl host_0a150(int, int, int);
extern short __cdecl host_0f940(int);
extern void __cdecl host_0fbf0(int, int, int, int);
extern int __cdecl host_0f7c0(int);
extern void __cdecl host_041a0(void);
#include <stdlib.h>

/* PSX double-buffer view: DRAWENV (0x5c) + DISPENV (0x14) + order-table words. */
typedef struct HostDisplayBuffer {
    unsigned char draw_000[0x16];
    unsigned char dtd_016;
    unsigned char dfe_017;
    unsigned char isbg_018;
    unsigned char draw_019[0x5c - 0x19];
    unsigned char disp_05c[0x14];
    unsigned long order_table_070[3];
} HostDisplayBuffer;

/* Speech index record: "TZC" tag, track, byte length, playback ticks. */
typedef struct HostSpeechRecord {
    char tag_000[4];
    long track_004;
    long bytes_008;
    long ticks_00c;
} HostSpeechRecord;

extern HostDisplayBuffer g_exe_ba2da0[2];
extern HostDisplayBuffer *g_exe_43f748;
extern long g_resource_prepare_scalar;
extern HostSpeechRecord g_exe_4352b8[];
extern HostSpeechRecord *g_exe_43e530;
extern int g_exe_43f744;
extern short g_exe_43e738[1024];
extern unsigned long g_exe_43f73c;

#define CTX (g_host_engine_interface.context_004)
#define CTX_BYTES ((unsigned char *)g_host_engine_interface.context_004)
#define CTX_DWORD(off) (*(unsigned long *)(CTX_BYTES + (off)))
#define CTX_WORD(off) (*(unsigned short *)(CTX_BYTES + (off)))
#define CTX_SHORT(off) (*(short *)(CTX_BYTES + (off)))
#define CTX_BYTE(off) (*(unsigned char *)(CTX_BYTES + (off)))

void __cdecl host_01d00(void);

void __cdecl host_01bd0(short x, short y, int horizontal, int vertical, short mode)
{
    host_01d00();
    CTX_SHORT(0x130) = x;
    CTX_SHORT(0x132) = y;
    CTX_SHORT(0x138) = mode;
    host_pair_frame_initialize(&g_exe_ba2da0[0], &g_exe_ba2da0[1]);
    g_exe_ba2da0[1].isbg_018 = 0;
    g_exe_ba2da0[0].isbg_018 = 0;
    g_exe_ba2da0[1].dtd_016 = 1;
    g_exe_ba2da0[0].dtd_016 = 1;
    CTX_BYTE(0x134) = 0;
    CTX_BYTE(0x135) = 0;
    CTX_BYTE(0x136) = 0;
    g_exe_43f748 = &g_exe_ba2da0[0];
    frame_chain_write(g_exe_ba2da0[0].order_table_070, 2);
    frame_chain_write(g_exe_ba2da0[1].order_table_070, 2);
    CTX_BYTE(0x129) = 1;
    CTX_SHORT(0x12e) = 0x80;
    CTX_SHORT(0x12c) = 0x80;
    if (vertical < 0)
        CTX_SHORT(0x12e) = 0;
    if (horizontal < 0)
        CTX_SHORT(0x12c) = 0;
    CTX_BYTE(0x12a) = (unsigned char)horizontal;
    CTX_BYTE(0x12b) = (unsigned char)vertical;
}

void __cdecl host_01d00(void)
{
    host_set_callback_one(0);
    host_zero_callback(0);
    host_sequence_next(0);
    CTX_BYTE(0x129) = 0;
}

void __cdecl host_01d30(void)
{
    host_set_callback_one(0);
    CTX_DWORD(0x124)++;
    host_set_callback_one(host_01d30);
}

void __cdecl host_01d80(void)
{
    if (g_resource_prepare_scalar > -1 && CTX->prepare_state_1b00 == 2) {
        if (--g_resource_prepare_scalar < 0) {
            host_0a150(9, 0, 0);
            CTX->prepare_state_1b00 = 0;
            empty_host_callback();
        }
    }
}

long __cdecl host_01dd0(int speech)
{
    short voice;
    if (g_resource_prepare_scalar < 0)
        CTX->prepare_state_1b00 = 0;
    if (CTX->prepare_state_1b00 != 0)
        return 0;
    if (CTX_DWORD(0x30) & 0x20000)
        return 1;
    if (CTX->mode_09c == 1)
        return 1;
    if (CTX->blocked_0df)
        return 1;
    g_exe_43e530 = &g_exe_4352b8[speech];
    g_exe_43f744 = speech + 1000;
    voice = host_0f940(speech + 1000);
    host_0fbf0(-1, voice, 8000, 8000);
    CTX->prepare_state_1b00 = 2;
    g_resource_prepare_scalar = g_exe_43e530->ticks_00c;
    return g_exe_43e530->ticks_00c;
}

void __cdecl host_01e90(void)
{
    if (CTX->prepare_state_1b00 != 0) {
        audio_native_fa10(g_exe_43f744, 0);
        host_0a150(9, 0, 0);
        CTX->prepare_state_1b00 = 0;
        g_resource_prepare_scalar = 0;
        empty_host_callback();
    }
}

int __cdecl host_01ee0(void)
{
    int level = 0;
    int i;
    host_zero_callback(g_exe_43e738, 1);
    for (i = 512; i < 1024; i += 4)
        level = (abs(g_exe_43e738[i] >> 8) + level) >> 1;
    if (host_0f7c0(g_exe_43f744))
        return rand() & 0x1f;
    return 0;
}

void __cdecl host_01f40(void)
{
    g_exe_43f73c = 0x100;
}

void __cdecl host_01f50(void)
{
    g_exe_43f73c = 0;
}

unsigned int __cdecl host_01f60(unsigned long trigger)
{
    unsigned int pad;
    host_041a0();
    pad = CTX_SHORT(0x5e) & 0xffff;
    if ((trigger & 0x4000) && (pad & 0x5000))
        return pad;
    if ((trigger & 0x8000) && pad)
        return pad;
    if ((trigger & 0x20000) && pad && ((pad & 8) || (pad & 0x4000)))
        return pad;
    return 0;
}
