/* PC-backed callback hypotheses; source names and complete context extent unknown. */
#include <stddef.h>
#include "resource_context.h"
typedef short ActorReference[2];
typedef struct ActorResource {
    void *path;
    unsigned short entry_count, unknown_006;
    unsigned long file_length;
    ActorReference *references;
    unsigned short residency_state, unknown_012;
} ActorResource;
typedef char resource_record_is_20[sizeof(ActorResource)==20 ? 1:-1];
typedef char callbacks_at_b0[offsetof(ResourceCallbackContext,load_complete)==0xb0 ? 1:-1];
extern ActorResource g_actor_resources[81];
extern int g_loading_resource;
extern ResourceCallbackContext *g_resource_context;
extern void __cdecl host_cancel_load(void);
extern void __cdecl resource_destroy(int resource_id);
void __cdecl resource_reset(int resource_id);

void __cdecl resource_reset(int resource_id)
{
    if (g_actor_resources[resource_id].residency_state == 0)
        g_actor_resources[resource_id].residency_state = 1;
}

void __cdecl resource_load_complete(void)
{
    if (g_actor_resources[g_loading_resource].residency_state == 2)
        g_actor_resources[g_loading_resource].residency_state = 3;
}

void __cdecl resource_cancel(int resource_id)
{
    switch (g_actor_resources[resource_id].residency_state) {
    case 2:
        g_resource_context->load_complete = 0;
        g_resource_context->load_failed = 0;
        host_cancel_load();
    case 1:
    case 3:
        g_actor_resources[resource_id].residency_state = 4;
        break;
    }
}

void __cdecl resource_load_failed(void)
{
    resource_cancel(g_loading_resource);
    if (g_actor_resources[g_loading_resource].residency_state == 4) {
        resource_destroy(g_loading_resource);
        resource_reset(g_loading_resource);
    }
}
