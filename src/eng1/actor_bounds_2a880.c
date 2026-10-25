/* Ordinary-C reconstruction of ENG1.DLL RVA 0x2a880.
 * The extent producer is reconstructed separately with measured data limits. */
#include "actor_bounds_2a880.h"

typedef unsigned char bounds_u8;
typedef unsigned short bounds_u16;
typedef unsigned long bounds_u32;
typedef char bounds_requires_32bit_pointer[(sizeof(void *) == 4) ? 1 : -1];
typedef char bounds_requires_32bit_ulong[(sizeof(bounds_u32) == 4) ? 1 : -1];
typedef char bounds_record_is_12_bytes[(sizeof(Query2a170Bounds) == 12) ? 1 : -1];

#include "extent_producer_30e40.h"

static bounds_u16 bounds_low_word(bounds_u32 value)
{
    return (bounds_u16)value;
}

int __cdecl actor_bounds_2a880(
    void *actor, Query2a170Bounds *bounds, bounds_u32 mode)
{
    bounds_u8 *actor_bytes = (bounds_u8 *)actor;
    bounds_u8 extent_scratch[16];
    bounds_u16 *extent_words = (bounds_u16 *)(void *)extent_scratch;
    volatile bounds_u16 *out_words =
        (volatile bounds_u16 *)(void *)bounds;
    bounds_u32 x_fixed;
    bounds_u32 y_fixed;
    bounds_u32 z_fixed;
    bounds_u16 x_position;
    bounds_u16 y_position;
    bounds_u16 z_position;
    bounds_u8 actor_flags;
    bounds_u16 x_near;
    bounds_u16 y_near;
    bounds_u16 z_near;
    bounds_u16 x_far;
    bounds_u16 y_far;
    bounds_u16 z_far;
    bounds_u16 x_minimum;
    bounds_u16 x_maximum;
    bounds_u16 y_minimum;
    bounds_u16 y_maximum;
    bounds_u16 z_minimum;
    bounds_u16 z_maximum;
    bounds_u32 masked_mode;

    masked_mode = mode & 0xffffUL;
    if (extent_producer_30e40(actor, extent_scratch, masked_mode) == 0)
        return 0;

    /* Native 0x2a8b3..0x2a8ba ORs the low words at scratch +6 and +8 and
     * exits before reading actor coordinates or touching the caller's output
     * when both are zero. */
    if ((extent_words[3] | extent_words[4]) == 0)
        return 0;

    /* Snapshot all coordinate fields and the initial x-direction flag before
     * the first output store, as the native body does. The y-direction flag is
     * reloaded after the x stores below. Volatile loads preserve read order. */
    x_fixed = *(volatile bounds_u32 *)(void *)(actor_bytes + 0x00);
    y_fixed = *(volatile bounds_u32 *)(void *)(actor_bytes + 0x04);
    z_fixed = *(volatile bounds_u32 *)(void *)(actor_bytes + 0x08);
    actor_flags = *(volatile bounds_u8 *)(void *)(actor_bytes + 0x54);
    x_position = (bounds_u16)(x_fixed >> 16);
    y_position = (bounds_u16)(y_fixed >> 16);
    z_position = (bounds_u16)(z_fixed >> 16);

    /* Each native packed-DWORD add/sub ends in WORD stores. Only the low-word
     * arithmetic is observable in this 12-byte output view. */
    x_near = extent_words[0];
    y_near = extent_words[1];
    z_near = extent_words[2];
    x_far = extent_words[3];
    y_far = extent_words[4];
    z_far = extent_words[5];

    if ((actor_flags & 0x10U) != 0) {
        x_maximum = bounds_low_word((bounds_u32)x_position - x_near);
        x_minimum = bounds_low_word((bounds_u32)x_position - x_near - x_far);
        out_words[3] = x_maximum;
        out_words[0] = x_minimum;
    } else {
        x_minimum = bounds_low_word((bounds_u32)x_position + x_near);
        x_maximum = bounds_low_word((bounds_u32)x_position + x_near + x_far);
        out_words[0] = x_minimum;
        out_words[3] = x_maximum;
    }

    /* Native 0x2a8fc reloads actor+0x54 after the x stores. The caller's
     * bounds pointer can alias the actor, so the x-max WORD store may change
     * the y-direction flag before this branch. */
    actor_flags = *(volatile bounds_u8 *)(void *)(actor_bytes + 0x54);
    if ((actor_flags & 0x20U) != 0) {
        y_maximum = bounds_low_word((bounds_u32)y_position - y_near);
        y_minimum = bounds_low_word((bounds_u32)y_position - y_near - y_far);
        out_words[4] = y_maximum;
        out_words[1] = y_minimum;
    } else {
        y_minimum = bounds_low_word((bounds_u32)y_position + y_near);
        y_maximum = bounds_low_word((bounds_u32)y_position + y_near + y_far);
        out_words[1] = y_minimum;
        out_words[4] = y_maximum;
    }

    z_minimum = bounds_low_word((bounds_u32)z_position + z_near);
    z_maximum = bounds_low_word((bounds_u32)z_position + z_near + z_far);
    out_words[2] = z_minimum;
    out_words[5] = z_maximum;
    return 1;
}
