#ifndef TESTS_MOCK_DDRAW_BACKEND_HPP
#define TESTS_MOCK_DDRAW_BACKEND_HPP

#include "dropin_renderer/ddraw_proxy.hpp"

namespace mock_ddraw {

enum class Mode {
    success,
    failure,
    null_arguments
};

struct BackendState {
    Mode mode;
    GUID *seen_guid;
    IDirectDraw **seen_output;
    IUnknown *seen_outer;
    LONG call_count;
    LONG add_ref_count;
    LONG release_count;
    LONG references;
    DWORD seen_last_error;
    DWORD forwarded_last_error;
    HRESULT failure_result;
};

void reset() noexcept;
BackendState &state() noexcept;
HRESULT WINAPI direct_draw_create(
    GUID *guid, IDirectDraw **out_object, IUnknown *outer);

} // namespace mock_ddraw

#endif
