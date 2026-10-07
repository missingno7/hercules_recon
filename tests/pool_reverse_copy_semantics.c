#include <stdio.h>
#include <string.h>
#include "../calibration/pool_reverse_copy.c"

static unsigned long checks;
static unsigned long failures;

static void expect(int ok, const char *label)
{
    ++checks;
    if (!ok) {
        ++failures;
        if (failures <= 20)
            printf("FAIL %s\n", label);
    }
}

static int rounded_dwords(int byte_count)
{
    return (byte_count + 3) >> 2;
}

static void reference_reverse(unsigned long *destination,
                              const unsigned long *source,
                              int byte_count)
{
    int count = rounded_dwords(byte_count);
    if (count <= 0)
        return;
    destination += count;
    source += count;
    while (count > 0) {
        --destination;
        --source;
        *destination = *source;
        --count;
    }
}

static void fill_words(unsigned long *words, int count, unsigned long seed)
{
    int i;
    for (i = 0; i < count; ++i)
        words[i] = seed + (unsigned long)(i * 0x1020304L) + (unsigned long)i;
}

static void test_rounding_and_scalar_tail(void)
{
    static const int lengths[] = {
        0,1,2,3,4,5,7,8,15,16,27,28,29,31,32,33,35,60,64,68,96
    };
    unsigned long source[40];
    unsigned long destination[40];
    unsigned long sentinel = 0x91a2b3c4UL;
    int cases = sizeof(lengths)/sizeof(lengths[0]);
    int test;
    int i;
    for (test = 0; test < cases; ++test) {
        int words = rounded_dwords(lengths[test]);
        fill_words(source, 40, 0x10293847UL);
        for (i = 0; i < 40; ++i)
            destination[i] = sentinel;
        reverse_copy_dwords(&destination[5], &source[3], lengths[test]);
        for (i = 0; i < words; ++i)
            expect(destination[5+i] == source[3+i], "rounded dword content");
        for (i = 0; i < 40; ++i)
            if (i < 5 || i >= 5 + words)
                expect(destination[i] == sentinel, "rounded copy leaves neighboring dwords intact");
    }
}

static void test_nonpositive_counts(void)
{
    static const int lengths[] = {-32,-8,-7,-5,-4,-3,-2,-1,0};
    unsigned long source[48];
    unsigned long destination[48];
    unsigned long source_before[48];
    unsigned long destination_before[48];
    int cases = sizeof(lengths)/sizeof(lengths[0]);
    int i;
    int test;
    for (test = 0; test < cases; ++test) {
        fill_words(source, 48, 0x3456789aUL);
        fill_words(destination, 48, 0xa1b2c3d4UL);
        memcpy(source_before, source, sizeof(source));
        memcpy(destination_before, destination, sizeof(destination));
        reverse_copy_dwords(&destination[24], &source[24], lengths[test]);
        for (i = 0; i < 48; ++i) {
            expect(source[i] == source_before[i], "nonpositive count preserves source");
            expect(destination[i] == destination_before[i], "nonpositive count performs no writes");
        }
    }
}

static void test_compactor_overlap(void)
{
    static const int shifts[] = {1,2,7};
    unsigned long arena[64];
    unsigned long saved[24];
    int test;
    int i;
    for (test = 0; test < sizeof(shifts)/sizeof(shifts[0]); ++test) {
        int source_index = 8;
        int words = 17;
        fill_words(arena, 64, 0x55667788UL);
        for (i = 0; i < words; ++i)
            saved[i] = arena[source_index+i];
        reverse_copy_dwords(&arena[source_index+shifts[test]],
                            &arena[source_index], words*4);
        for (i = 0; i < words; ++i)
            expect(arena[source_index+shifts[test]+i] == saved[i], "rightward overlapping compactor move");
    }
}

static void test_backward_only_direction(void)
{
    unsigned long actual[16];
    unsigned long expected[16];
    int i;
    fill_words(actual, 16, 0x11223344UL);
    memcpy(expected, actual, sizeof(actual));
    reference_reverse(&expected[4], &expected[5], 16);
    reverse_copy_dwords(&actual[4], &actual[5], 16);
    for (i = 0; i < 16; ++i)
        expect(actual[i] == expected[i], "overlap follows reverse-copy traversal");
    expect(actual[4] == actual[5], "leftward overlap is not treated as forward memmove");
}

int main(void)
{
    test_rounding_and_scalar_tail();
    test_nonpositive_counts();
    test_compactor_overlap();
    test_backward_only_direction();
    printf("checks=%lu failures=%lu\n", checks, failures);
    return failures != 0;
}
