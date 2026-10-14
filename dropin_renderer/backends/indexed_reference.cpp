#include "indexed_reference.hpp"
#include <limits>
#include <utility>
#include <algorithm>

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
int table_read(void *opaque, renderer_u32 offset, renderer_u8 *value)
{
    const auto &bytes = *static_cast<const std::vector<std::uint8_t> *>(opaque);
    if (value == nullptr || offset >= bytes.size()) return 0;
    *value = bytes[offset];
    return 1;
}
HercPixelTableReader table_view(const std::vector<std::uint8_t> &bytes)
{
    return {static_cast<renderer_u32>(bytes.size()), table_read,
            const_cast<std::vector<std::uint8_t> *>(&bytes)};
}
bool tables_valid(const IndexedRectangleCommand &command)
{
    const auto &plan = command.plan;
    const bool nonzero = std::any_of(command.texels.begin(), command.texels.end(),
                                    [](std::uint8_t value) { return value != 0; });
    switch (plan.pixel_mode) {
    case HERCULES_RECTANGLE_COPY:
        return plan.blend_selector == 0 && plan.shade_row == 0 &&
               command.shade_row.empty() && command.blend_table.empty();
    case HERCULES_RECTANGLE_RAW_BLEND:
        return plan.blend_selector < 4 && plan.shade_row == 0 && command.shade_row.empty() &&
               command.blend_table.size() == (nonzero ? 65536U : 0U);
    case HERCULES_RECTANGLE_SHADED_BLEND:
        return plan.blend_selector < 4 && plan.shade_row < 256 && plan.shade_row != 0x80 &&
               command.shade_row.size() == (nonzero ? 256U : 0U) &&
               command.blend_table.size() == (nonzero ? 65536U : 0U);
    default:
        return false;
    }
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
    const bool nonzero = std::any_of(captured.texels.begin(), captured.texels.end(),
                                    [](std::uint8_t value) { return value != 0; });
    if (nonzero && captured.plan.pixel_mode != HERCULES_RECTANGLE_COPY) {
        const auto blend_address = HERCULES_RECTANGLE_BLEND_BASE + captured.plan.blend_selector * 65536U;
        if (memory.read_observed_span(blend_address, 65536U, &captured.blend_table) != IndexedMemoryStatus::ok)
            return IndexedReferenceStatus::missing_table;
        if (captured.plan.pixel_mode == HERCULES_RECTANGLE_SHADED_BLEND) {
            const auto shade_address = HERCULES_RECTANGLE_SHADE_BASE + captured.plan.shade_row * 256U;
            if (memory.read_observed_span(shade_address, 256U, &captured.shade_row) != IndexedMemoryStatus::ok)
                return IndexedReferenceStatus::missing_table;
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
        command.sample_addresses.size() != command.texels.size() || !tables_valid(command))
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
    const auto shade = table_view(command.shade_row);
    HercPixelTableReader blends[4]{};
    if (command.plan.blend_selector < 4)
        blends[command.plan.blend_selector] = table_view(command.blend_table);
    HerculesRectangleEffects shared{};
    if (hercules_rectangle_render_tables(&command.plan, &texture, &surface, &shade, blends, 4,
                                        nullptr, nullptr, &shared)
        != HERCULES_RECTANGLE_OK)
        return IndexedReferenceStatus::invalid_command;
    effects->pixel_store_count = shared.pixel_store_count;
    effects->presentation_requested = effects->presentation_requested || shared.presentation_requested != 0;
    return IndexedReferenceStatus::ok;
}

} // namespace hercules::dropin
