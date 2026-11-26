#include "../shared/engine_interface.h"

/* ENG3.DLL 0x1460: actor motion transition helper (NEAR).
 *
 * This function advances actor motion, then changes the actor's motion state
 * under the native level-specific transition conditions. The field offsets
 * and calls are grounded in ENG3.DLL; these local type/name views do not claim
 * original declarations or original translation-unit ownership.
 */
typedef struct Actor Actor;
struct Actor {
    long x, y, z;
    unsigned char unknown_00c[0x2a - 0x0c];
    short state;
    unsigned char unknown_02c[0x30 - 0x2c];
    short ticks, phase;
    unsigned short sprite;
    unsigned char unknown_036[0x38 - 0x36];
    unsigned short profile;
    short scale;
    unsigned char unknown_03c[0x4a - 0x3c];
    unsigned short unknown_4a;
    long parameter;
    unsigned long flags, render_flags;
    unsigned char *owner_context;
    unsigned char unknown_05c[0x74 - 0x5c];
    short unknown_74;
    unsigned char unknown_076[0x88 - 0x76];
    unsigned char unknown_88[0x94 - 0x88];
    unsigned char unknown_94, unknown_95;
    unsigned char unknown_96[0x98 - 0x96];
    short unknown_98, unknown_9a;
    unsigned char unknown_09c[0xa8 - 0x9c];
    long unknown_a8, unknown_ac, unknown_b0, unknown_b4;
    long unknown_b8, unknown_bc, unknown_c0, unknown_c4;
    long unknown_c8, unknown_cc, unknown_d0, unknown_d4;
};

typedef struct AxisMotion {
    long speed, limit, acceleration, deceleration;
} AxisMotion;
typedef struct MovingObject {
    long x, y, z;
    unsigned char unknown_00c[0x94 - 0x0c];
    unsigned char negative_x, negative_y, negative_z;
    unsigned char unknown_097[0xa8 - 0x97];
    AxisMotion axis[3];
    long target_x, target_y, target_z;
} MovingObject;

typedef char actor_view_layout_verified[
    (sizeof(long) == 4 && sizeof(Actor) == 0xd8 &&
     offsetof(Actor, x) == 0x00 && offsetof(Actor, profile) == 0x38 &&
     offsetof(Actor, unknown_95) == 0x95 &&
     offsetof(Actor, unknown_9a) == 0x9a &&
     offsetof(Actor, unknown_ac) == 0xac &&
     offsetof(Actor, unknown_b0) == 0xb0 &&
     offsetof(Actor, unknown_b4) == 0xb4 &&
     offsetof(Actor, unknown_b8) == 0xb8 &&
     offsetof(Actor, unknown_c0) == 0xc0 &&
     offsetof(Actor, unknown_c4) == 0xc4 &&
     offsetof(Actor, unknown_cc) == 0xcc) ? 1 : -1];
typedef char moving_object_view_layout_verified[
    (sizeof(AxisMotion) == 0x10 && sizeof(MovingObject) == 0xe4 &&
     offsetof(MovingObject, negative_x) == 0x94 &&
     offsetof(MovingObject, axis) == 0xa8 &&
     offsetof(MovingObject, target_x) == 0xd8) ? 1 : -1];

extern Actor *g_eng3_actor_41d54;
extern void advance_object_motion(MovingObject *);
extern int eng3_1eaf0(Actor *);

int eng3_01460(Actor *p)
{
    long threshold = 0;

    if (*(signed char *)g_engine_interface.context_004 == 8) {
        threshold = 0x1c00000;
        if (g_eng3_actor_41d54->profile == 0) {
            long x_sector = (long)((unsigned long)p->x & 0xff800000UL);
            if (x_sector > 0x37000000 && x_sector < 0x41000000)
                threshold = 0x1400000;
        }
    }

    advance_object_motion((MovingObject *)p);
    p->unknown_9a = 0;
    if (p->unknown_b8 == 0) {
        p->unknown_c0 = p->unknown_c4;
        p->unknown_c4 = 0;
        p->unknown_95 = 0;
        p->unknown_b4 = p->unknown_b0;
        p->unknown_b0 = 0;
    }
    if (p->unknown_95 != 0)
        return 0;

    if (*(signed char *)g_engine_interface.context_004 == 8 && p->y >= threshold) {
        p->unknown_b0 = p->unknown_b4;
        p->unknown_c4 = p->unknown_c0;
        p->unknown_b8 >>= 1;
        p->unknown_c0 = 0;
        p->unknown_95 = 0xff;
        p->unknown_ac >>= 1;
        p->unknown_b4 = 0;
        p->unknown_9a = 1;
        p->unknown_cc >>= 1;
        if (p->unknown_b8 == 0)
            return 1;
    }

    if ((*(signed char *)g_engine_interface.context_004 == 10 ||
         *(signed char *)g_engine_interface.context_004 == 2) && eng3_1eaf0(p)) {
        p->unknown_b0 = p->unknown_b4;
        p->unknown_c4 = p->unknown_c0;
        p->unknown_b8 >>= 1;
        p->unknown_c0 = 0;
        p->unknown_95 = 0xff;
        p->unknown_ac >>= 1;
        p->unknown_b4 = 0;
        p->unknown_9a = 1;
        p->unknown_cc >>= 1;
        if (p->unknown_b8 == 0)
            return 1;
    }
    return 0;
}
