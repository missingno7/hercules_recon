#include "packet_capture.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

using hercules::dropin::CaptureLimits;
using hercules::dropin::CaptureProvenance;
using hercules::dropin::CaptureStatus;
using hercules::dropin::Event;
using hercules::dropin::EventKind;

constexpr renderer_raw32 kTable = 0x1000U;
constexpr renderer_raw32 kCommand = 0x1020U;
constexpr renderer_raw32 kFuture = 0x1040U;
constexpr renderer_raw32 kAlternate = 0x1060U;

struct Fixture {
    std::array<renderer_u8, 0x200> bytes{};
    FrameRaw32Region region{};

    Fixture()
    {
        region.base = kTable;
        region.size = static_cast<renderer_u32>(bytes.size());
        region.bytes = bytes.data();
    }

    renderer_u8 *at(renderer_raw32 address)
    {
        assert(address >= region.base);
        const renderer_u32 offset = address - region.base;
        assert(offset < bytes.size());
        return bytes.data() + offset;
    }

    void put16(renderer_raw32 address, renderer_u16 value)
    {
        renderer_u8 *p = at(address);
        p[0] = static_cast<renderer_u8>(value & 0xffU);
        p[1] = static_cast<renderer_u8>(value >> 8);
    }

    void put32(renderer_raw32 address, renderer_u32 value)
    {
        renderer_u8 *p = at(address);
        p[0] = static_cast<renderer_u8>(value & 0xffU);
        p[1] = static_cast<renderer_u8>((value >> 8) & 0xffU);
        p[2] = static_cast<renderer_u8>((value >> 16) & 0xffU);
        p[3] = static_cast<renderer_u8>((value >> 24) & 0xffU);
    }

    void make_command01(renderer_raw32 address, renderer_raw32 next,
                        renderer_i16 height)
    {
        put32(address, next);
        at(address)[7] = 0x01U;
        put16(address + 0x0cU, 15U);
        put16(address + 0x0eU, 240U);
        put16(address + 0x10U, 5U);
        put16(address + 0x12U, 224U);
        put16(address + 0x14U, 8U);
        put16(address + 0x16U, static_cast<renderer_u16>(height));
    }

    void make_e1(renderer_raw32 address, renderer_raw32 next,
                 renderer_u32 payload)
    {
        put32(address, next);
        put32(address + 4U, payload);
        at(address)[7] = 0xe1U;
    }

    void make_chain()
    {
        bytes.fill(0);
        put32(kTable, kCommand);
        make_command01(kCommand, kFuture, 7);
        make_e1(kFuture, 0, 0xe10001bbU);
        make_e1(kAlternate, 0, 0xe1000003U);
    }
};

const Event *packet_at(const std::vector<Event> &events,
                       renderer_raw32 address)
{
    for (const Event &event : events) {
        if (!event.owned_packet.empty() && event.raw_address == address)
            return &event;
    }
    return nullptr;
}

bool contains_packet(const std::vector<Event> &events,
                     renderer_raw32 address)
{
    return packet_at(events, address) != nullptr;
}

void test_live_future_payload_and_owned_copy()
{
    Fixture fixture;
    fixture.make_chain();
    RendererAc60StateView initial{};
    renderer_u32 callback_count = 0;

    const auto callback = [&](const Event &event) {
        ++callback_count;
        if (event.raw_address == kCommand) {
            assert(event.kind == EventKind::vram_move_request);
            assert(event.owned_packet.size() == 24U);
            assert(event.has_staging_result);
            assert(event.staging_status == RENDERER_AC60_APPLY_DEFERRED);
            assert(event.staged.deferred_copy.height == 7);
            /* Mutate the current live packet after its owned event was made. */
            fixture.put16(kCommand + 0x0cU, 99U);
            /* AC60-compatible traversal must read the future E1 payload only
             * when it reaches that record, after this callback returns. */
            fixture.put32(kFuture + 4U, 0xe1000042U);
        }
        return true;
    };

    CaptureLimits limits{};
    limits.max_visits = 16U;
    limits.max_events = 32U;
    const auto result = hercules::dropin::consume_offline(
        frame_raw32_region_resolve, &fixture.region, kTable, 1U, initial,
        callback, limits);

    assert(result.status == CaptureStatus::ok);
    assert(result.provenance == CaptureProvenance::controlled_offline_consumption);
    assert(callback_count == 2U);
    assert(contains_packet(result.events, kFuture));
    const Event *command = packet_at(result.events, kCommand);
    const Event *future = packet_at(result.events, kFuture);
    assert(command != nullptr && future != nullptr);
    assert(command->staged.deferred_copy.destination_x == 15);
    assert(command->owned_packet[0x0c] == 15U); // Owned copy survived mutation.
    assert(future->decoded.mode_payload == 0xe1000042U);
    assert(future->state_after.page.page_selector == 2U);
    assert(future->state_after.page.page_mode == 2U);

    renderer_u32 callback_events = 0;
    for (const Event &event : result.events) {
        if (event.kind == EventKind::callback_boundary) {
            ++callback_events;
            assert(event.origin ==
                   hercules::dropin::EventOrigin::controlled_offline_callback);
        }
    }
    assert(callback_events == 2U);
}

