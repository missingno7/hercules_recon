#ifndef PRIVATEHERCULES_PIXEL_H
#define PRIVATEHERCULES_PIXEL_H

#include "hercules_types.h"

#define HERC_PIXEL_COLOR_ROW_BYTES ((renderer_u32)256UL)
#define HERC_PIXEL_BLEND_BYTES ((renderer_u32)65536UL)
#define HERC_PIXEL_BLEND_SELECTORS ((renderer_u32)4UL)

typedef int (*HercPixelReadByteFn)(void *context, renderer_u32 offset,
                                   renderer_u8 *value);
typedef int (*HercPixelWriteByteFn)(void *context, renderer_u8 value);
typedef int (*HercPixelReadDestinationFn)(void *context,
                                          renderer_u8 *value);

/* Bounded read-only view of one already-selected LUT. The callback receives
 * only a byte offset; context is a process-local opaque adapter pointer, not
 * an address copied from the original executable. */
typedef struct HercPixelTableReader {
    renderer_u32 length;
    HercPixelReadByteFn read_byte;
    void *context;
} HercPixelTableReader;

typedef struct HercPixelDestinationAccess {
    HercPixelReadDestinationFn read_byte;
    HercPixelWriteByteFn write_byte;
    void *context;
} HercPixelDestinationAccess;

typedef enum HercPixelStepResult {
    HERC_PIXEL_INVALID = -1,
    HERC_PIXEL_NO_WRITE = 0,
    HERC_PIXEL_WROTE = 1
} HercPixelStepResult;

/* One C700/C882 step. shade_row is the selected 256-byte row rooted at
 * 0x91C488 + color*256; blend_tables contains the four selected 64-KiB
 * tables rooted at 0x92E3D0 + selector*0x10000. The callback order for a
 * nonzero source is shade read, destination read, blend read, destination
 * write. A zero source returns NO_WRITE without invoking any callback. */
#ifdef __cplusplus
extern "C" {
#endif

HercPixelStepResult herc_pixel_c882_step(
    renderer_u8 source_texel, const HercPixelTableReader *shade_row,
    const HercPixelTableReader *blend_tables, renderer_u32 blend_table_count,
    renderer_u32 selector, const HercPixelDestinationAccess *destination);

/* One active CC50 step. palette_row is the already-selected 256-byte row
 * rooted at 0x92C390 + row*256. Source zero skips palette and destination
 * access. A nonzero source always writes palette[source], including a zero
 * palette result; this path does not read the prior destination byte. */
HercPixelStepResult herc_pixel_cc50_step(
    renderer_u8 source_texel, const HercPixelTableReader *palette_row,
    const HercPixelDestinationAccess *destination);

/* One C2E0 fill byte for D420's gate-off/nonzero-row route. Unlike the
 * source-indexed loops above, this always writes the sampled byte, including
 * zero. Repeating the step models byte values only, not span geometry. */
HercPixelStepResult herc_pixel_c2e0_fill_byte(
    renderer_u8 fill_byte, const HercPixelDestinationAccess *destination);

/* C8F0 reads destination, then the already-selected blend table at
 * (source<<8)|destination, then writes. Source zero skips all callbacks.
 * CAB0 copies nonzero source directly with no destination/LUT read. */
HercPixelStepResult herc_pixel_c8f0_step(
    renderer_u8 source_texel, const HercPixelTableReader *blend_table,
    const HercPixelDestinationAccess *destination);
HercPixelStepResult herc_pixel_cab0_step(
    renderer_u8 source_texel, const HercPixelDestinationAccess *destination);

#ifdef __cplusplus
}
#endif

#endif
