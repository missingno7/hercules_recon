/* ENG1 19a10 / 19aa0: signed velocity approach, position update and boundary
 * continuation. Arithmetic uses explicit DWORD wrap before signed comparisons.
 * The boundary services remain ordinary unresolved dependencies. */
#include <stddef.h>

typedef unsigned long AxisDword;
typedef struct AxisMotion {
    long velocity, target, rate, limit;
} AxisMotion;
typedef struct AxisActor {
    AxisDword x, y, z;
    unsigned char pad_00c[0x94 - 0x0c];
    unsigned char direction_x, direction_y;
    unsigned char pad_096[0xa8 - 0x96];
    AxisMotion motion_x, motion_y;
} AxisActor;
typedef char axis_pointer32[(sizeof(void *) == 4) ? 1 : -1];
typedef char axis_motion_x_a8[(offsetof(AxisActor, motion_x) == 0xa8) ? 1 : -1];
typedef char axis_motion_y_b8[(offsetof(AxisActor, motion_y) == 0xb8) ? 1 : -1];

extern void *g_normal_actor_pool;
extern void __cdecl eng1_19c30(void);
extern void __cdecl eng1_19d30(void);
#define AXIS_ACTOR() ((AxisActor *)(*(void * volatile *)(void *)&g_normal_actor_pool))

void __cdecl eng1_19a10(void)
{
    AxisActor *actor = AXIS_ACTOR();
    long velocity = actor->motion_x.velocity;
    long target = actor->motion_x.target;
    if (velocity != target) {
        if (velocity > target) {
            actor->motion_x.velocity = (long)((AxisDword)velocity - (AxisDword)actor->motion_x.limit);
            actor = AXIS_ACTOR();
            target = actor->motion_x.target;
            if (actor->motion_x.velocity < target) {
                actor->motion_x.velocity = target;
                actor = AXIS_ACTOR();
            }
        } else {
            actor->motion_x.velocity = (long)((AxisDword)velocity + (AxisDword)actor->motion_x.rate);
            actor = AXIS_ACTOR();
            target = actor->motion_x.target;
            if (actor->motion_x.velocity > target) {
                actor->motion_x.velocity = target;
                actor = AXIS_ACTOR();
            }
        }
    }
    actor->x += actor->direction_x
        ? 0UL - (AxisDword)actor->motion_x.velocity
        : (AxisDword)actor->motion_x.velocity;
    eng1_19c30();
}

void __cdecl eng1_19aa0(void)
{
    AxisActor *actor = AXIS_ACTOR();
    long velocity = actor->motion_y.velocity;
    long target = actor->motion_y.target;
    if (velocity != target) {
        if (velocity > target) {
            actor->motion_y.velocity = (long)((AxisDword)velocity - (AxisDword)actor->motion_y.limit);
            actor = AXIS_ACTOR();
            target = actor->motion_y.target;
            if (actor->motion_y.velocity < target) {
                actor->motion_y.velocity = target;
                actor = AXIS_ACTOR();
            }
        } else {
            actor->motion_y.velocity = (long)((AxisDword)velocity + (AxisDword)actor->motion_y.rate);
            actor = AXIS_ACTOR();
            target = actor->motion_y.target;
            if (actor->motion_y.velocity > target) {
                actor->motion_y.velocity = target;
                actor = AXIS_ACTOR();
            }
        }
    }
    if (actor->direction_y)
        actor->y -= (AxisDword)actor->motion_y.velocity;
    else
        actor->y += (AxisDword)actor->motion_y.velocity;
    eng1_19d30();
}


/* ENG1 0x19b40 chooses the first enabled query in native priority order.
 * context_004 is the existing EngineInterface prefix field at RVA 0x70f24.
 * Bit 2 returns a signed clamp to the observed lower/upper table entries.
 * The six slice bases remain an unresolved file-backed data dependency. */
#include "../shared/engine_interface.h"

/* Native-backed two-byte view hypothesis: these six single-bit assignments
 * compile to the observed byte ORs without volatile or a wider store. This is
 * not proof of the original bitfield declaration. The existing unsigned-short
 * owner is accessed through a union containing
 * that type. This candidate defines no additional storage. */
typedef union AxisUsedMask {
    unsigned short word;
    struct { unsigned short bit0:1, bit1:1, bit2:1, bit3:1,
             bit4:1, bit5:1, bit6:1, bit7:1, high:8; } bits;
} AxisUsedMask;
extern unsigned short g_native_word_72168;
#define AXIS_USED_MASK (*(AxisUsedMask *)(void *)&g_native_word_72168)
extern unsigned short g_eng1_setup_word_72012;
extern unsigned short g_eng1_setup_word_72194;

/* Native slice bases differ by 16 bytes. The flat declaration intentionally
 * makes no claim about allocation capacity or the signed difficulty domain. */
extern long g_eng1_mask_values_553e8[];

int __cdecl eng1_19b40(int mask)
{
    long byte_offset;
    int position;
    long result;

    mask &= g_eng1_setup_word_72012;
    if (mask & 2) {
        AXIS_USED_MASK.bits.bit1 = 1;
        byte_offset = (long)*(signed char *)
            ((unsigned char *)g_engine_interface.context_004 + 0x0e)
            * (long)sizeof(long);
        position = (short)g_eng1_setup_word_72194;
        result = *(long *)((unsigned char *)(g_eng1_mask_values_553e8 + 4) + byte_offset);
        if (position < result)
            goto result_ready;
        result = *(long *)((unsigned char *)(g_eng1_mask_values_553e8 + 20) + byte_offset);
        if (position > result)
            goto result_ready;
        return position;
    }
    if (mask & 1) {
        AXIS_USED_MASK.bits.bit0 = 1;
        return g_eng1_mask_values_553e8[0 + *(signed char *)
            ((unsigned char *)g_engine_interface.context_004 + 0x0e)];
    }
    if (mask & 4) {
        AXIS_USED_MASK.bits.bit2 = 1;
        return g_eng1_mask_values_553e8[4 + *(signed char *)
            ((unsigned char *)g_engine_interface.context_004 + 0x0e)];
    }
    if (mask & 8) {
        AXIS_USED_MASK.bits.bit3 = 1;
        return g_eng1_mask_values_553e8[8 + *(signed char *)
            ((unsigned char *)g_engine_interface.context_004 + 0x0e)];
    }
    if (mask & 0x10) {
        AXIS_USED_MASK.bits.bit4 = 1;
        return g_eng1_mask_values_553e8[12 + *(signed char *)
            ((unsigned char *)g_engine_interface.context_004 + 0x0e)];
    }
    if (mask & 0x40) {
        AXIS_USED_MASK.bits.bit6 = 1;
        return g_eng1_mask_values_553e8[16 + *(signed char *)
            ((unsigned char *)g_engine_interface.context_004 + 0x0e)];
    }
    result = 0;
result_ready:
    return result;
}
