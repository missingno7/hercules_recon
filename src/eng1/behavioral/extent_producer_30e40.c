/* Ordinary-C reconstruction of ENG1.DLL RVAs 0x30e40 and 0x31ec0.
 * The typed 0x31080 metadata reader is reused from calibration/resource_metadata.c.
 * Dynamic profile-data extent behind g_header_pointer_70d2c remains unbounded. */
#include "extent_producer_30e40.h"
#include "resource_runtime_data.h"
#include "incomplete.h"

typedef unsigned char extent_u8;
typedef unsigned short extent_u16;
typedef unsigned long extent_u32;
typedef signed char extent_s8;
typedef signed short extent_s16;
typedef signed long extent_s32;
typedef char extent_requires_32bit_pointer[(sizeof(void *) == 4) ? 1 : -1];
typedef char extent_requires_32bit_ulong[(sizeof(extent_u32) == 4) ? 1 : -1];

extern ActorResource g_actor_resources[81];
extern void *__cdecl resource_metadata_pointer(
    unsigned long resource_id, int metadata_index);

static extent_s16 extent_word_signed(extent_u32 value)
{
    return (extent_s16)(extent_u16)value;
}

static extent_s32 extent_scale_signed(extent_s32 value, extent_u16 scale)
{
    extent_u32 product = (extent_u32)value * (extent_u32)scale;
    return (extent_s32)product >> 8;
}

static extent_u16 extent_scale_unsigned(extent_u16 value, extent_u16 scale)
{
    extent_u32 product = (extent_u32)value * (extent_u32)scale;
    return (extent_u16)(product >> 8);
}

static extent_s16 extent_subtract_bytes(extent_s8 left, extent_s8 right)
{
    return extent_word_signed((extent_u32)((extent_s32)left -
                                            (extent_s32)right));
}

static extent_s16 extent_half_byte(extent_s8 value)
{
    return (extent_s16)((extent_s32)value >> 1);
}

