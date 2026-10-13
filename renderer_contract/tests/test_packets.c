#include <stdio.h>
#include <string.h>
#include "hercules_packets.h"

static unsigned checks;
static unsigned failures;

static void expect(int condition, const char *name)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", name);
    }
}

static void put16(renderer_u8 *p, unsigned offset, renderer_u16 value)
{
    p[offset] = (renderer_u8)value;
    p[offset + 1] = (renderer_u8)(value >> 8);
}

static void put32(renderer_u8 *p, unsigned offset, renderer_u32 value)
{
    p[offset] = (renderer_u8)value;
    p[offset + 1] = (renderer_u8)(value >> 8);
    p[offset + 2] = (renderer_u8)(value >> 16);
    p[offset + 3] = (renderer_u8)(value >> 24);
}

static void header(renderer_u8 *p, size_t length, renderer_u32 next,
                   renderer_u8 command)
{
    memset(p, 0, length);
    put32(p, 0, next);
    p[4] = 0x11;
    p[5] = 0x22;
    p[6] = 0x33;
    p[7] = command;
}

static void test_ft4(void)
{
    renderer_u8 bytes[0x2a];
    RendererDecodedPacket out;
    RendererPacketStatus status;

    header(bytes, sizeof(bytes), 0x12345678UL, 0x2e);
    put16(bytes, 0x08, (renderer_u16)-2);
    put16(bytes, 0x0a, 5);
    bytes[0x0c] = 10; bytes[0x0d] = 20; put16(bytes, 0x0e, 0x3142);
    put16(bytes, 0x10, 30); put16(bytes, 0x12, (renderer_u16)-40);
    bytes[0x14] = 50; bytes[0x15] = 60; put16(bytes, 0x16, 0x9c02);
    put16(bytes, 0x18, 70); put16(bytes, 0x1a, 80);
    bytes[0x1c] = 90; bytes[0x1d] = 100;
    put16(bytes, 0x20, 110); put16(bytes, 0x22, 120);
    bytes[0x24] = 130; bytes[0x25] = 140;

    status = renderer_decode_packet(bytes, 0x26, &out);
    expect(status == RENDERER_PACKET_OK, "FT4 minimum accepted");
    expect(out.kind == RENDERER_PACKET_FT4 && out.minimum_size == 0x26,
           "FT4 family and consumer minimum");
    expect(out.next == 0x12345678UL && out.semitransparent == 1,
           "FT4 raw link and blend bit");
    expect(out.command == 0x2e && out.color[0][0] == 0x11 &&
           out.color[0][2] == 0x33, "FT4 consumed RGB lane and command");
    expect(out.x[0] == -2 && out.y[0] == 5 &&
           out.x[3] == 110 && out.y[3] == 120, "FT4 signed XY");
    expect(out.u[0] == 10 && out.v[3] == 140 &&
           out.clut == 0x3142 && out.tpage == 0x9c02,
           "FT4 UV and texture words");
    bytes[0x26] = 0xa5; bytes[0x27] = 0x5a;
    bytes[0x28] = 0xc3; bytes[0x29] = 0x3c;
    expect(renderer_decode_packet(bytes, 0x25, &out) ==
           RENDERER_PACKET_TRUNCATED, "FT4 truncated before last consumer byte");
    expect(bytes[0x26] == 0xa5 && bytes[0x27] == 0x5a &&
           bytes[0x28] == 0xc3 && bytes[0x29] == 0x3c,
           "FT4 input canary unchanged");
}

