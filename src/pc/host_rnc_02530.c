/* HERCULES.EXE RNC ProPack depacker 0x2530..0x30a0 (lane h1 candidate).
 * Methods 0 (stored), 1 (Huffman/LZ, LSB-first 16-bit bit stream) and 2 (LZ, MSB-first
 * byte bit stream).  Shares reset_slots/reverse_low_bits (src/pc/exe_helpers.c). Names and TU unproved. */
#include <string.h>

typedef struct Slot { long a; unsigned short b; long c; unsigned short d; } Slot;

void reset_slots(Slot *slot, unsigned char count);
unsigned int reverse_low_bits(unsigned int value, unsigned char count);

extern unsigned char g_exe_ba0240;          /* method-2 bit byte */
extern unsigned char g_exe_ba0244;          /* bits left */
extern unsigned char *g_exe_ba0248;         /* packed input cursor */
extern unsigned char *g_exe_ba024c;         /* output cursor */
extern unsigned long g_exe_ba2a64;          /* method-1 bit buffer */
extern Slot g_exe_ba2a80[16];               /* raw-length table */
extern Slot g_exe_ba2b80[16];               /* distance table */
extern Slot g_exe_ba2c80[16];               /* match-length table */
extern unsigned char *g_exe_ba2d80;         /* output end */

#define RNC_SWAP(v) ((((((((v) << 8) + (((v) >> 8) & 0xff)) << 8) + (((v) >> 16) & 0xff))) << 8) + ((v) >> 24))

static __inline unsigned short rnc_bits1(unsigned char count)
{
    unsigned short value = 0;
    unsigned long mask = 1;
    while (count--) {
        if (g_exe_ba0244 == 0) {
            g_exe_ba2a64 = *(unsigned long *)g_exe_ba0248;
            g_exe_ba0248 += 2;
            g_exe_ba0244 = 16;
        }
        if (g_exe_ba2a64 & 1)
            value |= mask;
        mask <<= 1;
        g_exe_ba2a64 >>= 1;
        g_exe_ba0244--;
    }
    return value;
}

static __inline unsigned short rnc_bits2(unsigned char count)
{
    unsigned short value = 0;
    while (count--) {
        if (g_exe_ba0244 == 0) {
            g_exe_ba0240 = *g_exe_ba0248++;
            g_exe_ba0244 = 8;
        }
        value <<= 1;
        if (g_exe_ba0240 & 0x80)
            value++;
        g_exe_ba0240 <<= 1;
        g_exe_ba0244--;
    }
    return value;
}

short __cdecl host_025f0(unsigned char *packed, unsigned char *out);
short __cdecl host_027f0(unsigned char *packed, unsigned char *out);
void __cdecl host_02b50(unsigned char *packed, unsigned char *out);
void __cdecl host_02ba0(Slot *table, unsigned char count);
unsigned short __cdecl host_02cb0(Slot *table);
unsigned short __cdecl host_02dd0(void);
unsigned short __cdecl host_02ea0(void);
void __cdecl host_030a0(Slot *table, unsigned char count);

unsigned long __cdecl host_02530(unsigned char *packed, unsigned char *out)
{
    unsigned long length;
    if (strncmp((char *)packed, "RNC", 3) != 0)
        return 0;
    switch (packed[3]) {
    case 0:
        length = *(unsigned long *)(packed + 4);
        memcpy(out, packed + 0x12, RNC_SWAP(length));
        break;
    case 1:
        host_025f0(packed, out);
        break;
    case 2:
        host_027f0(packed, out);
        break;
    default:
        return 0;
    }
    length = *(unsigned long *)(packed + 4);
    return RNC_SWAP(length);
}

short __cdecl host_025f0(unsigned char *packed, unsigned char *out)
{
    unsigned short count, length;
    unsigned char *source;
    unsigned char *next;
    host_02b50(packed, out);
    rnc_bits1(2);
    while (g_exe_ba024c < g_exe_ba2d80) {
        host_02ba0(g_exe_ba2a80, 16);
        host_02ba0(g_exe_ba2b80, 16);
        host_02ba0(g_exe_ba2c80, 16);
        count = rnc_bits1(16);
        for (;;) {
            length = host_02cb0(g_exe_ba2a80);
            while (length--)
                *g_exe_ba024c++ = *g_exe_ba0248++;
            next = g_exe_ba0248;
            g_exe_ba2a64 = (((((next[2] << 8) + next[1]) << 8) + next[0]) << g_exe_ba0244)
                + (((1 << g_exe_ba0244) - 1) & g_exe_ba2a64);
            if (--count == 0)
                break;
            source = g_exe_ba024c - host_02cb0(g_exe_ba2b80) - 1;
            length = host_02cb0(g_exe_ba2c80) + 2;
            while (length--)
                *g_exe_ba024c++ = *source++;
        }
    }
    return 0;
}

