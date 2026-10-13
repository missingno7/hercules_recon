#include "hercules_route_uv_models.h"

#define ROUTE_FLAG_CF ((renderer_u32)0x00000001UL)
#define ROUTE_FLAG_PF ((renderer_u32)0x00000004UL)
#define ROUTE_FLAG_AF ((renderer_u32)0x00000010UL)
#define ROUTE_FLAG_ZF ((renderer_u32)0x00000040UL)
#define ROUTE_FLAG_SF ((renderer_u32)0x00000080UL)
#define ROUTE_FLAG_OF ((renderer_u32)0x00000800UL)
#define ROUTE_STATUS_FLAGS (ROUTE_FLAG_CF | ROUTE_FLAG_PF | ROUTE_FLAG_AF | \
                            ROUTE_FLAG_ZF | ROUTE_FLAG_SF | ROUTE_FLAG_OF)
#define ROUTE_LOOP_CC50 ((renderer_u32)0x0040D322UL)
#define ROUTE_LOOP_80 ((renderer_u32)0x0040EE6DUL)
#define ROUTE_LOOP_SHADE ((renderer_u32)0x0040E56CUL)
#define ROUTE_LOOP_EXIT_CC50 ((renderer_u32)0x0040D348UL)
#define ROUTE_LOOP_EXIT_80 ((renderer_u32)0x0040EE93UL)
#define ROUTE_LOOP_EXIT_SHADE ((renderer_u32)0x0040E5A8UL)

static renderer_u32 renderer_route_replace_byte(renderer_u32 value,
                                                 renderer_u32 shift,
                                                 renderer_u8 byte_value)
{
    renderer_u32 mask;
    mask = (renderer_u32)0xffUL << shift;
    return (value & ~mask) | ((renderer_u32)byte_value << shift);
}

static renderer_u8 renderer_route_parity_even(renderer_u8 value)
{
    renderer_u32 bits;
    renderer_u32 count;
    bits = (renderer_u32)value;
    count = (renderer_u32)0u;
    while (bits != (renderer_u32)0u) {
        count += bits & (renderer_u32)1u;
        bits >>= 1;
    }
    return (renderer_u8)((count & (renderer_u32)1u) == (renderer_u32)0u);
}

static renderer_u32 renderer_route_arithmetic_flags(renderer_u32 old_flags,
                                                      renderer_u32 result,
                                                      renderer_u32 width_mask,
                                                      renderer_u32 sign_mask,
                                                      renderer_u32 a,
                                                      renderer_u32 b,
                                                      renderer_u32 carry_in,
                                                      renderer_u32 carry_out)
{
    renderer_u32 flags;
    renderer_u32 low_sum;
    renderer_u32 sign_a;
    renderer_u32 sign_b;
    renderer_u32 sign_r;
    flags = old_flags & ~ROUTE_STATUS_FLAGS;
    low_sum = (a & (renderer_u32)0x0fu) +
              (b & (renderer_u32)0x0fu) + carry_in;
    if (carry_out != (renderer_u32)0u)
        flags |= ROUTE_FLAG_CF;
    if (renderer_route_parity_even((renderer_u8)result) != (renderer_u8)0u)
        flags |= ROUTE_FLAG_PF;
    if (low_sum > (renderer_u32)0x0fu)
        flags |= ROUTE_FLAG_AF;
    if ((result & width_mask) == (renderer_u32)0u)
        flags |= ROUTE_FLAG_ZF;
    if ((result & sign_mask) != (renderer_u32)0u)
        flags |= ROUTE_FLAG_SF;
    sign_a = a & sign_mask;
    sign_b = b & sign_mask;
    sign_r = result & sign_mask;
    if ((sign_a == sign_b) && (sign_a != sign_r))
        flags |= ROUTE_FLAG_OF;
    return flags;
}

static renderer_u32 renderer_route_add32(renderer_u32 a,
                                          renderer_u32 b,
                                          renderer_u32 carry_in,
                                          renderer_u32 old_flags,
                                          renderer_u32 *result,
                                          renderer_u8 *carry_out)
{
    renderer_u32 first;
    renderer_u32 second;
    renderer_u32 carry_first;
    renderer_u32 carry_second;
    renderer_u32 carry;
    first = a + b;
    carry_first = (renderer_u32)(first < a);
    second = first + carry_in;
    carry_second = (renderer_u32)(second < first);
    carry = carry_first | carry_second;
    *result = second;
    *carry_out = (renderer_u8)carry;
    return renderer_route_arithmetic_flags(old_flags, second,
                                             (renderer_u32)0xffffffffUL,
                                             (renderer_u32)0x80000000UL,
                                             a, b, carry_in, carry);
}

