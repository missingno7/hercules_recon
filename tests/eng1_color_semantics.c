/* Source-only fixture for the ENG1 color routines; never loads game binaries. */
#include <stdio.h>
#include <string.h>

#include "../src/eng1/behavioral/color.c"
#include "../src/eng1/behavioral/color_storage.c"

unsigned short g_frame_word_6ee84;
unsigned char g_dispatch_key_7286c;
unsigned short g_dispatch_setting_4d858;
unsigned short g_dispatch_word_6ee80;
unsigned char g_packed_byte_70cf0;
unsigned char g_packed_byte_70cf1;
unsigned char g_packed_byte_70cf2;
unsigned long g_frame_global_70cf4;
unsigned char g_frame_global_734f0;
unsigned char g_frame_global_734f1;
unsigned char g_frame_global_734f2;
unsigned char *g_dispatch_frame_base_71ec8;
static unsigned long callback_value;
static int callback_count;

void __cdecl dispatch_engine_2c830(unsigned long frame_value)
{
    callback_value = frame_value;
    ++callback_count;
}

static int checks;
static int failures;

#define CHECK(condition) do { \
    ++checks; \
    if (!(condition)) { \
        ++failures; \
        printf("FAIL line %d: %s\n", __LINE__, #condition); \
    } \
} while (0)

static unsigned long table_hash(void)
{
    unsigned long hash;
    unsigned long value;
    int i;
    int byte_index;

    hash = 2166136261UL;
    for (i = 0; i != 512; ++i) {
        value = g_mesh_colors_6f4e8[i];
        for (byte_index = 0; byte_index != 4; ++byte_index) {
            hash = (hash ^ ((value >> (byte_index * 8)) & 0xffUL)) *
                   16777619UL;
        }
    }
    return hash;
}

static int table_is(unsigned long expected)
{
    int i;

    for (i = 0; i != 512; ++i) {
        if (g_mesh_colors_6f4e8[i] != expected)
            return 0;
    }
    return 1;
}

static int frame_canaries_ok(const unsigned char *storage, int total_size)
{
    int i;

    for (i = 0; i != 8; ++i) {
        if (storage[i] != 0xa5)
            return 0;
        if (storage[8 + total_size + i] != 0xa5)
            return 0;
    }
    return 1;
}

static int frame_bytes_match(const unsigned char *frame,
                             unsigned char red_a,
                             unsigned char green_a,
                             unsigned char blue_a,
                             unsigned char red_b,
                             unsigned char green_b,
                             unsigned char blue_b)
{
    int i;
    unsigned char expected;

    for (i = 0; i != 2 * 0x1488; ++i) {
        expected = 0xa5;
        if (i == 0x19)
            expected = red_a;
        else if (i == 0x1a)
            expected = green_a;
        else if (i == 0x1b)
            expected = blue_a;
        else if (i == 0x1488 + 0x19)
            expected = red_b;
        else if (i == 0x1488 + 0x1a)
            expected = green_b;
        else if (i == 0x1488 + 0x1b)
            expected = blue_b;
        if (frame[i] != expected)
            return 0;
    }
    return 1;
}

