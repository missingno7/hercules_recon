#include <windows.h>

#include <cstdint>
#include <atomic>
#include <cstdio>
#include <cstring>

#include "dropin_renderer/proxy_api.hpp"
#include "dropin_renderer/proxy_test_support.hpp"
#include "mock_ddraw_backend.hpp"

namespace {

using dropin_renderer::proxy::InstallStatus;
using dropin_renderer::proxy::Observation;
using dropin_renderer::proxy::ObservationKind;
using dropin_renderer::proxy::ObservationSink;

int g_failures = 0;

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++g_failures; \
    } \
} while (0)

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

using ChainInitFunction = int (DROPIN_PROXY_CDECL *)(void *, renderer_u32);
using SubmitPhysicalFunction = renderer_u32 (DROPIN_PROXY_CDECL *)(void *);

constexpr DWORD kChainIncomingError = 0x10203040UL;
constexpr DWORD kChainReturnedError = 0x20304050UL;
constexpr DWORD kSubmitReturnedError = 0x30405060UL;
constexpr renderer_u32 kChainResult = 0x13572468UL;
constexpr renderer_u32 kSubmitResult = 0x89abcdefUL;

void *g_chain_seen_first = nullptr;
renderer_u32 g_chain_seen_count = 0;
DWORD g_chain_seen_error = 0;
std::atomic<void *> g_submit_seen_first{nullptr};
std::atomic<DWORD> g_submit_seen_error{0};
LONG g_chain_calls = 0;
LONG g_submit_calls = 0;

int DROPIN_PROXY_CDECL original_chain_init(void *first, renderer_u32 count)
{
    g_chain_seen_first = first;
    g_chain_seen_count = count;
    g_chain_seen_error = GetLastError();
    InterlockedIncrement(&g_chain_calls);
    SetLastError(kChainReturnedError);
    return static_cast<int>(kChainResult);
}

renderer_u32 DROPIN_PROXY_CDECL original_submit(void *first)
{
    g_submit_seen_first.store(first, std::memory_order_relaxed);
    g_submit_seen_error.store(GetLastError(), std::memory_order_relaxed);
    InterlockedIncrement(&g_submit_calls);
    SetLastError(kSubmitReturnedError);
    return kSubmitResult;
}

struct ObservationLog {
    Observation events[64];
    volatile LONG count;
    volatile LONG nested_once;
    volatile LONG overflow;
    renderer_u32 *host_pointer_cell;
    void *nested_argument;
    renderer_u32 nested_raw_eax;
};

void DROPIN_PROXY_CDECL observation_sink(
    const Observation *observation, void *context)
{
    auto *log = static_cast<ObservationLog *>(context);
    const LONG index = InterlockedIncrement(&log->count) - 1;
    if (index < 0 || index >= static_cast<LONG>(sizeof(log->events) /
                                                sizeof(log->events[0]))) {
        InterlockedIncrement(&log->overflow);
        return;
    }
    log->events[index] = *observation;
    SetLastError(0xdeadbeefUL);

    if (observation->kind == static_cast<renderer_u32>(
            ObservationKind::slot81_enter) &&
        InterlockedCompareExchange(&log->nested_once, 1, 0) == 0) {
        const renderer_u32 clone_address = *log->host_pointer_cell;
        auto *clone = reinterpret_cast<const renderer_u32 *>(
            static_cast<std::uintptr_t>(clone_address));
        const auto nested_submit = word_function<SubmitPhysicalFunction>(
            clone[HERCULES_SLOT_SUBMIT]);
        log->nested_raw_eax = nested_submit(log->nested_argument);
    }
}

struct ThreadCall {
    SubmitPhysicalFunction submit;
    void *argument;
    renderer_u32 result;
};

DWORD WINAPI submit_thread(void *parameter)
{
    auto *call = static_cast<ThreadCall *>(parameter);
    call->result = call->submit(call->argument);
    return 0;
}

