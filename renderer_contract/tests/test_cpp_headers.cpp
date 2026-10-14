#if defined(_M_IX86) || defined(__i386__)
#define HERCULES_RENDERER_LIVE_X86 1
#endif
#include "hercules_host_abi.h"
#include "hercules_cif.h"
#include "eng1_geometry_association_observer.h"
#include "hercules_surface.h"
#include "hercules_packets.h"
#include "hercules_frame.h"
#include "hercules_textures.h"
#include "hercules_state.h"
#include "hercules_pixels.h"
#include "hercules_sprite_geometry.h"
#include "hercules_sprite_raster.h"
#include "hercules_quad_dispatch.h"
#include "hercules_staging.h"
#include "hercules_edge_steps.h"
#include "hercules_packed_uv.h"
#include "hercules_route_uv_models.h"

/* Link against C objects to check the reusable C++ consumer boundary. */
int main()
{
    HerculesHostSurfaceViewSnapshot surface_view;
    renderer_u8 wire[8] = {0,0,0,0,0x83,1,0,0xe1};
    HerculesE1StateUpdate state;
    RendererDecodedPacket packet;
    RendererD60CPackedInputs packed_inputs;
    RendererD60CPackedState packed_state;
    RendererD60CPackedStep packed_step;
    renderer_u32 offset = 0;
    if (sizeof(renderer_raw32) != 4)
        return 1;
    surface_view.active_pixel_address = 0x12345678u;
    surface_view.pitch_bytes = 336;
    surface_view.lock_held = 1;
    surface_view.directdraw_mode = 1;
    if (surface_view.active_pixel_address != 0x12345678u ||
        surface_view.pitch_bytes != 336 || surface_view.lock_held != 1 ||
        surface_view.directdraw_mode != 1 ||
        sizeof(surface_view.directdraw_mode) != 1)
        return 9;
#ifdef HERCULES_RENDERER_LIVE_X86
    if (sizeof(HerculesHostSurfaceLockService) != 4 ||
        sizeof(HerculesHostSurfaceUnlockService) != 4 ||
        sizeof(HerculesHostSurfacePresentService) != 4)
        return 10;
#endif
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
    packed_inputs.u_start = 0;
    packed_inputs.v_start = 0x00019b4du;
    packed_inputs.u_step = 0;
    packed_inputs.v_step = 0xffffff32u;
    packed_inputs.destination = 0x10020140u;
    packed_inputs.texture_base = 0x10001000u;
    packed_inputs.visible_count = 319;
    if (renderer_d60c_packed_init(&packed_inputs, &packed_state) !=
        RENDERER_D60C_PACKED_OK)
        return 9;
    if (renderer_d60c_packed_advance(&packed_state, 255, &packed_step) !=
        RENDERER_D60C_PACKED_OK || packed_step.store_required != 1 ||
        packed_state.completed_count != 1)
        return 10;
    RendererRouteUvStep route_step;
    if (renderer_e680_mode80_uv_step(0, 0, &route_step) !=
        RENDERER_ROUTE_UV_INVALID_ARGUMENT)
        return 11;
    return 0;
}
