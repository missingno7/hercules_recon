/* Ordinary-C reconstruction of the unpatched ENG1 tile command writer at 2ff0.
 * This is semantic reconstruction, not a function-byte or original-source claim.
 */
#include "tile_draw_storage.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef long s32;

typedef char tile_draw_32bit_ulong[(sizeof(u32) == 4) ? 1 : -1];
typedef char tile_draw_16bit_word[(sizeof(u16) == 2) ? 1 : -1];

extern u8 *g_header_pointer_70d44;
extern u32 g_frame_global_70cf8;
extern u32 g_frame_global_73484;
extern u8 *g_dispatch_frame_current_71ec4;

static s32 arithmetic_shift_right_4(u32 bits)
{
    u32 shifted;

    shifted = bits >> 4;
    if ((bits & 0x80000000UL) != 0)
        shifted |= 0xf0000000UL;
    return (s32)shifted;
}

static s32 add_wrap32(s32 first, s32 second)
{
    return (s32)((u32)first + (u32)second);
}

static s32 multiply_wrap32(s32 first, s32 second)
{
    return (s32)((u32)first * (u32)second);
}

static s32 read_s16(const u8 *bytes)
{
    return (s32)*(const short *)(const void *)bytes;
}

static u16 read_u16(const u8 *bytes)
{
    return *(const u16 *)(const void *)bytes;
}

static void write_u16(u8 *bytes, u16 value)
{
    *(u16 *)(void *)bytes = value;
}

static void write_u32(u8 *bytes, u32 value)
{
    *(u32 *)(void *)bytes = value;
}

static u16 read_grid_word(const u8 *grid, s32 index)
{
    u32 address;

    address = (u32)(unsigned long)grid + ((u32)index << 1);
    return *(const u16 *)(unsigned long)address;
}

static u8 *frame_cursor(void)
{
    return (u8 *)(unsigned long)g_frame_global_73484;
}

static u8 *load_live_frame(void)
{
    return *(u8 * volatile *)(void *)&g_dispatch_frame_current_71ec4;
}

static void link_to_live_frame_list(u8 *record, int list_index)
{
    u8 *frame;
    u32 previous;
    u32 offset;

    offset = 0x88UL + (u32)list_index * 4UL;
    frame = load_live_frame();
    previous = *(u32 *)(void *)(frame + offset);
    write_u32(record, previous);

    /* Native reloads 0x71ec4 for the list update. */
    frame = load_live_frame();
    write_u32(frame + offset, (u32)(unsigned long)record);
}

static void emit_tile_run(u16 start_word, s32 run_width, s32 screen_x,
                          s32 screen_y, u32 packet_color, u8 **cursor_io)
{
    u8 *record;
    u32 tile_bits;
    u32 color_class;
    u32 upper_byte;
    u32 list_index;
    u32 u0;
    u32 v0;
    u32 u1;
    u32 v1;
    u32 kind;
    u16 clut_word;

    record = *cursor_io;
    tile_bits = (u32)(s32)(short)start_word;
    color_class = tile_bits & 3UL;
    upper_byte = (tile_bits >> 8) & 0xffUL;
    list_index = upper_byte & 1UL;
    u0 = tile_bits & 0xf8UL;
    v0 = upper_byte & 0xf8UL;
    kind = color_class;

    write_u32(record + 4, packet_color);
    /* Native reads this WORD for each emitted segment, even though the value
     * is copied into the packet header before the tile word is decoded. */
    clut_word = *(volatile unsigned short *)&g_tile_texture_word_73480;
    write_u16(record + 0x0e, clut_word);

    link_to_live_frame_list(record, (int)list_index);

    if (kind == 0UL) {
        record[7] = 0x64U;
        write_u16(record + 8, (u16)screen_x);
        write_u16(record + 0x0a, (u16)screen_y);
        record[0x0c] = (u8)u0;
        record[0x0d] = (u8)v0;
        write_u16(record + 0x10, (u16)run_width);
        write_u16(record + 0x12, 0x10U);
        *cursor_io = record + 0x14;
        return;
    }

    write_u16(record + 8, (u16)screen_x);
    write_u16(record + 0x0a, (u16)screen_y);
    write_u16(record + 0x10, (u16)(screen_x + run_width));
    write_u16(record + 0x12, (u16)screen_y);
    write_u16(record + 0x16, (u16)(list_index ? 0x9eU : 0x9cU));
    write_u16(record + 0x18, (u16)screen_x);
    write_u16(record + 0x1a, (u16)(screen_y + 0x10));
    write_u16(record + 0x20, (u16)(screen_x + run_width));
    write_u16(record + 0x22, (u16)(screen_y + 0x10));

    record[0x0c] = (u8)u0;
    record[0x0d] = (u8)v0;
    record[0x14] = (u8)(u0 + ((u32)run_width >> (kind & 1UL)) - 1UL);
    record[0x15] = (u8)v0;
    record[0x1c] = (u8)u0;
    record[0x1d] = (u8)(v0 - ((kind & 2UL) ? 8UL : 0UL) + 0x0fUL);
    record[0x24] = record[0x14];
    record[0x25] = record[0x1d];
    *cursor_io = record + 0x28;
}

