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
