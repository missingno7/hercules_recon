#ifndef D420_ROWZERO_QUAD_DISPATCH_CONTRACT_H
#define D420_ROWZERO_QUAD_DISPATCH_CONTRACT_H

/* Pure contract mirror for the PC's two rectangular dispatch checks.
 * Inputs are raw four-lane DWORDs, not host pointers or a renderer ABI. */
#include "hercules_types.h"

static int renderer_i32_bits_le(renderer_u32 left, renderer_u32 right)
{
    /* Bias the sign bit to map two's-complement order onto unsigned order. */
    return (left ^ 0x80000000U) <= (right ^ 0x80000000U);
}

static renderer_u32 renderer_sar16_bits(renderer_u32 value)
{
    renderer_u32 result;
    result = value >> 16;
    if ((value & 0x80000000U) != 0)
        result |= 0xffff0000U;
    return result;
}

static renderer_u32 renderer_sign_extend_low16(renderer_u32 value)
{
    renderer_u32 result;
    result = value & 0x0000ffffU;
    if ((result & 0x00008000U) != 0)
        result |= 0xffff0000U;
    return result;
}

static int renderer_quad_direct_shape_eligible(
    renderer_u32 vertex_count,
    const renderer_u32 x[4], const renderer_u32 y[4],
    const renderer_u32 u[4], const renderer_u32 v[4])
{
    renderer_u32 dx_bits;
    renderer_u32 dy_bits;
    renderer_u32 du_bits;
    renderer_u32 dv_bits;

    if (vertex_count != 4 || x == 0 || y == 0 || u == 0 || v == 0)
        return 0;

    /* Match the signed JG branches and exact corner equalities in D420 and E680. */
    if (!renderer_i32_bits_le(x[0], x[2]) ||
        !renderer_i32_bits_le(y[0], y[2]) ||
        y[0] != y[1] || y[2] != y[3] ||
        x[0] != x[3] || x[1] != x[2])
        return 0;

    if (!renderer_i32_bits_le(u[0], u[2]) ||
        !renderer_i32_bits_le(v[0], v[2]) ||
        v[0] != v[1] || v[2] != v[3] ||
        u[0] != u[3] || u[1] != u[2])
        return 0;

    /* Unsigned subtraction preserves the x86 modulo-2^32 SUB result. */
    dx_bits = x[1] - x[0];
    dy_bits = y[3] - y[0];
    du_bits = u[2] - u[0];
    dv_bits = v[2] - v[0];
    return renderer_sar16_bits(du_bits) == renderer_sign_extend_low16(dx_bits) &&
           renderer_sar16_bits(dv_bits) == renderer_sign_extend_low16(dy_bits);
}

static int renderer_d420_cab0_candidate(
    renderer_u32 available, renderer_raw32 surface,
    renderer_u32 vertex_count,
    const renderer_u32 x[4], const renderer_u32 y[4],
    const renderer_u32 u[4], const renderer_u32 v[4])
{
    return available != 0 && surface != 0 &&
           renderer_quad_direct_shape_eligible(vertex_count, x, y, u, v);
}

static int renderer_e680_c8f0_candidate(
    renderer_u32 available, renderer_raw32 surface, renderer_u32 mode,
    renderer_u32 vertex_count,
    const renderer_u32 x[4], const renderer_u32 y[4],
    const renderer_u32 u[4], const renderer_u32 v[4])
{
    return available != 0 && surface != 0 && mode == 0x80 &&
           renderer_quad_direct_shape_eligible(vertex_count, x, y, u, v);
}

#endif
