#include <stdio.h>
#include "hercules_route_uv_models.h"

static int failures;

static int same_registers(const RendererRouteUvRegisters *actual,
                          const RendererRouteUvRegisters *expected)
{
    return actual->eax == expected->eax && actual->ebx == expected->ebx &&
           actual->ecx == expected->ecx && actual->edx == expected->edx &&
           actual->esi == expected->esi && actual->edi == expected->edi &&
           actual->ebp == expected->ebp && actual->esp == expected->esp &&
           actual->eflags == expected->eflags;
}

static void check(int condition, const char *label)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", label);
        failures++;
    }
}

static RendererRouteUvRegisters captured_cc50_raw(void)
{
    RendererRouteUvRegisters raw;
    raw.eax = (renderer_u32)0x0000013fUL;
    raw.ebx = (renderer_u32)0x9b4d0000UL;
    raw.ecx = (renderer_u32)0xff320100UL;
    raw.edx = (renderer_u32)0x00000000UL;
    raw.esi = (renderer_u32)0x10001000UL;
    raw.edi = (renderer_u32)0x10020200UL;
    raw.ebp = (renderer_u32)0x0092c690UL;
    raw.esp = (renderer_u32)0x20007fd0UL;
    raw.eflags = (renderer_u32)0x00000203UL;
    return raw;
}

static RendererRouteUvRegisters captured_e680_raw(void)
{
    RendererRouteUvRegisters raw;
    raw.eax = (renderer_u32)0x9b4d013fUL;
    raw.ebx = (renderer_u32)0x00000001UL;
    raw.ecx = (renderer_u32)0x000000ffUL;
    raw.edx = (renderer_u32)0x00000000UL;
    raw.esi = (renderer_u32)0x10001000UL;
    raw.edi = (renderer_u32)0x10020200UL;
    raw.ebp = (renderer_u32)0x0094e3d0UL;
    raw.esp = (renderer_u32)0x20007fe8UL;
    raw.eflags = (renderer_u32)0x00000206UL;
    return raw;
}

static void test_cc50(void)
{
    RendererRouteUvRegisters raw;
    RendererRouteUvRegisters expected_post;
    RendererRouteUvRegisters expected_next;
    RendererCc50UvInput input;
    RendererRouteUvStep step;
    enum RendererRouteUvStatus status;

    raw = captured_cc50_raw();
    expected_post = raw;
    expected_post.ebx = (renderer_u32)0x9a7f0100UL;
    expected_post.edx = (renderer_u32)0x00000101UL;
    expected_post.eflags = (renderer_u32)0x00000213UL;
    expected_next = expected_post;
    expected_next.eax = (renderer_u32)0x0000013eUL;
    expected_next.edx = (renderer_u32)0x00000044UL;
    expected_next.edi = (renderer_u32)0x10020201UL;
    expected_next.eflags = (renderer_u32)0x00000202UL;
    input.eax_step = (renderer_u32)0UL;
    input.ch_step = (renderer_u8)0xffu;
    input.source_byte = (renderer_u8)1u;
    input.palette_result = (renderer_u8)0x44u;
    status = renderer_cc50_uv_step(&raw, &input, &step);
    check(status == RENDERER_ROUTE_UV_OK, "CC50 captured vector status");
    check(same_registers(&step.post_uv, &expected_post), "CC50 captured post-UV registers");
    check(same_registers(&step.next, &expected_next), "CC50 captured next registers");
    check(step.source_address == (renderer_u32)0x10001100UL,
          "CC50 captured source address");
    check(step.palette_address == (renderer_u32)0x0092c691UL,
          "CC50 captured palette address");
    check(step.store_required == (renderer_u8)1u &&
          step.next_pc == (renderer_u32)0x0040d322UL,
          "CC50 captured store and continue decision");
}

