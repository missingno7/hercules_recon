/* Private ordinary-C model of ENG1:0x2cb70 copied-interface bridge. */
#include "../../../calibration/engine_interface.h"

typedef unsigned long (__cdecl *Eng1StopChannel)(unsigned long channel);

/* The caller proves the low WORD channel index; upper input bits are not
 * recovered. The wrapper forwards the full physical DWORD to slot 225. */
unsigned long __cdecl dispatch_engine_2cb70(unsigned long channel)
{
    return ((Eng1StopChannel)g_engine_interface.opaque_334[20])(channel);
}
