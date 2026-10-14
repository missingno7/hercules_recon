#include <setjmp.h>
#include <windows.h>
#include "host_engine_cells.h"

/* Address-labelled external views: source-table cell at RVA 0x365a0 and
 * status byte at RVA 0x7a01fc. Their original aggregate/source declarations
 * and semantic names remain unproved. */
extern void *g_host_engine_source_table_cell_365a0;
extern BYTE g_host_engine_call_flag_7a01fc;
extern void __cdecl host_engine_resolve(void);

typedef void (__cdecl *HostEngineMainCandidate)(void *, int);

void __cdecl host_engine_dispatch_5ec0(void)
{
    void *source_table;
    int call_flag;

    g_host_engine_selector_91c464 = 1;
    (void)setjmp(g_host_engine_jump_buffer_91bfd8);

    if (g_host_engine_gate_91c460 != 0)
        host_engine_resolve();

    if (g_host_engine_entry_91c45c != 0) {
        source_table = g_host_engine_source_table_cell_365a0;
        call_flag = g_host_engine_call_flag_7a01fc;
        ((HostEngineMainCandidate)g_host_engine_entry_91c45c)(
            source_table, call_flag);
    }

    g_host_engine_selector_91c464 = 0;
}
