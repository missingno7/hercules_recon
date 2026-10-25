/* ENG1 2b5e0. Reuse the established motion algorithm with an optional
 * bounds-centered target and the original flag40/80 stages. A null target
 * bypasses all steering, while ordinary acceleration and motion still run. */
/* Partial shared object/axis views: full, planar and vertical motion routines.
 * Reviewed module addresses and exact extents are recorded in recovery.json.
 * Clamp order is intentional: a negative limit can replace the lower clamp.
 * Names are semantic hypotheses; offsets and machine code are measured. */
typedef struct AxisMotion {
    long speed;
    long limit;
    long acceleration;
    long deceleration;
} AxisMotion;

typedef struct MovingObject {
    long x, y, z;
    unsigned char unknown_00c[0x94 - 0x0c];
    unsigned char negative_x, negative_y, negative_z;
    unsigned char unknown_097[0xa8 - 0x97];
    AxisMotion axis[3];
    long target_x, target_y, target_z;
} MovingObject;

#include "actor_bounds_2a880.h"
typedef Query2a170Bounds Bounds;
typedef char motion_long_is_32[sizeof(long)==4?1:-1];
#define MOTION_ADD(a,b) ((long)((unsigned long)(a)+(unsigned long)(b)))
#define MOTION_SUB(a,b) ((long)((unsigned long)(a)-(unsigned long)(b)))
unsigned char __cdecl eng1_native_code_target_002b5e0(MovingObject *p, MovingObject *target, unsigned char flags)
{
    unsigned char result = 0;
    Bounds bounds;
    long target_x,target_y,target_z;
    if (target == 0) flags &= 0xf8;
    else {
        if (actor_bounds_2a880(target,&bounds,0)) {
            target_x=(long)((((unsigned long)((long)bounds.mx[0]-bounds.mn[0])<<15)&0xffff0000UL)+((unsigned long)(long)bounds.mn[0]<<16));
            target_y=(long)((((unsigned long)((long)bounds.mx[1]-bounds.mn[1])<<15)&0xffff0000UL)+((unsigned long)(long)bounds.mn[1]<<16));
            target_z=(long)((((unsigned long)((long)bounds.mx[2]-bounds.mn[2])<<15)&0xffff0000UL)+((unsigned long)(long)bounds.mn[2]<<16));
        } else {target_x=target->x;target_y=target->y;target_z=target->z;}
        if (flags & 1) {
            if (p->negative_x) {
                if (p->x < target_x) {
                    p->axis[0].speed = MOTION_SUB(p->axis[0].speed,p->axis[0].deceleration);
                    result |= 1;
                } else p->axis[0].speed = MOTION_ADD(p->axis[0].speed,p->axis[0].acceleration);
            } else {
                if (p->x > target_x) {
                    p->axis[0].speed = MOTION_SUB(p->axis[0].speed,p->axis[0].deceleration);
                    result |= 1;
                } else p->axis[0].speed = MOTION_ADD(p->axis[0].speed,p->axis[0].acceleration);
            }
            if (p->axis[0].speed < 0) {
                p->axis[0].speed = 0;
                p->negative_x = ~p->negative_x;
            }
            if (p->axis[0].speed > p->axis[0].limit) p->axis[0].speed = p->axis[0].limit;
        }
        if (flags & 2) {
            if (p->negative_y) {
                if (p->y < target_y) {
                    p->axis[1].speed = MOTION_SUB(p->axis[1].speed,p->axis[1].deceleration);
                    result |= 2;
                } else p->axis[1].speed = MOTION_ADD(p->axis[1].speed,p->axis[1].acceleration);
            } else {
                if (p->y > target_y) {
                    p->axis[1].speed = MOTION_SUB(p->axis[1].speed,p->axis[1].deceleration);
                    result |= 2;
                } else p->axis[1].speed = MOTION_ADD(p->axis[1].speed,p->axis[1].acceleration);
            }
            if (p->axis[1].speed < 0) {
                p->axis[1].speed = 0;
                p->negative_y = ~p->negative_y;
            }
            if (p->axis[1].speed > p->axis[0].limit) p->axis[1].speed = p->axis[1].limit;
        }
        if (flags & 4) {
            if (p->negative_z) {
                if (p->z < target_z) {
                    p->axis[2].speed = MOTION_SUB(p->axis[2].speed,p->axis[2].deceleration);
                    result |= 4;
                } else p->axis[2].speed = MOTION_ADD(p->axis[2].speed,p->axis[2].acceleration);
            } else {
                if (p->z > target_z) {
                    p->axis[2].speed = MOTION_SUB(p->axis[2].speed,p->axis[2].deceleration);
                    result |= 4;
                } else p->axis[2].speed = MOTION_ADD(p->axis[2].speed,p->axis[2].acceleration);
            }
            if (p->axis[2].speed < 0) {
                p->axis[2].speed = 0;
                p->negative_z = ~p->negative_z;
            }
            if (p->axis[2].speed > p->axis[0].limit) p->axis[2].speed = p->axis[2].limit;
        }
    }
    if (flags & 8) {
        p->axis[0].speed = MOTION_ADD(p->axis[0].speed,MOTION_SUB(p->axis[0].acceleration,p->axis[0].deceleration));
        if (p->axis[0].speed < 0) p->axis[0].speed = 0;
        if (p->axis[0].speed > p->axis[0].limit) p->axis[0].speed = p->axis[0].limit;
    }
    if (flags & 16) {
        p->axis[1].speed = MOTION_ADD(p->axis[1].speed,MOTION_SUB(p->axis[1].acceleration,p->axis[1].deceleration));
        if (p->axis[1].speed < 0) p->axis[1].speed = 0;
        if (p->axis[1].speed > p->axis[1].limit) p->axis[1].speed = p->axis[1].limit;
    }
    if (flags & 32) {
        p->axis[2].speed = MOTION_ADD(p->axis[2].speed,MOTION_SUB(p->axis[2].acceleration,p->axis[2].deceleration));
        if (p->axis[2].speed < 0) p->axis[2].speed = 0;
        if (p->axis[2].speed > p->axis[2].limit) p->axis[2].speed = p->axis[2].limit;
    }
    if (flags & 64) {
        p->axis[0].speed = MOTION_ADD(p->axis[0].speed,p->axis[0].acceleration);
        if (p->axis[0].speed < 0) p->axis[0].speed = 0;
        if (p->axis[0].speed > p->axis[0].limit) p->axis[0].speed = p->axis[0].limit;
    }
    if (flags & 128) {
        p->axis[1].speed = MOTION_ADD(p->axis[1].speed,p->axis[1].acceleration);
        if (p->axis[1].speed < 0) p->axis[1].speed = 0;
        if (p->axis[1].speed > p->axis[1].limit) p->axis[1].speed = p->axis[1].limit;
    }
    if (p->negative_x) p->x = MOTION_SUB(p->x,p->axis[0].speed);
    else p->x = MOTION_ADD(p->x,p->axis[0].speed);
    if (p->negative_y) p->y = MOTION_SUB(p->y,p->axis[1].speed);
    else p->y = MOTION_ADD(p->y,p->axis[1].speed);
    if (p->negative_z) p->z = MOTION_SUB(p->z,p->axis[2].speed);
    else p->z = MOTION_ADD(p->z,p->axis[2].speed);
    return result;
}
