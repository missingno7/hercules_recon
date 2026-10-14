#ifndef RENDERER_PACKETS_H
#define RENDERER_PACKETS_H

#include <stddef.h>
#include "hercules_types.h"

#if defined(_MSC_VER)
#pragma pack(push, 1)
#else
#pragma pack(push, 1)
#endif

/* Exact prefix/allocation views supported by the native PC consumer and the
 * independently observed writers. These contain raw addresses, not pointers. */
typedef struct renderer_ft4_packet {
    renderer_raw32 next;
    renderer_u8 color_r, color_g, color_b, command;
    renderer_i16 x0, y0;
    renderer_u8 u0, v0;
    renderer_u16 clut;
    renderer_i16 x1, y1;
    renderer_u8 u1, v1;
    renderer_u16 tpage;
    renderer_i16 x2, y2;
    renderer_u8 u2, v2;
    renderer_u16 opaque_word_01e;
    renderer_i16 x3, y3;
    renderer_u8 u3, v3;
    renderer_u16 opaque_word_026;
} RendererFt4Packet;

typedef struct renderer_gt4_packet {
    renderer_raw32 next;
    renderer_u8 color0_r, color0_g, color0_b, command;
    renderer_i16 x0, y0;
    renderer_u8 u0, v0;
    renderer_u16 clut;
    renderer_u8 color1_r, color1_g, color1_b, opaque_byte_013;
    renderer_i16 x1, y1;
    renderer_u8 u1, v1;
    renderer_u16 tpage;
    renderer_u8 color2_r, color2_g, color2_b, opaque_byte_01f;
    renderer_i16 x2, y2;
    renderer_u8 u2, v2;
    renderer_u16 opaque_word_026;
    renderer_u8 color3_r, color3_g, color3_b, opaque_byte_02b;
    renderer_i16 x3, y3;
    renderer_u8 u3, v3;
    renderer_u16 opaque_word_032;
} RendererGt4Packet;

typedef struct renderer_ft3_prefix {
    renderer_raw32 next;
    renderer_u8 color_r, color_g, color_b, command;
    renderer_i16 x0, y0;
    renderer_u8 u0, v0;
    renderer_u16 clut;
    renderer_i16 x1, y1;
    renderer_u8 u1, v1;
    renderer_u16 tpage;
    renderer_i16 x2, y2;
    renderer_u8 u2, v2;
} RendererFt3Prefix;

typedef struct renderer_sprite64_packet {
    renderer_raw32 next;
    renderer_u8 producer_color[3]; /* AC60 does not consume these bytes. */
    renderer_u8 command;
    renderer_i16 x, y;
    renderer_u8 u, v;
    /* Producer-populated at +0x0e; AC60 does not read this word. */
    renderer_u16 producer_word_00e;
    renderer_u16 width, height;
} RendererSprite64Packet;

/* Compact sprites use current renderer color/page state. The fixed 0x74/0x7c
 * records are consumer-proven only through UV byte +0xd. No producer
 * allocation extent is claimed for this 14-byte prefix. */
typedef struct renderer_fixed_sprite_prefix {
    renderer_raw32 next;
    renderer_u8 producer_color[3]; /* AC60 uses renderer state instead. */
    renderer_u8 command;
    renderer_i16 x, y;
    renderer_u8 u, v;
} RendererFixedSpritePrefix;

/* AC60 reads the first DWORD only. A540 writes the following two DWORDs. */
typedef struct renderer_e1_producer_packet {
    renderer_raw32 next;
    renderer_u32 mode_payload;
    renderer_u32 producer_tail;
} RendererE1ProducerPacket;

#pragma pack(pop)

#define RENDERER_PACKET_ASSERT_SIZE(name, type, n) \
    typedef char name[(sizeof(type) == (n)) ? 1 : -1]
#define RENDERER_PACKET_ASSERT_OFFSET(name, type, field, n) \
    typedef char name[(offsetof(type, field) == (n)) ? 1 : -1]

