#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "indexed_gpu.hpp"
#include "title_resource.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using hercules::dropin::IndexedFrame;
using hercules::dropin::IndexedGpuBackend;
using hercules::dropin::IndexedGpuResult;
using hercules::dropin::IndexedGpuStatus;
using hercules::dropin::IndexedReferenceEffects;
using hercules::dropin::IndexedReferenceStatus;
using hercules::dropin::IndexedSpriteCommand;
using hercules::dropin::RgbaFrame;
using hercules::dropin::TitleScreenResource;

namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 720;
constexpr int kInfoHeight = 64;
constexpr const char *kWindowTitle =
    "Hercules controlled resource replay (not an original TITLE frame capture)";

enum class DisplayMode {
    cpu,
    gpu,
    difference
};

struct Options {
    std::string archive_path;
    bool smoke = false;
    bool hidden = false;
    std::optional<fs::path> screenshot;
    bool help = false;
};

struct Runtime {
    bool sdl_initialized = false;
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;
    SDL_GPUDevice *gpu_device = nullptr;
    std::unique_ptr<IndexedGpuBackend> gpu_backend;

    ~Runtime()
    {
        gpu_backend.reset();
        if (gpu_device != nullptr) SDL_DestroyGPUDevice(gpu_device);
        if (texture != nullptr) SDL_DestroyTexture(texture);
        if (renderer != nullptr) SDL_DestroyRenderer(renderer);
        if (window != nullptr) SDL_DestroyWindow(window);
        if (sdl_initialized) SDL_Quit();
    }
};

struct ReplayState {
    TitleScreenResource resource;
    IndexedFrame cpu_indices{};
    std::optional<IndexedFrame> gpu_indices;
    bool cpu_matches_loaded_reference = false;
    bool gpu_matches_cpu = false;
    std::size_t differing_indices = 0;
    std::string gpu_status = "not attempted";
    std::string cpu_frame_sha256;
    std::string gpu_frame_sha256;
};

struct SmokeReport {
    std::size_t rendered_frames = 0;
    bool reload_exercised = false;
    bool resize_exercised = false;
    std::string reload_result = "not attempted";
    std::string screenshot_path;
    std::string error;
};

std::string sdl_error(const char *operation)
{
    const char *detail = SDL_GetError();
    if (detail == nullptr || *detail == '\0') return operation;
    return std::string(operation) + ": " + detail;
}

std::vector<std::uint8_t> frame_bytes(const IndexedFrame &frame)
{
    return std::vector<std::uint8_t>(frame.begin(), frame.end());
}

bool frames_equal(const IndexedFrame &left, const IndexedFrame &right)
{
    return std::equal(left.begin(), left.end(), right.begin());
}

std::size_t count_differences(const IndexedFrame &left, const IndexedFrame &right)
{
    std::size_t count = 0;
    for (std::size_t i = 0; i < left.size(); ++i)
        if (left[i] != right[i]) ++count;
    return count;
}

std::string mode_name(DisplayMode mode)
{
    switch (mode) {
    case DisplayMode::cpu: return "CPU";
    case DisplayMode::gpu: return "GPU";
    case DisplayMode::difference: return "DIFFERENCE";
    }
    return "UNKNOWN";
}

std::string shortened(std::string text, std::size_t maximum = 116)
{
    if (text.size() > maximum) {
        text.resize(maximum > 3 ? maximum - 3 : maximum);
        if (maximum > 3) text += "...";
    }
    return text;
}

