#include "indexed_sprite_reference.hpp"
#include <algorithm>
#include <limits>
#include <utility>

namespace hercules::dropin {
namespace {
int read_table(void *context, renderer_u32 offset, renderer_u8 *value)
{
    const auto &bytes = *static_cast<const std::vector<std::uint8_t> *>(context);
    if (value == nullptr || offset >= bytes.size()) return 0;
    *value = bytes[offset];
    return 1;
}
HerculesRectangleTextureView texture_view(renderer_raw32 address, const renderer_u8 *bytes = nullptr)
{
    return {address, bytes, 65536U, 256U, 256U, 256U};
}
HerculesRectangleSurfaceView surface_view(renderer_u8 *bytes = nullptr)
{
    return {bytes, static_cast<renderer_u32>(indexed_frame_bytes), 320U, 240U, 320U};
}
}

IndexedReferenceStatus capture_indexed_sprite(
    const Event &event, const IndexedAddressMemory &memory,
    IndexedSpriteCommand *command)
{
    if (command == nullptr) return IndexedReferenceStatus::invalid_command;
    if (event.provenance != CaptureProvenance::controlled_offline_consumption ||
        !event.has_state_after || !event.has_staging_result ||
        event.staging_status != RENDERER_AC60_APPLY_OK ||
        (event.staged.kind != RENDERER_AC60_EVENT_VARIABLE_SPRITE &&
         event.staged.kind != RENDERER_AC60_EVENT_SPRITE_8 &&
         event.staged.kind != RENDERER_AC60_EVENT_SPRITE_16 &&
         !(event.staged.kind == RENDERER_AC60_EVENT_RASTER_SUPPRESSED &&
           (event.packet_kind == RENDERER_PACKET_SPRITE_VARIABLE ||
            event.packet_kind == RENDERER_PACKET_SPRITE_8 ||
            event.packet_kind == RENDERER_PACKET_SPRITE_16))))
        return IndexedReferenceStatus::unsupported;
    RendererDecodedPacket packet{};
    if (renderer_decode_packet(event.owned_packet.data(), event.owned_packet.size(), &packet) != RENDERER_PACKET_OK)
        return IndexedReferenceStatus::malformed_packet;
    IndexedSpriteCommand captured{};
    const auto texture = texture_view(event.state_after.workspace.texture_page_address);
    const auto surface = surface_view();
    const auto status = hercules_sprite_resolve(&packet, event.owned_packet.size(), &event.state_after,
                                              &texture, &surface, &captured.plan);
    if (status == HERCULES_SPRITE_NO_DRAW) return IndexedReferenceStatus::no_draw;
    if (status != HERCULES_SPRITE_OK)
        return status == HERCULES_SPRITE_TRUNCATED ? IndexedReferenceStatus::malformed_packet : IndexedReferenceStatus::unsupported;
    captured.packet_address = event.raw_address;
    captured.texture_generation = memory.generation();
    for (renderer_u32 row = 0; row < captured.plan.height; ++row) {
        for (renderer_u32 column = 0; column < captured.plan.width; ++column) {
            const std::uint64_t address = static_cast<std::uint64_t>(captured.plan.page_address) +
                captured.plan.source_start + row * captured.plan.source_pitch + column;
            if (address > std::numeric_limits<std::uint32_t>::max()) return IndexedReferenceStatus::missing_texture;
            std::uint8_t value;
            if (memory.read_byte(static_cast<std::uint32_t>(address), &value) != IndexedMemoryStatus::ok)
                return IndexedReferenceStatus::missing_texture;
            captured.texels.push_back(value);
            captured.sample_addresses.push_back(static_cast<std::uint32_t>(address));
        }
    }
    const bool nonzero = std::any_of(captured.texels.begin(), captured.texels.end(), [](std::uint8_t value) { return value != 0; });
    if (nonzero && captured.plan.pixel_mode == HERCULES_RECTANGLE_RAW_BLEND &&
        memory.read_observed_span(HERCULES_RECTANGLE_BLEND_BASE + captured.plan.blend_selector * 65536U,
                                  65536U, &captured.blend_table) != IndexedMemoryStatus::ok)
        return IndexedReferenceStatus::missing_table;
    *command = std::move(captured);
    return IndexedReferenceStatus::ok;
}

IndexedReferenceStatus render_indexed_sprite(
    const IndexedSpriteCommand &command, IndexedFrame *frame,
    IndexedReferenceEffects *effects)
{
    const auto &plan = command.plan;
    if (frame == nullptr || effects == nullptr || plan.width == 0 || plan.width > 256 ||
        plan.height == 0 || plan.height > 240 || command.texels.size() != plan.width * plan.height ||
        command.sample_addresses.size() != command.texels.size())
        return IndexedReferenceStatus::invalid_command;
    const bool nonzero = std::any_of(command.texels.begin(), command.texels.end(), [](std::uint8_t value) { return value != 0; });
    if ((plan.pixel_mode == HERCULES_RECTANGLE_COPY && (!command.blend_table.empty() || plan.blend_selector != 0)) ||
        (plan.pixel_mode == HERCULES_RECTANGLE_RAW_BLEND &&
         (plan.blend_selector >= 4 || command.blend_table.size() != (nonzero ? 65536U : 0U))) ||
        (plan.pixel_mode != HERCULES_RECTANGLE_COPY && plan.pixel_mode != HERCULES_RECTANGLE_RAW_BLEND))
        return IndexedReferenceStatus::invalid_command;
    std::vector<renderer_u8> scratch(65536U, 0);
    std::size_t index = 0;
    for (renderer_u32 row = 0; row < plan.height; ++row) {
        for (renderer_u32 column = 0; column < plan.width; ++column, ++index) {
            const std::uint64_t offset = static_cast<std::uint64_t>(plan.source_start) + row * plan.source_pitch + column;
            if (offset >= scratch.size() || static_cast<std::uint64_t>(plan.page_address) + offset != command.sample_addresses[index])
                return IndexedReferenceStatus::invalid_command;
            scratch[static_cast<std::size_t>(offset)] = command.texels[index];
        }
    }
    const auto texture = texture_view(plan.page_address, scratch.data());
    const auto surface = surface_view(frame->data());
    const HercPixelTableReader blend = {static_cast<renderer_u32>(command.blend_table.size()), read_table,
                                        const_cast<std::vector<std::uint8_t> *>(&command.blend_table)};
    HerculesRectangleEffects result{};
    if (hercules_sprite_render(&plan, &texture, &surface, &blend, nullptr, nullptr, &result) != HERCULES_SPRITE_OK)
        return IndexedReferenceStatus::invalid_command;
    effects->pixel_store_count = result.pixel_store_count;
    effects->presentation_requested = effects->presentation_requested || result.presentation_requested != 0;
    return IndexedReferenceStatus::ok;
}

} // namespace hercules::dropin
