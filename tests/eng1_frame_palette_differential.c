#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef void (__cdecl *Native5a30)(u32, u32, u32);
typedef void (__cdecl *Native42320)(void);

extern long g_camera_x_70ca0;
extern long g_camera_y_70ca4;
extern u16 g_frame_word_6ee84;
extern u8 g_frame_global_734f0, g_frame_global_734f1, g_frame_global_734f2;
extern u8 *g_dispatch_frame_base_71ec8;
extern u8 *g_header_pointer_70d44;
extern u32 g_frame_global_70cf8, g_frame_global_73484;
extern u8 *g_dispatch_frame_current_71ec4;
extern u16 g_tile_texture_word_73480;

long g_camera_x_70ca0;
long g_camera_y_70ca4;
u16 g_frame_word_6ee84;
u8 g_frame_global_734f0, g_frame_global_734f1, g_frame_global_734f2;
u8 *g_dispatch_frame_base_71ec8;
u8 *g_header_pointer_70d44;
u32 g_frame_global_73484;
u8 *g_dispatch_frame_current_71ec4;

/* Test-only cells for unused color.c entrypoints; actual color_storage.c owns
 * the shared frame color and mesh table. Any accidental dispatch fails. */
u16 g_dispatch_word_6ee80;
u16 g_dispatch_setting_4d858;
u8 g_dispatch_key_7286c;
u8 g_packed_byte_70cf0, g_packed_byte_70cf1, g_packed_byte_70cf2;
u32 g_frame_global_70cf4;

void __cdecl dispatch_engine_2c830(u32 unused_frame_value)
{
    (void)unused_frame_value;
    printf("FAIL uncalled color.c dependency dispatch_engine_2c830\n");
    ExitProcess(99U);
}

extern void __cdecl eng1_frame_target_042320(void);
extern void __cdecl eng1_color_rgb_forward_5a30(u32, u32, u32);
extern void __cdecl eng1_tile_draw_2ff0(const u32 *coordinates);

static u8 *native_module;
static unsigned long checks;
static unsigned long failures;

