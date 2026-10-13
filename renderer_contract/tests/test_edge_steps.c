#include <stdio.h>
#include "hercules_edge_steps.h"

static int failures = 0;
static int checks = 0;

#define CHECK(name, condition) do { \
    int check_passed = (condition); \
    ++checks; \
    if (!check_passed) { \
        ++failures; \
        printf("FAIL %s\n", name); \
    } \
} while (0)

int main(void)
{
    renderer_u32 lo;
    renderer_u32 hi;
    renderer_u32 shifted;
    struct renderer_edge_idiv_result result;

    CHECK("extent zero skips", renderer_edge_check_extent((renderer_u32)0u) == RENDERER_EDGE_EXTENT_SKIP);
    CHECK("extent 2047 accepted", renderer_edge_check_extent((renderer_u32)2047u) == RENDERER_EDGE_EXTENT_ACCEPT);
    CHECK("extent 2048 rejected", renderer_edge_check_extent((renderer_u32)2048u) == RENDERER_EDGE_EXTENT_REJECT);
    CHECK("table R0 sentinel", renderer_edge_reciprocal_entry((renderer_u32)0u) == (renderer_u32)0u);
    CHECK("table R1", renderer_edge_reciprocal_entry((renderer_u32)1u) == (renderer_u32)0x40000000u);
    CHECK("table R2047", renderer_edge_reciprocal_entry((renderer_u32)2047u) == (renderer_u32)0x00080100u);
    CHECK("table does not expose R2048", renderer_edge_reciprocal_entry((renderer_u32)2048u) == (renderer_u32)0u);

    shifted = renderer_edge_shrd30_from_q16_delta((renderer_i32)131072, (renderer_i32)524544, &lo, &hi);
    CHECK("D60C +delta product low", lo == (renderer_u32)0x02000000u);
    CHECK("D60C +delta product high", hi == (renderer_u32)0x00000010u);
    CHECK("D60C +delta SHRD30", shifted == (renderer_u32)0x00000040u);
    shifted = renderer_edge_shrd30_from_q16_delta((renderer_i32)-131072, (renderer_i32)524544, &lo, &hi);
    CHECK("D60C -delta product low", lo == (renderer_u32)0xfe000000u);
    CHECK("D60C -delta product high", hi == (renderer_u32)0xffffffefu);
    CHECK("D60C -delta SHRD30", shifted == (renderer_u32)0xffffffbfu);
    shifted = renderer_edge_shrd30_from_q16_delta((renderer_i32)-134152192, (renderer_i32)536870912, &lo, &hi);
    CHECK("D60C wide product high", hi == (renderer_u32)0xff002000u);
    CHECK("D60C wide SHRD30", shifted == (renderer_u32)0xfc008000u);

    CHECK("C2 zero dy skips IDIV", renderer_edge_c2_guarded_idiv_step(2, 0, &result) == RENDERER_EDGE_STEP_SKIP_ZERO_DY);
    CHECK("C2 dy2047 positive step", renderer_edge_c2_guarded_idiv_step(2, 2047, &result) == RENDERER_EDGE_STEP_OK);
    CHECK("C2 dy2047 positive quotient", result.quotient == 64);
    CHECK("C2 dy2047 positive remainder", result.remainder == 64);
    CHECK("C2 dy2047 negative step", renderer_edge_c2_guarded_idiv_step(-2, 2047, &result) == RENDERER_EDGE_STEP_OK);
    CHECK("C2 dy2047 negative quotient", result.quotient == -64);
    CHECK("C2 dy2047 negative remainder", result.remainder == -64);
    CHECK("C2 extent2048 rejected", renderer_edge_c2_guarded_idiv_step(1, 2048, &result) == RENDERER_EDGE_STEP_REJECT_DOMAIN);
    CHECK("C2 dx2048 rejected", renderer_edge_c2_guarded_idiv_step(2048, 2, &result) == RENDERER_EDGE_STEP_REJECT_DOMAIN);
    CHECK("C2 wide delta in-domain", renderer_edge_c2_guarded_idiv_step(-2047, 2, &result) == RENDERER_EDGE_STEP_OK);
    CHECK("C2 wide delta quotient", result.quotient == -67076096);
    CHECK("C2 wide delta remainder", result.remainder == 0);

    printf("checks=%d failures=%d\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
