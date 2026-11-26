/* ENG1 startup copied-interface forwarders.
 * These wrappers preserve the host callback's EAX result and forward every
 * physical cdecl stack word unchanged. The EXE callbacks remain the owners of
 * their behavior and state.
 */
#include "../shared/engine_interface.h"

typedef unsigned long (__cdecl *Eng1HostTwoDwords)(
    unsigned long first, unsigned long second);
typedef unsigned long (__cdecl *Eng1HostPointerThreeDwords)(
    void *first, unsigned long second, unsigned long third,
    unsigned long fourth);

/* ENG1 0x2c2a0 calls copied cell +0x38 (slot 14), initialized to EXE 0x48a0.
 * Native caller 0x1f6f0 pushes allocation bytes, then zero: formals are
 * (zero, allocation_bytes). Both arguments are physical DWORDs.
 */
unsigned long __cdecl eng1_target_02c2a0(unsigned long first,
                                          unsigned long second)
{
    return ((Eng1HostTwoDwords)g_engine_interface.opaque_038[0])(
        first, second);
}

/* ENG1 0x2c4a0 calls copied cell +0xa0 (slot 40), initialized to EXE 0x4690.
 * The physical view is (pointer, DWORD, DWORD, DWORD); native caller 0x1f750
 * forwards (resource_root, result, zero, zero) in that order.
 */
unsigned long __cdecl eng1_target_02c4a0(void *first,
                                         unsigned long second,
                                         unsigned long third,
                                         unsigned long fourth)
{
    return ((Eng1HostPointerThreeDwords)g_engine_interface.opaque_090[4])(
        first, second, third, fourth);
}
