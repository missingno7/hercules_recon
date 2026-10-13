#include <stdio.h>
#include "hercules_textures.h"

#define NATIVE_RECEIPT_SHA256 "109c7a7e9d99580167d20fa104bbd54de0f4f7ffc49d11a663f3a38309534212"

static int failures = 0;

static void fail_address(const char *name, int index,
                         renderer_u32 expected, renderer_u32 actual)
{
    printf("FAIL %s[%d]: expected 0x%08lx got 0x%08lx\n", name, index,
           (unsigned long)expected, (unsigned long)actual);
    failures++;
}

static void check_stream(const char *name,
                         HERC_TEX_ROW_MAPPING mapping,
                         renderer_i32 x,
                         renderer_i32 first_y,
                         renderer_i16 width,
                         renderer_i16 height,
                         const renderer_u32 *expected,
                         int expected_count)
{
    int dy;
    int dx;
    int byte_i;
    int index;
    renderer_u32 address;
    int ok;

    index = 0;
    for (dy = 0; dy < (int)height; ++dy) {
        for (dx = 0; dx < (int)width; ++dx) {
            for (byte_i = 0; byte_i < 2; ++byte_i) {
                ok = herc_tex_row_byte_address(
                    mapping, (renderer_i32)(x + dx),
                    (renderer_i32)(first_y + dy),
                    (renderer_u32)byte_i, &address);
                if (!ok) {
                    printf("FAIL %s[%d]: address helper rejected fixture\n",
                           name, index);
                    failures++;
                } else if (index >= expected_count ||
                           address != expected[index]) {
                    fail_address(name, index,
                                 index < expected_count ? expected[index] : 0U,
                                 address);
                }
                index++;
            }
        }
    }
    if (index != expected_count) {
        printf("FAIL %s: produced %d addresses, expected %d\n",
               name, index, expected_count);
        failures++;
    }
}

static void check_upload(const char *name,
                         renderer_i16 entry_y,
                         renderer_i32 x,
                         renderer_i32 y,
                         renderer_i16 width,
                         renderer_i16 height,
                         const renderer_u32 *expected,
                         int expected_count)
{
    HERC_TEX_ROW_MAPPING mapping;

    mapping = herc_tex_row_mapping_for_entry_y(entry_y);
    check_stream(name, mapping, x, y, width, height,
                 expected, expected_count);
}

static void check_copy(const char *name,
                       renderer_i16 source_entry_y,
                       renderer_i32 source_x,
                       renderer_i16 source_y,
                       renderer_i32 destination_x,
                       renderer_i32 destination_y,
                       renderer_i16 width,
                       renderer_i16 height,
                       const renderer_u32 *expected_source,
                       const renderer_u32 *expected_destination,
                       int expected_count)
{
    HERC_TEX_ROW_MAPPING mapping;

    /* A8F0 selects once from source RECT y, then uses that mapping for both
     * operands, even when destination y falls on the other standalone band. */
    mapping = herc_tex_row_mapping_for_entry_y(source_entry_y);
    check_stream(name, mapping, source_x, source_y, width, height,
                 expected_source, expected_count);
    check_stream(name, mapping, destination_x, destination_y, width, height,
                 expected_destination, expected_count);
}

static void check_api_edges(void)
{
    renderer_u32 address;
    int result;

    if (herc_tex_row_mapping_for_entry_y((renderer_i16)511) !=
        HERC_TEX_ROW_MAP_LOW_ENTRY) {
        printf("FAIL selector y=511\n");
        failures++;
    }
    if (herc_tex_row_mapping_for_entry_y((renderer_i16)512) !=
        HERC_TEX_ROW_MAP_HIGH_ENTRY) {
        printf("FAIL selector y=512\n");
        failures++;
    }
    result = herc_tex_row_byte_address((HERC_TEX_ROW_MAPPING)2, 0, 0, 0,
                                       &address);
    if (result != 0) {
        printf("FAIL invalid enum accepted\n");
        failures++;
    }
    result = herc_tex_row_byte_address(HERC_TEX_ROW_MAP_LOW_ENTRY,
                                       0, 0, 2, &address);
    if (result != 0) {
        printf("FAIL byte_in_word=2 accepted\n");
        failures++;
    }
    result = herc_tex_row_byte_address(HERC_TEX_ROW_MAP_LOW_ENTRY,
                                       0, 0, 0, (renderer_u32 *)0);
    if (result != 0) {
        printf("FAIL null result pointer accepted\n");
        failures++;
    }
    result = herc_tex_row_byte_address(HERC_TEX_ROW_MAP_LOW_ENTRY,
                                       20, (renderer_i32)32768, 0, &address);
    if (result == 0 || address != 0x009ee708U) {
        printf("FAIL 32-bit current row 32768: got 0x%08lx\n",
               (unsigned long)address);
        failures++;
    }
}

