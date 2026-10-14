#ifndef HERCULES_DROPIN_CORE_INDEXED_MEMORY_HPP
#define HERCULES_DROPIN_CORE_INDEXED_MEMORY_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include "hercules_textures.h"

namespace hercules::dropin {

enum class IndexedMemoryStatus : std::uint8_t {
    ok,
    invalid_argument,
    outside_supplied_range,
    unobserved_byte,
    unsupported_coordinate_domain,
    payload_size_mismatch,
    deferred_vram_move
};

/* A caller-bounded address space with one backing byte for every absolute
 * address in [base_address, base_address + byte_count). The range is an
 * explicit model boundary; it is not evidence of a native allocation extent.
 * Bytes start unobserved and cannot be read until supplied by the caller. */
class IndexedAddressMemory {
public:
    IndexedAddressMemory(std::uint32_t base_address, std::size_t byte_count);

    std::uint32_t base_address() const noexcept;
    std::size_t byte_count() const noexcept;
    /* Counts successful non-empty backing updates; it is only a mutation
     * observation and carries no world, entity, or callback identity. */
    std::uint64_t generation() const noexcept;

    IndexedMemoryStatus read_byte(std::uint32_t address,
                                  std::uint8_t *value_out) const noexcept;
    IndexedMemoryStatus read_observed_span(
        std::uint32_t address, std::size_t byte_count,
        std::vector<std::uint8_t> *bytes_out) const;

    /* Store caller-observed raw bytes in the shared address backing. This is
     * also the palette/CLUT update path; it performs no parsing or conversion. */
    IndexedMemoryStatus update_observed_span(
        std::uint32_t address,
        const std::vector<std::uint8_t> &detached_bytes);

    /* Synchronously copy an already detached caller payload into the address
     * model. The payload is not a native pointer or an alias into this memory.
     * Supported RECT coordinates are nonnegative and fully inside the bounded
     * 0..1023 x/y domain. The mapping is latched once from rect.y; all output
     * addresses and payload size are checked before any backing byte changes. */
    IndexedMemoryStatus upload_texture_rect(
        const HERC_TEX_RECT16 &rect,
        const std::vector<std::uint8_t> &detached_payload);

    /* A8F0's finite evidence shows forward-copy overlap behavior, which is not
     * implemented here. The explicit status prevents a caller treating a move
     * request as a completed memory mutation. */
    IndexedMemoryStatus request_vram_rectangle_move(
        const HERC_TEX_RECT16 &source_rect,
        renderer_i32 destination_x, renderer_i32 destination_y) const noexcept;

private:
    bool byte_offset(std::uint32_t address, std::size_t *offset_out) const noexcept;
    bool span_offset(std::uint32_t address, std::size_t length,
                     std::size_t *offset_out) const noexcept;

    std::uint32_t base_address_;
    std::vector<std::uint8_t> bytes_;
    std::vector<std::uint8_t> observed_;
    std::uint64_t generation_ = 0;
};

} // namespace hercules::dropin

#endif
