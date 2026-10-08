/* Private experiment: bridge dependencies plus resource-pool reclaim. */
#include "engine_interface.h"
extern unsigned long __cdecl engine_pool_compact(int);

extern unsigned char *g_resource_root;
extern void __cdecl resource_rebuild_blob(void);
extern void __cdecl resource_rebuild_group_rows(void);

void __cdecl engine_pool_save_arena(void *arena, unsigned long bytes)
{
    g_engine_interface.save_arena_050(arena, bytes);
}

void __cdecl engine_host_update(void)
{
    g_engine_interface.update_host_258();
}

void __cdecl resource_reclaim(int wait_mode)
{
    if (wait_mode != 0) {
        while (engine_pool_compact(0) == 0) {
        }
    } else {
        engine_pool_compact(0);
    }

    if (g_resource_root != (void *)0) {
        resource_rebuild_blob();
        resource_rebuild_group_rows();
    }
}
