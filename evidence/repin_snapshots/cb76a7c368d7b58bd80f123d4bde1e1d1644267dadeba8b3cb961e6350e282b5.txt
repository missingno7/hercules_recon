/* Ordinary-C behavioral candidates for the ENG1 primitive helpers at
 * 2ED10, 2F120, and 2F340.  Storage and host callback bodies are provided by
 * the measured resource, mesh-storage, and host-wrapper units. */
#include "mesh_visit.h"

extern u8 *g_header_pointer_70d50;
extern long g_mesh_cached_column_6ffb8;
extern u32 g_mesh_colors_6f4e8[512];

extern void __cdecl dispatch_engine_2c690(const void *vector);
extern void __cdecl dispatch_engine_2c6e0(void);
extern void __cdecl dispatch_engine_2c760(void *projected_xy);
extern void __cdecl dispatch_engine_2c7b0(void *depth_word);

static long emit_b_add(long a, long b)
{ return (long)((u32)a + (u32)b); }

static long emit_b_sub(long a, long b)
{ return (long)((u32)a - (u32)b); }

static long emit_b_mul(long a, long b)
{ return (long)((u32)a * (u32)b); }

static long emit_b_s8(u32 value, int shift)
{
    long result;
    result = (long)((value >> shift) & 0xffUL);
    if (result & 0x80L) result -= 0x100L;
    return result;
}

static long emit_b_s16(u32 value, int shift)
{
    long result;
    result = (long)((value >> shift) & 0xffffUL);
    if (result & 0x8000L) result -= 0x10000L;
    return result;
}

static short emit_b_short(long value)
{ return (short)((u32)value & 0xffffUL); }

static long emit_b_mul_shift(long a, long b, int shift)
{ return emit_b_mul(a, b) >> shift; }

/* The non-winding 2ED10 path uses CDQ/AND/ADD/SAR to divide its two first
 * color products with truncation toward zero. */
static long emit_b_mul_shift_zero(long a, long b, int shift)
{
    long product;
    product = emit_b_mul(a, b);
    if (product < 0) product = emit_b_add(product, (1L << shift) - 1L);
    return product >> shift;
}

static long emit_b_color_index(u32 colors, long x, long y,
                               int winding, int zero_rounding)
{
    long c0, c1, c2, c3, top, bottom;
    long (*mul_shift)(long, long, int);

    c0 = (long)(colors & 0xffUL);
    c1 = (long)((colors >> 8) & 0xffUL);
    c2 = (long)((colors >> 16) & 0xffUL);
    c3 = (long)((colors >> 24) & 0xffUL);
    mul_shift = emit_b_mul_shift;
    if (zero_rounding) mul_shift = emit_b_mul_shift_zero;

    if (winding) {
        top = emit_b_add(c1, emit_b_mul_shift(x,
                            emit_b_sub(c0, c1), 6));
        bottom = emit_b_add(c3, emit_b_mul_shift(x,
                               emit_b_sub(c2, c3), 6));
    } else {
        top = emit_b_add(c0, mul_shift(x, emit_b_sub(c1, c0), 6));
        bottom = emit_b_add(c2, mul_shift(x, emit_b_sub(c3, c2), 6));
    }
    return emit_b_add(top, emit_b_mul_shift(
               emit_b_sub(bottom, top), 0x20L - y, 6)) >> 2;
}

static long emit_b_sine(short *sine, long angle)
{ return (long)sine[(u32)angle & 0xfffUL]; }

static void emit_b_project(const void *vector, void *projected)
{
    dispatch_engine_2c690(vector);
    dispatch_engine_2c6e0();
    dispatch_engine_2c760(projected);
}

/* ENG1:2ED10.  The fourth argument is the actual preparation selector from
 * the caller.  Its nonzero path adds visit-local row/column/layer origins. */
