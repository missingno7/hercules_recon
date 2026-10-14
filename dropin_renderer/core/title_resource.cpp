#include "title_resource.hpp"
#include "packet_capture.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <windows.h>
#include <bcrypt.h>

namespace hercules::dropin {
namespace {
constexpr char kImageName[] = "M:\\GRAFIX\\MOVIE\\SCREENS\\MENUB.TGA";
constexpr char kCifName[] = "M:\\GRAFIX\\MAPS\\TITLE\\MAINMENU\\MAP_PSX.CIF";
constexpr char kImageSha[] = "04e83ebe21b7fcb806ed27d5eb6de003b7188ef10b977d0c776fb9ea30129c38";
constexpr char kCifSha[] = "11d04b7e7a51b8b6a6a99fe0b5fa7e436a11ca04fedf3c0a25033d5deeb2049b";
constexpr std::uint32_t kPacketBase = 0x10000000U;
void put16(renderer_u8 *p, unsigned value)
{
    p[0] = static_cast<renderer_u8>(value); p[1] = static_cast<renderer_u8>(value >> 8);
}
void put32(renderer_u8 *p, unsigned value)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = static_cast<renderer_u8>(value >> (i * 8));
}
std::vector<renderer_u8> controlled_screen_packets()
{
    std::vector<renderer_u8> bytes(0x200U, 0);
    put32(bytes.data(), kPacketBase + 0x20U);
    const auto e1 = [&](unsigned offset, unsigned page, unsigned next) {
        put32(bytes.data() + offset, kPacketBase + next);
        bytes[offset + 4] = static_cast<renderer_u8>(page);
        bytes[offset + 7] = 0xe1;
    };
    const auto sprite = [&](unsigned offset, unsigned x, unsigned u, unsigned width, unsigned next) {
        auto *p = bytes.data() + offset;
        put32(p, kPacketBase + next); p[4] = 0x80; p[7] = 0x64;
        put16(p + 8, x); put16(p + 10, 0); p[12] = static_cast<renderer_u8>(u);
        put16(p + 16, width); put16(p + 18, 240);
    };
    e1(0x20, 20, 0x40); sprite(0x40, 0, 128, 128, 0x80);
    e1(0x80, 22, 0xa0); sprite(0xa0, 128, 0, 192, 0xe0);
    bytes[0xe7] = 0x40;
    return bytes;
}
struct HashHandles {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    ~HashHandles() {
        if (hash) BCryptDestroyHash(hash);
        if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    }
};
void require(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}
}

std::string sha256_bytes(const std::vector<std::uint8_t> &bytes)
{
    HashHandles handles;
    require(BCryptOpenAlgorithmProvider(&handles.algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0, "SHA-256 provider unavailable");
    ULONG object_size = 0, written = 0;
    require(BCryptGetProperty(handles.algorithm, BCRYPT_OBJECT_LENGTH,
                             reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size), &written, 0) >= 0,
            "SHA-256 object query failed");
    std::vector<std::uint8_t> object(object_size);
    require(BCryptCreateHash(handles.algorithm, &handles.hash, object.data(), object_size, nullptr, 0, 0) >= 0,
            "SHA-256 hash creation failed");
    require(bytes.size() <= 0xffffffffULL, "SHA-256 input exceeds bounded resource size");
    if (!bytes.empty())
        require(BCryptHashData(handles.hash, const_cast<PUCHAR>(bytes.data()), static_cast<ULONG>(bytes.size()), 0) >= 0,
                "SHA-256 input failed");
    std::array<std::uint8_t, 32> hash{};
    require(BCryptFinishHash(handles.hash, hash.data(), static_cast<ULONG>(hash.size()), 0) >= 0,
            "SHA-256 finalization failed");
    std::ostringstream text;
    text << std::hex << std::setfill('0');
    for (const auto byte : hash) text << std::setw(2) << static_cast<unsigned>(byte);
    return text.str();
}

