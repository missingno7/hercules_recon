#include <stdio.h>
#include <string.h>
#include "hercules_packets.h"
#include "hercules_state.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

int main(void)
{
    renderer_u8 wire[52];
    RendererDecodedPacket packet;
    HerculesCommand01Move move;
    memset(wire, 0, sizeof(wire));
    CHECK(renderer_packet_capture_size(0) == 0);
    CHECK(renderer_decode_packet(wire, 7, &packet) == RENDERER_PACKET_SHORT_HEADER);
    CHECK(renderer_packet_capture_size(&packet) == 0);
    wire[7] = 0x01;
    CHECK(renderer_decode_packet(wire, 8, &packet) == RENDERER_PACKET_TRUNCATED);
    CHECK(packet.minimum_size == 16);
    CHECK(renderer_packet_capture_size(&packet) == 24);
    CHECK(renderer_decode_packet(wire, 16, &packet) == RENDERER_PACKET_UNSUPPORTED);
    CHECK(hercules_state_decode_command01(wire, 16, &move) == HERCULES_STATE_TRUNCATED);
    wire[22] = 0xfe; wire[23] = 0xff;
    CHECK(renderer_decode_packet(wire, 24, &packet) == RENDERER_PACKET_UNSUPPORTED);
    CHECK(renderer_packet_capture_size(&packet) == 24);
    CHECK(hercules_state_decode_command01(wire, 24, &move) == HERCULES_STATE_OK);
    CHECK(move.height == -2);
    wire[7] = 0xf3;
    CHECK(renderer_decode_packet(wire, 8, &packet) == RENDERER_PACKET_TRUNCATED);
    CHECK(renderer_packet_capture_size(&packet) == 12);
    wire[7] = 0x2c;
    CHECK(renderer_decode_packet(wire, 8, &packet) == RENDERER_PACKET_TRUNCATED);
    CHECK(renderer_packet_capture_size(&packet) == 38);
    wire[7] = 0x3c;
    CHECK(renderer_decode_packet(wire, 8, &packet) == RENDERER_PACKET_TRUNCATED);
    CHECK(renderer_packet_capture_size(&packet) == 50);
    wire[7] = 0x55;
    CHECK(renderer_decode_packet(wire, 8, &packet) == RENDERER_PACKET_UNRECOGNIZED);
    CHECK(renderer_packet_capture_size(&packet) == 8);
    puts("capture extent: direct envelope and transitive rectangle preserved");
    return 0;
}
