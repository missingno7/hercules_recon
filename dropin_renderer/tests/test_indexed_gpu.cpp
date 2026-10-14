#include "indexed_gpu.hpp"

#include "packet_capture.hpp"
#include "indexed_memory.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace {

using hercules::dropin::CaptureProvenance;
using hercules::dropin::CaptureStatus;
using hercules::dropin::Event;
using hercules::dropin::EventKind;
using hercules::dropin::IndexedAddressMemory;
using hercules::dropin::IndexedFrame;
using hercules::dropin::IndexedGpuBackend;
using hercules::dropin::IndexedGpuStatus;
using hercules::dropin::IndexedRectangleCommand;
using hercules::dropin::IndexedReferenceEffects;
using hercules::dropin::IndexedReferenceStatus;

constexpr renderer_raw32 kOrderTable = 0x00010000U;
constexpr renderer_raw32 kPacket = 0x00010100U;
constexpr renderer_raw32 kTerminator = 0x00010200U;

struct Region {
    renderer_raw32 address;
    const renderer_u8 *bytes;
    renderer_u32 length;
};

struct ResolverContext {
    std::array<Region, 3> regions{};
};

int resolve_region(void *opaque, renderer_raw32 address,
                   renderer_u32 byte_count,
                   const renderer_u8 **resolved_bytes)
{
    const auto *context = static_cast<const ResolverContext *>(opaque);
    if (context == nullptr || resolved_bytes == nullptr)
        return 0;
    for (const Region &region : context->regions) {
        if (address < region.address)
            continue;
        const std::uint64_t offset =
            static_cast<std::uint64_t>(address) - region.address;
        if (offset <= region.length &&
            byte_count <= region.length - offset) {
            *resolved_bytes = region.bytes + offset;
            return 1;
        }
    }
    return 0;
}

void write_u16(renderer_u8 *bytes, std::size_t offset, std::uint16_t value)
{
    bytes[offset + 0] = static_cast<renderer_u8>(value & 0xffU);
    bytes[offset + 1] = static_cast<renderer_u8>(value >> 8);
}

void write_i16(renderer_u8 *bytes, std::size_t offset, std::int16_t value)
{
    write_u16(bytes, offset, static_cast<std::uint16_t>(value));
}

void write_u32(renderer_u8 *bytes, std::size_t offset, std::uint32_t value)
{
    bytes[offset + 0] = static_cast<renderer_u8>(value & 0xffU);
    bytes[offset + 1] = static_cast<renderer_u8>((value >> 8) & 0xffU);
    bytes[offset + 2] = static_cast<renderer_u8>((value >> 16) & 0xffU);
    bytes[offset + 3] = static_cast<renderer_u8>((value >> 24) & 0xffU);
}

struct Fixture {
    std::array<renderer_u8, 4> order_table{};
    std::array<renderer_u8, 0x28> packet{};
    std::array<renderer_u8, 8> terminator{};
    ResolverContext resolver{};
    Event consumed_ft4{};
};

