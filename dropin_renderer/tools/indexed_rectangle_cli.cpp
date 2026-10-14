#include "indexed_reference.hpp"
#include "packet_capture.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#if defined(DROPIN_INDEXED_GPU_ENABLED)
#include "indexed_gpu.hpp"
#include <SDL3/SDL.h>
#endif

namespace {
#if defined(DROPIN_INDEXED_GPU_ENABLED)
struct GpuSession {
    SDL_GPUDevice *device = nullptr;
    bool video_initialized = false;
    ~GpuSession() {
        if (device) SDL_DestroyGPUDevice(device);
        if (video_initialized) SDL_Quit();
    }
};
#endif
std::vector<std::uint8_t> hex_bytes(const std::string &hex)
{
    if (hex.size() % 2 != 0) throw std::runtime_error("odd byte string");
    std::vector<std::uint8_t> bytes;
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        const auto digit = [](char c) -> unsigned {
            if (c >= '0' && c <= '9') return static_cast<unsigned>(c - '0');
            if (c >= 'a' && c <= 'f') return static_cast<unsigned>(c - 'a' + 10);
            throw std::runtime_error("invalid hex digit");
        };
        bytes.push_back(static_cast<std::uint8_t>(digit(hex[i]) * 16 + digit(hex[i + 1])));
    }
    return bytes;
}
}

int main(int argc, char **argv)
{
    using namespace hercules::dropin;
    if (argc != 3 && argc != 4) {
        std::cerr << "usage: indexed_rectangle_cli input.txt output.idx [--gpu]\n";
        return 2;
    }
    try {
        const bool gpu_requested = argc == 4;
        if (gpu_requested && std::string(argv[3]) != "--gpu") throw std::runtime_error("invalid option");
        std::ifstream input(argv[1]);
        std::string header, packet_hex, payload_hex;
        long rect_x, rect_y, rect_w, rect_h, seed;
        unsigned long address;
        if (!(input >> header >> address >> packet_hex >> rect_x >> rect_y >> rect_w >> rect_h >> payload_hex >> seed)
            || header != "HERCULES_INDEXED_RECTANGLE_V1" || address != 0x10001000UL ||
            seed < 0 || seed > 255 || rect_x < 0 || rect_y < 0 || rect_w < 0 || rect_h < 0 ||
            rect_x > 1023 || rect_y > 1023 || rect_w > 1024 || rect_h > 1024)
            throw std::runtime_error("invalid bounded fixture input");
        std::string extra;
        if (input >> extra) throw std::runtime_error("extra input fields");
        const auto packet = hex_bytes(packet_hex);
        if (packet.size() != 40) throw std::runtime_error("expected observed FT4 producer record");
        IndexedAddressMemory texture(HERC_TEX_MAP_BASE, 65536U);
        HERC_TEX_RECT16 rect{static_cast<renderer_i16>(rect_x), static_cast<renderer_i16>(rect_y),
                            static_cast<renderer_i16>(rect_w), static_cast<renderer_i16>(rect_h)};
        if (texture.upload_texture_rect(rect, hex_bytes(payload_hex)) != IndexedMemoryStatus::ok)
            throw std::runtime_error("unsupported upload");
        std::vector<renderer_u8> bytes(0x2000U, 0);
        std::copy(packet.begin(), packet.end(), bytes.begin() + 0x1000U);
        bytes[0x101U] = 0x10U; bytes[0x103U] = 0x10U; // OT: raw32 0x10001000.
        bytes[0x1047U] = 0x40U; // Explicit no-payload terminator at 0x10001040.
        FrameRaw32Region region{0x10000000UL, static_cast<renderer_u32>(bytes.size()), bytes.data()};
        IndexedRectangleCommand command;
        bool captured = false;
        const auto trace = consume_offline(frame_raw32_region_resolve, &region, 0x10000100UL, 1,
            RendererAc60StateView{}, [&](const Event &event) {
                if (event.packet_kind == RENDERER_PACKET_FT4) {
                    if (capture_indexed_rectangle(event, texture, &command) != IndexedReferenceStatus::ok)
                        return false;
                    captured = true;
                }
                return true;
            });
        if (trace.status != CaptureStatus::ok || !captured)
            throw std::runtime_error("unsupported primitive or missing observed texel");
        IndexedFrame initial, frame;
        initial.fill(static_cast<std::uint8_t>(seed)); frame = initial;
        IndexedReferenceEffects effects;
        if (render_indexed_rectangle(command, &frame, &effects) != IndexedReferenceStatus::ok)
            throw std::runtime_error("shared reference failed");
        std::string gpu_status = "not_requested";
        if (gpu_requested) {
#if defined(DROPIN_INDEXED_GPU_ENABLED)
            GpuSession session;
            if (!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(SDL_GetError());
            session.video_initialized = true;
            session.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXBC, false, "direct3d12");
            if (!session.device) { std::cerr << "SKIP: " << SDL_GetError() << '\n'; return 77; }
            {
                IndexedGpuBackend gpu(session.device);
                const auto result = gpu.render(command, initial);
                if (result.status != IndexedGpuStatus::ok) {
                    std::cerr << result.message << '\n';
                    return result.status == IndexedGpuStatus::unsupported ? 77 : 1;
                }
                if (result.indices != frame) throw std::runtime_error("GPU/shared reference index mismatch");
            }
            gpu_status = "exact_indices";
#else
            std::cerr << "SKIP: optional indexed GPU backend not built\n";
            return 77;
#endif
        }
        std::ofstream output(argv[2], std::ios::binary);
        output.write(reinterpret_cast<const char *>(frame.data()), static_cast<std::streamsize>(frame.size()));
        if (!output) throw std::runtime_error("frame output failed");
        std::cout << "{\"status\":\"PASS\",\"pixel_stores\":" << effects.pixel_store_count
                  << ",\"presentation_requested\":" << (effects.presentation_requested ? "true" : "false")
                  << ",\"captured_texels\":" << command.texels.size() << ",\"gpu\":\"" << gpu_status << "\"}\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
