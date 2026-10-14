#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3dcompiler.h>

#include <array>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
constexpr Uint32 kWidth = 1920;
constexpr Uint32 kHeight = 1440;
constexpr Uint8 kClearPixel[4] = {255, 0, 255, 255};
constexpr Uint8 kPolygonPixel[4] = {0, 255, 0, 255};

/* A small synthetic quad inside the test target. Custom --quad coordinates
 * are passed in the order the renderer's triangle-strip input expects. */
const std::array<double, 8> kSyntheticQuad = {
    64.0, 64.0, 256.0, 64.0,
    64.0, 256.0, 256.0, 256.0
};

struct GpuResources {
    SDL_GPUTexture *target = nullptr;
    SDL_GPUTransferBuffer *upload = nullptr;
    SDL_GPUTransferBuffer *download = nullptr;
    SDL_GPUBuffer *vertices = nullptr;
    SDL_GPUShader *vertex_shader = nullptr;
    SDL_GPUShader *fragment_shader = nullptr;
    SDL_GPUGraphicsPipeline *pipeline = nullptr;

    ~GpuResources()
    {
        if (pipeline != nullptr)
            SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
        if (vertex_shader != nullptr)
            SDL_ReleaseGPUShader(device, vertex_shader);
        if (fragment_shader != nullptr)
            SDL_ReleaseGPUShader(device, fragment_shader);
        if (vertices != nullptr)
            SDL_ReleaseGPUBuffer(device, vertices);
        if (upload != nullptr)
            SDL_ReleaseGPUTransferBuffer(device, upload);
        if (download != nullptr)
            SDL_ReleaseGPUTransferBuffer(device, download);
        if (target != nullptr)
            SDL_ReleaseGPUTexture(device, target);
    }

    SDL_GPUDevice *device = nullptr;
};

void PrintFormatSupport(SDL_GPUDevice *device)
{
    struct FormatCheck {
        const char *name;
        SDL_GPUTextureFormat format;
        SDL_GPUTextureUsageFlags usage;
        const char *usage_name;
    };
    const FormatCheck formats[] = {
        {"R8G8B8A8_UNORM", SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            SDL_GPU_TEXTUREUSAGE_COLOR_TARGET, "color_target"},
        {"B8G8R8A8_UNORM", SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
            SDL_GPU_TEXTUREUSAGE_COLOR_TARGET, "color_target"},
        {"R8G8B8A8_UNORM_SRGB", SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
            SDL_GPU_TEXTUREUSAGE_COLOR_TARGET, "color_target"},
        {"D24_UNORM_S8_UINT", SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
            SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET, "depth_target"},
        {"D32_FLOAT", SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
            SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET, "depth_target"}
    };
    for (size_t i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i) {
        const bool supported = SDL_GPUTextureSupportsFormat(device, formats[i].format,
            SDL_GPU_TEXTURETYPE_2D, formats[i].usage);
        std::printf("format %s %s=%s\n", formats[i].name, formats[i].usage_name,
            supported ? "yes" : "no");
    }
}

bool CompileShader(pD3DCompile compile,
                   const char *entrypoint,
                   const char *target,
                   ID3DBlob **shader)
{
    static const char source[] =
        "struct VSInput { float2 position : TEXCOORD0; };\n"
        "struct VSOutput { float4 position : SV_Position; };\n"
        "VSOutput VSMain(VSInput input) {\n"
        "  VSOutput output; output.position = float4(input.position, 0.5, 1.0);\n"
        "  return output;\n"
        "}\n"
        "float4 PSMain() : SV_Target0 { return float4(0.0, 1.0, 0.0, 1.0); }\n";
    ID3DBlob *diagnostics = nullptr;
    const HRESULT result = compile(source, sizeof(source) - 1, "sdl_gpu_probe.hlsl",
        nullptr, nullptr, entrypoint, target, D3DCOMPILE_ENABLE_STRICTNESS,
        0, shader, &diagnostics);
    if (diagnostics != nullptr) {
        if (FAILED(result)) {
            const char *message = static_cast<const char *>(diagnostics->GetBufferPointer());
            std::fprintf(stderr, "HLSL %s diagnostics: %.*s\n", entrypoint,
                static_cast<int>(diagnostics->GetBufferSize()), message);
        }
        diagnostics->Release();
    }
    if (FAILED(result)) {
        std::fprintf(stderr, "FAIL: D3DCompile(%s, %s) returned HRESULT 0x%08lx\n",
            entrypoint, target, static_cast<unsigned long>(result));
        return false;
    }
    return true;
}