int main(void)
{
    static const renderer_u32 a710_255_256[] = {
        0x0097e5eaU, 0x0097e5ebU, 0x009ee6eaU, 0x009ee6ebU
    };
    static const renderer_u32 a710_511_512[] = {
        0x009fe5eaU, 0x009fe5ebU, 0x009ee6eaU, 0x009ee6ebU
    };
    static const renderer_u32 a710_1023_1024[] = {
        0x00a8e5eaU, 0x00a8e5ebU, 0x00a6e6eaU, 0x00a6e6ebU
    };
    static const renderer_u32 a710_slow_511_512[] = {
        0x009fe6deU, 0x009fe6dfU, 0x00a0e5e0U, 0x00a0e5e1U,
        0x009ee7deU, 0x009ee7dfU, 0x009fe6e0U, 0x009fe6e1U
    };
    static const renderer_u32 a8f0_src_255_256[] = {
        0x0097e5eaU, 0x0097e5ebU, 0x009ee6eaU, 0x009ee6ebU
    };
    static const renderer_u32 a8f0_dst_255_256[] = {
        0x0097e608U, 0x0097e609U, 0x009ee708U, 0x009ee709U
    };
    static const renderer_u32 a8f0_src_511_512[] = {
        0x009fe5eaU, 0x009fe5ebU, 0x009ee6eaU, 0x009ee6ebU
    };
    static const renderer_u32 a8f0_dst_511_512[] = {
        0x009fe608U, 0x009fe609U, 0x009ee708U, 0x009ee709U
    };
    static const renderer_u32 a8f0_src_1023_1024[] = {
        0x00a8e5eaU, 0x00a8e5ebU, 0x00a6e6eaU, 0x00a6e6ebU
    };
    static const renderer_u32 a8f0_dst_1023_1024[] = {
        0x00a8e608U, 0x00a8e609U, 0x00a6e708U, 0x00a6e709U
    };
    static const renderer_u32 a8f0_src_slow_511_512[] = {
        0x009fe6deU, 0x009fe6dfU, 0x00a0e5e0U, 0x00a0e5e1U,
        0x009ee7deU, 0x009ee7dfU, 0x009fe6e0U, 0x009fe6e1U
    };
    static const renderer_u32 a8f0_dst_slow_511_512[] = {
        0x00a0e6deU, 0x00a0e6dfU, 0x00a1e5e0U, 0x00a1e5e1U,
        0x009fe7deU, 0x009fe7dfU, 0x00a0e6e0U, 0x00a0e6e1U
    };
    static const renderer_u32 a8f0_src_entry511[] = {
        0x009fe5eaU, 0x009fe5ebU, 0x009ee6eaU, 0x009ee6ebU
    };
    static const renderer_u32 a8f0_dst_entry511[] = {
        0x009ee708U, 0x009ee709U, 0x009ee808U, 0x009ee809U
    };
    static const renderer_u32 a8f0_src_entry512[] = {
        0x00a6e6eaU, 0x00a6e6ebU, 0x00a6e7eaU, 0x00a6e7ebU
    };
    static const renderer_u32 a8f0_dst_entry512[] = {
        0x00a8e608U, 0x00a8e609U, 0x00a6e708U, 0x00a6e709U
    };
    static const renderer_u32 a8f0_src_dest_counter_boundary[] = {
        0x0096e6eaU, 0x0096e6ebU, 0x0096e7eaU, 0x0096e7ebU
    };
    static const renderer_u32 a8f0_dst_counter_boundary[] = {
        0x009fe608U, 0x009fe609U, 0x009ee708U, 0x009ee709U
    };
    static const renderer_u32 a8f0_src_argument_width[] = {
        0x0096e6eaU, 0x0096e6ebU
    };
    static const renderer_u32 a8f0_dst_y_65536[] = {
        0x009ee708U, 0x009ee709U
    };
    static const renderer_u32 a8f0_dst_x_65536[] = {
        0x0096e6e0U, 0x0096e6e1U
    };

    printf("native receipt sha256: %s\n", NATIVE_RECEIPT_SHA256);
    check_api_edges();
    check_upload("a710_y255_256_fast", 255, 5, 255, 1, 2,
                 a710_255_256, 4);
    check_copy("a8f0_y255_256_fast", 255, 5, 255, 20, 255, 1, 2,
               a8f0_src_255_256, a8f0_dst_255_256, 4);
    check_upload("a710_y511_512_fast", 511, 5, 511, 1, 2,
                 a710_511_512, 4);
    check_copy("a8f0_y511_512_fast", 511, 5, 511, 20, 511, 1, 2,
               a8f0_src_511_512, a8f0_dst_511_512, 4);
    check_upload("a710_y1023_1024_fast", 1023, 5, 1023, 1, 2,
                 a710_1023_1024, 4);
    check_copy("a8f0_y1023_1024_fast", 1023, 5, 1023, 20, 1023, 1, 2,
               a8f0_src_1023_1024, a8f0_dst_1023_1024, 4);
    check_upload("a710_y511_512_horizontal_page_cross", 511, 127, 511, 2, 2,
                 a710_slow_511_512, 8);
    check_copy("a8f0_y511_512_horizontal_page_cross", 511, 127, 511, 255, 511,
               2, 2, a8f0_src_slow_511_512, a8f0_dst_slow_511_512, 8);
    check_copy("a8f0_source511_destination512_low_entry_band",
               511, 5, 511, 20, 512, 1, 2,
               a8f0_src_entry511, a8f0_dst_entry511, 4);
    check_copy("a8f0_source512_destination511_high_entry_band",
               512, 5, 512, 20, 511, 1, 2,
               a8f0_src_entry512, a8f0_dst_entry512, 4);
    check_copy("a8f0_destination_i32_counter_32767_to_32768_low_entry",
               0, 5, 0, 20, 32767, 1, 2,
               a8f0_src_dest_counter_boundary,
               a8f0_dst_counter_boundary, 4);
    check_copy("a8f0_destination_y_dword_65536_low_entry",
               0, 5, 0, 20, 65536, 1, 1,
               a8f0_src_argument_width, a8f0_dst_y_65536, 2);
    check_copy("a8f0_destination_x_dword_65536_masked_address",
               0, 5, 0, 65536, 0, 1, 1,
               a8f0_src_argument_width, a8f0_dst_x_65536, 2);

    if (failures != 0) {
        printf("%d texture row mapping checks failed\n", failures);
        return 1;
    }
    printf("13 native address-stream fixtures and API edge checks passed\n");
    return 0;
}
