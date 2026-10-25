#include <stdlib.h>
#include <string.h>
#include "../shared/engine_interface.h"
#include "host_bootstrap_text.h"

extern EngineInterface g_host_engine_interface;
extern void __cdecl save_arena(unsigned char *, unsigned long);
extern void __cdecl host_service_10690(void);
extern int __cdecl host_service_4040(const char *, const char *);

void __cdecl host_service_1ff0(void)
{
    memset(g_host_engine_interface.context_004, 0, sizeof(ResourceCallbackContext));
    memset(g_host_engine_interface.arena_008, 0, g_host_engine_interface.arena_bytes_00c);
    save_arena(g_host_engine_interface.arena_008, g_host_engine_interface.arena_bytes_00c);
    g_host_engine_interface.context_004->unknown_000[9] = 0;
    *(unsigned long *)(g_host_engine_interface.context_004->unknown_000 + 0x30) = 0;
    host_service_10690();
    g_host_engine_interface.context_004->unknown_000[0] = 0xff;
    g_host_engine_interface.context_004->unknown_000[3] = 0x11;
    g_host_engine_interface.context_004->unknown_12c = 0;
    *(unsigned short *)g_host_engine_interface.context_004->unknown_12e = 0;
    g_host_engine_interface.context_004->unknown_048 = 0x13546547UL;
    g_host_engine_interface.context_004->unknown_04c = 0xecab9ab8UL;
    srand(0x13546547U);
    (void)host_service_4040(g_host_empty_text_3f74c, g_host_default_engine_name_cell_35100);
}