bool test_directdraw_forwarding()
{
    using dropin_renderer::proxy::testing::set_directdraw_backend;
    mock_ddraw::reset();
    CHECK(set_directdraw_backend(&mock_ddraw::direct_draw_create));
    CHECK(!set_directdraw_backend(&mock_ddraw::direct_draw_create));

    GUID guid = { 0x12345678UL, 0x1234, 0x5678,
                  { 0x90, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67 } };
    auto *outer = reinterpret_cast<IUnknown *>(
        static_cast<std::uintptr_t>(0x12340000UL));
    IDirectDraw *created = nullptr;
    mock_ddraw::state().mode = mock_ddraw::Mode::success;
    mock_ddraw::state().forwarded_last_error = 0x55556666UL;
    SetLastError(0x11112222UL);
    const HRESULT created_result = DirectDrawCreate(&guid, &created, outer);
    CHECK(created_result == S_OK);
    CHECK(mock_ddraw::state().call_count == 1);
    CHECK(mock_ddraw::state().seen_guid == &guid);
    CHECK(mock_ddraw::state().seen_output == &created);
    CHECK(mock_ddraw::state().seen_outer == outer);
    CHECK(mock_ddraw::state().seen_last_error == 0x11112222UL);
    CHECK(GetLastError() == 0x55556666UL);
    CHECK(created != nullptr);
    CHECK(mock_ddraw::state().references == 1);
    CHECK(mock_ddraw::state().add_ref_count == 0);
    CHECK(mock_ddraw::state().release_count == 0);

    /* The caller owns the returned COM reference; the proxy leaves its
     * lifetime untouched and does not release it on return. */
    reinterpret_cast<IUnknown *>(created)->Release();
    CHECK(mock_ddraw::state().references == 0);
    CHECK(mock_ddraw::state().release_count == 1);

    const HRESULT custom_failure = static_cast<HRESULT>(0x88760001UL);
    IDirectDraw *sentinel = reinterpret_cast<IDirectDraw *>(
        static_cast<std::uintptr_t>(0x43210000UL));
    IDirectDraw *unchanged = sentinel;
    mock_ddraw::state().mode = mock_ddraw::Mode::failure;
    mock_ddraw::state().failure_result = custom_failure;
    mock_ddraw::state().forwarded_last_error = 0x77778888UL;
    SetLastError(0x33334444UL);
    const HRESULT failed_result = DirectDrawCreate(&guid, &unchanged, outer);
    CHECK(failed_result == custom_failure);
    CHECK(unchanged == sentinel);
    CHECK(mock_ddraw::state().seen_guid == &guid);
    CHECK(mock_ddraw::state().seen_output == &unchanged);
    CHECK(mock_ddraw::state().seen_outer == outer);
    CHECK(mock_ddraw::state().seen_last_error == 0x33334444UL);
    CHECK(GetLastError() == 0x77778888UL);
    CHECK(mock_ddraw::state().references == 0);
    CHECK(mock_ddraw::state().release_count == 1);

    mock_ddraw::state().mode = mock_ddraw::Mode::null_arguments;
    mock_ddraw::state().forwarded_last_error = ERROR_INVALID_HANDLE;
    SetLastError(0x9999aaaaUL);
    const HRESULT null_result = DirectDrawCreate(nullptr, nullptr, nullptr);
    CHECK(null_result == E_POINTER);
    CHECK(mock_ddraw::state().seen_guid == nullptr);
    CHECK(mock_ddraw::state().seen_output == nullptr);
    CHECK(mock_ddraw::state().seen_outer == nullptr);
    CHECK(mock_ddraw::state().seen_last_error == 0x9999aaaaUL);
    CHECK(GetLastError() == ERROR_INVALID_HANDLE);
    CHECK(mock_ddraw::state().call_count == 3);
    return true;
}