bool build_consumed_ft4(renderer_i16 x, renderer_i16 y,
                        renderer_u32 width, renderer_u32 height,
                        renderer_u8 u, renderer_u8 v,
                        Fixture *fixture, std::string *error)
{
    if (fixture == nullptr || error == nullptr || width == 0U || height == 0U ||
        static_cast<std::uint32_t>(u) + width > 256U ||
        static_cast<std::uint32_t>(v) + height > 256U) {
        if (error != nullptr)
            *error = "invalid test fixture arguments";
        return false;
    }
    *fixture = Fixture{};
    write_u32(fixture->order_table.data(), 0, kPacket);
    write_u32(fixture->packet.data(), 0, kTerminator);
    fixture->packet[4] = 0x64U;
    fixture->packet[5] = 0U;
    fixture->packet[6] = 0U;
    fixture->packet[7] = 0x2cU;
    fixture->terminator[7] = 0x40U;

    const std::array<std::size_t, 4> xy_offsets = {0x08, 0x10, 0x18, 0x20};
    const std::array<std::size_t, 4> uv_offsets = {0x0c, 0x14, 0x1c, 0x24};
    const std::array<std::int16_t, 4> xs = {
        x, static_cast<std::int16_t>(x + width),
        x, static_cast<std::int16_t>(x + width)
    };
    const std::array<std::int16_t, 4> ys = {
        y, y, static_cast<std::int16_t>(y + height),
        static_cast<std::int16_t>(y + height)
    };
    const std::array<std::uint8_t, 4> us = {
        u, static_cast<std::uint8_t>(u + width),
        u, static_cast<std::uint8_t>(u + width)
    };
    const std::array<std::uint8_t, 4> vs = {
        v, v, static_cast<std::uint8_t>(v + height),
        static_cast<std::uint8_t>(v + height)
    };
    for (std::size_t i = 0; i < 4; ++i) {
        write_i16(fixture->packet.data(), xy_offsets[i], xs[i]);
        write_i16(fixture->packet.data(), xy_offsets[i] + 2, ys[i]);
        fixture->packet[uv_offsets[i]] = us[i];
        fixture->packet[uv_offsets[i] + 1] = vs[i];
    }
    write_u16(fixture->packet.data(), 0x0e, 0U);
    write_u16(fixture->packet.data(), 0x16, 0U);
    write_u16(fixture->packet.data(), 0x1e, 0U);
    write_u16(fixture->packet.data(), 0x26, 0U);
    fixture->resolver.regions = {{
        {kOrderTable, fixture->order_table.data(),
         static_cast<renderer_u32>(fixture->order_table.size())},
        {kPacket, fixture->packet.data(),
         static_cast<renderer_u32>(fixture->packet.size())},
        {kTerminator, fixture->terminator.data(),
         static_cast<renderer_u32>(fixture->terminator.size())}
    }};

    std::optional<Event> captured;
    const hercules::dropin::PacketCallback callback =
        [&captured](const Event &event) {
            if (event.kind == EventKind::polygon &&
                event.packet_kind == RENDERER_PACKET_FT4)
                captured = event;
            return true;
        };
    RendererAc60StateView initial_state{};
    const auto consumption = hercules::dropin::consume_offline(
        resolve_region, &fixture->resolver, kOrderTable, 1U,
        initial_state, callback);
    if (consumption.status != CaptureStatus::ok || !captured.has_value() ||
        captured->provenance != CaptureProvenance::controlled_offline_consumption ||
        !captured->has_state_after || !captured->has_staging_result) {
        *error = "synthetic AC60 FT4 was not synchronously consumed and staged";
        return false;
    }
    fixture->consumed_ft4 = std::move(*captured);
    return true;
}

std::uint8_t texture_value(std::uint32_t u, std::uint32_t v)
{
    return static_cast<std::uint8_t>(1U + ((u * 29U + v * 47U) % 255U));
}

bool capture_snapshot(renderer_i16 x, renderer_i16 y,
                      renderer_u32 width, renderer_u32 height,
                      renderer_u8 u, renderer_u8 v,
                      bool include_zero,
                      Fixture *fixture, IndexedAddressMemory *memory,
                      IndexedRectangleCommand *command,
                      std::string *error, bool all_zero = false)
{
    if (!build_consumed_ft4(x, y, width, height, u, v, fixture, error))
        return false;
    for (renderer_u32 row = 0; row < height; ++row) {
        std::vector<std::uint8_t> texels;
        texels.reserve(width);
        for (renderer_u32 column = 0; column < width; ++column) {
            const bool zero_pixel = all_zero ||
                (include_zero && row == 0U && column == 1U);
            texels.push_back(zero_pixel ? 0U : texture_value(u + column, v + row));
        }
        const std::uint32_t address = HERC_TEX_MAP_BASE +
            (v + row) * 256U + u;
        if (memory->update_observed_span(address, texels) !=
            hercules::dropin::IndexedMemoryStatus::ok) {
            *error = "could not seed observed synthetic texture texels";
            return false;
        }
    }
    if (hercules::dropin::capture_indexed_rectangle(
            fixture->consumed_ft4, *memory, command) !=
        IndexedReferenceStatus::ok) {
        *error = "shared rectangle resolver could not snapshot the consumed FT4";
        return false;
    }
    if (command->plan.x != static_cast<renderer_u32>(x) ||
        command->plan.y != static_cast<renderer_u32>(y) ||
        command->plan.width != width || command->plan.height != height ||
        command->plan.u != u || command->plan.v != v) {
        *error = "shared rectangle plan differs from the source fixture";
        return false;
    }
    return true;
}