static void test_gt4(void)
{
    renderer_u8 bytes[0x36];
    RendererDecodedPacket out;

    header(bytes, sizeof(bytes), 0xfeed0123UL, 0x3c);
    put16(bytes, 0x08, 1); put16(bytes, 0x0a, 2);
    bytes[0x0c] = 3; bytes[0x0d] = 4; put16(bytes, 0x0e, 0x1112);
    bytes[0x10] = 21; bytes[0x11] = 22; bytes[0x12] = 23;
    put16(bytes, 0x14, 5); put16(bytes, 0x16, 6);
    bytes[0x18] = 7; bytes[0x19] = 8; put16(bytes, 0x1a, 0x9e00);
    bytes[0x1c] = 31; bytes[0x1d] = 32; bytes[0x1e] = 33;
    put16(bytes, 0x20, 9); put16(bytes, 0x22, 10);
    bytes[0x24] = 11; bytes[0x25] = 12;
    bytes[0x28] = 41; bytes[0x29] = 42; bytes[0x2a] = 43;
    put16(bytes, 0x2c, 13); put16(bytes, 0x2e, 14);
    bytes[0x30] = 15; bytes[0x31] = 16;

    expect(renderer_decode_packet(bytes, 0x32, &out) == RENDERER_PACKET_OK,
           "GT4 minimum accepted");
    expect(out.kind == RENDERER_PACKET_GT4 && out.color_count == 1 &&
           out.semitransparent == 0, "GT4 family and color count");
    expect(out.color_count == 1 && out.color[0][0] == 0x11 &&
           out.color[1][0] == 0 && out.color[2][2] == 0 &&
           out.color[3][0] == 0, "GT4 ignores later producer color lanes");
    expect(out.x[3] == 13 && out.y[3] == 14 &&
           out.u[3] == 15 && out.v[3] == 16 &&
           out.clut == 0x1112 && out.tpage == 0x9e00,
           "GT4 last vertex and texture words");
    bytes[0x32] = 0xa5; bytes[0x33] = 0x5a;
    bytes[0x34] = 0xc3; bytes[0x35] = 0x3c;
    expect(renderer_decode_packet(bytes, 0x31, &out) ==
           RENDERER_PACKET_TRUNCATED, "GT4 missing last consumer byte");
    expect(bytes[0x32] == 0xa5 && bytes[0x33] == 0x5a &&
           bytes[0x34] == 0xc3 && bytes[0x35] == 0x3c,
           "GT4 input canary unchanged");
}

static void test_ft3_and_sprites(void)
{
    renderer_u8 bytes[40];
    RendererDecodedPacket out;

    header(bytes, sizeof(bytes), 0x10002000UL, 0x24);
    put16(bytes, 8, 0xfffe); put16(bytes, 10, 0x0033);
    bytes[12] = 0x12; bytes[13] = 0x34; put16(bytes, 14, 0x5678);
    put16(bytes, 16, 40); put16(bytes, 18, 50);
    bytes[20] = 0x56; bytes[21] = 0x78; put16(bytes, 22, 0x9abc);
    put16(bytes, 24, 60); put16(bytes, 26, 70);
    bytes[28] = 0x9a; bytes[29] = 0xbc;
    expect(renderer_decode_packet(bytes, 0x1e, &out) == RENDERER_PACKET_OK,
           "FT3 minimum accepted");
    expect(out.kind == RENDERER_PACKET_FT3 && out.x[0] == -2 &&
           out.x[2] == 60 && out.u[2] == 0x9a, "FT3 fields");

    header(bytes, sizeof(bytes), 0xabcddcbaUL, 0x64);
    put16(bytes, 8, (renderer_u16)-8); put16(bytes, 10, 12);
    bytes[12] = 16; bytes[13] = 24; put16(bytes, 14, 0x4567);
    put16(bytes, 16, 32); put16(bytes, 18, 48);
    expect(renderer_decode_packet(bytes, 0x14, &out) == RENDERER_PACKET_OK,
           "variable sprite minimum accepted");
    expect(out.kind == RENDERER_PACKET_SPRITE_VARIABLE &&
           out.x[0] == -8 && out.width == 32 && out.height == 48 &&
           out.clut == 0 && out.color_count == 0 &&
           out.color[0][0] == 0 && out.color[0][2] == 0,
           "variable sprite fields; producer word and color are not consumed");
    expect(renderer_decode_packet(bytes, 0x13, &out) ==
           RENDERER_PACKET_TRUNCATED, "variable sprite truncation");

    header(bytes, sizeof(bytes), 0, 0x76);
    put16(bytes, 8, 100); put16(bytes, 10, 200);
    bytes[12] = 7; bytes[13] = 9;
    expect(renderer_decode_packet(bytes, 0x0e, &out) == RENDERER_PACKET_OK &&
           out.kind == RENDERER_PACKET_SPRITE_8 &&
           out.width == 8 && out.height == 8 && out.semitransparent == 1 &&
           out.color_count == 0 && out.color[0][0] == 0 && out.clut == 0,
           "fixed 8 sprite uses state color/page and semi flag");
    header(bytes, sizeof(bytes), 0, 0x7c);
    expect(renderer_decode_packet(bytes, 0x0d, &out) ==
           RENDERER_PACKET_TRUNCATED, "fixed sprite truncation");
    expect(renderer_decode_packet(bytes, 0x0e, &out) == RENDERER_PACKET_OK &&
           out.kind == RENDERER_PACKET_SPRITE_16 &&
           out.width == 16 && out.height == 16 && out.color_count == 0 &&
           out.color[0][0] == 0 && out.clut == 0,
           "fixed 16 sprite uses state color/page");
}