static void test_mode80(void)
{
    RendererRouteUvRegisters raw;
    RendererRouteUvRegisters expected_post;
    RendererRouteUvRegisters expected_next;
    RendererE680Mode80UvInput input;
    RendererRouteUvStep step;
    enum RendererRouteUvStatus status;

    raw = captured_e680_raw();
    expected_post = raw;
    expected_post.eax = (renderer_u32)0x9a7f013fUL;
    expected_post.ebx = (renderer_u32)0x00000101UL;
    expected_post.edx = (renderer_u32)0x00000100UL;
    expected_post.eflags = (renderer_u32)0x00000206UL;
    expected_next = expected_post;
    expected_next.eax = (renderer_u32)0x9a7f013eUL;
    expected_next.edx = (renderer_u32)0x00000166UL;
    expected_next.edi = (renderer_u32)0x10020201UL;
    expected_next.eflags = (renderer_u32)0x00000202UL;
    input.eax_step = (renderer_u32)0xff320000UL;
    input.esi_correction = (renderer_u32)0UL;
    input.source_byte = (renderer_u8)1u;
    input.destination_byte = (renderer_u8)0xa5u;
    input.blend_result = (renderer_u8)0x66u;
    status = renderer_e680_mode80_uv_step(&raw, &input, &step);
    check(status == RENDERER_ROUTE_UV_OK, "E680 mode80 captured vector status");
    check(same_registers(&step.post_uv, &expected_post), "E680 mode80 captured post-UV registers");
    check(same_registers(&step.next, &expected_next), "E680 mode80 captured next registers");
    check(step.source_address == (renderer_u32)0x10001100UL,
          "E680 mode80 captured source address");
    check(step.blend_address == (renderer_u32)0x0094e575UL,
          "E680 mode80 captured blend address");
    check(step.eax_carry == (renderer_u8)1u && step.ebx_carry == (renderer_u8)0u &&
          step.pointer_carry_in == (renderer_u8)0u &&
          step.pointer_carry_out == (renderer_u8)0u,
          "E680 mode80 captured carry chain");

    input.source_byte = (renderer_u8)0u;
    status = renderer_e680_mode80_uv_step(&raw, &input, &step);
    check(status == RENDERER_ROUTE_UV_OK &&
          step.store_required == (renderer_u8)0u &&
          step.blend_address == (renderer_u32)0u,
          "E680 mode80 transparent source skips blend/store");
}

static void test_shade(void)
{
    RendererRouteUvRegisters raw;
    RendererRouteUvRegisters expected_post;
    RendererRouteUvRegisters expected_next;
    RendererE680ShadeUvInput input;
    RendererRouteUvStep step;
    enum RendererRouteUvStatus status;

    raw = captured_e680_raw();
    raw.esp = (renderer_u32)0x20007fd0UL;
    expected_post = raw;
    expected_post.eax = (renderer_u32)0x9a7f013fUL;
    expected_post.ebx = (renderer_u32)0x00000101UL;
    expected_post.edx = (renderer_u32)0x00000100UL;
    expected_post.eflags = (renderer_u32)0x00000206UL;
    expected_next = expected_post;
    expected_next.eax = (renderer_u32)0x9a7f013eUL;
    expected_next.edx = (renderer_u32)0x00002266UL;
    expected_next.edi = (renderer_u32)0x10020201UL;
    expected_next.eflags = (renderer_u32)0x00000202UL;
    input.eax_step = (renderer_u32)0xff320000UL;
    input.esi_correction = (renderer_u32)0UL;
    input.shade_base = (renderer_u32)0x00924388UL;
    input.source_byte = (renderer_u8)1u;
    input.destination_byte = (renderer_u8)0xa5u;
    input.shade_result = (renderer_u8)0x22u;
    input.blend_result = (renderer_u8)0x66u;
    status = renderer_e680_shade_uv_step(&raw, &input, &step);
    check(status == RENDERER_ROUTE_UV_OK, "E680 shade captured vector status");
    check(same_registers(&step.post_uv, &expected_post), "E680 shade captured post-UV registers");
    check(same_registers(&step.next, &expected_next), "E680 shade captured next registers");
    check(step.source_address == (renderer_u32)0x10001100UL &&
          step.shade_address == (renderer_u32)0x00924389UL &&
          step.blend_address == (renderer_u32)0x00950675UL,
          "E680 shade captured source/shade/blend addresses");
}

static void test_argument_guards(void)
{
    RendererRouteUvRegisters raw;
    RendererCc50UvInput cc50;
    RendererE680Mode80UvInput mode80;
    RendererE680ShadeUvInput shade;
    RendererRouteUvStep step;

    raw = captured_e680_raw();
    check(renderer_cc50_uv_step(0, &cc50, &step) == RENDERER_ROUTE_UV_INVALID_ARGUMENT,
          "CC50 NULL registers");
    check(renderer_e680_mode80_uv_step(&raw, 0, &step) == RENDERER_ROUTE_UV_INVALID_ARGUMENT,
          "E680 mode80 NULL input");
    check(renderer_e680_shade_uv_step(&raw, &shade, 0) == RENDERER_ROUTE_UV_INVALID_ARGUMENT,
          "E680 shade NULL output");
    raw.eax &= (renderer_u32)0xffff0000UL;
    check(renderer_e680_mode80_uv_step(&raw, &mode80, &step) == RENDERER_ROUTE_UV_INVALID_COUNT,
          "zero AX rejected");
    raw.eax |= (renderer_u32)321UL;
    check(renderer_cc50_uv_step(&raw, &cc50, &step) == RENDERER_ROUTE_UV_INVALID_COUNT,
          "AX above proven maximum rejected");
}

int main(void)
{
    test_cc50();
    test_mode80();
    test_shade();
    test_argument_guards();
    if (failures != 0) {
        fprintf(stderr, "%d route UV checks failed\n", failures);
        return 1;
    }
    printf("route UV standalone checks passed\n");
    return 0;
}