bool run_positive_case(IndexedGpuBackend *backend, const char *name,
                       renderer_i16 x, renderer_i16 y,
                       renderer_u32 width, renderer_u32 height,
                       renderer_u8 u, renderer_u8 v,
                       bool include_zero, bool *saw_preserved_zero,
                       std::string *error)
{
    Fixture fixture{};
    IndexedAddressMemory memory(HERC_TEX_MAP_BASE, 65536U);
    IndexedRectangleCommand command{};
    if (!capture_snapshot(x, y, width, height, u, v, include_zero,
                          &fixture, &memory, &command, error))
        return false;

    IndexedFrame initial{};
    for (std::size_t i = 0; i < initial.size(); ++i)
        initial[i] = static_cast<std::uint8_t>((i * 13U + 7U) & 0xffU);
    if (include_zero) {
        const std::size_t zero_destination =
            static_cast<std::size_t>(y) * hercules::dropin::indexed_frame_width +
            static_cast<std::size_t>(x) + 1U;
        initial[zero_destination] = 0xd3U;
    }
    const IndexedFrame seeded = initial;

    IndexedFrame expected = initial;
    IndexedReferenceEffects effects{};
    if (hercules::dropin::render_indexed_rectangle(command, &expected, &effects) !=
            IndexedReferenceStatus::ok ||
        !effects.presentation_requested) {
        *error = std::string(name) + ": shared CPU CAB0 oracle rejected the snapshot";
        return false;
    }
    const auto result = backend->render(command, initial);
    if (result.status != IndexedGpuStatus::ok) {
        *error = std::string(name) + ": GPU render failed: " + result.message;
        return false;
    }
    if (result.indices != expected) {
        std::size_t mismatch = 0;
        while (mismatch < expected.size() &&
               result.indices[mismatch] == expected[mismatch])
            ++mismatch;
        char detail[192]{};
        const std::size_t x_pixel = mismatch % hercules::dropin::indexed_frame_width;
        const std::size_t y_pixel = mismatch / hercules::dropin::indexed_frame_width;
        std::snprintf(detail, sizeof(detail),
            "%s: full-frame readback differs at (%zu,%zu), GPU=%u CPU=%u",
            name, x_pixel, y_pixel,
            static_cast<unsigned>(result.indices[mismatch]),
            static_cast<unsigned>(expected[mismatch]));
        *error = detail;
        return false;
    }
    if (include_zero) {
        const std::size_t zero_destination =
            static_cast<std::size_t>(y) * hercules::dropin::indexed_frame_width +
            static_cast<std::size_t>(x) + 1U;
        if (command.texels.size() <= 1U || command.texels[1] != 0U ||
            seeded[zero_destination] != 0xd3U ||
            result.indices[zero_destination] != seeded[zero_destination]) {
            *error = std::string(name) +
                ": zero source did not preserve the seeded destination index";
            return false;
        }
        *saw_preserved_zero = true;
    }
    std::printf("PASS %s: rect=(%u,%u %ux%u) uv=(%u,%u), stores=%u, full-frame indexed equality\n",
        name, command.plan.x, command.plan.y, command.plan.width,
        command.plan.height, command.plan.u, command.plan.v,
        effects.pixel_store_count);
    return true;
}

