/* Private behavioral reconstruction of ENG1 frame preparation/upload helpers.
 * This is an address-backed source hypothesis, not historical TU identity or
 * FUNCTION_MATCH evidence. PC call and field facts are recorded in the
 * adjacent frame_commands handoff. */
#include "../../../calibration/engine_dispatch_externs.h"

extern void *g_frame_owner_70c80;
extern void *g_frame_owner_72870;
extern void *g_frame_owner_7348c;
extern u32 g_frame_global_73484;
extern u8 g_frame_global_73498;
extern u8 g_frame_config_5df08[];

extern u32 g_frame_upload_pending_6ee7c;
extern u16 g_frame_upload_coordinates_734a0[];
extern u8 *g_header_pointer_70d38;
extern u8 g_root_count_70ed1;

/* This wrapper is a separate ENG1 callable (RVA 0x2c7f0): it forwards the
 * rectangle/data pair to copied interface slot +0x204. Its provider remains
 * an explicit dependency of this candidate. */
extern void __cdecl dispatch_engine_2c7f0(const void *, const void *);

#include "data_views.h"

void __cdecl dispatch_local_05600(void)
{
    u8 *frame;
    u8 *base;
    int level;
    u16 primitive_offset;

    frame = g_dispatch_frame_current_71ec4;
    g_engine_interface.frame_chain_write_144(frame + 0x88, 0x500UL);

    frame = g_dispatch_frame_current_71ec4;
    base = g_dispatch_frame_base_71ec8;
    if (frame == base) {
        level = (int)(s8)g_engine_interface.context_004->unknown_000[0];
        primitive_offset = *(u16 *)(g_frame_config_5df08 + level * 28 + 6);
        g_frame_global_73484 = (u32)g_frame_owner_70c80 +
                               (u32)primitive_offset * 40UL;
    } else {
        g_frame_global_73484 = (u32)g_frame_owner_70c80;
    }
}

/* The fixed eight-byte payload at ENG1 RVA 0x4d810 is four WORDs:
 * x=0x0300, y=0x00e0, width=0x0100, height=0x0020. The host callback copies
 * it synchronously; this private object preserves those observed values. */
static const u16 g_palette_upload_rectangle[4] = {
    0x0300, 0x00e0, 0x0100, 0x0020
};

void __cdecl dispatch_local_058b0(void)
{
    if (g_frame_global_73498 != 0) {
        dispatch_engine_2c7f0(g_palette_upload_rectangle,
                              g_frame_owner_7348c);
        g_frame_global_73498 = 0;
    }
}

void __cdecl dispatch_local_02cb0(void)
{
    u8 *entry;
    int index;
    int signed_count;

    signed_count = (int)(s8)g_root_count_70ed1;
    entry = g_header_pointer_70d38;
    index = 0;
    if (signed_count <= 0)
        return;

    while (index < (int)(s8)g_root_count_70ed1) {
        u8 *descriptor;
        u16 frame_count_word;
        u16 period_word;
        int frame_count;
        int period;

        descriptor = entry + *(u32 *)entry;
        entry += 4;
        frame_count_word = *(u16 *)descriptor;
        period_word = *(u16 *)(descriptor + 2);
        frame_count = (int)frame_count_word;
        period = (int)period_word;

        if (period != 0) {
            int tick;
            int width;
            int height;
            int frame_number;
            int byte_offset;
            u16 rectangle[4];
            const u8 *pixels;

            tick = (int)g_engine_interface.context_004->tick_038;
            if (tick % period == 0) {
                rectangle[0] = *(u16 *)(descriptor + 4);
                rectangle[1] = *(u16 *)(descriptor + 6);
                rectangle[2] = *(u16 *)(descriptor + 8);
                rectangle[3] = *(u16 *)(descriptor + 10);

                tick = (int)g_engine_interface.context_004->tick_038;
                width = (int)(short)rectangle[2];
                height = (int)(short)rectangle[3];
                frame_number = (tick / period) % frame_count;
                byte_offset = frame_number * width * height * 2;
                pixels = descriptor + 12 + byte_offset;
                dispatch_engine_2c7f0(rectangle, pixels);
            }
        }

        ++index;
    }
}

void __cdecl dispatch_local_050d0(void)
{
    u32 pending;
    u8 *pixels;
    u16 *coordinate_cursor;
    u16 coordinate;

    pending = g_frame_upload_pending_6ee7c;
    if (pending == 0)
        return;

    pixels = (u8 *)g_frame_owner_72870;
    coordinate_cursor = g_frame_upload_coordinates_734a0 + 1;
    coordinate = g_frame_upload_coordinates_734a0[0];

    for (;;) {
        u16 rectangle[4];
        u32 rectangle_height;
        int previous_x;
        int previous_y;

        previous_x = (int)(coordinate & 0x0f);
        previous_y = (int)(coordinate >> 4);
        rectangle[0] = (u16)((previous_y + 0x30) << 4);
        rectangle[1] = (u16)(previous_x << 5);
        rectangle[2] = 0x10;
        rectangle_height = 0x20;

        pending = g_frame_upload_pending_6ee7c - 1UL;
        g_frame_upload_pending_6ee7c = pending;

        while (pending != 0) {
            u16 next_coordinate;
            int next_x;
            int next_y;

            next_coordinate = *coordinate_cursor;
            next_x = (int)(next_coordinate & 0x0f);
            next_y = (int)(next_coordinate >> 4);
            if (next_x != previous_x + 1 || next_y != previous_y) {
                ++coordinate_cursor;
                coordinate = next_coordinate;
                break;
            }

            ++coordinate_cursor;
            coordinate = next_coordinate;
            previous_x = next_x;
            previous_y = next_y;
            rectangle_height += 0x20;
            pending = g_frame_upload_pending_6ee7c - 1UL;
            g_frame_upload_pending_6ee7c = pending;
        }

        rectangle[3] = (u16)rectangle_height;
        dispatch_engine_2c7f0(rectangle, pixels);
        pixels += rectangle_height * 32UL;

        if (g_frame_upload_pending_6ee7c == 0)
            return;
    }
}
