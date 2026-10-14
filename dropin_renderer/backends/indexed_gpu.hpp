#ifndef HERCULES_DROPIN_INDEXED_GPU_HPP
#define HERCULES_DROPIN_INDEXED_GPU_HPP

#include <memory>
#include <string>

#include <SDL3/SDL_gpu.h>

#include "indexed_reference.hpp"

namespace hercules::dropin {

enum class IndexedGpuStatus {
    ok,
    unsupported,
    error
};

struct IndexedGpuResult {
    IndexedGpuStatus status = IndexedGpuStatus::error;
    IndexedFrame indices{};
    std::string message;
};

/* Offscreen indexed FT4 rectangle backend for the shared plan's bounded
 * unblended tpage/clut-zero 1:1 domain. It owns no source data: render accepts
 * a const, detached command snapshot and a const seed frame, consumes both
 * synchronously, and returns a separate fence-complete readback. */
class IndexedGpuBackend {
public:
    explicit IndexedGpuBackend(SDL_GPUDevice *device);
    ~IndexedGpuBackend();

    IndexedGpuBackend(const IndexedGpuBackend &) = delete;
    IndexedGpuBackend &operator=(const IndexedGpuBackend &) = delete;

    IndexedGpuStatus initialization_status() const noexcept;
    const std::string &initialization_message() const noexcept;

    IndexedGpuResult render(const IndexedRectangleCommand &command,
                            const IndexedFrame &initial_indices);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace hercules::dropin

#endif