void configure_blend(IndexedRectangleCommand *command, renderer_u32 mode,
                    renderer_u32 selector, renderer_u32 row,
                    bool shade_to_zero = false)
{
    command->plan.pixel_mode = mode;
    command->plan.blend_selector = selector;
    command->plan.shade_row = row;
    command->shade_row.clear();
    command->blend_table.clear();

    const bool has_nonzero_source = std::any_of(
        command->texels.begin(), command->texels.end(),
        [](std::uint8_t value) { return value != 0U; });
    if (!has_nonzero_source || mode == HERCULES_RECTANGLE_COPY)
        return;

    command->blend_table.resize(256U * 256U);
    for (std::uint32_t source = 0; source < 256U; ++source) {
        for (std::uint32_t destination = 0; destination < 256U; ++destination) {
            std::uint32_t value = source * 0x9e3779b9U;
            value ^= destination * 0x85ebca6bU;
            value ^= (selector + 1U) * 0xc2b2ae35U;
            value ^= value >> 16;
            value *= 0x7feb352dU;
            value ^= value >> 15;
            value *= 0x846ca68bU;
            value ^= value >> 16;
            if (shade_to_zero && source == 0U)
                value = 0x5aU + selector * 17U;
            command->blend_table[(source << 8) | destination] =
                static_cast<std::uint8_t>(value & 0xffU);
        }
    }

    if (mode == HERCULES_RECTANGLE_SHADED_BLEND) {
        command->shade_row.resize(256U);
        for (std::uint32_t source = 0; source < 256U; ++source) {
            const std::uint32_t mapped =
                (source * 37U + selector * 53U + 19U) & 0xffU;
            command->shade_row[source] = static_cast<std::uint8_t>(
                shade_to_zero ? 0U : mapped);
        }
    }
}

bool compare_full_frame(const char *name, const IndexedFrame &actual,
                        const IndexedFrame &expected, std::string *error)
{
    if (actual == expected)
        return true;
    std::size_t mismatch = 0;
    while (mismatch < expected.size() && actual[mismatch] == expected[mismatch])
        ++mismatch;
    char detail[224]{};
    const std::size_t x_pixel = mismatch % hercules::dropin::indexed_frame_width;
    const std::size_t y_pixel = mismatch / hercules::dropin::indexed_frame_width;
    std::snprintf(detail, sizeof(detail),
        "%s: full-frame readback differs at (%zu,%zu), GPU=%u CPU=%u",
        name, x_pixel, y_pixel, static_cast<unsigned>(actual[mismatch]),
        static_cast<unsigned>(expected[mismatch]));
    *error = detail;
    return false;
}

bool run_blend_case(IndexedGpuBackend *backend, const char *name,
                    renderer_i16 x, renderer_i16 y,
                    renderer_u32 width, renderer_u32 height,
                    renderer_u8 u, renderer_u8 v,
                    renderer_u32 mode, renderer_u32 selector,
                    bool include_zero, bool all_zero,
                    bool shade_to_zero, std::string *error)
{
    Fixture fixture{};
    IndexedAddressMemory memory(HERC_TEX_MAP_BASE, 65536U);
    IndexedRectangleCommand command{};
    if (!capture_snapshot(x, y, width, height, u, v, include_zero,
                          &fixture, &memory, &command, error, all_zero))
        return false;
    const renderer_u32 row = mode == HERCULES_RECTANGLE_SHADED_BLEND
        ? 37U : 0U;
    configure_blend(&command, mode, selector, row, shade_to_zero);

    IndexedFrame initial{};
    for (std::size_t i = 0; i < initial.size(); ++i)
        initial[i] = static_cast<std::uint8_t>((i * 31U + 0x63U) & 0xffU);
    const std::size_t transparent_destination =
        static_cast<std::size_t>(y) * hercules::dropin::indexed_frame_width +
        static_cast<std::size_t>(x) + 1U;
    if (include_zero || all_zero)
        initial[transparent_destination] = 0xd3U;

    IndexedFrame expected = initial;
    IndexedReferenceEffects effects{};
    if (hercules::dropin::render_indexed_rectangle(command, &expected, &effects) !=
            IndexedReferenceStatus::ok || !effects.presentation_requested) {
        *error = std::string(name) + ": CPU reference rejected the blend snapshot";
        return false;
    }
    const auto result = backend->render(command, initial);
    if (result.status != IndexedGpuStatus::ok) {
        *error = std::string(name) + ": GPU render failed: " + result.message;
        return false;
    }
    if (!compare_full_frame(name, result.indices, expected, error))
        return false;
    if ((include_zero || all_zero) &&
        result.indices[transparent_destination] != initial[transparent_destination]) {
        *error = std::string(name) + ": zero source changed its seeded destination";
        return false;
    }
    const std::size_t nonzero_sources = static_cast<std::size_t>(std::count_if(
        command.texels.begin(), command.texels.end(),
        [](std::uint8_t value) { return value != 0U; }));
    if (effects.pixel_store_count != nonzero_sources) {
        *error = std::string(name) + ": CPU reference did not store once per nonzero source";
        return false;
    }
    std::printf("PASS %s: mode=%u selector=%u source-zero=%u shade-zero=%u stores=%u, full-frame indexed equality\n",
        name, mode, selector,
        static_cast<unsigned>(include_zero || all_zero),
        static_cast<unsigned>(shade_to_zero), effects.pixel_store_count);
    return true;
}