bool load_title_screen(const std::string &archive_path, TitleScreenResource *output, std::string *error)
{
    if (output == nullptr || error == nullptr) return false;
    try {
        TitleScreenResource loaded;
        loaded.label = "MENUb / MAINMENU.CIF - controlled resource replay";
        loaded.image = read_archive_member(archive_path, kImageName, 0x14312U);
        loaded.cif = read_archive_member(archive_path, kCifName, HERCULES_CIF_REQUIRED_BYTES);
        if (loaded.image.status != ArchiveReadStatus::ok)
            throw std::runtime_error("MENUb archive read: " + loaded.image.message);
        if (loaded.cif.status != ArchiveReadStatus::ok)
            throw std::runtime_error("MAINMENU.CIF archive read: " + loaded.cif.message);
        loaded.image_sha256 = sha256_bytes(loaded.image.bytes);
        loaded.cif_sha256 = sha256_bytes(loaded.cif.bytes);
        require(loaded.image_sha256 == kImageSha && loaded.cif_sha256 == kCifSha,
                "Screen resources differ from the verified PC inputs");
        require(loaded.image.bytes.size() == 0x14312U && loaded.cif.bytes.size() == HERCULES_CIF_REQUIRED_BYTES,
                "Screen resource extent mismatch");
        /* This consumer follows TITLE's observed raw +0x312 upload. It does
         * not interpret the embedded TGA palette or flip its stored rows. */
        HerculesCifView cif{};
        require(hercules_cif_view(loaded.cif.bytes.data(), loaded.cif.bytes.size(), &cif) == HERCULES_CIF_OK,
                "CIF view failed");
        for (renderer_u32 index = 0; index < 256; ++index)
            require(hercules_cif_palette_entry(&cif, index, &loaded.palette[index]) == HERCULES_CIF_OK,
                    "CIF palette entry failed");
        IndexedAddressMemory memory(HERCULES_CIF_SHADE_ADDRESS,
                                    HERC_TEX_MAP_BASE + 0x100000U - HERCULES_CIF_SHADE_ADDRESS);
        for (renderer_u32 ordinal = 0; ordinal < HERCULES_CIF_TABLE_COUNT; ++ordinal) {
            HerculesCifSpan span{};
            require(hercules_cif_table_span(&cif, ordinal, &span) == HERCULES_CIF_OK, "CIF span failed");
            const std::vector<std::uint8_t> observed(span.bytes, span.bytes + span.length);
            require(memory.update_observed_span(span.address, observed) == IndexedMemoryStatus::ok, "CIF writer outside backing");
        }
        const std::vector<std::uint8_t> pixels(loaded.image.bytes.begin() + 0x312U, loaded.image.bytes.end());
        require(memory.upload_texture_rect({320, 256, 160, 256}, pixels) == IndexedMemoryStatus::ok,
                "TITLE upload mapping failed");
        auto bytes = controlled_screen_packets();
        FrameRaw32Region region{kPacketBase, static_cast<renderer_u32>(bytes.size()), bytes.data()};
        RendererAc60StateView state{};
        state.clip_bounds.bottom = 240;
        const auto trace = consume_offline(frame_raw32_region_resolve, &region, kPacketBase, 1, state,
            [&](const Event &event) {
                if (event.packet_kind != RENDERER_PACKET_SPRITE_VARIABLE) return true;
                IndexedSpriteCommand command;
                require(capture_indexed_sprite(event, memory, &command) == IndexedReferenceStatus::ok,
                        "Controlled screen sprite capture failed");
                IndexedReferenceEffects effects;
                require(render_indexed_sprite(command, &loaded.cpu_indices, &effects) == IndexedReferenceStatus::ok,
                        "Controlled screen sprite reference failed");
                loaded.pixel_stores += effects.pixel_store_count;
                loaded.presentation_requested = loaded.presentation_requested || effects.presentation_requested;
                loaded.commands.push_back(std::move(command));
                return true;
            });
        require(trace.status == CaptureStatus::ok && loaded.commands.size() == 2, "Screen command stream incomplete");
        for (std::size_t row = 0; row < 240; ++row)
            require(std::equal(loaded.cpu_indices.begin() + row * 320, loaded.cpu_indices.begin() + (row + 1) * 320,
                               pixels.begin() + row * 320), "Screen result differs from uploaded source rows");
        *output = std::move(loaded);
        error->clear();
        return true;
    } catch (const std::exception &failure) {
        *error = failure.what();
        return false;
    }
}

RgbaFrame title_screen_rgba(const IndexedFrame &indices,
                          const std::array<HerculesCifPaletteEntry, 256> &palette)
{
    RgbaFrame rgba(indexed_frame_bytes * 4);
    for (std::size_t i = 0; i < indices.size(); ++i) {
        const auto &entry = palette[indices[i]];
        rgba[i * 4] = entry.red; rgba[i * 4 + 1] = entry.green;
        rgba[i * 4 + 2] = entry.blue; rgba[i * 4 + 3] = 255;
    }
    return rgba;
}

} // namespace hercules::dropin
