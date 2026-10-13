#include <windows.h>

/* A test DLL only. Three pointer parameters preserve the x86 import ABI;
   the probe does not call this entry point. */
HRESULT WINAPI DirectDrawCreate(void *guid, void **out, void *outer)
{
    (void)guid;
    (void)out;
    (void)outer;
    OutputDebugStringA("renderer_contract loader fixture: unexpected DirectDrawCreate call\n");
    return E_NOTIMPL;
}