bool prepare_replay(const std::string &archive_path,
                    IndexedGpuBackend *gpu_backend,
                    const std::string &gpu_setup_message,
                    ReplayState *output, std::string *error)
{
    if (output == nullptr || error == nullptr) return false;

    ReplayState candidate;
    if (!hercules::dropin::load_title_screen(archive_path, &candidate.resource, error))
        return false;
    if (candidate.resource.commands.size() != 2) {
        *error = "Pinned resource replay did not produce the expected two owned sprite commands";
        return false;
    }

    candidate.cpu_indices.fill(0);
    std::uint32_t cpu_pixel_stores = 0;
    bool cpu_presentation_requested = false;
    for (const IndexedSpriteCommand &command : candidate.resource.commands) {
        IndexedReferenceEffects effects{};
        const auto status = hercules::dropin::render_indexed_sprite(
            command, &candidate.cpu_indices, &effects);
        if (status != IndexedReferenceStatus::ok) {
            *error = "CPU sprite replay failed with status " +
                std::to_string(static_cast<int>(status));
            return false;
        }
        cpu_pixel_stores += effects.pixel_store_count;
        cpu_presentation_requested =
            cpu_presentation_requested || effects.presentation_requested;
    }
    candidate.cpu_matches_loaded_reference =
        frames_equal(candidate.cpu_indices, candidate.resource.cpu_indices);
    candidate.cpu_frame_sha256 =
        hercules::dropin::sha256_bytes(frame_bytes(candidate.cpu_indices));
    if (!candidate.cpu_matches_loaded_reference) {
        candidate.gpu_status = "not compared because CPU replay differs from the loaded reference";
    } else if (cpu_pixel_stores != candidate.resource.pixel_stores ||
               cpu_presentation_requested != candidate.resource.presentation_requested) {
        candidate.gpu_status = "not compared because replay effects differ from the loaded reference";
    } else if (gpu_backend == nullptr) {
        candidate.gpu_status = gpu_setup_message.empty()
            ? "unavailable: D3D12/DXBC device was not created"
            : gpu_setup_message;
    } else if (gpu_backend->initialization_status() != IndexedGpuStatus::ok) {
        const char *kind = gpu_backend->initialization_status() == IndexedGpuStatus::unsupported
            ? "unsupported: " : "error: ";
        candidate.gpu_status = std::string(kind) + gpu_backend->initialization_message();
    } else {
        IndexedFrame gpu_indices{};
        bool gpu_ok = true;
        for (const IndexedSpriteCommand &command : candidate.resource.commands) {
            IndexedGpuResult result = gpu_backend->render(command, gpu_indices);
            if (result.status != IndexedGpuStatus::ok) {
                const char *kind = result.status == IndexedGpuStatus::unsupported
                    ? "unsupported: " : "error: ";
                candidate.gpu_status = std::string(kind) + result.message;
                gpu_ok = false;
                break;
            }
            gpu_indices = std::move(result.indices);
        }
        if (gpu_ok) {
            candidate.gpu_matches_cpu =
                frames_equal(candidate.cpu_indices, gpu_indices);
            candidate.differing_indices =
                count_differences(candidate.cpu_indices, gpu_indices);
            candidate.gpu_frame_sha256 =
                hercules::dropin::sha256_bytes(frame_bytes(gpu_indices));
            candidate.gpu_status = candidate.gpu_matches_cpu
                ? "exact: full 76,800-byte indexed frame matches CPU"
                : "mismatch: full indexed-frame comparison found differences";
            candidate.gpu_indices = std::move(gpu_indices);
        }
    }

    *output = std::move(candidate);
    error->clear();
    return true;
}

bool path_component_equal(const fs::path &left, const fs::path &right)
{
    const std::wstring a = left.native();
    const std::wstring b = right.native();
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::towlower(a[i]) != std::towlower(b[i])) return false;
    }
    return true;
}

bool path_is_within(const fs::path &root, const fs::path &candidate)
{
    auto root_part = root.begin();
    auto candidate_part = candidate.begin();
    for (; root_part != root.end(); ++root_part, ++candidate_part) {
        if (candidate_part == candidate.end() ||
            !path_component_equal(*root_part, *candidate_part))
            return false;
    }
    return true;
}

fs::path weakly_canonical_or_throw(const fs::path &path, const char *description)
{
    std::error_code ec;
    fs::path result = fs::weakly_canonical(path, ec);
    if (ec) throw std::runtime_error(std::string(description) + ": " + ec.message());
    return result;
}

fs::path find_project_root(const char *argv0)
{
    std::vector<fs::path> starts;
    starts.push_back(fs::current_path());
    if (argv0 != nullptr && *argv0 != '\0') {
        fs::path executable(argv0);
        if (executable.is_relative()) executable = fs::current_path() / executable;
        starts.push_back(executable.parent_path());
    }
    for (const fs::path &start : starts) {
        fs::path current = start;
        while (!current.empty()) {
            std::error_code ec;
            const bool looks_like_root =
                fs::is_regular_file(current / "AGENTS.md", ec) &&
                fs::is_regular_file(current / "dropin_renderer/core/title_resource.hpp", ec) &&
                fs::is_directory(current / "work", ec);
            if (looks_like_root) return weakly_canonical_or_throw(current, "Project root");
            const fs::path parent = current.parent_path();
            if (parent == current) break;
            current = parent;
        }
    }
    throw std::runtime_error(
        "Could not locate the Hercules project root to validate the screenshot work path");
}

