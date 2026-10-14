#ifndef HERCULES_DROPIN_INDEXED_COMMAND_HPP
#define HERCULES_DROPIN_INDEXED_COMMAND_HPP

#include <cstdint>
#include <vector>
#include "hercules_rectangle.h"

namespace hercules::dropin {

/* One synchronous primitive observation. Only the texels actually consumed
 * by the supported rectangle are copied; unknown page bytes stay unknown.
 * This tile is private owned storage, with original UV/address provenance in
 * the plan. It is not a copied frame, allocation or texture-page assertion. */
struct IndexedRectangleCommand {
    HerculesRectanglePlan plan{};
    std::vector<std::uint8_t> texels;
    std::vector<std::uint32_t> sample_addresses;
    /* Selected table snapshots belong to this command's consumption instant.
     * Empty tables are permitted when every source texel is zero. */
    std::vector<std::uint8_t> shade_row;
    std::vector<std::uint8_t> blend_table;
    std::uint32_t packet_address = 0;
    std::uint64_t texture_generation = 0;
};

} // namespace hercules::dropin
#endif
