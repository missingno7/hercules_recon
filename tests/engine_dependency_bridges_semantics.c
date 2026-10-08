#include <stdio.h>
#include <string.h>
#include "../calibration/engine_file_bridge.c"
#include "../calibration/engine_dependency_bridges.c"

EngineInterface g_engine_interface;
unsigned char *g_resource_root;

static unsigned long checks;
static unsigned long failures;
static unsigned long compact_calls;
static unsigned long compact_first_calls;
static unsigned long compact_second_calls;
static unsigned long header_calls;
static unsigned long rows_calls;
static unsigned long save_calls;
static unsigned long update_first_calls;
static unsigned long update_second_calls;
static void *save_arena_seen;
static unsigned long save_bytes_seen;
static void *header_root_seen;
static void *rows_root_seen;
static unsigned long rebuild_order[4];
static unsigned long rebuild_order_count;
static int compact_bad_pool;
static unsigned char old_root;
static unsigned char relocated_once;
static unsigned char relocated_loop;

static void expect(const char *label, int passed)
{
    ++checks;
    if (!passed) {
        ++failures;
        printf("FAIL: %s\n", label);
    }
}

static void __cdecl save_callback(void *arena, unsigned long bytes)
{
    ++save_calls;
    save_arena_seen = arena;
    save_bytes_seen = bytes;
}

static void __cdecl update_second(void)
{
    ++update_second_calls;
}

static void __cdecl update_first(void)
{
    ++update_first_calls;
    g_engine_interface.update_host_258 = update_second;
}

static unsigned long __cdecl compact_once_and_relocate(int pool_id)
{
    ++compact_calls;
    if (pool_id != 0) {
        compact_bad_pool = 1;
    }
    g_resource_root = &relocated_once;
    return 0;
}

static unsigned long __cdecl compact_wait_second(int pool_id);

static unsigned long __cdecl compact_wait_first(int pool_id)
{
    ++compact_calls;
    ++compact_first_calls;
    if (pool_id != 0) {
        compact_bad_pool = 1;
    }
    g_engine_interface.compact_pool_034 = compact_wait_second;
    return 0;
}

static unsigned long __cdecl compact_wait_second(int pool_id)
{
    ++compact_calls;
    ++compact_second_calls;
    if (pool_id != 0) {
        compact_bad_pool = 1;
    }
    g_resource_root = &relocated_loop;
    return 1;
}

static unsigned long __cdecl compact_null_root(int pool_id)
{
    ++compact_calls;
    if (pool_id != 0) {
        compact_bad_pool = 1;
    }
    g_resource_root = (void *)0;
    return 1;
}

void __cdecl resource_rebuild_blob(void)
{
    ++header_calls;
    header_root_seen = g_resource_root;
    rebuild_order[rebuild_order_count++] = 1;
}

void __cdecl resource_rebuild_group_rows(void)
{
    ++rows_calls;
    rows_root_seen = g_resource_root;
    rebuild_order[rebuild_order_count++] = 2;
}

int main(void)
{
    char arena[8];
    unsigned long headers_before;
    unsigned long rows_before;

    g_engine_interface.save_arena_050 = save_callback;
    engine_pool_save_arena(arena, 0x87654321UL);
    expect("slot20 forwards arena and full byte count", save_calls == 1 &&
           save_arena_seen == arena && save_bytes_seen == 0x87654321UL);

    g_engine_interface.update_host_258 = update_first;
    engine_host_update();
    engine_host_update();
    expect("slot150 callback is loaded live on each call",
           update_first_calls == 1 && update_second_calls == 1);

    g_engine_interface.compact_pool_034 = compact_once_and_relocate;
    g_resource_root = &old_root;
    resource_reclaim(0);
    expect("wait mode zero compacts once and observes relocated root",
           compact_calls == 1 && !compact_bad_pool &&
           header_calls == 1 && rows_calls == 1 &&
           header_root_seen == &relocated_once && rows_root_seen == &relocated_once);
    expect("root rebuild routines run in target order",
           rebuild_order_count == 2 && rebuild_order[0] == 1 && rebuild_order[1] == 2);

    compact_calls = 0;
    compact_first_calls = 0;
    compact_second_calls = 0;
    header_calls = 0;
    rows_calls = 0;
    rebuild_order_count = 0;
    g_engine_interface.compact_pool_034 = compact_wait_first;
    g_resource_root = &old_root;
    resource_reclaim(-3);
    expect("nonzero mode retries through the replaced live slot",
           compact_calls == 2 && compact_first_calls == 1 &&
           compact_second_calls == 1 && !compact_bad_pool);
    expect("retry path rebuilds the final relocated root in order",
           header_calls == 1 && rows_calls == 1 &&
           header_root_seen == &relocated_loop && rows_root_seen == &relocated_loop &&
           rebuild_order_count == 2 && rebuild_order[0] == 1 && rebuild_order[1] == 2);

    headers_before = header_calls;
    rows_before = rows_calls;
    g_engine_interface.compact_pool_034 = compact_null_root;
    g_resource_root = &old_root;
    resource_reclaim(0);
    expect("null live root skips both rebuild calls after compaction",
           header_calls == headers_before && rows_calls == rows_before);

    printf("engine dependency bridge fixture: %lu checks, %lu failures\n",
           checks, failures);
    return failures ? 1 : 0;
}