void test_live_link_reload_skips_replaced_successor()
{
    Fixture fixture;
    fixture.make_chain();
    RendererAc60StateView initial{};
    renderer_u32 callback_count = 0;

    const auto callback = [&](const Event &event) {
        ++callback_count;
        if (event.raw_address == kCommand)
            fixture.put32(kCommand, kAlternate);
        return true;
    };

    const auto result = hercules::dropin::consume_offline(
        frame_raw32_region_resolve, &fixture.region, kTable, 1U, initial,
        callback);
    assert(result.status == CaptureStatus::ok);
    assert(callback_count == 2U);
    assert(contains_packet(result.events, kAlternate));
    assert(!contains_packet(result.events, kFuture));
}

void test_passive_observation_is_separate()
{
    Fixture fixture;
    fixture.make_chain();
    const auto result = hercules::dropin::observe_entry(
        frame_raw32_region_resolve, &fixture.region, kTable, 1U);

    assert(result.status == CaptureStatus::ok);
    assert(result.provenance == CaptureProvenance::passive_entry_observation);
    const Event *command = packet_at(result.events, kCommand);
    assert(command != nullptr);
    assert(command->kind == EventKind::vram_move_request);
    assert(!command->has_staging_result);
    assert(command->has_deferred_move);
    assert(command->deferred_move.height == 7);
    const renderer_u8 captured_byte = command->owned_packet[0x0c];
    fixture.put16(kCommand + 0x0cU, 77U);
    assert(command->owned_packet[0x0c] == captured_byte);
}

void test_sixteen_byte_command01_is_rejected()
{
    Fixture fixture;
    fixture.make_chain();
    /* The table and packet header/direct envelope are readable, but the
     * transitive rectangle ends at +0x18 and is unavailable. */
    fixture.region.size = 0x30U;
    const auto result = hercules::dropin::observe_entry(
        frame_raw32_region_resolve, &fixture.region, kTable, 1U);

    assert(result.status == CaptureStatus::malformed_packet);
    assert(result.events.size() == 1U);
    assert(result.events[0].kind == EventKind::unsupported);
    assert(result.events[0].owned_packet.size() == 8U);
}

void test_unknown_packet_is_explicit()
{
    Fixture fixture;
    fixture.bytes.fill(0);
    fixture.put32(kTable, kCommand);
    fixture.put32(kCommand, 0);
    fixture.at(kCommand)[7] = 0x55U;

    const auto result = hercules::dropin::observe_entry(
        frame_raw32_region_resolve, &fixture.region, kTable, 1U);
    assert(result.status == CaptureStatus::complete_with_unsupported);
    const Event *unknown = packet_at(result.events, kCommand);
    assert(unknown != nullptr);
    assert(unknown->kind == EventKind::unsupported);
    assert(unknown->decode_status == RENDERER_PACKET_UNRECOGNIZED);
    assert(unknown->owned_packet.size() == 8U);
}

void test_polygon_references_are_not_mutations()
{
    Fixture fixture;
    fixture.bytes.fill(0);
    fixture.put32(kTable, kCommand);
    fixture.put32(kCommand, 0);
    fixture.at(kCommand)[4] = 0x64U;
    fixture.at(kCommand)[7] = 0x2cU;
    fixture.put16(kCommand + 0x0eU, 0x456U);
    fixture.put16(kCommand + 0x16U, 0x123U);

    const auto result = hercules::dropin::observe_entry(
        frame_raw32_region_resolve, &fixture.region, kTable, 1U);
    assert(result.status == CaptureStatus::ok);
    const Event *polygon = packet_at(result.events, kCommand);
    assert(polygon != nullptr && polygon->kind == EventKind::polygon);
    bool saw_texture_reference = false;
    bool saw_palette_reference = false;
    for (const Event &event : result.events) {
        if (event.related_packet != kCommand)
            continue;
        if (event.kind == EventKind::texture_reference) {
            saw_texture_reference = true;
            assert(event.decoded.tpage == 0x123U);
        } else if (event.kind == EventKind::palette_reference) {
            saw_palette_reference = true;
            assert(event.decoded.clut == 0x456U);
        }
        assert(event.kind != EventKind::texture_mutation);
        assert(event.kind != EventKind::palette_mutation);
        assert(event.kind != EventKind::surface_clear);
        assert(event.kind != EventKind::present_request);
    }
    assert(saw_texture_reference && saw_palette_reference);
}

} // namespace

int main()
{
    test_live_future_payload_and_owned_copy();
    test_live_link_reload_skips_replaced_successor();
    test_passive_observation_is_separate();
    test_sixteen_byte_command01_is_rejected();
    test_unknown_packet_is_explicit();
    test_polygon_references_are_not_mutations();
    std::cout << "drop-in command capture checks passed\n";
    return 0;
}
