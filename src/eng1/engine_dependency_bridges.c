/* Private experiment: bridge dependencies plus resource-pool reclaim. */
#include "../shared/engine_interface.h"
extern unsigned long __cdecl title_0c310(int);

extern unsigned char *g_resource_root;
extern void __cdecl resource_rebuild_blob(void);
extern void __cdecl resource_rebuild_group_rows(void);

void __cdecl title_0c390(void *arena, unsigned long bytes)
{
    g_engine_interface.save_arena_050(arena, bytes);
}

void __cdecl title_0ca00(void)
{
    g_engine_interface.update_host_258();
}

void __cdecl resource_reclaim(int wait_mode)
{
    if (wait_mode != 0) {
        while (title_0c310(0) == 0) {
        }
    } else {
        title_0c310(0);
    }

    if (g_resource_root != (void *)0) {
        resource_rebuild_blob();
        resource_rebuild_group_rows();
    }
}
