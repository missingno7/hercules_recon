/* HERCULES.EXE PSX-shim continuation 0x41a0..0x42e0 (lane h4 candidate):
 * pad read, display-buffer put, bounded random and its generator.
 * Names and TU unproved; continues the h1 shim view (calibration/host_psx_shim_01bd0.c). */
#include "../shared/engine_interface.h"

extern EngineInterface g_host_engine_interface;
/* PSX pad receive buffer registered by host_input_packets_initialize_4180:
 * byte 0 status (0xff = no pad), byte 1 type/size, bytes 2..3 active-low buttons. */
extern unsigned char g_host_setup_input_buffer1_7a2a40[];
extern int __cdecl host_frame_header_copy(const unsigned long *source);
extern int __cdecl host_zero_callback();

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

#define CTX (g_host_engine_interface.context_004)
#define CTX_BYTES ((unsigned char *)g_host_engine_interface.context_004)
#define CTX_WORD(off) (*(unsigned short *)(CTX_BYTES + (off)))

unsigned long __cdecl host_04280(void);

void __cdecl host_041a0(void)
{
    unsigned int pad;
    unsigned int previous;

    pad = (unsigned short)~*(unsigned short *)(g_host_setup_input_buffer1_7a2a40 + 2);
    CTX->unknown_060 = 0;
    if (g_host_setup_input_buffer1_7a2a40[0] == 0xff)
        CTX->unknown_060 |= 1;
    else if ((g_host_setup_input_buffer1_7a2a40[1] & 0xf0) != 0x40)
        CTX->unknown_060 |= 2;
    if (CTX->unknown_060 != 0)
        pad = 0;
    previous = CTX->unknown_05c;
    CTX->unknown_05c = (unsigned short)pad;
    CTX->unknown_05e = (unsigned short)((previous ^ CTX->unknown_05c) & CTX->unknown_05c);
    CTX_WORD(0x64) = 0;
    CTX->unknown_062 = 0;
    CTX_WORD(0x66) = 0;
}

void __cdecl host_04240(HostDisplayBuffer *buffer)
{
    host_frame_header_copy((const unsigned long *)buffer->draw_000);
    host_zero_callback(buffer->disp_05c);
}

unsigned long __cdecl host_04260(unsigned long range)
{
    if (range == 0)
        return 0;
    return host_04280() % range;
}

unsigned long __cdecl host_04280(void)
{
    CTX->unknown_048 += CTX->unknown_04c;
    if (CTX->unknown_048 > 0xffff) {
        CTX->unknown_048 &= 0xffff;
        CTX->unknown_04c = (CTX->unknown_048 + CTX->unknown_04c + 1) & 0xffff;
    } else {
        CTX->unknown_04c = (CTX->unknown_04c + CTX->unknown_048) & 0xffff;
    }
    return CTX->unknown_04c;
}
