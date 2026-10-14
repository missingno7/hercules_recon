#include "indexed_gpu.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <sstream>
#include <utility>

namespace hercules::dropin {
namespace {

constexpr std::uint32_t kTextureStride = 256U;
constexpr std::uint32_t kTexturePageAddress = 0x0096e6e0U;
constexpr std::uint32_t kTexturePageBytes = 256U * 256U;
constexpr std::uint32_t kTexturePageCount = 16U;
constexpr std::uint32_t kMaxRectangleSide = 8U;
constexpr std::size_t kBlendTableBytes = 256U * 256U;
constexpr std::size_t kShadeRowBytes = 256U;
constexpr std::size_t kTileUploadOffset = indexed_frame_bytes;
constexpr std::size_t kTransferOffsetAlignment = 4U;

std::size_t align_upload_offset(std::size_t offset)
{
    return (offset + kTransferOffsetAlignment - 1U) &
        ~(kTransferOffsetAlignment - 1U);
}

const char kIndexedShader[] =
    "struct VSOut { float4 position : SV_Position; };\n"
    "VSOut VSMain(uint vertexId : SV_VertexID) {\n"
    "  VSOut output;\n"
    "  if (vertexId == 0) output.position = float4(-1.0, -1.0, 0.0, 1.0);\n"
    "  else if (vertexId == 1) output.position = float4(-1.0, 3.0, 0.0, 1.0);\n"
    "  else output.position = float4(3.0, -1.0, 0.0, 1.0);\n"
    "  return output;\n"
    "}\n"
    "Texture2D<uint> page : register(t0, space2);\n"
    "Texture2D<uint> seed_frame : register(t1, space2);\n"
    "Texture2D<uint> selected_lut : register(t2, space2);\n"
    "Texture2D<uint> shade_row : register(t3, space2);\n"
    "cbuffer Rectangle : register(b0, space3) { int4 bounds; uint4 mode; };\n"
    "uint PSMain(float4 position : SV_Position) : SV_Target0 {\n"
    "  uint2 pixel = (uint2)position.xy;\n"
    "  uint2 origin = (uint2)bounds.xy;\n"
    "  uint2 extent = (uint2)bounds.zw;\n"
    "  if (pixel.x < origin.x || pixel.y < origin.y ||\n"
    "      pixel.x >= origin.x + extent.x || pixel.y >= origin.y + extent.y)\n"
    "    discard;\n"
    "  uint value = page.Load(int3(int2(pixel - origin), 0));\n"
    "  if (value == 0) discard;\n"
    "  if (mode.x == 0) return value;\n"
    "  uint mapped_source = value;\n"
    "  if (mode.x == 2)\n"
    "    mapped_source = shade_row.Load(int3(int2(value, 0), 0));\n"
    "  uint prior_destination = seed_frame.Load(int3(int2(pixel), 0));\n"
    "  return selected_lut.Load(int3(int2(prior_destination, mapped_source), 0));\n"
    "}\n";

struct RectangleUniform {
    std::int32_t bounds[4];
    std::uint32_t mode[4];
};

static_assert(sizeof(RectangleUniform) == 32,
              "rectangle HLSL cbuffer must match two 16-byte registers");

std::string with_sdl_error(const char *operation)
{
    std::ostringstream message;
    message << operation << ": " << SDL_GetError();
    return message.str();
}

struct GpuRunResources {
    SDL_GPUDevice *device = nullptr;
    SDL_GPUTexture *target = nullptr;
    SDL_GPUTexture *seed_frame = nullptr;
    SDL_GPUTexture *source = nullptr;
    SDL_GPUTexture *blend_table = nullptr;
    SDL_GPUTexture *shade_row = nullptr;
    SDL_GPUTransferBuffer *upload = nullptr;
    SDL_GPUTransferBuffer *download = nullptr;

