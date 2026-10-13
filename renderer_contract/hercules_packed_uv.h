#ifndef RENDERER_D60C_PACKED_UV_H
#define RENDERER_D60C_PACKED_UV_H

#include "hercules_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum RendererD60CPackedStatus {
    RENDERER_D60C_PACKED_OK = 0,
    RENDERER_D60C_PACKED_INVALID_ARGUMENT = 1,
    RENDERER_D60C_PACKED_INVALID_COUNT = 2,
    RENDERER_D60C_PACKED_EXHAUSTED = 3,
    RENDERER_D60C_PACKED_INVALID_STATE = 4
};

/* These are raw 32-bit register/global words captured at the D60C setup
 * boundary. They are not host pointers and are not derived from geometry. */
typedef struct RendererD60CPackedInputs {
    renderer_u32 u_start;
    renderer_u32 v_start;
    renderer_u32 u_step;
    renderer_u32 v_step;
    renderer_u32 destination;
    renderer_u32 texture_base;
    renderer_u32 visible_count;
} RendererD60CPackedInputs;

/* The seven register fields keep their complete x86 32-bit values. The
 * final three fields are decoder invariants, not original registers. */
typedef struct RendererD60CPackedState {
    renderer_u32 eax;
    renderer_u32 ebx;
    renderer_u32 ecx;
    renderer_u32 edx;
    renderer_u32 esi;
    renderer_u32 edi;
    renderer_u32 ebp;
    renderer_u16 initial_count;
    renderer_u16 completed_count;
    renderer_u16 v_step_fraction_word;
} RendererD60CPackedState;

/* One observed pixel step. Fields ending in _after_* are exact register
 * boundaries around the instructions at DC8A..DC9E. */
typedef struct RendererD60CPackedStep {
    renderer_u32 source_address;
    renderer_u32 source_offset;
    renderer_u32 store_address;
    renderer_u32 eax_after_texture_read;
    renderer_u32 ebx_after_add;
    renderer_u32 ecx_after_adc_cl_dl;
    renderer_u32 edi_after_increment;
    renderer_u32 eax_after_add;
    renderer_u32 ecx_after_adc_ch_dh;
    renderer_u32 ebp_after_dec_bp;
    renderer_u8 sampled_texel;
    renderer_u8 u_add_carry;
    renderer_u8 v_add_carry;
    renderer_u8 store_required;
    renderer_u8 loop_continues;
} RendererD60CPackedStep;

/* Model DC35..DC87 setup using the original raw start/step words, destination,
 * texture base, and clipped visible count. Native count domain is 1..320. */
enum RendererD60CPackedStatus renderer_d60c_packed_init(
    const RendererD60CPackedInputs *inputs,
    RendererD60CPackedState *state);

/* Model one DC8A..DC9E sample. sampled_texel is the already-read byte at
 * (ESI + ECX) modulo 2^32. A zero texel suppresses the store indication but
 * still advances every register lane and the BP count, as native does. */
enum RendererD60CPackedStatus renderer_d60c_packed_advance(
    RendererD60CPackedState *state,
    renderer_u8 sampled_texel,
    RendererD60CPackedStep *step);

#ifdef __cplusplus
}
#endif

#endif
