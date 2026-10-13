#ifndef HRC_STATE_COMMAND_WIRE_PROPOSAL_H
#define HRC_STATE_COMMAND_WIRE_PROPOSAL_H

/*
 * Private package proposal only. These packed wire views describe observed
 * byte offsets; they do not assert original SDK types, source names, or that
 * every producer initializes the opaque fields.
 */
#include <stddef.h>
#include "hercules_packets.h"

#pragma pack(push, 1)
typedef struct HrcF3WireView {
    renderer_raw32 ordering_link;    /* +0 */
    renderer_i16  x;                 /* +4 */
    renderer_u8   encoded_y;         /* +6; interpreted as unsigned */
    renderer_u8   tag;               /* +7; 0xF3 */
    renderer_i16  right_delta;       /* +8 */
    renderer_i16  bottom_delta;      /* +0xA */
} HrcF3WireView;

typedef struct HrcCommand01WireView {
    renderer_raw32 ordering_link;    /* +0 */
    renderer_u8   opaque_4_to_6[3];  /* +4..+6; not written by setter A610 */
    renderer_u8   tag;               /* +7; 0x01 */
    renderer_u8   opaque_8_to_b[4];  /* +8..+0xB; not consumed by AC60 */
    renderer_i16  destination_x;     /* +0xC; AC60 sign-extends */
    renderer_i16  destination_y;     /* +0xE; AC60 sign-extends */
    renderer_i16  source_x;          /* +0x10; A8F0 rectangle argument */
    renderer_i16  source_y;          /* +0x12 */
    renderer_i16  width;             /* +0x14 */
    renderer_i16  height;            /* +0x16 */
} HrcCommand01WireView;
#pragma pack(pop)

typedef char hrc_assert_f3_size[(sizeof(HrcF3WireView) == 12) ? 1 : -1];
typedef char hrc_assert_command01_size[(sizeof(HrcCommand01WireView) == 24) ? 1 : -1];
typedef char hrc_assert_f3_tag_offset[(offsetof(HrcF3WireView, tag) == 7) ? 1 : -1];
typedef char hrc_assert_command01_destination_offset[(offsetof(HrcCommand01WireView, destination_x) == 12) ? 1 : -1];
typedef char hrc_assert_command01_source_offset[(offsetof(HrcCommand01WireView, source_x) == 16) ? 1 : -1];

/*
 * Semantics captured separately in state_commands.md:
 * E1 reuses RendererE1ProducerPacket from hercules_packets.h. AC60 reads
 *   its mode_payload at +4; its tag byte is the high byte of that DWORD.
 * E1: A8E6FC=(word&0x180)==0x180; A8E6F4=word&0x1f;
 *     A8E6F8=(word>>5)&3. A909FC is not written in this branch.
 * F3: y=(encoded_y>=240 ? encoded_y-240 : encoded_y); clamp
 *     x, y, x+right_delta, y+bottom_delta to x:[0,320], y:[0,240].
 * 01: call A8F0(pointer to +0x10, signed destination_x, signed destination_y).
 *     A8F0's pointed source rectangle consists of the four signed WORDs above.
 */

#endif
