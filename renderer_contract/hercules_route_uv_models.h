#ifndef HERCULES_ROUTE_UV_MODELS_H
#define HERCULES_ROUTE_UV_MODELS_H

#include "hercules_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum RendererRouteUvStatus {
    RENDERER_ROUTE_UV_OK = 0,
    RENDERER_ROUTE_UV_INVALID_ARGUMENT = 1,
    RENDERER_ROUTE_UV_INVALID_COUNT = 2
};

typedef struct RendererRouteUvRegisters {
    renderer_u32 eax;
    renderer_u32 ebx;
    renderer_u32 ecx;
    renderer_u32 edx;
    renderer_u32 esi;
    renderer_u32 edi;
    renderer_u32 ebp;
    renderer_u32 esp;
    renderer_u32 eflags;
} RendererRouteUvRegisters;

typedef struct RendererRouteUvStep {
    RendererRouteUvRegisters post_uv;
    RendererRouteUvRegisters next;
    renderer_u32 next_pc;
    renderer_u32 source_address;
    renderer_u32 destination_address;
    renderer_u32 palette_address;
    renderer_u32 shade_address;
    renderer_u32 blend_address;
    renderer_u8 source_byte;
    renderer_u8 store_required;
    renderer_u8 loop_continues;
    renderer_u8 eax_carry;
    renderer_u8 ebx_carry;
    renderer_u8 pointer_carry_in;
    renderer_u8 pointer_carry_out;
} RendererRouteUvStep;

typedef struct RendererCc50UvInput {
    renderer_u32 eax_step;
    renderer_u8 ch_step;
    renderer_u8 source_byte;
    renderer_u8 palette_result;
} RendererCc50UvInput;

typedef struct RendererE680Mode80UvInput {
    renderer_u32 eax_step;
    renderer_u32 esi_correction;
    renderer_u8 source_byte;
    renderer_u8 destination_byte;
    renderer_u8 blend_result;
} RendererE680Mode80UvInput;

typedef struct RendererE680ShadeUvInput {
    renderer_u32 eax_step;
    renderer_u32 esi_correction;
    renderer_u32 shade_base;
    renderer_u8 source_byte;
    renderer_u8 destination_byte;
    renderer_u8 shade_result;
    renderer_u8 blend_result;
} RendererE680ShadeUvInput;

/* Inputs are raw pre-instruction registers at the named loop entry.
 * The API accepts signed-positive AX values 1..320; the captured fixture
 * observes countdown values 319..1 and does not exercise AX=320.
 * The functions model only the original register and byte-address transitions. */
enum RendererRouteUvStatus renderer_cc50_uv_step(
    const RendererRouteUvRegisters *raw_loop_state,
    const RendererCc50UvInput *input,
    RendererRouteUvStep *step);

enum RendererRouteUvStatus renderer_e680_mode80_uv_step(
    const RendererRouteUvRegisters *raw_loop_state,
    const RendererE680Mode80UvInput *input,
    RendererRouteUvStep *step);

enum RendererRouteUvStatus renderer_e680_shade_uv_step(
    const RendererRouteUvRegisters *raw_loop_state,
    const RendererE680ShadeUvInput *input,
    RendererRouteUvStep *step);

#ifdef __cplusplus
}
#endif

#endif