static void check(int condition, const char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

static void seed(u8 *bytes, size_t length, u8 salt)
{
    size_t i;
    for (i = 0; i < length; ++i)
        bytes[i] = (u8)(salt ^ (u8)(i * 13U + (i >> 3)));
}

static u8 *native_rva(u32 rva)
{
    return native_module + rva;
}

static void put_dword(u8 *base, u32 offset, u32 value)
{
    memcpy(base + offset, &value, sizeof(value));
}

static u32 get_dword(const u8 *base, u32 offset)
{
    u32 value;
    memcpy(&value, base + offset, sizeof(value));
    return value;
}

static void normalize_pointer_words(u8 *bytes, size_t length,
                                    const u8 *frame, size_t frame_size,
                                    const u8 *output, size_t output_size)
{
    static const size_t pointer_offsets[4] = { 0x70, 0x7c, 0x88, 0x8c };
    size_t index;
    for (index = 0; index < 4; ++index) {
        size_t offset = pointer_offsets[index];
        u32 value;
        u32 frame_base;
        u32 output_base;
        if (offset + sizeof(u32) > length)
            continue;
        value = *(u32 *)(void *)(bytes + offset);
        frame_base = (u32)(unsigned long)frame;
        output_base = (u32)(unsigned long)output;
        if (value >= output_base && value - output_base < output_size) {
            *(u32 *)(void *)(bytes + offset) =
                0x80000000UL | (value - output_base);
        } else if (value >= frame_base && value - frame_base < frame_size) {
            *(u32 *)(void *)(bytes + offset) =
                0x40000000UL | (value - frame_base);
        }
    }
}

static void run_native_5a30_case(Native5a30 native, const char *label,
                                 u32 red, u32 green, u32 blue, u16 frame_word)
{
    enum { FRAME_BYTES = 0x14a4 };
    u8 native_frame[FRAME_BYTES], source_frame[FRAME_BYTES];
    u8 *native_rgb = native_rva(0x734f0UL);
    u16 *native_word = (u16 *)(void *)native_rva(0x6ee84UL);
    u8 **native_base = (u8 **)(void *)native_rva(0x71ec8UL);
    unsigned long before = failures;

    seed(native_frame, sizeof(native_frame), 0x61U);
    seed(source_frame, sizeof(source_frame), 0x61U);
    *native_word = frame_word;
    *native_base = native_frame;
    g_frame_word_6ee84 = frame_word;
    g_dispatch_frame_base_71ec8 = source_frame;
    native_rgb[0] = g_frame_global_734f0 = 0x11U;
    native_rgb[1] = g_frame_global_734f1 = 0x22U;
    native_rgb[2] = g_frame_global_734f2 = 0x33U;

    native(red, green, blue);
    eng1_color_rgb_forward_5a30(red, green, blue);
    check(native_rgb[0] == (u8)red && native_rgb[1] == (u8)green &&
          native_rgb[2] == (u8)blue, "native 5a30 stores low RGB bytes");
    check(g_frame_global_734f0 == (u8)red &&
          g_frame_global_734f1 == (u8)green &&
          g_frame_global_734f2 == (u8)blue, "source 5a30 stores low RGB bytes");
    check(memcmp(native_frame, source_frame, sizeof(native_frame)) == 0,
          "native/source 5a30 frame writes agree through actual color.c 5e00");
    check(native_frame[0x19] == (u8)red && native_frame[0x1a] == (u8)green &&
          native_frame[0x1b] == (u8)blue &&
          native_frame[0x14a1] == (u8)red &&
          native_frame[0x14a2] == (u8)green &&
          native_frame[0x14a3] == (u8)blue,
          "both frame color triplets receive the selected channels");
    if (failures == before)
        printf("PASS 5a30 %s\n", label);
}

static void run_paired_42320_zero_height_case(Native42320 native,
                                               const char *label,
                                               long camera_x, long camera_y,
                                               u8 red, u8 green, u8 blue)
{
    enum { FRAME_BYTES = 0x14a4, OUTPUT_BYTES = 0x40 };
    union { u32 align; u8 bytes[FRAME_BYTES]; } native_frame_storage;
    union { u32 align; u8 bytes[FRAME_BYTES]; } source_frame_storage;
    union { u32 align; u8 bytes[OUTPUT_BYTES]; } native_output_storage;
    union { u32 align; u8 bytes[OUTPUT_BYTES]; } source_output_storage;
    u8 *native_frame = native_frame_storage.bytes;
    u8 *source_frame = source_frame_storage.bytes;
    u8 *native_output = native_output_storage.bytes;
    u8 *source_output = source_output_storage.bytes;
    u8 native_frame_copy[FRAME_BYTES], source_frame_copy[FRAME_BYTES];
    u8 native_output_copy[OUTPUT_BYTES], source_output_copy[OUTPUT_BYTES];
    u8 native_header[4], source_header[4];
    u8 *saved_header;
    u8 *saved_current;
    u8 *saved_base;
    u32 saved_camera_x, saved_camera_y, saved_frame_color, saved_cursor;
    u16 saved_frame_word, saved_texture;
    u8 saved_rgb[3];
    u8 *native_rgb = native_rva(0x734f0UL);
    u8 **native_header_slot = (u8 **)(void *)native_rva(0x70d44UL);
    u8 **native_current_slot = (u8 **)(void *)native_rva(0x71ec4UL);
    u8 **native_base_slot = (u8 **)(void *)native_rva(0x71ec8UL);
    u32 *native_cursor_slot = (u32 *)(void *)native_rva(0x73484UL);
    u32 *native_color_slot = (u32 *)(void *)native_rva(0x70cf8UL);
    u16 *native_word_slot = (u16 *)(void *)native_rva(0x6ee84UL);
    u16 *native_texture_slot = (u16 *)(void *)native_rva(0x73480UL);
    u32 native_cursor, source_cursor;
    unsigned long before = failures;

    seed(native_frame, FRAME_BYTES, 0x4dU);
    seed(source_frame, FRAME_BYTES, 0x4dU);
    seed(native_output, OUTPUT_BYTES, 0x9bU);
    seed(source_output, OUTPUT_BYTES, 0x9bU);
    memset(native_header, 0, sizeof(native_header));
    memset(source_header, 0, sizeof(source_header));
    *(u16 *)(void *)native_header = *(u16 *)(void *)source_header = 1U;
    *(u16 *)(void *)(native_header + 2) = *(u16 *)(void *)(source_header + 2) = 0U;
    put_dword(native_frame, 0x88UL,
              (u32)(unsigned long)(native_output + 0x10));
    put_dword(native_frame, 0x8cUL,
              (u32)(unsigned long)(native_output + 0x20));
    put_dword(source_frame, 0x88UL,
              (u32)(unsigned long)(source_output + 0x10));
    put_dword(source_frame, 0x8cUL,
              (u32)(unsigned long)(source_output + 0x20));

    saved_camera_x = (u32)*((long *)(void *)native_rva(0x70ca0UL));
    saved_camera_y = (u32)*((long *)(void *)native_rva(0x70ca4UL));
    saved_header = *native_header_slot;
    saved_current = *native_current_slot;
    saved_base = *native_base_slot;
    saved_cursor = *native_cursor_slot;
    saved_frame_color = *native_color_slot;
    saved_frame_word = *native_word_slot;
    saved_texture = *native_texture_slot;
    saved_rgb[0] = native_rgb[0];
    saved_rgb[1] = native_rgb[1];
    saved_rgb[2] = native_rgb[2];

    *(long *)(void *)native_rva(0x70ca0UL) = camera_x;
    *(long *)(void *)native_rva(0x70ca4UL) = camera_y;
    *native_header_slot = native_header;
    *native_current_slot = native_frame;
    *native_base_slot = native_frame;
    *native_cursor_slot = (u32)(unsigned long)(native_output + 0x10);
    *native_color_slot = 0x12345678UL;
    *native_word_slot = 0x8001U;
    *native_texture_slot = 0x1357U;
    native_rgb[0] = 0x11U; native_rgb[1] = 0x22U; native_rgb[2] = 0x33U;

    g_camera_x_70ca0 = camera_x;
    g_camera_y_70ca4 = camera_y;
    g_header_pointer_70d44 = source_header;
    g_dispatch_frame_current_71ec4 = source_frame;
    g_dispatch_frame_base_71ec8 = source_frame;
    g_frame_global_73484 = (u32)(unsigned long)(source_output + 0x10);
    g_frame_global_70cf8 = 0x12345678UL;
    g_frame_word_6ee84 = 0x8001U;
    g_tile_texture_word_73480 = 0x1357U;
    g_frame_global_734f0 = 0x11U;
    g_frame_global_734f1 = 0x22U;
    g_frame_global_734f2 = 0x33U;

    native();
    eng1_frame_target_042320();

    native_cursor = *native_cursor_slot -
        (u32)(unsigned long)native_output;
    source_cursor = g_frame_global_73484 -
        (u32)(unsigned long)source_output;
    check(native_cursor == source_cursor && native_cursor == 0x10UL,
          "native/source 2ff0 zero-height cursor remains unchanged");
    check(native_rgb[0] == red && native_rgb[1] == green && native_rgb[2] == blue,
          "native 42320 chooses expected RGB after native 2ff0");
    check(g_frame_global_734f0 == red && g_frame_global_734f1 == green &&
          g_frame_global_734f2 == blue,
          "source 42320 chooses expected RGB after source 2ff0");

    memcpy(native_frame_copy, native_frame, sizeof(native_frame_copy));
    memcpy(source_frame_copy, source_frame, sizeof(source_frame_copy));
    memcpy(native_output_copy, native_output, sizeof(native_output_copy));
    memcpy(source_output_copy, source_output, sizeof(source_output_copy));
    normalize_pointer_words(native_frame_copy, sizeof(native_frame_copy),
                            native_frame, FRAME_BYTES, native_output, OUTPUT_BYTES);
    normalize_pointer_words(source_frame_copy, sizeof(source_frame_copy),
                            source_frame, FRAME_BYTES, source_output, OUTPUT_BYTES);
    check(memcmp(native_frame_copy, source_frame_copy, FRAME_BYTES) == 0,
          "native/source outer callback, real 2ff0 tail and 5e00 frame state agree");
    check(memcmp(native_output_copy, source_output_copy, OUTPUT_BYTES) == 0,
          "native/source zero-height output buffers agree");
    check(get_dword(native_frame, 0x70UL) ==
              (u32)(unsigned long)(native_output + 0x10) &&
          get_dword(native_frame, 0x7cUL) ==
              (u32)(unsigned long)(native_output + 0x20) &&
          get_dword(source_frame, 0x70UL) ==
              (u32)(unsigned long)(source_output + 0x10) &&
          get_dword(source_frame, 0x7cUL) ==
              (u32)(unsigned long)(source_output + 0x20),
          "both helpers preserve the old list heads at frame +0x70/+0x7c");

    *(long *)(void *)native_rva(0x70ca0UL) = (long)saved_camera_x;
    *(long *)(void *)native_rva(0x70ca4UL) = (long)saved_camera_y;
    *native_header_slot = saved_header;
    *native_current_slot = saved_current;
    *native_base_slot = saved_base;
    *native_cursor_slot = saved_cursor;
    *native_color_slot = saved_frame_color;
    *native_word_slot = saved_frame_word;
    *native_texture_slot = saved_texture;
    native_rgb[0] = saved_rgb[0];
    native_rgb[1] = saved_rgb[1];
    native_rgb[2] = saved_rgb[2];

    if (failures == before)
        printf("PASS 42320 paired zero-height %s\n", label);
}

int main(int argc, char **argv)
{
    HMODULE module;
    Native5a30 native_5a30;
    Native42320 native_42320;
    u8 *native_rgb;
    u16 *native_word;
    u8 **native_base;
    u8 saved_rgb[3];
    u16 saved_word;
    u8 *saved_base;

    if (argc != 2) {
        printf("usage: eng1_call_42320_paired_fixture ENG1.DLL\n");
        return 2;
    }
    module = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (module == 0) {
        printf("LoadLibraryExA failed: %lu\n", GetLastError());
        return 3;
    }
    native_module = (u8 *)module;
    native_5a30 = (Native5a30)(void *)native_rva(0x5a30UL);
    native_42320 = (Native42320)(void *)native_rva(0x42320UL);
    native_rgb = native_rva(0x734f0UL);
    native_word = (u16 *)(void *)native_rva(0x6ee84UL);
    native_base = (u8 **)(void *)native_rva(0x71ec8UL);
    saved_rgb[0] = native_rgb[0]; saved_rgb[1] = native_rgb[1];
    saved_rgb[2] = native_rgb[2];
    saved_word = *native_word;
    saved_base = *native_base;

    run_native_5a30_case(native_5a30, "low-byte positive-word",
                         0x1234563fUL, 0x98765475UL, 0xabcdefbcUL, 0x1234U);
    run_native_5a30_case(native_5a30, "negative-word",
                         0x1000017fUL, 0x2000028bUL, 0x30000347UL, 0x8000U);
    run_native_5a30_case(native_5a30, "all-ones-word",
                         0xffffffffUL, 0x11111111UL, 0x22222222UL, 0xffffU);

    run_paired_42320_zero_height_case(native_42320, "threshold 31",
                                      0L, 685L, 0x3fU, 0x75U, 0xbcU);
    run_paired_42320_zero_height_case(native_42320, "threshold 32",
                                      0L, 688L, 0x7fU, 0x8bU, 0x47U);
    run_paired_42320_zero_height_case(native_42320, "negative quotient",
                                      0L, 589L, 0x3fU, 0x75U, 0xbcU);
    run_paired_42320_zero_height_case(native_42320, "signed floor X",
                                      -5L, 685L, 0x3fU, 0x75U, 0xbcU);
    run_paired_42320_zero_height_case(native_42320, "wrapped subtraction",
                                      9L, (long)0x800002b0UL,
                                      0x3fU, 0x75U, 0xbcU);

    native_rgb[0] = saved_rgb[0]; native_rgb[1] = saved_rgb[1];
    native_rgb[2] = saved_rgb[2];
    *native_word = saved_word;
    *native_base = saved_base;
    check(FreeLibrary(module) != 0,
          "original DLL unloads after all modified cells are restored");
    printf("eng1_call_42320_paired cases=8 checks=%lu failures=%lu\n",
           checks, failures);
    return failures == 0 ? 0 : 1;
}
