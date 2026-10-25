/* Measured ENG1 audio thunks over the copied EngineInterface prefix.
 * Function-pointer views use the callback slots already owned by the host
 * interface provider; this file introduces no callback storage. */
#include "../shared/engine_interface.h"

typedef void (__cdecl *AudioNoArg)(void);
typedef void (__cdecl *AudioStatus)(signed char *);
typedef void (__cdecl *AudioInfo)(int, unsigned long *);
typedef void (__cdecl *AudioSetChannel)(short, short, short);
typedef void (__cdecl *AudioGetChannel)(short, short *, short *);
typedef void (__cdecl *AudioSetMusic)(short, short);
typedef short (__cdecl *AudioStart)(unsigned short, unsigned short,
                                   int, int, int, short, short);

/* ENG1:2caf0 -> copied-table offset 0x370, host EXE:0xbe30. */
void __cdecl dispatch_engine_2caf0(void)
{
    ((AudioNoArg)g_engine_interface.opaque_334[15])();
}

/* ENG1:2ca60 -> copied-table offset 0x300, host EXE:0xc0f0. */
void __cdecl dispatch_engine_2ca60(signed char *status)
{
    ((AudioStatus)g_engine_interface.opaque_2f0[4])(status);
}

/* ENG1:2ca70 -> copied-table offset 0x310, host EXE:0xc0e0. */
void __cdecl dispatch_engine_2ca70(int channel, unsigned long *info)
{
    ((AudioInfo)g_engine_interface.opaque_2f0[8])(channel, info);
}

/* ENG1:2cab0 -> copied-table offset 0x35c, host EXE:0xc200. */
void __cdecl dispatch_engine_2cab0(short left, short right)
{
    ((AudioSetMusic)g_engine_interface.opaque_334[10])(left, right);
}

/* ENG1:2cb00 -> copied-table offset 0x374, host EXE:0xc1c0. */
void __cdecl dispatch_engine_2cb00(short channel, short *left, short *right)
{
    ((AudioGetChannel)g_engine_interface.opaque_334[16])(
        channel, left, right);
}

/* ENG1:2cb80 -> copied-table offset 0x388, host EXE:0xbf20.
 * Preserve the signed-short channel result returned by the host callback. */
short __cdecl dispatch_engine_2cb80(unsigned short bank,
                                    unsigned short sample,
                                    int zero_a, int frequency, int zero_b,
                                    short left, short right)
{
    return ((AudioStart)g_engine_interface.opaque_334[21])(
        bank, sample, zero_a, frequency, zero_b, left, right);
}

/* ENG1:2cbb0 -> copied-table offset 0x390, host EXE:0xc160. */
void __cdecl dispatch_engine_2cbb0(short channel, short left, short right)
{
    ((AudioSetChannel)g_engine_interface.opaque_334[23])(
        channel, left, right);
}