bool CreatePipeline(GpuResources *resources, bool *prerequisite_missing)
{
    *prerequisite_missing = false;
    HMODULE compiler_module = LoadLibraryExA("d3dcompiler_47.dll", nullptr,
        LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (compiler_module == nullptr) {
        std::fprintf(stderr, "SKIP: system d3dcompiler_47.dll is unavailable\n");
        *prerequisite_missing = true;
        return false;
    }
    const FARPROC compiler_procedure = GetProcAddress(compiler_module, "D3DCompile");
    pD3DCompile compile = nullptr;
    static_assert(sizeof(compile) == sizeof(compiler_procedure),
        "Win32 API and shader compiler function pointers must have equal size");
    std::memcpy(&compile, &compiler_procedure, sizeof(compile));
    if (compile == nullptr) {
        std::fprintf(stderr, "SKIP: d3dcompiler_47.dll has no D3DCompile export\n");
        *prerequisite_missing = true;
        FreeLibrary(compiler_module);
        return false;
    }

    ID3DBlob *vertex_bytecode = nullptr;
    ID3DBlob *fragment_bytecode = nullptr;
    const bool vertex_compiled = CompileShader(compile, "VSMain", "vs_5_1", &vertex_bytecode);
    const bool fragment_compiled = vertex_compiled &&
        CompileShader(compile, "PSMain", "ps_5_1", &fragment_bytecode);
    if (!vertex_compiled || !fragment_compiled) {
        if (vertex_bytecode != nullptr)
            vertex_bytecode->Release();
        if (fragment_bytecode != nullptr)
            fragment_bytecode->Release();
        FreeLibrary(compiler_module);
        return false;
    }

    const SDL_GPUShaderCreateInfo vertex_info = {
        vertex_bytecode->GetBufferSize(),
        static_cast<const Uint8 *>(vertex_bytecode->GetBufferPointer()),
        "VSMain",
        SDL_GPU_SHADERFORMAT_DXBC,
        SDL_GPU_SHADERSTAGE_VERTEX,
        0, 0, 0, 0,
        0
    };
    const SDL_GPUShaderCreateInfo fragment_info = {
        fragment_bytecode->GetBufferSize(),
        static_cast<const Uint8 *>(fragment_bytecode->GetBufferPointer()),
        "PSMain",
        SDL_GPU_SHADERFORMAT_DXBC,
        SDL_GPU_SHADERSTAGE_FRAGMENT,
        0, 0, 0, 0,
        0
    };
    resources->vertex_shader = SDL_CreateGPUShader(resources->device, &vertex_info);
    resources->fragment_shader = SDL_CreateGPUShader(resources->device, &fragment_info);
    vertex_bytecode->Release();
    fragment_bytecode->Release();
    FreeLibrary(compiler_module);
    if (resources->vertex_shader == nullptr || resources->fragment_shader == nullptr) {
        std::fprintf(stderr, "FAIL: SDL_CreateGPUShader: %s\n", SDL_GetError());
        return false;
    }

    const SDL_GPUVertexBufferDescription buffer_description = {
        0,
        sizeof(float) * 2,
        SDL_GPU_VERTEXINPUTRATE_VERTEX,
        0
    };
    const SDL_GPUVertexAttribute attribute = {
        0,
        0,
        SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
        0
    };
    SDL_GPUColorTargetDescription color_description = {};
    color_description.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.vertex_shader = resources->vertex_shader;
    pipeline_info.fragment_shader = resources->fragment_shader;
    pipeline_info.vertex_input_state.vertex_buffer_descriptions = &buffer_description;
    pipeline_info.vertex_input_state.num_vertex_buffers = 1;
    pipeline_info.vertex_input_state.vertex_attributes = &attribute;
    pipeline_info.vertex_input_state.num_vertex_attributes = 1;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
    pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pipeline_info.rasterizer_state.enable_depth_clip = true;
    pipeline_info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    pipeline_info.target_info.color_target_descriptions = &color_description;
    pipeline_info.target_info.num_color_targets = 1;

    resources->pipeline = SDL_CreateGPUGraphicsPipeline(resources->device, &pipeline_info);
    if (resources->pipeline == nullptr) {
        std::fprintf(stderr, "FAIL: SDL_CreateGPUGraphicsPipeline: %s\n", SDL_GetError());
        return false;
    }
    std::printf("runtime HLSL: system d3dcompiler_47.dll compiled vs_5_1/ps_5_1 DXBC\n");
    return true;
}

bool RunDiagnostic(SDL_GPUDevice *device,
                   const std::array<double, 8> &quad,
                   bool expect_visible_polygon,
                   bool *prerequisite_missing)
{
    *prerequisite_missing = false;
    if (!SDL_GPUTextureSupportsFormat(device, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_COLOR_TARGET)) {
        std::fprintf(stderr, "SKIP: RGBA8 color target format is unsupported\n");
        *prerequisite_missing = true;
        return false;
    }

    GpuResources resources;
    resources.device = device;
    const SDL_GPUTextureCreateInfo texture_info = {
        SDL_GPU_TEXTURETYPE_2D,
        SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
        kWidth,
        kHeight,
        1,
        1,
        SDL_GPU_SAMPLECOUNT_1,
        0
    };
    resources.target = SDL_CreateGPUTexture(device, &texture_info);
    if (resources.target == nullptr) {
        std::fprintf(stderr, "FAIL: SDL_CreateGPUTexture: %s\n", SDL_GetError());
        return false;
    }

    const Uint32 byte_count = 4 * kWidth * kHeight;
    const SDL_GPUTransferBufferCreateInfo download_info = {
        SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, byte_count, 0
    };
    resources.download = SDL_CreateGPUTransferBuffer(device, &download_info);
    if (resources.download == nullptr) {
        std::fprintf(stderr, "FAIL: SDL_CreateGPUTransferBuffer(download): %s\n", SDL_GetError());
        return false;
    }

    const float vertices[8] = {
        static_cast<float>(2.0 * quad[0] / kWidth - 1.0),
        static_cast<float>(1.0 - 2.0 * quad[1] / kHeight),
        static_cast<float>(2.0 * quad[2] / kWidth - 1.0),
        static_cast<float>(1.0 - 2.0 * quad[3] / kHeight),
        static_cast<float>(2.0 * quad[4] / kWidth - 1.0),
        static_cast<float>(1.0 - 2.0 * quad[5] / kHeight),
        static_cast<float>(2.0 * quad[6] / kWidth - 1.0),
        static_cast<float>(1.0 - 2.0 * quad[7] / kHeight)
    };
    const SDL_GPUBufferCreateInfo vertex_buffer_info = {
        SDL_GPU_BUFFERUSAGE_VERTEX,
        sizeof(vertices),
        0
    };
    resources.vertices = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
    if (resources.vertices == nullptr) {
        std::fprintf(stderr, "FAIL: SDL_CreateGPUBuffer: %s\n", SDL_GetError());
        return false;
    }
    const SDL_GPUTransferBufferCreateInfo upload_info = {
        SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, sizeof(vertices), 0
    };
    resources.upload = SDL_CreateGPUTransferBuffer(device, &upload_info);
    if (resources.upload == nullptr) {
        std::fprintf(stderr, "FAIL: SDL_CreateGPUTransferBuffer(upload): %s\n", SDL_GetError());
        return false;
    }
    void *upload_data = SDL_MapGPUTransferBuffer(device, resources.upload, false);
    if (upload_data == nullptr) {
        std::fprintf(stderr, "FAIL: SDL_MapGPUTransferBuffer(upload): %s\n", SDL_GetError());
        return false;
    }
    std::memcpy(upload_data, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, resources.upload);
    if (!CreatePipeline(&resources, prerequisite_missing))
        return false;

    SDL_GPUCommandBuffer *command = SDL_AcquireGPUCommandBuffer(device);
    if (command == nullptr) {
        std::fprintf(stderr, "FAIL: SDL_AcquireGPUCommandBuffer: %s\n", SDL_GetError());
        return false;
    }
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command);
    if (copy_pass == nullptr) {
        std::fprintf(stderr, "FAIL: SDL_BeginGPUCopyPass(upload): %s\n", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command);
        return false;
    }
    const SDL_GPUTransferBufferLocation upload_location = {resources.upload, 0};
    const SDL_GPUBufferRegion vertex_region = {resources.vertices, 0, sizeof(vertices)};
    SDL_UploadToGPUBuffer(copy_pass, &upload_location, &vertex_region, false);
    SDL_EndGPUCopyPass(copy_pass);

    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture = resources.target;
    color_target.clear_color = {1.0f, 0.0f, 1.0f, 1.0f};
    color_target.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(command, &color_target, 1, nullptr);
    if (render_pass == nullptr) {
        std::fprintf(stderr, "FAIL: SDL_BeginGPURenderPass: %s\n", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command);
        return false;
    }
    SDL_BindGPUGraphicsPipeline(render_pass, resources.pipeline);
    const SDL_GPUViewport viewport = {0.0f, 0.0f,
        static_cast<float>(kWidth), static_cast<float>(kHeight), 0.0f, 1.0f};
    const SDL_Rect scissor = {0, 0, static_cast<int>(kWidth), static_cast<int>(kHeight)};
    SDL_SetGPUViewport(render_pass, &viewport);
    SDL_SetGPUScissor(render_pass, &scissor);
    const SDL_GPUBufferBinding vertex_binding = {resources.vertices, 0};
    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);
    SDL_DrawGPUPrimitives(render_pass, 4, 1, 0, 0);
    SDL_EndGPURenderPass(render_pass);

    copy_pass = SDL_BeginGPUCopyPass(command);
    if (copy_pass == nullptr) {
        std::fprintf(stderr, "FAIL: SDL_BeginGPUCopyPass(download): %s\n", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command);
        return false;
    }
    const SDL_GPUTextureRegion source = {
        resources.target, 0, 0, 0, 0, 0, kWidth, kHeight, 1
    };
    const SDL_GPUTextureTransferInfo destination = {resources.download, 0, 0, 0};
    SDL_DownloadFromGPUTexture(copy_pass, &source, &destination);
    SDL_EndGPUCopyPass(copy_pass);

    SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(command);
    if (fence == nullptr) {
        std::fprintf(stderr, "FAIL: SDL_SubmitGPUCommandBufferAndAcquireFence: %s\n", SDL_GetError());
        return false;
    }
    const bool waited = SDL_WaitForGPUFences(device, true, &fence, 1);
    SDL_ReleaseGPUFence(device, fence);
    if (!waited) {
        std::fprintf(stderr, "FAIL: SDL_WaitForGPUFences: %s\n", SDL_GetError());
        return false;
    }

    const Uint8 *pixels = static_cast<const Uint8 *>(
        SDL_MapGPUTransferBuffer(device, resources.download, false));
    if (pixels == nullptr) {
        std::fprintf(stderr, "FAIL: SDL_MapGPUTransferBuffer(download): %s\n", SDL_GetError());
        return false;
    }
    const auto pixel_at = [pixels](Uint32 x, Uint32 y) {
        return pixels + (static_cast<size_t>(y) * kWidth + x) * 4;
    };
    bool clear_matches = true;
    for (size_t i = 0; i < static_cast<size_t>(kWidth) * kHeight; ++i) {
        const Uint8 *pixel = pixels + i * 4;
        if (std::memcmp(pixel, kClearPixel, sizeof(kClearPixel)) != 0 &&
            std::memcmp(pixel, kPolygonPixel, sizeof(kPolygonPixel)) != 0) {
            std::fprintf(stderr, "FAIL: pixel[%zu] is neither the clear nor polygon color: {%u,%u,%u,%u}\n",
                i, pixel[0], pixel[1], pixel[2], pixel[3]);
            clear_matches = false;
            break;
        }
    }
    const Uint32 sample_x = expect_visible_polygon ? 160 : kWidth / 2;
    const Uint32 sample_y = expect_visible_polygon ? 160 : kHeight / 2;
    const Uint8 *top_left = pixel_at(0, 0);
    const Uint8 *sample = pixel_at(sample_x, sample_y);
    const bool corner_matches = std::memcmp(top_left, kClearPixel, 4) == 0;
    const bool sample_matches = std::memcmp(sample, kPolygonPixel, 4) == 0;
    size_t polygon_pixel_count = 0;
    for (size_t i = 0; i < static_cast<size_t>(kWidth) * kHeight; ++i) {
        if (std::memcmp(pixels + i * 4, kPolygonPixel, 4) == 0)
            ++polygon_pixel_count;
    }
    SDL_UnmapGPUTransferBuffer(device, resources.download);

    if (!clear_matches || !corner_matches || (expect_visible_polygon && !sample_matches)) {
        std::fprintf(stderr,
            "FAIL: readback check clear=%s corner=%s sample=(%u,%u):%s polygon_pixels=%zu\n",
            clear_matches ? "ok" : "bad", corner_matches ? "ok" : "bad",
            sample_x, sample_y, sample_matches ? "green" : "clear", polygon_pixel_count);
        return false;
    }
    std::printf("offscreen render/readback: %ux%u RGBA8 clear={255,0,255,255}, "
        "quad vertex strip={0,255,0,255}, polygon_pixels=%zu, sample=(%u,%u):%s\n",
        kWidth, kHeight, polygon_pixel_count, sample_x, sample_y,
        sample_matches ? "green" : "clear");
    return true;
}

