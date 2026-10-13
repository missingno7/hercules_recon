#include "hercules_host_abi.h"
#include "hercules_packets.h"
#include "hercules_frame.h"
#include "hercules_textures.h"
#include "hercules_state.h"
#include "hercules_pixels.h"
#include "hercules_sprite_geometry.h"
#include "hercules_quad_dispatch.h"
#include "hercules_staging.h"
#include "hercules_edge_steps.h"

/* Link against C objects to check the reusable C++ consumer boundary. */
int main()
{
    renderer_u8 wire[8] = {0,0,0,0,0x83,1,0,0xe1};
    HerculesE1StateUpdate state;
    RendererDecodedPacket packet;
    renderer_u32 offset = 0;
    if (sizeof(renderer_raw32) != 4)
        return 1;
    if (renderer_decode_packet(wire,sizeof(wire),&packet) != RENDERER_PACKET_OK)
        return 2;
    if (hercules_state_decode_e1(wire,sizeof(wire),&state) != HERCULES_STATE_OK)
        return 3;
    if (state.page_index != 3 || state.table_selector != 0 ||
        state.alternate_bank != 1)
        return 4;
    if (!herc_tex_upload_byte_offset(5,224,0,&offset) || offset != 0xe00a)
        return 5;
    RendererAc60StateView staging = {0};
    RendererAc60Event event;
    if (renderer_ac60_apply_packet(wire,sizeof(wire),&staging,&event) !=
        RENDERER_AC60_APPLY_OK || event.kind != RENDERER_AC60_EVENT_E1_STATE)
        return 6;
    if (renderer_edge_reciprocal_entry(2047) != 0x80100u)
        return 7;
    if (!herc_tex_upload_high_y_byte_address(0,512,0,&offset) ||
        offset != 0x00a6e6e0u)
        return 8;
    return 0;
}
