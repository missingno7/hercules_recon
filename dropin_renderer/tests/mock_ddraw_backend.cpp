#include <windows.h>

#include "mock_ddraw_backend.hpp"

namespace mock_ddraw {
namespace {

struct FakeUnknownVtable {
    HRESULT (WINAPI *query_interface)(IUnknown *, REFIID, void **);
    ULONG (WINAPI *add_ref)(IUnknown *);
    ULONG (WINAPI *release)(IUnknown *);
};

struct FakeDirectDrawObject {
    const FakeUnknownVtable *vtable;
    volatile LONG references;
};

BackendState g_state = {};
FakeDirectDrawObject g_fake_object = {};
LONG g_query_calls = 0;

HRESULT WINAPI fake_query_interface(IUnknown *, REFIID, void **output)
{
    InterlockedIncrement(&g_query_calls);
    if (output != nullptr)
        *output = nullptr;
    return E_NOINTERFACE;
}

ULONG WINAPI fake_add_ref(IUnknown *unknown)
{
    auto *object = reinterpret_cast<FakeDirectDrawObject *>(unknown);
    InterlockedIncrement(&g_state.add_ref_count);
    return static_cast<ULONG>(InterlockedIncrement(&object->references));
}

ULONG WINAPI fake_release(IUnknown *unknown)
{
    auto *object = reinterpret_cast<FakeDirectDrawObject *>(unknown);
    InterlockedIncrement(&g_state.release_count);
    const LONG references = InterlockedDecrement(&object->references);
    g_state.references = references;
    return static_cast<ULONG>(references);
}

const FakeUnknownVtable g_fake_vtable = {
    fake_query_interface, fake_add_ref, fake_release
};

} // namespace

void reset() noexcept
{
    g_state.mode = Mode::success;
    g_state.seen_guid = nullptr;
    g_state.seen_output = nullptr;
    g_state.seen_outer = nullptr;
    g_state.call_count = 0;
    g_state.add_ref_count = 0;
    g_state.release_count = 0;
    g_state.references = 0;
    g_state.seen_last_error = ERROR_SUCCESS;
    g_state.forwarded_last_error = ERROR_SUCCESS;
    g_state.failure_result = E_FAIL;
    g_fake_object.vtable = &g_fake_vtable;
    g_fake_object.references = 0;
    g_query_calls = 0;
}

BackendState &state() noexcept
{
    return g_state;
}

HRESULT WINAPI direct_draw_create(
    GUID *guid, IDirectDraw **out_object, IUnknown *outer)
{
    InterlockedIncrement(&g_state.call_count);
    g_state.seen_last_error = GetLastError();
    g_state.seen_guid = guid;
    g_state.seen_output = out_object;
    g_state.seen_outer = outer;

    if (g_state.mode == Mode::null_arguments) {
        if (guid != nullptr || out_object != nullptr || outer != nullptr) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return E_INVALIDARG;
        }
        SetLastError(g_state.forwarded_last_error);
        return E_POINTER;
    }

    if (g_state.mode == Mode::failure) {
        SetLastError(g_state.forwarded_last_error);
        return g_state.failure_result;
    }

    if (guid == nullptr || out_object == nullptr) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_POINTER;
    }

    InterlockedIncrement(&g_fake_object.references);
    g_state.references = g_fake_object.references;
    *out_object = reinterpret_cast<IDirectDraw *>(&g_fake_object);
    SetLastError(g_state.forwarded_last_error);
    return S_OK;
}

} // namespace mock_ddraw
