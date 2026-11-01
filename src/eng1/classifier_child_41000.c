/* ENG1 0x41000: apply a classifier row to a child actor. Reads row columns 3
 * then 2 in native order; the row index is not bounds-checked natively. */
#include "interpreters.h"
#include "actor_control_update.h"
#include "read_guard.h"

void __cdecl eng1_native_code_target_0041000(Actor *child,
                                             unsigned long category)
{
    unsigned short *child_frame;
    unsigned short replacement_y_high;
    unsigned short table_frame;
    unsigned short frame;

    child_frame = (unsigned short *)(void *)
        ((unsigned char *)(void *)child + 0x2a);

    replacement_y_high = g_eng1_classifier_rows_65678[category][3];
    if (replacement_y_high != 0U)
        child->y = (long)((unsigned long)replacement_y_high << 16);

    table_frame = g_eng1_classifier_rows_65678[category][2];
    *child_frame = table_frame;
    if (table_frame == 0U)
        *child_frame = 1U;
    if (*child_frame >= 8U)
        *child_frame = 0x10U;

    frame = *child_frame;
    if (frame == 0x10U) {
        select_frame(child, 0x0e);
        return;
    }

    switch (frame & 3U) {
    case 0U:
    case 1U:
        select_frame(child, 0);
        break;
    case 2U:
        select_frame(child, 1);
        break;
    case 3U:
        select_frame(child, 2);
        break;
    default:
        break;
    }
}
