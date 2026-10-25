/* PC-backed byte setters at 0x1fb0 and 0x1fd0. The actual host interface
 * context cell is shared with existing providers. Original declaration/TU and
 * full host table storage remain unproved. */
#include <windows.h>
#include "../shared/engine_interface.h"

extern EngineInterface g_host_engine_interface;

void __cdecl cleanup_native_1fb0(BYTE value)
{
    ((BYTE *)g_host_engine_interface.context_004)[0x0b] = value;
    ((BYTE *)g_host_engine_interface.context_004)[0x0c] = value;
}

void __cdecl cleanup_native_1fd0(BYTE value)
{
    ((BYTE *)g_host_engine_interface.context_004)[0x0e] = value;
}
