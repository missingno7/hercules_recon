#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef long s32;
typedef void (__cdecl *Native2ff0)(const u32 *);

extern unsigned short g_tile_texture_word_73480;
extern void __cdecl eng1_tile_draw_2ff0(const u32 *coordinates);
u8 *g_header_pointer_70d44;
u32 g_frame_global_70cf8;
u32 g_frame_global_73484;
u8 *g_dispatch_frame_current_71ec4;

enum {
    FRAME_BYTES = 0x1488,
    OUTPUT_BYTES = 0x5000,
    OUTPUT_START = 0x400,
    HEADER_BYTES = 4 + 21 * 17 * 2
};

static u8 *native_module;
static unsigned long checks;
static unsigned long failures;
static unsigned long native_color_saved;
static unsigned long native_cursor_saved;
static unsigned short native_texture_saved;
static u8 *native_header_saved;
static u8 *native_frame_saved;

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
        bytes[i] = (u8)(salt ^ (u8)(i * 29U + (i >> 3)));
}

static u8 *native_rva(u32 rva)
{
    return native_module + rva;
}

static void write_word(u8 *bytes, u16 value)
{
    *(u16 *)(void *)bytes = value;
}

static unsigned long pointer_offset(const u8 *base, const u8 *pointer)
{
    return (unsigned long)((unsigned long)pointer - (unsigned long)base);
}

static void normalize_pointer(u8 *bytes, const u8 *frame, const u8 *output)
{
        u32 value = *(u32 *)(void *)bytes;
        if (value >= (u32)(unsigned long)output &&
            value < (u32)(unsigned long)(output + OUTPUT_BYTES)) {
            *(u32 *)(void *)bytes =
                0x80000000UL | (value - (u32)(unsigned long)output);
        } else if (value >= (u32)(unsigned long)frame &&
                   value < (u32)(unsigned long)(frame + FRAME_BYTES)) {
            *(u32 *)(void *)bytes =
                0x40000000UL | (value - (u32)(unsigned long)frame);
        }
}

static void normalize_frame_links(u8 *bytes, const u8 *frame,
                                  const u8 *output)
{
    normalize_pointer(bytes + 0x70, frame, output);
    normalize_pointer(bytes + 0x7c, frame, output);
    normalize_pointer(bytes + 0x88, frame, output);
    normalize_pointer(bytes + 0x8c, frame, output);
}

static void normalize_packet_links(u8 *bytes, u32 end,
                                   const u8 *frame, const u8 *output)
{
    u32 offset = OUTPUT_START;
    if (end < OUTPUT_START || end > OUTPUT_BYTES)
        return;
    while (offset < end) {
        u32 size;
        if (end - offset < 8UL) {
            check(0, "truncated packet before link normalization");
            return;
        }
        if (bytes[offset + 7] == 0x64U)
            size = 0x14UL;
        else if (bytes[offset + 7] == 0x2cU)
            size = 0x28UL;
        else {
            check(0, "unknown packet opcode before link normalization");
            return;
        }
        if (size > end - offset) {
            check(0, "packet crosses captured cursor");
            return;
        }
        /* Only the packet's first DWORD is a linked-list pointer. */
        normalize_pointer(bytes + offset, frame, output);
        offset += size;
    }
}

static u8 *source_rva(u32 rva)
{
    return native_rva(rva);
}

static void seed_frame_pair(u8 *native_frame, u8 *source_frame,
                            u8 *native_output, u8 *source_output)
{
    seed(native_frame, FRAME_BYTES, 0x51U);
    seed(source_frame, FRAME_BYTES, 0x51U);
    seed(native_output, OUTPUT_BYTES, 0xc7U);
    seed(source_output, OUTPUT_BYTES, 0xc7U);
    *(u32 *)(void *)(native_frame + 0x88) =
        (u32)(unsigned long)(native_output + 0x100);
    *(u32 *)(void *)(native_frame + 0x8c) =
        (u32)(unsigned long)(native_output + 0x200);
    *(u32 *)(void *)(source_frame + 0x88) =
        (u32)(unsigned long)(source_output + 0x100);
    *(u32 *)(void *)(source_frame + 0x8c) =
        (u32)(unsigned long)(source_output + 0x200);
}

