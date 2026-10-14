#include "packet_capture.hpp"

#include <algorithm>
#include <new>
#include <utility>

namespace hercules::dropin {
namespace {

struct WalkContext {
    FrameRaw32Resolver resolver = nullptr;
    void *resolver_context = nullptr;
    CaptureLimits limits{};
    CaptureResult *result = nullptr;
    const PacketCallback *callback = nullptr;
    RendererAc60StateView state{};
    bool controlled = false;
    bool unsupported_seen = false;
};

bool read_bytes(WalkContext &context, renderer_raw32 address,
                renderer_u32 length, const renderer_u8 **bytes)
{
    return context.resolver != nullptr && bytes != nullptr && address != 0 &&
           context.resolver(context.resolver_context, address, length, bytes) != 0 &&
           *bytes != nullptr;
}

bool append_event(WalkContext &context, Event &&event)
{
    if (context.result == nullptr ||
        context.result->events.size() >= context.limits.max_events) {
        if (context.result != nullptr)
            context.result->status = CaptureStatus::event_limit_exceeded;
        return false;
    }
    event.sequence = static_cast<std::uint32_t>(context.result->events.size());
    context.result->events.emplace_back(std::move(event));
    return true;
}

Event base_packet_event(const WalkContext &context, renderer_raw32 address,
                        const RendererDecodedPacket &decoded,
                        RendererPacketStatus decode_status,
                        std::vector<renderer_u8> owned)
{
    Event event{};
    event.provenance = context.controlled
        ? CaptureProvenance::controlled_offline_consumption
        : CaptureProvenance::passive_entry_observation;
    event.origin = context.controlled ? EventOrigin::shared_staging_model
                                      : EventOrigin::packet_decode;
    event.raw_address = address;
    event.command = decoded.command;
    event.packet_kind = decoded.kind;
    event.decode_status = decode_status;
    event.decoded = decoded;
    event.owned_packet = std::move(owned);
    return event;
}

EventKind classify_passive(const RendererDecodedPacket &decoded,
                           RendererPacketStatus decode_status)
{
    switch (decoded.kind) {
    case RENDERER_PACKET_FT3:
    case RENDERER_PACKET_FT4:
    case RENDERER_PACKET_GT4:
        if (decode_status == RENDERER_PACKET_OK)
            return EventKind::polygon;
        break;
    case RENDERER_PACKET_SPRITE_VARIABLE:
    case RENDERER_PACKET_SPRITE_8:
    case RENDERER_PACKET_SPRITE_16:
        if (decode_status == RENDERER_PACKET_OK)
            return EventKind::sprite;
        break;
    case RENDERER_PACKET_E1_MODE:
        if (decode_status == RENDERER_PACKET_OK)
            return EventKind::geometry_state;
        break;
    case RENDERER_PACKET_KNOWN_UNSUPPORTED:
        if (decode_status == RENDERER_PACKET_UNSUPPORTED &&
            decoded.command == 0xf3U)
            return EventKind::geometry_state;
        if (decode_status == RENDERER_PACKET_UNSUPPORTED &&
            decoded.command == 0x01U)
            return EventKind::vram_move_request;
        break;
    case RENDERER_PACKET_OBSERVED_UNDRAWN:
        return EventKind::unsupported;
    case RENDERER_PACKET_UNKNOWN:
    default:
        break;
    }
    return EventKind::unsupported;
}

EventKind classify_staged(const RendererAc60Event &staged,
                          RendererAc60ApplyStatus status)
{
    if (status == RENDERER_AC60_APPLY_DEFERRED &&
        staged.kind == RENDERER_AC60_EVENT_DEFERRED_COPY)
        return EventKind::vram_move_request;
    switch (staged.kind) {
    case RENDERER_AC60_EVENT_FT3:
    case RENDERER_AC60_EVENT_FT4:
    case RENDERER_AC60_EVENT_GT4:
        return EventKind::polygon;
    case RENDERER_AC60_EVENT_VARIABLE_SPRITE:
    case RENDERER_AC60_EVENT_SPRITE_8:
    case RENDERER_AC60_EVENT_SPRITE_16:
        return EventKind::sprite;
    case RENDERER_AC60_EVENT_E1_STATE:
    case RENDERER_AC60_EVENT_F3_CLIP:
    case RENDERER_AC60_EVENT_OT_GATE:
    case RENDERER_AC60_EVENT_IGNORED_RASTER:
    case RENDERER_AC60_EVENT_RASTER_SUPPRESSED:
        return EventKind::geometry_state;
    default:
        return EventKind::unsupported;
    }
}

bool add_texture_palette_references(WalkContext &context,
                                   const Event &packet_event)
{
    if (packet_event.kind != EventKind::polygon)
        return true;
    Event texture{};
    texture.kind = EventKind::texture_reference;
    texture.origin = context.controlled ? EventOrigin::shared_staging_model
                                       : EventOrigin::packet_decode;
    texture.provenance = packet_event.provenance;
    texture.raw_address = packet_event.raw_address;
    texture.related_packet = packet_event.raw_address;
    texture.command = packet_event.command;
    texture.packet_kind = packet_event.packet_kind;
    texture.decode_status = packet_event.decode_status;
    texture.decoded = packet_event.decoded;
    if (!append_event(context, std::move(texture)))
        return false;

    Event palette{};
    palette.kind = EventKind::palette_reference;
    palette.origin = context.controlled ? EventOrigin::shared_staging_model
                                       : EventOrigin::packet_decode;
    palette.provenance = packet_event.provenance;
    palette.raw_address = packet_event.raw_address;
    palette.related_packet = packet_event.raw_address;
    palette.command = packet_event.command;
    palette.packet_kind = packet_event.packet_kind;
    palette.decode_status = packet_event.decode_status;
    palette.decoded = packet_event.decoded;
    /* These are packet references only, not texture or palette mutations. */
    return append_event(context, std::move(palette));
}

bool add_callback_boundary(WalkContext &context, renderer_raw32 packet,
                           renderer_u8 command)
{
    Event boundary{};
    boundary.kind = EventKind::callback_boundary;
    boundary.origin = EventOrigin::controlled_offline_callback;
    boundary.provenance = CaptureProvenance::controlled_offline_consumption;
    boundary.raw_address = packet;
    boundary.related_packet = packet;
    boundary.command = command;
    return append_event(context, std::move(boundary));
}

int visit_packet(void *opaque, renderer_raw32 address)
{
    WalkContext *context = static_cast<WalkContext *>(opaque);
    try {
        const renderer_u8 *header = nullptr;
        if (context == nullptr || context->result == nullptr ||
            !read_bytes(*context, address, 8U, &header)) {
            if (context != nullptr && context->result != nullptr)
                context->result->status = CaptureStatus::malformed_packet;
            return 0;
        }

        RendererDecodedPacket header_decoded{};
        const RendererPacketStatus header_status =
            renderer_decode_packet(header, 8U, &header_decoded);
        if (header_status == RENDERER_PACKET_SHORT_HEADER ||
            header_status == RENDERER_PACKET_INVALID_ARGUMENT) {
            context->result->status = CaptureStatus::malformed_packet;
            return 0;
        }
        const renderer_u16 required_size =
            renderer_packet_capture_size(&header_decoded);
        if (required_size < 8U || required_size > context->limits.max_packet_bytes) {
            context->result->status = CaptureStatus::packet_limit_exceeded;
            return 0;
        }

        std::vector<renderer_u8> owned;
        owned.resize(required_size);
        const renderer_u8 *full = nullptr;
        if (!read_bytes(*context, address, required_size, &full)) {
            owned.assign(header, header + 8U);
            Event event = base_packet_event(*context, address, header_decoded,
                                            RENDERER_PACKET_TRUNCATED,
                                            std::move(owned));
            event.kind = EventKind::unsupported;
            if (!append_event(*context, std::move(event)))
                return 0;
            context->result->status = CaptureStatus::malformed_packet;
            return 0;
        }
        std::copy(full, full + required_size, owned.begin());

        RendererDecodedPacket decoded{};
        const RendererPacketStatus decode_status = renderer_decode_packet(
            owned.data(), owned.size(), &decoded);
        if (decode_status == RENDERER_PACKET_SHORT_HEADER ||
            decode_status == RENDERER_PACKET_TRUNCATED ||
            decode_status == RENDERER_PACKET_INVALID_ARGUMENT) {
            Event event = base_packet_event(*context, address, decoded,
                                            decode_status, std::move(owned));
            event.kind = EventKind::unsupported;
            if (!append_event(*context, std::move(event)))
                return 0;
            context->result->status = CaptureStatus::malformed_packet;
            return 0;
        }

        Event event = base_packet_event(*context, address, decoded,
                                        decode_status, std::move(owned));
        if (context->controlled) {
            RendererAc60Event staged{};
            const RendererAc60ApplyStatus apply_status =
                renderer_ac60_apply_packet(event.owned_packet.data(),
                                           event.owned_packet.size(),
                                           &context->state, &staged);
            event.kind = classify_staged(staged, apply_status);
            event.staging_status = apply_status;
            event.has_staging_result = true;
            event.staged = staged;
            event.state_after = context->state;
            event.has_state_after = true;
            event.origin = EventOrigin::shared_staging_model;
            if (event.kind == EventKind::unsupported)
                context->unsupported_seen = true;
        } else {
            event.kind = classify_passive(decoded, decode_status);
            if (event.kind == EventKind::geometry_state &&
                decoded.command == 0xe1U) {
                event.payload_status = hercules_state_decode_e1(
                    event.owned_packet.data(), event.owned_packet.size(),
                    &event.e1_state);
                event.has_e1_state = event.payload_status == HERCULES_STATE_OK;
                if (!event.has_e1_state)
                    event.kind = EventKind::unsupported;
            } else if (event.kind == EventKind::geometry_state &&
                       decoded.command == 0xf3U) {
                event.payload_status = hercules_state_decode_f3(
                    event.owned_packet.data(), event.owned_packet.size(),
                    &event.clip_bounds);
                event.has_clip_bounds =
                    event.payload_status == HERCULES_STATE_OK;
                if (!event.has_clip_bounds)
                    event.kind = EventKind::unsupported;
            } else if (event.kind == EventKind::vram_move_request) {
                event.payload_status = hercules_state_decode_command01(
                    event.owned_packet.data(), event.owned_packet.size(),
                    &event.deferred_move);
                event.has_deferred_move =
                    event.payload_status == HERCULES_STATE_OK;
                if (!event.has_deferred_move)
                    event.kind = EventKind::unsupported;
            }
            if (event.kind == EventKind::unsupported)
                context->unsupported_seen = true;
        }

        if (!append_event(*context, Event(event)))
            return 0;
        if (!add_texture_palette_references(*context, event))
            return 0;

        if (!context->controlled)
            return 1;

        bool keep_walking = true;
        try {
            keep_walking = (*context->callback)(event);
        } catch (...) {
            context->result->status = CaptureStatus::callback_failed;
            (void)add_callback_boundary(*context, address, event.command);
            return 0;
        }
        if (!add_callback_boundary(*context, address, event.command))
            return 0;
        if (!keep_walking) {
            context->result->status = CaptureStatus::callback_stopped;
            return 0;
        }
        return 1;
    } catch (const std::bad_alloc &) {
        if (context != nullptr && context->result != nullptr)
            context->result->status = CaptureStatus::event_limit_exceeded;
        return 0;
    } catch (...) {
        if (context != nullptr && context->result != nullptr)
            context->result->status = CaptureStatus::frame_failed;
        return 0;
    }
}

CaptureResult run_capture(FrameRaw32Resolver resolver, void *resolver_context,
                          renderer_raw32 order_table_base,
                          renderer_u32 slot_count, CaptureLimits limits,
                          CaptureProvenance provenance, bool controlled,
                          RendererAc60StateView initial_state,
                          const PacketCallback *callback)
{
    CaptureResult result{};
    result.provenance = provenance;
    if (resolver == nullptr || order_table_base == 0 || slot_count == 0 ||
        limits.max_visits == 0 || limits.max_events == 0 ||
        limits.max_packet_bytes < 8U ||
        (controlled && (callback == nullptr || !*callback))) {
        result.status = CaptureStatus::invalid_argument;
        return result;
    }

    WalkContext context{};
    context.resolver = resolver;
    context.resolver_context = resolver_context;
    context.limits = limits;
    context.result = &result;
    context.callback = callback;
    context.state = initial_state;
    context.controlled = controlled;
    std::vector<renderer_raw32> visited;
    try {
        visited.resize(limits.max_visits);
        result.events.reserve(std::min<std::size_t>(limits.max_events, 1024U));
    } catch (...) {
        result.status = CaptureStatus::packet_limit_exceeded;
        return result;
    }

    FrameVisitGuard guard{};
    frame_visit_guard_init(&guard, visited.data(), limits.max_visits);
    result.status = CaptureStatus::ok;
    result.frame_status = frame_walk_ac60_order_table_visit(
        resolver, resolver_context, order_table_base, slot_count, &guard,
        visit_packet, &context);
    if (result.status != CaptureStatus::ok)
        return result;
    if (result.frame_status != FRAME_WALK_OK) {
        result.status = CaptureStatus::frame_failed;
        return result;
    }
    if (context.unsupported_seen)
        result.status = CaptureStatus::complete_with_unsupported;
    return result;
}

} // namespace

CaptureResult observe_entry(FrameRaw32Resolver resolver, void *resolver_context,
                           renderer_raw32 order_table_base,
                           renderer_u32 slot_count, CaptureLimits limits)
{
    return run_capture(resolver, resolver_context, order_table_base,
                       slot_count, limits,
                       CaptureProvenance::passive_entry_observation, false,
                       RendererAc60StateView{}, nullptr);
}

CaptureResult consume_offline(FrameRaw32Resolver resolver,
                              void *resolver_context,
                              renderer_raw32 order_table_base,
                              renderer_u32 slot_count,
                              RendererAc60StateView initial_state,
                              const PacketCallback &callback,
                              CaptureLimits limits)
{
    return run_capture(resolver, resolver_context, order_table_base,
                       slot_count, limits,
                       CaptureProvenance::controlled_offline_consumption,
                       true, initial_state, &callback);
}

} // namespace hercules::dropin
