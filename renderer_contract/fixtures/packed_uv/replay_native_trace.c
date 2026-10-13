#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hercules_packed_uv.h"

static int parse_hex(const char *text, renderer_u32 *value)
{
    char *end;
    unsigned long parsed;
    if (text == 0 || value == 0 || *text == '\0')
        return 0;
    parsed = strtoul(text, &end, 16);
    if (end == text || *end != '\0' || parsed > 0xffffffffUL)
        return 0;
    *value = (renderer_u32)parsed;
    return 1;
}

static int split_tokens(char *line, char *tokens[], int capacity)
{
    int count = 0;
    char *token = strtok(line, " \t\r\n");
    while (token != 0) {
        if (count >= capacity)
            return -1;
        tokens[count++] = token;
        token = strtok(0, " \t\r\n");
    }
    return count;
}

static int compare_word(const char *fixture, renderer_u32 sample,
                        const char *field, renderer_u32 expected,
                        renderer_u32 actual)
{
    if (expected == actual)
        return 1;
    printf("FAIL %s sample=%lu field=%s expected=%08lX actual=%08lX\n",
           fixture, (unsigned long)sample, field,
           (unsigned long)expected, (unsigned long)actual);
    return 0;
}

static int read_values(char *tokens[], int first, int last,
                       renderer_u32 values[], int value_capacity)
{
    int token_index;
    int value_index = 0;
    if (last - first > value_capacity)
        return 0;
    for (token_index = first; token_index < last; ++token_index) {
        if (!parse_hex(tokens[token_index], &values[value_index++]))
            return 0;
    }
    return 1;
}

