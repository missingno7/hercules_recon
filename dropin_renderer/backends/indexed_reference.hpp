#ifndef HERCULES_DROPIN_INDEXED_REFERENCE_HPP
#define HERCULES_DROPIN_INDEXED_REFERENCE_HPP

#include <array>
#include <cstdint>
#include "event.hpp"
#include "indexed_command.hpp"
#include "indexed_memory.hpp"

namespace hercules::dropin {

constexpr std::size_t indexed_frame_width = 320;
constexpr std::size_t indexed_frame_height = 240;
constexpr std::size_t indexed_frame_bytes = indexed_frame_width * indexed_frame_height;
using IndexedFrame = std::array<std::uint8_t, indexed_frame_bytes>;

enum class IndexedReferenceStatus {
    ok,
    unsupported,
    malformed_packet,
    missing_texture,
    missing_table,
    invalid_command,
    no_draw
};

struct IndexedReferenceEffects {
    std::uint32_t pixel_store_count = 0;
    bool presentation_requested = false;
};

/* Resolve the authoritative primitive domain at synchronous consumption,
 * then capture its observed texels and selected shade/blend table snapshots.
 * Source-zero commands need no table bytes. Output is unchanged on failure.
 * Passive entry observations cannot establish consumed state. */
IndexedReferenceStatus capture_indexed_rectangle(
    const Event &event, const IndexedAddressMemory &memory,
    IndexedRectangleCommand *command);

/* Private CPU reference output only. Reuse shared CAB0/C8F0/C700 steps;
 * do not present, mutate original state, or implement unknown primitives.
 * On success preserve a sticky presentation request even for zero stores. */
IndexedReferenceStatus render_indexed_rectangle(
    const IndexedRectangleCommand &command, IndexedFrame *frame,
    IndexedReferenceEffects *effects);

} // namespace hercules::dropin
#endif