bool has_reparse_attribute(const fs::path &path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

bool has_reparse_component_below(const fs::path &root, const fs::path &candidate)
{
    if (has_reparse_attribute(root)) return true;
    const fs::path relative = candidate.lexically_relative(root);
    if (relative.empty() && candidate != root) return true;
    fs::path current = root;
    for (const fs::path &part : relative) {
        if (part == "." || part.empty()) continue;
        current /= part;
        if (has_reparse_attribute(current)) return true;
    }
    return false;
}

fs::path validate_screenshot_path(const fs::path &project_root,
                                  const fs::path &requested)
{
    const fs::path lexical_work = (project_root / "work").lexically_normal();
    const fs::path lexical_target =
        (requested.is_absolute() ? requested : project_root / requested).lexically_normal();
    if (!path_is_within(lexical_work, lexical_target) ||
        lexical_target == lexical_work) {
        throw std::runtime_error("Screenshot path must be inside the repository work directory");
    }
    if (has_reparse_component_below(lexical_work, lexical_target)) {
        throw std::runtime_error("Screenshot path may not traverse a reparse point or symlink");
    }

    const std::wstring extension = lexical_target.extension().native();
    if (extension.size() != 4 ||
        std::towlower(extension[0]) != L'.' ||
        std::towlower(extension[1]) != L'b' ||
        std::towlower(extension[2]) != L'm' ||
        std::towlower(extension[3]) != L'p') {
        throw std::runtime_error("Screenshot path must use a .bmp extension");
    }
    std::error_code ec;
    if (!fs::is_directory(lexical_target.parent_path(), ec) || ec)
        throw std::runtime_error("Screenshot parent directory must already exist");
    if (fs::exists(lexical_target, ec) && fs::is_directory(lexical_target, ec))
        throw std::runtime_error("Screenshot path names a directory");

    const fs::path canonical_work =
        weakly_canonical_or_throw(lexical_work, "Work directory");
    const fs::path canonical_target =
        weakly_canonical_or_throw(lexical_target, "Screenshot path");
    if (!path_is_within(canonical_work, canonical_target) ||
        canonical_target == canonical_work) {
        throw std::runtime_error("Canonical screenshot path is outside the repository work directory");
    }
    if (has_reparse_component_below(canonical_work, canonical_target))
        throw std::runtime_error("Canonical screenshot path contains a reparse point");
    return canonical_target;
}

Options parse_options(int argc, char **argv)
{
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument(argv[i]);
        if (argument == "--help" || argument == "-h") {
            options.help = true;
        } else if (argument == "--archive") {
            if (i + 1 >= argc) throw std::runtime_error("--archive requires a filesystem path");
            options.archive_path = argv[++i];
        } else if (argument == "--smoke") {
            options.smoke = true;
        } else if (argument == "--hidden") {
            options.hidden = true;
        } else if (argument == "--screenshot") {
            if (i + 1 >= argc) throw std::runtime_error("--screenshot requires a BMP path under work");
            options.screenshot = fs::path(argv[++i]);
        } else {
            throw std::runtime_error("Unknown argument: " + argument);
        }
    }
    if (!options.help && options.archive_path.empty())
        throw std::runtime_error("--archive <FS> is required");
    return options;
}

void print_usage()
{
    std::cout
        << "Usage: hercules_preview --archive <FS> [--smoke] [--hidden] "
           "[--screenshot work\\\\preview.bmp]\n"
        << "Controls: C CPU, G GPU, D indexed differences, Space cycle, "
           "R reload pinned resources, Esc quit.\n";
}

std::string gpu_short_status(const ReplayState &state)
{
    if (!state.gpu_indices) return shortened(state.gpu_status, 74);
    if (state.gpu_matches_cpu) return "exact";
    return "mismatch";
}

std::string status_line(const ReplayState &state, DisplayMode mode)
{
    std::ostringstream text;
    text << "MODE " << mode_name(mode)
         << " | CPU replay " << (state.cpu_matches_loaded_reference ? "exact" : "DIFF");
    if (state.gpu_indices) {
        text << " | GPU " << gpu_short_status(state)
             << " | " << state.differing_indices << " differing indices";
    } else {
        text << " | GPU " << gpu_short_status(state);
    }
    return shortened(text.str());
}

