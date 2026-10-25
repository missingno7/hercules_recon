/* Ordinary-C reconstruction of ENG1.DLL RVA 0x42320 and its
 * color forwarding helper 0x5a30.  This is a function task only. */
typedef unsigned char Eng1Call42320Byte;
typedef unsigned short Eng1Call42320Word;
typedef unsigned long Eng1Call42320Dword;

typedef char eng1_call_42320_long_is_32[(sizeof(long) == 4) ? 1 : -1];
typedef char eng1_call_42320_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_call_42320_dword_is_32[
    (sizeof(Eng1Call42320Dword) == 4) ? 1 : -1];

extern long g_camera_x_70ca0;
extern long g_camera_y_70ca4;

extern Eng1Call42320Word g_frame_word_6ee84;
extern Eng1Call42320Byte g_frame_global_734f0;
extern Eng1Call42320Byte g_frame_global_734f1;
extern Eng1Call42320Byte g_frame_global_734f2;
extern Eng1Call42320Byte *g_dispatch_frame_base_71ec8;

/* The source body for this missing semantic dependency is supplied by a
 * separate bounded reconstruction.  Its measured input view is read-only and
 * contains only the two DWORDs consumed at native offsets 0 and 4. */
extern void __cdecl eng1_tile_draw_2ff0(const Eng1Call42320Dword *xy);
extern void __cdecl eng1_frame_color_write_5e00(int unused_frame_word);

static long eng1_call_42320_signed_from_bits(Eng1Call42320Dword bits)
{
    if (bits <= 0x7fffffffUL)
        return (long)bits;
    return -1L - (long)(~bits);
}

static long eng1_call_42320_sar2(long value)
{
    Eng1Call42320Dword bits;

    bits = (Eng1Call42320Dword)value;
    bits >>= 2;
    if (((Eng1Call42320Dword)value & 0x80000000UL) != 0UL)
        bits |= 0xc0000000UL;
    return eng1_call_42320_signed_from_bits(bits);
}

static long eng1_call_42320_wrapped_delta_div3(long value)
{
    Eng1Call42320Dword bits;
    long signed_delta;

    bits = (Eng1Call42320Dword)value - 0x250UL;
    signed_delta = eng1_call_42320_signed_from_bits(bits);
    return signed_delta / 3L;
}

void __cdecl eng1_color_rgb_forward_5a30(Eng1Call42320Dword red_arg,
                                    Eng1Call42320Dword green_arg,
                                    Eng1Call42320Dword blue_arg)
{
    int signed_frame_word;

    /* Native stores red before it reads and sign-extends the WORD. */
    *(volatile Eng1Call42320Byte *)(void *)&g_frame_global_734f0 =
        (Eng1Call42320Byte)red_arg;
    signed_frame_word = (int)(short)
        *(volatile Eng1Call42320Word *)(void *)&g_frame_word_6ee84;

    *(volatile Eng1Call42320Byte *)(void *)&g_frame_global_734f1 =
        (Eng1Call42320Byte)green_arg;
    *(volatile Eng1Call42320Byte *)(void *)&g_frame_global_734f2 =
        (Eng1Call42320Byte)blue_arg;
    eng1_frame_color_write_5e00(signed_frame_word);
}

void __cdecl eng1_frame_target_042320(void)
{
    Eng1Call42320Dword xy[2];
    volatile Eng1Call42320Dword *xy_view;
    long camera_x;
    long camera_y;

    xy_view = xy;
    camera_x = *(volatile long *)(void *)&g_camera_x_70ca0;
    camera_y = *(volatile long *)(void *)&g_camera_y_70ca4;
    xy_view[0] = (Eng1Call42320Dword)eng1_call_42320_sar2(camera_x);
    xy_view[1] = (Eng1Call42320Dword)
        eng1_call_42320_wrapped_delta_div3(camera_y);
    eng1_tile_draw_2ff0(xy);

    if (eng1_call_42320_signed_from_bits(xy_view[1]) < 32L)
        eng1_color_rgb_forward_5a30(0x3fUL, 0x75UL, 0xbcUL);
    else
        eng1_color_rgb_forward_5a30(0x7fUL, 0x8bUL, 0x47UL);
}
