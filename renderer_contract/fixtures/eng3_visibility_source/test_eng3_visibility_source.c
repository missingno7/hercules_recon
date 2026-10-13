#include <stdio.h>
#include <string.h>

typedef unsigned int U32;
typedef unsigned short U16;
typedef signed int S32;
typedef struct Eng3VisibilityRow {
    U32 selector_bits;
    U32 lower_bits;
    U32 upper_bits;
} Eng3VisibilityRow;

S32 eng3_indexed_coordinate_from_object_x(U32 object_x_bits);
int eng3_indexed_interval_contains(const Eng3VisibilityRow *rows,
                                   U16 index,
                                   S32 coordinate);
void eng3_write_interval_endpoints(Eng3VisibilityRow *rows,
                                   U32 row_index,
                                   U32 lower_bits,
                                   U32 extent_bits);

static int check_bytes(const unsigned char *actual,
                       const unsigned char *expected,
                       unsigned int count)
{
    unsigned int i;
    for (i = 0; i < count; ++i) {
        if (actual[i] != expected[i])
            return 0;
    }
    return 1;
}

static int check_condition(int condition, const char *label)
{
    if (!condition) {
        printf("FAIL %s\n", label);
        return 0;
    }
    printf("PASS %s\n", label);
    return 1;
}

#define CHECK(condition, label) \
    if (!check_condition((condition), (label))) return 1

int main(void)
{
    Eng3VisibilityRow row;
    unsigned char expected_row[12] = {
        0x00, 0x00, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0x00,
        0x14, 0x00, 0x00, 0x00
    };
    unsigned char expected_wrap_endpoints[8] = {
        0xf8, 0xff, 0xff, 0x7f,
        0x08, 0x00, 0x00, 0x80
    };
    U32 object_x;

    memset(&row, 0, sizeof(row));
    eng3_write_interval_endpoints(&row, 0, 10U, 10U);
    CHECK(check_bytes((const unsigned char *)&row, expected_row, 12),
          "row bytes for [10,20) preserve controlled selector zero");
    CHECK(eng3_indexed_interval_contains(&row, 0, 9) == 0,
          "coordinate 9 rejects");
    CHECK(eng3_indexed_interval_contains(&row, 0, 10) == 1,
          "coordinate 10 accepts");
    CHECK(eng3_indexed_interval_contains(&row, 0, 19) == 1,
          "coordinate 19 accepts");
    CHECK(eng3_indexed_interval_contains(&row, 0, 20) == 0,
          "coordinate 20 rejects");

    object_x = ((U32)9U * 128U) << 16;
    CHECK(eng3_indexed_coordinate_from_object_x(object_x) == 9 &&
          eng3_indexed_interval_contains(&row, 0,
              eng3_indexed_coordinate_from_object_x(object_x)) == 0,
          "encoded object coordinate 9 decodes then rejects");
    object_x = ((U32)10U * 128U) << 16;
    CHECK(eng3_indexed_coordinate_from_object_x(object_x) == 10 &&
          eng3_indexed_interval_contains(&row, 0,
              eng3_indexed_coordinate_from_object_x(object_x)) == 1,
          "encoded object coordinate 10 decodes then accepts");
    object_x = ((U32)19U * 128U) << 16;
    CHECK(eng3_indexed_coordinate_from_object_x(object_x) == 19 &&
          eng3_indexed_interval_contains(&row, 0,
              eng3_indexed_coordinate_from_object_x(object_x)) == 1,
          "encoded object coordinate 19 decodes then accepts");
    object_x = ((U32)20U * 128U) << 16;
    CHECK(eng3_indexed_coordinate_from_object_x(object_x) == 20 &&
          eng3_indexed_interval_contains(&row, 0,
              eng3_indexed_coordinate_from_object_x(object_x)) == 0,
          "encoded object coordinate 20 decodes then rejects");

    row.selector_bits = 0x12345678U;
    eng3_write_interval_endpoints(&row, 0, 0x7ffffff8U, 0x10U);
    CHECK(row.selector_bits == 0x12345678U,
          "endpoint writer leaves selector DWORD unchanged");
    CHECK(check_bytes(((const unsigned char *)&row) + 4,
                      expected_wrap_endpoints, 8),
          "endpoint addition wraps modulo 2^32 with exact bytes");
    CHECK(eng3_indexed_interval_contains(&row, 0, -2147483647L - 1L) == 0,
          "wrapped high endpoint is interpreted as signed negative");

    object_x = ((U32)0xff80U) << 16;
    CHECK(eng3_indexed_coordinate_from_object_x(object_x) == -1 &&
          eng3_indexed_interval_contains(&row, 0,
              eng3_indexed_coordinate_from_object_x(object_x)) == 0,
          "signed high WORD 0xff80 decodes to -1 then rejects");
    object_x = ((U32)0x7f80U) << 16;
    CHECK(eng3_indexed_coordinate_from_object_x(object_x) == 255,
          "positive high WORD SAR 7 maps 0x7f80 to 255");

    printf("C_SOURCE_FIXTURE_OK\n");
    return 0;
}