static void run_case(Native2ff0 native, const char *label,
                     short width, short height,
                     const u16 *grid, const u32 coordinates[2],
                     u16 texture_word)
{
    u8 native_header[HEADER_BYTES];
    u8 source_header[HEADER_BYTES];
    u8 native_frame[FRAME_BYTES];
    u8 source_frame[FRAME_BYTES];
    u8 native_output[OUTPUT_BYTES];
    u8 source_output[OUTPUT_BYTES];
    u8 native_frame_copy[FRAME_BYTES];
    u8 source_frame_copy[FRAME_BYTES];
    u8 native_output_copy[OUTPUT_BYTES];
    u8 source_output_copy[OUTPUT_BYTES];
    u8 **native_header_slot;
    u8 **native_frame_slot;
    u32 *native_color_slot;
    u32 *native_cursor_slot;
    u16 *native_texture_slot;
    u32 native_cursor;
    u32 source_cursor;
    s32 first_row;
    s32 first_column;
    u16 first_tile;
    unsigned long before_failures;
    size_t cells;
    size_t i;

    before_failures = failures;
    memset(native_header, 0, sizeof(native_header));
    memset(source_header, 0, sizeof(source_header));
    write_word(native_header, (u16)width);
    write_word(native_header + 2, (u16)height);
    write_word(source_header, (u16)width);
    write_word(source_header + 2, (u16)height);
    cells = (size_t)(unsigned short)width * (size_t)(unsigned short)height;
    for (i = 0; i < cells; ++i) {
        write_word(native_header + 4 + i * 2, grid[i]);
        write_word(source_header + 4 + i * 2, grid[i]);
    }

    seed_frame_pair(native_frame, source_frame,
                    native_output, source_output);
    native_header_slot = (u8 **)(void *)native_rva(0x70d44UL);
    native_color_slot = (u32 *)(void *)native_rva(0x70cf8UL);
    native_cursor_slot = (u32 *)(void *)native_rva(0x73484UL);
    native_frame_slot = (u8 **)(void *)native_rva(0x71ec4UL);
    native_texture_slot = (u16 *)(void *)native_rva(0x73480UL);

    *native_header_slot = native_header;
    *native_color_slot = 0x12345678UL;
    *native_cursor_slot = (u32)(unsigned long)(native_output + OUTPUT_START);
    *native_frame_slot = native_frame;
    *native_texture_slot = texture_word;

    g_header_pointer_70d44 = source_header;
    g_frame_global_70cf8 = 0x12345678UL;
    g_frame_global_73484 = (u32)(unsigned long)(source_output + OUTPUT_START);
    g_dispatch_frame_current_71ec4 = source_frame;
    g_tile_texture_word_73480 = texture_word;

    native(coordinates);
    eng1_tile_draw_2ff0(coordinates);

    native_cursor = *native_cursor_slot -
        (u32)(unsigned long)native_output;
    source_cursor = g_frame_global_73484 -
        (u32)(unsigned long)source_output;
    check(native_cursor == source_cursor,
          "native/source cursor advancement and base offset");
    check(native_cursor >= OUTPUT_START && native_cursor <= OUTPUT_BYTES,
          "fixture output cursor stays in allocated record buffer");

    memcpy(native_frame_copy, native_frame, sizeof(native_frame_copy));
    memcpy(source_frame_copy, source_frame, sizeof(source_frame_copy));
    memcpy(native_output_copy, native_output, sizeof(native_output_copy));
    memcpy(source_output_copy, source_output, sizeof(source_output_copy));
    normalize_frame_links(native_frame_copy, native_frame, native_output);
    normalize_frame_links(source_frame_copy, source_frame, source_output);
    normalize_packet_links(native_output_copy, native_cursor,
                           native_frame, native_output);
    normalize_packet_links(source_output_copy, source_cursor,
                           source_frame, source_output);
    check(memcmp(native_frame_copy, source_frame_copy,
                 sizeof(native_frame_copy)) == 0,
          "frame contents and both linked-list rotations agree");
    check(memcmp(native_output_copy, source_output_copy,
                 (size_t)native_cursor) == 0,
          "emitted packet bytes and linked record pointers agree");

    if (native_cursor > OUTPUT_START) {
        u8 *first_native = native_output + OUTPUT_START;
        u8 *first_source = source_output + OUTPUT_START;
        u32 expected_color;
        first_row = ((s32)coordinates[1] >> 4);
        if (first_row < 0)
            first_row = 0;
        first_column = ((s32)coordinates[0] >> 4) % width;
        first_tile = grid[(size_t)first_row * (size_t)width +
                          (size_t)first_column];
        expected_color = ((first_tile & 3UL) == 0)
            ? 0x64345678UL : 0x2c345678UL;
        if (*(u32 *)(void *)(first_native + 4) != expected_color ||
            *(u32 *)(void *)(first_source + 4) != expected_color)
            printf("color native=0x%08lx source=0x%08lx expected=0x%08lx\n",
                   *(u32 *)(void *)(first_native + 4),
                   *(u32 *)(void *)(first_source + 4), expected_color);
        check(*(u32 *)(void *)(first_native + 4) == expected_color &&
              *(u32 *)(void *)(first_source + 4) == expected_color,
              "emitted record stores masked packet color");
        check(*(u16 *)(void *)(first_native + 0x0e) == texture_word &&
              *(u16 *)(void *)(first_source + 0x0e) == texture_word,
              "nonzero live 0x73480 WORD is stored at record +0x0e");
    }

    if (failures == before_failures)
        printf("PASS 2ff0 %s cursor=0x%lx\n", label, native_cursor);
    else
        printf("CASE 2ff0 %s has failures (native cursor=0x%lx source cursor=0x%lx)\n",
               label, native_cursor, source_cursor);
}