int main(void)
{
    unsigned char frame_storage[8 + (2 * 0x1488) + 8];
    int i;

    checks = 0;
    failures = 0;
    callback_count = 0;
    memset(frame_storage, 0xa5, sizeof frame_storage);
    g_dispatch_frame_base_71ec8 = frame_storage + 8;
    CHECK(g_mesh_colors_6f4e8[0] == 0);
    CHECK(g_frame_global_70cf8 == 0);
    CHECK(g_frame_global_70cfc == 0);
    CHECK(g_frame_global_73488 == 0);
    g_frame_word_6ee84 = 100;
    g_packed_byte_70cf0 = 64;
    g_packed_byte_70cf1 = 128;
    g_packed_byte_70cf2 = 192;
    g_frame_global_70cf4 = 0xffffffffUL;
    g_frame_global_734f0 = 0x11;
    g_frame_global_734f1 = 0x22;
    g_frame_global_734f2 = 0x33;

    eng1_color_grid_update_5a60(10, 20, 30, 50, 80, 110);
    CHECK(g_frame_global_70cf4 == 0x009e521dUL);
    CHECK(g_frame_global_70cf8 == 0x00553e27UL);
    CHECK(g_frame_global_70cfc == 89);
    CHECK(g_mesh_colors_6f4e8[63] == 0x009e521dUL);
    CHECK(g_mesh_colors_6f4e8[127] == 0x009e5256UL);
    CHECK(g_mesh_colors_6f4e8[191] == 0x009e8f1dUL);
    CHECK(g_mesh_colors_6f4e8[255] == 0x009e8f56UL);
    CHECK(g_mesh_colors_6f4e8[319] == 0x00c7521dUL);
    CHECK(g_mesh_colors_6f4e8[383] == 0x00c75256UL);
    CHECK(g_mesh_colors_6f4e8[447] == 0x00c78f1dUL);
    CHECK(g_mesh_colors_6f4e8[511] == 0x00c78f56UL);
    CHECK(table_hash() == 0x7903674dUL);
    CHECK(g_dispatch_frame_base_71ec8[0x19] == 0x11);
    CHECK(g_dispatch_frame_base_71ec8[0x1a] == 0x22);
    CHECK(g_dispatch_frame_base_71ec8[0x1b] == 0x33);
    CHECK(g_dispatch_frame_base_71ec8[0x14a1] == 0x11);
    CHECK(g_dispatch_frame_base_71ec8[0x14a2] == 0x22);
    CHECK(g_dispatch_frame_base_71ec8[0x14a3] == 0x33);
    CHECK(frame_bytes_match(g_dispatch_frame_base_71ec8,
                            0x11, 0x22, 0x33, 0x11, 0x22, 0x33));
    CHECK(frame_canaries_ok(frame_storage, 2 * 0x1488));

    for (i = 0; i != 512; ++i)
        g_mesh_colors_6f4e8[i] = 0xdeadbeefUL;
    g_dispatch_frame_base_71ec8[0x19] = 0x77;
    eng1_color_grid_update_5a60(10, 20, 30, 50, 80, 110);
    CHECK(g_mesh_colors_6f4e8[0] == 0xdeadbeefUL);
    CHECK(g_mesh_colors_6f4e8[511] == 0xdeadbeefUL);
    CHECK(table_is(0xdeadbeefUL));
    CHECK(g_dispatch_frame_base_71ec8[0x19] == 0x77);

    eng1_color_profile_5dc0(0x15);
    CHECK(g_frame_global_73488 == 0x2c151515UL);
    CHECK(g_frame_global_70cf4 == 0x00966432UL);
    CHECK(g_frame_global_70cfc == 100);
    CHECK(table_hash() == 0x7ebb55d5UL);
    CHECK(g_mesh_colors_6f4e8[64 + 63] == 0x009664c4UL);
    CHECK(g_mesh_colors_6f4e8[448 + 63] == 0x00c4c4c4UL);

    g_frame_global_734f0 = 0;
    g_frame_global_734f1 = 0xff;
    g_frame_global_734f2 = 0x80;
    eng1_frame_color_write_5e00(-1);
    CHECK(g_dispatch_frame_base_71ec8[0x19] == 0);
    CHECK(g_dispatch_frame_base_71ec8[0x1a] == 0xff);
    CHECK(g_dispatch_frame_base_71ec8[0x1b] == 0x80);
    CHECK(g_dispatch_frame_base_71ec8[0x14a1] == 0);
    CHECK(g_dispatch_frame_base_71ec8[0x14a2] == 0xff);
    CHECK(g_dispatch_frame_base_71ec8[0x14a3] == 0x80);
    CHECK(frame_bytes_match(g_dispatch_frame_base_71ec8,
                            0, 0xff, 0x80, 0, 0xff, 0x80));
    CHECK(frame_canaries_ok(frame_storage, 2 * 0x1488));

    g_dispatch_setting_4d858 = 128;
    g_dispatch_word_6ee80 = 0;
    g_frame_word_6ee84 = 100;
    g_dispatch_key_7286c = 0;
    eng1_color_fade_step_5d50();
    CHECK(g_frame_word_6ee84 == 100);
    CHECK(callback_count == 0);
    g_dispatch_key_7286c = 8;
    eng1_color_fade_step_5d50();
    CHECK(g_frame_word_6ee84 == 108);
    CHECK(g_dispatch_key_7286c == 8);
    CHECK(callback_value == 108);

    g_frame_word_6ee84 = 125;
    g_dispatch_key_7286c = 8;
    eng1_color_fade_step_5d50();
    CHECK(g_frame_word_6ee84 == 128);
    CHECK(g_dispatch_key_7286c == 0);
    CHECK(callback_value == 128);

    g_frame_word_6ee84 = 5;
    g_dispatch_key_7286c = 0xf6;
    g_dispatch_word_6ee80 = 0;
    eng1_color_fade_step_5d50();
    CHECK(g_frame_word_6ee84 == 0);
    CHECK(g_dispatch_key_7286c == 0);
    CHECK(callback_value == 0);

    g_frame_word_6ee84 = 0;
    g_dispatch_key_7286c = 0xff;
    g_dispatch_word_6ee80 = 0x8000;
    eng1_color_fade_step_5d50();
    CHECK(g_frame_word_6ee84 == 0xffff);
    CHECK(g_dispatch_key_7286c == 0xff);
    CHECK(callback_value == 0xff);
    CHECK(callback_count == 4);

    printf("checks=%d failures=%d\n", checks, failures);
    return failures ? 1 : 0;
}
