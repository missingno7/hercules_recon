#include "hercules_edge_steps.h"

struct renderer_edge_u64_pair {
    renderer_u32 low;
    renderer_u32 high;
};

static renderer_u32 renderer_edge_abs_i32(renderer_i32 value)
{
    if (value < 0) {
        return (renderer_u32)(-(value + 1)) + (renderer_u32)1u;
    }
    return (renderer_u32)value;
}

static struct renderer_edge_u64_pair renderer_edge_mul_u32(
    renderer_u32 left, renderer_u32 right)
{
    renderer_u32 a0 = left & (renderer_u32)0xffffu;
    renderer_u32 a1 = left >> 16;
    renderer_u32 b0 = right & (renderer_u32)0xffffu;
    renderer_u32 b1 = right >> 16;
    renderer_u32 p00 = a0 * b0;
    renderer_u32 p01 = a0 * b1;
    renderer_u32 p10 = a1 * b0;
    renderer_u32 p11 = a1 * b1;
    renderer_u32 middle = (p00 >> 16) + (p01 & (renderer_u32)0xffffu) +
        (p10 & (renderer_u32)0xffffu);
    struct renderer_edge_u64_pair result;

    result.low = (p00 & (renderer_u32)0xffffu) | (middle << 16);
    result.high = p11 + (p01 >> 16) + (p10 >> 16) + (middle >> 16);
    return result;
}

static struct renderer_edge_u64_pair renderer_edge_negate_pair(
    struct renderer_edge_u64_pair value)
{
    renderer_u32 carry = (value.low == (renderer_u32)0u)
        ? (renderer_u32)1u : (renderer_u32)0u;
    value.low = ~value.low + (renderer_u32)1u;
    value.high = ~value.high + carry;
    return value;
}

enum renderer_edge_extent_status renderer_edge_check_extent(renderer_u32 extent)
{
    if (extent == (renderer_u32)0u) {
        return RENDERER_EDGE_EXTENT_SKIP;
    }
    if (extent >= (renderer_u32)0x800u) {
        return RENDERER_EDGE_EXTENT_REJECT;
    }
    return RENDERER_EDGE_EXTENT_ACCEPT;
}

renderer_u32 renderer_edge_reciprocal_entry(renderer_u32 denominator)
{
    if (denominator == (renderer_u32)0u || denominator >= (renderer_u32)0x800u) {
        return (renderer_u32)0u;
    }
    return (renderer_u32)0x40000000u / denominator;
}

renderer_u32 renderer_edge_shrd30_from_q16_delta(
    renderer_i32 q16_delta, renderer_i32 reciprocal,
    renderer_u32 *product_low, renderer_u32 *product_high)
{
    renderer_u32 delta_magnitude = renderer_edge_abs_i32(q16_delta);
    renderer_u32 reciprocal_magnitude = renderer_edge_abs_i32(reciprocal);
    struct renderer_edge_u64_pair product = renderer_edge_mul_u32(
        delta_magnitude, reciprocal_magnitude);

    if ((q16_delta < 0) != (reciprocal < 0)) {
        product = renderer_edge_negate_pair(product);
    }
    if (product_low != 0) {
        *product_low = product.low;
    }
    if (product_high != 0) {
        *product_high = product.high;
    }
    /* SHRD EAX,EDX,30: low 32 bits of the unsigned pair shifted right 30. */
    return (product.low >> 30) | (product.high << 2);
}

enum renderer_edge_step_status renderer_edge_c2_guarded_idiv_step(
    renderer_i32 delta_x, renderer_i32 delta_y,
    struct renderer_edge_idiv_result *result)
{
    renderer_u32 magnitude;
    renderer_u32 shifted_bits;
    renderer_i32 dividend;
    renderer_u32 dividend_magnitude;
    renderer_u32 quotient_magnitude;
    renderer_u32 remainder_magnitude;
    renderer_i32 quotient;
    renderer_i32 remainder;

    if (delta_y == 0) {
        return RENDERER_EDGE_STEP_SKIP_ZERO_DY;
    }
    if (delta_y < 0 || delta_y >= (renderer_i32)0x800) {
        return RENDERER_EDGE_STEP_REJECT_DOMAIN;
    }
    magnitude = renderer_edge_abs_i32(delta_x);
    if (magnitude >= (renderer_u32)0x800u) {
        return RENDERER_EDGE_STEP_REJECT_DOMAIN;
    }

    /* The accepted |delta_x|<2048 domain keeps the x86 SHL EAX,16 result in
     * signed 32-bit range. Convert the unsigned shift bits without a wide type. */
    shifted_bits = ((renderer_u32)delta_x) << 16;
    dividend = (shifted_bits <= (renderer_u32)0x7fffffffu)
        ? (renderer_i32)shifted_bits
        : -(renderer_i32)(~shifted_bits) - 1;
    dividend_magnitude = renderer_edge_abs_i32(dividend);
    quotient_magnitude = dividend_magnitude / (renderer_u32)delta_y;
    remainder_magnitude = dividend_magnitude % (renderer_u32)delta_y;

    quotient = (renderer_i32)quotient_magnitude;
    remainder = (renderer_i32)remainder_magnitude;
    if (dividend < 0) {
        quotient = -quotient;
        remainder = -remainder;
    }
    if (result != 0) {
        result->quotient = quotient;
        result->remainder = remainder;
    }
    return RENDERER_EDGE_STEP_OK;
}