short __cdecl host_027f0(unsigned char *packed, unsigned char *out)
{
    unsigned short length, offset;
    unsigned char *source;
    host_02b50(packed, out);
    rnc_bits2(2);
    while (g_exe_ba024c < g_exe_ba2d80) {
        for (;;) {
            if (rnc_bits2(1) == 0) {
                *g_exe_ba024c++ = *g_exe_ba0248++;
                continue;
            }
            if (rnc_bits2(1)) {
                if (rnc_bits2(1) == 0) {
                    length = 2;
                    offset = *g_exe_ba0248++ + 1;
                } else {
                    if (rnc_bits2(1) == 0) {
                        length = 3;
                    } else {
                        length = *g_exe_ba0248++ + 8;
                        if (length == 8) {
                            rnc_bits2(1);
                            break;
                        }
                    }
                    offset = host_02ea0();
                }
                source = g_exe_ba024c - offset;
                while (length--)
                    *g_exe_ba024c++ = *source++;
            } else {
                length = host_02dd0();
                if (length == 9) {
                    length = (rnc_bits2(4) << 2) + 12;
                    while (length--)
                        *g_exe_ba024c++ = *g_exe_ba0248++;
                } else {
                    source = g_exe_ba024c - host_02ea0();
                    while (length--)
                        *g_exe_ba024c++ = *source++;
                }
            }
        }
    }
    return 0;
}

void __cdecl host_02b50(unsigned char *packed, unsigned char *out)
{
    g_exe_ba0248 = packed + 0x12;
    g_exe_ba024c = out;
    g_exe_ba2d80 = out + RNC_SWAP(*(unsigned long *)(packed + 4));
    g_exe_ba0244 = 0;
}

void __cdecl host_02ba0(Slot *table, unsigned char count)
{
    unsigned char i;
    reset_slots(table, count);
    count = (unsigned char)rnc_bits1(5);
    if (count) {
        if (count > 16)
            count = 16;
        for (i = 0; i < count; i++)
            table[i].d = rnc_bits1(4);
        host_030a0(table, count);
    }
}

unsigned short __cdecl host_02cb0(Slot *table)
{
    unsigned char index = 0;
    Slot *entry = table;
    while (!(entry->d && (g_exe_ba2a64 & ((1 << entry->d) - 1)) == (unsigned long)entry->c)) {
        entry++;
        index++;
    }
    rnc_bits1((unsigned char)entry->d);
    if (index < 2)
        return index;
    return (1 << (index - 1)) | rnc_bits1((unsigned char)(index - 1));
}

unsigned short __cdecl host_02dd0(void)
{
    unsigned short value = rnc_bits2(1) + 4;
    if (rnc_bits2(1) == 0)
        return value;
    return (value << 1) + rnc_bits2(1) - 2;
}

unsigned short __cdecl host_02ea0(void)
{
    unsigned short offset = 0;
    if (rnc_bits2(1)) {
        offset = rnc_bits2(1);
        if (rnc_bits2(1)) {
            offset = ((offset << 1) + rnc_bits2(1)) | 4;
            if (rnc_bits2(1) == 0)
                offset = (offset << 1) + rnc_bits2(1);
        } else if (offset == 0) {
            offset = rnc_bits2(1) + 2;
        }
    }
    return (offset << 8) + *g_exe_ba0248++ + 1;
}

/* 0x3060: recorded FUNCTION_MATCH in src/pc/exe_helpers.c; shown here at its unit position. */
void reset_slots(Slot *slot, unsigned char count)
{
    while (count--) {
        slot->a = 0; slot->b = 0xffff; slot->c = 0; slot->d = 0;
        slot++;
    }
}

void __cdecl host_030a0(Slot *table, unsigned char count)
{
    unsigned short bits = 1, i;
    unsigned long code = 0;
    unsigned long step = 0x80000000UL;
    while (bits <= 16) {
        for (i = 0; i < count; i++) {
            if (table[i].d == bits) {
                table[i].c = reverse_low_bits(code / step, (unsigned char)bits);
                code += step;
            }
        }
        bits++;
        step >>= 1;
    }
}

/* 0x3110: recorded FUNCTION_MATCH in src/pc/exe_helpers.c; shown here at its unit position. */
unsigned int reverse_low_bits(unsigned int value, unsigned char count)
{
    unsigned int result = 0;
    while (count--) {
        result <<= 1;
        if (value & 1) result |= 1;
        value >>= 1;
    }
    return result;
}
