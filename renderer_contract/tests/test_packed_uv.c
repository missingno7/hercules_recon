#include <stdio.h>
#include "hercules_packed_uv.h"

static int checks = 0;
static int failures = 0;

#define CHECK(label, condition) do { \
    ++checks; \
    if (!(condition)) { \
        ++failures; \
        printf("FAIL %s\n", label); \
    } \
} while (0)

static RendererD60CPackedInputs phase7_inputs(renderer_u32 count)
{
    RendererD60CPackedInputs inputs;
    inputs.u_start = (renderer_u32)0x00000000u;
    inputs.v_start = (renderer_u32)0x00019b4du;
    inputs.u_step = (renderer_u32)0x00000000u;
    inputs.v_step = (renderer_u32)0xffffff32u;
    inputs.destination = (renderer_u32)0x10020140u;
    inputs.texture_base = (renderer_u32)0x10001000u;
    inputs.visible_count = count;
    return inputs;
}

int main(void)
{
    RendererD60CPackedInputs inputs;
    RendererD60CPackedInputs invalid;
    RendererD60CPackedState state;
    RendererD60CPackedState one_state;
    RendererD60CPackedStep step;
    renderer_u32 i;

    inputs = phase7_inputs((renderer_u32)319u);
    CHECK("phase7 init accepted", renderer_d60c_packed_init(&inputs, &state) == RENDERER_D60C_PACKED_OK);
    CHECK("phase7 exact initial EAX", state.eax == (renderer_u32)0x9b4d0000u);
    CHECK("phase7 exact initial EBX", state.ebx == (renderer_u32)0x00000000u);
    CHECK("phase7 full initial ECX", state.ecx == (renderer_u32)0x00000100u);
    CHECK("phase7 exact initial EDX", state.edx == (renderer_u32)0x0000ff00u);
    CHECK("phase7 raw texture register", state.esi == inputs.texture_base);
    CHECK("phase7 destination-minus-one", state.edi == (renderer_u32)0x1002013fu);
    CHECK("phase7 packed step and count", state.ebp == (renderer_u32)0xff32013fu);

    invalid = phase7_inputs((renderer_u32)0u);
    CHECK("zero span rejected", renderer_d60c_packed_init(&invalid, &state) == RENDERER_D60C_PACKED_INVALID_COUNT);
    invalid = phase7_inputs((renderer_u32)321u);
    CHECK("over-width span rejected", renderer_d60c_packed_init(&invalid, &state) == RENDERER_D60C_PACKED_INVALID_COUNT);
    CHECK("null inputs rejected", renderer_d60c_packed_init(0, &state) == RENDERER_D60C_PACKED_INVALID_ARGUMENT);
    CHECK("null state rejected", renderer_d60c_packed_init(&inputs, 0) == RENDERER_D60C_PACKED_INVALID_ARGUMENT);

    invalid = phase7_inputs((renderer_u32)1u);
    invalid.u_start = (renderer_u32)0x80010000u;
    CHECK("high-bit U setup accepted", renderer_d60c_packed_init(&invalid, &state) == RENDERER_D60C_PACKED_OK);
    CHECK("U setup uses logical SHR and keeps full ECX", state.ecx == (renderer_u32)0x00000101u);

    inputs = phase7_inputs((renderer_u32)1u);
    CHECK("one-pixel endpoint accepted", renderer_d60c_packed_init(&inputs, &one_state) == RENDERER_D60C_PACKED_OK);
    CHECK("one-pixel zero source advances", renderer_d60c_packed_advance(&one_state, (renderer_u8)0u, &step) == RENDERER_D60C_PACKED_OK);
    CHECK("zero source requests no store", step.store_required == (renderer_u8)0u);
    CHECK("zero source still advances destination", step.edi_after_increment == inputs.destination);
    CHECK("zero source still advances recurrence", one_state.completed_count == (renderer_u16)1u &&
          one_state.eax != (renderer_u32)0x9b4d0000u);
    CHECK("one-pixel endpoint reaches zero", one_state.ebp == (renderer_u32)0xff320000u &&
          step.loop_continues == (renderer_u8)0u);
    CHECK("advance after exhaustion rejected", renderer_d60c_packed_advance(&one_state, (renderer_u8)1u, &step) == RENDERER_D60C_PACKED_EXHAUSTED);

    inputs = phase7_inputs((renderer_u32)320u);
    CHECK("320-pixel endpoint accepted", renderer_d60c_packed_init(&inputs, &state) == RENDERER_D60C_PACKED_OK);
    for (i = (renderer_u32)0u; i < (renderer_u32)320u; ++i) {
        CHECK("320-pixel bounded advance", renderer_d60c_packed_advance(&state, (renderer_u8)0u, &step) == RENDERER_D60C_PACKED_OK);
    }
    CHECK("320-pixel endpoint reaches zero", (state.ebp & (renderer_u32)0xffffu) == (renderer_u32)0u &&
          state.completed_count == (renderer_u16)320u &&
          step.loop_continues == (renderer_u8)0u);
    CHECK("320-pixel high EBP lane preserved", (state.ebp >> 16) == (renderer_u32)0xff32u);
    CHECK("320-pixel terminal state rejects extra advance", renderer_d60c_packed_advance(&state, (renderer_u8)0u, &step) == RENDERER_D60C_PACKED_EXHAUSTED);

    printf("checks=%u failures=%d\n", (unsigned int)checks, failures);
    return failures == 0 ? 0 : 1;
}