int main(void)
{
    char line[512];
    char *tokens[24];
    char fixture[64];
    renderer_u32 header[15];
    renderer_u32 values[20];
    renderer_u32 native_regs[7];
    renderer_u32 sample_index;
    renderer_u32 sample_count;
    renderer_u32 i;
    int token_count;
    int fixture_count = 0;
    int ok = 1;
    RendererD60CPackedInputs inputs;
    RendererD60CPackedState state;
    RendererD60CPackedStep step;
    enum RendererD60CPackedStatus status;

    while (fgets(line, sizeof(line), stdin) != 0) {
        token_count = split_tokens(line, tokens, 24);
        if (token_count == 0)
            continue;
        if (token_count != 17 || strcmp(tokens[0], "F") != 0 ||
            strlen(tokens[1]) >= sizeof(fixture) ||
            !read_values(tokens, 2, 17, header, 15)) {
            printf("FAIL malformed fixture header\n");
            return 2;
        }
        strcpy(fixture, tokens[1]);
        inputs.u_start = header[0];
        inputs.v_start = header[1];
        inputs.u_step = header[2];
        inputs.v_step = header[3];
        inputs.destination = header[4];
        inputs.texture_base = header[5];
        inputs.visible_count = header[6];
        sample_count = header[7];
        if (sample_count != inputs.visible_count || sample_count == (renderer_u32)0u ||
            sample_count > (renderer_u32)320u) {
            printf("FAIL %s invalid sample/count range\n", fixture);
            return 2;
        }
        for (i = (renderer_u32)0u; i < (renderer_u32)7u; ++i)
            native_regs[i] = header[8u + i];
        status = renderer_d60c_packed_init(&inputs, &state);
        if (status != RENDERER_D60C_PACKED_OK) {
            printf("FAIL %s init status=%u\n", fixture, (unsigned int)status);
            return 2;
        }
        ok = compare_word(fixture, (renderer_u32)0u, "init_eax", native_regs[0], state.eax) && ok;
        ok = compare_word(fixture, (renderer_u32)0u, "init_ebx", native_regs[1], state.ebx) && ok;
        ok = compare_word(fixture, (renderer_u32)0u, "init_ecx_full", native_regs[2], state.ecx) && ok;
        ok = compare_word(fixture, (renderer_u32)0u, "init_edx", native_regs[3], state.edx) && ok;
        ok = compare_word(fixture, (renderer_u32)0u, "init_esi", native_regs[4], state.esi) && ok;
        ok = compare_word(fixture, (renderer_u32)0u, "init_edi", native_regs[5], state.edi) && ok;
        ok = compare_word(fixture, (renderer_u32)0u, "init_ebp", native_regs[6], state.ebp) && ok;

        for (sample_index = (renderer_u32)0u;
             sample_index < sample_count; ++sample_index) {
            if (fgets(line, sizeof(line), stdin) == 0) {
                printf("FAIL %s missing sample %lu\n", fixture,
                       (unsigned long)sample_index);
                return 2;
            }
            token_count = split_tokens(line, tokens, 24);
            if (token_count != 20 || strcmp(tokens[0], "S") != 0 ||
                !read_values(tokens, 1, 20, values, 20)) {
                printf("FAIL %s malformed sample %lu\n", fixture,
                       (unsigned long)sample_index);
                return 2;
            }
            ok = compare_word(fixture, sample_index, "before_eax", values[1], state.eax) && ok;
            ok = compare_word(fixture, sample_index, "before_ebx", values[2], state.ebx) && ok;
            ok = compare_word(fixture, sample_index, "before_ecx_full", values[3], state.ecx) && ok;
            ok = compare_word(fixture, sample_index, "before_edx", values[4], state.edx) && ok;
            ok = compare_word(fixture, sample_index, "before_esi", values[5], state.esi) && ok;
            ok = compare_word(fixture, sample_index, "before_edi", values[6], state.edi) && ok;
            ok = compare_word(fixture, sample_index, "before_ebp", values[7], state.ebp) && ok;

            status = renderer_d60c_packed_advance(&state,
                (renderer_u8)values[0], &step);
            if (status != RENDERER_D60C_PACKED_OK) {
                printf("FAIL %s advance sample=%lu status=%u\n", fixture,
                       (unsigned long)sample_index, (unsigned int)status);
                return 2;
            }
            ok = compare_word(fixture, sample_index, "source_address", values[8], step.source_address) && ok;
            ok = compare_word(fixture, sample_index, "source_offset_full_ecx", values[3], step.source_offset) && ok;
            ok = compare_word(fixture, sample_index, "eax_after_texture_read", values[9], step.eax_after_texture_read) && ok;
            ok = compare_word(fixture, sample_index, "ebx_after_add", values[10], step.ebx_after_add) && ok;
            ok = compare_word(fixture, sample_index, "u_add_carry", values[11], step.u_add_carry) && ok;
            ok = compare_word(fixture, sample_index, "ecx_after_adc_cl_dl", values[12], step.ecx_after_adc_cl_dl) && ok;
            ok = compare_word(fixture, sample_index, "edi_after_increment", values[13], step.edi_after_increment) && ok;
            ok = compare_word(fixture, sample_index, "store_address", values[13], step.store_address) && ok;
            ok = compare_word(fixture, sample_index, "store_required", values[14], step.store_required) && ok;
            ok = compare_word(fixture, sample_index, "eax_after_add", values[15], step.eax_after_add) && ok;
            ok = compare_word(fixture, sample_index, "v_add_carry", values[16], step.v_add_carry) && ok;
            ok = compare_word(fixture, sample_index, "ecx_after_adc_ch_dh", values[17], step.ecx_after_adc_ch_dh) && ok;
            ok = compare_word(fixture, sample_index, "ebp_after_dec_bp", values[18], step.ebp_after_dec_bp) && ok;
            ok = compare_word(fixture, sample_index, "loop_continues",
                              sample_index + (renderer_u32)1u < sample_count,
                              step.loop_continues) && ok;
        }
        if ((state.ebp & (renderer_u32)0xffffu) != (renderer_u32)0u ||
            state.completed_count != state.initial_count) {
            printf("FAIL %s terminal count did not reach zero\n", fixture);
            ok = 0;
        }
        printf("PASS %s samples=%lu\n", fixture, (unsigned long)sample_count);
        ++fixture_count;
    }
    if (fixture_count == 0) {
        printf("FAIL no native span fixtures were supplied\n");
        return 2;
    }
    if (!ok)
        return 1;
    printf("native fixture runs=%d all register transitions matched\n", fixture_count);
    return 0;
}