static renderer_u32 renderer_route_add8(renderer_u8 a,
                                         renderer_u8 b,
                                         renderer_u32 carry_in,
                                         renderer_u32 old_flags,
                                         renderer_u8 *result,
                                         renderer_u8 *carry_out)
{
    renderer_u32 sum;
    renderer_u8 byte_result;
    renderer_u32 carry;
    sum = (renderer_u32)a + (renderer_u32)b + carry_in;
    byte_result = (renderer_u8)sum;
    carry = (sum > (renderer_u32)0xffu);
    *result = byte_result;
    *carry_out = (renderer_u8)carry;
    return renderer_route_arithmetic_flags(old_flags,
                                             (renderer_u32)byte_result,
                                             (renderer_u32)0xffu,
                                             (renderer_u32)0x80u,
                                             (renderer_u32)a,
                                             (renderer_u32)b,
                                             carry_in, carry);
}

static renderer_u32 renderer_route_dec16_flags(renderer_u32 old_flags,
                                                renderer_u16 old_value)
{
    renderer_u32 flags;
    renderer_u16 result;
    flags = old_flags & ~ROUTE_STATUS_FLAGS;
    if ((old_flags & ROUTE_FLAG_CF) != (renderer_u32)0u)
        flags |= ROUTE_FLAG_CF;
    result = (renderer_u16)(old_value - (renderer_u16)1u);
    if (renderer_route_parity_even((renderer_u8)result) != (renderer_u8)0u)
        flags |= ROUTE_FLAG_PF;
    if ((old_value & (renderer_u16)0x0fu) == (renderer_u16)0u)
        flags |= ROUTE_FLAG_AF;
    if (result == (renderer_u16)0u)
        flags |= ROUTE_FLAG_ZF;
    if ((result & (renderer_u16)0x8000u) != (renderer_u16)0u)
        flags |= ROUTE_FLAG_SF;
    if (old_value == (renderer_u16)0x8000u)
        flags |= ROUTE_FLAG_OF;
    return flags;
}

static renderer_u32 renderer_route_initial_count(const RendererRouteUvRegisters *raw)
{
    return raw->eax & (renderer_u32)0xffffu;
}

static void renderer_route_finish(RendererRouteUvRegisters *current,
                                  RendererRouteUvStep *step,
                                  renderer_u32 continue_pc,
                                  renderer_u32 exit_pc)
{
    renderer_u16 count;
    renderer_u16 remaining;
    count = (renderer_u16)(current->eax & (renderer_u32)0xffffu);
    remaining = (renderer_u16)(count - (renderer_u16)1u);
    current->eax = (current->eax & (renderer_u32)0xffff0000UL) |
                   (renderer_u32)remaining;
    current->edi += (renderer_u32)1u;
    /* TEST or AND immediately before the output path clears CF. INC EDI
     * preserves that bit; DEC AX then replaces the remaining arithmetic flags. */
    current->eflags = renderer_route_dec16_flags(
        current->eflags & ~ROUTE_FLAG_CF, count);
    step->next = *current;
    step->loop_continues = (renderer_u8)(remaining != (renderer_u16)0u &&
                                          remaining < (renderer_u16)0x8000u);
    step->next_pc = step->loop_continues != (renderer_u8)0u ?
                    continue_pc : exit_pc;
}

static enum RendererRouteUvStatus renderer_route_validate(
    const RendererRouteUvRegisters *raw,
    const void *input,
    RendererRouteUvStep *step)
{
    renderer_u32 count;
    if (raw == 0 || input == 0 || step == 0)
        return RENDERER_ROUTE_UV_INVALID_ARGUMENT;
    count = renderer_route_initial_count(raw);
    if (count == (renderer_u32)0u || count > (renderer_u32)320u)
        return RENDERER_ROUTE_UV_INVALID_COUNT;
    return RENDERER_ROUTE_UV_OK;
}

