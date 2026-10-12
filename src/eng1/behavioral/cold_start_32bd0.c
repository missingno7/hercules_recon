/* Behavioral C behavioral model for the ENG1 restart/frame entry 32bd0.
 * The byte names are observed cells, not original structure/member names. */
#include "../../../calibration/engine_interface.h"
#include "cold_start_frame_data.h"
#include "incomplete.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned long u32;
typedef long s32;

extern u8 g_cold_start_state_bytes_70820[0x1c];
extern int g_context_sequence_index;
extern u32 g_frame_global_73484;
extern u8 *g_dispatch_frame_current_71ec4;

/* Separate entries with independently reviewed starts.  Their effects are
 * deliberately left external; the fixture records only the calls we make. */
extern void __cdecl dispatch_local_2c60(const u16 rectangle[4],
                                        u32 mode, u32 key);
extern void __cdecl dispatch_local_33420(s32, s32, s32, s32, s32, s32, s32);
extern void __cdecl dispatch_local_33620(s32, s32, s32, s32, s32, s32, s32);
extern void __cdecl dispatch_local_1f40(void);
extern void __cdecl dispatch_engine_2c980(void *, u32, u32, u32, u32);

static s16 state_word(unsigned offset)
{
    return *(const s16 *)(const void *)(g_cold_start_state_bytes_70820 + offset);
}

static void store_state_word(unsigned offset, s16 value)
{
    *(s16 *)(void *)(g_cold_start_state_bytes_70820 + offset) = value;
}

static s16 context_word(const u8 *context, unsigned offset)
{
    return *(const s16 *)(const void *)(context + offset);
}

static void store_context_word(u8 *context, unsigned offset, s16 value)
{
    *(s16 *)(void *)(context + offset) = value;
}

/* The target's signed multiply-and-SAR sequence rounds toward zero. */
static s32 divide_by_256_toward_zero(s32 value)
{
    if (value < 0)
        return -((-value) / 256);
    return value / 256;
}

static s32 clamp(s32 value, s32 low, s32 high)
{
    if (value < low)
        return low;
    if (value > high)
        return high;
    return value;
}

static void draw_33620(s32 a, s32 b, s32 c, s32 d,
                       s32 e, s32 f, s32 g)
{
    dispatch_local_33620(a, b, c, d, e, f, g);
}

static void draw_33420(s32 a, s32 b, s32 c, s32 d,
                       s32 e, s32 f, s32 g)
{
    dispatch_local_33420(a, b, c, d, e, f, g);
}

static u32 repeat_byte(u8 value)
{
    return (u32)value | ((u32)value << 8) |
           ((u32)value << 16) | ((u32)value << 24);
}

static void coordinate_pair(u16 row, u8 *first, u8 *second)
{
    *first = g_frame_coordinate_prefix_5dd80[(u32)row * 2UL];
    *second = g_frame_coordinate_prefix_5dd80[(u32)row * 2UL + 1UL];
}

static void draw_changed_pair(unsigned state_offset, unsigned cache_offset,
                              u32 mode)
{
    u16 current;
    u16 cached;
    u16 rectangle[4];
    u8 coordinate_x;
    u8 coordinate_y;

    current = *(u16 *)(void *)(g_cold_start_state_bytes_70820 + state_offset);
    cached = *(u16 *)(void *)(g_cold_start_state_bytes_70820 + cache_offset);
    if (current == cached)
        return;

    /* Only 192 two-byte pairs are measured at 0x5dd80. Stop before either
     * the native-order cache write or an unsupported table access. */
    if (current >= COLD_FRAME_COORDINATE_PREFIX_ROWS) {
        eng1_incomplete_behavior(
            0x32bd0UL,
            "frame coordinate row exceeds the measured 192-pair prefix");
        return;
    }

    *(u16 *)(void *)(g_cold_start_state_bytes_70820 + cache_offset) = current;
    coordinate_pair(current, &coordinate_x, &coordinate_y);
    rectangle[0] = (u16)((u16)coordinate_x * 12U);
    rectangle[1] = (u16)((u16)coordinate_y * 24U + 0x238U);
    rectangle[2] = 0x000cU;
    rectangle[3] = 0x0018U;
    dispatch_local_2c60(rectangle, mode, 0x238UL);
}

