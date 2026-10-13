#ifndef PRIVATEHERCULES_TEXTURES_H
#define PRIVATEHERCULES_TEXTURES_H

#include "hercules_types.h"
#include <stddef.h>

/* Private, evidence-bounded description of HERCULES.EXE renderer services.
 * This is a reference ABI artifact; it is not canonical game source. */
typedef struct HERC_TEX_RECT16 {
    renderer_i16 x;
    renderer_i16 y;
    renderer_i16 width;
    renderer_i16 height;
} HERC_TEX_RECT16;
typedef char herc_tex_assert_rect16[(sizeof(HERC_TEX_RECT16) == 8) ? 1 : -1];
typedef char herc_tex_assert_rect16_offsets[
    (offsetof(HERC_TEX_RECT16,x)==0 && offsetof(HERC_TEX_RECT16,y)==2 &&
     offsetof(HERC_TEX_RECT16,width)==4 && offsetof(HERC_TEX_RECT16,height)==6)
    ? 1 : -1];

/* Raw address/reference helpers are portable and do not store host pointers.
 * The original callbacks are exposed only for an explicitly opted-in live
 * 32-bit x86 adapter; their addresses still belong to the pinned host image. */
#if defined(HERCULES_RENDERER_LIVE_X86)
RENDERER_ASSERT_LIVE_X86_POINTER_WIDTH(herc_tex_assert_live_pointer_width);
#if defined(_MSC_VER)
#define HERC_TEX_CDECL __cdecl
#else
#define HERC_TEX_CDECL __attribute__((cdecl))
#endif

/* Host callback table entries (HERCULES.EXE image base 0x400000):
 * +0x204 / RVA 0xA710: reads RECT16 and copies width*height rows of 16-bit
 * words from tightly packed source bytes; synchronously consumes source;
 * calls BD80 once. Caller ignores the result, so return type is not asserted.
 * +0x208 / RVA 0xA640: seven cdecl arguments and a packed DWORD result. Raw
 * argument names are deliberate; preserve all seven args and the return.
 * +0x20c / RVA 0xA8F0: RECT16 source plus two sign-extended 32-bit destination
 * stack slots whose signed low WORDs are consumed; calls BD80 and returns 0.
 * +0x23c / RVA 0x70C0: one DWORD whose low byte selects separate color/surface
 * behavior. Pointer typedefs require the guarded 32-bit x86 build.
 */
typedef void (HERC_TEX_CDECL *HERC_TEX_UPLOAD_FN)(
    const HERC_TEX_RECT16 *rect, const renderer_u8 *packed_words);
typedef renderer_u32 (HERC_TEX_CDECL *HERC_TEX_A640_FN)(
    const renderer_u8 *arg1, renderer_u32 arg2, renderer_u32 arg3,
    renderer_u32 arg4, renderer_u32 arg5, renderer_u32 arg6,
    renderer_u32 arg7);
typedef renderer_u32 (HERC_TEX_CDECL *HERC_TEX_COPY_FN)(
    const HERC_TEX_RECT16 *source_rect, renderer_i32 destination_x,
    renderer_i32 destination_y);
typedef void (HERC_TEX_CDECL *HERC_TEX_COLOR_FN)(renderer_u32 color_or_ff);
#endif

/* The helper's supported upload domain is deliberately narrower than every
 * native signed-WORD input: x in [0,1024), y in [0,512), with the rectangle
 * fully inside those bounds. It is an address-expression model, not an
 * allocation/ownership claim. */
#define HERC_TEX_UPLOAD_X_WORDS 1024U
#define HERC_TEX_UPLOAD_Y_ROWS  512U
#define HERC_TEX_MAP_BASE       ((renderer_u32)0x0096e6e0UL)
#define HERC_TEX_CLUT_BASE      ((renderer_u32)0x0092c390UL)

#ifdef __cplusplus
extern "C" {
#endif

int herc_tex_rect_supported(const HERC_TEX_RECT16 *rect);
int herc_tex_upload_byte_offset(
    renderer_u32 x_word, renderer_u32 y_row, renderer_u32 byte_in_word,
    renderer_u32 *offset_from_map_base);

/* A rasterizer sample is a byte at packet U,V. Ordinary tpage selects one of
 * 16 64-KiB pages; the 0x180 alternate-bank condition selects map-base+1 MiB
 * and replaces the ordinary page index. This computes an address expression
 * only and never dereferences it. */
int herc_tex_sample_byte_offset(
    renderer_u16 tpage, renderer_u8 u, renderer_u8 v,
    renderer_u32 *offset_from_map_base);

/* Observed CLUT transform: row=(clut_word>>6)-0x1e0, with values outside
 * [0,30] mapped to row zero. The blend-table selector is (tpage>>5)&3. */
renderer_u32 herc_tex_clut_row(renderer_u16 clut_word);
renderer_u32 herc_tex_clut_byte_offset(renderer_u16 clut_word,
                                       renderer_u8 texel);
/* Only the active palette loop CC50 after the D420 gate and nonzero-row test. */
int herc_tex_cc50_active_clut_writes(renderer_u8 texel);
renderer_u32 herc_tex_blend_table_index(renderer_u16 tpage);

/* Exact limited predicate for observed C882, CA51, and CBF4 loops, each of
 * which skips source byte zero. It does not describe the D420 CLUT path or all
 * raster paths. */
int herc_tex_c882_ca51_cbf4_writes(renderer_u8 texel);

/* A640's observed return path with its callee A340 returning 0. Raw DWORD
 * arguments are numbered as in HERC_TEX_A640_FN. */
renderer_u32 herc_tex_a640_return_mode0(
    renderer_u32 arg2, renderer_u32 arg4, renderer_u32 arg5);

#ifdef __cplusplus
}
#endif

#endif