void __cdecl eng1_tile_draw_2ff0(const u32 *coordinates)
{
    u8 *header;
    u8 *grid;
    u8 *cursor;
    u32 x_bits;
    u32 y_bits;
    u32 packet_color;
    s32 width;
    s32 height;
    s32 x_tile;
    s32 y_tile;
    s32 x_fraction;
    s32 y_fraction;
    s32 x_column;
    s32 row_index;
    s32 row_base;
    s32 screen_y;
    s32 screen_y_limit;

    packet_color = (g_frame_global_70cf8 & 0x00ffffffUL) | 0x2c000000UL;
    header = g_header_pointer_70d44;
    cursor = frame_cursor();
    width = read_s16(header);
    height = read_s16(header + 2);
    grid = header + 4;

    x_bits = coordinates[0];
    y_bits = coordinates[1];
    x_tile = arithmetic_shift_right_4(x_bits);
    y_tile = arithmetic_shift_right_4(y_bits);
    x_fraction = (s32)(x_bits & 0x0fUL);
    y_fraction = (s32)(y_bits & 0x0fUL);

    /* Keep the native signed-divide operation. On the pinned VC5 x86 build a
     * zero width follows the processor divide-error path, checked separately. */
    x_column = x_tile % width;
    screen_y = -y_fraction;
    screen_y_limit = 0x110 - y_fraction;
    row_index = y_tile;
    row_base = multiply_wrap32(width, row_index);

    /* The native comparison is retained: for a four-bit y fraction its
     * -fraction >= 0x110-fraction branch is unreachable. */
    if (screen_y >= screen_y_limit)
        goto finish;

    while (screen_y < screen_y_limit) {
        if (row_index >= 0 && row_index < height) {
            s32 sample_count;
            s32 column;

            sample_count = 0;
            column = x_column;
            while (sample_count <= 0x14) {
                u16 start_word;
                s32 run_width;
                s32 segment_x;
                int segment_complete;

                start_word = read_grid_word(grid,
                    add_wrap32(row_base, column));
                segment_x = sample_count * 0x10 - x_fraction;
                run_width = 0;
                segment_complete = 0;

                while (!segment_complete) {
                    u16 next_word;

                    ++column;
                    if (!(column < width))
                        column = 0;
                    run_width += 0x10;
                    ++sample_count;

                    /* The native code reads this following cell before the
                     * sample_count > 20 termination test. */
                    next_word = read_grid_word(grid,
                        add_wrap32(row_base, column));
                    if (sample_count > 0x14 ||
                        add_wrap32((s32)(short)start_word, run_width) !=
                            (s32)(short)next_word) {
                        emit_tile_run(start_word, run_width, segment_x,
                                      screen_y, packet_color, &cursor);
                        segment_complete = 1;
                    }
                }
            }
        }

        row_index = add_wrap32(row_index, 1);
        row_base = add_wrap32(row_base, width);
        screen_y += 0x10;
    }

finish:
    {
        u8 *frame;
        u32 old_head;

        frame = load_live_frame();
        old_head = *(u32 *)(void *)(frame + 0x88);
        write_u32(frame + 0x70, old_head);

        frame = load_live_frame();
        write_u32(frame + 0x88, (u32)(unsigned long)(frame + 0x70));

        frame = load_live_frame();
        old_head = *(u32 *)(void *)(frame + 0x8c);
        write_u32(frame + 0x7c, old_head);

        frame = load_live_frame();
        write_u32(frame + 0x8c, (u32)(unsigned long)(frame + 0x7c));
        g_frame_global_73484 = (u32)(unsigned long)cursor;
    }
}
