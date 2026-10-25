/* PC-derived ENG1 camera / host transform-setup bodies, private behavioral
 * hypothesis. No preferred addresses, storage definitions or binary dispatch. */
#include "../shared/engine_dispatch_externs.h"

extern u32 g_frame_global_734f4;
extern u32 g_frame_global_70ce8, g_frame_global_70cec;
extern u8 g_mode_table_value_70d00;
extern short g_element_width_70d14;
extern u8 g_group_count_70ece;
extern u8 *g_header_pointer_70d54, *g_header_pointer_70d58;
extern u8 *g_rebuilt_row_pointers_70dcc[];
extern u16 g_rebuilt_row_offsets_70e0c[];

/* All below are distinct observed cells or prefixes, with no storage here. */
extern long g_render_transform_70c60[8];
extern long g_camera_x_70ca0, g_camera_y_70ca4, g_camera_z_70ca8;
extern long g_camera_grid_x_70cac, g_camera_grid_y_70cb0;
extern long g_camera_grid_z_70cb4;
extern long g_camera_offset_x_70cb8, g_camera_offset_y_70cbc;
extern long g_camera_offset_z_70cc0;
extern long g_camera_prior_x_70cc4, g_camera_prior_y_70cc8;
extern long g_camera_angle_x_70cd0, g_camera_angle_y_70cd4;
extern long g_camera_center_x_70cdc, g_camera_center_y_70ce0;
extern long g_camera_focal_70ce4;
extern u8 g_camera_iterations_70cfd, g_camera_bounds_id_70cff;
extern short g_camera_adjust_x_70d02, g_camera_adjust_y_70d04;
extern short g_camera_adjust_z_70d06;
extern short g_camera_depth_low_70d0c, g_camera_depth_high_70d0e;
/* Only low16 flag bits participate; +12 is the independent selected-width
 * WORD. A native wide read does not justify overlapping DWORD storage. */
extern u16 g_camera_flags_70d10;
extern long g_camera_z_offset_70d18, g_camera_z_base_70d1c;
extern u16 g_camera_threshold_x_5e058[];
extern u16 g_camera_threshold_y_5e080[];
extern u16 g_camera_threshold_z_5e0a8[];

/* Physical caller ABI; source bounds helper uses the shared resource provider. */
extern int __cdecl dispatch_local_20ea0(void *, int, int, long);
extern void __cdecl dispatch_engine_2c9d0(u32, u32);
extern void __cdecl dispatch_engine_2c9f0(u32);

/* Ordinary unsigned intermediates specify the x86 low32 arithmetic. */
static long add32(long a, long b) { return (long)((u32)a + (u32)b); }
static long sub32(long a, long b) { return (long)((u32)a - (u32)b); }
static long mul32(long a, long b) { return (long)((u32)a * (u32)b); }
static long shift32(long a, int n) { return (long)((u32)a << n); }
static long neg32(long a) { return (long)(0UL - (u32)a); }
static long actor_long(const u8 *p, int off) { return *(const long *)(p + off); }
static long actor_coord(const u8 *p, int off)
{ return (long)(short)(actor_long(p, off) >> 16); }

static int camera_step(const u16 *table, long distance)
{
    u16 magnitude;
    int index;
    if (distance < 0) distance = neg32(distance);
    magnitude = (u16)distance;
    index = 1;
    if (table[0] < magnitude)
        /* Native pointer advances after every read, including the last one. */
        while (table[index++] < magnitude) {}
    return index - 1;
}