RENDERER_PACKET_ASSERT_SIZE(renderer_assert_ft4_size, RendererFt4Packet, 0x28);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_ft4_next, RendererFt4Packet, next, 0x00);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_ft4_command, RendererFt4Packet, command, 0x07);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_ft4_xy0, RendererFt4Packet, x0, 0x08);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_ft4_uv0, RendererFt4Packet, u0, 0x0c);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_ft4_clut, RendererFt4Packet, clut, 0x0e);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_ft4_xy1, RendererFt4Packet, x1, 0x10);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_ft4_tpage, RendererFt4Packet, tpage, 0x16);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_ft4_xy2, RendererFt4Packet, x2, 0x18);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_ft4_xy3, RendererFt4Packet, x3, 0x20);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_ft4_uv3, RendererFt4Packet, u3, 0x24);
RENDERER_PACKET_ASSERT_SIZE(renderer_assert_gt4_size, RendererGt4Packet, 0x34);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_gt4_color1, RendererGt4Packet, color1_r, 0x10);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_gt4_xy1, RendererGt4Packet, x1, 0x14);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_gt4_tpage, RendererGt4Packet, tpage, 0x1a);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_gt4_color2, RendererGt4Packet, color2_r, 0x1c);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_gt4_xy3, RendererGt4Packet, x3, 0x2c);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_gt4_uv3, RendererGt4Packet, u3, 0x30);
RENDERER_PACKET_ASSERT_SIZE(renderer_assert_ft3_prefix_size, RendererFt3Prefix, 0x1e);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_ft3_xy2, RendererFt3Prefix, x2, 0x18);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_ft3_uv2, RendererFt3Prefix, u2, 0x1c);
RENDERER_PACKET_ASSERT_SIZE(renderer_assert_sprite64_size, RendererSprite64Packet, 0x14);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_sprite64_dimensions, RendererSprite64Packet, width, 0x10);
RENDERER_PACKET_ASSERT_SIZE(renderer_assert_fixed_sprite_size, RendererFixedSpritePrefix, 0x0e);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_fixed_sprite_uv, RendererFixedSpritePrefix, u, 0x0c);
RENDERER_PACKET_ASSERT_SIZE(renderer_assert_e1_producer_size, RendererE1ProducerPacket, 0x0c);
RENDERER_PACKET_ASSERT_OFFSET(renderer_assert_e1_payload, RendererE1ProducerPacket, mode_payload, 0x04);

typedef enum RendererPacketKind {
    RENDERER_PACKET_UNKNOWN = 0,
    RENDERER_PACKET_FT3 = 1,
    RENDERER_PACKET_FT4 = 2,
    RENDERER_PACKET_GT4 = 3,
    RENDERER_PACKET_SPRITE_VARIABLE = 4,
    RENDERER_PACKET_SPRITE_8 = 5,
    RENDERER_PACKET_SPRITE_16 = 6,
    RENDERER_PACKET_E1_MODE = 7,
    RENDERER_PACKET_KNOWN_UNSUPPORTED = 8,
    RENDERER_PACKET_OBSERVED_UNDRAWN = 9
} RendererPacketKind;

typedef enum RendererPacketStatus {
    RENDERER_PACKET_OK = 0,
    RENDERER_PACKET_INVALID_ARGUMENT = 1,
    RENDERER_PACKET_SHORT_HEADER = 2,
    RENDERER_PACKET_TRUNCATED = 3,
    RENDERER_PACKET_UNSUPPORTED = 4,
    RENDERER_PACKET_UNRECOGNIZED = 5,
    RENDERER_PACKET_RANGE_ERROR = 6,
    RENDERER_PACKET_ADDRESS_OVERFLOW = 7,
    RENDERER_PACKET_KNOWN_UNDRAWN = 8
} RendererPacketStatus;

/* Raw OT descriptor only. Per-module frame layout supplies first_slot and
 * slot_count; this is not a common frame aggregate and contains no pointer. */
typedef struct renderer_raw_order_table {
    renderer_raw32 first_slot;
    renderer_u32 slot_count;
} RendererRawOrderTable;

typedef struct renderer_decoded_packet {
    RendererPacketKind kind;
    renderer_u16 minimum_size;
    renderer_u8 command;
    renderer_u8 semitransparent;
    renderer_u8 producer_tag_observed; /* Exact 0x68 only; no 0x6a producer claim. */
    renderer_u8 vertex_count;
    /* Number of color lanes consumed by the PC path; GT4's later producer
     * lanes are intentionally not interpreted because AC60 ignores them. */
    renderer_u8 color_count;
    renderer_raw32 next;
    renderer_u8 color[4][3];
    renderer_i16 x[4];
    renderer_i16 y[4];
    renderer_u8 u[4];
    renderer_u8 v[4];
    /* Set only for polygon records whose AC60 path consumes the UV word. */
    renderer_u16 clut;
    renderer_u16 tpage;
    renderer_u16 width;
    renderer_u16 height;
    renderer_u32 mode_payload;
} RendererDecodedPacket;

/* Compute a bucket address and its next-slot sentinel as raw32 values. For the
 * last bucket, sentinel is one-past the table and must never be dereferenced. */
#ifdef __cplusplus
extern "C" {
#endif
RendererPacketStatus renderer_order_table_addresses(
    const RendererRawOrderTable *table, renderer_u32 bucket_index,
    renderer_raw32 *bucket_address, renderer_raw32 *next_sentinel_address);

RendererPacketStatus renderer_decode_packet(
    const renderer_u8 *bytes, size_t length, RendererDecodedPacket *decoded);

/* Required owned observation span for a decoder-produced complete header.
 * minimum_size continues to describe AC60's direct access envelope. Command
 * 01 additionally passes the inline rectangle at +0x10 to A8F0, so capture
 * needs 24 bytes even though AC60 directly reads only 16. This helper does
 * not claim producer allocation size, drawing support, or frame ownership.
 * A TRUNCATED/UNSUPPORTED/UNRECOGNIZED decode may still supply that header.
 * Null/short-header results return zero and must not be captured. */
renderer_u16 renderer_packet_capture_size(const RendererDecodedPacket *decoded);

#ifdef __cplusplus
}
#endif

#endif