static void move_toward(unsigned context_offset, s16 target)
{
    s16 value;

    value = context_word((const u8 *)g_engine_interface.context_004,
                         context_offset);
    if (value < target)
        value = (s16)(value + 1);
    else if (value > target)
        value = (s16)(value - 1);
    store_context_word((u8 *)g_engine_interface.context_004,
                       context_offset, value);
}

static void move_state_toward(unsigned offset, s16 target)
{
    s16 value;

    value = state_word(offset);
    if (value < target)
        value = (s16)(value + 1);
    else if (value > target)
        value = (s16)(value - 1);
    store_state_word(offset, value);
}

static void draw_vertical_layout(s32 height, s32 position)
{
    s32 span;
    s32 pieces;
    s32 index;
    s32 rows;
    s32 remainder;
    s32 tail;
    s32 extra;

    span = height - 12;
    pieces = (span + 63) / 64;
    for (index = 0; index < pieces; ++index)
        draw_33620(0x5c + index * 64, 0x1a, 0x18, 0x28,
                   0x40, 0x0d, 0);

    draw_33620(0x5c + pieces * 64, 0x1a, 0x18, 0x28,
               span & 0x3f, 0x0d, 0);

    rows = position / 6;
    remainder = position % 24;
    for (index = 0; index < rows; ++index)
        draw_33620(0x52 + index * 24, 0x1d, 0, 0x28, 0x18, 6, 1);
    draw_33620(0x52 + rows * 24, 0x1d, 0, 0x28,
               remainder, 6, 1);

    tail = 24 - remainder;
    extra = height / 6 - rows;
    if (extra == 0) {
        draw_33620(0x52, 0x1d, remainder, 0x30, height, 6, 1);
    } else {
        draw_33620(0x52, 0x1d, remainder, 0x30, tail, 6, 1);
    }

    if (extra > 1) {
        for (index = 0; index < extra - 1; ++index)
            draw_33620(0x52 + (rows + 1 + index) * 24,
                       0x1d, 0, 0x30, 0x18, 6, 1);
    }

    span = height - 24 * (2 * rows + 1);
    if (span > 0)
        draw_33620(0x52 + 24 * (2 * rows + 1),
                   0x1d, 0, 0x30, span, 6, 1);
}

static void draw_scaled_layout(s32 scale)
{
    s32 base_x;
    s32 base_y;
    s32 index;
    s32 rows;
    s32 remainder;
    s32 extra;
    s32 offset;
    s32 remaining;

    base_x = (s32)state_word(0x18);
    base_y = (s32)state_word(0x1a);
    draw_33420(base_x - 20, base_y, 160, 80, 36, 16, 0);
    draw_33420(base_x + 48, base_y, 200, 80, 36, 16, 0);
    for (index = 0; index <= 64; index += 64)
        draw_33420(base_x + index, base_y + 3, 24, 40, 64, 13, 0);

    rows = scale / 6;
    remainder = scale % 24;
    for (index = 0; index < rows; ++index)
        draw_33420(base_x + index * 24, base_y + 6,
                   0, 40, 24, 6, 1);
    draw_33420(base_x + rows * 24, base_y + 6,
               0, 40, remainder, 6, 1);
    draw_33420(base_x + scale, base_y + 6,
               remainder, 48, 24 - remainder, 6, 1);

    extra = (64 - scale) / 6;
    offset = 24 * (rows + 1);
    for (index = 0; index < extra; ++index)
        draw_33420(base_x + offset + index * 24,
                   base_y + 6, 0, 48, 24, 6, 1);

    remaining = 64 - offset - extra * 24;
    if (remaining > 0)
        draw_33420(base_x + offset + extra * 24, base_y + 6,
                   0, 48, remaining, 6, 1);
}

