#include "indexed_memory.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace hercules::dropin {

namespace {

constexpr std::uint64_t kAddressSpaceSize = 0x100000000ULL;
constexpr renderer_i32 kIndexedCoordinateLimit = 1024;

struct PendingWrite {
    std::size_t offset;
    std::uint8_t value;
};

} // namespace

IndexedAddressMemory::IndexedAddressMemory(std::uint32_t base_address,
                                           std::size_t byte_count)
    : base_address_(base_address)
{
    const std::uint64_t available =
        kAddressSpaceSize - static_cast<std::uint64_t>(base_address);
    if (static_cast<std::uint64_t>(byte_count) > available)
        throw std::invalid_argument("indexed address range exceeds 32-bit space");
    bytes_.resize(byte_count);
    observed_.resize(byte_count, 0U);
}

std::uint32_t IndexedAddressMemory::base_address() const noexcept
{
    return base_address_;
}

std::size_t IndexedAddressMemory::byte_count() const noexcept
{
    return bytes_.size();
}

std::uint64_t IndexedAddressMemory::generation() const noexcept
{
    return generation_;
}

bool IndexedAddressMemory::byte_offset(std::uint32_t address,
                                       std::size_t *offset_out) const noexcept
{
    if (offset_out == nullptr || address < base_address_)
        return false;
    const std::uint64_t offset =
        static_cast<std::uint64_t>(address) - base_address_;
    if (offset >= static_cast<std::uint64_t>(bytes_.size()))
        return false;
    *offset_out = static_cast<std::size_t>(offset);
    return true;
}

bool IndexedAddressMemory::span_offset(std::uint32_t address,
                                       std::size_t length,
                                       std::size_t *offset_out) const noexcept
{
    if (offset_out == nullptr || address < base_address_)
        return false;
    const std::uint64_t offset =
        static_cast<std::uint64_t>(address) - base_address_;
    const std::uint64_t capacity = static_cast<std::uint64_t>(bytes_.size());
    if (offset > capacity ||
        static_cast<std::uint64_t>(length) > capacity - offset)
        return false;
    *offset_out = static_cast<std::size_t>(offset);
    return true;
}

IndexedMemoryStatus IndexedAddressMemory::read_byte(
    std::uint32_t address, std::uint8_t *value_out) const noexcept
{
    if (value_out == nullptr)
        return IndexedMemoryStatus::invalid_argument;
    std::size_t offset = 0;
    if (!byte_offset(address, &offset))
        return IndexedMemoryStatus::outside_supplied_range;
    if (observed_[offset] == 0U)
        return IndexedMemoryStatus::unobserved_byte;
    *value_out = bytes_[offset];
    return IndexedMemoryStatus::ok;
}

IndexedMemoryStatus IndexedAddressMemory::read_observed_span(
    std::uint32_t address, std::size_t length,
    std::vector<std::uint8_t> *bytes_out) const
{
    if (bytes_out == nullptr)
        return IndexedMemoryStatus::invalid_argument;
    std::size_t offset = 0;
    if (!span_offset(address, length, &offset))
        return IndexedMemoryStatus::outside_supplied_range;

    std::vector<std::uint8_t> copy;
    copy.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
        if (observed_[offset + i] == 0U)
            return IndexedMemoryStatus::unobserved_byte;
        copy.push_back(bytes_[offset + i]);
    }
    *bytes_out = std::move(copy);
    return IndexedMemoryStatus::ok;
}

IndexedMemoryStatus IndexedAddressMemory::update_observed_span(
    std::uint32_t address,
    const std::vector<std::uint8_t> &detached_bytes)
{
    std::size_t offset = 0;
    if (!span_offset(address, detached_bytes.size(), &offset))
        return IndexedMemoryStatus::outside_supplied_range;
    if (detached_bytes.empty())
        return IndexedMemoryStatus::ok;

    std::copy(detached_bytes.begin(), detached_bytes.end(),
              bytes_.begin() + static_cast<std::ptrdiff_t>(offset));
    std::fill(observed_.begin() + static_cast<std::ptrdiff_t>(offset),
              observed_.begin() + static_cast<std::ptrdiff_t>(offset + detached_bytes.size()),
              static_cast<std::uint8_t>(1U));
    ++generation_;
    return IndexedMemoryStatus::ok;
}

IndexedMemoryStatus IndexedAddressMemory::upload_texture_rect(
    const HERC_TEX_RECT16 &rect,
    const std::vector<std::uint8_t> &detached_payload)
{
    if (rect.x < 0 || rect.y < 0 || rect.width < 0 || rect.height < 0 ||
        rect.x >= kIndexedCoordinateLimit || rect.y >= kIndexedCoordinateLimit ||
        static_cast<renderer_i32>(rect.x) + rect.width > kIndexedCoordinateLimit ||
        static_cast<renderer_i32>(rect.y) + rect.height > kIndexedCoordinateLimit)
        return IndexedMemoryStatus::unsupported_coordinate_domain;

    const std::size_t expected_size =
        static_cast<std::size_t>(rect.width) *
        static_cast<std::size_t>(rect.height) * 2U;
    if (detached_payload.size() != expected_size)
        return IndexedMemoryStatus::payload_size_mismatch;

    /* A710 latches this from the entry y and keeps it for every row. */
    const HERC_TEX_ROW_MAPPING mapping =
        herc_tex_row_mapping_for_entry_y(rect.y);
    if (detached_payload.empty())
        return IndexedMemoryStatus::ok;

    std::vector<PendingWrite> writes;
    writes.reserve(detached_payload.size());
    std::size_t payload_index = 0;
    for (renderer_i32 row = 0; row < rect.height; ++row) {
        const renderer_i32 current_y = static_cast<renderer_i32>(rect.y) + row;
        for (renderer_i32 word = 0; word < rect.width; ++word) {
            const renderer_i32 x_words = static_cast<renderer_i32>(rect.x) + word;
            for (renderer_u32 byte_in_word = 0; byte_in_word < 2U;
                 ++byte_in_word) {
                renderer_u32 address = 0;
                if (!herc_tex_row_byte_address(mapping, x_words, current_y,
                                               byte_in_word, &address))
                    return IndexedMemoryStatus::unsupported_coordinate_domain;
                std::size_t offset = 0;
                if (!byte_offset(address, &offset))
                    return IndexedMemoryStatus::outside_supplied_range;
                writes.push_back(PendingWrite{offset,
                                              detached_payload[payload_index]});
                ++payload_index;
            }
        }
    }

    /* Commit only after the complete payload and every output address passed. */
    for (const PendingWrite &write : writes) {
        bytes_[write.offset] = write.value;
        observed_[write.offset] = 1U;
    }
    ++generation_;
    return IndexedMemoryStatus::ok;
}

IndexedMemoryStatus IndexedAddressMemory::request_vram_rectangle_move(
    const HERC_TEX_RECT16 &source_rect,
    renderer_i32 destination_x, renderer_i32 destination_y) const noexcept
{
    (void)source_rect;
    (void)destination_x;
    (void)destination_y;
    return IndexedMemoryStatus::deferred_vram_move;
}

} // namespace hercules::dropin