bool run_overlapping_blends(IndexedGpuBackend *backend, std::string *error)
{
    Fixture first_fixture{};
    Fixture second_fixture{};
    IndexedAddressMemory first_memory(HERC_TEX_MAP_BASE, 65536U);
    IndexedAddressMemory second_memory(HERC_TEX_MAP_BASE, 65536U);
    IndexedRectangleCommand first{};
    IndexedRectangleCommand second{};
    if (!capture_snapshot(9, 8, 4, 2, 6, 3, true, &first_fixture,
                          &first_memory, &first, error) ||
        !capture_snapshot(11, 8, 4, 2, 12, 7, false, &second_fixture,
                          &second_memory, &second, error))
        return false;
    configure_blend(&first, HERCULES_RECTANGLE_RAW_BLEND, 1U, 0U);
    configure_blend(&second, HERCULES_RECTANGLE_SHADED_BLEND, 3U, 41U);

    IndexedFrame initial{};
    for (std::size_t i = 0; i < initial.size(); ++i)
        initial[i] = static_cast<std::uint8_t>((i * 17U + 0x21U) & 0xffU);
    initial[8U * hercules::dropin::indexed_frame_width + 10U] = 0xe7U;
    IndexedFrame expected_after_first = initial;
    IndexedReferenceEffects first_effects{};
    if (hercules::dropin::render_indexed_rectangle(
            first, &expected_after_first, &first_effects) !=
            IndexedReferenceStatus::ok) {
        *error = "overlap first draw: CPU reference rejected RAW_BLEND";
        return false;
    }
    const auto first_result = backend->render(first, initial);
    if (first_result.status != IndexedGpuStatus::ok ||
        !compare_full_frame("overlap first draw", first_result.indices,
                            expected_after_first, error)) {
        if (first_result.status != IndexedGpuStatus::ok)
            *error = "overlap first draw: GPU render failed: " + first_result.message;
        return false;
    }

    IndexedFrame expected_after_second = expected_after_first;
    IndexedReferenceEffects second_effects{};
    if (hercules::dropin::render_indexed_rectangle(
            second, &expected_after_second, &second_effects) !=
            IndexedReferenceStatus::ok) {
        *error = "overlap second draw: CPU reference rejected SHADED_BLEND";
        return false;
    }
    const auto second_result = backend->render(second, first_result.indices);
    if (second_result.status != IndexedGpuStatus::ok) {
        *error = "overlap second draw: GPU render failed: " + second_result.message;
        return false;
    }
    if (!compare_full_frame("overlap second draw fed from prior GPU result",
                            second_result.indices, expected_after_second, error))
        return false;
    std::puts("PASS overlapping RAW_BLEND then SHADED_BLEND fed from prior GPU readback");
    return true;
}

bool expect_unsupported(IndexedGpuBackend *backend,
                        const IndexedRectangleCommand &command,
                        const char *case_name, std::string *error)
{
    IndexedFrame seed{};
    const auto result = backend->render(command, seed);
    if (result.status != IndexedGpuStatus::unsupported ||
        result.message.find("unsupported:") != 0U) {
        *error = std::string(case_name) +
            ": malformed command was not explicitly rejected";
        return false;
    }
    std::printf("PASS malformed GPU command %s: %s\n",
        case_name, result.message.c_str());
    return true;
}