void update_window_title(Runtime &runtime, const ReplayState &state,
                         DisplayMode mode, const std::string &notice)
{
    std::string title = std::string("Hercules controlled resource replay | ") +
        mode_name(mode) + " | CPU " +
        (state.cpu_matches_loaded_reference ? "exact" : "DIFF") +
        " | GPU " + gpu_short_status(state);
    if (!notice.empty()) title += " | " + notice;
    SDL_SetWindowTitle(runtime.window, shortened(std::move(title), 180).c_str());
}

RgbaFrame image_for_mode(const ReplayState &state, DisplayMode mode)
{
    if (mode == DisplayMode::cpu)
        return hercules::dropin::title_screen_rgba(
            state.cpu_indices, state.resource.palette);
    if (mode == DisplayMode::gpu && state.gpu_indices)
        return hercules::dropin::title_screen_rgba(
            *state.gpu_indices, state.resource.palette);

    RgbaFrame difference(hercules::dropin::indexed_frame_bytes * 4U, 255U);
    if (mode == DisplayMode::difference && state.gpu_indices) {
        for (std::size_t i = 0; i < state.cpu_indices.size(); ++i) {
            const bool differs = state.cpu_indices[i] != (*state.gpu_indices)[i];
            difference[i * 4U + 0U] = differs ? 255U : 8U;
            difference[i * 4U + 1U] = differs ? 48U : 12U;
            difference[i * 4U + 2U] = differs ? 220U : 18U;
            difference[i * 4U + 3U] = 255U;
        }
    } else {
        for (std::size_t i = 0; i < hercules::dropin::indexed_frame_bytes; ++i) {
            difference[i * 4U + 0U] = 8U;
            difference[i * 4U + 1U] = 12U;
            difference[i * 4U + 2U] = 18U;
            difference[i * 4U + 3U] = 255U;
        }
    }
    return difference;
}

void draw_debug_text(SDL_Renderer *renderer, float x, float y,
                     const std::string &line)
{
    (void)SDL_RenderDebugText(renderer, x, y, line.c_str());
}

