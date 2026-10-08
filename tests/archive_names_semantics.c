#include <stdio.h>
#include <string.h>

#include "../calibration/archive_names.c"

/* A synthetic test backing store, not a declaration of target capacity. */
char g_archive_scratch[512];
const char g_archive_prefix_maps[15] = "M:\\GRAFIX\\MAPS";
const char g_archive_prefix_chop[15] = "M:\\GRAFIX\\CHOP";
const char g_archive_prefix_sonix[18] = "M:\\LANGUAGE\\SONIX";

static int checks;
static int failures;

static void check(const char *name, int passed)
{
    ++checks;
    if (!passed) {
        ++failures;
        printf("FAIL: %s\n", name);
    }
}

static char *reference_normalize(char *scratch, const char *input)
{
    static const char maps[] = "M:\\GRAFIX\\MAPS";
    static const char chop[] = "M:\\GRAFIX\\CHOP";
    static const char sonix[] = "M:\\LANGUAGE\\SONIX";
    const unsigned char *source;
    char *destination;
    const char *prefix;
    unsigned char value;
    int start;
    int position;
    int copy_length;
    int i;

    if (input == NULL || *input == '\0')
        return NULL;

    source = (const unsigned char *)input;
    destination = &scratch[100];
    for (;;) {
        value = *source;
        if (value >= (unsigned char)'a' && value <= (unsigned char)'z')
            value = (unsigned char)(value - (unsigned char)('a' - 'A'));
        *destination = (char)value;
        ++destination;
        ++source;
        if (*source == 0) {
            *destination = '\0';
            break;
        }
    }

    start = 100;
    if (scratch[101] != ':') {
        if (scratch[100] != '\\') {
            scratch[99] = '\\';
            start = 99;
        }
        --start;
        scratch[start] = ':';
        --start;
        scratch[start] = 'M';
    }

    switch ((signed char)scratch[start]) {
    case 'O':
        start -= 12;
        prefix = maps;
        copy_length = 15;
        break;
    case 'P':
        start -= 12;
        prefix = chop;
        copy_length = 15;
        break;
    case 'Q':
    case 'R':
        start -= 15;
        prefix = sonix;
        copy_length = 18;
        break;
    case 'N':
    default:
        position = start + 3;
        while (scratch[position] != '\\' && scratch[position] != '\0')
            ++position;
        --position;
        scratch[position] = ':';
        scratch[position - 1] = 'M';
        return &scratch[position - 1];
    case 'M':
        return &scratch[start];
    }

    for (i = 0; i < copy_length; ++i)
        scratch[start + i] = prefix[i];
    scratch[start + copy_length - 1] = '\\';
    return &scratch[start];
}

static unsigned long reference_hash(const char *input)
{
    const signed char *cursor;
    unsigned long sum;
    unsigned long length;
    unsigned long value;
    int shift;

    cursor = (const signed char *)input;
    sum = 0UL;
    length = 0UL;
    shift = 0;
    while (*cursor != 0) {
        value = (unsigned long)(long)*cursor;
        sum += value << shift;
        shift += 8;
        if (shift > 24)
            shift = 0;
        ++length;
        ++cursor;
    }
    return sum + length;
}

static void path_case(const char *name, const char *input,
                      const char *expected_text, int expected_offset)
{
    char expected_scratch[512];
    char *expected_result;
    char *actual_result;

    memset(g_archive_scratch, 0, sizeof(g_archive_scratch));
    memset(expected_scratch, 0, sizeof(expected_scratch));
    expected_result = reference_normalize(expected_scratch, input);
    actual_result = archive_normalize_name(input);
    check(name, actual_result != NULL && expected_result != NULL &&
          actual_result == g_archive_scratch + expected_offset &&
          expected_result == expected_scratch + expected_offset &&
          strcmp(actual_result, expected_text) == 0 &&
          strcmp(expected_result, expected_text) == 0 &&
          memcmp(g_archive_scratch, expected_scratch,
                 sizeof(g_archive_scratch)) == 0);
}

static void hash_case(const char *name, const char *input,
                      unsigned long expected)
{
    unsigned long actual;
    unsigned long modeled;

    actual = archive_name_hash(input);
    modeled = reference_hash(input);
    check(name, actual == expected && modeled == expected);
}