bool ParseQuad(int argc, char **argv, std::array<double, 8> *quad)
{
    if (argc == 2 && std::strcmp(argv[1], "--diagnostic") == 0) {
        *quad = kSyntheticQuad;
        return true;
    }
    if (argc != 11 || std::strcmp(argv[1], "--diagnostic") != 0 ||
        std::strcmp(argv[2], "--quad") != 0) {
        return false;
    }
    for (size_t i = 0; i < quad->size(); ++i) {
        char *end = nullptr;
        errno = 0;
        const double value = std::strtod(argv[i + 3], &end);
        if (errno != 0 || end == argv[i + 3] || *end != '\0' || !std::isfinite(value))
            return false;
        (*quad)[i] = value;
    }
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    std::array<double, 8> quad{};
    const bool run_diagnostic = argc > 1;
    const bool valid_arguments = argc == 1 || ParseQuad(argc, argv, &quad);
    if (!valid_arguments) {
        std::fprintf(stderr,
            "usage: sdl_gpu_probe [--diagnostic [--quad x0 y0 x1 y1 x2 y2 x3 y3]]\n");
        return 2;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SKIP: SDL_INIT_VIDEO failed: %s\n", SDL_GetError());
        return 77;
    }

    std::printf("SDL GPU compiled drivers:");
    for (int i = 0; i < SDL_GetNumGPUDrivers(); ++i) {
        const char *name = SDL_GetGPUDriver(i);
        std::printf(" %s", name ? name : "<null>");
    }
    std::printf("\n");

    const SDL_GPUShaderFormat formats = static_cast<SDL_GPUShaderFormat>(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXBC | SDL_GPU_SHADERFORMAT_DXIL);
    const char *driver_preference = run_diagnostic ? "direct3d12" : nullptr;
    SDL_GPUDevice *device = SDL_CreateGPUDevice(formats, false, driver_preference);
    if (device == nullptr) {
        std::fprintf(stderr, "SKIP: SDL_CreateGPUDevice(%s) failed: %s\n",
            driver_preference ? driver_preference : "auto", SDL_GetError());
        SDL_Quit();
        return 77;
    }

    const char *driver = SDL_GetGPUDeviceDriver(device);
    const SDL_GPUShaderFormat available_formats = SDL_GetGPUShaderFormats(device);
    std::printf("SDL GPU device created: driver=%s shader_formats=0x%08x\n",
        driver ? driver : "<null>", static_cast<unsigned int>(available_formats));
    PrintFormatSupport(device);

    bool ok = true;
    bool diagnostic_prerequisite_missing = false;
    if (run_diagnostic) {
        if (driver == nullptr || std::strcmp(driver, "direct3d12") != 0 ||
            (available_formats & SDL_GPU_SHADERFORMAT_DXBC) == 0) {
            std::fprintf(stderr, "SKIP: runtime HLSL diagnostic requires Direct3D 12 DXBC support\n");
            SDL_DestroyGPUDevice(device);
            SDL_Quit();
            return 77;
        }
        const bool synthetic_quad = argc == 2;
        std::printf("illustrative quad coordinates: %.3f,%.3f %.3f,%.3f %.3f,%.3f %.3f,%.3f%s\n",
            quad[0], quad[1], quad[2], quad[3], quad[4], quad[5], quad[6], quad[7],
            synthetic_quad ? " (synthetic)" : " (caller-supplied)");
        ok = RunDiagnostic(device, quad, synthetic_quad,
            &diagnostic_prerequisite_missing);
    }

    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    if (!ok) {
        return diagnostic_prerequisite_missing ? 77 : 1;
    }
    std::printf("SDL GPU device destroyed cleanly\n");
    return 0;
}