bool render_frame(Runtime &runtime, const ReplayState &state,
                  DisplayMode mode, const std::string &notice,
                  const std::optional<fs::path> &screenshot,
                  std::string *error)
{
    const RgbaFrame rgba = image_for_mode(state, mode);
    if (!SDL_UpdateTexture(runtime.texture, nullptr, rgba.data(),
                           static_cast<int>(hercules::dropin::indexed_frame_width * 4U))) {
        if (error != nullptr) *error = sdl_error("SDL_UpdateTexture");
        return false;
    }
    if (!SDL_RenderClear(runtime.renderer)) {
        if (error != nullptr) *error = sdl_error("SDL_RenderClear");
        return false;
    }

    int output_width = 0;
    int output_height = 0;
    if (!SDL_GetRenderOutputSize(runtime.renderer, &output_width, &output_height)) {
        if (error != nullptr) *error = sdl_error("SDL_GetRenderOutputSize");
        return false;
    }

    SDL_SetRenderDrawColor(runtime.renderer, 19, 24, 32, 255);
    const SDL_FRect panel{0.0f, 0.0f, static_cast<float>(output_width),
                          static_cast<float>(kInfoHeight)};
    if (!SDL_RenderFillRect(runtime.renderer, &panel)) {
        if (error != nullptr) *error = sdl_error("SDL_RenderFillRect(info)");
        return false;
    }
    SDL_SetRenderDrawColor(runtime.renderer, 227, 233, 241, 255);
    draw_debug_text(runtime.renderer, 12.0f, 5.0f,
        "HERCULES RESOURCE REPLAY: MENUB.TGA + MAINMENU.CIF (NOT AN ORIGINAL TITLE FRAME)");
    SDL_SetRenderDrawColor(runtime.renderer, 158, 176, 198, 255);
    draw_debug_text(runtime.renderer, 12.0f, 20.0f,
        "C CPU   G GPU   D DIFF   SPACE CYCLE   R RELOAD PINNED RESOURCES   ESC QUIT");
    SDL_SetRenderDrawColor(runtime.renderer,
        state.gpu_indices && !state.gpu_matches_cpu ? 255 : 160,
        state.gpu_indices && !state.gpu_matches_cpu ? 112 : 222,
        state.gpu_indices && !state.gpu_matches_cpu ? 112 : 176, 255);
    draw_debug_text(runtime.renderer, 12.0f, 35.0f,
                    status_line(state, mode));
    std::string detail = notice.empty()
        ? "RESOURCE SHA image " + state.resource.image_sha256.substr(0, 8) +
          "  CIF " + state.resource.cif_sha256.substr(0, 8)
        : notice;
    if (mode == DisplayMode::difference && !state.gpu_indices)
        detail += "  DIFFERENCE UNAVAILABLE: GPU RESULT MISSING";
    draw_debug_text(runtime.renderer, 12.0f, 50.0f, shortened(std::move(detail)));

    if (output_width > 0 && output_height > kInfoHeight) {
        const float image_width = static_cast<float>(hercules::dropin::indexed_frame_width);
        const float image_height = static_cast<float>(hercules::dropin::indexed_frame_height);
        const float available_width = static_cast<float>(output_width);
        const float available_height = static_cast<float>(output_height - kInfoHeight);
        const float scale = std::min(available_width / image_width,
                                     available_height / image_height);
        const float width = image_width * scale;
        const float height = image_height * scale;
        const SDL_FRect destination{
            (available_width - width) * 0.5f,
            static_cast<float>(kInfoHeight) + (available_height - height) * 0.5f,
            width, height
        };
        if (!SDL_RenderTexture(runtime.renderer, runtime.texture, nullptr, &destination)) {
            if (error != nullptr) *error = sdl_error("SDL_RenderTexture");
            return false;
        }
    }

    bool screenshot_ok = true;
    std::string screenshot_error;
    if (screenshot) {
        SDL_Surface *surface = SDL_RenderReadPixels(runtime.renderer, nullptr);
        if (surface == nullptr) {
            screenshot_ok = false;
            screenshot_error = sdl_error("SDL_RenderReadPixels");
        } else {
            const std::string path = screenshot->u8string();
            screenshot_ok = SDL_SaveBMP(surface, path.c_str());
            if (!screenshot_ok) screenshot_error = sdl_error("SDL_SaveBMP");
            SDL_DestroySurface(surface);
        }
    }
    SDL_RenderPresent(runtime.renderer);
    if (!screenshot_ok) {
        if (error != nullptr) *error = screenshot_error;
        return false;
    }
    return true;
}

bool reload_resources(const Options &options, Runtime &runtime,
                      const std::string &gpu_setup_message,
                      ReplayState *state, std::string *error)
{
    ReplayState replacement;
    if (!prepare_replay(options.archive_path, runtime.gpu_backend.get(),
                        gpu_setup_message, &replacement, error))
        return false;
    *state = std::move(replacement);
    return true;
}

void choose_mode(DisplayMode requested, const ReplayState &state,
                 DisplayMode *active, std::string *notice)
{
    if (active == nullptr || notice == nullptr) return;
    if (requested == DisplayMode::gpu && !state.gpu_indices) {
        *active = DisplayMode::cpu;
        *notice = "GPU unavailable; CPU replay remains visible. " + state.gpu_status;
        return;
    }
    *active = requested;
    if (requested == DisplayMode::gpu && !state.gpu_matches_cpu)
        *notice = "GPU output differs from CPU; difference mode highlights changed indices.";
    else if (requested == DisplayMode::difference && state.gpu_indices)
        *notice = "Difference view lights only indices with different CPU/GPU values.";
    else
        notice->clear();
}

void cycle_mode(const ReplayState &state, DisplayMode *active,
                std::string *notice)
{
    if (state.gpu_indices) {
        const DisplayMode next =
            *active == DisplayMode::cpu ? DisplayMode::gpu :
            *active == DisplayMode::gpu ? DisplayMode::difference :
                                          DisplayMode::cpu;
        choose_mode(next, state, active, notice);
    } else {
        const DisplayMode next = *active == DisplayMode::cpu
            ? DisplayMode::difference : DisplayMode::cpu;
        choose_mode(next, state, active, notice);
    }
}

void json_string(std::ostream &out, const std::string &value)
{
    out << '"';
    for (unsigned char character : value) {
        switch (character) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (character < 0x20U) {
                const char digits[] = "0123456789abcdef";
                out << "\\u00" << digits[character >> 4U]
                    << digits[character & 0x0fU];
            } else {
                out << static_cast<char>(character);
            }
        }
    }
    out << '"';
}

