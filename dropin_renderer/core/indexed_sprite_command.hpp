#ifndef HERCULES_DROPIN_INDEXED_SPRITE_COMMAND_HPP
#define HERCULES_DROPIN_INDEXED_SPRITE_COMMAND_HPP

#include <cstdint>
#include <vector>
#include "hercules_sprite_raster.h"

namespace hercules::dropin {

/* Owned at controlled consumption, separate from the FT4 rectangle family. */
struct IndexedSpriteCommand {
    HerculesSpritePlan plan{};
    std::vector<std::uint8_t> texels;
    std::vector<std::uint32_t> sample_addresses;
    std::vector<std::uint8_t> blend_table;
    std::uint32_t packet_address = 0;
    std::uint64_t texture_generation = 0;
};

} // namespace hercules::dropin
#endif
