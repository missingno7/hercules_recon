/* Ordinary-C reconstruction of ENG1.DLL 0x30010.
 * This behavioral source makes no FUNCTION_MATCH claim. */
#include "initial_spatial_roots.h"
#include "spatial_read_guard.h"
#include "incomplete.h"

typedef unsigned short query_u16;
typedef unsigned long query_u32;
typedef unsigned char query_u8;
typedef char query_requires_32bit_pointer[(sizeof(void *) == 4) ? 1 : -1];
typedef char query_requires_32bit_ulong[(sizeof(query_u32) == 4) ? 1 : -1];

extern query_u32 g_configuration_dword_713bc;

/* Missing index query remains an ordinary external dependency. */
extern query_u32 __cdecl eng1_native_code_target_002a170(query_u32, query_u32, query_u32, query_u32);

static int query_30010_known_collision_map(const query_u16 *map)
{
    return map == g_eng1_collision_map_04f1e0 ||
           map == g_eng1_collision_map_058830 ||
           map == g_eng1_collision_map_059fd8 ||
           map == g_eng1_collision_map_0616f8 ||
           map == g_eng1_collision_map_0630a8 ||
           map == g_eng1_collision_map_068590 ||
           map == g_eng1_collision_map_06b118;
}

/* The data owner at 0x713bc is a mutable pointer cell. Native code loads it
 * only after the 0x2a170 query call, so keep that load live and in that order. */
query_u32 __cdecl dispatch_local_30010(
    query_u32 x, query_u32 y, query_u32 z, query_u32 mask)
{
    query_u32 index;
    query_u32 table_address;
    const query_u16 *table;
    query_u16 tile_flags;

    index = eng1_native_code_target_002a170(x, y, z, 0UL);
    table_address = *(volatile query_u32 *)(void *)&g_configuration_dword_713bc;
    table = (const query_u16 *)(unsigned long)table_address;

    /* Only the measured collision-map prefixes have source owners. */
    if (!query_30010_known_collision_map(table))
        eng1_incomplete_behavior(0x713bcUL,
            "0x30010 collision-map pointer has no measured source owner; abort before read");

    eng1_guard_spatial_read(table, index, 2UL, 2UL, "0x30010 collision-map WORD");
    tile_flags = *(const query_u16 *)(const void *)
        ((const query_u8 *)(const void *)table + index * 2UL);
    if ((tile_flags & 0x8000U) != 0)
        return 0UL;

    /* Native MOV AX,[table+index*2] replaces the low WORD of the query result
     * before AND EAX,ffff. The returned bits come from the table WORD. */
    return (query_u32)tile_flags & (mask & 0xffffUL);
}
