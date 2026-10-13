#include <stdio.h>
#include <stdlib.h>
#include "hercules_route_uv_models.h"

static void put_registers(const RendererRouteUvRegisters *r)
{
    printf("%08lx,%08lx,%08lx,%08lx,%08lx,%08lx,%08lx,%08lx,%08lx",
           (unsigned long)r->eax, (unsigned long)r->ebx,
           (unsigned long)r->ecx, (unsigned long)r->edx,
           (unsigned long)r->esi, (unsigned long)r->edi,
           (unsigned long)r->ebp, (unsigned long)r->esp,
           (unsigned long)r->eflags);
}

static int parse_row(char *line, unsigned int values[18])
{
    return sscanf(line,
        "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
        &values[0], &values[1], &values[2], &values[3], &values[4],
        &values[5], &values[6], &values[7], &values[8], &values[9],
        &values[10], &values[11], &values[12], &values[13], &values[14],
        &values[15], &values[16], &values[17]) == 18;
}

int main(void)
{
    char line[512];
    unsigned int values[18];
    RendererRouteUvRegisters raw;
    RendererRouteUvStep step;
    RendererCc50UvInput cc50;
    RendererE680Mode80UvInput mode80;
    RendererE680ShadeUvInput shade;
    enum RendererRouteUvStatus status;
    unsigned int route;
    unsigned long records;

    records = 0UL;

    while (fgets(line, (int)sizeof(line), stdin) != 0) {
        if (!parse_row(line, values)) {
            fprintf(stderr, "invalid transition input row\n");
            return 2;
        }
        records++;
        route = values[0];
        raw.eax = (renderer_u32)values[1];
        raw.ebx = (renderer_u32)values[2];
        raw.ecx = (renderer_u32)values[3];
        raw.edx = (renderer_u32)values[4];
        raw.esi = (renderer_u32)values[5];
        raw.edi = (renderer_u32)values[6];
        raw.ebp = (renderer_u32)values[7];
        raw.esp = (renderer_u32)values[8];
        raw.eflags = (renderer_u32)values[9];

        if (route == 0u) {
            cc50.eax_step = (renderer_u32)values[10];
            cc50.ch_step = (renderer_u8)values[11];
            cc50.source_byte = (renderer_u8)values[14];
            cc50.palette_result = (renderer_u8)values[17];
            status = renderer_cc50_uv_step(&raw, &cc50, &step);
        } else if (route == 1u) {
            mode80.eax_step = (renderer_u32)values[10];
            mode80.esi_correction = (renderer_u32)values[11];
            mode80.source_byte = (renderer_u8)values[14];
            mode80.destination_byte = (renderer_u8)values[15];
            mode80.blend_result = (renderer_u8)values[17];
            status = renderer_e680_mode80_uv_step(&raw, &mode80, &step);
        } else if (route == 2u) {
            shade.eax_step = (renderer_u32)values[10];
            shade.esi_correction = (renderer_u32)values[11];
            shade.shade_base = (renderer_u32)values[12];
            shade.source_byte = (renderer_u8)values[14];
            shade.destination_byte = (renderer_u8)values[15];
            shade.shade_result = (renderer_u8)values[16];
            shade.blend_result = (renderer_u8)values[17];
            status = renderer_e680_shade_uv_step(&raw, &shade, &step);
        } else {
            fprintf(stderr, "unknown route id\n");
            return 3;
        }
        if (status != RENDERER_ROUTE_UV_OK) {
            printf("%u\n", (unsigned int)status);
            continue;
        }
        printf("0,");
        put_registers(&step.next);
        printf(",%08lx,", (unsigned long)step.next_pc);
        put_registers(&step.post_uv);
        printf(",%08lx,%08lx,%08lx,%08lx,%08lx,%u,%u,%u,%u,%u,%u\n",
               (unsigned long)step.source_address,
               (unsigned long)step.palette_address,
               (unsigned long)step.shade_address,
               (unsigned long)step.blend_address,
               (unsigned long)step.destination_address,
               (unsigned int)step.eax_carry,
               (unsigned int)step.ebx_carry,
               (unsigned int)step.pointer_carry_in,
               (unsigned int)step.pointer_carry_out,
               (unsigned int)step.store_required,
               (unsigned int)step.loop_continues);
    }
    if (records == 0UL) {
        fprintf(stderr, "no native trace records supplied\n");
        return 4;
    }
    return 0;
}