bool test_explicit_table_install_and_wrappers()
{
    using dropin_renderer::proxy::install_selected_engine_table;

    renderer_u32 source[ENGINE_TABLE_WORD_COUNT] = {};
    renderer_u32 mismatched_cell = 0x12345678UL;
    CHECK(install_selected_engine_table(nullptr, &mismatched_cell,
                                         nullptr, nullptr) ==
          InstallStatus::invalid_argument);
    CHECK(install_selected_engine_table(source, nullptr, nullptr, nullptr) ==
          InstallStatus::invalid_argument);
    CHECK(install_selected_engine_table(source, &mismatched_cell,
                                         nullptr, nullptr) ==
          InstallStatus::source_pointer_mismatch);
    CHECK(mismatched_cell == 0x12345678UL);

    source[0] = 0;
    renderer_u32 bad_signature_cell = pointer_word(source);
    CHECK(install_selected_engine_table(source, &bad_signature_cell,
                                         nullptr, nullptr) ==
          InstallStatus::bad_signature);
    CHECK(bad_signature_cell == pointer_word(source));

    source[0] = ENGINE_TABLE_SIGNATURE;
    for (renderer_u32 i = 1; i < ENGINE_TABLE_WORD_COUNT; ++i)
        source[i] = 0xa5000000UL | i;
    source[HERCULES_SLOT_CHAIN_INIT] = function_word(&original_chain_init);
    source[HERCULES_SLOT_SUBMIT] = function_word(&original_submit);
    renderer_u32 null_callback_source[ENGINE_TABLE_WORD_COUNT];
    std::memcpy(null_callback_source, source, sizeof(source));
    null_callback_source[HERCULES_SLOT_CHAIN_INIT] = 0;
    renderer_u32 null_callback_cell = pointer_word(null_callback_source);
    CHECK(install_selected_engine_table(null_callback_source,
                                         &null_callback_cell,
                                         nullptr, nullptr) ==
          InstallStatus::null_callback);
    CHECK(null_callback_cell == pointer_word(null_callback_source));

    renderer_u32 source_before[ENGINE_TABLE_WORD_COUNT];
    std::memcpy(source_before, source, sizeof(source));
    renderer_u32 host_pointer_cell = pointer_word(source);
    ObservationLog log = {};
    log.host_pointer_cell = &host_pointer_cell;
    log.nested_argument = reinterpret_cast<void *>(
        static_cast<std::uintptr_t>(0x89abc004UL));
    CHECK(install_selected_engine_table(
              source, &host_pointer_cell,
              static_cast<ObservationSink>(&observation_sink), &log) ==
          InstallStatus::installed);
    CHECK(host_pointer_cell != pointer_word(source));
    CHECK(source[HERCULES_SLOT_CHAIN_INIT] ==
          function_word(&original_chain_init));
    CHECK(source[HERCULES_SLOT_SUBMIT] == function_word(&original_submit));
    CHECK(std::memcmp(source, source_before, sizeof(source)) == 0);

    auto *clone = reinterpret_cast<const renderer_u32 *>(
        static_cast<std::uintptr_t>(host_pointer_cell));
    CHECK(clone != source);
    for (renderer_u32 i = 0; i < ENGINE_TABLE_WORD_COUNT; ++i) {
        if (i == HERCULES_SLOT_CHAIN_INIT || i == HERCULES_SLOT_SUBMIT)
            continue;
        CHECK(clone[i] == source_before[i]);
    }
    CHECK(clone[HERCULES_SLOT_CHAIN_INIT] != source[HERCULES_SLOT_CHAIN_INIT]);
    CHECK(clone[HERCULES_SLOT_SUBMIT] != source[HERCULES_SLOT_SUBMIT]);
    CHECK(install_selected_engine_table(source, &host_pointer_cell,
                                         nullptr, nullptr) ==
          InstallStatus::already_installed);

    const auto chain_init = word_function<ChainInitFunction>(
        clone[HERCULES_SLOT_CHAIN_INIT]);
    const auto submit = word_function<SubmitPhysicalFunction>(
        clone[HERCULES_SLOT_SUBMIT]);
    void *chain_argument = reinterpret_cast<void *>(
        static_cast<std::uintptr_t>(0x76543000UL));
    SetLastError(kChainIncomingError);
    const int chain_result = chain_init(chain_argument, 0x2468UL);
    CHECK(static_cast<renderer_u32>(chain_result) == kChainResult);
    CHECK(g_chain_seen_first == chain_argument);
    CHECK(g_chain_seen_count == 0x2468UL);
    CHECK(g_chain_seen_error == kChainIncomingError);
    CHECK(GetLastError() == kChainReturnedError);

    SetLastError(0x41414141UL);
    const int second_chain_result = chain_init(chain_argument, 0x2468UL);
    CHECK(static_cast<renderer_u32>(second_chain_result) == kChainResult);
    CHECK(g_chain_seen_error == 0x41414141UL);
    CHECK(GetLastError() == kChainReturnedError);

    void *submit_argument = reinterpret_cast<void *>(
        static_cast<std::uintptr_t>(0x76543210UL));
    SetLastError(0x41424344UL);
    const renderer_u32 raw_eax = submit(submit_argument);
    CHECK(raw_eax == kSubmitResult);
    CHECK(g_submit_seen_first.load(std::memory_order_relaxed) == submit_argument);
    CHECK(g_submit_seen_error.load(std::memory_order_relaxed) == 0x41424344UL);
    CHECK(GetLastError() == kSubmitReturnedError);
    CHECK(log.nested_raw_eax == kSubmitResult);

    if (log.count != 8) {
        std::fprintf(stderr, "observation count after serial calls=%ld\n",
                     static_cast<long>(log.count));
        for (LONG i = 0; i < log.count && i < 16; ++i) {
            std::fprintf(stderr,
                         "event[%ld] seq=%u tid=%u slot=%u kind=%u arg0=%08x arg1=%08x eax=%08x\n",
                         static_cast<long>(i), log.events[i].sequence,
                         log.events[i].thread_id, log.events[i].slot,
                         log.events[i].kind, log.events[i].argument0,
                         log.events[i].argument1, log.events[i].raw_eax);
        }
    }
    CHECK(log.count == 8);
    CHECK(log.overflow == 0);
    const renderer_u32 expected_kinds[8] = {
        static_cast<renderer_u32>(ObservationKind::slot81_enter),
        static_cast<renderer_u32>(ObservationKind::slot89_enter),
        static_cast<renderer_u32>(ObservationKind::slot89_return),
        static_cast<renderer_u32>(ObservationKind::slot81_return),
        static_cast<renderer_u32>(ObservationKind::slot81_enter),
        static_cast<renderer_u32>(ObservationKind::slot81_return),
        static_cast<renderer_u32>(ObservationKind::slot89_enter),
        static_cast<renderer_u32>(ObservationKind::slot89_return)
    };
    for (renderer_u32 i = 0; i < 8; ++i) {
        CHECK(log.events[i].sequence == i + 1);
        CHECK(log.events[i].kind == expected_kinds[i]);
        CHECK(log.events[i].thread_id == GetCurrentThreadId());
    }
    CHECK(log.events[0].argument0 == pointer_word(chain_argument));
    CHECK(log.events[0].argument1 == 0x2468UL);
    CHECK(log.events[1].argument0 == pointer_word(log.nested_argument));
    CHECK(log.events[2].raw_eax == kSubmitResult);
    CHECK(log.events[3].raw_eax == kChainResult);
    CHECK(log.events[4].argument0 == pointer_word(chain_argument));
    CHECK(log.events[5].raw_eax == kChainResult);
    CHECK(log.events[6].argument0 == pointer_word(submit_argument));
    CHECK(log.events[7].raw_eax == kSubmitResult);

    /* Concurrent synthetic calls retain per-call arguments and raw EAX while
     * the shared immutable installation and stable clone remain in use. */
    constexpr unsigned kThreadCount = 4;
    ThreadCall calls[kThreadCount] = {};
    HANDLE threads[kThreadCount] = {};
    for (unsigned i = 0; i < kThreadCount; ++i) {
        calls[i].submit = submit;
        calls[i].argument = reinterpret_cast<void *>(
            static_cast<std::uintptr_t>(0x90000000UL + i * 4U));
        threads[i] = CreateThread(nullptr, 0, submit_thread,
                                  &calls[i], 0, nullptr);
        CHECK(threads[i] != nullptr);
    }
    if (threads[0] != nullptr && threads[1] != nullptr &&
        threads[2] != nullptr && threads[3] != nullptr) {
        CHECK(WaitForMultipleObjects(kThreadCount, threads, TRUE,
                                     10000) == WAIT_OBJECT_0);
        for (unsigned i = 0; i < kThreadCount; ++i) {
            CHECK(calls[i].result == kSubmitResult);
            CHECK(CloseHandle(threads[i]) != 0);
        }
    } else {
        for (unsigned i = 0; i < kThreadCount; ++i) {
            if (threads[i] != nullptr) {
                WaitForSingleObject(threads[i], 10000);
                CloseHandle(threads[i]);
            }
        }
        CHECK(false);
    }
    CHECK(log.count == 16);
    CHECK(log.overflow == 0);
    for (unsigned i = 8; i < 16; ++i)
        CHECK(log.events[i].sequence == i + 1U);
    for (unsigned i = 0; i < kThreadCount; ++i) {
        bool entered = false;
        bool returned = false;
        for (unsigned j = 8; j < 16; ++j) {
            if (log.events[j].argument0 != pointer_word(calls[i].argument))
                continue;
            if (log.events[j].kind == static_cast<renderer_u32>(
                    ObservationKind::slot89_enter))
                entered = true;
            if (log.events[j].kind == static_cast<renderer_u32>(
                    ObservationKind::slot89_return)) {
                returned = true;
                CHECK(log.events[j].raw_eax == kSubmitResult);
            }
        }
        CHECK(entered);
        CHECK(returned);
    }
    CHECK(g_submit_calls == 6);
    CHECK(g_chain_calls == 2);
    return true;
}

} // namespace

int main()
{
    test_directdraw_forwarding();
    test_explicit_table_install_and_wrappers();
    if (g_failures != 0) {
        std::fprintf(stderr, "%d proxy check(s) failed\n", g_failures);
        return 1;
    }
    std::puts("proxy synthetic x86 checks passed");
    return 0;
}
