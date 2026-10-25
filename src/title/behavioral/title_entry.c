/* Behavioral draft; no FUNCTION_MATCH claim.
 * Native TITLE.DLL export PC_DLLEngineMain, RVA 0xc1f0.
 * The EXE caller at RVA 0x5f03 discards EAX and cleans both arguments.
 */
#include <string.h>
#include "../title_engine.h"
#include "title_trace.h"

typedef char title_entry_interface_is_1000[
    sizeof(TitleEngineInterface) == 1000 ? 1 : -1];

/* This is the same native base used by the movement-table reader at 0xd910.
 * Integration must provide one storage binding, not an overlapping flag cell.
 * The exact original aggregate declaration remains unknown.
 */
extern int g_257e8[];

int title_0c390(int arena, int arena_bytes);
void title_0ca00(void);
void title_05fb0(void);

void __cdecl PC_DLLEngineMain(const TitleEngineInterface *host, unsigned int flags)
{
    unsigned int i;
    unsigned long signature;

    title_trace("entry", flags, g_257e8[0], (unsigned long)host);

    if (g_257e8[0]) {
        if (!host)
            return;
        memcpy(&g_engine_interface, host, 1000);
        title_trace("interface_copied", g_engine_interface.signature_000,
                    (unsigned long)g_engine_interface.context_004,
                    (unsigned long)g_engine_interface.arena_008);
        if (g_engine_interface.signature_000 != 0x5ac00cacUL)
            return;

        /* Native c21d loads only host[0], then c221..c22b tests the same
         * cached value 250 times. It does not reject null callback slots.
         */
        signature = host->signature_000;
        for (i = 0; i < 250; ++i) {
            if (!signature)
                return;
        }
        title_trace("arena_begin", (unsigned long)g_engine_interface.arena_008,
                    g_engine_interface.arena_bytes_00c, 0);
        title_0c390((int)g_engine_interface.arena_008,
                     (int)g_engine_interface.arena_bytes_00c);
        /* Native c23f explicitly ignores the callback's EAX result. */
        g_257e8[0] = 0;
        title_trace("arena_end", 0, 0, 0);
    }

    title_trace("host_update_begin", 0, 0, 0);
    title_0ca00();
    g_engine_interface.context_004->unknown_040 = flags & 1;
    title_trace("frame_begin", flags & 1,
                (unsigned long)g_engine_interface.context_004, 0);
    title_05fb0();
    title_trace("frame_return", 0, 0, 0);
}
