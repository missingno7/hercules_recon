#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <atomic>
#include <cstdint>

#include "dropin_renderer/proxy_api.hpp"

namespace dropin_renderer::proxy {
namespace {

static_assert(sizeof(void *) == sizeof(renderer_u32),
              "the live table adapter is an x86-only ABI");

using ChainInitFunction = int (DROPIN_PROXY_CDECL *)(void *, renderer_u32);
/* Slot 89 is semantically void in the host contract. This private physical
 * signature returns the raw x86 EAX word so synthetic observations can
 * capture it; the host does not consume that register for this callback. */
using SubmitPhysicalFunction = renderer_u32 (DROPIN_PROXY_CDECL *)(void *);

struct Installation {
    ChainInitFunction chain_init;
    SubmitPhysicalFunction submit;
    ObservationSink observer;
    void *observer_context;
};

alignas(renderer_u32) renderer_u32 g_table_clone[ENGINE_TABLE_WORD_COUNT] = {};
Installation g_installation = {};
std::atomic<const Installation *> g_installed{nullptr};
std::atomic_flag g_installing = ATOMIC_FLAG_INIT;
volatile LONG g_observation_sequence = 0;

renderer_u32 pointer_word(const void *pointer) noexcept
{
    return static_cast<renderer_u32>(reinterpret_cast<std::uintptr_t>(pointer));
}

template <typename Function>
renderer_u32 function_word(Function function) noexcept
{
    return static_cast<renderer_u32>(reinterpret_cast<std::uintptr_t>(function));
}

template <typename Function>
Function word_function(renderer_u32 word) noexcept
{
    return reinterpret_cast<Function>(static_cast<std::uintptr_t>(word));
}

const Installation *installed() noexcept
{
    return g_installed.load(std::memory_order_acquire);
}

void observe(const Installation *state, ObservationKind kind,
             renderer_u32 slot, renderer_u32 argument0,
             renderer_u32 argument1, renderer_u32 raw_eax,
             renderer_u32 target) noexcept
{
    if (state == nullptr || state->observer == nullptr)
        return;

    const DWORD previous_error = GetLastError();
    Observation observation = {};
    observation.sequence = static_cast<renderer_u32>(
        InterlockedIncrement(&g_observation_sequence));
    observation.thread_id = GetCurrentThreadId();
    observation.slot = slot;
    observation.kind = static_cast<renderer_u32>(kind);
    observation.argument0 = argument0;
    observation.argument1 = argument1;
    observation.raw_eax = raw_eax;
    observation.target = target;
    state->observer(&observation, state->observer_context);
    SetLastError(previous_error);
}

int DROPIN_PROXY_CDECL chain_init_wrapper(
    void *first_slot, renderer_u32 count) noexcept
{
    const Installation *state = installed();
    if (state == nullptr || state->chain_init == nullptr)
        return 0;

    const renderer_u32 first_word = pointer_word(first_slot);
    const renderer_u32 target = function_word(state->chain_init);
    observe(state, ObservationKind::slot81_enter,
            HERCULES_SLOT_CHAIN_INIT, first_word, count, 0, target);

    const int result = state->chain_init(first_slot, count);
    const DWORD callback_error = GetLastError();
    observe(state, ObservationKind::slot81_return,
            HERCULES_SLOT_CHAIN_INIT, first_word, count,
            static_cast<renderer_u32>(result), target);
    SetLastError(callback_error);
    return result;
}

renderer_u32 DROPIN_PROXY_CDECL submit_wrapper(void *first_slot) noexcept
{
    const Installation *state = installed();
    if (state == nullptr || state->submit == nullptr)
        return 0;

    const renderer_u32 first_word = pointer_word(first_slot);
    const renderer_u32 target = function_word(state->submit);
    observe(state, ObservationKind::slot89_enter,
            HERCULES_SLOT_SUBMIT, first_word, 0, 0, target);

    const renderer_u32 raw_eax = state->submit(first_slot);
    const DWORD callback_error = GetLastError();
    observe(state, ObservationKind::slot89_return,
            HERCULES_SLOT_SUBMIT, first_word, 0, raw_eax, target);
    SetLastError(callback_error);
    return raw_eax;
}

} // namespace

InstallStatus install_selected_engine_table(
    const renderer_u32 *source_words,
    renderer_u32 *host_pointer_cell,
    ObservationSink observer,
    void *observer_context) noexcept
{
    if (source_words == nullptr || host_pointer_cell == nullptr ||
        (reinterpret_cast<std::uintptr_t>(host_pointer_cell) &
         (sizeof(renderer_u32) - 1U)) != 0)
        return InstallStatus::invalid_argument;

    if (g_installing.test_and_set(std::memory_order_acquire))
        return InstallStatus::install_busy;

    const auto finish_failed_install = []() noexcept {
        g_installing.clear(std::memory_order_release);
    };

    if (installed() != nullptr) {
        finish_failed_install();
        return InstallStatus::already_installed;
    }

    const renderer_u32 source_address = pointer_word(source_words);
    const renderer_u32 current_pointer = static_cast<renderer_u32>(
        InterlockedCompareExchange(
            reinterpret_cast<volatile LONG *>(host_pointer_cell), 0, 0));
    if (current_pointer != source_address) {
        finish_failed_install();
        return InstallStatus::source_pointer_mismatch;
    }

    if (source_words[0] != ENGINE_TABLE_SIGNATURE) {
        finish_failed_install();
        return InstallStatus::bad_signature;
    }

    const ChainInitFunction original_chain_init = word_function<
        ChainInitFunction>(source_words[HERCULES_SLOT_CHAIN_INIT]);
    const SubmitPhysicalFunction original_submit = word_function<
        SubmitPhysicalFunction>(source_words[HERCULES_SLOT_SUBMIT]);
    if (original_chain_init == nullptr || original_submit == nullptr) {
        finish_failed_install();
        return InstallStatus::null_callback;
    }

    const renderer_u32 replace_slots[2] = {
        HERCULES_SLOT_CHAIN_INIT, HERCULES_SLOT_SUBMIT
    };
    const renderer_u32 replacement_values[2] = {
        function_word(&chain_init_wrapper), function_word(&submit_wrapper)
    };
    const int clone_status = engine_table_clone_selected(
        source_words, g_table_clone, replace_slots, replacement_values, 2);
    if (clone_status != ENGINE_TABLE_CLONE_OK) {
        finish_failed_install();
        return InstallStatus::clone_rejected;
    }

    g_installation.chain_init = original_chain_init;
    g_installation.submit = original_submit;
    g_installation.observer = observer;
    g_installation.observer_context = observer_context;
    g_installed.store(&g_installation, std::memory_order_release);

    /* The stable clone is complete before publication, and the compare side
     * refuses to overwrite a host update made during cloning. */
    const LONG observed_pointer = InterlockedCompareExchange(
        reinterpret_cast<volatile LONG *>(host_pointer_cell),
        static_cast<LONG>(pointer_word(g_table_clone)),
        static_cast<LONG>(source_address));
    if (static_cast<renderer_u32>(observed_pointer) != source_address) {
        g_installed.store(nullptr, std::memory_order_release);
        g_installing.clear(std::memory_order_release);
        return InstallStatus::source_pointer_mismatch;
    }

    g_installing.clear(std::memory_order_release);
    return InstallStatus::installed;
}

} // namespace dropin_renderer::proxy
