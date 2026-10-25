/* ENG1 native entry at RVA 0x2c180. Ordinary-C behavioral reconstruction.
 * The return value is not part of the observed host contract. */
#include "../shared/engine_interface.h"

extern void __cdecl engine_pool_save_arena(void *, unsigned long);
extern void __cdecl engine_host_update(void);
extern void __cdecl engine_dispatch(void);

/* Native initialized DWORD at RVA 0x5b648; ENG1-local ownership. */
unsigned long g_eng1_entry_first_use_5b648 = 1;

void __cdecl PC_DLLEngineMain(const EngineInterface *source,
                             unsigned long flags)
{
    unsigned long index;
    unsigned long *destination;
    const unsigned long *words;

    if (g_eng1_entry_first_use_5b648 != 0) {
        if (source == 0)
            return;
        destination = (unsigned long *)&g_engine_interface;
        words = (const unsigned long *)source;
        for (index = 0; index != 250; ++index)
            destination[index] = words[index];
        if (g_engine_interface.signature_000 != 0x5ac00cacUL)
            return;
        /* Native captures source[0] once after the forward copy. Its 250
         * identical tests need only one semantic test of that value. */
        if (words[0] == 0)
            return;
        engine_pool_save_arena(g_engine_interface.arena_008,
                               g_engine_interface.arena_bytes_00c);
        g_eng1_entry_first_use_5b648 = 0;
    }
    engine_host_update();
    g_engine_interface.context_004->unknown_040 = flags & 1UL;
    engine_dispatch();
}
