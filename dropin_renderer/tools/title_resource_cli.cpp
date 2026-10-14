#include "title_resource.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#if defined(DROPIN_INDEXED_GPU_ENABLED)
#include "indexed_gpu.hpp"
#include <SDL3/SDL.h>
#endif

namespace {
namespace fs = std::filesystem;
void write(const fs::path &path, const std::vector<std::uint8_t> &bytes)
{
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!output) throw std::runtime_error("Could not write controlled fixture output");
}
fs::path checked_output(const fs::path &requested)
{
    const fs::path root = fs::canonical(fs::path(__FILE__).parent_path().parent_path().parent_path());
    const fs::path work = fs::canonical(root / "work");
    const fs::path parent = fs::canonical(requested.parent_path());
    const auto relative = parent.lexically_relative(work);
    if (relative.empty() || *relative.begin() == fs::path("..") || requested.filename().empty())
        throw std::runtime_error("Output must be a fresh directory under this checkout's work folder");
    const fs::path output = parent / requested.filename();
    if (fs::exists(output)) throw std::runtime_error("Output directory must be fresh");
    fs::create_directory(output);
    return output;
}
#if defined(DROPIN_INDEXED_GPU_ENABLED)
struct GpuSession {
    SDL_GPUDevice *device = nullptr;
    bool initialized = false;
    ~GpuSession() {
        if (device) SDL_DestroyGPUDevice(device);
        if (initialized) SDL_Quit();
    }
};
#endif
}

int main(int argc, char **argv)
{
    using namespace hercules::dropin;
    try {
        if (argc != 3 && argc != 4) throw std::runtime_error("usage: title_resource_cli archive.fs fresh-work-output [--gpu]");
        const bool requested_gpu = argc == 4;
        if (requested_gpu && std::string(argv[3]) != "--gpu") throw std::runtime_error("Unknown option");
        const fs::path output = checked_output(fs::absolute(argv[2]));
        TitleScreenResource scene;
        std::string error;
        if (!load_title_screen(argv[1], &scene, &error)) throw std::runtime_error(error);
        std::vector<std::uint8_t> indices(scene.cpu_indices.begin(), scene.cpu_indices.end());
        const auto rgba = title_screen_rgba(scene.cpu_indices, scene.palette);
        std::vector<std::uint8_t> palette;
        for (const auto &entry : scene.palette) {
            palette.push_back(entry.red); palette.push_back(entry.green);
            palette.push_back(entry.blue); palette.push_back(entry.flags);
        }
        std::string gpu_status = "not_requested";
        if (requested_gpu) {
#if defined(DROPIN_INDEXED_GPU_ENABLED)
            GpuSession session;
            if (!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(SDL_GetError());
            session.initialized = true;
            session.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXBC, false, "direct3d12");
            if (!session.device) throw std::runtime_error(SDL_GetError());
            {
                IndexedGpuBackend backend(session.device);
                IndexedFrame frame{};
                for (const auto &command : scene.commands) {
                    const auto result = backend.render(command, frame);
                    if (result.status != IndexedGpuStatus::ok) throw std::runtime_error(result.message);
                    frame = result.indices;
                }
                if (frame != scene.cpu_indices) throw std::runtime_error("Complete GPU/CPU screen index mismatch");
            }
            gpu_status = "exact_indices";
#else
            throw std::runtime_error("GPU backend was not built");
#endif
        }
        write(output / "frame.idx", indices);
        write(output / "frame.rgba", rgba);
        write(output / "palette.entries", palette);
        std::cout << "{\"status\":\"PASS\",\"image_sha256\":\"" << scene.image_sha256
                  << "\",\"cif_sha256\":\"" << scene.cif_sha256
                  << "\",\"frame_sha256\":\"" << sha256_bytes(indices)
                  << "\",\"rgba_sha256\":\"" << sha256_bytes(rgba)
                  << "\",\"palette_sha256\":\"" << sha256_bytes(palette)
                  << "\",\"sprite_count\":" << scene.commands.size()
                  << ",\"pixel_stores\":" << scene.pixel_stores
                  << ",\"presentation_requested\":" << (scene.presentation_requested ? "true" : "false")
                  << ",\"gpu\":\"" << gpu_status << "\"}\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
