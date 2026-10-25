#include "../src/shared/engine_interface.h"
EngineInterface g_engine_interface;
unsigned int __cdecl dispatch_engine_2c470(void);
typedef unsigned int (__cdecl *RawRandom)(void);
typedef union RawRandomBits { unsigned long bits; RawRandom call; } RawRandomBits;
static unsigned int calls, result;
static unsigned int __cdecl observe(void) { ++calls; return result; }
int main(void)
{
    RawRandomBits slot;
    unsigned int i;
    unsigned int values[4] = {0, 0xffff, 0x80000000U, 0xf1234567U};
    slot.call = observe;
    g_engine_interface.opaque_090[2] = slot.bits;
    for (i = 0; i != 4; ++i) {
        result = values[i];
        if (dispatch_engine_2c470() != result || calls != i + 1) return 1;
    }
    return 0;
}
