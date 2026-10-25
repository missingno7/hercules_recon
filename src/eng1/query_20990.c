/* Ordinary-C reconstruction of the native ENG1 RVA 0x20990. */
#include "query_20990.h"
#include "incomplete.h"

typedef unsigned char query_u8;
typedef unsigned short query_u16;
typedef unsigned long query_u32;
typedef long query_s32;

typedef char query_20990_requires_32bit_pointer[(sizeof(void *) == 4) ? 1 : -1];
typedef char query_20990_requires_32bit_ulong[(sizeof(query_u32) == 4) ? 1 : -1];

extern query_u32 g_eng1_grid_query_coefficient_prefix_5e0d0[26];
extern query_u8 g_actor_group_sequence_5e138[0x70];

static query_u32 query_20990_sar8(query_u32 bits)
{
    query_u32 shifted;
    shifted = bits >> 8;
    if ((bits & 0x80000000UL) != 0UL)
        shifted |= 0xff000000UL;
    return shifted;
}

static query_s32 query_20990_sar4(query_s32 value)
{
    query_u32 bits;
    bits = (query_u32)value;
    bits >>= 4;
    if (((query_u32)value & 0x80000000UL) != 0UL)
        bits |= 0xf0000000UL;
    return (query_s32)bits;
}

static void query_20990_unavailable(query_u32 rva, const char *reason)
{
    eng1_incomplete_behavior(rva, reason);
    for (;;)
        ;
}

/*
 * This accessor is intentionally local to 0x20990. The 104-byte prefix is
 * one existing owner; byte offsets 104..212 are views into the existing
 * mutable 112-byte 0x5e138 owner. No third coefficient array is introduced.
 */
static query_u32 query_20990_coefficient(query_s32 index, query_u32 lane)
{
    query_s32 signed_offset;
    query_u32 offset;
    query_u32 rva;
    volatile query_u32 *word;

    signed_offset = index * 16L + (query_s32)(lane * 4UL);
    if (signed_offset < 0L) {
        rva = 0x5e0d0UL + (query_u32)signed_offset;
        query_20990_unavailable(rva,
            "0x20990 coefficient address precedes the measured source owners");
    }

    offset = (query_u32)signed_offset;
    if (offset <= 100UL) {
        word = (volatile query_u32 *)((volatile query_u8 *)
            g_eng1_grid_query_coefficient_prefix_5e0d0 + offset);
        return *word;
    }
    if (offset >= 104UL && offset <= 212UL) {
        word = (volatile query_u32 *)(g_actor_group_sequence_5e138 +
                                      (offset - 104UL));
        return *word;
    }

    query_20990_unavailable(0x5e0d0UL + offset,
        "0x20990 coefficient DWORD is outside both measured owner views");
    return 0UL;
}

static query_u8 query_20990_program_byte(const query_u8 *program,
                                         query_u32 offset)
{
    query_u32 address;
    address = (query_u32)(unsigned long)program + offset;
    return *(volatile query_u8 *)(unsigned long)address;
}

static query_u32 query_20990_cursor_address(const query_u8 *program,
                                           query_u32 offset)
{
    return (query_u32)(unsigned long)program + offset;
}

static query_u32 query_20990_interpolate(query_u32 from, query_u32 to,
                                         query_u32 factor)
{
    return from + query_20990_sar8((to - from) * factor);
}

long ENG1_QUERY_20990_CDECL query_decode_20990(
    query_u32 x, query_u32 y, const query_u8 *program)
{
    query_s32 tag;
    query_u32 cursor;
    query_u32 sample0;
    query_u32 sample1;
    query_u32 sample2;
    query_u32 sample3;
    query_u32 low_index;
    query_s32 high_index;
    query_u32 low0;
    query_u32 high0;
    query_u32 low3;
    query_u32 low_factor;
    query_u32 high_factor;
    query_u32 first_row;
    query_u32 second_row;

    tag = (query_s32)(signed char)query_20990_program_byte(program, 0UL);
    cursor = 1UL;

    if (tag == 0L) {
        sample0 = (query_u32)(query_s32)(signed char)
            query_20990_program_byte(program, cursor);
        if ((query_s32)sample0 < 0L)
            return -1L;
        if (sample0 >= 64UL)
            return (long)((sample0 << 1) & 0x7fUL);
        sample0 <<= 1;
        ++cursor;
        sample1 = (query_u32)(query_20990_program_byte(program, cursor) & 0x3fU);
        sample1 <<= 1;
        low_factor = x & query_20990_coefficient(0L, 1UL);
        low_factor *= query_20990_coefficient(0L, 2UL);
        return (long)query_20990_interpolate(sample0, sample1, low_factor);
    }

    if (tag > 0L && tag < 16L) {
        low_index = (query_u32)tag;
        low0 = query_20990_coefficient((query_s32)low_index, 0UL);
        cursor += query_20990_sar8(low0 * x);
        sample0 = (query_u32)(query_s32)(signed char)
            query_20990_program_byte(program, cursor);
        if ((query_s32)sample0 < 0L)
            return -1L;
        if (sample0 >= 64UL)
            return (long)((sample0 << 1) & 0x7fUL);
        sample0 <<= 1;
        ++cursor;
        sample1 = (query_u32)(query_20990_program_byte(program, cursor) & 0x3fU);
        sample1 <<= 1;
        low_factor = x & query_20990_coefficient((query_s32)low_index, 1UL);
        low_factor *= query_20990_coefficient((query_s32)low_index, 2UL);
        return (long)query_20990_interpolate(sample0, sample1, low_factor);
    }

    low_index = (query_u32)tag & 0x0fUL;
    high_index = query_20990_sar4(tag) - 1L;
    low0 = query_20990_coefficient((query_s32)low_index, 0UL);
    high0 = query_20990_coefficient(high_index, 0UL);
    low3 = query_20990_coefficient((query_s32)low_index, 3UL);
    cursor += query_20990_sar8(low0 * x);
    cursor += query_20990_sar8(high0 * y) * (low3 + 1UL);

    sample0 = (query_u32)(query_s32)(signed char)
        query_20990_program_byte(program, cursor);
    if ((query_s32)sample0 < 0L)
        return -1L;
    if (sample0 >= 64UL)
        return (long)((sample0 << 1) & 0x7fUL);
    sample0 <<= 1;
    ++cursor;

    low_factor = x & query_20990_coefficient((query_s32)low_index, 1UL);
    low_factor *= query_20990_coefficient((query_s32)low_index, 2UL);
    sample1 = (query_u32)(query_20990_program_byte(program, cursor) & 0x3fU);
    sample1 <<= 1;
    first_row = query_20990_interpolate(sample0, sample1, low_factor);

    cursor += low3;
    sample2 = (query_u32)(query_20990_program_byte(program, cursor) & 0x3fU);
    ++cursor;
    sample3 = (query_u32)(query_20990_program_byte(program, cursor) & 0x3fU);
    sample2 <<= 1;
    sample3 <<= 1;
    second_row = query_20990_interpolate(sample2, sample3, low_factor);

    high_factor = y & query_20990_coefficient(high_index, 1UL);
    high_factor *= query_20990_coefficient(high_index, 2UL);
    return (long)query_20990_interpolate(first_row, second_row, high_factor);
}
