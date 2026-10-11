/*
 * Private behavioral differential fixture for recovered TITLE primitives.
 * The original DLL is loaded only by this test program and is called at
 * reviewed RVAs. Reconstructed implementations are linked from repository
 * source files at build time; they never delegate to the oracle.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef struct DigitSprite {
    char unknown_00[0x34];
    short frame;
    char unknown_36[0x54 - 0x36];
    unsigned long flags;
} DigitSprite;

typedef struct ColourInput {
    unsigned char unknown_000[0x4a];
    unsigned short intensity_04a;
    unsigned char unknown_04c[8];
    unsigned long flags_054;
} ColourInput;

void set_decimal_digits(DigitSprite *, DigitSprite *, DigitSprite *, DigitSprite *, int);
unsigned long make_colour(ColourInput *);
int sentinel_count(short *);

typedef void (__cdecl *OriginalDigits)(DigitSprite *, DigitSprite *, DigitSprite *, DigitSprite *, int);
typedef unsigned long (__cdecl *OriginalColour)(ColourInput *);
typedef int (__cdecl *OriginalSentinel)(short *);

typedef char digit_sprite_must_be_0x58_bytes[(sizeof(DigitSprite) == 0x58) ? 1 : -1];
typedef char colour_input_must_be_0x58_bytes[(sizeof(ColourInput) == 0x58) ? 1 : -1];

static unsigned long random_state = 0xD1FF3A71UL;
static unsigned long decimal_cases;
static unsigned long colour_cases;
static unsigned long sentinel_cases;
static unsigned long random_word(void)
{
    random_state = random_state * 1664525UL + 1013904223UL;
    return random_state;
}

static int random_int(void)
{
    unsigned long word;
    word = random_word();
    if (word & 0x80000000UL) return -(int)(word & 0x7fffffffUL);
    return (int)word;
}

static void make_alias_patterns(int position, int highest, int map[4],
                                int patterns[16][4], int *count)
{
    int choice;
    if (position == 4) {
        memcpy(patterns[*count], map, sizeof(map[0]) * 4);
        ++*count;
        return;
    }
    for (choice = 0; choice <= highest + 1; ++choice) {
        map[position] = choice;
        make_alias_patterns(position + 1,
                            choice > highest ? choice : highest,
                            map, patterns, count);
    }
}

static void bind_args(DigitSprite boxes[4], int map[4], DigitSprite *args[4])
{
    int i;
    for (i = 0; i < 4; ++i) args[i] = &boxes[map[i]];
}

static int check_decimal(OriginalDigits original, int value, int pattern[4])
{
    DigitSprite expected[4];
    DigitSprite actual[4];
    DigitSprite *expected_args[4];
    DigitSprite *actual_args[4];
    unsigned char *expected_bytes;
    unsigned char *actual_bytes;
    unsigned long seed;
    unsigned long i;
    unsigned long byte_count;
    seed = random_state;
    expected_bytes = (unsigned char *)expected;
    actual_bytes = (unsigned char *)actual;
    byte_count = sizeof(expected);
    for (i = 0; i < byte_count; ++i) expected_bytes[i] = (unsigned char)random_word();
    memcpy(actual, expected, sizeof(expected));
    bind_args(expected, pattern, expected_args);
    bind_args(actual, pattern, actual_args);
    original(expected_args[0], expected_args[1], expected_args[2], expected_args[3], value);
    set_decimal_digits(actual_args[0], actual_args[1], actual_args[2], actual_args[3], value);
    ++decimal_cases;
    for (i = 0; i < byte_count; ++i) {
        if (expected_bytes[i] != actual_bytes[i]) {
            printf("DIVERGENCE set_decimal_digits case=%lu value=%ld alias=%d%d%d%d seed=%08lx byte=%lu original=%02x candidate=%02x\n",
                   decimal_cases, (long)value, pattern[0], pattern[1], pattern[2], pattern[3],
                   seed, i, (unsigned int)expected_bytes[i], (unsigned int)actual_bytes[i]);
            return 0;
        }
    }
    return 1;
}

static int test_decimal(HMODULE module)
{
    static int boundaries[] = {
        (-2147483647 - 1), -2000000, -1001, -1000, -999, -101, -100, -99,
        -11, -10, -9, -1, 0, 1, 9, 10, 11, 99, 100, 101, 999, 1000,
        1001, 2000000, 2147483647
    };
    int patterns[16][4];
    int map[4];
    int pattern_count;
    int p;
    int i;
    int value;
    int boundary_count;
    OriginalDigits original;
    map[0] = 0;
    pattern_count = 0;
    make_alias_patterns(1, 0, map, patterns, &pattern_count);
    if (pattern_count != 15) {
        printf("HARNESS ERROR: expected 15 pointer-alias partitions, got %d\n", pattern_count);
        return 0;
    }
    original = (OriginalDigits)((unsigned char *)module + 0x1a2c0);
    boundary_count = sizeof(boundaries) / sizeof(boundaries[0]);
    for (p = 0; p < pattern_count; ++p) {
        for (i = 0; i < boundary_count; ++i) {
            if (!check_decimal(original, boundaries[i], patterns[p])) return 0;
        }
    }
    for (i = 0; i < 2048; ++i) {
        value = random_int();
        for (p = 0; p < pattern_count; ++p) {
            if (!check_decimal(original, value, patterns[p])) return 0;
        }
    }
    printf("PASS set_decimal_digits cases=%lu alias_partitions=%d boundary_values=%d random_values=2048 full_state_bytes=%u\n",
           decimal_cases, pattern_count, boundary_count, (unsigned int)sizeof(DigitSprite) * 4);
    return 1;
}

static int check_colour(OriginalColour original, unsigned short intensity, unsigned long flags)
{
    ColourInput input;
    unsigned long expected;
    unsigned long actual;
    memset(&input, 0xA5, sizeof(input));
    input.intensity_04a = intensity;
    input.flags_054 = flags;
    expected = original(&input);
    actual = make_colour(&input);
    ++colour_cases;
    if (expected != actual) {
        printf("DIVERGENCE make_colour case=%lu intensity=%u flags=%08lx original=%08lx candidate=%08lx\n",
               colour_cases, (unsigned int)intensity, flags, expected, actual);
        return 0;
    }
    return 1;
}

static int test_colour(HMODULE module)
{
    OriginalColour original;
    unsigned long flags[3];
    unsigned long i;
    original = (OriginalColour)((unsigned char *)module + 0x189a0);
    flags[0] = 0;
    flags[1] = 4;
    flags[2] = 0xffffffffUL;
    for (i = 0; i < 65536UL; ++i) {
        if (!check_colour(original, (unsigned short)i, flags[0]) ||
            !check_colour(original, (unsigned short)i, flags[1]) ||
            !check_colour(original, (unsigned short)i, flags[2])) return 0;
    }
    for (i = 0; i < 2048UL; ++i) {
        if (!check_colour(original, (unsigned short)random_word(), random_word())) return 0;
    }
    printf("PASS make_colour cases=%lu exhaustive_intensities=65536 flags=0,4,ffffffff\n", colour_cases);
    return 1;
}

static int test_sentinel(HMODULE module)
{
    OriginalSentinel original;
    short expected_data[130];
    short actual_data[130];
    unsigned long round;
    unsigned long length;
    unsigned long i;
    int expected;
    int actual;
    original = (OriginalSentinel)((unsigned char *)module + 0xcee0);
    for (length = 0; length <= 128; ++length) {
        for (i = 0; i < length; ++i) {
            expected_data[i] = (short)(random_word() & 0xffffUL);
            if (expected_data[i] == -1) expected_data[i] = -2;
        }
        expected_data[length] = -1;
        memcpy(actual_data, expected_data, sizeof(expected_data));
        expected = original(expected_data);
        actual = sentinel_count(actual_data);
        ++sentinel_cases;
        if (expected != actual || memcmp(expected_data, actual_data, sizeof(expected_data)) != 0) {
            printf("DIVERGENCE sentinel_count case=%lu length=%lu original=%ld candidate=%ld\n",
                   sentinel_cases, length, (long)expected, (long)actual);
            return 0;
        }
    }
    for (round = 0; round < 2048UL; ++round) {
        length = random_word() % 129UL;
        for (i = 0; i < length; ++i) {
            expected_data[i] = (short)(random_word() & 0xffffUL);
            if (expected_data[i] == -1) expected_data[i] = 1;
        }
        expected_data[length] = -1;
        memcpy(actual_data, expected_data, sizeof(expected_data));
        expected = original(expected_data);
        actual = sentinel_count(actual_data);
        ++sentinel_cases;
        if (expected != actual || memcmp(expected_data, actual_data, sizeof(expected_data)) != 0) {
            printf("DIVERGENCE sentinel_count case=%lu length=%lu original=%ld candidate=%ld\n",
                   sentinel_cases, length, (long)expected, (long)actual);
            return 0;
        }
    }
    printf("PASS sentinel_count cases=%lu lengths=0..128 plus 2048 deterministic random\n", sentinel_cases);
    return 1;
}

int main(int argc, char **argv)
{
    HMODULE module;
    if (argc != 2) {
        printf("usage: oracle_diff.exe <original-TITLE.DLL>\n");
        return 2;
    }
    module = LoadLibraryA(argv[1]);
    if (module == NULL) {
        printf("HARNESS ERROR: LoadLibraryA failed error=%08lx path=%s\n",
               (unsigned long)GetLastError(), argv[1]);
        return 3;
    }
    if (!test_decimal(module) || !test_colour(module) || !test_sentinel(module)) {
        FreeLibrary(module);
        return 1;
    }
    FreeLibrary(module);
    printf("PASS all selected original-RVA behavioral comparisons\n");
    return 0;
}