bool check_malformed_blend_commands(IndexedGpuBackend *backend,
                                    const IndexedRectangleCommand &valid_raw,
                                    std::string *error)
{
    IndexedRectangleCommand malformed = valid_raw;
    malformed.blend_table.pop_back();
    if (!expect_unsupported(backend, malformed, "short selected LUT", error))
        return false;
    IndexedRectangleCommand valid_shaded = valid_raw;
    configure_blend(&valid_shaded, HERCULES_RECTANGLE_SHADED_BLEND, 2U, 17U);
    malformed = valid_shaded;
    malformed.shade_row.pop_back();
    if (!expect_unsupported(backend, malformed, "short selected shade row", error))
        return false;
    malformed = valid_raw;
    malformed.plan.pixel_mode = 0xffffffffU;
    if (!expect_unsupported(backend, malformed, "unknown pixel mode", error))
        return false;
    malformed = valid_raw;
    malformed.plan.blend_selector = 4U;
    if (!expect_unsupported(backend, malformed, "blend selector outside 0..3", error))
        return false;
    malformed = valid_raw;
    malformed.plan.shade_row = 1U;
    if (!expect_unsupported(backend, malformed, "RAW_BLEND shade row", error))
        return false;
    malformed = valid_raw;
    malformed.plan.pixel_mode = HERCULES_RECTANGLE_SHADED_BLEND;
    malformed.plan.shade_row = 0x80U;
    if (!expect_unsupported(backend, malformed, "neutral red used as SHADED_BLEND", error))
        return false;
    malformed = valid_raw;
    malformed.plan.pixel_mode = HERCULES_RECTANGLE_COPY;
    if (!expect_unsupported(backend, malformed, "COPY with LUT payload", error))
        return false;
    return true;
}

bool check_explicit_unsupported(IndexedGpuBackend *backend,
                                const IndexedRectangleCommand &valid_command,
                                const IndexedFrame &seeded,
                                std::string *error)
{
    IndexedRectangleCommand unsupported = valid_command;
    unsupported.plan.width = 9U;
    unsupported.texels.resize(unsupported.plan.width * unsupported.plan.height);
    unsupported.sample_addresses.resize(unsupported.texels.size());
    const IndexedFrame before = seeded;
    const auto result = backend->render(unsupported, seeded);
    if (result.status != IndexedGpuStatus::unsupported ||
        result.message.find("unsupported:") != 0U || seeded != before) {
        *error = "out-of-domain rectangle did not return an explicit unsupported result";
        return false;
    }
    std::printf("PASS unsupported rectangle: %s\n", result.message.c_str());
    return true;
}

