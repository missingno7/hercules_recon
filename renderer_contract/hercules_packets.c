#include "hercules_packets.h"
#include "hercules_state_wire.h"

renderer_u16 renderer_packet_capture_size(const RendererDecodedPacket *decoded)
{
    if (decoded == 0 || decoded->minimum_size == 0)
        return 0;
    if (decoded->command == 0x01U)
        return (renderer_u16)sizeof(HrcCommand01WireView);
    return decoded->minimum_size;
}

static renderer_u16 renderer_read_u16(const renderer_u8 *p)
{
    return (renderer_u16)((renderer_u16)p[0] |
                          ((renderer_u16)p[1] << 8));
}

static renderer_u32 renderer_read_u32(const renderer_u8 *p)
{
    return (renderer_u32)p[0] |
           ((renderer_u32)p[1] << 8) |
           ((renderer_u32)p[2] << 16) |
           ((renderer_u32)p[3] << 24);
}

static renderer_i16 renderer_read_i16(const renderer_u8 *p)
{
    renderer_u16 value;
    renderer_i32 signed_value;

    value = renderer_read_u16(p);
    signed_value = (renderer_i32)value;
    if ((value & 0x8000U) != 0)
        signed_value -= 0x10000L;
    return (renderer_i16)signed_value;
}

static void renderer_clear_decoded(RendererDecodedPacket *decoded)
{
    renderer_u8 *p;
    size_t i;

    p = (renderer_u8 *)decoded;
    for (i = 0; i < sizeof(*decoded); ++i)
        p[i] = 0;
}

static void renderer_copy_color(RendererDecodedPacket *decoded,
                                const renderer_u8 *bytes,
                                unsigned index, unsigned offset)
{
    decoded->color[index][0] = bytes[offset + 0];
    decoded->color[index][1] = bytes[offset + 1];
    decoded->color[index][2] = bytes[offset + 2];
}

static void renderer_copy_vertex(RendererDecodedPacket *decoded,
                                 const renderer_u8 *bytes,
                                 unsigned index, unsigned xy_offset,
                                 unsigned uv_offset)
{
    decoded->x[index] = renderer_read_i16(bytes + xy_offset);
    decoded->y[index] = renderer_read_i16(bytes + xy_offset + 2);
    decoded->u[index] = bytes[uv_offset];
    decoded->v[index] = bytes[uv_offset + 1];
}

RendererPacketStatus renderer_order_table_addresses(
    const RendererRawOrderTable *table, renderer_u32 bucket_index,
    renderer_raw32 *bucket_address, renderer_raw32 *next_sentinel_address)
{
    renderer_u32 bucket_offset;
    renderer_u32 sentinel_offset;

    if (table == 0 || bucket_address == 0 || next_sentinel_address == 0)
        return RENDERER_PACKET_INVALID_ARGUMENT;
    if (bucket_index >= table->slot_count)
        return RENDERER_PACKET_RANGE_ERROR;
    if (bucket_index >= 0x3fffffffUL)
        return RENDERER_PACKET_ADDRESS_OVERFLOW;

    bucket_offset = bucket_index * 4UL;
    sentinel_offset = (bucket_index + 1UL) * 4UL;
    if (table->first_slot > (renderer_raw32)0xffffffffUL - sentinel_offset)
        return RENDERER_PACKET_ADDRESS_OVERFLOW;

    *bucket_address = table->first_slot + bucket_offset;
    *next_sentinel_address = table->first_slot + sentinel_offset;
    return RENDERER_PACKET_OK;
}

