#include <stdio.h>
#define HERCULES_RENDERER_LIVE_X86 1
#include "hercules_textures.h"

static int check(int condition, const char *name)
{
    if (!condition) {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    HERC_TEX_RECT16 rect;
    renderer_u32 offset;
    int failures = 0;

    rect.x = 0; rect.y = 224; rect.width = 7; rect.height = 3;
    failures += check(herc_tex_rect_supported(&rect), "rect domain");
    failures += check(herc_tex_upload_byte_offset(5, 224, 0, &offset) &&
                      offset == 0x0000E00AU, "y224 starts at +0xE000 row");
    failures += check(herc_tex_upload_byte_offset(127, 478, 0, &offset) &&
                      offset == 0x0008DEFEU, "x page crossing low byte");
    failures += check(herc_tex_upload_byte_offset(128, 478, 0, &offset) &&
                      offset == 0x0009DE00U, "x page crossing next page");
    failures += check(herc_tex_upload_byte_offset(1023, 511, 1, &offset) &&
                      offset == 0x000FFFFFU, "low-bank address upper bound");
    failures += check(!herc_tex_upload_byte_offset(0, 512, 0, &offset),
                      "reject unbounded high-y upload domain");
    rect.x = 0; rect.y = 511; rect.width = 1; rect.height = 2;
    failures += check(!herc_tex_rect_supported(&rect), "reject y crossing");
    rect.x = 1024; rect.y = 0; rect.width = 0; rect.height = 0;
    failures += check(!herc_tex_rect_supported(&rect), "reject empty x=1024");

    failures += check(herc_tex_sample_byte_offset(0x001eU, 7, 9, &offset) &&
                      offset == 0x000F0907U, "ordinary page index");
    failures += check(herc_tex_sample_byte_offset(0x0180U, 7, 9, &offset) &&
                      offset == 0x00100907U, "alternate page base");
    failures += check(herc_tex_blend_table_index(0x0060U) == 3U,
                      "blend table index");
    failures += check(herc_tex_clut_row((renderer_u16)(0x1e0U << 6)) == 0U,
                      "CLUT row zero");
    failures += check(herc_tex_clut_row((renderer_u16)(0x1feU << 6)) == 30U,
                      "CLUT row thirty");
    failures += check(herc_tex_clut_byte_offset(
                          (renderer_u16)(0x1feU << 6), 0x7fU) == 0x1e7fU,
                      "CLUT palette byte offset");
    failures += check(!herc_tex_cc50_active_clut_writes(0U) &&
                      herc_tex_cc50_active_clut_writes(1U),
                      "active CC50 CLUT loop skips source zero");
    failures += check(herc_tex_clut_row(0U) == 0U, "invalid CLUT clamps zero");
    failures += check(!herc_tex_c882_ca51_cbf4_writes(0U) &&
                      herc_tex_c882_ca51_cbf4_writes(1U),
                      "C882/CA51/CBF4 skip source zero");
    failures += check(herc_tex_a640_return_mode0(0x300U, 0x34U, 0x02U) == 0x210U,
                      "A640 packed mode-zero return");
    if (failures == 0)
        printf("PASS texture helper synthetic cases\n");
    return failures != 0;
}
