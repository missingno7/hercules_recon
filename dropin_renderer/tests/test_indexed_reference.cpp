#include "indexed_reference.hpp"
#include "indexed_sprite_reference.hpp"
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
IndexedAddressMemory table_memory() {
    return IndexedAddressMemory(0x91c488U, HERC_TEX_MAP_BASE + 65536U - 0x91c488U);
}
std::vector<std::uint8_t> blend_bytes(unsigned selector) {
    std::vector<std::uint8_t> bytes(65536U);
    for (unsigned source = 0; source < 256; ++source)
        for (unsigned destination = 0; destination < 256; ++destination)
            bytes[source * 256 + destination] = static_cast<std::uint8_t>(
                (source * 37 + destination * 13 + selector * 53) ^ 0xb7U);
    return bytes;
}
IndexedRectangleCommand capture_blended(Fixture &fixture, IndexedAddressMemory &memory,
                                       IndexedReferenceStatus expected = IndexedReferenceStatus::ok) {
    IndexedRectangleCommand command;
    command.texels = {99};
    bool checked = false;
    const auto trace = consume_offline(frame_raw32_region_resolve, &fixture.region, 0x1000, 1, {}, [&](const Event &event) {
        if (event.packet_kind == RENDERER_PACKET_FT4) {
            check(capture_indexed_rectangle(event, memory, &command) == expected, "blended capture status");
            checked = true;
        }
        return true;
    });
    check(trace.status == CaptureStatus::ok && checked, "blended traversal");
    return command;
}
void blended_cases() {
    for (unsigned selector = 0; selector < 4; ++selector) {
        for (bool shaded : {false, true}) {
            Fixture fixture;
            fixture.bytes[0x27] = 0x2e;
            fixture.bytes[0x24] = shaded ? 0x7f : 0x80;
            put16(fixture.bytes.data() + 0x36, selector * 32);
            auto memory = table_memory();
            check(memory.upload_texture_rect({0,0,1,2}, {0,0x5a,0x77,0x6b}) == IndexedMemoryStatus::ok, "blend texels");
            const auto table = blend_bytes(selector);
            check(memory.update_observed_span(0x92e3d0U + selector * 65536U, table) == IndexedMemoryStatus::ok, "observed selected blend table");
            std::vector<std::uint8_t> shade(256);
            for (unsigned i = 0; i < 256; ++i) shade[i] = static_cast<std::uint8_t>(i ^ 0x5a);
            if (shaded)
                check(memory.update_observed_span(0x91c488U + 127 * 256, shade) == IndexedMemoryStatus::ok, "observed shade row");
            auto command = capture_blended(fixture, memory);
            check(command.plan.pixel_mode == (shaded ? HERCULES_RECTANGLE_SHADED_BLEND : HERCULES_RECTANGLE_RAW_BLEND)
                && command.plan.blend_selector == selector, "red route and tpage selector distinct");
            IndexedFrame frame; frame.fill(0xa5);
            IndexedReferenceEffects effects;
            check(render_indexed_rectangle(command, &frame, &effects) == IndexedReferenceStatus::ok, "blended CPU render");
            check(frame[321] == 0xa5 && effects.pixel_store_count == 3 && effects.presentation_requested, "source zero skipped but accepted");
            const unsigned mapped = shaded ? 0 : 0x5a;
            check(frame[322] == table[mapped * 256 + 0xa5], "nonzero shaded-to-zero still blends");
            if (shaded && selector == 0) {
                const auto old_table = command.blend_table;
                const auto clut_alias = HERC_TEX_CLUT_BASE + 32 * 256 + 64 + 0xa5;
                check(clut_alias == 0x92e3d0U + 0xa5, "CLUT row32 aliases blend row zero");
                check(memory.update_observed_span(clut_alias, {0}) == IndexedMemoryStatus::ok, "aliased CLUT observed writer");
                const auto second = capture_blended(fixture, memory);
                check(second.blend_table[0xa5] == 0 && command.blend_table == old_table,
                      "later capture sees alias update while first table snapshot remains owned");
                IndexedFrame later; later.fill(0xa5);
                check(render_indexed_rectangle(second, &later, &effects) == IndexedReferenceStatus::ok && later[322] == 0,
                      "zero LUT result is written rather than skipped");
                IndexedFrame replay; replay.fill(0xa5);
                check(render_indexed_rectangle(command, &replay, &effects) == IndexedReferenceStatus::ok && replay[322] == frame[322],
                      "old table snapshot independently replayable");
            }
            command.blend_table.pop_back();
            const auto before = frame;
            check(render_indexed_rectangle(command, &frame, &effects) == IndexedReferenceStatus::invalid_command && frame == before,
                  "truncated table rejected before destination writes");
        }
    }
    for (bool shaded : {false, true}) {
        Fixture fixture; fixture.bytes[0x27] = 0x2e; fixture.bytes[0x24] = shaded ? 0x7f : 0x80;
        auto memory = table_memory();
        check(memory.upload_texture_rect({0,0,1,2}, {0,0,0,0}) == IndexedMemoryStatus::ok, "zero blended upload");
        const auto command = capture_blended(fixture, memory);
        check(command.shade_row.empty() && command.blend_table.empty(), "zero sources do not require unobserved tables");
        IndexedFrame frame; frame.fill(0xa5); const auto before = frame;
        IndexedReferenceEffects effects;
        check(render_indexed_rectangle(command, &frame, &effects) == IndexedReferenceStatus::ok && frame == before &&
              effects.pixel_store_count == 0 && effects.presentation_requested, "transparent blended request without reads");
        check(memory.upload_texture_rect({0,0,1,2}, {0,1,0,0}) == IndexedMemoryStatus::ok, "nonzero missing-table upload");
        const auto rejected = capture_blended(fixture, memory, IndexedReferenceStatus::missing_table);
        check(rejected.texels == std::vector<std::uint8_t>({99}), "missing table failure leaves command unchanged");
    }
}
}
void sprite_capture_case() {
    Fixture fixture;
    auto *wire = fixture.bytes.data() + 0x20;
    std::fill(wire + 4, wire + 0x28, 0);
    wire[7] = 0x64; put16(wire + 8, 1); put16(wire + 10, 1);
    put16(wire + 16, 2); put16(wire + 18, 2);
    RendererAc60StateView state{}; state.clip_bounds.bottom = 240;
    IndexedAddressMemory memory(HERC_TEX_MAP_BASE, 65536);
    IndexedSpriteCommand command; command.texels = {99};
    auto consume = [&](RendererAc60StateView initial, IndexedReferenceStatus expected) {
        bool seen = false;
        auto trace = consume_offline(frame_raw32_region_resolve, &fixture.region, 0x1000, 1, initial,
            [&](const Event &event) {
                if (event.packet_kind != RENDERER_PACKET_SPRITE_VARIABLE) return true;
                check(capture_indexed_sprite(event, memory, &command) == expected, "sprite capture status");
                Event entry = event; entry.provenance = CaptureProvenance::passive_entry_observation;
                check(capture_indexed_sprite(entry, memory, &command) == IndexedReferenceStatus::unsupported,
                      "passive sprite entry cannot stand in for consumption");
                seen = true; return true;
            });
        check(trace.status == CaptureStatus::ok && seen, "sprite chain consumed");
    };
    consume(state, IndexedReferenceStatus::missing_texture);
    check(command.texels == std::vector<std::uint8_t>({99}), "failed sprite capture unchanged");
    check(memory.upload_texture_rect({0,0,1,2}, {0,1,2,3}) == IndexedMemoryStatus::ok, "sprite upload");
    consume(state, IndexedReferenceStatus::ok);
    check(memory.upload_texture_rect({0,0,1,2}, {9,9,9,9}) == IndexedMemoryStatus::ok, "later sprite upload");
    IndexedFrame frame; frame.fill(0xa5); IndexedReferenceEffects effects;
    check(render_indexed_sprite(command, &frame, &effects) == IndexedReferenceStatus::ok &&
          frame[321] == 0xa5 && frame[322] == 1 && frame[641] == 2 && frame[642] == 3 &&
          effects.pixel_store_count == 3 && effects.presentation_requested, "sprite owns consumed bytes");
    const auto saved = command.texels;
    state.raster_suppression_flag = 1;
    consume(state, IndexedReferenceStatus::no_draw);
    check(command.texels == saved, "suppression leaves owned command unchanged");
}
int main() {
    try { zero_case(); ordered_upload_case(); rejection_cases(); blended_cases(); sprite_capture_case(); }
    catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
    std::cout << "indexed reference: ordered uploads, table snapshots, alias writes, indexed blend and zero-source behavior pass\n";
    return 0;
}
