/* Execute reconstructed source only. Byte matching is verified separately. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../src/title/pilot.c"

#define LAYOUT(name, condition) typedef char name[(condition) ? 1 : -1]
LAYOUT(pointer_is_32_bit, sizeof(void *) == 4);
LAYOUT(link_flags_offset, offsetof(LinkNode, flags_050) == 0x50);
LAYOUT(link_prev_e4_offset, offsetof(LinkNode, prev_0e4) == 0xe4);
LAYOUT(link_next_e8_offset, offsetof(LinkNode, next_0e8) == 0xe8);
LAYOUT(link_prev_12c_offset, offsetof(LinkNode, prev_12c) == 0x12c);
LAYOUT(link_next_130_offset, offsetof(LinkNode, next_130) == 0x130);
LAYOUT(link_object_size, sizeof(LinkNode) == 0x134);
LAYOUT(colour_intensity_offset, offsetof(ColourInput, intensity_04a) == 0x4a);
LAYOUT(colour_flags_offset, offsetof(ColourInput, flags_054) == 0x54);

static unsigned checks;
static unsigned failures;
#define CHECK(condition) do { \
    ++checks; \
    if (!(condition)) { \
        ++failures; \
        printf("FAIL line %d: %s\n", __LINE__, #condition); \
    } \
} while (0)

static void test_unlink(void)
{
    LinkNode left, item, right;
    unsigned mask;
    for (mask = 0; mask < 4; ++mask) {
        memset(&left, 0, sizeof(left));
        memset(&item, 0, sizeof(item));
        memset(&right, 0, sizeof(right));
        left.next_130 = &item;
        right.prev_12c = &item;
        item.prev_12c = (mask & 1) ? &left : 0;
        item.next_130 = (mask & 2) ? &right : 0;
        unlink_12c(&item);
        CHECK(left.next_130 == ((mask & 1) ? item.next_130 : &item));
        CHECK(right.prev_12c == ((mask & 2) ? item.prev_12c : &item));
        /* This function preserves its own links; the other unlink clears them. */
        CHECK(item.prev_12c == ((mask & 1) ? &left : 0));
        CHECK(item.next_130 == ((mask & 2) ? &right : 0));

        left.next_0e8 = &item;
        right.prev_0e4 = &item;
        item.prev_0e4 = (mask & 1) ? &left : 0;
        item.next_0e8 = (mask & 2) ? &right : 0;
        item.flags_050 = 0x100;
        unlink_0e4(&item);
        CHECK(left.next_0e8 == ((mask & 1) ? ((mask & 2) ? &right : 0) : &item));
        CHECK(right.prev_0e4 == ((mask & 2) ? ((mask & 1) ? &left : 0) : &item));
        CHECK(item.prev_0e4 == 0 && item.next_0e8 == 0);
        CHECK(item.flags_050 == 0x100);
    }
    item.prev_0e4 = &left;
    item.next_0e8 = &right;
    left.next_0e8 = &item;
    right.prev_0e4 = &item;
    item.flags_050 = 0x80000;
    unlink_0e4(&item);
    CHECK(item.prev_0e4 == &left && item.next_0e8 == &right);
    CHECK(left.next_0e8 == &item && right.prev_0e4 == &item);
    CHECK(item.flags_050 == 0x80000);
}

static void test_colour(void)
{
    static unsigned short inputs[] = {0, 1, 254, 255, 256, 65535};
    static unsigned long expected[] = {
        0x2c000000UL, 0x2c010101UL, 0x2cfefefeUL,
        0x2cffffffUL, 0x2cffffffUL, 0x2cffffffUL
    };
    ColourInput input;
    unsigned i;
    memset(&input, 0, sizeof(input));
    for (i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        input.intensity_04a = inputs[i];
        input.flags_054 = 0;
        CHECK(make_colour(&input) == expected[i]);
        input.flags_054 = 4;
        CHECK(make_colour(&input) == (expected[i] | 0x02000000UL));
        input.flags_054 = 8;
        CHECK(make_colour(&input) == expected[i]);
        CHECK(input.intensity_04a == inputs[i] && input.flags_054 == 8);
    }
}

static void test_sentinel(void)
{
    short empty[] = {-1};
    short one[] = {0, -1};
    short three[] = {32767, -32768, 0, -1};
    short early[] = {4, -1, 9, 10, -1};
    CHECK(sentinel_count(empty) == -1);
    CHECK(sentinel_count(one) == 0);
    CHECK(sentinel_count(three) == 2);
    CHECK(sentinel_count(early) == 0);
    CHECK(three[0] == 32767 && three[1] == -32768 && three[3] == -1);
}

int main(void)
{
    test_unlink();
    test_colour();
    test_sentinel();
    printf("%u semantic checks; %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