void __cdecl dispatch_local_2fb70(void)
{
    long x, delta, step, value, scale;
    int index, bounded;
    u32 flags;
    u8 *actor;
    /* 20ea0 writes seven WORDs for every nonzero mode, including 0x18. */
    short bounds[7];

    g_camera_prior_y_70cc8 = g_camera_y_70ca4;
    x = g_camera_x_70ca0;
    g_camera_prior_x_70cc4 = x;
    bounded = 0;
    if ((u8)g_camera_flags_70d10 & 0x80) {
        bounded = dispatch_local_20ea0(bounds,
            (int)(s8)g_camera_bounds_id_70cff, 0x18, x);
        x = g_camera_x_70ca0;
    }

    index = 0;
    if ((int)(s8)g_camera_iterations_70cfd > 0) {
        flags = g_camera_flags_70d10;
        actor = (u8 *)g_frame_global_734f4;
        do {
            flags = g_camera_flags_70d10;
            if (!(flags & 0x100)) {
                if ((u8)flags & 2) {
                    delta = sub32(actor_coord(actor, 0), x);
                    if (flags & 1) delta = shift32(delta, 20) >> 20;
                    delta = add32(delta, g_camera_center_x_70cdc);
                    delta = sub32(shift32(delta, 1), 0x140);
                    if (flags & 1) delta = shift32(delta, 19) >> 19;
                    value = (long)g_camera_adjust_x_70d02;
                    if (actor[0x94] == 0) value = neg32(value);
                    delta = sub32(delta, value);
                    step = camera_step(g_camera_threshold_x_5e058, delta);
                    if (flags & 1) {
                        x = delta > 0 ? add32(x, step) : sub32(x, step);
                        x = (long)(((u32)sub32(x, 0x80) & 0xfffUL) + 0x80UL);
                        g_camera_x_70ca0 = x;
                    } else if (step != 0) {
                        if (delta > 0) {
                            x = add32(x, step);
                            g_camera_x_70ca0 = x;
                            if (x <= (long)g_frame_global_70cec) goto camera_z;
                            x = (long)g_frame_global_70cec;
                        } else {
                            x = sub32(x, step);
                            g_camera_x_70ca0 = x;
                            if (x >= (long)g_frame_global_70ce8) goto camera_z;
                            x = (long)g_frame_global_70ce8;
                        }
                        g_camera_x_70ca0 = x;
                    }
                } else {
                    x = actor_coord(actor, 0);
                    g_camera_x_70ca0 = x;
                    if (x > (long)g_frame_global_70cec)
                        x = (long)g_frame_global_70cec;
                    else if (x < (long)g_frame_global_70ce8)
                        x = (long)g_frame_global_70ce8;
                    g_camera_x_70ca0 = x;
                }
            }
camera_z:
            if ((u8)g_camera_flags_70d10 & 8) {
                delta = sub32(actor_coord(actor, 8), g_camera_z_70ca8);
                delta = sub32(add32(delta, g_camera_z_offset_70d18),
                              g_camera_focal_70ce4);
                delta = add32(shift32(delta, 1), (long)g_camera_adjust_z_70d06);
                step = camera_step(g_camera_threshold_z_5e0a8, delta);
                value = g_camera_z_70ca8;
                if (step != 0)
                    value = delta > 0 ? add32(value, step) : sub32(value, step);
                g_camera_z_70ca8 = value;
                /* Native TEST step,(index>0), not step!=0 && index>0. */
                if (((u32)step & (u32)(index > 0)) != 0) goto next_iteration;
            } else {
                value = sub32(add32(actor_coord(actor, 8), g_camera_z_offset_70d18),
                              g_camera_focal_70ce4);
                g_camera_z_70ca8 = value;
            }

            flags = g_camera_flags_70d10;
            if (!(flags & 0x200)) {
                if ((u8)flags & 4) {
                    value = g_camera_y_70ca4;
                    delta = sub32(actor_coord(actor, 4), value);
                    delta = add32(delta, g_camera_center_y_70ce0);
                    delta = sub32(shift32(delta, 1), 0x168);
                    if ((u8)g_camera_flags_70d10 & 0x40) {
                        short depth;
                        depth = *(short *)(actor + 0xe);
                        if (depth < g_camera_depth_low_70d0c) {
                            scale = shift32(add32(sub32(g_camera_depth_low_70d0c, depth), 0x10), 4);
                            if (scale > 0x200) scale = 0x200;
                            delta = mul32(scale, delta) >> 8;
                        }
                        if (depth > g_camera_depth_high_70d0e) {
                            scale = shift32(add32(sub32(depth, g_camera_depth_high_70d0e), 0x10), 4);
                            if (scale > 0x200) scale = 0x200;
                            delta = mul32(scale, delta) >> 8;
                        }
                    }
                    delta = add32(delta, (long)g_camera_adjust_y_70d04);
                    step = camera_step(g_camera_threshold_y_5e080, delta);
                    if (step != 0) {
                        if (delta > 0) {
                            value = add32(value, step);
                            scale = sub32(shift32((long)g_element_width_70d14, 6), 0x40);
                            if (value > scale) value = scale;
                        } else {
                            value = sub32(value, step);
                            if (value < 0x7c) value = 0x7c;
                        }
                    }
                } else {
                    value = actor_coord(actor, 4);
                    if (!bounded) {
                        scale = sub32(shift32((long)g_element_width_70d14, 6), 0x40);
                        if (value > scale) value = scale;
                    }
                }
                if (bounded) {
                    if (value < (long)bounds[0]) value = (long)bounds[0];
                    else if (value > (long)bounds[2]) value = (long)bounds[2];
                }
                g_camera_y_70ca4 = value;
            }
next_iteration:
            ++index;
        } while (index < (int)(s8)g_camera_iterations_70cfd);
    }
    g_camera_grid_y_70cb0 = g_camera_y_70ca4 >> 6;
    g_camera_grid_x_70cac = x >> 6;
    g_camera_grid_z_70cb4 = g_camera_z_70ca8 >> 6;
}

