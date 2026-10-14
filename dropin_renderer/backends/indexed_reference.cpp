#include "indexed_reference.hpp"
#include <limits>
#include <utility>

namespace hercules::dropin {
namespace {
HerculesRectangleTextureView texture_view(const renderer_u8 *bytes = nullptr)
{
    return {HERC_TEX_MAP_BASE, bytes, 65536U, 256U, 256U, 256U};
}
HerculesRectangleSurfaceView surface_view(renderer_u8 *bytes = nullptr)
{
    return {bytes, static_cast<renderer_u32>(indexed_frame_bytes), 320U, 240U, 320U};
}
}

IndexedReferenceStatus capture_indexed_rectangle(
    const Event &event, const IndexedAddressMemory &memory,
    IndexedRectangleCommand *command)
{
    if (command == nullptr)
        return IndexedReferenceStatus::invalid_command;
    if (event.provenance != CaptureProvenance::controlled_offline_consumption ||
        !event.has_state_after || !event.has_staging_result ||
        event.staging_status != RENDERER_AC60_APPLY_OK ||
        event.staged.kind != RENDERER_AC60_EVENT_FT4)
        return IndexedReferenceStatus::unsupported;
    RendererDecodedPacket packet{};
    if (renderer_decode_packet(event.owned_packet.data(), event.owned_packet.size(), &packet)
        != RENDERER_PACKET_OK)
        return IndexedReferenceStatus::malformed_packet;
    const auto texture = texture_view();
    const auto surface = surface_view();
    IndexedRectangleCommand captured{};
    const auto status = hercules_rectangle_resolve(&packet, event.owned_packet.size(),
        &event.state_after, &texture, &surface, &captured.plan);
    if (status != HERCULES_RECTANGLE_OK)
        return status == HERCULES_RECTANGLE_TRUNCATED
            ? IndexedReferenceStatus::malformed_packet : IndexedReferenceStatus::unsupported;
    captured.packet_address = event.raw_address;
    captured.texture_generation = memory.generation();
    for (renderer_u32 row = 0; row < captured.plan.height; ++row) {
        for (renderer_u32 column = 0; column < captured.plan.width; ++column) {
            const std::uint64_t address = static_cast<std::uint64_t>(captured.plan.page_address)
                + captured.plan.source_start + row * captured.plan.source_pitch + column;
            if (address > std::numeric_limits<std::uint32_t>::max())
                return IndexedReferenceStatus::missing_texture;
            std::uint8_t value = 0;
            if (memory.read_byte(static_cast<std::uint32_t>(address), &value)
                != IndexedMemoryStatus::ok)
                return IndexedReferenceStatus::missing_texture;
            captured.texels.push_back(value);
            captured.sample_addresses.push_back(static_cast<std::uint32_t>(address));
        }
    }
    *command = std::move(captured);
    return IndexedReferenceStatus::ok;
}

IndexedReferenceStatus render_indexed_rectangle(
    const IndexedRectangleCommand &command, IndexedFrame *frame,
    IndexedReferenceEffects *effects)
{
    if (frame == nullptr || effects == nullptr || command.plan.width == 0 ||
        command.plan.height == 0 || command.plan.width > 8 || command.plan.height > 8 ||
        command.texels.size() != command.plan.width * command.plan.height ||
        command.sample_addresses.size() != command.texels.size())
        return IndexedReferenceStatus::invalid_command;
    /* Padding belongs only to this private scratch adapter. The shared renderer
     * reads exactly the resolved rectangle; no padded byte is claimed observed. */
    std::vector<renderer_u8> scratch_page(65536U, 0);
    std::size_t index = 0;
    for (renderer_u32 row = 0; row < command.plan.height; ++row) {
        for (renderer_u32 column = 0; column < command.plan.width; ++column, ++index) {
            const std::uint64_t offset = static_cast<std::uint64_t>(command.plan.source_start)
                + row * command.plan.source_pitch + column;
            if (offset >= scratch_page.size() ||
                static_cast<std::uint64_t>(command.plan.page_address) + offset != command.sample_addresses[index])
                return IndexedReferenceStatus::invalid_command;
            scratch_page[static_cast<std::size_t>(offset)] = command.texels[index];
        }
    }
    const auto texture = texture_view(scratch_page.data());
    const auto surface = surface_view(frame->data());
    HerculesRectangleEffects shared{};
    if (hercules_rectangle_render(&command.plan, &texture, &surface, nullptr, nullptr, &shared)
        != HERCULES_RECTANGLE_OK)
        return IndexedReferenceStatus::invalid_command;
    effects->pixel_store_count = shared.pixel_store_count;
    effects->presentation_requested = effects->presentation_requested || shared.presentation_requested != 0;
    return IndexedReferenceStatus::ok;
}

} // namespace hercules::dropin
