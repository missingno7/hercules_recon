#ifndef HERCULES_DROPIN_CORE_EVENT_HPP
#define HERCULES_DROPIN_CORE_EVENT_HPP

#include <cstdint>
#include <vector>

#include "hercules_packets.h"
#include "hercules_staging.h"
#include "hercules_state.h"

namespace hercules::dropin {

enum class CaptureProvenance : std::uint8_t {
    passive_entry_observation,
    controlled_offline_consumption
};

/* These are diagnostic categories, not drawing instructions. Packet capture
 * emits only categories with a decoded or staging-backed observation. */
enum class EventKind : std::uint8_t {
    polygon,
    sprite,
    geometry_state,
    texture_reference,
    palette_reference,
    texture_mutation,
    palette_mutation,
    vram_move_request,
    callback_boundary,
    surface_clear,
    present_request,
    unsupported
};

enum class EventOrigin : std::uint8_t {
    packet_decode,
    shared_staging_model,
    controlled_offline_callback,
    external_observation
};

/* A pointer-free diagnostic record. Packet events own the exact byte span
 * needed by the shared decoder/transitive command view; raw_address remains a
 * target token and is never cast to a host pointer. */
struct Event {
    EventKind kind = EventKind::unsupported;
    EventOrigin origin = EventOrigin::packet_decode;
    CaptureProvenance provenance = CaptureProvenance::passive_entry_observation;
    renderer_raw32 raw_address = 0;
    renderer_raw32 related_packet = 0;
    std::uint32_t sequence = 0;
    std::uint8_t command = 0;
    RendererPacketKind packet_kind = RENDERER_PACKET_UNKNOWN;
    RendererPacketStatus decode_status = RENDERER_PACKET_INVALID_ARGUMENT;
    RendererAc60ApplyStatus staging_status = RENDERER_AC60_APPLY_INVALID_ARGUMENT;
    bool has_staging_result = false;
    RendererDecodedPacket decoded{};
    RendererAc60Event staged{};
    RendererAc60StateView state_after{};
    bool has_state_after = false;
    HerculesStateStatus payload_status = HERCULES_STATE_INVALID_ARGUMENT;
    HerculesE1StateUpdate e1_state{};
    bool has_e1_state = false;
    HerculesF3ClipBounds clip_bounds{};
    bool has_clip_bounds = false;
    HerculesCommand01Move deferred_move{};
    bool has_deferred_move = false;
    std::vector<renderer_u8> owned_packet;
};

} // namespace hercules::dropin

#endif
