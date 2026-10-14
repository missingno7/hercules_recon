#include "indexed_reference.hpp"
#include "packet_capture.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

using namespace hercules::dropin;
namespace {
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
void put16(renderer_u8 *p, unsigned value) { p[0] = static_cast<renderer_u8>(value); p[1] = static_cast<renderer_u8>(value >> 8); }
void put32(renderer_u8 *p, unsigned value) { for (unsigned i = 0; i < 4; ++i) p[i] = static_cast<renderer_u8>(value >> (i * 8)); }
struct Fixture {
    std::array<renderer_u8, 0x200> bytes{};
    FrameRaw32Region region{0x1000U, static_cast<renderer_u32>(bytes.size()), bytes.data()};
    void packet(unsigned offset, unsigned x, unsigned next, bool asymmetric = false) {
        auto *p = bytes.data() + offset;
        put32(p, next); p[4] = 0x64; p[7] = 0x2c;
        put16(p + 8, x); put16(p + 10, 1);
        put16(p + 16, x + 2); put16(p + 18, 1); p[20] = 2;
        put16(p + 24, x); put16(p + 26, 3); p[28] = asymmetric ? 1 : 0; p[29] = 2;
        put16(p + 32, x + 2); put16(p + 34, 3); p[36] = 2; p[37] = 2;
    }
    Fixture(bool two = false, bool asymmetric = false) {
        put32(bytes.data(), 0x1020U);
        packet(0x20, 1, two ? 0x1060U : 0x10a0U, asymmetric);
        if (two) packet(0x60, 5, 0x10a0U);
        bytes[0xa7] = 0x40;
    }
};
void zero_case() {
    Fixture fixture;
    IndexedAddressMemory memory(HERC_TEX_MAP_BASE, 65536);
    check(memory.upload_texture_rect({0,0,1,2}, {0,0,0,0}) == IndexedMemoryStatus::ok, "zero upload");
    IndexedFrame frame, initial; frame.fill(0xa5); initial = frame;
    IndexedReferenceEffects effects;
    bool captured = false;
    auto trace = consume_offline(frame_raw32_region_resolve, &fixture.region, 0x1000, 1, {}, [&](const Event &event) {
        if (event.packet_kind != RENDERER_PACKET_FT4) return true;
        IndexedRectangleCommand command;
        check(capture_indexed_rectangle(event, memory, &command) == IndexedReferenceStatus::ok, "zero capture");
        check(render_indexed_rectangle(command, &frame, &effects) == IndexedReferenceStatus::ok, "zero render");
        check(command.texels.size() == 4, "only consumed samples owned"); captured = true; return true;
    });
    check(trace.status == CaptureStatus::ok && captured, "completed zero trace");
    check(frame == initial && effects.pixel_store_count == 0 && effects.presentation_requested, "zero stores still requests presentation");
}
void ordered_upload_case() {
    Fixture fixture(true);
    IndexedAddressMemory memory(HERC_TEX_MAP_BASE, 65536);
    check(memory.upload_texture_rect({0,0,1,2}, {0,0x5a,0x77,0x6b}) == IndexedMemoryStatus::ok, "first upload");
    IndexedFrame frame; frame.fill(0xa5);
    IndexedRectangleCommand first;
    unsigned count = 0;
    auto trace = consume_offline(frame_raw32_region_resolve, &fixture.region, 0x1000, 1, {}, [&](const Event &event) {
        if (event.packet_kind != RENDERER_PACKET_FT4) return true;
        IndexedRectangleCommand command;
        check(capture_indexed_rectangle(event, memory, &command) == IndexedReferenceStatus::ok, "ordered capture");
        IndexedReferenceEffects effects;
        check(render_indexed_rectangle(command, &frame, &effects) == IndexedReferenceStatus::ok, "ordered render");
        if (++count == 1) {
            first = command;
            check(effects.pixel_store_count == 3, "first store count");
            check(memory.upload_texture_rect({0,0,1,2}, {1,2,3,4}) == IndexedMemoryStatus::ok, "between-command upload");
        } else {
            check(command.texels == std::vector<std::uint8_t>({1,2,3,4}), "next command observes upload");
            check(command.texture_generation > first.texture_generation, "separate memory observation generations");
        }
        return true;
    });
    check(trace.status == CaptureStatus::ok && count == 2, "two ordered commands");
    check(first.texels == std::vector<std::uint8_t>({0,0x5a,0x77,0x6b}), "detached old tile retained");
    check(frame[321] == 0xa5 && frame[322] == 0x5a && frame[641] == 0x77 && frame[642] == 0x6b, "first rectangle indices");
    check(frame[325] == 1 && frame[326] == 2 && frame[645] == 3 && frame[646] == 4, "second rectangle indices");
    IndexedFrame replay; replay.fill(0xa5); IndexedReferenceEffects effects;
    check(render_indexed_rectangle(first, &replay, &effects) == IndexedReferenceStatus::ok && replay[322] == 0x5a, "old snapshot independently renderable");
    first.sample_addresses[0] += 1;
    const auto before = replay;
    check(render_indexed_rectangle(first, &replay, &effects) == IndexedReferenceStatus::invalid_command && replay == before, "bad association rejected before writes");
}
void rejection_cases() {
    IndexedAddressMemory memory(HERC_TEX_MAP_BASE, 65536);
    Fixture fixture;
    bool checked = false;
    consume_offline(frame_raw32_region_resolve, &fixture.region, 0x1000, 1, {}, [&](const Event &event) {
        if (event.packet_kind == RENDERER_PACKET_FT4) {
            IndexedRectangleCommand command; command.texels = {99};
            check(capture_indexed_rectangle(event, memory, &command) == IndexedReferenceStatus::missing_texture, "unknown bytes not zero");
            check(command.texels == std::vector<std::uint8_t>({99}), "failed capture leaves output unchanged");
            checked = true;
        }
        return true;
    });
    check(checked, "missing-byte case reached");
    check(memory.upload_texture_rect({0,0,1,2}, {1,2,3,4}) == IndexedMemoryStatus::ok, "fixture upload");
    Fixture asymmetric(false, true);
    consume_offline(frame_raw32_region_resolve, &asymmetric.region, 0x1000, 1, {}, [&](const Event &event) {
        if (event.packet_kind == RENDERER_PACKET_FT4) {
            IndexedRectangleCommand command;
            check(capture_indexed_rectangle(event, memory, &command) == IndexedReferenceStatus::unsupported, "generic UV route explicitly deferred");
        }
        return true;
    });
    const auto passive = observe_entry(frame_raw32_region_resolve, &fixture.region, 0x1000, 1);
    IndexedRectangleCommand command;
    check(capture_indexed_rectangle(passive.events.front(), memory, &command) == IndexedReferenceStatus::unsupported, "entry snapshot not consumed-state proof");
}
}
int main() {
    try { zero_case(); ordered_upload_case(); rejection_cases(); }
    catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
    std::cout << "indexed reference: ordered uploads, owned tiles, transparency and rejection pass\n";
    return 0;
}
