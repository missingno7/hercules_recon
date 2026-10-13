/* Name avoids Windows legacy installer detection of executable names
 * containing "patch" (including "dispatch"). No elevated runtime needed. */
#include <stdio.h>
#include "hercules_quad_dispatch.h"

int main(void)
{
    const renderer_u32 x[4] = { 1U, 3U, 3U, 1U };
    const renderer_u32 y[4] = { 1U, 1U, 3U, 3U };
    const renderer_u32 u[4] = { 0U, 0x00020000U, 0x00020000U, 0U };
    const renderer_u32 v[4] = { 0U, 0U, 0x00020000U, 0x00020000U };
    renderer_u32 bad_u[4] = { 0U, 0x00010000U, 0x00020000U, 0U };
    const renderer_u32 too_wide_x[4] = { 1U, 0x00008001U, 0x00008001U, 1U };

    if (!renderer_quad_direct_shape_eligible(4U, x, y, u, v))
        return 1;
    if (!renderer_d420_cab0_candidate(1U, 0x10001000U, 4U, x, y, u, v))
        return 2;
    if (!renderer_e680_c8f0_candidate(1U, 0x10001000U, 0x80U, 4U, x, y, u, v))
        return 3;
    if (renderer_quad_direct_shape_eligible(3U, x, y, u, v))
        return 4;
    if (renderer_quad_direct_shape_eligible(4U, x, y, bad_u, v))
        return 5;
    if (renderer_d420_cab0_candidate(0U, 0x10001000U, 4U, x, y, u, v))
        return 6;
    if (renderer_d420_cab0_candidate(1U, 0U, 4U, x, y, u, v))
        return 7;
    if (renderer_e680_c8f0_candidate(1U, 0x10001000U, 0x40U, 4U, x, y, u, v))
        return 8;
    if (renderer_quad_direct_shape_eligible(4U, too_wide_x, y, u, v))
        return 9;

    puts("quad dispatch contract cases passed");
    return 0;
}
