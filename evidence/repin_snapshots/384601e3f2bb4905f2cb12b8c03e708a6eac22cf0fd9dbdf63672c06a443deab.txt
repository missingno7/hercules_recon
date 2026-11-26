/* ENG1 2bc50/2bd20: motion and floor-contact response.
 * Keep the separate motion call in the damped entry and the inline update in
 * the other entry. Reload state after contact callbacks. Partial object view. */
typedef struct AxisMotion { long speed, limit, acceleration, deceleration; } AxisMotion;
typedef struct MovingObject {
    long x,y,z;
    unsigned char unknown_00c[0x94-0xc];
    unsigned char negative_x,negative_y,negative_z;
    unsigned char unknown_097[0xa8-0x97];
    AxisMotion axis[3];
} MovingObject;
typedef char long_is_32[sizeof(long)==4?1:-1];
extern void advance_object_motion(MovingObject *);
extern int eng1_29e20(MovingObject *);
extern int eng1_29f50(MovingObject *);
#define ADD(a,b) ((long)((unsigned long)(a)+(unsigned long)(b)))
#define SUB(a,b) ((long)((unsigned long)(a)-(unsigned long)(b)))

int __cdecl damped_contact_motion(MovingObject *p)
{
    int result=0;
    advance_object_motion(p);
    if (p->axis[1].speed==0) {
        p->axis[1].acceleration=p->axis[1].deceleration;
        p->axis[1].deceleration=0;
        p->negative_y=0;
        p->axis[0].deceleration=p->axis[0].acceleration;
        p->axis[0].acceleration=0;
    }
    if (!p->negative_y && eng1_29e20(p)) {
        p->axis[1].deceleration=p->axis[1].acceleration;
        p->axis[1].speed>>=1;
        p->axis[0].limit>>=1;
        p->axis[1].acceleration=0;
        p->negative_y=255;
        p->axis[0].acceleration=p->axis[0].deceleration;
        p->axis[0].deceleration=0;
        result=1;
        p->axis[2].limit>>=1;
        if (!p->axis[1].speed)result=3;
    }
    return result;
}

int __cdecl contact_motion(MovingObject *p)
{
    int result=0;
    p->axis[0].speed=ADD(p->axis[0].speed,SUB(p->axis[0].acceleration,p->axis[0].deceleration));
    if (p->axis[0].speed<0)p->axis[0].speed=0;
    if (p->axis[0].speed>p->axis[0].limit)p->axis[0].speed=p->axis[0].limit;
    p->axis[1].speed=ADD(p->axis[1].speed,SUB(p->axis[1].acceleration,p->axis[1].deceleration));
    if (p->axis[1].speed<0)p->axis[1].speed=0;
    if (p->axis[1].speed>p->axis[1].limit)p->axis[1].speed=p->axis[1].limit;
    p->axis[2].speed=ADD(p->axis[2].speed,SUB(p->axis[2].acceleration,p->axis[2].deceleration));
    if (p->axis[2].speed<0)p->axis[2].speed=0;
    if (p->axis[2].speed>p->axis[2].limit)p->axis[2].speed=p->axis[2].limit;
    if (p->negative_x)p->x=SUB(p->x,p->axis[0].speed);
    else p->x=ADD(p->x,p->axis[0].speed);
    if (p->negative_y)p->y=SUB(p->y,p->axis[1].speed);
    else p->y=ADD(p->y,p->axis[1].speed);
    if (p->negative_z)p->z=SUB(p->z,p->axis[2].speed);
    else p->z=ADD(p->z,p->axis[2].speed);
    if (p->axis[1].speed==0) {
        p->axis[1].acceleration=p->axis[1].deceleration;
        p->axis[1].deceleration=0;
        p->negative_y=0;
        p->axis[0].deceleration=p->axis[0].acceleration;
        p->axis[0].acceleration=0;
    }
    if (!p->negative_y && eng1_29f50(p)) {
        p->axis[1].speed>>=1;
        p->axis[1].deceleration=p->axis[1].acceleration;
        p->axis[1].acceleration=0;
        p->negative_y=255;
        p->axis[0].acceleration=p->axis[0].deceleration;
        p->axis[0].deceleration=0;
        result=1;
        if (!p->axis[1].speed)result=3;
    }
    return result;
}

