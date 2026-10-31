/* Private ordinary-C reconstruction of the ENG1 color startup helpers.
 * Function task only: this source does not claim original TU ownership. */
typedef unsigned char eng1_u8;
typedef unsigned short eng1_u16;
typedef unsigned long eng1_u32;
typedef char eng1_requires_32bit_ulong[(sizeof(eng1_u32) == 4) ? 1 : -1];

extern eng1_u16 g_frame_word_6ee84;
extern eng1_u8 g_dispatch_key_7286c;
extern eng1_u16 g_dispatch_setting_4d858;
extern eng1_u16 g_dispatch_word_6ee80;
extern eng1_u8 g_packed_byte_70cf0;
extern eng1_u8 g_packed_byte_70cf1;
extern eng1_u8 g_packed_byte_70cf2;
extern eng1_u32 g_frame_global_70cf4;
extern eng1_u32 g_frame_global_70cf8;
extern eng1_u8 g_frame_global_70cfc;
extern eng1_u32 g_frame_global_73488;
extern eng1_u8 g_frame_global_734f0;
extern eng1_u8 g_frame_global_734f1;
extern eng1_u8 g_frame_global_734f2;
extern eng1_u8 *g_dispatch_frame_base_71ec8;
extern eng1_u32 g_mesh_colors_6f4e8[512];
extern void __cdecl dispatch_engine_2c830(eng1_u32 frame_value);
void __cdecl eng1_color_profile_5dc0(int level);

static int eng1_clamp_above_255(int component)
{
    return component > 255 ? 255 : component;
}

static int eng1_multiply_low32_shift15(int left, int right)
{
    eng1_u32 product;

    product = (eng1_u32)left * (eng1_u32)right;
    return (int)((long)product >> 15);
}

static eng1_u32 eng1_pack_rgb(int red, int green, int blue)
{
    return (eng1_u32)red + ((eng1_u32)green << 8) +
           ((eng1_u32)blue << 16);
}

void __cdecl eng1_color_fade_step_5d50(void)
{
    int signed_step;
    eng1_u16 next_value;
    int clamped;

    signed_step = (int)(signed char)g_dispatch_key_7286c;
    if (signed_step == 0)
        return;

    next_value = (eng1_u16)(g_frame_word_6ee84 + signed_step);
    g_frame_word_6ee84 = next_value;
    clamped = 0;
    if (signed_step > 0) {
        if (next_value > g_dispatch_setting_4d858) {
            next_value = g_dispatch_setting_4d858;
            clamped = 1;
        } else if ((short)next_value < (short)g_dispatch_word_6ee80) {
            next_value = g_dispatch_word_6ee80;
            clamped = 1;
        }
    } else if ((short)next_value < (short)g_dispatch_word_6ee80) {
        next_value = g_dispatch_word_6ee80;
        clamped = 1;
    }
    if (clamped) {
        g_dispatch_key_7286c = 0;
        g_frame_word_6ee84 = next_value;
    }

    eng1_color_profile_5dc0((int)(short)next_value);
    dispatch_engine_2c830((eng1_u32)(eng1_u8)next_value);
}

void __cdecl eng1_frame_color_write_5e00(int unused_frame_word)
{
    int red;
    int green;
    int blue;
    eng1_u8 *frame;

    (void)unused_frame_word;
    red = eng1_clamp_above_255((int)g_frame_global_734f0);
    green = eng1_clamp_above_255((int)g_frame_global_734f1);
    blue = eng1_clamp_above_255((int)g_frame_global_734f2);
    frame = g_dispatch_frame_base_71ec8;

    frame[0x19] = (eng1_u8)red;
    frame[0x1a] = (eng1_u8)green;
    frame[0x1b] = (eng1_u8)blue;
    frame[0x1488 + 0x19] = (eng1_u8)red;
    frame[0x1488 + 0x1a] = (eng1_u8)green;
    frame[0x1488 + 0x1b] = (eng1_u8)blue;
}

void __cdecl eng1_color_grid_update_5a60(int base_red,
                                         int base_green,
                                         int base_blue,
                                         int rate_red,
                                         int rate_green,
                                         int rate_blue)
{
    int frame_word;
    int scaled_red;
    int scaled_green;
    int scaled_blue;
    int red;
    int green;
    int blue;
    int average;
    int red_accumulator;
    int green_accumulator;
    int blue_accumulator;
    int row;
    int bank;
    int table_red;
    int table_green;
    int table_blue;
    int bank_red;
    int bank_green;
    int bank_blue;
    eng1_u32 new_color;

    frame_word = (int)(short)g_frame_word_6ee84;
    scaled_red = (rate_red * 2) * frame_word;
    scaled_green = (rate_green * 2) * frame_word;
    scaled_blue = (rate_blue * 2) * frame_word;

    g_frame_global_70cf8 =
        (((eng1_u32)scaled_blue & 0x00ffff00UL) << 8) +
        (eng1_u32)(scaled_red >> 8) +
        ((eng1_u32)scaled_green & 0xffffff00UL);

    red = (eng1_multiply_low32_shift15(
               scaled_red, (int)g_packed_byte_70cf0)) + base_red;
    green = (eng1_multiply_low32_shift15(
                 scaled_green, (int)g_packed_byte_70cf1)) + base_green;
    blue = (eng1_multiply_low32_shift15(
                scaled_blue, (int)g_packed_byte_70cf2)) + base_blue;
    red = eng1_clamp_above_255(red);
    green = eng1_clamp_above_255(green);
    blue = eng1_clamp_above_255(blue);

    average = (red + green + blue) / 3;
    g_frame_global_70cfc = (eng1_u8)average;
    new_color = eng1_pack_rgb(red, green, blue);
    if (new_color == g_frame_global_70cf4)
        return;

    g_frame_global_70cf4 = new_color;
    red_accumulator = 0;
    green_accumulator = 0;
    blue_accumulator = 0;

    for (row = 0; row != 64; ++row) {
        table_red = eng1_clamp_above_255(
            base_red + (red_accumulator >> 13));
        table_green = eng1_clamp_above_255(
            base_green + (green_accumulator >> 13));
        table_blue = eng1_clamp_above_255(
            base_blue + (blue_accumulator >> 13));

        for (bank = 0; bank != 8; ++bank) {
            /* Native off bits select the weighted/clamped current color. */
            bank_red = (bank & 1) ? table_red : red;
            bank_green = (bank & 2) ? table_green : green;
            bank_blue = (bank & 4) ? table_blue : blue;
            g_mesh_colors_6f4e8[bank * 64 + row] =
                eng1_pack_rgb(bank_red, bank_green, bank_blue);
        }

        red_accumulator += scaled_red;
        green_accumulator += scaled_green;
        blue_accumulator += scaled_blue;
    }

    eng1_frame_color_write_5e00(frame_word);
}

void __cdecl eng1_color_profile_5dc0(int level)
{
    g_frame_global_73488 = 0x2c000000UL +
                           (eng1_u32)level * 0x00010101UL;
    eng1_color_grid_update_5a60(0, 0, 0, 128, 128, 128);
}
