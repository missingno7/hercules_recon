#include "resource_record.h"
#include "resource_context.h"

extern ActorResource g_actor_resources[81];
extern ActorResource *g_active_resource_record;
extern int g_loading_resource;
extern unsigned long g_reported_total_bytes;
extern unsigned long g_resource_start_tick;
extern ResourceCallbackContext *g_resource_context;
extern const char g_resource_suffix[];
extern const char g_provider_bad_id_format[];

extern void __cdecl engine_diagnostic(const char *format, ...);
extern unsigned long __cdecl resource_file_length(const char *path, const char *suffix);
extern ActorReference *__cdecl resource_allocate(
    ActorReference **output_slot, unsigned long bytes, unsigned long flags);
extern void __cdecl resource_reclaim(int reason);
extern int __cdecl resource_load_async(
    const char *path, const char *suffix, int sector_offset,
    void *destination, unsigned long bytes);
extern unsigned long __cdecl resource_load_sync(
    const char *path, const char *suffix, void *destination,
    unsigned long bytes);
extern void __cdecl resource_cancel(int resource_id);
extern void __cdecl resource_load_complete(void);
extern void __cdecl resource_load_failed(void);

int __cdecl resource_provider(int resource_id, int async_mode)
{
    ActorResource *resource;
    ActorResource *original_resource;
    ActorReference **output_slot;
    ActorReference *allocated;
    ResourceCallbackContext *context;
    unsigned long prefix_bytes;
    unsigned long file_length;
    unsigned long total_bytes;
    unsigned long request_bytes;
    int initialized_rows;

    if (resource_id >= 81) {
        engine_diagnostic(g_provider_bad_id_format, resource_id);
        return 1;
    }

    context = g_resource_context;
    if (context->mode_09c == 1 || context->blocked_0df != 0)
        return 0;

    original_resource = &g_actor_resources[resource_id];
    if (original_resource->residency_state == 3)
        return 0;

    g_active_resource_record = original_resource;
    prefix_bytes = ((unsigned long)original_resource->entry_count + 1UL) * 4UL;
    file_length = original_resource->file_length;
    if (file_length == 0) {
        file_length = resource_file_length((const char *)original_resource->path, g_resource_suffix);
        g_active_resource_record->file_length = file_length;
    }

    total_bytes = file_length + prefix_bytes;
    output_slot = &original_resource->references;
    request_bytes = (total_bytes & 0xfffff800UL) + 0x1000UL;
    allocated = resource_allocate(output_slot, request_bytes, 0x20UL);
    if (allocated == 0) {
        resource_reclaim(0);
        allocated = resource_allocate(output_slot, request_bytes, 0x20UL);
    }

    if (allocated == 0) {
        if (async_mode == 0)
            original_resource->residency_state = 0;
        return 1;
    }

    initialized_rows = 0;
    resource = g_active_resource_record;
    while (initialized_rows < (int)resource->entry_count + 1) {
        allocated[initialized_rows][0] = 0;
        allocated[initialized_rows][1] = 0;
        ++initialized_rows;
        resource = g_active_resource_record;
    }

    context = g_resource_context;
    original_resource->residency_state = 2;
    g_reported_total_bytes = total_bytes;
    g_loading_resource = resource_id;
    original_resource->unknown_012 = 1;
    g_resource_start_tick = context->tick_038;

    if (async_mode != 0) {
        context->load_complete = resource_load_complete;
        g_resource_context->load_failed = resource_load_failed;
        resource = g_active_resource_record;
        (void)resource_load_async(
            (const char *)resource->path, g_resource_suffix, 0,
            (unsigned char *)resource->references + prefix_bytes, file_length);
        return -1;
    }

    if (resource_load_sync(
            (const char *)resource->path, g_resource_suffix,
            (unsigned char *)resource->references + prefix_bytes, file_length) != 0) {
        resource_load_complete();
        return 0;
    }

    resource_cancel(resource_id);
    return 0;
}
