/* Native-evidenced behavioral reconstruction of ENG1 RVA 0x33e0/0x3410.
 * The provider, resource rows, residency callbacks, and host forwarding remain
 * owned by their existing shared source files. */
#include "../shared/resource_record.h"
#include "initial_read_guards.h"

extern ActorResource g_actor_resources[81];
extern int __cdecl resource_provider(int resource_id, int async_mode);

void __cdecl eng1_target_03410(const unsigned char *resource_ids,
                               int resource_id)
{
    unsigned long byte_index;
    unsigned char value;

    if (resource_ids == 0)
        return;

    eng1_guard_unreadable_identity(
        resource_ids, "ENG1 0x3410 resource id list");
    byte_index = 0;
    for (;;) {
        eng1_guard_initial_read(
            resource_ids, byte_index, 1UL, 1UL,
            "ENG1 0x3410 resource id list BYTE scan");
        value = resource_ids[byte_index];
        if (value == 0xffU)
            return;

        if ((int)value == resource_id &&
            g_actor_resources[resource_id].references == 0) {
            (void)resource_provider((int)value, 0);
            g_actor_resources[resource_id].unknown_012 = 2;
        }

        ++byte_index;
    }
}

void __cdecl eng1_target_033e0(const unsigned char *first_list,
                               const unsigned char *second_list)
{
    int resource_id;

    for (resource_id = 0; resource_id < 81; ++resource_id) {
        eng1_target_03410(first_list, resource_id);
        eng1_target_03410(second_list, resource_id);
    }
}