void __cdecl dispatch_local_2f840(void)
{
    long actor_z, row, column, layer, row_start, packed, key;
    long angle, initial_angle, base_x, base_y, phase, offset;
    long sine, cosine, z_shift, x_shift, screen_x, screen_y;
    u8 *descriptor, *row_data;
    short *trig;
    short field;
    u32 flags;
    u8 *actor;

    actor = (u8 *)g_frame_global_734f4;
    actor_z = actor_long(actor, 8) >> 16;
    column = actor_long(actor, 4) >> 16;
    layer = add32((long)(short)actor_z, 0x40) >> 7;
    row = g_camera_x_70ca0 >> 6;
    column = (long)(short)column >> 6;
    if (layer < 0) layer = 0;
    else if (layer > (int)(s8)g_group_count_70ece - 1)
        layer = (int)(s8)g_group_count_70ece - 1;
    row_start = (long)(short)g_rebuilt_row_offsets_70e0c[layer + 16];
    row = sub32(row, row_start);
    if (row < 0) row = 0;
    else if (row > (long)(short)g_rebuilt_row_offsets_70e0c[layer] - 1)
        row = (long)(short)g_rebuilt_row_offsets_70e0c[layer] - 1;
    if (column < 0) column = 0;
    else if (column > (long)g_element_width_70d14 - 1)
        column = (long)g_element_width_70d14 - 1;
    key = *(long *)(g_rebuilt_row_pointers_70dcc[layer] +
        mul32(add32(mul32(row, (long)g_element_width_70d14), column), 4));
    row = add32(row, row_start);
    key = (key >> 11) & 0x1fff;
    row_data = g_header_pointer_70d54;
    descriptor = g_header_pointer_70d58 +
        shift32((long)*(u16 *)(row_data + key * 4), 4);
    packed = *(long *)(descriptor + 8);
    initial_angle = (long)(((u32)packed >> 24) << 4);
    angle = packed & 0xfff;
    offset = g_mode_table_value_70d00 ? 0x20 : 0;
    phase = g_camera_x_70ca0 & 0x3f;
    angle = sub32(angle, initial_angle);
    angle = shift32(angle, 20) >> 20;
    angle = add32(mul32(angle, phase) >> 6, offset);
    angle = add32(angle, initial_angle) & 0xfff;
    base_x = *(long *)descriptor;
    base_y = *(long *)(descriptor + 4);
    field = *(short *)(descriptor + 0xe);
    trig = (short *)g_engine_interface.opaque_010[1];
    if (!((u16)field & 0xc000)) {
        row = shift32(sub32(row, field & 0x3fff), 6);
        base_x = add32(base_x, mul32((long)trig[packed & 0xfff], row));
        base_y = add32(base_y, mul32((long)trig[sub32(packed & 0xfff, 0x400) & 0xfff], row));
    }
    sine = (long)trig[angle & 0xfff];
    cosine = (long)trig[sub32(angle, 0x400) & 0xfff];
    base_x = add32(base_x, mul32(sine, phase));
    base_y = add32(base_y, mul32(cosine, phase));
    actor_z = sub32(actor_z, g_camera_z_base_70d1c);
    initial_angle = add32(g_camera_angle_y_70cd4, 0x400);
    z_shift = mul32(sub32(0x1000, (long)trig[sub32(initial_angle, 0x400) & 0xfff]), actor_z) >> 6;
    x_shift = mul32((long)trig[initial_angle & 0xfff], actor_z) >> 6;
    flags = g_camera_flags_70d10;
    if ((u8)flags & 0x10) {
        actor = (u8 *)g_frame_global_734f4;
        row = sub32(g_camera_x_70ca0, actor_long(actor, 0) >> 16);
        z_shift = sub32(z_shift, mul32((long)trig[sub32(g_camera_angle_y_70cd4, 0x400) & 0xfff], row) >> 6);
        x_shift = sub32(x_shift, mul32(sub32(0x1000, (long)trig[g_camera_angle_y_70cd4 & 0xfff]), row) >> 6);
    }
    base_x = add32(base_x, sub32(mul32(x_shift, sine), mul32(z_shift, cosine)) >> 6);
    base_y = add32(base_y, add32(mul32(x_shift, cosine), mul32(z_shift, sine)) >> 6);
    x_shift = g_camera_offset_x_70cb8 >> 10;
    z_shift = g_camera_offset_z_70cc0 >> 10;
    base_x = add32(base_x, sub32(mul32(x_shift, sine), mul32(z_shift, cosine)) >> 6);
    base_y = add32(base_y, add32(mul32(x_shift, cosine), mul32(z_shift, sine)) >> 6);
    g_render_transform_70c60[0] = base_x;
    g_render_transform_70c60[2] = base_y;
    g_render_transform_70c60[1] = add32((long)(short)(g_camera_offset_y_70cbc >> 16), g_camera_y_70ca4);
    if (angle == 1) angle = 0;
    g_render_transform_70c60[3] = g_camera_angle_x_70cd0;
    g_render_transform_70c60[4] = sub32(g_camera_angle_y_70cd4, angle);
    g_render_transform_70c60[7] = add32(sub32(g_camera_y_70ca4, 0x20) >> 6, 2);
    screen_y = g_camera_center_y_70ce0;
    screen_x = g_camera_center_x_70cdc;
    g_render_transform_70c60[6] = sub32(g_camera_x_70ca0, 0x20) >> 6;
    dispatch_engine_2c9d0((u32)screen_x, (u32)screen_y);
    dispatch_engine_2c9f0((u32)g_camera_focal_70ce4);
}