static void run_zero_width_fault(Native2ff0 native)
{
    u8 native_header[8];
    u8 source_header[8];
    u8 native_frame[FRAME_BYTES];
    u8 source_frame[FRAME_BYTES];
    u8 native_output[OUTPUT_BYTES];
    u8 source_output[OUTPUT_BYTES];
    u32 coordinates[2];
    u8 **native_header_slot;
    u8 **native_frame_slot;
    u32 *native_color_slot;
    u32 *native_cursor_slot;
    u32 native_exception;
    u32 source_exception;

    memset(native_header, 0, sizeof(native_header));
    memset(source_header, 0, sizeof(source_header));
    write_word(native_header, 0);
    write_word(native_header + 2, 1);
    write_word(source_header, 0);
    write_word(source_header + 2, 1);
    seed_frame_pair(native_frame, source_frame,
                    native_output, source_output);
    native_header_slot = (u8 **)(void *)native_rva(0x70d44UL);
    native_color_slot = (u32 *)(void *)native_rva(0x70cf8UL);
    native_cursor_slot = (u32 *)(void *)native_rva(0x73484UL);
    native_frame_slot = (u8 **)(void *)native_rva(0x71ec4UL);
    *native_header_slot = native_header;
    *native_color_slot = 0x12345678UL;
    *native_cursor_slot = (u32)(unsigned long)(native_output + OUTPUT_START);
    *native_frame_slot = native_frame;
    g_header_pointer_70d44 = source_header;
    g_frame_global_70cf8 = 0x12345678UL;
    g_frame_global_73484 = (u32)(unsigned long)(source_output + OUTPUT_START);
    g_dispatch_frame_current_71ec4 = source_frame;
    coordinates[0] = 0;
    coordinates[1] = 0;
    native_exception = 0;
    source_exception = 0;

    __try {
        native(coordinates);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        native_exception = GetExceptionCode();
    }
    __try {
        eng1_tile_draw_2ff0(coordinates);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        source_exception = GetExceptionCode();
    }

    check(native_exception == EXCEPTION_INT_DIVIDE_BY_ZERO,
          "native zero-width signed IDIV raises divide error");
    check(source_exception == native_exception,
          "VC5 source signed modulo preserves zero-width divide error");
    check(*native_cursor_slot == (u32)(unsigned long)(native_output + OUTPUT_START) &&
          g_frame_global_73484 == (u32)(unsigned long)(source_output + OUTPUT_START),
          "divide error occurs before either cursor is advanced");
}

