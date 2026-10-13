#include "hercules_projection.h"

static renderer_i32 hercules_projection_signed32(renderer_u32 raw)
{
    if (raw <= 0x7fffffffUL)
        return (renderer_i32)raw;
    return (renderer_i32)(-1L - (renderer_i32)(~raw));
}

static renderer_i32 hercules_projection_signed16(renderer_u16 raw)
{
    if (raw <= 0x7fffU)
        return (renderer_i32)raw;
    return (renderer_i32)(-1L - (renderer_i32)(0xffffU - raw));
}

static renderer_i32 hercules_projection_sar32(renderer_u32 raw,
                                               unsigned int count)
{
    renderer_u32 shifted;

    if (count == 0U)
        return hercules_projection_signed32(raw);
    shifted = raw >> count;
    if ((raw & 0x80000000UL) != 0UL)
        shifted |= 0xffffffffUL << (32U - count);
    return hercules_projection_signed32(shifted);
}

static renderer_u32 hercules_projection_product(renderer_i32 left,
                                                 renderer_i32 right)
{
    return ((renderer_u32)left) * ((renderer_u32)right);
}

static renderer_i32 hercules_projection_row(
    const struct hercules_projection_config_view *config,
    const struct hercules_projection_point_view *point,
    unsigned int row)
{
    renderer_u32 accumulator;
    renderer_u32 product;
    renderer_i32 scaled;
    renderer_u32 translated;
    unsigned int column;

    accumulator = 0UL;
    for (column = 0U; column != 3U; ++column) {
        product = hercules_projection_product(
            (renderer_i32)config->matrix_row_major[row * 3U + column],
            (renderer_i32)point->xyz[column]);
        accumulator += product;
    }
    scaled = hercules_projection_sar32(accumulator, 12U);
    translated = ((renderer_u32)scaled) +
                 ((renderer_u32)config->translation_xyz[row]);
    return hercules_projection_signed16((renderer_u16)translated);
}

/* Returns nonzero only for the x86 signed IDIV overflow condition. Division
 * is performed on unsigned magnitudes so INT_MIN/-1 never invokes C UB. */
static int hercules_projection_batch_divide(
    renderer_i32 numerator, renderer_i32 denominator,
    renderer_u32 *quotient_raw)
{
    renderer_u32 numerator_raw;
    renderer_u32 denominator_raw;
    renderer_u32 numerator_magnitude;
    renderer_u32 denominator_magnitude;
    renderer_u32 quotient_magnitude;
    int numerator_negative;
    int denominator_negative;

    numerator_raw = (renderer_u32)numerator;
    denominator_raw = (renderer_u32)denominator;
    numerator_negative = (numerator_raw & 0x80000000UL) != 0UL;
    denominator_negative = (denominator_raw & 0x80000000UL) != 0UL;

    if (numerator_raw == 0x80000000UL && denominator_raw == 0xffffffffUL)
        return 1;

    numerator_magnitude = numerator_negative ? 0UL - numerator_raw : numerator_raw;
    denominator_magnitude = denominator_negative ? 0UL - denominator_raw : denominator_raw;
    if (denominator_magnitude == 0UL)
        return 1;
    quotient_magnitude = numerator_magnitude / denominator_magnitude;
    if (numerator_negative != denominator_negative)
        *quotient_raw = 0UL - quotient_magnitude;
    else
        *quotient_raw = quotient_magnitude;
    return 0;
}

