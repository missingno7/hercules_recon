#ifndef DROPIN_RENDERER_PROXY_TEST_SUPPORT_HPP
#define DROPIN_RENDERER_PROXY_TEST_SUPPORT_HPP

#if !defined(DROPIN_PROXY_TESTING)
#error "This backend injection API is available only in synthetic test builds."
#endif

#include "dropin_renderer/ddraw_proxy.hpp"

namespace dropin_renderer::proxy::testing {

/* One-time backend injection for a synthetic process. The production DLL is
 * compiled without this function or its storage. */
bool set_directdraw_backend( HRESULT (WINAPI *backend)(
    GUID *, IDirectDraw **, IUnknown *)) noexcept;

} // namespace dropin_renderer::proxy::testing

#endif