int main(void)
{
    char no_bytes[1];
    char high_80[2];
    char high_ff[2];
    char ff_01[3];
    char wrapped[13];
    char expected_scratch[512];
    char alias_expected[512];
    char *actual_result;
    char *expected_result;
    char *first_result;
    char *second_result;
    int i;

    for (i = 0; i < (int)sizeof(g_archive_scratch); ++i)
        g_archive_scratch[i] = (char)(i * 37 + 11);
    memcpy(expected_scratch, g_archive_scratch, sizeof(expected_scratch));
    actual_result = archive_normalize_name(NULL);
    check("null input returns NULL without scratch writes",
          actual_result == NULL &&
          memcmp(g_archive_scratch, expected_scratch,
                 sizeof(g_archive_scratch)) == 0);

    no_bytes[0] = '\0';
    actual_result = archive_normalize_name(no_bytes);
    check("empty input returns NULL without scratch writes",
          actual_result == NULL &&
          memcmp(g_archive_scratch, expected_scratch,
                 sizeof(g_archive_scratch)) == 0);

    path_case("driveless lowercase path gains M prefix", "foo", "M:\\FOO", 97);
    path_case("leading backslash keeps its source offset", "\\foo", "M:\\FOO", 98);
    path_case("M drive uppercases ASCII and retains dot segments",
              "m:\\foo\\..\\bar", "M:\\FOO\\..\\BAR", 100);
    path_case("O drive maps to GRAFIX MAPS", "O:\\foo",
              "M:\\GRAFIX\\MAPS\\FOO", 88);
    path_case("P drive maps to GRAFIX CHOP", "P:\\foo",
              "M:\\GRAFIX\\CHOP\\FOO", 88);
    path_case("Q drive maps to LANGUAGE SONIX", "Q:\\foo",
              "M:\\LANGUAGE\\SONIX\\FOO", 85);
    path_case("R drive shares LANGUAGE SONIX mapping", "R:\\foo",
              "M:\\LANGUAGE\\SONIX\\FOO", 85);
    path_case("N drive drops first directory component", "N:\\foo\\bar",
              "M:\\BAR", 104);
    path_case("other drive drops first directory component", "C:\\foo\\bar",
              "M:\\BAR", 104);
    path_case("drive with one directory component returns M colon",
              "C:\\foo", "M:", 104);
    path_case("forward slash is not a separator", "foo/bar",
              "M:\\FOO/BAR", 97);
    path_case("O colon without slash retains old bytes after copied prefix",
              "O:foo", "M:\\GRAFIX\\MAPS\\OO", 88);
    path_case("M colon returns unchanged drive prefix", "M:", "M:", 100);
    path_case("short C colon scans zeroed scratch tail", "C:", "M:", 101);

    memset(g_archive_scratch, 0, sizeof(g_archive_scratch));
    memset(alias_expected, 0, sizeof(alias_expected));
    strcpy(g_archive_scratch + 180, "r:\\dir\\leaf");
    strcpy(alias_expected + 180, "r:\\dir\\leaf");
    expected_result = reference_normalize(alias_expected, alias_expected + 180);
    actual_result = archive_normalize_name(g_archive_scratch + 180);
    check("input and result share the one scratch allocation",
          actual_result == g_archive_scratch + 85 &&
          expected_result == alias_expected + 85 &&
          strcmp(actual_result, "M:\\LANGUAGE\\SONIX\\DIR\\LEAF") == 0 &&
          memcmp(g_archive_scratch, alias_expected,
                 sizeof(g_archive_scratch)) == 0);

    memset(g_archive_scratch, 0, sizeof(g_archive_scratch));
    memset(expected_scratch, 0, sizeof(expected_scratch));
    expected_result = reference_normalize(expected_scratch, "foo");
    first_result = archive_normalize_name("foo");
    if (first_result != NULL)
        first_result[0] = 'Z';
    if (expected_result != NULL)
        expected_result[0] = 'Z';
    expected_result = reference_normalize(expected_scratch, "m:\\x");
    second_result = archive_normalize_name("m:\\x");
    check("returned pointer aliases scratch and future calls share it",
          first_result == g_archive_scratch + 97 &&
          second_result == g_archive_scratch + 100 &&
          expected_result == expected_scratch + 100 &&
          g_archive_scratch[97] == 'Z' &&
          memcmp(g_archive_scratch, expected_scratch,
                 sizeof(g_archive_scratch)) == 0);

    memset(g_archive_scratch, 0, sizeof(g_archive_scratch));
    memset(expected_scratch, 0, sizeof(expected_scratch));
    expected_result = reference_normalize(expected_scratch, "M:\\ABCDEFGHIJK");
    actual_result = archive_normalize_name("M:\\ABCDEFGHIJK");
    check("long M-drive path establishes stale tail", actual_result != NULL &&
          actual_result == g_archive_scratch + 100 &&
          expected_result == expected_scratch + 100 &&
          memcmp(g_archive_scratch, expected_scratch,
                 sizeof(g_archive_scratch)) == 0);
    expected_result = reference_normalize(expected_scratch, "C:");
    actual_result = archive_normalize_name("C:");
    check("short C colon reuses prior stale tail and offset",
          actual_result == g_archive_scratch + 112 &&
          expected_result == expected_scratch + 112 &&
          strcmp(actual_result, "M:") == 0 &&
          memcmp(g_archive_scratch, expected_scratch,
                 sizeof(g_archive_scratch)) == 0);

    no_bytes[0] = '\0';
    high_80[0] = (char)0x80;
    high_80[1] = '\0';
    high_ff[0] = (char)0xff;
    high_ff[1] = '\0';
    ff_01[0] = (char)0xff;
    ff_01[1] = 1;
    ff_01[2] = '\0';
    for (i = 0; i < 12; ++i)
        wrapped[i] = 0x7f;
    wrapped[12] = '\0';

    hash_case("empty hash is zero", no_bytes, 0x00000000UL);
    hash_case("single ASCII byte includes length", "A", 0x00000042UL);
    hash_case("four-lane hash cycles after byte four", "ABCDE", 0x4443428bUL);
    hash_case("0x80 is sign extended", high_80, 0xffffff81UL);
    hash_case("0xff plus length wraps to zero", high_ff, 0x00000000UL);
    hash_case("signed 0xff then lane-one byte", ff_01, 0x00000101UL);
    hash_case("repeated positive bytes wrap the 32-bit sum", wrapped,
              0x7e7e7e89UL);

    printf("archive names fixture: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
