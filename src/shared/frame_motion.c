/* Indexed signed-short animation steps, scaled to 16.16 by historical VC5.
 * Active-step < 0 suppresses all memory access through the step table.
 * The 3D variant intentionally applies the opposite z sign convention from
 * advance_object_motion. Names are provisional; offsets are measured. */
typedef struct FrameMotionObject {
    long x, y, z;
    unsigned char unknown_00c[0x94 - 0x0c];
    unsigned char negative_x, negative_y, negative_z;
    unsigned char unknown_097;
    short vertical_step;
    unsigned char unknown_09a[0xf0 - 0x9a];
    void *steps;
    long step_index;
    unsigned char unknown_0f8[0x11e - 0xf8];
    short active_step;
} FrameMotionObject;

typedef struct Step2D { short x, y; } Step2D;
typedef struct Step3D { short x, y, z; } Step3D;

void apply_frame_motion_2d(FrameMotionObject *p)
{
    Step2D *step;
    short x;
    if (p->active_step >= 0) {
        step = (Step2D *)p->steps + p->step_index;
        x = step->x;
        if (p->negative_x) p->x -= x << 16;
        else p->x += x << 16;
        p->y += step->y << 16;
        p->z += p->vertical_step << 16;
    }
}

void apply_frame_motion_3d(FrameMotionObject *p)
{
    Step3D *step;
    short x, z;
    if (p->active_step >= 0) {
        step = (Step3D *)p->steps + p->step_index;
        x = step->x;
        z = step->z;
        if (p->negative_x) p->x -= x << 16;
        else p->x += x << 16;
        p->y += step->y << 16;
        if (p->negative_z) p->z += z << 16;
        else p->z -= z << 16;
    }
}
