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

void advance_object_motion(MovingObject *p)
{
    p->axis[0].speed += p->axis[0].acceleration - p->axis[0].deceleration;
    if (p->axis[0].speed < 0) p->axis[0].speed = 0;
    if (p->axis[0].speed > p->axis[0].limit) p->axis[0].speed = p->axis[0].limit;
    p->axis[1].speed += p->axis[1].acceleration - p->axis[1].deceleration;
    if (p->axis[1].speed < 0) p->axis[1].speed = 0;
    if (p->axis[1].speed > p->axis[1].limit) p->axis[1].speed = p->axis[1].limit;
    p->axis[2].speed += p->axis[2].acceleration - p->axis[2].deceleration;
    if (p->axis[2].speed < 0) p->axis[2].speed = 0;
    if (p->axis[2].speed > p->axis[2].limit) p->axis[2].speed = p->axis[2].limit;
    if (p->negative_x) p->x -= p->axis[0].speed;
    else p->x += p->axis[0].speed;
    if (p->negative_y) p->y -= p->axis[1].speed;
    else p->y += p->axis[1].speed;
    if (p->negative_z) p->z -= p->axis[2].speed;
    else p->z += p->axis[2].speed;
}

void advance_planar_motion(MovingObject *p)
{
    p->axis[0].speed += p->axis[0].acceleration - p->axis[0].deceleration;
    if (p->axis[0].speed < 0) p->axis[0].speed = 0;
    if (p->axis[0].speed > p->axis[0].limit) p->axis[0].speed = p->axis[0].limit;
    p->axis[1].speed += p->axis[1].acceleration - p->axis[1].deceleration;
    if (p->axis[1].speed < 0) p->axis[1].speed = 0;
    if (p->axis[1].speed > p->axis[1].limit) p->axis[1].speed = p->axis[1].limit;
    if (p->negative_x) p->x -= p->axis[0].speed;
    else p->x += p->axis[0].speed;
    if (p->negative_y) p->y -= p->axis[1].speed;
    else p->y += p->axis[1].speed;
}

void advance_vertical_motion(MovingObject *p)
{
    p->axis[2].speed += p->axis[2].acceleration - p->axis[2].deceleration;
    if (p->axis[2].speed < 0) p->axis[2].speed = 0;
    if (p->axis[2].speed > p->axis[2].limit) p->axis[2].speed = p->axis[2].limit;
    if (p->negative_z) p->z -= p->axis[2].speed;
    else p->z += p->axis[2].speed;
}

/* Low three flags select target steering; the next three apply ordinary
 * acceleration/deceleration afterwards. Positions always advance.
 * The y/z steering comparisons intentionally use the x-axis limit.
 * Underflow complements the whole direction byte, not just its truth value.
 * Only low argument/return bytes are established by this function. */
unsigned char advance_target_motion(MovingObject *p, unsigned char flags)
{
    unsigned char result = 0;
    if (flags & 1) {
        if (p->negative_x) {
            if (p->x < p->target_x) {
                p->axis[0].speed -= p->axis[0].deceleration;
                result |= 1;
            } else p->axis[0].speed += p->axis[0].acceleration;
        } else {
            if (p->x > p->target_x) {
                p->axis[0].speed -= p->axis[0].deceleration;
                result |= 1;
            } else p->axis[0].speed += p->axis[0].acceleration;
        }
        if (p->axis[0].speed < 0) {
            p->axis[0].speed = 0;
            p->negative_x = ~p->negative_x;
        }
        if (p->axis[0].speed > p->axis[0].limit) p->axis[0].speed = p->axis[0].limit;
    }
    if (flags & 2) {
        if (p->negative_y) {
            if (p->y < p->target_y) {
                p->axis[1].speed -= p->axis[1].deceleration;
                result |= 2;
            } else p->axis[1].speed += p->axis[1].acceleration;
        } else {
            if (p->y > p->target_y) {
                p->axis[1].speed -= p->axis[1].deceleration;
                result |= 2;
            } else p->axis[1].speed += p->axis[1].acceleration;
        }
        if (p->axis[1].speed < 0) {
            p->axis[1].speed = 0;
            p->negative_y = ~p->negative_y;
        }
        if (p->axis[1].speed > p->axis[0].limit) p->axis[1].speed = p->axis[1].limit;
    }
    if (flags & 4) {
        if (p->negative_z) {
            if (p->z < p->target_z) {
                p->axis[2].speed -= p->axis[2].deceleration;
                result |= 4;
            } else p->axis[2].speed += p->axis[2].acceleration;
        } else {
            if (p->z > p->target_z) {
                p->axis[2].speed -= p->axis[2].deceleration;
                result |= 4;
            } else p->axis[2].speed += p->axis[2].acceleration;
        }
        if (p->axis[2].speed < 0) {
            p->axis[2].speed = 0;
            p->negative_z = ~p->negative_z;
        }
        if (p->axis[2].speed > p->axis[0].limit) p->axis[2].speed = p->axis[2].limit;
    }
    if (flags & 8) {
        p->axis[0].speed += p->axis[0].acceleration - p->axis[0].deceleration;
        if (p->axis[0].speed < 0) p->axis[0].speed = 0;
        if (p->axis[0].speed > p->axis[0].limit) p->axis[0].speed = p->axis[0].limit;
    }
    if (flags & 16) {
        p->axis[1].speed += p->axis[1].acceleration - p->axis[1].deceleration;
        if (p->axis[1].speed < 0) p->axis[1].speed = 0;
        if (p->axis[1].speed > p->axis[1].limit) p->axis[1].speed = p->axis[1].limit;
    }
    if (flags & 32) {
        p->axis[2].speed += p->axis[2].acceleration - p->axis[2].deceleration;
        if (p->axis[2].speed < 0) p->axis[2].speed = 0;
        if (p->axis[2].speed > p->axis[2].limit) p->axis[2].speed = p->axis[2].limit;
    }
    if (p->negative_x) p->x -= p->axis[0].speed;
    else p->x += p->axis[0].speed;
    if (p->negative_y) p->y -= p->axis[1].speed;
    else p->y += p->axis[1].speed;
    if (p->negative_z) p->z -= p->axis[2].speed;
    else p->z += p->axis[2].speed;
    return result;
}
