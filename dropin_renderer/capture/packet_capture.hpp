#ifndef HERCULES_DROPIN_CAPTURE_PACKET_CAPTURE_HPP
#define HERCULES_DROPIN_CAPTURE_PACKET_CAPTURE_HPP

#include <functional>
#include <vector>

#include "event.hpp"

#include "hercules_frame.h"
#include "hercules_state.h"

namespace hercules::dropin {

enum class CaptureStatus {
    ok,
    complete_with_unsupported,
    invalid_argument,
    malformed_packet,
    packet_limit_exceeded,
    event_limit_exceeded,
    callback_stopped,
    callback_failed,
    frame_failed
};

struct CaptureLimits {
    renderer_u32 max_visits = 256;
    renderer_u32 max_events = 768;
    renderer_u16 max_packet_bytes = 0x100;
};

struct CaptureResult {
    CaptureStatus status = CaptureStatus::invalid_argument;
    FrameWalkStatus frame_status = FRAME_WALK_INVALID_ARGUMENT;
    CaptureProvenance provenance = CaptureProvenance::passive_entry_observation;
    std::vector<Event> events;
};

/* The callback is invoked synchronously once for each packet event. It may
 * inspect or mutate the resolver's live memory. When it returns, the shared
 * AC60 walker reloads the current packet link before selecting the next
 * packet. The packet event passed to it owns its bytes. This is an offline,
 * controlled consumer hook; it is not an observed game callback such as BD80. */
using PacketCallback = std::function<bool(const Event &packet_event)>;

/* Captures a bounded entry-time observation. It does not invoke a caller
 * callback and does not run the staging state machine. It is a passive offline
 * snapshot of packets reached by the shared bounded walker. */
CaptureResult observe_entry(
    FrameRaw32Resolver resolver, void *resolver_context,
    renderer_raw32 order_table_base, renderer_u32 slot_count,
    CaptureLimits limits = {});

/* Performs controlled offline consumption: decode and apply each packet with
 * the shared staging helper, publish one owned packet event, synchronously
 * invoke callback, then return to the shared walker so it reloads live links
 * and future packet bytes. No pixels, VRAM, palette, clear, or presentation
 * effects are executed by this driver. */
CaptureResult consume_offline(
    FrameRaw32Resolver resolver, void *resolver_context,
    renderer_raw32 order_table_base, renderer_u32 slot_count,
    RendererAc60StateView initial_state, const PacketCallback &callback,
    CaptureLimits limits = {});

} // namespace hercules::dropin

#endif