void __cdecl dispatch_local_2ed10(const u32 *stream,
                                  Eng1MeshVisit *visit,
                                  int count,
                                  int prepared)
{
    u8 *vector;
    u32 packed, next, colors, flags;
    u32 remaining;
    long row, column, layer_y, color_offset, color_index;
    long x, y, z;

    vector = (u8 *)g_engine_interface.opaque_010[0] + 0x40;
    if (prepared) {
        row = visit->row_local_048;
        column = visit->column_local_04c;
        layer_y = visit->layer_y_038;
    } else {
        row = 0;
        column = 0;
        layer_y = 0;
    }

    flags = (u32)visit->flags_02c;
    colors = *(u32 *)(g_header_pointer_70d50 + ((flags >> 4) * 4));
    color_offset = (long)((flags & 7UL) << 6);
    packed = *stream++;
    remaining = (u32)count;
    do {
        x = emit_b_s8(packed, 0);
        y = emit_b_s16(packed, 8);
        z = emit_b_s8(packed, 24);
        if (visit->winding_024) {
            x = emit_b_add(row, x);
        } else {
            x = emit_b_sub(emit_b_add(row, 0x40L), x);
        }
        y = emit_b_add(column, y);
        z = emit_b_add(layer_y, z);

        *(short *)(vector + 0) = emit_b_short(x);
        *(short *)(vector + 2) = emit_b_short(y);
        *(short *)(vector + 4) = emit_b_short(z);
        dispatch_engine_2c690(vector);
        dispatch_engine_2c6e0();

        color_index = emit_b_color_index(colors,
            emit_b_s8(packed, 0), emit_b_s16(packed, 8),
            visit->winding_024 != 0,
            visit->winding_024 == 0);
        *(u32 *)(vector + 4) = g_mesh_colors_6f4e8[
            color_offset + color_index];

        dispatch_engine_2c760(vector);
        /* 2ED10 reads the next stream DWORD after its projected callback. */
        next = *stream++;
        vector += 8;
        packed = next;
        --remaining;
    } while (remaining != 0);
}

/* ENG1:2F120.  The native callback sequence projects each source point to
 * the shared staging vector, then sends the terminal depth through 2C7B0. */
void __cdecl dispatch_local_2f120(const u32 *stream,
                                  Eng1MeshVisit *visit,
                                  int count)
{
    u8 *stage;
    short *sine;
    u32 packed, next, remaining;
    long column_delta, angle_origin, radius_origin, angle, depth;
    long x, y, z, product;
    short vector[4];

    stage = (u8 *)g_engine_interface.opaque_010[0] + 0x40;
    sine = (short *)g_engine_interface.opaque_010[1];
    column_delta = emit_b_sub(visit->column_01c,
                              g_mesh_cached_column_6ffb8);
    angle_origin = emit_b_sub(visit->anchor_row_050,
                              visit->row_018);
    angle_origin = emit_b_sub(emit_b_mul(
                              emit_b_sub(angle_origin, 0x10L), 0x40L),
                              visit->cached_044);
    radius_origin = emit_b_sub(0x28cL, visit->layer_y_038);
    packed = *stream++;
    remaining = (u32)count;
    do {
        angle = emit_b_sub(emit_b_mul(0x40L - emit_b_s8(packed, 0), 2L),
                           angle_origin);
        angle = (long)(short)angle;
        depth = emit_b_sub(emit_b_s8(packed, 24), radius_origin);
        depth = (long)(short)depth;
        x = emit_b_mul_shift(emit_b_sine(sine, angle), depth, 12);
        y = emit_b_add(emit_b_s16(packed, 8),
                       emit_b_add(emit_b_mul(column_delta, 0x20L), 0x20L));
        z = emit_b_mul_shift(emit_b_sine(sine, angle - 0x400L),
                             depth, 12);
        vector[0] = emit_b_short(x);
        vector[1] = emit_b_short(y);
        vector[2] = emit_b_short(z);
        dispatch_engine_2c690(vector);
        dispatch_engine_2c6e0();

        /* 2F120 reads the next packed word before its current 2C760 call. */
        next = *stream++;
        dispatch_engine_2c760(stage);
        packed = next;
        angle_origin = emit_b_add(angle_origin, 8L);
        --remaining;
    } while (remaining != 0);

    /* Native terminal sample closes the cylinder at its fixed end angle. */
    angle = emit_b_sub(0x40L, radius_origin) & 0xfffL;
    product = emit_b_mul(emit_b_sine(sine, angle), radius_origin);
    x = emit_b_sub(0, product) >> 12;
    angle = emit_b_sub(-0x3c0L, radius_origin) & 0xfffL;
    product = emit_b_mul(emit_b_sine(sine, angle), radius_origin);
    z = emit_b_sub(0, product) >> 12;
    y = emit_b_add(emit_b_mul(column_delta, 0x20L), 0x20L);
    vector[0] = emit_b_short(x);
    vector[1] = emit_b_short(y);
    vector[2] = emit_b_short(z);
    dispatch_engine_2c690(vector);
    dispatch_engine_2c6e0();
    dispatch_engine_2c7b0(&visit->depth_010);
}

