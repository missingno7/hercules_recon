/* Private measured forwarder for ENG1 RVA 0x2c830.
 * This wrapper uses the real copied EngineInterface table. The callback is a
 * physical cdecl DWORD-in / void-result view; EXE 0x70c0 consumes only the
 * low input byte, but this bridge forwards the full DWORD unchanged.
 */
#include "../shared/engine_interface.h"

typedef void (__cdecl *Eng1ColorCallback)(unsigned long frame_value);

/* 0x2c830 calls [ENG1+0x7115c] = table offset 0x23c, slot 143,
 * opaque_1fc[16]. The initialized host cell targets EXE RVA 0x70c0. */
void __cdecl dispatch_engine_2c830(unsigned long frame_value)
{
    ((Eng1ColorCallback)g_engine_interface.opaque_1fc[16])(frame_value);
}
