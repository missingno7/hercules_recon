#include "../../calibration/engine_interface.h"
#include "../../calibration/host_callback_protocol_externs.h"

extern EngineInterface g_host_engine_interface;
/* Observed callsite view: one ignored DWORD; common original leaf prototype
 * is unproved. The separately compiled exact leaf source remains unchanged. */
extern int __cdecl host_zero_callback(unsigned long);
extern void __cdecl empty_host_callback(void);
extern void __cdecl host_free(void *);
extern void __cdecl bulk_free(unsigned long);
extern void __cdecl host_engine_select(const char *, const char *);

int __cdecl host_service_4040(const char *prefix, const char *suffix)
{
    (void)host_zero_callback(0);
    if (g_host_engine_interface.context_004->unknown_0e4 != 0)
        host_free(g_host_engine_interface.context_004->unknown_0e4);
    bulk_free(0);
    g_host_engine_interface.context_004->load_complete = 0;
    g_host_engine_interface.context_004->load_failed = 0;
    empty_host_callback();
    (void)host_set_callback_one(0);
    *(unsigned long *)(g_host_engine_interface.context_004->unknown_0e8 + 0x20) = 0x801ffff0UL;
    *(unsigned long *)(g_host_engine_interface.context_004->unknown_0e8 + 0x24) = 0;
    host_engine_select(prefix, suffix);
    return 0;
}