int main(int argc, char **argv)
{
    HMODULE module;
    Native2ff0 native;
    u32 coordinates[2];
    u16 grid_a[21 * 17];
    u16 grid_b[3 * 4];
    u16 grid_c[4];
    u8 *native_color_slot;
    u8 *native_cursor_slot;
    u8 *native_texture_slot;
    u8 **native_header_slot;
    u8 **native_frame_slot;
    u32 i;

    if (argc != 2) {
        printf("usage: tile_fixture.exe ENG1.DLL\n");
        return 2;
    }
    module = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (module == 0) {
        printf("LoadLibraryExA failed: %lu\n", GetLastError());
        return 3;
    }
    native_module = (u8 *)module;
    native = (Native2ff0)(void *)native_rva(0x2ff0UL);

    native_color_slot = native_rva(0x70cf8UL);
    native_cursor_slot = native_rva(0x73484UL);
    native_texture_slot = native_rva(0x73480UL);
    native_header_slot = (u8 **)(void *)native_rva(0x70d44UL);
    native_frame_slot = (u8 **)(void *)native_rva(0x71ec4UL);
    native_color_saved = *(u32 *)(void *)native_color_slot;
    native_cursor_saved = *(u32 *)(void *)native_cursor_slot;
    native_texture_saved = *(u16 *)(void *)native_texture_slot;
    native_header_saved = *native_header_slot;
    native_frame_saved = *native_frame_slot;

    for (i = 0; i < 21 * 17; ++i)
        grid_a[i] = (u16)(0x0100U + i * 0x10U);
    for (i = 0; i < 3 * 4; ++i)
        grid_b[i] = (u16)(0x0200U + i * 3U);
    grid_c[0] = 0x0000U;
    grid_c[1] = 0x0001U;
    grid_c[2] = 0x0102U;
    grid_c[3] = 0x0203U;

    coordinates[0] = 0x00000011UL;
    coordinates[1] = 0x00000021UL;
    run_case(native, "positive 21-cell merged run", 21, 17,
             grid_a, coordinates, 0xbeefU);

    coordinates[0] = 0x0000002fUL;
    coordinates[1] = 0xffffffffUL;
    run_case(native, "negative row clipping and wrapped columns", 3, 4,
             grid_b, coordinates, 0x8123U);

    coordinates[0] = 0x00000001UL;
    coordinates[1] = 0x00000000UL;
    run_case(native, "four packet kinds and list selection", 4, 1,
             grid_c, coordinates, 0x7a5cU);

    coordinates[0] = 0x00000000UL;
    coordinates[1] = 0x00000020UL;
    run_case(native, "zero-height head rotation", 1, 0,
             grid_c, coordinates, 0x1357U);

    run_zero_width_fault(native);

    *(u32 *)(void *)native_color_slot = native_color_saved;
    *(u32 *)(void *)native_cursor_slot = native_cursor_saved;
    *(u16 *)(void *)native_texture_slot = native_texture_saved;
    *native_header_slot = native_header_saved;
    *native_frame_slot = native_frame_saved;
    check(FreeLibrary(module) != 0,
          "original oracle module unloads after native cells are restored");

    printf("tile_draw_2ff0 cases=5 checks=%lu failures=%lu\n",
           checks, failures);
    return failures == 0 ? 0 : 1;
}