static void test_e1_unknown_and_unsupported(void)
{
    renderer_u8 bytes[16];
    RendererDecodedPacket out;

    header(bytes, sizeof(bytes), 0x77889900UL, 0xe1);
    put32(bytes, 4, 0xe1000180UL);
    put32(bytes, 8, 0x11223344UL);
    expect(renderer_decode_packet(bytes, 8, &out) == RENDERER_PACKET_OK &&
           out.kind == RENDERER_PACKET_E1_MODE &&
           out.mode_payload == 0xe1000180UL, "E1 minimum and raw DWORD");
    expect(renderer_decode_packet(bytes, 7, &out) ==
           RENDERER_PACKET_SHORT_HEADER, "short common header");

    header(bytes, sizeof(bytes), 0, 0x01);
    expect(renderer_decode_packet(bytes, 8, &out) ==
           RENDERER_PACKET_TRUNCATED && out.minimum_size == 0x10 &&
           out.kind == RENDERER_PACKET_KNOWN_UNSUPPORTED,
           "known move command direct-access bound");
    expect(renderer_decode_packet(bytes, 0x0f, &out) ==
           RENDERER_PACKET_TRUNCATED && out.minimum_size == 0x10,
           "known move command one byte short");
    expect(renderer_decode_packet(bytes, 0x10, &out) ==
           RENDERER_PACKET_UNSUPPORTED &&
           out.kind == RENDERER_PACKET_KNOWN_UNSUPPORTED,
           "known opcode reported unsupported");
    header(bytes, sizeof(bytes), 0, 0xf3);
    expect(renderer_decode_packet(bytes, 0x0b, &out) ==
           RENDERER_PACKET_TRUNCATED && out.minimum_size == 0x0c,
           "known rectangle command direct-access bound");
    expect(renderer_decode_packet(bytes, 0x0c, &out) ==
           RENDERER_PACKET_UNSUPPORTED,
           "known rectangle command unsupported");
    header(bytes, sizeof(bytes), 0, 0x55);
    expect(renderer_decode_packet(bytes, 8, &out) ==
           RENDERER_PACKET_UNRECOGNIZED &&
           out.kind == RENDERER_PACKET_UNKNOWN, "unknown tag explicit");
    expect(renderer_decode_packet(0, 8, &out) ==
           RENDERER_PACKET_INVALID_ARGUMENT, "null input rejected");
}

