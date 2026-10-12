/* Ordinary-C reconstruction of ENG1.DLL RVA 0x2a170.
 * Direct dependencies remain explicit externals; this is not FUNCTION_MATCH. */
#include "actor_pipeline_storage_views.h"
#include "incomplete.h"
#include "query_2a170.h"

typedef unsigned char query_u8;
typedef unsigned short query_u16;
typedef unsigned long query_u32;
typedef long query_s32;
typedef char query_2a170_requires_32bit_pointer[(sizeof(void *) == 4) ? 1 : -1];
typedef char query_2a170_requires_32bit_ulong[(sizeof(query_u32) == 4) ? 1 : -1];
typedef char query_2a170_bounds_is_12_bytes[(sizeof(Query2a170Bounds) == 12) ? 1 : -1];

/* Unrecovered services are independently recorded external dependencies. */
extern int __cdecl eng1_native_code_target_002a880(void *, Query2a170Bounds *, unsigned long);
extern unsigned long __cdecl eng1_native_code_target_0020d30(long, long, long, unsigned long);
/* Native SAR of a signed 16.16 DWORD yields a sign-extended upper WORD.
 * Express that mapping without relying on C's implementation-defined signed
 * right shift. */
static query_s32 query_2a170_integer_part(query_u32 fixed)
{
    query_u32 high = fixed >> 16;
    if ((high & 0x8000UL) != 0)
        return (query_s32)high - 0x10000L;
    return (query_s32)high;
}

static query_s32 query_2a170_live_count(void)
{
    return (query_s32)*(volatile query_u32 *)(void *)g_actor_worklist_71b20;
}

query_u32 __cdecl dispatch_local_2a170(
    query_u32 x, query_u32 y, query_u32 z, query_u32 step)
{
    query_s32 initial_count;
    query_s32 live_count;
    query_u32 processed;
    query_u32 actor_word;
    query_u32 result;
    void *actor;
    Query2a170Bounds bounds;

    initial_count = query_2a170_live_count();
    g_eng1_spatial_match_actor_6fedc = 0UL;

    if (initial_count > 0) {
        processed = 0UL;
        for (;;) {
            ++processed;
            if (processed >= 120UL)
                eng1_incomplete_behavior(0x71b20UL,
                    "0x2a170 next worklist entry exceeds the measured 120-DWORD owner prefix");

            actor_word = *(volatile query_u32 *)(void *)
                (g_actor_worklist_71b20 + processed);
            actor = (void *)(unsigned long)actor_word;

            if (eng1_native_code_target_002a880(actor, &bounds, 0UL) != 0 &&
                point_in_box_2a810(x, y, z, &bounds) != 0) {
                g_eng1_spatial_match_actor_6fedc = actor_word;
                result = *(volatile query_u32 *)(void *)
                    ((query_u8 *)actor + 0xd8UL);
                return result;
            }

            /* Native 0x2a1c8 reloads the count after each rejected entry.
             * Re-read it after the providers because they may mutate state. */
            live_count = query_2a170_live_count();
            if ((query_s32)processed >= live_count)
                break;
        }
    }

    return eng1_native_code_target_0020d30(query_2a170_integer_part(x),
                       query_2a170_integer_part(y),
                       query_2a170_integer_part(z), step) & 0xffffUL;
}
