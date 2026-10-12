#ifndef ENG1_COLD_START_FRAME_DATA_H
#define ENG1_COLD_START_FRAME_DATA_H

typedef unsigned char cold_frame_u8;
typedef unsigned short cold_frame_u16;
typedef unsigned long cold_frame_u32;

/* Measured bytes only: this bound prevents the candidate from indexing past
 * the recovered prefix. It does not claim the original table's declaration. */
#define COLD_FRAME_COORDINATE_PREFIX_BYTES 0x180U
#define COLD_FRAME_COORDINATE_PREFIX_ROWS \
    (COLD_FRAME_COORDINATE_PREFIX_BYTES / 2U)

extern cold_frame_u8
    g_frame_coordinate_prefix_5dd80[COLD_FRAME_COORDINATE_PREFIX_BYTES];

/* The target accesses a 12-byte loader-zero prefix at 0x6ffc8. Its original
 * aggregate and full extent are unknown. Macros retain exact relative cells. */
extern cold_frame_u8 g_frame_color_state_prefix_6ffc8[0x0c];

#define g_frame_global_6ffc8 \
    (*(cold_frame_u32 *)(void *)(g_frame_color_state_prefix_6ffc8 + 0x00))
#define g_frame_global_6ffcc \
    (*(cold_frame_u32 *)(void *)(g_frame_color_state_prefix_6ffc8 + 0x04))
#define g_frame_global_6ffd0 \
    (*(cold_frame_u16 *)(void *)(g_frame_color_state_prefix_6ffc8 + 0x08))

#endif