void emit_smoke_report(const ReplayState *state, const SmokeReport &report,
                       DisplayMode mode, bool hidden, int exit_status)
{
    std::cout << "{\"schema_version\":1,\"app\":\"hercules_preview\""
              << ",\"original_game_executed\":false"
              << ",\"windowed\":true,\"hidden\":" << (hidden ? "true" : "false")
              << ",\"mode\":";
    json_string(std::cout, mode_name(mode));
    std::cout << ",\"rendered_frames\":" << report.rendered_frames
              << ",\"reload_exercised\":" << (report.reload_exercised ? "true" : "false")
              << ",\"reload_result\":";
    json_string(std::cout, report.reload_result);
    std::cout << ",\"resize_exercised\":" << (report.resize_exercised ? "true" : "false")
              << ",\"image_sha256\":";
    if (state != nullptr) json_string(std::cout, state->resource.image_sha256);
    else std::cout << "null";
    std::cout << ",\"cif_sha256\":";
    if (state != nullptr) json_string(std::cout, state->resource.cif_sha256);
    else std::cout << "null";
    std::cout << ",\"cpu_frame_sha256\":";
    if (state != nullptr) json_string(std::cout, state->cpu_frame_sha256);
    else std::cout << "null";
    std::cout << ",\"gpu_frame_sha256\":";
    if (state != nullptr && state->gpu_indices)
        json_string(std::cout, state->gpu_frame_sha256);
    else std::cout << "null";
    std::cout << ",\"commands\":"
              << (state != nullptr ? state->resource.commands.size() : 0U)
              << ",\"pixel_stores\":"
              << (state != nullptr ? state->resource.pixel_stores : 0U)
              << ",\"cpu_matches_loaded_reference\":"
              << (state != nullptr && state->cpu_matches_loaded_reference ? "true" : "false")
              << ",\"gpu_status\":";
    if (state != nullptr) json_string(std::cout, state->gpu_status);
    else std::cout << "null";
    std::cout << ",\"gpu_matches_cpu\":";
    if (state != nullptr && state->gpu_indices)
        std::cout << (state->gpu_matches_cpu ? "true" : "false");
    else std::cout << "null";
    std::cout << ",\"differing_indices\":"
              << (state != nullptr ? state->differing_indices : 0U)
              << ",\"verified\":"
              << (state != nullptr &&
                  state->cpu_matches_loaded_reference &&
                  state->gpu_indices.has_value() &&
                  state->gpu_matches_cpu ? "true" : "false")
              << ",\"screenshot\":";
    if (!report.screenshot_path.empty()) json_string(std::cout, report.screenshot_path);
    else std::cout << "null";
    std::cout << ",\"error\":";
    if (!report.error.empty()) json_string(std::cout, report.error);
    else std::cout << "null";
    std::cout << ",\"exit_status\":" << exit_status << "}\n";
}

bool save_and_present(Runtime &runtime, const ReplayState &state,
                      DisplayMode mode, const std::optional<fs::path> &screenshot,
                      SmokeReport *report, const std::string &notice,
                      std::string *error)
{
    update_window_title(runtime, state, mode, notice);
    if (!render_frame(runtime, state, mode, notice, screenshot, error)) return false;
    if (report != nullptr) {
        ++report->rendered_frames;
        if (screenshot) report->screenshot_path = screenshot->u8string();
    }
    return true;
}

bool initialize_sdl(Runtime &runtime, const Options &options,
                    std::string *error)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        if (error != nullptr) *error = sdl_error("SDL_Init");
        return false;
    }
    runtime.sdl_initialized = true;
    const SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE |
        (options.hidden ? SDL_WINDOW_HIDDEN : 0);
    runtime.window = SDL_CreateWindow(kWindowTitle, kWindowWidth, kWindowHeight, flags);
    if (runtime.window == nullptr) {
        if (error != nullptr) *error = sdl_error("SDL_CreateWindow");
        return false;
    }
    runtime.renderer = SDL_CreateRenderer(runtime.window, nullptr);
    if (runtime.renderer == nullptr) {
        if (error != nullptr) *error = sdl_error("SDL_CreateRenderer");
        return false;
    }
    runtime.texture = SDL_CreateTexture(runtime.renderer, SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        static_cast<int>(hercules::dropin::indexed_frame_width),
        static_cast<int>(hercules::dropin::indexed_frame_height));
    if (runtime.texture == nullptr) {
        if (error != nullptr) *error = sdl_error("SDL_CreateTexture(RGBA32)");
        return false;
    }
    if (!SDL_SetTextureScaleMode(runtime.texture, SDL_SCALEMODE_NEAREST)) {
        if (error != nullptr) *error = sdl_error("SDL_SetTextureScaleMode(nearest)");
        return false;
    }
    if (!SDL_SetTextureBlendMode(runtime.texture, SDL_BLENDMODE_NONE)) {
        if (error != nullptr) *error = sdl_error("SDL_SetTextureBlendMode(none)");
        return false;
    }
    return true;
}

