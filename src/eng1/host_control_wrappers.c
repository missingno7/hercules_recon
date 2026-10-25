/* Private ENG1 copied-interface control/loader/RNG forwarding subset.
 * Function shapes follow wrapper bytes, direct call sites, the current engine
 * prototypes, and initialized copied-table slots. No independent dispatch table
 * or host behavior is introduced here.
 */
#include "../shared/engine_interface.h"

typedef void (__cdecl *Eng1HostVoid0)(void);
typedef unsigned long (__cdecl *Eng1HostPointerDwordResult)(
    void *first, unsigned long second);
typedef void (__cdecl *Eng1HostPointerDword)(
    void *first, unsigned long second);
typedef unsigned int (__cdecl *Eng1HostRandomLimit)(int limit);

/* ENG1:2c200 -> copied offset 0x18/slot 6, host EXE RVA 0xbe30. */
void __cdecl dispatch_engine_2c200(void)
{
    ((Eng1HostVoid0)g_engine_interface.opaque_010[2])();
}

/* ENG1:2c210 -> copied offset 0x1c/slot 7, host EXE RVA 0xa340.
 * The physical 32-bit EAX result is consumed by one observed caller, so retain
 * it even though the current engine-control declaration ignores it. */
unsigned long __cdecl dispatch_engine_2c210(void *first,
                                             unsigned long second)
{
    return ((Eng1HostPointerDwordResult)g_engine_interface.opaque_010[3])(
        first, second);
}

/* ENG1:2c370 -> copied offset 0x64/slot 25, host EXE RVA 0x4040. */
void __cdecl dispatch_engine_2c370(void *first, unsigned long second)
{
    ((Eng1HostPointerDword)g_engine_interface.opaque_064[0])(first, second);
}

/* ENG1:2c430 -> copied offset 0x88/slot 34, host EXE RVA 0xbe30. */
void __cdecl dispatch_engine_2c430(void)
{
    ((Eng1HostVoid0)g_engine_interface.opaque_088[0])();
}

/* ENG1:2c580 -> copied offset 0xe4/slot 57, host EXE RVA 0x1610. */
void __cdecl dispatch_engine_2c580(void)
{
    ((Eng1HostVoid0)g_engine_interface.opaque_0e4[0])();
}

/* ENG1:2c460 -> copied offset 0x94/slot 37, host EXE RVA 0x4260.
 * Callers consume the full 32-bit result; the original host body performs an
 * unsigned remainder with the argument's 32-bit bit pattern as divisor. */
unsigned int __cdecl dispatch_engine_2c460(int limit)
{
    return ((Eng1HostRandomLimit)g_engine_interface.opaque_090[1])(limit);
}

/* ENG1:2c470 jumps through copied offset0x98/slot38 to EXE:4280.
 * The original host owns RNG state and consumption; this bridge adds none. */
unsigned int __cdecl dispatch_engine_2c470(void)
{
    typedef unsigned int (__cdecl *RawRandom)(void);
    return ((RawRandom)g_engine_interface.opaque_090[2])();
}
