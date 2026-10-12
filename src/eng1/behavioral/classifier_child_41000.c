/* Bounded ordinary-C candidate for ENG1.DLL RVA 0x41000.
 * The classifier rows and frame selector remain their existing providers. */
#include <stdio.h>
#include "actor_control_update.h"
#include "read_guard.h"
#include "incomplete.h"

typedef char eng1_41000_long_is_32[(sizeof(long) == 4) ? 1 : -1];
typedef char eng1_41000_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_41000_table_row_is_12[
    (sizeof(g_eng1_classifier_rows_65678[0]) == 12) ? 1 : -1];
typedef char eng1_41000_actor_y_at_4[(offsetof(Actor, y) == 4) ? 1 : -1];
typedef char eng1_41000_actor_frame_at_2a[
    (offsetof(Actor, unknown_02a) == 0x2a) ? 1 : -1];

/* 0x41000 reads columns 3 then 2 of the caller's 12-byte classifier row.
 * Its native access is bounded here to the existing measured 18-row owner. */
void __cdecl eng1_native_code_target_0041000(Actor *child, unsigned long category)
{
    volatile unsigned short *row;
    volatile unsigned short *child_frame;
    unsigned short replacement_y_high;
    unsigned short table_frame;
    unsigned short frame;
    char reason[144];

    if (category >= 18UL) {
        sprintf(reason,
                "0x41000 classifier row %lu is outside the measured 18-row owner at RVA 0x65678",
                category);
        eng1_incomplete_behavior(0x65678UL, reason);
    }

    row = (volatile unsigned short *)(void *)
        &g_eng1_classifier_rows_65678[category][0];
    child_frame = (volatile unsigned short *)(void *)
        ((unsigned char *)(void *)child + 0x2a);

    replacement_y_high = row[3];
    if (replacement_y_high != 0U)
        child->y = (long)((unsigned long)replacement_y_high << 16);

    table_frame = row[2];
    *child_frame = table_frame;
    if (table_frame == 0U)
        *child_frame = 1U;
    if (*child_frame >= 8U)
        *child_frame = 0x10U;

    /* Native reloads child+0x2a after its normalization stores. */
    frame = *child_frame;
    if (frame == 0x10U) {
        eng1_guarded_select_frame(child, 0x0e);
        return;
    }

    switch (frame & 3U) {
    case 0U:
    case 1U:
        eng1_guarded_select_frame(child, 0);
        break;
    case 2U:
        eng1_guarded_select_frame(child, 1);
        break;
    case 3U:
        eng1_guarded_select_frame(child, 2);
        break;
    default:
        break;
    }
}
