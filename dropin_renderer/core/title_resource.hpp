#ifndef HERCULES_DROPIN_TITLE_RESOURCE_HPP
#define HERCULES_DROPIN_TITLE_RESOURCE_HPP

#include <array>
#include <string>
#include <vector>
#include "archive_reader.hpp"
#include "hercules_cif.h"
#include "indexed_sprite_reference.hpp"

namespace hercules::dropin {

/* A controlled screen-resource replay, not an original TITLE frame capture. */
struct TitleScreenResource {
    std::string label;
    ArchiveMember image;
    ArchiveMember cif;
    std::string image_sha256;
    std::string cif_sha256;
    std::array<HerculesCifPaletteEntry, 256> palette{};
    std::vector<IndexedSpriteCommand> commands;
    IndexedFrame cpu_indices{};
    std::uint32_t pixel_stores = 0;
    bool presentation_requested = false;
};

bool load_title_screen(const std::string &archive_path,
                       TitleScreenResource *output, std::string *error);

/* Opaque modern window pixels from original PC palette entries. */
using RgbaFrame = std::vector<std::uint8_t>;
RgbaFrame title_screen_rgba(const IndexedFrame &indices,
                          const std::array<HerculesCifPaletteEntry, 256> &palette);

std::string sha256_bytes(const std::vector<std::uint8_t> &bytes);

} // namespace hercules::dropin
#endif
