#ifndef HERCULES_ENG1_GROUP_LAYOUTS_H
#define HERCULES_ENG1_GROUP_LAYOUTS_H
#include <stddef.h>
/* Observed contiguous resource prefix at ENG1 0x70e4c. Runtime counts
 * remain unchecked; these sixteen rows are not a universal capacity proof. */
typedef struct ResourceGroupState {
    short cumulative_by_row[16];
    short bounds_by_row[16][3];
} ResourceGroupState;
typedef char eng1_group_bounds_offset[
    (offsetof(ResourceGroupState, bounds_by_row) == 0x20) ? 1 : -1];
typedef char eng1_group_prefix_size[
    (sizeof(ResourceGroupState) == 0x80) ? 1 : -1];
extern ResourceGroupState g_resource_group_state_70e4c;
#define g_group_cumulative_70e4c (g_resource_group_state_70e4c.cumulative_by_row)
#define g_render_visible_bounds_70e6c (g_resource_group_state_70e4c.bounds_by_row)
#endif
