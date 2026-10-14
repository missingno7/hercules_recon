#ifndef DROPIN_RENDERER_DDRAW_PROXY_HPP
#define DROPIN_RENDERER_DDRAW_PROXY_HPP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <unknwn.h>

struct IDirectDraw;

/* This is the sole DirectDraw entry point imported by the pinned Hercules
 * executable. Keep the native WINAPI argument layout exactly intact. */
extern "C" HRESULT WINAPI DirectDrawCreate(
    GUID *guid, IDirectDraw **out_object, IUnknown *outer);

#endif