static void test_observed_68_route(void)
{
    renderer_u8 bytes[9];
    RendererDecodedPacket out;
    RendererPacketStatus status;

    header(bytes, sizeof(bytes), 0x10203040UL, 0x68);
    bytes[8] = 0xa5;
    status = renderer_decode_packet(bytes, 8, &out);
    expect(status == RENDERER_PACKET_KNOWN_UNDRAWN &&
           out.kind == RENDERER_PACKET_OBSERVED_UNDRAWN &&
           out.minimum_size == 8 && out.command == 0x68,
           "producer-observed 0x68 has bounded recognized header route");
    expect(out.producer_tag_observed == 1 && out.next == 0x10203040UL &&
           out.vertex_count == 0 && out.color_count == 0 && out.clut == 0 &&
           out.tpage == 0 && out.width == 0 && out.height == 0 &&
           bytes[8] == 0xa5,
           "0x68 preserves link, invents no payload, and respects span");
    expect(renderer_decode_packet(bytes, 7, &out) ==
           RENDERER_PACKET_SHORT_HEADER,
           "0x68 cannot read tag from a seven-byte span");

    header(bytes, sizeof(bytes), 0x50607080UL, 0x6a);
    bytes[8] = 0x5a;
    expect(renderer_decode_packet(bytes, 8, &out) ==
           RENDERER_PACKET_KNOWN_UNDRAWN &&
           out.kind == RENDERER_PACKET_OBSERVED_UNDRAWN &&
           out.command == 0x6a && out.producer_tag_observed == 0 &&
           out.next == 0x50607080UL && bytes[8] == 0x5a,
           "0x6a shares normalized consumer route but is not producer-observed");

    header(bytes, sizeof(bytes), 0, 0x69);
    expect(renderer_decode_packet(bytes, 8, &out) ==
           RENDERER_PACKET_UNRECOGNIZED &&
           out.kind == RENDERER_PACKET_UNKNOWN,
           "nearby 0x69 remains explicitly unknown");
    header(bytes, sizeof(bytes), 0, 0x70);
    expect(renderer_decode_packet(bytes, 8, &out) ==
           RENDERER_PACKET_UNRECOGNIZED &&
           out.kind == RENDERER_PACKET_UNKNOWN,
           "unverified 0x70 remains explicitly unknown");
}

static void test_raw_order_table_addresses(void)
{
    RendererRawOrderTable table;
    renderer_raw32 bucket;
    renderer_raw32 sentinel;

    table.first_slot = 0x1000UL;
    table.slot_count = 3;
    expect(renderer_order_table_addresses(&table, 0, &bucket, &sentinel) ==
           RENDERER_PACKET_OK && bucket == 0x1000UL && sentinel == 0x1004UL,
           "first bucket and next-slot sentinel");
    expect(renderer_order_table_addresses(&table, 2, &bucket, &sentinel) ==
           RENDERER_PACKET_OK && bucket == 0x1008UL && sentinel == 0x100cUL,
           "last bucket computes one-past sentinel without dereferencing");
    expect(renderer_order_table_addresses(&table, 3, &bucket, &sentinel) ==
           RENDERER_PACKET_RANGE_ERROR, "bucket index outside descriptor");
    table.first_slot = 0xfffffffcUL;
    table.slot_count = 1;
    expect(renderer_order_table_addresses(&table, 0, &bucket, &sentinel) ==
           RENDERER_PACKET_ADDRESS_OVERFLOW,
           "one-past sentinel address overflow rejected");
    expect(renderer_order_table_addresses(0, 0, &bucket, &sentinel) ==
           RENDERER_PACKET_INVALID_ARGUMENT, "null table descriptor rejected");
    expect(renderer_order_table_addresses(&table, 0, 0, &sentinel) ==
           RENDERER_PACKET_INVALID_ARGUMENT, "null bucket output rejected");
}

int main(void)
{
    test_ft4();
    test_gt4();
    test_ft3_and_sprites();
    test_e1_unknown_and_unsupported();
    test_observed_68_route();
    test_raw_order_table_addresses();
    printf("renderer_packet_fixture checks=%u failures=%u\n",
           checks, failures);
    return failures != 0;
}
