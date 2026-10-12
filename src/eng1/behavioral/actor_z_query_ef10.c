/* Ordinary-C behavioral reconstruction of ENG1.DLL RVA 0x0ef10.
 * The only direct dependency is the existing bounded 0x20b50 grid query.
 */
#include "interpreters.h"
#include "grid_query_20b50.h"

typedef char eng1_ef10_pointer_is_32bit[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_ef10_ulong_is_32bit[(sizeof(unsigned long) == 4) ? 1 : -1];

void __cdecl eng1_native_code_target_000ef10(Actor *actor)
{
    volatile unsigned char *bytes;
    volatile unsigned long *render_flags;
    long z;

    bytes = (volatile unsigned char *)actor;
    render_flags = (volatile unsigned long *)(bytes + 0x54);
    z = *(volatile long *)(bytes + 8);

    if (z < -0x100000L) {
        unsigned char state;
        unsigned long flags;

        state = bytes[0x25];
        flags = *render_flags;
        bytes[0x25] = (unsigned char)(state | 2U);
        *render_flags = flags & ~8UL;
        return;
    }

    {
        long y;
        unsigned short step;
        long z_tile;
        long x;
        long y_tile;
        long x_tile;
        unsigned short query_result;
        unsigned char state;

        y = *(volatile long *)(bytes + 4);
        step = *(volatile unsigned short *)(bytes + 0x38);
        z_tile = z >> 16;
        x = *(volatile long *)bytes;
        y_tile = y >> 16;
        x_tile = x >> 16;

        query_result = (unsigned short)grid_scan_20b50(
            x_tile, y_tile, z_tile, (unsigned long)step);

        if (query_result == 0x7fffU) {
            unsigned long flags;

            state = bytes[0x25];
            flags = *render_flags;
            bytes[0x25] = (unsigned char)(state | 2U);
            *render_flags = flags & ~8UL;
            return;
        }

        state = bytes[0x25];
        bytes[0x25] = (unsigned char)(state & 0xfdU);
        if (query_result > 0x100U) {
            unsigned long flags;

            flags = *render_flags;
            *render_flags = flags & ~8UL;
            return;
        }

        *(volatile unsigned short *)(bytes + 0x28) = query_result;
        *render_flags = *render_flags | 8UL;
    }
}
