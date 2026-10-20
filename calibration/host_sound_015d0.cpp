/* HERCULES.EXE Sonix sound-control continuation 0x15d0..0x1b90 (lane h1 candidate).
 * C++ (extern "C"): base/index order of 0x1610/0x16b0 matches only as C++. */
extern "C" {
#include "engine_interface.h"

extern EngineInterface g_host_engine_interface;
extern void __cdecl host_diagnostic(const char *, ...);
extern void __cdecl host_free(void *);
extern unsigned long __cdecl host_sequence_next(int);
extern int __cdecl host_zero_callback(int);
extern short __cdecl return_false(int);
extern void __cdecl host_music_apply_1000(void);
extern void __cdecl host_music_initialize_10e0(void);
/* Observed call shape: both loaders receive BYTE arguments. */
extern int __cdecl host_music_load_1110(unsigned char, unsigned char);
extern int __cdecl host_sfx_load_1330(unsigned char, unsigned char);

/* Empty PSX sound-library stubs; LINK folded the identical bodies into the
 * single RET at 0xbe30.  Per-call prototypes are observed argument shapes. */
extern void __cdecl empty_host_callback(void);
extern void __cdecl host_sound_stub_1(short);
extern void __cdecl host_sound_stub_2(short, short);
extern void __cdecl host_sound_stub_3(short, short, short);
extern void __cdecl host_sound_stub_attr(void *, short, short);

/* Sound-library replacements in 0xbfe0..0xc2d0 (unrecorded). */
extern void __cdecl host_0bfe0(short);
extern short __cdecl host_0c140(short);
extern void __cdecl host_0c100(void);
extern void __cdecl host_0c200(short, short);
extern void __cdecl host_0c220(int);
extern short __cdecl host_0c250(void *, short);
extern void __cdecl host_0c280(short, short, short);
extern void __cdecl host_0c2b0(short);
extern int __cdecl host_0c2d0(int);
extern void __cdecl host_01f40(void);

extern unsigned long g_exe_43f738;

#define SONIX_CONTEXT ((unsigned char *)g_host_engine_interface.context_004)
#define SONIX_DWORD(off) (*(unsigned long *)(SONIX_CONTEXT + (off)))
#define SONIX_WORD(off) (*(unsigned short *)(SONIX_CONTEXT + (off)))
#define SONIX_SHORT(off) (*(short *)(SONIX_CONTEXT + (off)))
#define SONIX_BYTE(off) (*(unsigned char *)(SONIX_CONTEXT + (off)))
#define SONIX_POINTER(off) (*(unsigned char **)(SONIX_CONTEXT + (off)))

void __cdecl host_015d0(void)
{
    if (SONIX_DWORD(0x1afc) != 0xffffffffUL) {
        host_0c2b0(SONIX_SHORT(0x1afc));
        empty_host_callback();
        host_sound_stub_1(SONIX_SHORT(0x1afc));
        empty_host_callback();
    }
}

void __cdecl host_01610(void)
{
    int slot;
    host_diagnostic("UnLoadAllSfxMusic\n");
    host_015d0();
    for (slot = 0; slot < 32; slot++) {
        if (((short *)(SONIX_CONTEXT + 0x2b0))[slot] != -1) {
            host_0bfe0((short)slot);
            host_free(((unsigned char **)(SONIX_CONTEXT + 0x13c))[slot]);
            ((short *)(SONIX_CONTEXT + 0x2b0))[slot] = -1;
        }
    }
    for (slot = 0; slot < 24; slot++)
        host_0c140((short)slot);
    empty_host_callback();
    host_0c2d0(0);
    empty_host_callback();
    empty_host_callback();
    host_sequence_next(3);
}

int __cdecl host_016b0(unsigned char *list)
{
    int i;
    int slot;
    for (i = 0; i < 0x800; i++) {
        SONIX_BYTE(0x2f0 + i * 3) = 0xff;
        SONIX_BYTE(0x2f1 + i * 3) = 0xff;
    }
    host_diagnostic("LoadLevelSonix ------- ------- ------\n");
    if (*list == 0xff) {
        list++;
        SONIX_DWORD(0x1b04) = *list++;
        SONIX_DWORD(0x1b08) = *list++;
    } else {
        SONIX_DWORD(0x1b04) = 0x80;
        SONIX_DWORD(0x1b08) = 0x80;
    }
    host_music_apply_1000();
    if (*list == 0xfe) {
        list++;
        host_sound_stub_2(*list, *list);
        list++;
        host_01f40();
    } else {
        host_sound_stub_2(0, 0);
    }
    host_diagnostic("\n LoadLevelSonix setting Music %d Sfx %d", SONIX_DWORD(0x1b04), SONIX_DWORD(0x1b08));
    host_music_load_1110(*list, 1);
    slot = 2;
    while (*++list != 0xff) {
        if (*list != 0x63) {
            host_diagnostic("\n Load Level Sonix %d\n", *list);
            host_sfx_load_1330(*list, (unsigned char)slot);
            slot++;
        }
    }
    return 0;
}

void __cdecl host_01b40(void);

void __cdecl host_017d0(unsigned char slot, unsigned char song)
{
    int vab;
    host_01b40();
    if (SONIX_DWORD(0x1b14) != 0) {
        slot++;
        vab = slot;
        if (vab == 0)
            vab = 1;
        if (SONIX_SHORT(0x2b0 + slot * 2) == -1) {
            host_diagnostic("ERROR Specified vabid is not loaded \n\n");
            return;
        }
        host_015d0();
        SONIX_SHORT(0x1af0 + slot * 2) = host_0c250(SONIX_POINTER(0x24c + (song * 5 + slot) * 4), vab);
        host_diagnostic("\n StartMusic 0x%x 0x%x 0x%x", SONIX_DWORD(0x1b04), SONIX_DWORD(0x1b0c), SONIX_DWORD(0x1b14));
        host_sound_stub_3(SONIX_SHORT(0x1af0 + slot * 2), SONIX_SHORT(0x1b14), SONIX_SHORT(0x1b14));
        host_0c280(SONIX_SHORT(0x1af0 + slot * 2), 1, 0);
        SONIX_DWORD(0x1afc) = SONIX_SHORT(0x1af0 + slot * 2);
        host_diagnostic("StartMusic %d %d\n", slot, song);
    }
}

void __cdecl host_018f0(unsigned char slot, unsigned char song)
{
    int vab;
    host_01b40();
    slot++;
    vab = slot;
        if (vab == 0)
            vab = 1;
    if (SONIX_SHORT(0x2b0 + slot * 2) == -1) {
        host_diagnostic("ERROR Specified vabid is not loaded \n\n");
        return;
    }
    host_015d0();
    SONIX_SHORT(0x1af0 + slot * 2) = host_0c250(SONIX_POINTER(0x24c + (song * 5 + slot) * 4), vab);
    host_diagnostic("\n StartMusic 0x%x 0x%x 0x%x", SONIX_DWORD(0x1b04), SONIX_DWORD(0x1b0c), SONIX_DWORD(0x1b14));
    host_sound_stub_3(SONIX_SHORT(0x1af0 + slot * 2), SONIX_SHORT(0x1b14), SONIX_SHORT(0x1b14));
    host_0c280(SONIX_SHORT(0x1af0 + slot * 2), 1, 0);
    SONIX_DWORD(0x1afc) = SONIX_SHORT(0x1af0 + slot * 2);
    host_diagnostic("StartMusic %d %d\n", slot, song);
}

void __cdecl host_01a00(int mode)
{
    int i;
    if (g_exe_43f738 == 0) {
        g_exe_43f738 = 1;
        host_music_initialize_10e0();
        SONIX_DWORD(0x1b24) = 1;
    }
    host_0c100();
    host_0c220(0x1000);
    host_sound_stub_attr(SONIX_CONTEXT + 0x1a0, 1, 1);
    host_zero_callback(1);
    return_false(4);
    host_sound_stub_2(0, 0);
    empty_host_callback();
    SONIX_DWORD(0x1b20) = host_0c2d0(mode);
    if (mode == 0)
        host_0c220(0x1000);
    else
        host_0c220(5);
    empty_host_callback();
    SONIX_DWORD(0x1b28) = 0x69;
    SONIX_DWORD(0x1b2c) = 0x69;
    SONIX_DWORD(0x1b30) = 1;
    host_0c200(0x69, 0x69);
    SONIX_DWORD(0x1afc) = 0xffffffffUL;
    for (i = 0; i < 0x800; i++) {
        SONIX_BYTE(0x2f0 + i * 3) = 0xff;
        SONIX_BYTE(0x2f1 + i * 3) = 0xff;
    }
    for (i = 0; i < 32; i++)
        SONIX_WORD(0x2b0 + i * 2) = 0xffff;
}

void __cdecl host_01b40(void)
{
    SONIX_DWORD(0x1b2c) = 0x69;
    SONIX_DWORD(0x1b28) = 0x69;
    SONIX_DWORD(0x1b30) = 1;
    host_0c200(SONIX_SHORT(0x1b28), SONIX_SHORT(0x1b28));
}

void __cdecl host_01b90(int quiet)
{
    SONIX_DWORD(0x1b30) = 1;
    if (SONIX_DWORD(0x1afc) != 0xffffffffUL) {
        if (quiet == 0)
            SONIX_DWORD(0x1b2c) = 0x69;
        else
            SONIX_DWORD(0x1b2c) = 0;
    }
}
}
