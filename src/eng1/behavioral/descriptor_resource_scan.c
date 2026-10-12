/* Native-evidenced behavioral reconstruction for ENG1 RVA 0x3910.
 * Descriptor streams are byte sets terminated by 0xff; every byte is tested
 * against the current row index. The shared row table, cancellation,
 * destruction, and cleanup providers keep their existing owners. */
#include "../../../calibration/resource_record.h"
#include "initial_read_guards.h"

extern ActorResource g_actor_resources[81];
extern void __cdecl resource_cancel(int resource_id);
extern void __cdecl dispatch_local_034d0(void);

static int list_contains_resource(const unsigned char *list,
                                  unsigned long resource_index,
                                  const char *site)
{
    unsigned char value;
    unsigned long byte_index;

    byte_index = 0;
    for (;;) {
        eng1_guard_initial_read(list, byte_index, 1UL, 1UL, site);
        value = list[byte_index];
        if (value == 0xffU)
            return 0;
        if ((unsigned long)value == resource_index)
            return 1;
        ++byte_index;
    }
}

/* Two pointer-shaped cdecl arguments; the native caller ignores EAX. */
void __cdecl eng1_target_03910(const unsigned char *first_list,
                                         const unsigned char *second_list)
{
    unsigned long resource_index;

    for (resource_index = 0; resource_index < 81UL; ++resource_index) {
        if (g_actor_resources[resource_index].residency_state == 0)
            continue;

        if (first_list != 0) {
            if (list_contains_resource(
                    first_list, resource_index,
                    "ENG1 0x3910 first BYTE-FF descriptor list"))
                continue;
        }

        if (second_list != 0) {
            if (list_contains_resource(
                    second_list, resource_index,
                    "ENG1 0x3910 second BYTE-FF descriptor list"))
                continue;
        }

        resource_cancel((int)resource_index);
    }

    dispatch_local_034d0();
}
