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