enum RendererRouteUvStatus renderer_cc50_uv_step(
    const RendererRouteUvRegisters *raw,
    const RendererCc50UvInput *input,
    RendererRouteUvStep *step)
{
    RendererRouteUvRegisters current;
    renderer_u32 added;
    renderer_u32 flags;
    renderer_u8 carry_eax;
    renderer_u8 carry_ebx;
    renderer_u8 ignored_carry;
    renderer_u8 ch_before;
    renderer_u8 ch_after;
    enum RendererRouteUvStatus status;

    status = renderer_route_validate(raw, input, step);
    if (status != RENDERER_ROUTE_UV_OK)
        return status;
    current = *raw;
    step->source_byte = input->source_byte;
    step->destination_address = raw->edi;
    step->palette_address = (renderer_u32)0u;
    step->shade_address = (renderer_u32)0u;
    step->blend_address = (renderer_u32)0u;

    /* D322 loads DL from the incoming BL before either packed addition. */
    current.edx = renderer_route_replace_byte(current.edx, 0,
                                               (renderer_u8)raw->ebx);
    flags = renderer_route_add32(current.eax, input->eax_step,
                                 (renderer_u32)0u, current.eflags,
                                 &added, &carry_eax);
    current.eax = added;
    current.eflags = flags;
    flags = renderer_route_add32(current.ebx, current.ecx,
                                 (renderer_u32)(carry_eax != 0u), current.eflags,
                                 &added, &carry_ebx);
    current.ebx = added;
    current.eflags = flags;

    /* D32C snapshots the incoming CH in DH before D32E loads the texel. */
    ch_before = (renderer_u8)(raw->ecx >> 8);
    current.edx = renderer_route_replace_byte(current.edx, 8, ch_before);
    step->source_address = current.esi + current.edx;
    current.edx = renderer_route_replace_byte(current.edx, 0, input->source_byte);

    flags = renderer_route_add8((renderer_u8)(current.ecx >> 8), input->ch_step,
                                (renderer_u32)(carry_ebx != 0u), current.eflags,
                                &ch_after, &ignored_carry);
    current.ecx = renderer_route_replace_byte(current.ecx, 8, ch_after);
    current.eflags = flags;
    step->post_uv = current;
    step->eax_carry = carry_eax;
    step->ebx_carry = carry_ebx;
    step->pointer_carry_in = (renderer_u8)0u;
    step->pointer_carry_out = (renderer_u8)0u;

    current.edx = renderer_route_replace_byte(current.edx, 8, (renderer_u8)0u);
    if (input->source_byte != (renderer_u8)0u) {
        step->palette_address = current.ebp + current.edx;
        current.edx = renderer_route_replace_byte(current.edx, 0,
                                                   input->palette_result);
        step->store_required = (renderer_u8)1u;
    } else {
        step->store_required = (renderer_u8)0u;
    }
    renderer_route_finish(&current, step, ROUTE_LOOP_CC50, ROUTE_LOOP_EXIT_CC50);
    return RENDERER_ROUTE_UV_OK;
}

enum RendererRouteUvStatus renderer_e680_mode80_uv_step(
    const RendererRouteUvRegisters *raw,
    const RendererE680Mode80UvInput *input,
    RendererRouteUvStep *step)
{
    RendererRouteUvRegisters current;
    renderer_u32 added;
    renderer_u32 flags;
    renderer_u8 carry_eax;
    renderer_u8 carry_ebx;
    renderer_u8 carry_pointer;
    enum RendererRouteUvStatus status;

    status = renderer_route_validate(raw, input, step);
    if (status != RENDERER_ROUTE_UV_OK)
        return status;
    current = *raw;
    step->source_byte = input->source_byte;
    step->destination_address = raw->edi;
    step->palette_address = (renderer_u32)0u;
    step->shade_address = (renderer_u32)0u;
    step->blend_address = (renderer_u32)0u;

    /* EE6D forms DH:DL from old BL and zero before sampling at ESI+EDX. */
    current.edx = renderer_route_replace_byte(current.edx, 8,
                                               (renderer_u8)raw->ebx);
    flags = renderer_route_add32(current.eax, input->eax_step,
                                 (renderer_u32)0u, current.eflags,
                                 &added, &carry_eax);
    current.eax = added;
    current.eflags = flags;
    flags = renderer_route_add32(current.ebx, current.ecx,
                                 (renderer_u32)(carry_eax != 0u), current.eflags,
                                 &added, &carry_ebx);
    current.ebx = added;
    current.eflags = flags;
    current.edx &= (renderer_u32)0xffffff00UL;
    step->source_address = current.esi + current.edx;
    current.edx = renderer_route_replace_byte(current.edx, 8, input->source_byte);
    flags = renderer_route_add32(current.esi, input->esi_correction,
                                 (renderer_u32)(carry_ebx != 0u), current.eflags,
                                 &added, &carry_pointer);
    current.esi = added;
    current.eflags = flags;
    step->post_uv = current;
    step->eax_carry = carry_eax;
    step->ebx_carry = carry_ebx;
    step->pointer_carry_in = carry_ebx;
    step->pointer_carry_out = carry_pointer;

    if (input->source_byte != (renderer_u8)0u) {
        current.edx = renderer_route_replace_byte(current.edx, 0,
                                                   input->destination_byte);
        step->blend_address = current.ebp + current.edx;
        current.edx = renderer_route_replace_byte(current.edx, 0,
                                                   input->blend_result);
        step->store_required = (renderer_u8)1u;
    } else {
        current.edx &= (renderer_u32)0xffffff00UL;
        step->store_required = (renderer_u8)0u;
    }
    renderer_route_finish(&current, step, ROUTE_LOOP_80, ROUTE_LOOP_EXIT_80);
    return RENDERER_ROUTE_UV_OK;
}

