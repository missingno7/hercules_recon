#include "indexed_memory.hpp"

#include <cstdint>
#include <cstdio>
#include <vector>

using hercules::dropin::IndexedAddressMemory;
using hercules::dropin::IndexedMemoryStatus;

namespace {

int failures = 0;

void check(bool condition, const char *label)
{
    if (!condition) {
        std::printf("FAIL: %s\n", label);
        ++failures;
    }
}

void check_byte(IndexedAddressMemory &memory, std::uint32_t address,
                std::uint8_t expected, const char *label)
{
    std::uint8_t actual = 0xEEU;
    const IndexedMemoryStatus status = memory.read_byte(address, &actual);
    check(status == IndexedMemoryStatus::ok && actual == expected, label);
}

} // namespace

int main()
{
    constexpr std::uint32_t kClutBase = HERC_TEX_CLUT_BASE;
    constexpr std::uint32_t kMapBase = HERC_TEX_MAP_BASE;
    constexpr std::uint32_t kAddressEnd = 0x00AFE6E0U;
    IndexedAddressMemory memory(kClutBase,
                                static_cast<std::size_t>(kAddressEnd - kClutBase));

    check(memory.base_address() == kClutBase, "caller supplies model base");
    check(memory.byte_count() == static_cast<std::size_t>(kAddressEnd - kClutBase),
          "caller supplies model range length");

    std::uint8_t value = 0xD7U;
    check(memory.read_byte(kMapBase, &value) == IndexedMemoryStatus::unobserved_byte,
          "unobserved byte is rejected rather than read as zero");
    check(value == 0xD7U, "failed byte read preserves output argument");

    std::vector<std::uint8_t> clut_bytes{0x00U, 0x55U, 0xFFU};
    check(memory.update_observed_span(kClutBase, clut_bytes) ==
              IndexedMemoryStatus::ok,
          "raw CLUT bytes enter the same observed address model");
    check_byte(memory, kClutBase, 0x00U, "raw CLUT source zero is retained");
    check_byte(memory, kClutBase + 1U, 0x55U, "raw CLUT middle byte is retained");

    const HERC_TEX_RECT16 cross_255_256{5, 255, 1, 2};
    std::vector<std::uint8_t> first_upload{0x11U, 0x00U, 0x33U, 0x44U};
    check(memory.upload_texture_rect(cross_255_256, first_upload) ==
              IndexedMemoryStatus::ok,
          "upload uses one latched mapping across rows 255 and 256");
    check_byte(memory, 0x0097E5EAU, 0x11U, "row 255 low byte address");
    check_byte(memory, 0x0097E5EBU, 0x00U, "uploaded zero byte remains observed");
    check_byte(memory, 0x009EE6EAU, 0x33U,
               "row 256 keeps the entry-y low mapping");
    check_byte(memory, 0x009EE6EBU, 0x44U,
               "row 256 second byte keeps the entry-y low mapping");

    first_upload.assign(first_upload.size(), 0xFEU);
    check_byte(memory, 0x0097E5EBU, 0x00U,
               "upload bytes are owned after caller payload changes");
    const std::uint64_t before_later_upload = memory.generation();
    const std::vector<std::uint8_t> later_upload{0xA1U, 0xA2U, 0xA3U, 0xA4U};
    check(memory.upload_texture_rect(cross_255_256, later_upload) ==
              IndexedMemoryStatus::ok,
          "later upload updates the same address backing");
    check(memory.generation() == before_later_upload + 1U,
          "generation advances once for a completed upload");
    check_byte(memory, 0x0097E5EAU, 0xA1U,
               "later upload replaces the earlier row 255 byte");
    check_byte(memory, 0x009EE6EAU, 0xA3U,
               "later upload replaces the earlier row 256 byte");

    const HERC_TEX_RECT16 cross_511_512{5, 511, 1, 2};
    const std::vector<std::uint8_t> low_entry_upload{0x51U, 0x52U, 0x53U, 0x54U};
    check(memory.upload_texture_rect(cross_511_512, low_entry_upload) ==
              IndexedMemoryStatus::ok,
          "entry y 511 stays latched while the upload crosses 512");
    check_byte(memory, 0x009FE5EAU, 0x51U, "row 511 uses low-entry address");
    check_byte(memory, 0x009EE6EAU, 0x53U,
               "row 512 after entry 511 still uses low-entry address");

    const HERC_TEX_RECT16 high_entry_row{5, 512, 1, 1};
    const std::vector<std::uint8_t> high_entry_upload{0x61U, 0x62U};
    check(memory.upload_texture_rect(high_entry_row, high_entry_upload) ==
              IndexedMemoryStatus::ok,
          "entry y 512 selects high-entry mapping");
    check_byte(memory, 0x00A6E6EAU, 0x61U,
               "row 512 with entry y 512 uses high-entry address");

    /* The finite texture address witness x=284,y=514 reaches R[0] at
     * 0xA8E918. A raw observation of that same absolute address must alias. */
    const HERC_TEX_RECT16 reciprocal_alias{284, 514, 1, 1};
    const std::vector<std::uint8_t> alias_upload{0x00U, 0x7AU};
    check(memory.upload_texture_rect(reciprocal_alias, alias_upload) ==
              IndexedMemoryStatus::ok,
          "bounded upload address expression can overlap captured table bytes");
    check_byte(memory, 0x00A8E918U, 0x00U,
               "texture upload retains source zero at the overlapping byte");
    const std::vector<std::uint8_t> other_table_observation{0xC3U};
    check(memory.update_observed_span(0x00A8E918U, other_table_observation) ==
              IndexedMemoryStatus::ok,
          "raw overlapping table observation updates shared backing");
    check_byte(memory, 0x00A8E918U, 0xC3U,
               "overlapping address reads the latest shared byte");

    std::vector<std::uint8_t> span_out{0xEEU};
    check(memory.read_observed_span(0x00A8E918U, 2U, &span_out) ==
              IndexedMemoryStatus::ok,
          "fully observed spans can be read");
    check(span_out == std::vector<std::uint8_t>({0xC3U, 0x7AU}),
          "span read reflects overlapping update and neighboring byte");
    std::vector<std::uint8_t> unchanged_span{0xEEU};
    check(memory.read_observed_span(kMapBase + 0x100U, 2U, &unchanged_span) ==
              IndexedMemoryStatus::unobserved_byte,
          "span read rejects any missing byte");
    check(unchanged_span == std::vector<std::uint8_t>{0xEEU},
          "failed span read leaves output unchanged");

    const std::uint64_t before_empty = memory.generation();
    const HERC_TEX_RECT16 empty_rect{7, 255, 3, 0};
    check(memory.upload_texture_rect(empty_rect, {}) == IndexedMemoryStatus::ok,
          "empty rectangle accepts an empty detached payload");
    check(memory.generation() == before_empty,
          "empty upload does not claim a backing mutation");

    const std::uint64_t before_rejection = memory.generation();
    const HERC_TEX_RECT16 bad_domain{-1, 0, 1, 1};
    check(memory.upload_texture_rect(bad_domain, {0xAAU, 0xBBU}) ==
              IndexedMemoryStatus::unsupported_coordinate_domain,
          "negative RECT coordinates are explicit unsupported domain");
    const HERC_TEX_RECT16 small_rect{0, 0, 1, 1};
    check(memory.upload_texture_rect(small_rect, {0xAAU}) ==
              IndexedMemoryStatus::payload_size_mismatch,
          "payload length must match width times height times two");
    check(memory.generation() == before_rejection,
          "rejected uploads do not advance generation");

    IndexedAddressMemory short_memory(kMapBase, 2U);
    const HERC_TEX_RECT16 crosses_supplied_end{0, 0, 2, 1};
    const std::vector<std::uint8_t> atomic_rejection{1U, 2U, 3U, 4U};
    check(short_memory.upload_texture_rect(crosses_supplied_end, atomic_rejection) ==
              IndexedMemoryStatus::outside_supplied_range,
          "all output addresses must fit the caller-supplied range");
    check(short_memory.generation() == 0U,
          "range rejection leaves generation unchanged");
    check(short_memory.read_byte(kMapBase, &value) ==
              IndexedMemoryStatus::unobserved_byte,
          "range rejection writes none of the earlier valid output bytes");

    check(memory.request_vram_rectangle_move(cross_255_256, 20, 260) ==
              IndexedMemoryStatus::deferred_vram_move,
          "VRAM move exposes deferred status");

    if (failures != 0) {
        std::printf("%d indexed address memory checks failed\n", failures);
        return 1;
    }
    std::puts("indexed address memory checks passed");
    return 0;
}