int skip_gpu(const char *reason)
{
    std::fprintf(stderr, "SKIP: %s\n", reason);
    return 77;
}

} // namespace

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        const std::string reason =
            std::string("SDL video initialization failed: ") + SDL_GetError();
        return skip_gpu(reason.c_str());
    }

    const SDL_GPUShaderFormat shader_formats = SDL_GPU_SHADERFORMAT_DXBC;
    SDL_GPUDevice *device = SDL_CreateGPUDevice(shader_formats, false,
                                                "direct3d12");
    if (device == nullptr) {
        char reason[512]{};
        std::snprintf(reason, sizeof(reason),
            "no Direct3D 12 GPU adapter/device is available: %s", SDL_GetError());
        SDL_Quit();
        return skip_gpu(reason);
    }
    const char *driver = SDL_GetGPUDeviceDriver(device);
    const SDL_GPUShaderFormat available = SDL_GetGPUShaderFormats(device);
    if (driver == nullptr || std::strcmp(driver, "direct3d12") != 0 ||
        (available & SDL_GPU_SHADERFORMAT_DXBC) == 0) {
        SDL_DestroyGPUDevice(device);
        SDL_Quit();
        return skip_gpu("selected adapter does not expose the required D3D12/DXBC path");
    }
    std::printf("SDL GPU device: driver=%s shader_formats=0x%08x\n",
        driver, static_cast<unsigned>(available));

    int exit_code = 0;
    {
        IndexedGpuBackend backend(device);
        if (backend.initialization_status() == IndexedGpuStatus::unsupported) {
            const std::string reason = "R8_UINT path blocker: " +
                backend.initialization_message();
            std::fprintf(stderr, "BLOCKED: %s\n", reason.c_str());
            exit_code = 77;
        } else if (backend.initialization_status() != IndexedGpuStatus::ok) {
            std::fprintf(stderr, "FAIL: indexed GPU backend initialization: %s\n",
                backend.initialization_message().c_str());
            exit_code = 1;
        } else {
            std::printf("Indexed GPU backend: %s\n",
                backend.initialization_message().c_str());
            bool saw_preserved_zero = false;
            std::string error;
            if (!run_positive_case(&backend, "base UV",
                    1, 1, 2, 2, 0, 0, true, &saw_preserved_zero, &error) ||
                !run_positive_case(&backend, "shifted UV",
                    4, 5, 3, 2, 2, 1, false, &saw_preserved_zero, &error)) {
                std::fprintf(stderr, "FAIL: %s\n", error.c_str());
                exit_code = 1;
            } else if (!saw_preserved_zero) {
                std::fprintf(stderr, "FAIL: zero-source preservation case was not exercised\n");
                exit_code = 1;
            } else {
                Fixture fixture{};
                IndexedAddressMemory memory(HERC_TEX_MAP_BASE, 65536U);
                IndexedRectangleCommand command{};
                IndexedFrame seeded{};
                if (!capture_snapshot(1, 1, 2, 2, 0, 0, true,
                                      &fixture, &memory, &command, &error) ||
                    !check_explicit_unsupported(&backend, command, seeded, &error)) {
                    std::fprintf(stderr, "FAIL: %s\n", error.c_str());
                    exit_code = 1;
                } else {
                    bool blend_cases_ok = true;
                    for (renderer_u32 selector = 0; selector < 4U && blend_cases_ok;
                         ++selector) {
                        char case_name[80]{};
                        std::snprintf(case_name, sizeof(case_name),
                            "red-neutral RAW_BLEND selector %u", selector);
                        blend_cases_ok = run_blend_case(&backend, case_name,
                            7, 9, 3, 2, 4, 6,
                            HERCULES_RECTANGLE_RAW_BLEND, selector,
                            true, false, false, &error);
                        if (!blend_cases_ok)
                            break;
                        std::snprintf(case_name, sizeof(case_name),
                            "SHADED_BLEND selector %u", selector);
                        blend_cases_ok = run_blend_case(&backend, case_name,
                            13, 11, 3, 2, 8, 5,
                            HERCULES_RECTANGLE_SHADED_BLEND, selector,
                            true, false, false, &error);
                    }
                    if (blend_cases_ok)
                        blend_cases_ok = run_blend_case(&backend,
                            "SHADED_BLEND maps nonzero source to index zero",
                            17, 14, 3, 2, 9, 4,
                            HERCULES_RECTANGLE_SHADED_BLEND, 2U,
                            false, false, true, &error);
                    if (blend_cases_ok)
                        blend_cases_ok = run_blend_case(&backend,
                            "transparent RAW_BLEND with no LUT snapshot",
                            21, 17, 3, 2, 10, 5,
                            HERCULES_RECTANGLE_RAW_BLEND, 0U,
                            false, true, false, &error);
                    if (blend_cases_ok)
                        blend_cases_ok = run_blend_case(&backend,
                            "transparent SHADED_BLEND with no table snapshots",
                            25, 20, 3, 2, 11, 6,
                            HERCULES_RECTANGLE_SHADED_BLEND, 3U,
                            false, true, false, &error);
                    if (blend_cases_ok)
                        blend_cases_ok = run_overlapping_blends(&backend, &error);

                    Fixture blend_fixture{};
                    IndexedAddressMemory blend_memory(HERC_TEX_MAP_BASE, 65536U);
                    IndexedRectangleCommand valid_raw{};
                    if (blend_cases_ok &&
                        !capture_snapshot(29, 23, 3, 2, 17, 7, false,
                                         &blend_fixture, &blend_memory,
                                         &valid_raw, &error))
                        blend_cases_ok = false;
                    if (blend_cases_ok) {
                        configure_blend(&valid_raw,
                            HERCULES_RECTANGLE_RAW_BLEND, 0U, 0U);
                        blend_cases_ok = check_malformed_blend_commands(
                            &backend, valid_raw, &error);
                    }
                    if (!blend_cases_ok) {
                        std::fprintf(stderr, "FAIL: %s\n", error.c_str());
                        exit_code = 1;
                    } else {
                        std::puts("PASS indexed GPU COPY and selected-LUT blend comparisons");
                    }
                }
            }
        }
    }
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    return exit_code;
}