enum RendererRouteUvStatus renderer_e680_shade_uv_step(
    const RendererRouteUvRegisters *raw,
    const RendererE680ShadeUvInput *input,
    RendererRouteUvStep *step)
{
    RendererRouteUvRegisters current;
    renderer_u32 added;
    renderer_u32 flags;
    renderer_u32 shade_index;
    renderer_u8 carry_eax;
    renderer_u8 carry_ebx;
    renderer_u8 carry_pointer;
    enum RendererRouteUvStatus status;

    status = renderer_route_validate(raw, input, step);
    if (status != RENDERER_ROUTE_UV_OK)
        return status;
    current = *raw;
    step->source_byte = input->source_byte;
    step->destination_address = raw->edi;
    step->palette_address = (renderer_u32)0u;
    step->shade_address = (renderer_u32)0u;
    step->blend_address = (renderer_u32)0u;

    /* E56C shares the generic source and pointer sequence with mode 0x80. */
    current.edx = renderer_route_replace_byte(current.edx, 8,
                                               (renderer_u8)raw->ebx);
    flags = renderer_route_add32(current.eax, input->eax_step,
                                 (renderer_u32)0u, current.eflags,
                                 &added, &carry_eax);
    current.eax = added;
    current.eflags = flags;
    flags = renderer_route_add32(current.ebx, current.ecx,
                                 (renderer_u32)(carry_eax != 0u), current.eflags,
                                 &added, &carry_ebx);
    current.ebx = added;
    current.eflags = flags;
    current.edx &= (renderer_u32)0xffffff00UL;
    step->source_address = current.esi + current.edx;
    current.edx = renderer_route_replace_byte(current.edx, 8, input->source_byte);
    flags = renderer_route_add32(current.esi, input->esi_correction,
                                 (renderer_u32)(carry_ebx != 0u), current.eflags,
                                 &added, &carry_pointer);
    current.esi = added;
    current.eflags = flags;
    step->post_uv = current;
    step->eax_carry = carry_eax;
    step->ebx_carry = carry_ebx;
    step->pointer_carry_in = carry_ebx;
    step->pointer_carry_out = carry_pointer;

    if (input->source_byte != (renderer_u8)0u) {
        /* E585..E593 reduces the raw texel to a byte and adds the shade base. */
        shade_index = (renderer_u32)input->source_byte;
        step->shade_address = input->shade_base + shade_index;
        current.edx = step->shade_address;
        current.edx = renderer_route_replace_byte(current.edx, 8,
                                                   input->shade_result);
        current.edx &= (renderer_u32)0x0000ffffUL;
        current.edx = renderer_route_replace_byte(current.edx, 0,
                                                   input->destination_byte);
        step->blend_address = current.ebp + current.edx;
        current.edx = renderer_route_replace_byte(current.edx, 0,
                                                   input->blend_result);
        step->store_required = (renderer_u8)1u;
    } else {
        current.edx &= (renderer_u32)0xffffff00UL;
        step->store_required = (renderer_u8)0u;
    }
    renderer_route_finish(&current, step, ROUTE_LOOP_SHADE, ROUTE_LOOP_EXIT_SHADE);
    return RENDERER_ROUTE_UV_OK;
}
