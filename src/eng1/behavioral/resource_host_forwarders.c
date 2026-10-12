/* Private ordinary-C forwarding models for three ENG1 copied-interface calls.
 * The EXE callback owns the resource behavior; these wrappers preserve its raw
 * 32-bit EAX value and forward the native cdecl stack words in order.
 */
#include "../../../calibration/engine_interface.h"

typedef unsigned long (__cdecl *Eng1HostTwoPointers)(
    const char *prefix, const char *suffix);
typedef unsigned long (__cdecl *Eng1HostOneDword)(unsigned long row_word_000);
typedef unsigned long (__cdecl *Eng1HostPointerAndDword)(
    void *row_buffer, unsigned long load_mode);

/* ENG1 0x2c400 -> copied interface +0x80 (slot 32), EXE callback RVA 0x3c50.
 * The native caller at 0x1fadf passes the selected prefix, then suffix. */
unsigned long __cdecl eng1_target_02c400(const char *prefix,
                                          const char *suffix)
{
    return ((Eng1HostTwoPointers)g_engine_interface.opaque_07c[1])(
        prefix, suffix);
}

/* ENG1 0x2c4f0 -> copied interface +0xc0 (slot 48), EXE callback RVA 0x3ce0.
 * The native caller at 0x1fae8 passes one physical DWORD. */
unsigned long __cdecl eng1_target_02c4f0(unsigned long row_word_000)
{
    return ((Eng1HostOneDword)g_engine_interface.opaque_0bc[1])(
        row_word_000);
}

/* ENG1 0x2c480 -> copied interface +0x9c (slot 39), EXE callback RVA 0x3d00.
 * The native caller at 0x1fb07 pushes load_mode, then row_buffer; the wrapper
 * reloads and pushes them in that same cdecl formal order. */
unsigned long __cdecl eng1_target_02c480(void *row_buffer,
                                          unsigned long load_mode)
{
    return ((Eng1HostPointerAndDword)g_engine_interface.opaque_090[3])(
        row_buffer, load_mode);
}