RendererPacketStatus renderer_decode_packet(
    const renderer_u8 *bytes, size_t length, RendererDecodedPacket *decoded)
{
    renderer_u8 command;
    renderer_u8 normalized;
    unsigned i;

    if (bytes == 0 || decoded == 0)
        return RENDERER_PACKET_INVALID_ARGUMENT;

    renderer_clear_decoded(decoded);
    if (length < 8)
        return RENDERER_PACKET_SHORT_HEADER;

    decoded->next = renderer_read_u32(bytes);
    command = bytes[7];
    normalized = (renderer_u8)(command & (renderer_u8)0xfd);
    decoded->command = command;
    decoded->semitransparent =
        (renderer_u8)((command & 0x02U) != 0);

    if (normalized == 0x24U) {
        decoded->kind = RENDERER_PACKET_FT3;
        decoded->minimum_size = 0x1eU;
        decoded->vertex_count = 3;
        decoded->color_count = 1;
        if (length < decoded->minimum_size)
            return RENDERER_PACKET_TRUNCATED;
        renderer_copy_color(decoded, bytes, 0, 4);
        renderer_copy_vertex(decoded, bytes, 0, 0x08, 0x0c);
        renderer_copy_vertex(decoded, bytes, 1, 0x10, 0x14);
        renderer_copy_vertex(decoded, bytes, 2, 0x18, 0x1c);
        decoded->clut = renderer_read_u16(bytes + 0x0e);
        decoded->tpage = renderer_read_u16(bytes + 0x16);
        return RENDERER_PACKET_OK;
    }

    if (normalized == 0x2cU) {
        static const unsigned xy_offsets[4] = { 0x08, 0x10, 0x18, 0x20 };
        static const unsigned uv_offsets[4] = { 0x0c, 0x14, 0x1c, 0x24 };

        decoded->kind = RENDERER_PACKET_FT4;
        decoded->minimum_size = 0x26U;
        decoded->vertex_count = 4;
        decoded->color_count = 1;
        if (length < decoded->minimum_size)
            return RENDERER_PACKET_TRUNCATED;
        renderer_copy_color(decoded, bytes, 0, 4);
        for (i = 0; i < 4; ++i)
            renderer_copy_vertex(decoded, bytes, i,
                                 xy_offsets[i], uv_offsets[i]);
        decoded->clut = renderer_read_u16(bytes + 0x0e);
        decoded->tpage = renderer_read_u16(bytes + 0x16);
        return RENDERER_PACKET_OK;
    }

    if (normalized == 0x3cU) {
        static const unsigned color_offsets[4] = { 0x04, 0x10, 0x1c, 0x28 };
        static const unsigned xy_offsets[4] = { 0x08, 0x14, 0x20, 0x2c };
        static const unsigned uv_offsets[4] = { 0x0c, 0x18, 0x24, 0x30 };

        decoded->kind = RENDERER_PACKET_GT4;
        decoded->minimum_size = 0x32U;
        decoded->vertex_count = 4;
        decoded->color_count = 1;
        if (length < decoded->minimum_size)
            return RENDERER_PACKET_TRUNCATED;
        for (i = 0; i < 4; ++i) {
            if (i == 0)
                renderer_copy_color(decoded, bytes, i, color_offsets[i]);
            renderer_copy_vertex(decoded, bytes, i,
                                 xy_offsets[i], uv_offsets[i]);
        }
        decoded->clut = renderer_read_u16(bytes + 0x0e);
        decoded->tpage = renderer_read_u16(bytes + 0x1a);
        return RENDERER_PACKET_OK;
    }

    if (normalized == 0x64U) {
        decoded->kind = RENDERER_PACKET_SPRITE_VARIABLE;
        decoded->minimum_size = 0x14U;
        if (length < decoded->minimum_size)
            return RENDERER_PACKET_TRUNCATED;
        decoded->x[0] = renderer_read_i16(bytes + 0x08);
        decoded->y[0] = renderer_read_i16(bytes + 0x0a);
        decoded->u[0] = bytes[0x0c];
        decoded->v[0] = bytes[0x0d];
        decoded->width = renderer_read_u16(bytes + 0x10);
        decoded->height = renderer_read_u16(bytes + 0x12);
        return RENDERER_PACKET_OK;
    }

    if (normalized == 0x74U || normalized == 0x7cU) {
        decoded->kind = (normalized == 0x74U)
            ? RENDERER_PACKET_SPRITE_8 : RENDERER_PACKET_SPRITE_16;
        decoded->minimum_size = 0x0eU;
        if (length < decoded->minimum_size)
            return RENDERER_PACKET_TRUNCATED;
        decoded->x[0] = renderer_read_i16(bytes + 0x08);
        decoded->y[0] = renderer_read_i16(bytes + 0x0a);
        decoded->u[0] = bytes[0x0c];
        decoded->v[0] = bytes[0x0d];
        decoded->width = (normalized == 0x74U) ? 8U : 16U;
        decoded->height = decoded->width;
        return RENDERER_PACKET_OK;
    }

    /* ENG3.DLL 0x22f00 writes linked 0x68 records with a 0x0c producer
     * stride. AC60 only reads their link/tag and, after the shared group-60
     * page-base refresh, skips both raster calls. Do not invent payload fields
     * or treat this route as a side-effect-free decoder operation. */
    if (normalized == 0x68U) {
        decoded->kind = RENDERER_PACKET_OBSERVED_UNDRAWN;
        decoded->minimum_size = 0x08U;
        decoded->producer_tag_observed =
            (renderer_u8)(command == 0x68U);
        return RENDERER_PACKET_KNOWN_UNDRAWN;
    }

    if (command == 0xe1U) {
        decoded->kind = RENDERER_PACKET_E1_MODE;
        decoded->minimum_size = 0x08U;
        if (length < decoded->minimum_size)
            return RENDERER_PACKET_TRUNCATED;
        decoded->mode_payload = renderer_read_u32(bytes + 0x04);
        return RENDERER_PACKET_OK;
    }

    if (command == 0x01U || command == 0xf3U) {
        decoded->kind = RENDERER_PACKET_KNOWN_UNSUPPORTED;
        /* These are known AC60 commands but their payloads are deliberately
         * not interpreted here. Preserve the native direct-access bounds. */
        decoded->minimum_size = (command == 0x01U) ? 0x10U : 0x0cU;
        if (length < decoded->minimum_size)
            return RENDERER_PACKET_TRUNCATED;
        return RENDERER_PACKET_UNSUPPORTED;
    }

    decoded->kind = RENDERER_PACKET_UNKNOWN;
    decoded->minimum_size = 0x08U;
    return RENDERER_PACKET_UNRECOGNIZED;
}