void initialize_gpu(Runtime &runtime, std::string *setup_message)
{
    runtime.gpu_device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_DXBC, false, "direct3d12");
    if (runtime.gpu_device == nullptr) {
        if (setup_message != nullptr)
            *setup_message = "unavailable: " +
                sdl_error("SDL_CreateGPUDevice(DXBC, direct3d12)");
        return;
    }
    runtime.gpu_backend = std::make_unique<IndexedGpuBackend>(runtime.gpu_device);
    if (runtime.gpu_backend->initialization_status() != IndexedGpuStatus::ok) {
        const char *kind =
            runtime.gpu_backend->initialization_status() == IndexedGpuStatus::unsupported
            ? "unsupported: " : "error: ";
        if (setup_message != nullptr)
            *setup_message = std::string(kind) +
                runtime.gpu_backend->initialization_message();
    } else if (setup_message != nullptr) {
        *setup_message = "D3D12/DXBC backend ready";
    }
}

bool run_smoke(const Options &options, Runtime &runtime,
               const std::string &gpu_setup_message, ReplayState *state,
               DisplayMode *active, SmokeReport *report,
               const std::optional<fs::path> &screenshot)
{
    std::string error;
    std::string mode_notice;
    if (!save_and_present(runtime, *state, DisplayMode::cpu, std::nullopt,
                          report, mode_notice, &error)) {
        report->error = error;
        return false;
    }

    ReplayState before_reload = *state;
    report->reload_exercised = true;
    if (!reload_resources(options, runtime, gpu_setup_message, state, &error)) {
        report->reload_result = "failed: " + error;
        report->error = "Smoke resource reload failed: " + error;
        return false;
    }
    report->reload_result =
        before_reload.resource.image_sha256 == state->resource.image_sha256 &&
        before_reload.resource.cif_sha256 == state->resource.cif_sha256 &&
        frames_equal(before_reload.cpu_indices, state->cpu_indices)
        ? "same pinned resource hashes and CPU frame"
        : "reload changed a pinned resource hash or CPU frame";
    if (report->reload_result != "same pinned resource hashes and CPU frame") {
        report->error = "Smoke reload result changed pinned resource output";
        return false;
    }

    choose_mode(DisplayMode::gpu, *state, active, &mode_notice);
    std::string frame_error;
    if (!save_and_present(runtime, *state, *active, std::nullopt,
                          report, mode_notice, &frame_error)) {
        report->error = frame_error;
        return false;
    }
    choose_mode(DisplayMode::difference, *state, active, &mode_notice);
    if (!save_and_present(runtime, *state, *active, std::nullopt,
                          report, mode_notice, &frame_error)) {
        report->error = frame_error;
        return false;
    }

    report->resize_exercised = SDL_SetWindowSize(runtime.window, 1000, 700);
    if (!report->resize_exercised) {
        report->error = sdl_error("SDL_SetWindowSize(smoke resize)");
        return false;
    }
    SDL_PumpEvents();
    choose_mode(DisplayMode::cpu, *state, active, &mode_notice);
    if (!save_and_present(runtime, *state, *active, screenshot,
                          report, mode_notice, &frame_error)) {
        report->error = frame_error;
        return false;
    }
    return true;
}

