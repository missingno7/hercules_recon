#ifndef DROPIN_RENDERER_PROXY_API_HPP
#define DROPIN_RENDERER_PROXY_API_HPP

#include "hercules_host_abi.h"

#if defined(_MSC_VER)
#define DROPIN_PROXY_CDECL __cdecl
#else
#define DROPIN_PROXY_CDECL __attribute__((cdecl))
#endif

namespace dropin_renderer::proxy {

enum class InstallStatus : renderer_u32 {
    installed = 0,
    invalid_argument = 1,
    source_pointer_mismatch = 2,
    bad_signature = 3,
    null_callback = 4,
    clone_rejected = 5,
    already_installed = 6,
    install_busy = 7
};

enum class ObservationKind : renderer_u32 {
    slot81_enter = 1,
    slot81_return = 2,
    slot89_enter = 3,
    slot89_return = 4
};

/* The sink receives a stack record synchronously and must copy it if it needs
 * to retain the observation. It may be called recursively or concurrently. */
struct Observation {
    renderer_u32 sequence;
    renderer_u32 thread_id;
    renderer_u32 slot;
    renderer_u32 kind;
    renderer_u32 argument0;
    renderer_u32 argument1;
    renderer_u32 raw_eax;
    renderer_u32 target;
};

using ObservationSink = void (DROPIN_PROXY_CDECL *)(
    const Observation *observation, void *context);

/* Installs one process-lifetime 250-word clone into an explicitly supplied
 * synthetic host pointer cell. The caller must provide readable source words
 * and a writable, aligned pointer cell containing source_words, and keep both
 * stable for the duration of this call. Publication uses compare/exchange so
 * a pointer-cell change during installation is rejected. This does not run
 * from DllMain and does not attach automatically to Hercules. */
InstallStatus install_selected_engine_table(
    const renderer_u32 *source_words,
    renderer_u32 *host_pointer_cell,
    ObservationSink observer,
    void *observer_context) noexcept;

} // namespace dropin_renderer::proxy

#endif
