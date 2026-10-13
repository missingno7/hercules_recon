#include <stddef.h>
#include <stdio.h>
#include "hercules_state.h"

typedef char check_e1_size[(sizeof(RendererE1ProducerPacket) == 12) ? 1 : -1];
typedef char check_e1_payload[(offsetof(RendererE1ProducerPacket, mode_payload) == 4) ? 1 : -1];
typedef char check_e1_extension[(offsetof(RendererE1ProducerPacket, producer_tail) == 8) ? 1 : -1];
typedef char check_f3_size[(sizeof(HrcF3WireView) == 12) ? 1 : -1];
typedef char check_f3_x[(offsetof(HrcF3WireView, x) == 4) ? 1 : -1];
typedef char check_f3_y[(offsetof(HrcF3WireView, encoded_y) == 6) ? 1 : -1];
typedef char check_f3_tag[(offsetof(HrcF3WireView, tag) == 7) ? 1 : -1];
typedef char check_f3_right[(offsetof(HrcF3WireView, right_delta) == 8) ? 1 : -1];
typedef char check_f3_bottom[(offsetof(HrcF3WireView, bottom_delta) == 10) ? 1 : -1];
typedef char check_move_size[(sizeof(HrcCommand01WireView) == 24) ? 1 : -1];
typedef char check_move_tag[(offsetof(HrcCommand01WireView, tag) == 7) ? 1 : -1];
typedef char check_move_destination[(offsetof(HrcCommand01WireView, destination_x) == 12) ? 1 : -1];
typedef char check_move_source[(offsetof(HrcCommand01WireView, source_x) == 16) ? 1 : -1];
typedef char check_move_height[(offsetof(HrcCommand01WireView, height) == 22) ? 1 : -1];

static int checks;
static int failures;

static void check(int condition, const char *label)
{
    checks++;
    if (!condition) {
        failures++;
        printf("FAIL %s\n", label);
    }
}

static void put_u16(renderer_u8 *bytes, unsigned int offset, renderer_u16 value)
{
    bytes[offset] = (renderer_u8)(value & 0xFFU);
    bytes[offset + 1] = (renderer_u8)((value >> 8) & 0xFFU);
}

static void put_u32(renderer_u8 *bytes, unsigned int offset, renderer_u32 value)
{
    bytes[offset] = (renderer_u8)(value & 0xFFU);
    bytes[offset + 1] = (renderer_u8)((value >> 8) & 0xFFU);
    bytes[offset + 2] = (renderer_u8)((value >> 16) & 0xFFU);
    bytes[offset + 3] = (renderer_u8)((value >> 24) & 0xFFU);
}

static void put_i16(renderer_u8 *bytes, unsigned int offset, renderer_i32 value)
{
    put_u16(bytes, offset, (renderer_u16)value);
}

static void test_e1(void)
{
    renderer_u8 wire[12];
    HerculesE1StateUpdate decoded;
    HerculesStateStatus status;

    put_u32(wire, 4, 0x000001B5UL);
    wire[7] = 0xE1;
    status = hercules_state_decode_e1(wire, 8, &decoded);
    check(status == HERCULES_STATE_OK, "E1 decodes the eight-byte AC60 prefix");
    check(decoded.page_index == 0x15U, "E1 extracts low five page bits");
    check(decoded.table_selector == 1U, "E1 extracts bits five and six");
    check(decoded.alternate_bank == 1U, "E1 requires both 0x80 and 0x100 bits");

    wire[4] = 0xFF;
    wire[5] = 0xFF;
    wire[6] = 0xFF;
    wire[7] = 0xE1;
    status = hercules_state_decode_e1(wire, 8, &decoded);
    check(status == HERCULES_STATE_OK, "E1 ignores high payload bits replaced by the tag");
    check(decoded.page_index == 31U && decoded.table_selector == 3U &&
          decoded.alternate_bank == 1U, "E1 masks retain only the three specified state fields");

    wire[4] = 0x80;
    wire[5] = 0;
    wire[6] = 0;
    wire[7] = 0xE1;
    status = hercules_state_decode_e1(wire, 8, &decoded);
    check(status == HERCULES_STATE_OK && decoded.alternate_bank == 0U,
          "E1 bit 0x80 alone does not select the alternate bank");

    decoded.page_index = 99;
    decoded.table_selector = 88;
    decoded.alternate_bank = 77;
    status = hercules_state_decode_e1(wire, 7, &decoded);
    check(status == HERCULES_STATE_TRUNCATED, "E1 rejects a short prefix");
    check(decoded.page_index == 99 && decoded.table_selector == 88 &&
          decoded.alternate_bank == 77, "short E1 leaves output unchanged");
    wire[7] = 0;
    status = hercules_state_decode_e1(wire, 8, &decoded);
    check(status == HERCULES_STATE_WRONG_TAG, "E1 rejects the wrong command byte");
    check(decoded.page_index == 99 && decoded.table_selector == 88 &&
          decoded.alternate_bank == 77, "wrong-tag E1 leaves output unchanged");
}