void process_event(const SDL_Event &event, bool *quit, Options const &options,
                   Runtime &runtime, ReplayState *state,
                   const std::string &gpu_setup_message,
                   DisplayMode *active, std::string *notice,
                   SmokeReport *report)
{
    if (event.type == SDL_EVENT_QUIT ||
        event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        *quit = true;
        return;
    }
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) return;

    switch (event.key.key) {
    case SDLK_ESCAPE:
        *quit = true;
        break;
    case SDLK_C:
        choose_mode(DisplayMode::cpu, *state, active, notice);
        break;
    case SDLK_G:
        choose_mode(DisplayMode::gpu, *state, active, notice);
        break;
    case SDLK_D:
        choose_mode(DisplayMode::difference, *state, active, notice);
        break;
    case SDLK_SPACE:
        cycle_mode(*state, active, notice);
        break;
    case SDLK_R: {
        std::string error;
        if (reload_resources(options, runtime, gpu_setup_message, state, &error)) {
            *notice = "Reloaded pinned MENUB.TGA and MAINMENU.CIF archive members.";
            if (*active == DisplayMode::gpu && !state->gpu_indices)
                *active = DisplayMode::cpu;
        } else {
            *notice = "Reload failed; previous frame remains active: " + error;
        }
        break;
    }
    default:
        break;
    }
}

int run_interactive(const Options &options, Runtime &runtime,
                    const std::string &gpu_setup_message,
                    ReplayState *state,
                    const std::optional<fs::path> &screenshot)
{
    bool quit = false;
    bool screenshot_written = false;
    DisplayMode active = DisplayMode::cpu;
    std::string notice;
    while (!quit) {
        SDL_Event event{};
        if (SDL_WaitEventTimeout(&event, 16))
            process_event(event, &quit, options, runtime, state,
                          gpu_setup_message, &active, &notice, nullptr);
        while (SDL_PollEvent(&event))
            process_event(event, &quit, options, runtime, state,
                          gpu_setup_message, &active, &notice, nullptr);
        if (quit) break;
        std::string error;
        const std::optional<fs::path> &frame_screenshot =
            screenshot && !screenshot_written ? screenshot : std::nullopt;
        if (!save_and_present(runtime, *state, active, frame_screenshot,
                              nullptr, notice, &error)) {
            std::cerr << error << '\n';
            return 1;
        }
        if (frame_screenshot) {
            screenshot_written = true;
            std::cout << "Saved controlled replay screenshot to "
                      << frame_screenshot->u8string() << '\n';
        }
    }
    return 0;
}

} // namespace

int main(int argc, char **argv)
{
    Options options;
    try {
        options = parse_options(argc, argv);
        if (options.help) {
            print_usage();
            return 0;
        }
    } catch (const std::exception &failure) {
        std::cerr << failure.what() << '\n';
        print_usage();
        return 2;
    }

    SmokeReport report;
    ReplayState state;
    DisplayMode active = DisplayMode::cpu;
    bool have_state = false;
    int exit_status = 1;
    try {
        std::optional<fs::path> screenshot;
        if (options.screenshot) {
            const fs::path root = find_project_root(argc > 0 ? argv[0] : nullptr);
            screenshot = validate_screenshot_path(root, *options.screenshot);
        }

        Runtime runtime;
        std::string error;
        if (!initialize_sdl(runtime, options, &error))
            throw std::runtime_error(error);

        std::string gpu_setup_message;
        initialize_gpu(runtime, &gpu_setup_message);

        if (!prepare_replay(options.archive_path, runtime.gpu_backend.get(),
                            gpu_setup_message, &state, &error))
            throw std::runtime_error(error);
        have_state = true;

        if (options.smoke) {
            const bool rendered = run_smoke(options, runtime, gpu_setup_message,
                &state, &active, &report, screenshot);
            if (!rendered && report.error.empty())
                report.error = "Smoke rendering failed";
            exit_status = rendered &&
                state.cpu_matches_loaded_reference &&
                state.gpu_indices.has_value() &&
                state.gpu_matches_cpu ? 0 : 1;
            if (exit_status != 0 && report.error.empty()) {
                report.error = state.gpu_indices
                    ? "Smoke verification failed: GPU frame did not match CPU"
                    : "Smoke verification failed: requested GPU result unavailable";
            }
            emit_smoke_report(&state, report, active, options.hidden, exit_status);
            return exit_status;
        }
        return run_interactive(options, runtime, gpu_setup_message,
                               &state, screenshot);
    } catch (const std::exception &failure) {
        report.error = failure.what();
        if (options.smoke) {
            emit_smoke_report(have_state ? &state : nullptr, report,
                              active, options.hidden, 1);
        } else {
            std::cerr << report.error << '\n';
        }
        return 1;
    }
}
