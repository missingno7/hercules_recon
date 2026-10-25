/* ENG1 RVA 0x20ea0. Packed-row interpolation recovered from PC instructions.
 * Four cdecl stack arguments; any nonzero mode writes fourteen output bytes.
 * Keep native wraparound, unchecked signed index and live aliased reads.
 * REA supplied a structural lead, not this ABI or acceptance. */
#include "resource_runtime_data.h"

static long scaled(long delta, long fraction)
{
    return (long)((unsigned long)delta * (unsigned long)fraction) >> 16;
}

int __cdecl dispatch_local_20ea0(void *output, int id, int mode, long position)
{
    unsigned char *cell, *entry;
    unsigned short *left, *right, *out;
    unsigned long count, i, next_time;
    long fraction, denominator, lx, rx, lh, rh, ly, ry, la, ra, value;

    if (id >= (int)g_root_count_70ed3) return 0;
    if (position < 0) position = 0;
    cell = g_header_pointer_70d4c + id * 4;
    entry = cell + (*(unsigned long *)cell & ~3UL);
    count = *(unsigned short *)entry;
    if (count < 1) count = 1;
    left = right = (unsigned short *)(entry + 4);
    next_time = (unsigned long)mode;
    for (i = 1; i < count; ++i) {
        right += 8;
        next_time = right[0];
        if ((long)next_time > position) break;
        left = right;
    }
    denominator = (long)((unsigned long)left[0] - next_time);
    fraction = 0;
    if (denominator != 0)
        fraction = (long)(((unsigned long)position - left[0]) << 16) / denominator;
    if (mode != 0) {
        out = (unsigned short *)output;
        rx = right[1]; rh = right[3] >> 1;
        lx = left[1]; lh = left[3] >> 1;
        out[0] = (unsigned short)(scaled(rh - rx - lh + lx, fraction) - lh + lx);
        out[2] = (unsigned short)(scaled(lh - rx - rh + lx, fraction) + lh + lx);
        ry = right[2]; ra = right[4]; la = left[4]; ly = left[2];
        value = scaled(la - ra - ry + ly, fraction) + la + ly;
        out[1] = out[3] = (unsigned short)value;
        out[4] = (unsigned short)(scaled((long)left[5] - right[5], fraction) + left[5]);
        out[5] = (unsigned short)(scaled((long)left[6] - right[6], fraction) + left[6]);
        /* Native code reloads this lane after the preceding output write. */
        out[6] = (unsigned short)(scaled((long)left[6] - right[6], fraction) + left[6]);
    }
    return 1;
}
