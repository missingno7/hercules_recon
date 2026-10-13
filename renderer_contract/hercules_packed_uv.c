#include "hercules_packed_uv.h"

static renderer_u32 renderer_d60c_rotl16(renderer_u32 value)
{
    return (value << 16) | (value >> 16);
}

static renderer_u32 renderer_d60c_replace_byte(renderer_u32 word,
                                                renderer_u32 shift,
                                                renderer_u8 value)
{
    renderer_u32 mask = (renderer_u32)0xffu << shift;
    return (word & ~mask) | ((renderer_u32)value << shift);
}

enum RendererD60CPackedStatus renderer_d60c_packed_init(
    const RendererD60CPackedInputs *inputs,
    RendererD60CPackedState *state)
{
    RendererD60CPackedState initial;
    renderer_u32 eax;
    renderer_u32 ecx;
    renderer_u32 edx;
    renderer_u32 ebp;
    renderer_u8 byte_value;

    if (inputs == 0 || state == 0)
        return RENDERER_D60C_PACKED_INVALID_ARGUMENT;
    if (inputs->visible_count == (renderer_u32)0u ||
        inputs->visible_count > (renderer_u32)320u)
        return RENDERER_D60C_PACKED_INVALID_COUNT;

    initial.ebx = inputs->u_start << 16;
    ecx = inputs->u_start >> 16;
    eax = inputs->v_start >> 16;
    byte_value = (renderer_u8)eax;
    initial.ecx = renderer_d60c_replace_byte(ecx, 8, byte_value);

    edx = renderer_d60c_rotl16(inputs->u_step);
    eax = inputs->v_step >> 16;
    byte_value = (renderer_u8)eax;
    initial.edx = renderer_d60c_replace_byte(edx, 8, byte_value);

    initial.eax = inputs->v_start << 16;
    initial.esi = inputs->texture_base;
    initial.edi = inputs->destination - (renderer_u32)1u;

    ebp = inputs->v_step << 16;
    initial.v_step_fraction_word = (renderer_u16)(ebp >> 16);
    initial.ebp = (ebp & (renderer_u32)0xffff0000u) |
                  (renderer_u32)(renderer_u16)inputs->visible_count;
    initial.initial_count = (renderer_u16)inputs->visible_count;
    initial.completed_count = (renderer_u16)0u;

    *state = initial;
    return RENDERER_D60C_PACKED_OK;
}

enum RendererD60CPackedStatus renderer_d60c_packed_advance(
    RendererD60CPackedState *state,
    renderer_u8 sampled_texel,
    RendererD60CPackedStep *step)
{
    RendererD60CPackedState next;
    RendererD60CPackedStep observed;
    renderer_u32 old_value;
    renderer_u32 sum;
    renderer_u32 old_cl;
    renderer_u32 dl;
    renderer_u32 cl_result;
    renderer_u32 old_ch;
    renderer_u32 dh;
    renderer_u32 ch_result;
    renderer_u32 bp;
    renderer_u32 expected_remaining;

    if (state == 0 || step == 0)
        return RENDERER_D60C_PACKED_INVALID_ARGUMENT;
    if (state->initial_count == (renderer_u16)0u ||
        state->initial_count > (renderer_u16)320u ||
        state->completed_count > state->initial_count)
        return RENDERER_D60C_PACKED_INVALID_STATE;
    expected_remaining = (renderer_u32)(state->initial_count - state->completed_count);
    if ((state->ebp & (renderer_u32)0xffffu) != expected_remaining ||
        (state->ebp >> 16) != (renderer_u32)state->v_step_fraction_word)
        return RENDERER_D60C_PACKED_INVALID_STATE;
    if (expected_remaining == (renderer_u32)0u)
        return RENDERER_D60C_PACKED_EXHAUSTED;

    next = *state;
    observed.source_offset = next.ecx;
    observed.source_address = next.esi + next.ecx;
    observed.sampled_texel = sampled_texel;
    next.eax = (next.eax & (renderer_u32)0xffffff00u) |
               (renderer_u32)sampled_texel;
    observed.eax_after_texture_read = next.eax;

    old_value = next.ebx;
    sum = old_value + next.edx;
    next.ebx = sum;
    observed.u_add_carry = (renderer_u8)(sum < old_value);
    observed.ebx_after_add = next.ebx;

    old_cl = next.ecx & (renderer_u32)0xffu;
    dl = next.edx & (renderer_u32)0xffu;
    cl_result = (old_cl + dl + (renderer_u32)observed.u_add_carry) &
                (renderer_u32)0xffu;
    next.ecx = (next.ecx & (renderer_u32)0xffffff00u) | cl_result;
    observed.ecx_after_adc_cl_dl = next.ecx;

    next.edi += (renderer_u32)1u;
    observed.edi_after_increment = next.edi;
    observed.store_address = next.edi;
    observed.store_required = (renderer_u8)(sampled_texel != (renderer_u8)0u);

    old_value = next.eax;
    sum = old_value + next.ebp;
    next.eax = sum;
    observed.v_add_carry = (renderer_u8)(sum < old_value);
    observed.eax_after_add = next.eax;

    old_ch = (next.ecx >> 8) & (renderer_u32)0xffu;
    dh = (next.edx >> 8) & (renderer_u32)0xffu;
    ch_result = (old_ch + dh + (renderer_u32)observed.v_add_carry) &
                (renderer_u32)0xffu;
    next.ecx = renderer_d60c_replace_byte(next.ecx, 8,
                                           (renderer_u8)ch_result);
    observed.ecx_after_adc_ch_dh = next.ecx;

    bp = (next.ebp & (renderer_u32)0xffffu) - (renderer_u32)1u;
    next.ebp = (next.ebp & (renderer_u32)0xffff0000u) |
               (bp & (renderer_u32)0xffffu);
    next.completed_count = (renderer_u16)(next.completed_count + (renderer_u16)1u);
    observed.ebp_after_dec_bp = next.ebp;
    observed.loop_continues = (renderer_u8)(bp != (renderer_u32)0u);

    *state = next;
    *step = observed;
    return RENDERER_D60C_PACKED_OK;
}