void __cdecl dispatch_local_32bd0(void)
{
    u8 *context;
    s32 sequence_frame;
    s32 sequence_row;
    s32 height;
    s32 position;
    s32 scale;
    s32 blend;
    s32 complement;
    s32 color_a;
    s32 color_b;
    s32 index;
    u32 cursor;

    context = (u8 *)g_engine_interface.context_004;

    if (context[6] == 3) {
        /* Native 0x32bed uses LEA ECX,[EAX+1e], so the addition wraps in
         * 32-bit arithmetic before the signed division.  Avoid signed-C
         * overflow while retaining that measured register behavior. */
        sequence_frame = (s32)((u32)g_context_sequence_index + 30UL);
        sequence_row = sequence_frame / 30;
        draw_33620(0x18, 0xd0, 0xf0,
                   (sequence_row / 10) * 16 + 0x28, 0x10, 0x10, 3);
        draw_33620(0x25, 0xd0, 0xf0,
                   (sequence_row % 10) * 16 + 0x28, 0x10, 0x10, 3);
        /* Native 0x32c67 decrements the DWORD and 0x32c6d tests its sign. */
        g_context_sequence_index =
            (s32)((u32)g_context_sequence_index - 1UL);
        if (g_context_sequence_index < 0)
            context[3] = 0x13;
    }

    context = (u8 *)g_engine_interface.context_004;
    if (context[4] == 0) {
        for (index = 0; index < g_cold_start_state_bytes_70820[8]; ++index)
            move_toward(0x22, context_word(context, 0x24));
        for (index = 0; index < g_cold_start_state_bytes_70820[9]; ++index)
            move_toward(0x20, context_word(context, 0x1e));
        for (index = 0; index < g_cold_start_state_bytes_70820[0x0a]; ++index)
            move_state_toward(0x10, state_word(0x12));
    }

    draw_changed_pair(0x00, 0x04, 0);
    draw_changed_pair(0x02, 0x06, 0x0c);
    context = (u8 *)g_engine_interface.context_004;

    height = divide_by_256_toward_zero(
        (s32)context_word(context, 0x20) * 146);
    height = clamp(height, 0x1c, 0xff);
    position = divide_by_256_toward_zero(
        (s32)context_word(context, 0x22) * 146);
    position = clamp(position, 0, height - 1);

    if (*(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x0c) != 0) {
        blend = (18 * (s32)*(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x0c)) / 256;
        draw_33620(0x116, 0x15 + 18 - blend, 0xc8,
                   18 - blend, 0x18, blend, 1);
    }
    if (*(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x0e) != 0) {
        blend = (20 * (s32)*(u16 *)(void *)(g_cold_start_state_bytes_70820 + 0x0e)) / 256;
        draw_33620(0x3f, 0x25 + 20 - blend, 0xd0,
                   0x12 + 20 - blend, 0x18, blend, 1);
    }

    draw_33620(0xec, 0x14, 0x88, 0, 0x40, 0x1f, 0);
    draw_33620(height + 0x41, 0x17, 0xc8, 0x50, 0x24, 0x10, 0);
    draw_33620(0x10, 0x14, 0, 0, 0x50, 0x25, 0);
    draw_33620(0x60, 0x24, 0xc0, 0x20, 0x10, 5, 0);
    draw_33620(0x2b, 0x17, 0, 0x38, 0x18, 0x18, 2);
    draw_33620(0x103, 0x17, 0x18, 0x38, 0x18, 0x18, 2);
    draw_vertical_layout(height, position);

    if (g_cold_start_state_bytes_70820[0x14] != 0) {
        if (g_cold_start_state_bytes_70820[0x14] == 2) {
            blend = (s32)state_word(0x16);
            blend = clamp(blend, 0, 0x80);
            g_frame_global_6ffd0 = (u16)blend;
            /* Native 0x33146 reloads the DWORD after the WORD store and
             * 0x33152 explicitly keeps its low WORD for both colors. */
            blend = (s32)(*(u32 *)(void *)
                (g_frame_color_state_prefix_6ffc8 + 0x08) & 0xffffUL);
            store_state_word(0x16, (s16)blend);
            complement = 0x80 - blend;
            color_a = 0x80 - (complement * complement) / 0x80;
            color_b = (blend * blend) / 0x80;
            g_frame_global_6ffc8 = repeat_byte((u8)color_a);
            g_frame_global_6ffcc = repeat_byte((u8)color_b);
        }

        scale = divide_by_256_toward_zero(
            (s32)state_word(0x10) * 64);
        scale = clamp(scale, 0, 0x3f);
        draw_scaled_layout(scale);
    }

    dispatch_local_1f40();
    dispatch_engine_2c980((void *)g_frame_global_73484,
                          0, 0, 0x180, 0);

    cursor = g_frame_global_73484;
    *(u32 *)(void *)cursor =
        *(u32 *)(void *)(g_dispatch_frame_current_71ec4 + 0x1374);

    cursor = g_frame_global_73484;
    *(u32 *)(void *)(g_dispatch_frame_current_71ec4 + 0x1374) = cursor;

    g_frame_global_73484 += 0x0c;
}