renderer_i32 hercules_projection_batch_eval(
    const struct hercules_projection_config_view *config,
    const struct hercules_projection_point_view points[
        HERCULES_PROJECTION_BATCH_POINT_COUNT],
    struct hercules_projection_batch_result_view *result,
    renderer_u32 *fault_point,
    renderer_raw32 *raw_eax)
{
    renderer_u32 factor_raw;
    renderer_u32 quotient_raw;
    renderer_i32 factor;
    renderer_i32 camera_x;
    renderer_i32 camera_y;
    renderer_i32 camera_z;
    renderer_u32 product_raw;
    renderer_i32 scaled;
    renderer_u32 sum_raw;
    renderer_u32 last_y_sum;
    unsigned int point_index;

    if (config == 0 || points == 0 || result == 0) {
        if (fault_point != 0)
            *fault_point = 0xffffffffUL;
        return HERCULES_PROJECTION_BATCH_BAD_ARGUMENT;
    }

    factor_raw = config->focal << 16;
    factor = hercules_projection_signed32(factor_raw);
    last_y_sum = 0UL;

    for (point_index = 0U;
         point_index != HERCULES_PROJECTION_BATCH_POINT_COUNT;
         ++point_index) {
        camera_x = hercules_projection_row(config, &points[point_index], 0U);
        result->camera_xyz_bits[point_index * 3U] = (renderer_u32)camera_x;
        camera_y = hercules_projection_row(config, &points[point_index], 1U);
        result->camera_xyz_bits[point_index * 3U + 1U] = (renderer_u32)camera_y;
        camera_z = hercules_projection_row(config, &points[point_index], 2U);
        result->camera_xyz_bits[point_index * 3U + 2U] = (renderer_u32)camera_z;

        quotient_raw = factor_raw;
        if (camera_z != 0L &&
            hercules_projection_batch_divide(factor, camera_z, &quotient_raw)) {
            if (fault_point != 0)
                *fault_point = (renderer_u32)point_index;
            return HERCULES_PROJECTION_BATCH_IDIV_OVERFLOW;
        }

        product_raw = quotient_raw * (renderer_u32)camera_x;
        scaled = hercules_projection_sar32(product_raw, 16U);
        sum_raw = (renderer_u32)scaled + (renderer_u32)config->center_x;
        result->projected_xy[point_index * 2U] = (renderer_u16)sum_raw;

        product_raw = quotient_raw * (renderer_u32)camera_y;
        scaled = hercules_projection_sar32(product_raw, 16U);
        sum_raw = (renderer_u32)scaled + (renderer_u32)config->center_y;
        result->projected_xy[point_index * 2U + 1U] = (renderer_u16)sum_raw;
        last_y_sum = sum_raw;
    }

    if (fault_point != 0)
        *fault_point = HERCULES_PROJECTION_BATCH_POINT_COUNT;
    if (raw_eax != 0)
        *raw_eax = last_y_sum;
    return HERCULES_PROJECTION_BATCH_OK;
}

static renderer_i32 hercules_projection_divide_trunc_zero(
    renderer_i32 numerator, renderer_i32 denominator)
{
    renderer_u32 numerator_raw;
    renderer_u32 magnitude;
    renderer_u32 denominator_raw;
    renderer_u32 quotient;

    numerator_raw = (renderer_u32)numerator;
    denominator_raw = (renderer_u32)denominator;
    if ((numerator_raw & 0x80000000UL) != 0UL) {
        magnitude = 0UL - numerator_raw;
        quotient = magnitude / denominator_raw;
        return hercules_projection_signed32(0UL - quotient);
    }
    quotient = numerator_raw / denominator_raw;
    return hercules_projection_signed32(quotient);
}

void hercules_projection_eval(
    const struct hercules_projection_config_view *config,
    const struct hercules_projection_point_view *point,
    struct hercules_projection_result_view *result)
{
    renderer_u32 numerator_raw;
    renderer_i32 numerator;
    renderer_i32 quotient;
    renderer_u32 x_product;
    renderer_u32 y_product;
    renderer_i32 x_scaled;
    renderer_i32 y_scaled;
    renderer_u32 x_sum;
    renderer_u32 y_sum;

    result->camera_x = hercules_projection_row(config, point, 0U);
    result->camera_y = hercules_projection_row(config, point, 1U);
    result->camera_z = hercules_projection_row(config, point, 2U);
    if (result->camera_z < 1L)
        result->camera_z = 1L;

    numerator_raw = config->focal << 16;
    numerator = hercules_projection_signed32(numerator_raw);
    quotient = hercules_projection_divide_trunc_zero(
        numerator, result->camera_z);
    x_product = hercules_projection_product(quotient, result->camera_x);
    y_product = hercules_projection_product(quotient, result->camera_y);
    x_scaled = hercules_projection_sar32(x_product, 16U);
    y_scaled = hercules_projection_sar32(y_product, 16U);
    x_sum = ((renderer_u32)x_scaled) + ((renderer_u32)config->center_x);
    y_sum = ((renderer_u32)y_scaled) + ((renderer_u32)config->center_y);

    result->projected_x = (renderer_u16)x_sum;
    result->projected_y = (renderer_u16)y_sum;
    result->raw_eax = (renderer_raw32)y_sum;
}
