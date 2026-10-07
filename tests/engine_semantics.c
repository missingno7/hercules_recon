/* Behavioral checks execute reconstructed C only. No oracle DLL is loaded. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../src/shared/motion.c"
#include "../src/shared/relative.c"
#include "../src/shared/frame_motion.c"
#include "../src/shared/bounds.c"

#define LAYOUT(name, expression) typedef char name[(expression) ? 1 : -1]
LAYOUT(target_position, offsetof(MovingObject, target_x) == 0xd8);
LAYOUT(frame_vertical, offsetof(FrameMotionObject, vertical_step) == 0x98);
LAYOUT(frame_table, offsetof(FrameMotionObject, steps) == 0xf0);
LAYOUT(frame_index, offsetof(FrameMotionObject, step_index) == 0xf4);
LAYOUT(frame_active, offsetof(FrameMotionObject, active_step) == 0x11e);
LAYOUT(step2_size, sizeof(Step2D) == 4);
LAYOUT(step3_size, sizeof(Step3D) == 6);

static unsigned checks, failures;
#define CHECK(test) do { ++checks; if (!(test)) { ++failures; \
    if (failures < 20) printf("FAIL line %d: %s\n", __LINE__, #test); } } while (0)

static void test_target_motion(void)
{
    const unsigned char directions[] = {0, 1, 2, 128, 255};
    const long speeds[] = {-3, 0, 4, 12};
    MovingObject actual, expected;
    unsigned flags, d, mode, axis, expected_mask;
    int relation;
    long positions[3], targets[3], speed, acceleration, deceleration, limit;
    unsigned char signs[3];
    for (flags = 0; flags < 256; ++flags) for (d = 0; d < 5; ++d) {
        for (mode = 0; mode < 4; ++mode) for (relation = -1; relation <= 1; ++relation) {
            memset(&actual, 0xa5, sizeof(actual));
            positions[0] = 100; positions[1] = -200; positions[2] = 300;
            for (axis = 0; axis < 3; ++axis) {
                targets[axis] = positions[axis] + relation * 10;
                signs[axis] = directions[(d + axis) % 5];
                actual.axis[axis].speed = speeds[(mode + axis) % 4];
                actual.axis[axis].limit = axis == 0 ? 5 : (axis == 1 ? 2 : 11);
                if (mode == 3) actual.axis[axis].limit -= 8;
                actual.axis[axis].acceleration = axis + 1;
                actual.axis[axis].deceleration = axis + 3;
            }
            actual.x = positions[0]; actual.y = positions[1]; actual.z = positions[2];
            actual.target_x = targets[0]; actual.target_y = targets[1]; actual.target_z = targets[2];
            actual.negative_x = signs[0]; actual.negative_y = signs[1]; actual.negative_z = signs[2];
            expected = actual;
            expected_mask = 0;
            /* Axis-wise model, independent of the source's duplicated branch tree.
               Positive signed distance along motion means moving away. */
            for (axis = 0; axis < 3; ++axis) {
                speed = actual.axis[axis].speed;
                acceleration = actual.axis[axis].acceleration;
                deceleration = actual.axis[axis].deceleration;
                limit = actual.axis[axis].limit;
                if (flags & (1 << axis)) {
                    if ((positions[axis] - targets[axis]) * (signs[axis] ? -1 : 1) > 0) {
                        speed -= deceleration;
                        expected_mask += 1 << axis;
                    } else speed += acceleration;
                    if (speed < 0) { speed = 0; signs[axis] = 255 - signs[axis]; }
                    if (speed > actual.axis[0].limit) speed = limit;
                }
                if (flags & (8 << axis)) {
                    speed += acceleration - deceleration;
                    if (speed < 0) speed = 0;
                    if (speed > limit) speed = limit;
                }
                expected.axis[axis].speed = speed;
                positions[axis] += speed * (signs[axis] ? -1 : 1);
            }
            expected.x = positions[0]; expected.y = positions[1]; expected.z = positions[2];
            expected.negative_x = signs[0]; expected.negative_y = signs[1]; expected.negative_z = signs[2];
            CHECK(advance_target_motion(&actual, (unsigned char)flags) == expected_mask);
            CHECK(memcmp(&actual, &expected, sizeof(actual)) == 0);
        }
    }
}

