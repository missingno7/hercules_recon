#include <stdio.h>
typedef unsigned long U32;
typedef signed long S32;
typedef char u32_is_32[(sizeof(U32) == 4) ? 1 : -1];
typedef char s32_is_32[(sizeof(S32) == 4) ? 1 : -1];

static S32 rounded_groups_input(U32 byte_count)
{
    return (S32)(byte_count + 3UL) >> 2;
}

typedef struct BoundaryCase {
    U32 bytes;
    S32 expected;
} BoundaryCase;

int main(void)
{
    static const BoundaryCase cases[] = {
        {0x00000000UL, 0},
        {0x00000001UL, 1},
        {0x00000003UL, 1},
        {0x00000004UL, 1},
        {0x7ffffffcUL, 0x1fffffffL},
        {0x7ffffffdUL, -536870912L},
        {0x7fffffffUL, -536870912L},
        {0x80000000UL, -536870912L},
        {0xfffffffcUL, -1L},
        {0xfffffffdUL, 0},
        {0xffffffffUL, 0}
    };
    volatile U32 input;
    unsigned int i;
    unsigned int failures = 0;
    for (i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
        S32 actual;
        input = cases[i].bytes;
        actual = rounded_groups_input(input);
        if (actual != cases[i].expected) {
            ++failures;
            printf("FAIL bytes=%08lx expected=%ld actual=%ld\n",
                   input, cases[i].expected, actual);
        }
    }
    printf("boundary_cases=%u failures=%u\n",
           (unsigned int)(sizeof(cases)/sizeof(cases[0])), failures);
    return failures != 0;
}
