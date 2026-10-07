/* Historical x86 execution of reconstructed source only; never load an oracle. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../src/shared/object_commands.c"
#include "../src/shared/motion.c"
#include "../src/shared/relative.c"

#define LAYOUT(name, expression) typedef char name[(expression) ? 1 : -1]
LAYOUT(long_is_32_bit, sizeof(long) == 4);
LAYOUT(command_flags, offsetof(CommandOwner, flags_050) == 0x50);
LAYOUT(command_pointer, offsetof(CommandOwner, command_128) == 0x128);
LAYOUT(motion_direction, offsetof(MovingObject, negative_x) == 0x94);
LAYOUT(motion_axis, offsetof(MovingObject, axis) == 0xa8);
LAYOUT(motion_axis_size, sizeof(AxisMotion) == 16);
LAYOUT(relative_direction, offsetof(RelativeMeasure, direction) == 10);
LAYOUT(relative_size, sizeof(RelativeMeasure) == 12);

static unsigned checks, failures;
#define CHECK(test) do { ++checks; if (!(test)) { ++failures; \
    printf("FAIL line %d: %s\n", __LINE__, #test); } } while (0)

static void test_command(void)
{
    CommandOwner owner, before;
    unsigned short code, *cursor = &code;
    unsigned long value, expected;
    memset(&owner, 0xa5, sizeof(owner));
    owner.flags_050 = 0;
    owner.command_128 = &cursor;
    before = owner;
    /* Exhaust the word domain using a range-based expectation, rather than
       repeating the implementation's mask expression in the test. */
    for (value = 0; value <= 65535; ++value) {
        code = (unsigned short)value;
        expected = (value >= 0xb780 && value <= 0xb7ff) ? value - 0x8000 : value;
        clear_command_high_bit(&owner);
        CHECK(code == expected);
    }
    CHECK(memcmp(&before, &owner, sizeof(owner)) == 0);
    CHECK(cursor == &code);
    code = 0xb780;
    owner.flags_050 = 0x10000000;
    clear_command_high_bit(&owner);
    CHECK(code == 0xb780);
    owner.command_128 = 0;
    owner.flags_050 = 0;
    clear_command_high_bit(&owner);
    CHECK(owner.command_128 == 0);
    owner.command_128 = &cursor;
    cursor = 0;
    owner.flags_050 = 0x10000000;
    clear_command_high_bit(&owner); /* flag gate must prevent inner dereference */
    CHECK(cursor == 0);
}

static void test_motion(void)
{
    MovingObject object, expected;
    unsigned signs;
    for (signs = 0; signs < 8; ++signs) {
        memset(&object, 0xa5, sizeof(object));
        object.x = 1000; object.y = 2000; object.z = 3000;
        object.negative_x = (signs & 1) ? 255 : 0;
        object.negative_y = (signs & 2) ? 2 : 0;
        object.negative_z = (signs & 4) ? 1 : 0;
        object.axis[0].speed = 10; object.axis[0].limit = 50;
        object.axis[0].acceleration = 5; object.axis[0].deceleration = 2;
        object.axis[1].speed = 2; object.axis[1].limit = 10;
        object.axis[1].acceleration = 0; object.axis[1].deceleration = 9;
        object.axis[2].speed = 90; object.axis[2].limit = 60;
        object.axis[2].acceleration = 4; object.axis[2].deceleration = 0;
        expected = object;
        expected.axis[0].speed = 13; expected.axis[1].speed = 0; expected.axis[2].speed = 60;
        expected.x = (signs & 1) ? 987 : 1013;
        expected.z = (signs & 4) ? 2940 : 3060;
        advance_object_motion(&object);
        CHECK(memcmp(&object, &expected, sizeof(object)) == 0);
    }
    memset(&object, 0, sizeof(object));
    object.axis[0].limit = -7;
    object.axis[0].deceleration = 1;
    advance_object_motion(&object);
    CHECK(object.axis[0].speed == -7 && object.x == -7);
    /* Preserve lower-then-upper clamp behavior, even for an unusual limit. */
    memset(&object, 0, sizeof(object));
    object.axis[0].limit = 10; object.axis[0].speed = 10;
    object.axis[1].limit = 10; object.axis[1].speed = 0;
    advance_object_motion(&object);
    CHECK(object.axis[0].speed == 10 && object.x == 10 && object.y == 0);
}

static void test_relative(void)
{
    FixedPosition a, b;
    RelativeMeasure result;
    unsigned signs, direction;
    memset(&b, 0, sizeof(b));
    for (signs = 0; signs < 8; ++signs) {
        a.x = ((signs & 1) ? -64L : 64L) * 65536L;
        a.y = ((signs & 2) ? -32L : 32L) * 65536L;
        a.z = ((signs & 4) ? -16L : 16L) * 65536L;
        direction = 4 + ((signs & 1) ? 2 : 0) - ((signs & 2) ? 4 : 0) + ((signs & 4) ? 8 : 0);
        measure_relative_vector(&a, &b, &result);
        CHECK(result.x == ((signs & 1) ? -64 : 64));
        CHECK(result.y == ((signs & 2) ? -32 : 32));
        CHECK(result.z == ((signs & 4) ? -16 : 16));
        CHECK(result.planar_distance == 76 && result.distance == 79);
        CHECK(result.direction == direction);
    }
    a.x = 32L * 65536L; a.y = 64L * 65536L; a.z = 128L * 65536L;
    measure_relative_vector(&a, &b, &result);
    CHECK(result.planar_distance == 76 && result.distance == 155 && result.direction == 5);
    a.x = -1; a.y = 65535; a.z = -65537;
    measure_relative_vector(&a, &b, &result);
    CHECK(result.x == -1 && result.y == 0 && result.z == -2);
    CHECK(result.planar_distance == 1 && result.distance == 2 && result.direction == 14);
    a.x = -2147483647L - 1; a.y = 0; a.z = 0;
    measure_relative_vector(&a, &b, &result);
    CHECK(result.x == -32768 && result.direction == 7);
    CHECK(result.planar_distance == -12288 && result.distance == -11264);
    /* The -32768 short absolute-value edge is retained, not modernized. */
    memset(&a, 0, sizeof(a));
    measure_relative_vector(&a, &a, &result);
    CHECK(result.x == 0 && result.y == 0 && result.z == 0);
    CHECK(result.distance == 0 && result.planar_distance == 0 && result.direction == 4);
}

int main(void)
{
    test_command(); test_motion(); test_relative();
    printf("%u semantic checks; %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