int __cdecl profile_extent_31ec0(
    void *actor, void *extent_words, extent_u32 mode)
{
    volatile extent_u8 *actor_bytes = (volatile extent_u8 *)actor;
    volatile extent_u8 *profile_root;
    volatile extent_u8 *profile_row;
    volatile extent_u8 *record_cursor;
    volatile extent_u8 *selected_tail;
    volatile extent_u16 *out = (volatile extent_u16 *)extent_words;
    extent_u16 profile_index;
    extent_u8 count_byte;
    extent_s32 signed_mode = (extent_s32)mode;
    extent_u32 relative_row;
    extent_u32 first_dword;
    extent_u32 tail_dword;
    extent_u32 following_dword;
    extent_s32 first_axis;
    extent_s32 second_axis;
    extent_s32 third_axis;
    extent_u32 fourth_axis;
    extent_u32 fifth_axis;
    extent_u32 sixth_axis;
    extent_s16 first_out;
    extent_s16 second_out;
    extent_s16 third_out;
    extent_u16 fourth_out;
    extent_u16 fifth_out;
    extent_u16 sixth_out;
    extent_u16 scale_a;
    extent_u16 scale_b;
    extent_u16 scale_c;
    extent_u8 actor_flags;

    profile_index = *(volatile extent_u16 *)(void *)(actor_bytes + 0x34);
    profile_root = (volatile extent_u8 *)g_header_pointer_70d2c;
    if (profile_root == 0)
        eng1_incomplete_behavior(0x70d2cUL,
            "0x31ec0 profile-root pointer has no reconstructed data");
    if (signed_mode < 0)
        eng1_incomplete_behavior(0x31ec0UL,
            "0x31ec0 negative mode selects an unverified preceding record");
    relative_row = *(volatile extent_u32 *)(void *)(profile_root +
        (extent_u32)profile_index * 4UL);
    profile_row = profile_root + relative_row +
        (extent_u32)profile_index * 4UL + 2UL;
    count_byte = *(volatile extent_u8 *)(void *)profile_row;
    record_cursor = profile_row + 2;
    first_dword = *(volatile extent_u32 *)(void *)record_cursor;
    record_cursor += 4;

    if (mode == 0UL) {
        if (count_byte == 0U)
            return 0;
    } else if (signed_mode > (extent_s32)count_byte) {
        return 0;
    } else {
        record_cursor += (extent_s32)(mode * 8UL - 8UL);
    }

    tail_dword = *(volatile extent_u32 *)(void *)record_cursor;
    selected_tail = record_cursor + 4;
    following_dword = *(volatile extent_u32 *)(void *)selected_tail;

    first_axis = (extent_s16)(extent_u16)(tail_dword >> 8);
    first_axis = (extent_s16)first_axis;
    second_axis = (extent_s8)(extent_u8)(tail_dword >> 24);
    third_axis = (extent_s8)(extent_u8)tail_dword;

    if (first_dword != 0UL) {
        first_axis += (extent_s8)(extent_u8)first_dword;
        second_axis += (extent_s16)(extent_u16)(first_dword >> 8);
        third_axis += (extent_s8)(extent_u8)(first_dword >> 24);
    }

    fourth_axis = (following_dword >> 8) & 0xffffUL;
    fifth_axis = (following_dword >> 24) & 0xffUL;
    sixth_axis = following_dword & 0xffUL;

    actor_flags = *(volatile extent_u8 *)(void *)(actor_bytes + 0x54);
    scale_a = *(volatile extent_u16 *)(void *)(actor_bytes + 0x3a);
    if ((actor_flags & 0x80U) != 0U) {
        scale_b = *(volatile extent_u16 *)(void *)(actor_bytes + 0x3c);
        scale_c = *(volatile extent_u16 *)(void *)(actor_bytes + 0x3e);
        if (((extent_u16)(scale_a | scale_b | scale_c)) != 0x0100U) {
            first_axis = extent_scale_signed(first_axis, scale_a);
            fourth_axis = extent_scale_unsigned(
                (extent_u16)fourth_axis, scale_a);
            second_axis = extent_scale_signed(second_axis, scale_b);
            fifth_axis = extent_scale_unsigned(
                (extent_u16)fifth_axis, scale_b);
            third_axis = extent_scale_signed(third_axis, scale_c);
            sixth_axis = extent_scale_unsigned(
                (extent_u16)sixth_axis, scale_c);
        }
    } else if (scale_a != 0x0100U) {
        first_axis = extent_scale_signed(first_axis, scale_a);
        second_axis = extent_scale_signed(second_axis, scale_a);
        third_axis = extent_scale_signed(third_axis, scale_a);
        fourth_axis = extent_scale_unsigned(
            (extent_u16)fourth_axis, scale_a);
        fifth_axis = extent_scale_unsigned(
            (extent_u16)fifth_axis, scale_a);
        sixth_axis = extent_scale_unsigned(
            (extent_u16)sixth_axis, scale_a);
    }

    first_out = (extent_s16)first_axis;
    second_out = (extent_s16)second_axis;
    third_out = (extent_s16)third_axis;
    fourth_out = (extent_u16)fourth_axis;
    fifth_out = (extent_u16)fifth_axis;
    sixth_out = (extent_u16)sixth_axis;

    out[0] = (extent_u16)first_out;
    out[5] = sixth_out;
    out[2] = (extent_u16)third_out;
    out[1] = (extent_u16)second_out;
    out[4] = fifth_out;
    out[3] = fourth_out;
    return (int)count_byte;
}

