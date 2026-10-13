#include <stdio.h>
#include "hercules_textures.h"

static int expect_address(renderer_i16 x, renderer_i16 y,
                          renderer_u32 byte, renderer_u32 expected)
{
    renderer_u32 actual = 0;
    if (!herc_tex_upload_high_y_byte_address(x, y, byte, &actual) ||
        actual != expected) {
        fprintf(stderr, "address mismatch x=%d y=%d byte=%lu got=%08lx expected=%08lx\n",
                (int)x, (int)y, (unsigned long)byte,
                (unsigned long)actual, (unsigned long)expected);
        return 0;
    }
    return 1;
}

static int rejects(renderer_i16 x, renderer_i16 y, renderer_u32 byte)
{
    renderer_u32 actual = 0xdeadbeefUL;
    return !herc_tex_upload_high_y_byte_address(x, y, byte, &actual) &&
           actual == 0xdeadbeefUL;
}

int main(void)
{
    renderer_u32 first;
    renderer_u32 last;

    if (!expect_address(0, 512, 0, 0x00a6e6e0UL) ||
        !expect_address(127, 1023, 1, 0x00a8e6dfUL) ||
        !expect_address(284, 514, 0, 0x00a8e918UL))
        return 1;
    if (!herc_tex_upload_high_y_byte_address(0, 512, 0, &first) ||
        !herc_tex_upload_high_y_byte_address(127, 1023, 1, &last) ||
        first != HERC_TEX_MAP_BASE + 0x00100000UL ||
        last + 1U != 0x00a8e6e0UL) {
        fprintf(stderr, "high-y literal endpoints or address-only witness changed\n");
        return 1;
    }
    if (!rejects(0, 511, 0) || !rejects(0, 1024, 0) ||
        !rejects(-1, 512, 0) || !rejects(1024, 512, 0) ||
        !rejects(0, 512, 2) ||
        herc_tex_upload_high_y_byte_address(0, 512, 0, 0)) {
        fprintf(stderr, "bounded high-y reference accepted an excluded input\n");
        return 1;
    }
    puts("PASS high-y address endpoints and address-only reciprocal witness");
    return 0;
}
