/* Behavioral C model of the measured ENG1 command writers at 0x33420
 * and 0x33620. It uses the existing frame cursor, host interface, state and
 * color-prefix owners; it does not claim original source identity or exact
 * function bytes. */
#include "../../../calibration/engine_interface.h"
#include "../../../calibration/frame_lifecycle_bridges.h"
#include "cold_start_frame_data.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef long s32;
typedef u32 (__cdecl *PackCallback)(u32, u32);
typedef void (__cdecl *PacketCallback)(void *);
typedef void (__cdecl *FlagCallback)(void *, u32);

typedef char draw_commands_require_32bit_ulong[(sizeof(u32) == 4) ? 1 : -1];

extern u8 g_cold_start_state_bytes_70820[0x1c];
extern u32 g_frame_global_73484;
extern u32 g_frame_global_73488;
extern u8 *g_dispatch_frame_current_71ec4;

static u16 read_u16(const u8 *bytes)
{
    return *(const u16 *)(const void *)bytes;
}

static u32 read_u32(const u8 *bytes)
{
    return *(const u32 *)(const void *)bytes;
}

static void write_u16(u8 *bytes, u16 value)
{
    *(u16 *)(void *)bytes = value;
}

static void write_u32(u8 *bytes, u32 value)
{
    *(u32 *)(void *)bytes = value;
}

static u32 call_pack_callback(u32 first, u32 second)
{
    PackCallback callback;

    callback = *(PackCallback *)(void *)((u8 *)&g_engine_interface + 0x1e8);
    return callback(first, second);
}

static void call_packet_initializer(void *record)
{
    PacketCallback callback;

    callback = *(PacketCallback *)(void *)((u8 *)&g_engine_interface + 0x2d0);
    callback(record);
}

static void call_record_flag(void *record, u32 enabled)
{
    FlagCallback callback;

    callback = *(FlagCallback *)(void *)((u8 *)&g_engine_interface + 0x2e8);
    callback(record, enabled);
}

static void link_record(u8 *record)
{
    u8 *frame;
    u32 previous;

    frame = g_dispatch_frame_current_71ec4;
    previous = read_u32(frame + 0x1374);
    write_u32(record, previous);
    write_u32(frame + 0x1374, (u32)(unsigned long)record);
}

void __cdecl dispatch_local_33620(s32 arg1, s32 arg2, s32 arg3, s32 arg4,
                                  s32 arg5, s32 arg6, s32 arg7)
{
    u8 *record;
    u32 encoded;

    record = (u8 *)(unsigned long)g_frame_global_73484;
    encoded = call_pack_callback(0x160UL, (u32)arg7 + 0x20UL);

    write_u16(record + 0x08, (u16)arg1);
    write_u16(record + 0x0a, (u16)arg2);
    record[0x0c] = (u8)arg3;
    record[0x0d] = (u8)arg4;
    write_u16(record + 0x0e, (u16)encoded);
    write_u32(record + 0x04, g_frame_global_73488);
    write_u16(record + 0x10, (u16)arg5);
    write_u16(record + 0x12, (u16)arg6);
    record[0x07] = 0x64U;
    if (arg7 == 4)
        record[0x07] = 0x66U;

    link_record(record);
    g_frame_global_73484 = (u32)(unsigned long)(record + 0x14);

    if (arg7 == 4) {
        u8 *extra;

        extra = record + 0x14;
        dispatch_engine_2c980(extra, 0UL, 0UL, 0x180UL, 0UL);
        /* Native 0x33620 reloads the global cursor after this callback. */
        extra = (u8 *)(unsigned long)g_frame_global_73484;
        link_record(extra);
        g_frame_global_73484 = (u32)(unsigned long)(extra + 0x0c);
    }
}

static u32 pack_coordinate(u32 low, u32 high)
{
    return (low & 0xffffUL) + (high << 16);
}

static void append_blend_record(u8 *record, u32 color, u32 x0, u32 y0,
                                u32 x1, u32 y1, u16 uv00, u16 uv10,
                                u16 flags, u16 uv01, u16 uv11,
                                u32 encoded)
{
    write_u32(record + 0x04, color);
    write_u32(record + 0x08, x0);
    write_u16(record + 0x0c, uv00);
    write_u16(record + 0x0e, (u16)encoded);
    write_u32(record + 0x10, y0);
    write_u16(record + 0x14, uv10);
    write_u16(record + 0x16, flags);
    write_u32(record + 0x18, x1);
    write_u16(record + 0x1c, uv01);
    write_u32(record + 0x20, y1);
    write_u16(record + 0x24, uv11);

    call_packet_initializer(record);
    call_record_flag(record, 1UL);
}

void __cdecl dispatch_local_33420(s32 arg1, s32 arg2, s32 arg3, s32 arg4,
                                  s32 arg5, s32 arg6, s32 arg7)
{
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    u32 u;
    u32 v;
    u32 x0;
    u32 y0;
    u32 x1;
    u32 y1;
    u32 sum;
    u32 uv_x;
    u32 uv_y;
    u32 encoded;
    u32 index;
    u8 *cursor;
    u32 color;
    u16 flags;
    u16 blend_word;

    if (g_cold_start_state_bytes_70820[0x14] != 2U) {
        dispatch_local_33620(arg1, arg2, (u8)arg3, (u8)arg4,
                             arg5, arg6, arg7);
        return;
    }
    blend_word = read_u16(g_frame_color_state_prefix_6ffc8 + 0x08);
    if (blend_word == 0U)
        return;
    if (blend_word >= 0x80U) {
        dispatch_local_33620(arg1, arg2, (u8)arg3, (u8)arg4,
                             arg5, arg6, arg7);
        return;
    }

    x = (u32)arg1;
    y = (u32)arg2;
    width = (u32)arg5;
    height = (u32)arg6;
    u = (u8)arg3;
    v = (u8)arg4;

    x0 = pack_coordinate(x, y);
    y0 = pack_coordinate((x + width) & 0xffffUL, y);
    x1 = pack_coordinate(x, y + height);
    y1 = pack_coordinate((x + width) & 0xffffUL, y + height);

    /* The instruction sequence computes these in 32-bit registers; the
     * stored WORD fields are their low words. */
    sum = ((width & 0xffUL) + u) & 0xffUL;
    uv_x = u + (v << 8);
    uv_y = u + ((height + v) << 8);
    /* The native helper captured ESI from the cursor global before its first
     * host callback and retains that record base through the loop. */
    cursor = (u8 *)(unsigned long)g_frame_global_73484;
    encoded = call_pack_callback(0x160UL, arg7 + 0x20UL);

    for (index = 0; index < 2UL; ++index) {
        color = read_u32(g_frame_color_state_prefix_6ffc8 + index * 4UL);
        /* Native 0x33420 reuses the formal arg6 stack slot for 0x180 before
         * its callback loop.  The later [ESP+0x34] load therefore observes
         * that local value, not the incoming height argument. */
        flags = (u16)(0x180UL | ((index + 1UL) << 5));
        append_blend_record(cursor, color, x0, y0, x1, y1,
                            (u16)uv_x, (u16)(sum + (v << 8)), flags,
                            (u16)uv_y, (u16)(((height + v) << 8) + sum),
                            encoded);
        link_record(cursor);
        cursor += 0x28;
        encoded = call_pack_callback(0x160UL, 0x26UL);
    }

    g_frame_global_73484 = (u32)(unsigned long)cursor;
}