    ~GpuRunResources()
    {
        if (download != nullptr)
            SDL_ReleaseGPUTransferBuffer(device, download);
        if (upload != nullptr)
            SDL_ReleaseGPUTransferBuffer(device, upload);
        if (shade_row != nullptr)
            SDL_ReleaseGPUTexture(device, shade_row);
        if (blend_table != nullptr)
            SDL_ReleaseGPUTexture(device, blend_table);
        if (source != nullptr)
            SDL_ReleaseGPUTexture(device, source);
        if (seed_frame != nullptr)
            SDL_ReleaseGPUTexture(device, seed_frame);
        if (target != nullptr)
            SDL_ReleaseGPUTexture(device, target);
    }
};

bool compile_shader(pD3DCompile compile, const char *entrypoint,
                    const char *target, ID3DBlob **bytecode,
                    std::string *message)
{
    ID3DBlob *diagnostics = nullptr;
    const HRESULT result = compile(
        kIndexedShader, sizeof(kIndexedShader) - 1,
        "dropin_indexed_gpu.hlsl", nullptr, nullptr, entrypoint, target,
        D3DCOMPILE_ENABLE_STRICTNESS, 0, bytecode, &diagnostics);
    if (FAILED(result)) {
        std::ostringstream detail;
        detail << "D3DCompile(" << entrypoint << ", " << target
               << ") failed with HRESULT 0x" << std::hex
               << static_cast<unsigned long>(result);
        if (diagnostics != nullptr && diagnostics->GetBufferPointer() != nullptr) {
            detail << ": " << static_cast<const char *>(
                diagnostics->GetBufferPointer());
        }
        *message = detail.str();
        if (diagnostics != nullptr)
            diagnostics->Release();
        return false;
    }
    if (diagnostics != nullptr)
        diagnostics->Release();
    return true;
}

IndexedGpuResult failure(IndexedGpuStatus status, std::string message)
{
    IndexedGpuResult result;
    result.status = status;
    result.message = std::move(message);
    return result;
}

bool validate_snapshot(const IndexedRectangleCommand &command,
                       std::string *message)
{
    const HerculesRectanglePlan &plan = command.plan;
    if (plan.page_address != kTexturePageAddress) {
        *message = "unsupported: rectangle plan does not target the captured tpage-zero page";
        return false;
    }
    if (plan.width == 0U || plan.height == 0U ||
        plan.width > kMaxRectangleSide || plan.height > kMaxRectangleSide ||
        plan.x >= indexed_frame_width || plan.y >= indexed_frame_height ||
        plan.width > indexed_frame_width - plan.x ||
        plan.height > indexed_frame_height - plan.y ||
        plan.u >= kTextureStride || plan.v >= kTextureStride ||
        plan.width > kTextureStride - plan.u ||
        plan.height > kTextureStride - plan.v) {
        *message = "unsupported: rectangle plan is outside the bounded 320x240, 1..8, 256-stride domain";
        return false;
    }
    const std::uint64_t expected_source_offset =
        static_cast<std::uint64_t>(plan.v) * kTextureStride + plan.u;
    const std::uint64_t expected_destination_offset =
        static_cast<std::uint64_t>(plan.y) * indexed_frame_width + plan.x;
    const std::size_t expected_texels =
        static_cast<std::size_t>(plan.width) * plan.height;
    if (plan.source_pitch != kTextureStride ||
        plan.source_start != expected_source_offset ||
        plan.destination_pitch != indexed_frame_width ||
        plan.destination_start != expected_destination_offset ||
        command.texels.size() != expected_texels ||
        command.sample_addresses.size() != expected_texels) {
        *message = "unsupported: owned rectangle snapshot does not match its resolved plan";
        return false;
    }

    const bool has_nonzero_source = std::any_of(
        command.texels.begin(), command.texels.end(),
        [](std::uint8_t value) { return value != 0U; });
    const auto bad_blend_payload = [message](const char *detail) {
        *message = std::string("unsupported: ") + detail;
        return false;
    };
    const bool has_blend_table = command.blend_table.size() == kBlendTableBytes;
    const bool has_shade_row = command.shade_row.size() == kShadeRowBytes;
    if (plan.pixel_mode == HERCULES_RECTANGLE_COPY) {
        if (plan.blend_selector != 0U || plan.shade_row != 0U ||
            !command.blend_table.empty() || !command.shade_row.empty()) {
            return bad_blend_payload(
                "COPY requires selector/row zero and no selected tables");
        }
    } else if (plan.pixel_mode == HERCULES_RECTANGLE_RAW_BLEND) {
        if (plan.blend_selector > 3U || plan.shade_row != 0U ||
            !command.shade_row.empty()) {
            return bad_blend_payload(
                "RAW_BLEND requires selector 0..3, row zero, and no shade row");
        }
        if ((has_nonzero_source && !has_blend_table) ||
            (!has_nonzero_source && !command.blend_table.empty())) {
            return bad_blend_payload(
                "RAW_BLEND requires one 65536-byte selected LUT iff any source is nonzero");
        }
    } else if (plan.pixel_mode == HERCULES_RECTANGLE_SHADED_BLEND) {
        if (plan.blend_selector > 3U || plan.shade_row > 255U ||
            plan.shade_row == 0x80U) {
            return bad_blend_payload(
                "SHADED_BLEND requires selector 0..3 and a shade row in 0..255 except 128");
        }
        if (has_nonzero_source) {
            if (!has_blend_table || !has_shade_row)
                return bad_blend_payload(
                    "SHADED_BLEND requires one 65536-byte LUT and one 256-byte shade row iff any source is nonzero");
        } else if (!command.blend_table.empty() || !command.shade_row.empty()) {
            return bad_blend_payload(
                "all-zero SHADED_BLEND commands must not carry selected tables");
        }
    } else {
        return bad_blend_payload("rectangle pixel mode is unknown");
    }

    std::size_t sample = 0;
    for (std::uint32_t row = 0; row < plan.height; ++row) {
        for (std::uint32_t column = 0; column < plan.width; ++column) {
            const std::uint64_t expected_address =
                static_cast<std::uint64_t>(plan.page_address) +
                expected_source_offset +
                static_cast<std::uint64_t>(row) * plan.source_pitch + column;
            if (expected_address > std::numeric_limits<std::uint32_t>::max() ||
                command.sample_addresses[sample] != expected_address) {
                *message = "unsupported: sampled byte addresses differ from the shared 256-stride plan";
                return false;
            }
            ++sample;
        }
    }
    return true;
}

bool validate_sprite_snapshot(const IndexedSpriteCommand &command,
                              std::string *message)
{
    const HerculesSpritePlan &plan = command.plan;
    if (plan.page_address < kTexturePageAddress) {
        *message = "unsupported: sprite page precedes the captured texture-page range";
        return false;
    }
    const std::uint32_t page_offset = plan.page_address - kTexturePageAddress;
    if (page_offset % kTexturePageBytes != 0U ||
        page_offset / kTexturePageBytes >= kTexturePageCount) {
        *message = "unsupported: sprite page is not an aligned page in range 0..15";
        return false;
    }
    if (plan.width == 0U || plan.width > kTextureStride ||
        plan.height == 0U || plan.height > indexed_frame_height ||
        plan.x >= indexed_frame_width || plan.y >= indexed_frame_height ||
        plan.width > indexed_frame_width - plan.x ||
        plan.height > indexed_frame_height - plan.y ||
        plan.u >= kTextureStride || plan.width > kTextureStride - plan.u ||
        plan.v >= kTextureStride || plan.height > kTextureStride - plan.v) {
        *message = "unsupported: sprite plan is outside the clipped 320x240, 256x256 page domain";
        return false;
    }

    const std::uint64_t expected_source_offset =
        static_cast<std::uint64_t>(plan.v) * kTextureStride + plan.u;
    const std::uint64_t expected_destination_offset =
        static_cast<std::uint64_t>(plan.y) * indexed_frame_width + plan.x;
    const std::size_t expected_texels =
        static_cast<std::size_t>(plan.width) * plan.height;
    if (plan.source_pitch != kTextureStride ||
        plan.source_start != expected_source_offset ||
        plan.destination_pitch != indexed_frame_width ||
        plan.destination_start != expected_destination_offset ||
        command.texels.size() != expected_texels ||
        command.sample_addresses.size() != expected_texels) {
        *message = "unsupported: owned sprite tile does not match its resolved 256-stride plan";
        return false;
    }

    const bool has_nonzero_source = std::any_of(
        command.texels.begin(), command.texels.end(),
        [](std::uint8_t value) { return value != 0U; });
    if (plan.pixel_mode == HERCULES_RECTANGLE_COPY) {
        if (plan.blend_selector != 0U || !command.blend_table.empty()) {
            *message = "unsupported: sprite COPY requires selector zero and no LUT snapshot";
            return false;
        }
    } else if (plan.pixel_mode == HERCULES_RECTANGLE_RAW_BLEND) {
        if (plan.blend_selector > 3U) {
            *message = "unsupported: sprite RAW_BLEND selector must be in 0..3";
            return false;
        }
        if ((has_nonzero_source &&
             command.blend_table.size() != kBlendTableBytes) ||
            (!has_nonzero_source && !command.blend_table.empty())) {
            *message = "unsupported: sprite RAW_BLEND requires a 65536-byte selected LUT iff any source is nonzero";
            return false;
        }
    } else {
        *message = "unsupported: sprite pixel mode must be COPY or RAW_BLEND";
        return false;
    }

    std::size_t sample = 0;
    for (std::uint32_t row = 0; row < plan.height; ++row) {
        for (std::uint32_t column = 0; column < plan.width; ++column) {
            const std::uint64_t expected_address =
                static_cast<std::uint64_t>(plan.page_address) +
                expected_source_offset +
                static_cast<std::uint64_t>(row) * plan.source_pitch + column;
            if (expected_address > std::numeric_limits<std::uint32_t>::max() ||
                command.sample_addresses[sample] != expected_address) {
                *message = "unsupported: sprite sample addresses do not match the page/UV tile association";
                return false;
            }
            ++sample;
        }
    }
    return true;
}

bool create_texture(SDL_GPUDevice *device, SDL_GPUTextureUsageFlags usage,
                    std::uint32_t width, std::uint32_t height,
                    SDL_GPUTexture **texture, std::string *message)
{
    SDL_GPUTextureCreateInfo info = {};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R8_UINT;
    info.usage = usage;
    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    *texture = SDL_CreateGPUTexture(device, &info);
    if (*texture == nullptr) {
        *message = with_sdl_error("SDL_CreateGPUTexture(R8_UINT)");
        return false;
    }
    return true;
}

} // namespace

struct IndexedGpuBackend::Impl {
    SDL_GPUDevice *device = nullptr;
    SDL_GPUShader *vertex_shader = nullptr;
    SDL_GPUShader *fragment_shader = nullptr;
    SDL_GPUGraphicsPipeline *pipeline = nullptr;
    IndexedGpuStatus status = IndexedGpuStatus::error;
    std::string message;

