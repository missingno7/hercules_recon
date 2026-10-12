#include <stdio.h>
#include "incomplete.h"
#include "spatial_read_guard.h"
#include "initial_spatial_roots.h"
typedef struct Eng1SpatialReadLimit {
    const void *base;
    unsigned long original_rva, bytes;
} Eng1SpatialReadLimit;
static const Eng1SpatialReadLimit limits[] = {
    { g_eng1_spatial_types_068170, 0x68170UL, 970UL },
    { g_eng1_spatial_types_04ede8, 0x4ede8UL, 1010UL },
    { g_eng1_spatial_types_061130, 0x61130UL, 1480UL },
    { g_eng1_spatial_types_0587a0, 0x587a0UL, 130UL },
    { g_eng1_spatial_types_06fe60, 0x6fe60UL, 70UL },
    { g_eng1_spatial_types_062bf8, 0x62bf8UL, 820UL },
    { g_eng1_collision_map_04f1e0, 0x4f1e0UL, 64UL },
    { g_eng1_collision_map_058830, 0x58830UL, 48UL },
    { g_eng1_collision_map_059fd8, 0x59fd8UL, 64UL },
    { g_eng1_collision_map_0616f8, 0x616f8UL, 64UL },
    { g_eng1_collision_map_0630a8, 0x630a8UL, 60UL },
    { g_eng1_collision_map_068590, 0x68590UL, 72UL },
    { g_eng1_collision_map_06b118, 0x6b118UL, 64UL },
    { &g_eng1_spatial_identity_06ff90, 0x6ff90UL, 0UL },
    { &g_eng1_spatial_identity_0702a8, 0x702a8UL, 0UL },
};
void eng1_guard_spatial_read(const void *base, unsigned long index,
                             unsigned long stride, unsigned long width,
                             const char *site)
{
    unsigned int i;
    char reason[224];
    for (i=0; i<sizeof(limits)/sizeof(limits[0]); ++i) {
        if (base != limits[i].base) continue;
        if (stride == 0 || width == 0 || width > stride ||
            width > limits[i].bytes ||
            index > (limits[i].bytes-width)/stride) {
            sprintf(reason, "%s spatial index %lu stride %lu width %lu capacity %lu",
                    site, index, stride, width, limits[i].bytes);
            eng1_incomplete_behavior(limits[i].original_rva, reason);
        }
        return;
    }
}