static void test_frame_motion(void)
{
    Step2D steps2[3] = {{17, 23}, {-5, 7}, {0, -9}};
    Step3D steps3[3] = {{17, 23, 31}, {-5, 7, -3}, {0, -9, 11}};
    const unsigned char directions[] = {0, 1, 2, 128, 255};
    FrameMotionObject actual, expected;
    int index, xsign, zsign;
    for (index = 0; index < 3; ++index) for (xsign = 0; xsign < 5; ++xsign) {
        for (zsign = 0; zsign < 5; ++zsign) {
            memset(&actual, 0xa5, sizeof(actual));
            actual.x = 1000; actual.y = -2000; actual.z = 3000;
            actual.negative_x = directions[xsign]; actual.negative_z = directions[zsign];
            actual.active_step = 0; actual.step_index = index; actual.vertical_step = -2;
            actual.steps = steps2;
            expected = actual;
            expected.x += (xsign ? -1L : 1L) * steps2[index].x * 65536L;
            expected.y += steps2[index].y * 65536L;
            expected.z -= 131072;
            apply_frame_motion_2d(&actual);
            CHECK(memcmp(&actual, &expected, sizeof(actual)) == 0);
            actual.steps = steps3;
            expected = actual;
            expected.x += (xsign ? -1L : 1L) * steps3[index].x * 65536L;
            expected.y += steps3[index].y * 65536L;
            expected.z += (zsign ? 1L : -1L) * steps3[index].z * 65536L;
            apply_frame_motion_3d(&actual);
            CHECK(memcmp(&actual, &expected, sizeof(actual)) == 0);
        }
    }
    actual.active_step = -1; actual.steps = 0; actual.step_index = -100;
    expected = actual;
    apply_frame_motion_2d(&actual); apply_frame_motion_3d(&actual);
    CHECK(memcmp(&actual, &expected, sizeof(actual)) == 0);
    CHECK(steps2[1].x == -5 && steps2[1].y == 7 && steps3[1].z == -3);
}

static void test_vector_variants(void)
{
    FixedPosition a, b = {0, 0, 0};
    RelativeMeasure signed_result, absolute_result, xz_result;
    unsigned signs;
    for (signs = 0; signs < 8; ++signs) {
        a.x = ((signs & 1) ? -64L : 64L) * 65536L;
        a.y = ((signs & 2) ? -32L : 32L) * 65536L;
        a.z = ((signs & 4) ? -16L : 16L) * 65536L;
        measure_relative_vector(&a, &b, &signed_result);
        measure_absolute_vector(&a, &b, &absolute_result);
        measure_xz_vector(&a, &b, &xz_result);
        CHECK(absolute_result.x == 64 && absolute_result.y == 32 && absolute_result.z == 16);
        CHECK(absolute_result.planar_distance == 76 && absolute_result.distance == 79);
        CHECK(absolute_result.direction == signed_result.direction);
        CHECK(xz_result.x == 64 && xz_result.y == 0 && xz_result.z == 16);
        CHECK(xz_result.planar_distance == 70 && xz_result.distance == 0);
        CHECK(xz_result.direction == 4 + ((signs & 1) ? 2 : 0) + ((signs & 4) ? 8 : 0));
    }
    a.x = 16L * 65536L; a.y = -12345; a.z = 64L * 65536L;
    measure_xz_vector(&a, &b, &xz_result);
    CHECK(xz_result.planar_distance == 70 && xz_result.direction == 4);
    a.x = -1; a.y = 65535; a.z = -65537;
    measure_absolute_vector(&a, &b, &absolute_result);
    CHECK(absolute_result.x == 1 && absolute_result.y == 0 && absolute_result.z == 2);
    a.x = -2147483647L - 1; a.y = 0; a.z = 0;
    measure_absolute_vector(&a, &b, &absolute_result);
    CHECK(absolute_result.x == -32768 && absolute_result.direction == 7);
    CHECK(absolute_result.planar_distance == -12288 && absolute_result.distance == -11264);
    measure_xz_vector(&a, &b, &xz_result);
    CHECK(xz_result.x == -32768 && xz_result.planar_distance == -12288 && xz_result.direction == 6);
}

static void test_planar_bounds(void)
{
    Bounds3D a = {-4, -5, -32768, 4, 5, -1};
    Bounds3D b = {-2, -3, 1, 2, 3, 32767};
    int x, y, common, px, py;
    for (x = -6; x <= 6; ++x) for (y = -7; y <= 7; ++y) {
        b.min_x = x; b.max_x = x + 1; b.min_y = y; b.max_y = y + 1;
        common = 0;
        for (px = x; px <= x + 1; ++px) for (py = y; py <= y + 1; ++py)
            if (px >= -4 && px <= 4 && py >= -5 && py <= 5) common = 1;
        CHECK(planar_bounds_overlap(&a, &b) == common);
        CHECK(planar_bounds_overlap(&b, &a) == common);
        CHECK(bounds_overlap(&a, &b) == 0); /* z disjointness is ignored only by planar. */
    }
}

int main(void)
{
    test_target_motion(); test_frame_motion(); test_vector_variants(); test_planar_bounds();
    printf("%u semantic checks; %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
