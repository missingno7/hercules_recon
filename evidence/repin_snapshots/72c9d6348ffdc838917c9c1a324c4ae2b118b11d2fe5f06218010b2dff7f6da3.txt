/* PC ENG1 primitive-to-host-vector emitters, private behavioral candidates.
 * The native caller ABI is (packed stream, measured visit prefix, count).
 * This file owns no storage and supplies no host callback replacements. */
#include "mesh_visit.h"

extern void __cdecl dispatch_engine_2c690(const void *vector);
extern void __cdecl dispatch_engine_2c6e0(void);
extern void __cdecl dispatch_engine_2c760(void *projected_xy);

static long emit_add32(long a, long b)
{ return (long)((u32)a + (u32)b); }

static long emit_sub32(long a, long b)
{ return (long)((u32)a - (u32)b); }

static long emit_shl32(long a, int n)
{ return (long)((u32)a << n); }

static long emit_byte_signed(u32 value, int shift)
{
    long result;
    result = (long)((value >> shift) & 0xffUL);
    if (result & 0x80L) result -= 0x100L;
    return result;
}

static long emit_word_signed(u32 value, int shift)
{
    long result;
    result = (long)((value >> shift) & 0xffffUL);
    if (result & 0x8000L) result -= 0x10000L;
    return result;
}

static short emit_short(long value)
{ return (short)((u32)value & 0xffffUL); }

static long emit_sine(short *table, long angle)
{ return (long)table[(u32)angle & 0xfffUL]; }

/* ENG1:2e600. Coordinates are signed X byte, signed Y word, signed Z byte.
 * The native routine prefetches the next stream DWORD before the XY callback;
 * this ordering is observable when a host callback aliases or edits the stream. */
void __cdecl dispatch_local_2e600(const u32 *stream,
                                 Eng1MeshVisit *visit,
                                 int count)
{
    u8 *vector;
    u32 packed;
    u32 remaining;
    long x, y, z;

    vector = (u8 *)g_engine_interface.opaque_010[0] + 0x40;
    packed = *stream++;
    remaining = (u32)count;
    do {
        x = emit_byte_signed(packed, 0);
        if (!visit->winding_024)
            x = emit_sub32(0x40L, x);
        y = emit_word_signed(packed, 8);
        z = emit_byte_signed(packed, 24);

        *(short *)(vector + 0) = emit_short(x);
        *(short *)(vector + 2) = emit_short(y);
        *(short *)(vector + 4) = emit_short(z);
        dispatch_engine_2c690(vector);
        dispatch_engine_2c6e0();

        packed = *stream++;
        *(u32 *)(vector + 4) = 0;
        dispatch_engine_2c760(vector);
        vector += 8;
        --remaining;
    } while (remaining != 0);
}

/* ENG1:2e6c0. Add the caller-supplied local origin, with the native alternate
 * winding mapping X to 64-X. The output scratch records advance by eight. */
void __cdecl dispatch_local_2e6c0(const u32 *stream,
                                 Eng1MeshVisit *visit,
                                 int count)
{
    u8 *vector;
    u32 packed;
    u32 remaining;
    long x, y, z;

    vector = (u8 *)g_engine_interface.opaque_010[0] + 0x40;
    packed = *stream++;
    remaining = (u32)count;
    do {
        x = emit_add32(emit_byte_signed(packed, 0), visit->row_local_048);
        if (!visit->winding_024)
            x = emit_sub32(emit_add32(0x40L, visit->row_local_048),
                           emit_byte_signed(packed, 0));
        y = emit_add32(emit_word_signed(packed, 8),
                       visit->column_local_04c);
        z = emit_add32(emit_byte_signed(packed, 24), visit->layer_y_038);

        *(short *)(vector + 0) = emit_short(x);
        *(short *)(vector + 2) = emit_short(y);
        *(short *)(vector + 4) = emit_short(z);
        dispatch_engine_2c690(vector);
        dispatch_engine_2c6e0();

        packed = *stream++;
        dispatch_engine_2c760(vector);
        vector += 8;
        --remaining;
    } while (remaining != 0);
}

/* ENG1:2ea50. Rotate the packed X/Z vertex around the sine-table Y axis using
 * visit-relative fixed-point origins. All angle and Z terms pass through the
 * native signed-WORD truncation before the 12-bit table index/multiply. */
void __cdecl dispatch_local_2ea50(const u32 *stream,
                                 Eng1MeshVisit *visit,
                                 int count)
{
    u8 *vector;
    short *sine;
    u32 packed;
    u32 remaining;
    long angle_origin, z_origin;
    long angle, depth, y, x, z;
    long s, c;

    vector = (u8 *)g_engine_interface.opaque_010[0] + 0x40;
    sine = (short *)g_engine_interface.opaque_010[1];
    packed = *stream++;
    remaining = (u32)count;

    if (visit->variant_025 == 1) {
        angle_origin = emit_sub32(visit->anchor_row_050,
                                  visit->row_018);
        angle_origin = emit_shl32(
            emit_sub32(angle_origin, 0x10L), 6);
        angle_origin = emit_sub32(angle_origin, visit->cached_044);
        z_origin = emit_sub32(0x28cL, visit->layer_y_038);

        do {
            angle = emit_byte_signed(packed, 0);
            if (visit->winding_024)
                angle = emit_sub32(angle, angle_origin);
            else
                angle = emit_sub32(
                    emit_sub32(0x40L, angle), angle_origin);
            angle = (long)(short)angle;

            depth = emit_sub32(emit_byte_signed(packed, 24), z_origin);
            depth = (long)(short)depth;
            y = emit_word_signed(packed, 8);
            s = emit_sine(sine, angle);
            c = emit_sine(sine, emit_sub32(angle, 0x400L));
            x = (s * depth) >> 12;
            z = (c * depth) >> 12;

            *(short *)(vector + 0) = emit_short(x);
            *(short *)(vector + 2) = emit_short(y);
            *(short *)(vector + 4) = emit_short(z);
            dispatch_engine_2c690(vector);
            dispatch_engine_2c6e0();
            packed = *stream++;
            dispatch_engine_2c760(vector);
            vector += 8;
            --remaining;
        } while (remaining != 0);
    } else {
        angle_origin = emit_sub32(visit->row_018,
                                  visit->anchor_row_050);
        angle_origin = emit_shl32(
            emit_add32(angle_origin, 0x11L), 6);
        angle_origin = emit_sub32(angle_origin, visit->cached_044);
        z_origin = emit_sub32(-0x28cL, visit->layer_y_038);

        do {
            angle = emit_byte_signed(packed, 0);
            if (visit->winding_024)
                angle = emit_sub32(angle, angle_origin);
            else
                angle = emit_sub32(
                    emit_sub32(0x40L, angle), angle_origin);
            angle = (long)(short)angle;

            depth = emit_sub32(z_origin,
                               emit_byte_signed(packed, 24));
            depth = (long)(short)depth;
            y = emit_word_signed(packed, 8);
            s = emit_sine(sine, angle);
            c = emit_sine(sine, emit_sub32(angle, 0x400L));
            x = (s * depth) >> 12;
            z = (c * depth) >> 12;

            *(short *)(vector + 0) = emit_short(x);
            *(short *)(vector + 2) = emit_short(y);
            *(short *)(vector + 4) = emit_short(z);
            dispatch_engine_2c690(vector);
            dispatch_engine_2c6e0();
            packed = *stream++;
            dispatch_engine_2c760(vector);
            vector += 8;
            --remaining;
        } while (remaining != 0);
    }
}