    ~Impl()
    {
        if (pipeline != nullptr)
            SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
        if (vertex_shader != nullptr)
            SDL_ReleaseGPUShader(device, vertex_shader);
        if (fragment_shader != nullptr)
            SDL_ReleaseGPUShader(device, fragment_shader);
    }

    bool initialize(SDL_GPUDevice *gpu_device)
    {
        device = gpu_device;
        if (device == nullptr) {
            status = IndexedGpuStatus::error;
            message = "SDL_GPUDevice is null";
            return false;
        }

        if (!SDL_GPUTextureSupportsFormat(device, SDL_GPU_TEXTUREFORMAT_R8_UINT,
                SDL_GPU_TEXTURETYPE_2D,
                SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ)) {
            status = IndexedGpuStatus::unsupported;
            message = "R8_UINT 2D graphics storage-read texture is unsupported by this adapter";
            return false;
        }
        if (!SDL_GPUTextureSupportsFormat(device, SDL_GPU_TEXTUREFORMAT_R8_UINT,
                SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_COLOR_TARGET)) {
            status = IndexedGpuStatus::unsupported;
            message = "R8_UINT 2D color target is unsupported by this adapter";
            return false;
        }

        HMODULE compiler_module = LoadLibraryExA("d3dcompiler_47.dll", nullptr,
            LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (compiler_module == nullptr) {
            status = IndexedGpuStatus::unsupported;
            message = "system d3dcompiler_47.dll is unavailable";
            return false;
        }
        const FARPROC procedure = GetProcAddress(compiler_module, "D3DCompile");
        pD3DCompile compile = nullptr;
        static_assert(sizeof(compile) == sizeof(procedure),
                      "D3DCompile pointer size must match FARPROC");
        std::memcpy(&compile, &procedure, sizeof(compile));
        if (compile == nullptr) {
            status = IndexedGpuStatus::unsupported;
            message = "system d3dcompiler_47.dll has no D3DCompile export";
            FreeLibrary(compiler_module);
            return false;
        }

        ID3DBlob *vertex_bytecode = nullptr;
        ID3DBlob *fragment_bytecode = nullptr;
        const bool vertex_ok = compile_shader(
            compile, "VSMain", "vs_5_1", &vertex_bytecode, &message);
        const bool fragment_ok = vertex_ok && compile_shader(
            compile, "PSMain", "ps_5_1", &fragment_bytecode, &message);
        if (!vertex_ok || !fragment_ok) {
            if (vertex_bytecode != nullptr)
                vertex_bytecode->Release();
            if (fragment_bytecode != nullptr)
                fragment_bytecode->Release();
            status = IndexedGpuStatus::error;
            FreeLibrary(compiler_module);
            return false;
        }

        SDL_GPUShaderCreateInfo vertex_info = {};
        vertex_info.code_size = vertex_bytecode->GetBufferSize();
        vertex_info.code = static_cast<const Uint8 *>(
            vertex_bytecode->GetBufferPointer());
        vertex_info.entrypoint = "VSMain";
        vertex_info.format = SDL_GPU_SHADERFORMAT_DXBC;
        vertex_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
        vertex_shader = SDL_CreateGPUShader(device, &vertex_info);

        SDL_GPUShaderCreateInfo fragment_info = {};
        fragment_info.code_size = fragment_bytecode->GetBufferSize();
        fragment_info.code = static_cast<const Uint8 *>(
            fragment_bytecode->GetBufferPointer());
        fragment_info.entrypoint = "PSMain";
        fragment_info.format = SDL_GPU_SHADERFORMAT_DXBC;
        fragment_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        fragment_info.num_storage_textures = 4;
        fragment_info.num_uniform_buffers = 1;
        fragment_shader = SDL_CreateGPUShader(device, &fragment_info);
        vertex_bytecode->Release();
        fragment_bytecode->Release();
        FreeLibrary(compiler_module);
        if (vertex_shader == nullptr || fragment_shader == nullptr) {
            status = IndexedGpuStatus::error;
            message = with_sdl_error("SDL_CreateGPUShader(DXBC)");
            return false;
        }

        SDL_GPUColorTargetDescription color_target = {};
        color_target.format = SDL_GPU_TEXTUREFORMAT_R8_UINT;
        SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
        pipeline_info.vertex_shader = vertex_shader;
        pipeline_info.fragment_shader = fragment_shader;
        pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        pipeline_info.rasterizer_state.enable_depth_clip = true;
        pipeline_info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
        pipeline_info.target_info.color_target_descriptions = &color_target;
        pipeline_info.target_info.num_color_targets = 1;
        pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
        if (pipeline == nullptr) {
            status = IndexedGpuStatus::error;
            message = with_sdl_error("SDL_CreateGPUGraphicsPipeline(R8_UINT)");
            return false;
        }

        status = IndexedGpuStatus::ok;
        message = "runtime HLSL compiled vs_5_1/ps_5_1 DXBC; R8_UINT source/seed/LUT storage reads and integer color target ready";
        return true;
    }
};

struct IndexedGpuBackend::DrawSnapshot {
    std::uint32_t x;
    std::uint32_t y;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t pixel_mode;
    std::uint32_t blend_selector;
    std::uint32_t shade_row_index;
    const std::vector<std::uint8_t> *texels;
    const std::vector<std::uint8_t> *blend_table;
    const std::vector<std::uint8_t> *shade_table;
};

IndexedGpuBackend::IndexedGpuBackend(SDL_GPUDevice *device)
    : impl_(std::make_unique<Impl>())
{
    impl_->initialize(device);
}

IndexedGpuBackend::~IndexedGpuBackend() = default;

IndexedGpuStatus IndexedGpuBackend::initialization_status() const noexcept
{
    return impl_->status;
}

const std::string &IndexedGpuBackend::initialization_message() const noexcept
{
    return impl_->message;
}

IndexedGpuResult IndexedGpuBackend::render(
    const IndexedRectangleCommand &command,
    const IndexedFrame &initial_indices)
{
    if (impl_->status != IndexedGpuStatus::ok)
        return failure(impl_->status, impl_->message);
    std::string validation_message;
    if (!validate_snapshot(command, &validation_message))
        return failure(IndexedGpuStatus::unsupported,
                       std::move(validation_message));

    const DrawSnapshot draw = {
        command.plan.x, command.plan.y,
        command.plan.width, command.plan.height,
        command.plan.pixel_mode, command.plan.blend_selector,
        command.plan.shade_row,
        &command.texels, &command.blend_table, &command.shade_row
    };
    return render_validated(draw, initial_indices);
}

IndexedGpuResult IndexedGpuBackend::render(
    const IndexedSpriteCommand &command,
    const IndexedFrame &initial_indices)
{
    if (impl_->status != IndexedGpuStatus::ok)
        return failure(impl_->status, impl_->message);
    std::string validation_message;
    if (!validate_sprite_snapshot(command, &validation_message))
        return failure(IndexedGpuStatus::unsupported,
                       std::move(validation_message));

    const DrawSnapshot draw = {
        command.plan.x, command.plan.y,
        command.plan.width, command.plan.height,
        static_cast<std::uint32_t>(command.plan.pixel_mode),
        command.plan.blend_selector, 0U,
        &command.texels, &command.blend_table, nullptr
    };
    return render_validated(draw, initial_indices);
}

IndexedGpuResult IndexedGpuBackend::render_validated(
    const DrawSnapshot &draw, const IndexedFrame &initial_indices)
{
    if (impl_->status != IndexedGpuStatus::ok)
        return failure(impl_->status, impl_->message);

    std::string validation_message;

    GpuRunResources resources;
    resources.device = impl_->device;
    if (!create_texture(resources.device, SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
            static_cast<std::uint32_t>(indexed_frame_width),
            static_cast<std::uint32_t>(indexed_frame_height),
            &resources.target, &validation_message) ||
        !create_texture(resources.device,
            SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ,
            static_cast<std::uint32_t>(indexed_frame_width),
            static_cast<std::uint32_t>(indexed_frame_height),
            &resources.seed_frame, &validation_message) ||
        !create_texture(resources.device,
            SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ,
            draw.width, draw.height,
            &resources.source, &validation_message) ||
        !create_texture(resources.device,
            SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ,
            256U, 256U, &resources.blend_table, &validation_message) ||
        !create_texture(resources.device,
            SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ,
            256U, 1U, &resources.shade_row, &validation_message)) {
        return failure(IndexedGpuStatus::error, std::move(validation_message));
    }

    const std::size_t blend_upload_offset = align_upload_offset(
        kTileUploadOffset + draw.texels->size());
    const std::size_t shade_upload_offset =
        blend_upload_offset + kBlendTableBytes;
    const std::size_t upload_bytes_count = shade_upload_offset + kShadeRowBytes;
    SDL_GPUTransferBufferCreateInfo upload_info = {};
    upload_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    upload_info.size = static_cast<Uint32>(upload_bytes_count);
    resources.upload = SDL_CreateGPUTransferBuffer(resources.device, &upload_info);
    if (resources.upload == nullptr) {
        return failure(IndexedGpuStatus::error,
                       with_sdl_error("SDL_CreateGPUTransferBuffer(upload)"));
    }
    SDL_GPUTransferBufferCreateInfo download_info = {};
    download_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    download_info.size = static_cast<Uint32>(indexed_frame_bytes);
    resources.download = SDL_CreateGPUTransferBuffer(resources.device,
                                                      &download_info);
    if (resources.download == nullptr) {
        return failure(IndexedGpuStatus::error,
                       with_sdl_error("SDL_CreateGPUTransferBuffer(download)"));
    }

    auto *upload_bytes = static_cast<std::uint8_t *>(
        SDL_MapGPUTransferBuffer(resources.device, resources.upload, false));
    if (upload_bytes == nullptr) {
        return failure(IndexedGpuStatus::error,
                       with_sdl_error("SDL_MapGPUTransferBuffer(upload)"));
    }
    std::memcpy(upload_bytes, initial_indices.data(), indexed_frame_bytes);
    std::memcpy(upload_bytes + kTileUploadOffset, draw.texels->data(),
                draw.texels->size());
    std::memset(upload_bytes + blend_upload_offset, 0, kBlendTableBytes);
    if (draw.blend_table != nullptr && !draw.blend_table->empty()) {
        std::memcpy(upload_bytes + blend_upload_offset,
                    draw.blend_table->data(), draw.blend_table->size());
    }
    std::memset(upload_bytes + shade_upload_offset, 0, kShadeRowBytes);
    if (draw.shade_table != nullptr && !draw.shade_table->empty()) {
        std::memcpy(upload_bytes + shade_upload_offset,
                    draw.shade_table->data(), draw.shade_table->size());
    }
    SDL_UnmapGPUTransferBuffer(resources.device, resources.upload);

    SDL_GPUCommandBuffer *gpu_command =
        SDL_AcquireGPUCommandBuffer(resources.device);
    if (gpu_command == nullptr) {
        return failure(IndexedGpuStatus::error,
                       with_sdl_error("SDL_AcquireGPUCommandBuffer"));
    }

    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(gpu_command);
    if (copy_pass == nullptr) {
        SDL_CancelGPUCommandBuffer(gpu_command);
        return failure(IndexedGpuStatus::error,
                       with_sdl_error("SDL_BeginGPUCopyPass(upload)"));
    }
    const SDL_GPUTextureTransferInfo frame_source = {
        resources.upload, 0,
        static_cast<Uint32>(indexed_frame_width),
        static_cast<Uint32>(indexed_frame_height)
    };
    const SDL_GPUTextureRegion frame_destination = {
        resources.target, 0, 0, 0, 0, 0,
        static_cast<Uint32>(indexed_frame_width),
        static_cast<Uint32>(indexed_frame_height), 1
    };
    SDL_UploadToGPUTexture(copy_pass, &frame_source, &frame_destination, false);
    const SDL_GPUTextureRegion seed_destination = {
        resources.seed_frame, 0, 0, 0, 0, 0,
        static_cast<Uint32>(indexed_frame_width),
        static_cast<Uint32>(indexed_frame_height), 1
    };
    SDL_UploadToGPUTexture(copy_pass, &frame_source, &seed_destination, false);
    const SDL_GPUTextureTransferInfo tile_source = {
        resources.upload, static_cast<Uint32>(kTileUploadOffset),
        draw.width, draw.height
    };
    const SDL_GPUTextureRegion tile_destination = {
        resources.source, 0, 0, 0, 0, 0,
        draw.width, draw.height, 1
    };
    SDL_UploadToGPUTexture(copy_pass, &tile_source, &tile_destination, false);
    const SDL_GPUTextureTransferInfo blend_source = {
        resources.upload, static_cast<Uint32>(blend_upload_offset), 256U, 256U
    };
    const SDL_GPUTextureRegion blend_destination = {
        resources.blend_table, 0, 0, 0, 0, 0, 256U, 256U, 1
    };
    SDL_UploadToGPUTexture(copy_pass, &blend_source, &blend_destination, false);
    const SDL_GPUTextureTransferInfo shade_source = {
        resources.upload, static_cast<Uint32>(shade_upload_offset), 256U, 1U
    };
    const SDL_GPUTextureRegion shade_destination = {
        resources.shade_row, 0, 0, 0, 0, 0, 256U, 1U, 1
    };
    SDL_UploadToGPUTexture(copy_pass, &shade_source, &shade_destination, false);
    SDL_EndGPUCopyPass(copy_pass);

    SDL_GPUColorTargetInfo color_info = {};
    color_info.texture = resources.target;
    color_info.load_op = SDL_GPU_LOADOP_LOAD;
    color_info.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass *render_pass =
        SDL_BeginGPURenderPass(gpu_command, &color_info, 1, nullptr);
    if (render_pass == nullptr) {
        SDL_CancelGPUCommandBuffer(gpu_command);
        return failure(IndexedGpuStatus::error,
                       with_sdl_error("SDL_BeginGPURenderPass(R8_UINT)"));
    }
    SDL_BindGPUGraphicsPipeline(render_pass, impl_->pipeline);
    const SDL_GPUViewport viewport = {
        0.0f, 0.0f, static_cast<float>(indexed_frame_width),
        static_cast<float>(indexed_frame_height), 0.0f, 1.0f
    };
    const SDL_Rect scissor = {
        0, 0, static_cast<int>(indexed_frame_width),
        static_cast<int>(indexed_frame_height)
    };
    SDL_SetGPUViewport(render_pass, &viewport);
    SDL_SetGPUScissor(render_pass, &scissor);
    SDL_GPUTexture *storage_textures[] = {
        resources.source, resources.seed_frame,
        resources.blend_table, resources.shade_row
    };
    SDL_BindGPUFragmentStorageTextures(render_pass, 0, storage_textures, 4);
    const RectangleUniform uniform = {{
        static_cast<std::int32_t>(draw.x),
        static_cast<std::int32_t>(draw.y),
        static_cast<std::int32_t>(draw.width),
        static_cast<std::int32_t>(draw.height)
    }, {
        draw.pixel_mode,
        draw.blend_selector,
        draw.shade_row_index,
        0U
    }};
    SDL_PushGPUFragmentUniformData(gpu_command, 0, &uniform, sizeof(uniform));
    SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(render_pass);

    copy_pass = SDL_BeginGPUCopyPass(gpu_command);
    if (copy_pass == nullptr) {
        SDL_CancelGPUCommandBuffer(gpu_command);
        return failure(IndexedGpuStatus::error,
                       with_sdl_error("SDL_BeginGPUCopyPass(download)"));
    }
    const SDL_GPUTextureRegion readback_source = {
        resources.target, 0, 0, 0, 0, 0,
        static_cast<Uint32>(indexed_frame_width),
        static_cast<Uint32>(indexed_frame_height), 1
    };
    const SDL_GPUTextureTransferInfo readback_destination = {
        resources.download, 0,
        static_cast<Uint32>(indexed_frame_width),
        static_cast<Uint32>(indexed_frame_height)
    };
    SDL_DownloadFromGPUTexture(copy_pass, &readback_source,
                               &readback_destination);
    SDL_EndGPUCopyPass(copy_pass);

    SDL_GPUFence *fence =
        SDL_SubmitGPUCommandBufferAndAcquireFence(gpu_command);
    if (fence == nullptr) {
        return failure(IndexedGpuStatus::error,
                       with_sdl_error("SDL_SubmitGPUCommandBufferAndAcquireFence"));
    }
    const bool waited = SDL_WaitForGPUFences(resources.device, true, &fence, 1);
    SDL_ReleaseGPUFence(resources.device, fence);
    if (!waited) {
        return failure(IndexedGpuStatus::error,
                       with_sdl_error("SDL_WaitForGPUFences"));
    }

    const auto *readback = static_cast<const std::uint8_t *>(
        SDL_MapGPUTransferBuffer(resources.device, resources.download, false));
    if (readback == nullptr) {
        return failure(IndexedGpuStatus::error,
                       with_sdl_error("SDL_MapGPUTransferBuffer(download)"));
    }
    IndexedGpuResult result;
    std::memcpy(result.indices.data(), readback, indexed_frame_bytes);
    SDL_UnmapGPUTransferBuffer(resources.device, resources.download);
    result.status = IndexedGpuStatus::ok;
    result.message = "R8_UINT indexed draw and full readback completed after GPU fence";
    return result;
}

} // namespace hercules::dropin