static void make_f3(renderer_u8 *wire, renderer_i32 x, renderer_u8 encoded_y,
                    renderer_i32 dx, renderer_i32 dy)
{
    put_i16(wire, 4, x);
    wire[6] = encoded_y;
    wire[7] = 0xF3;
    put_i16(wire, 8, dx);
    put_i16(wire, 10, dy);
}

static void test_f3(void)
{
    renderer_u8 wire[12];
    HerculesF3ClipBounds decoded;
    HerculesStateStatus status;
    unsigned int y;

    make_f3(wire, -5, 0, 10, 0);
    status = hercules_state_decode_f3(wire, 12, &decoded);
    check(status == HERCULES_STATE_OK, "F3 decodes its 12-byte inline payload");
    check(decoded.left == 0 && decoded.right == 5,
          "F3 computes right from unclamped signed x before clamping");

    for (y = 0xF0U; y <= 0xFFU; y++) {
        make_f3(wire, 0, (renderer_u8)y, 0, 0);
        status = hercules_state_decode_f3(wire, 12, &decoded);
        check(status == HERCULES_STATE_OK && decoded.top == (renderer_i32)(y - 0xF0U),
              "F3 subtracts 0xF0 once for each encoded y in 240..255");
    }

    make_f3(wire, 0, 239, 0, 10);
    status = hercules_state_decode_f3(wire, 12, &decoded);
    check(status == HERCULES_STATE_OK && decoded.top == 239 && decoded.bottom == 240,
          "F3 leaves y=239 unchanged and clamps bottom to 240");

    make_f3(wire, 32767, 255, 32767, -20);
    status = hercules_state_decode_f3(wire, 12, &decoded);
    check(status == HERCULES_STATE_OK && decoded.left == 320 && decoded.right == 320 &&
          decoded.top == 15 && decoded.bottom == 0,
          "F3 sign-extends deltas and independently clamps all bounds");

    make_f3(wire, -32768, 240, -32768, 1);
    status = hercules_state_decode_f3(wire, 12, &decoded);
    check(status == HERCULES_STATE_OK && decoded.left == 0 && decoded.top == 0 &&
          decoded.right == 0 && decoded.bottom == 1,
          "F3 handles signed minima and encoded y=240");

    decoded.left = 11;
    decoded.top = 22;
    decoded.right = 33;
    decoded.bottom = 44;
    status = hercules_state_decode_f3(wire, 11, &decoded);
    check(status == HERCULES_STATE_TRUNCATED, "F3 rejects an 11-byte payload");
    check(decoded.left == 11 && decoded.top == 22 && decoded.right == 33 &&
          decoded.bottom == 44, "short F3 leaves output unchanged");
    wire[7] = 0;
    status = hercules_state_decode_f3(wire, 12, &decoded);
    check(status == HERCULES_STATE_WRONG_TAG, "F3 rejects the wrong command byte");
    check(decoded.left == 11 && decoded.top == 22 && decoded.right == 33 &&
          decoded.bottom == 44, "wrong-tag F3 leaves output unchanged");
}

static void test_command01(void)
{
    renderer_u8 wire[24];
    HerculesCommand01Move decoded;
    HerculesStateStatus status;

    put_i16(wire, 12, -320);
    put_i16(wire, 14, 240);
    put_i16(wire, 16, -17);
    put_i16(wire, 18, 255);
    put_i16(wire, 20, 64);
    put_i16(wire, 22, -2);
    wire[7] = 0x01;
    status = hercules_state_decode_command01(wire, 24, &decoded);
    check(status == HERCULES_STATE_OK, "command 01 decodes the embedded A8F0 rectangle");
    check(decoded.destination_x == -320 && decoded.destination_y == 240,
          "command 01 sign-extends destination coordinates in AC60 order");
    check(decoded.source_x == -17 && decoded.source_y == 255 &&
          decoded.width == 64 && decoded.height == -2,
          "command 01 reads four signed WORDs from packet+0x10");

    decoded.destination_x = 11;
    decoded.destination_y = 22;
    decoded.source_x = 33;
    decoded.source_y = 44;
    decoded.width = 55;
    decoded.height = 66;
    status = hercules_state_decode_command01(wire, 23, &decoded);
    check(status == HERCULES_STATE_TRUNCATED, "command 01 requires the 0x18 reachable extent");
    check(decoded.destination_x == 11 && decoded.destination_y == 22 &&
          decoded.source_x == 33 && decoded.source_y == 44 &&
          decoded.width == 55 && decoded.height == 66,
          "short command 01 leaves output unchanged");
    wire[7] = 0;
    status = hercules_state_decode_command01(wire, 24, &decoded);
    check(status == HERCULES_STATE_WRONG_TAG, "command 01 rejects the wrong command byte");
    check(decoded.destination_x == 11 && decoded.destination_y == 22 &&
          decoded.source_x == 33 && decoded.source_y == 44 &&
          decoded.width == 55 && decoded.height == 66,
          "wrong-tag command 01 leaves output unchanged");
}

int main(void)
{
    test_e1();
    test_f3();
    test_command01();
    printf("hercules_state_fixture checks=%d failures=%d\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