/* ENG1:2F340.  Variant 1 and its alternate use opposite radial origins;
 * winding changes the angular orientation and color-corner interpolation. */
void __cdecl dispatch_local_2f340(const u32 *stream,
                                  Eng1MeshVisit *visit,
                                  int count)
{
    u8 *stage;
    short *sine;
    u32 packed, next, colors, flags, remaining;
    long angle_origin, radius_origin, angle, depth;
    long color_offset, color_index, x, y, z;
    short vector[4];

    stage = (u8 *)g_engine_interface.opaque_010[0] + 0x40;
    sine = (short *)g_engine_interface.opaque_010[1];
    flags = (u32)visit->flags_02c;
    colors = *(u32 *)(g_header_pointer_70d50 + ((flags >> 4) * 4));
    color_offset = (long)((flags & 7UL) << 6);
    if (visit->variant_025 == 1) {
        angle_origin = emit_b_sub(visit->anchor_row_050,
                                  visit->row_018);
        angle_origin = emit_b_sub(emit_b_mul(
                                  emit_b_sub(angle_origin, 0x10L), 0x40L),
                                  visit->cached_044);
        radius_origin = emit_b_sub(0x28cL, visit->layer_y_038);
    } else {
        angle_origin = emit_b_sub(visit->row_018,
                                  visit->anchor_row_050);
        angle_origin = emit_b_sub(emit_b_mul(
                                  emit_b_add(angle_origin, 0x11L), 0x40L),
                                  visit->cached_044);
        radius_origin = emit_b_sub(-0x28cL, visit->layer_y_038);
    }

    packed = *stream++;
    remaining = (u32)count;
    do {
        x = emit_b_s8(packed, 0);
        y = emit_b_s16(packed, 8);
        z = emit_b_s8(packed, 24);
        if ((visit->variant_025 == 1 && !visit->winding_024) ||
            (visit->variant_025 != 1 && visit->winding_024))
            angle = emit_b_sub(emit_b_sub(0x40L, angle_origin), x);
        else
            angle = emit_b_sub(x, angle_origin);
        angle = (long)(short)angle;
        if (visit->variant_025 == 1)
            depth = emit_b_sub(z, radius_origin);
        else
            depth = emit_b_sub(radius_origin, z);
        depth = (long)(short)depth;

        vector[0] = emit_b_short(emit_b_mul_shift(
            emit_b_sine(sine, angle), depth, 12));
        vector[1] = emit_b_short(y);
        vector[2] = emit_b_short(emit_b_mul_shift(
            emit_b_sine(sine, angle - 0x400L), depth, 12));
        dispatch_engine_2c690(vector);
        dispatch_engine_2c6e0();

        color_index = emit_b_color_index(colors, x, y,
                                         visit->winding_024 != 0, 0);
        *(u32 *)(stage + 4) = g_mesh_colors_6f4e8[
            color_offset + color_index];
        dispatch_engine_2c760(stage);
        next = *stream++;
        stage += 8;
        packed = next;
        --remaining;
    } while (remaining != 0);
}
