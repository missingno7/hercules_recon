#include "../shared/engine_interface.h"
extern EngineInterface g_host_engine_interface;
extern unsigned long g_resource_prepare_scalar;
extern void __cdecl empty_host_callback(void);

void __cdecl host_file_prepare(void)
{
    empty_host_callback();
    g_host_engine_interface.context_004->prepare_state_1b00 = 0;
    g_resource_prepare_scalar = 0;
}