int __cdecl extent_producer_30e40(
    void *actor, void *extent_words, extent_u32 mode)
{
    volatile extent_u8 *actor_bytes = (volatile extent_u8 *)actor;
    volatile extent_u16 *out = (volatile extent_u16 *)extent_words;
    extent_u16 metadata_index;
    extent_u8 resource_id;
    extent_u16 resource_count;
    extent_s32 signed_count;
    extent_s32 signed_mode = (extent_s32)mode;
    volatile extent_u8 *metadata;
    volatile extent_u8 *record;
    extent_s8 record_count;
    extent_s8 record_byte_0;
    extent_s8 record_byte_1;
    extent_s8 record_byte_2;
    extent_s8 record_byte_3;
    extent_s8 record_byte_4;
    extent_s8 record_byte_5;
    extent_s16 values[6];
    extent_u16 scale_a;
    extent_u16 scale_b;
    extent_u8 actor_flags;
    extent_u32 i;

    metadata_index = *(volatile extent_u16 *)(void *)(actor_bytes + 0x34);
    if (metadata_index == 0U)
        return 0;

    resource_id = *(volatile extent_u8 *)(void *)(actor_bytes + 0x22);
    if (resource_id == 0U)
        return profile_extent_31ec0(actor, extent_words, mode);

    if (resource_id >= 81U)
        eng1_incomplete_behavior(0x5cee0UL,
            "0x30e40 resource-row index exceeds the measured 81-row prefix");

    resource_count = g_actor_resources[resource_id].entry_count;
    if (metadata_index > resource_count)
        return 0;

    metadata = (volatile extent_u8 *)resource_metadata_pointer(
        (extent_u32)resource_id, (int)metadata_index);
    if (metadata == 0)
        return 0;

    record_count = (extent_s8)*(volatile extent_u8 *)(void *)(metadata + 1);
    signed_count = (extent_s32)record_count;
    if (mode == 0UL) {
        if (record_count == 0)
            return 0;
        record = metadata + 2;
    } else {
        if (signed_mode < 0)
            eng1_incomplete_behavior(0x30e40UL,
                "0x30e40 negative mode selects an unverified preceding record");
        if (signed_mode > signed_count)
            return 0;
        record = metadata + (extent_s32)(mode * 6UL - 4UL);
    }

    if (*(volatile extent_u16 *)(void *)record == 0U &&
        *(volatile extent_u16 *)(void *)(record + 2) == 0U &&
        *(volatile extent_u16 *)(void *)(record + 4) == 0U)
        return 0;

    record_byte_0 = (extent_s8)*(volatile extent_u8 *)(void *)record;
    record_byte_1 = (extent_s8)*(volatile extent_u8 *)(void *)(record + 1);
    record_byte_2 = (extent_s8)*(volatile extent_u8 *)(void *)(record + 2);
    record_byte_3 = (extent_s8)*(volatile extent_u8 *)(void *)(record + 3);
    record_byte_4 = (extent_s8)*(volatile extent_u8 *)(void *)(record + 4);
    record_byte_5 = (extent_s8)*(volatile extent_u8 *)(void *)(record + 5);
    values[0] = record_byte_0;
    values[1] = record_byte_1;
    values[2] = extent_half_byte(record_byte_2);
    values[3] = extent_subtract_bytes(record_byte_3, record_byte_0);
    values[4] = extent_subtract_bytes(record_byte_4, record_byte_1);
    values[5] = extent_word_signed((extent_u32)(extent_s32)(
        extent_half_byte(record_byte_5) - values[2]));

    actor_flags = *(volatile extent_u8 *)(void *)(actor_bytes + 0x54);
    scale_a = *(volatile extent_u16 *)(void *)(actor_bytes + 0x3a);
    if ((actor_flags & 0x80U) != 0U) {
        scale_b = *(volatile extent_u16 *)(void *)(actor_bytes + 0x3c);
        if (scale_a != 0x0100U) {
            values[0] = (extent_s16)extent_scale_signed(values[0], scale_a);
            values[3] = (extent_s16)extent_scale_signed(values[3], scale_a);
            values[2] = (extent_s16)extent_scale_signed(values[2], scale_a);
            values[5] = (extent_s16)extent_scale_signed(values[5], scale_a);
        }
        if (scale_b != 0x0100U) {
            values[1] = (extent_s16)extent_scale_signed(values[1], scale_a);
            values[4] = (extent_s16)extent_scale_signed(values[4], scale_a);
        }
    } else if (scale_a != 0x0100U) {
        for (i = 0; i != 6UL; ++i)
            values[i] = (extent_s16)extent_scale_signed(values[i], scale_a);
    }

    out[0] = (extent_u16)values[0];
    out[4] = (extent_u16)values[4];
    out[2] = (extent_u16)values[2];
    out[1] = (extent_u16)values[1];
    out[3] = (extent_u16)values[3];
    out[5] = (extent_u16)values[5];
    return (int)record_count;
}
