#include "hercules_host_abi.h"

#include <stdio.h>
#include <string.h>

static int same_words(const renderer_u32 *left, const renderer_u32 *right)
{
    return memcmp(left, right, sizeof(renderer_u32) * ENGINE_TABLE_WORD_COUNT) == 0;
}

static void fill_source(renderer_u32 *words)
{
    renderer_u32 i;
    for (i = 0; i < ENGINE_TABLE_WORD_COUNT; ++i)
        words[i] = (renderer_u32)0x13570000u + i;
    words[0] = ENGINE_TABLE_SIGNATURE;
}

static void fill_sentinel(renderer_u32 *words)
{
    renderer_u32 i;
    for (i = 0; i < ENGINE_TABLE_WORD_COUNT; ++i)
        words[i] = (renderer_u32)0xccccccccu;
}

static int expect_failure_without_output_change(int status_expected,
                                                const renderer_u32 *source,
                                                const renderer_u32 *slots,
                                                const renderer_u32 *values,
                                                renderer_u32 count)
{
    renderer_u32 output[ENGINE_TABLE_WORD_COUNT];
    renderer_u32 before[ENGINE_TABLE_WORD_COUNT];
    int status;

    fill_sentinel(output);
    memcpy(before, output, sizeof(output));
    status = engine_table_clone_selected(source, output, slots, values, count);
    return status == status_expected && same_words(output, before);
}

int main(void)
{
    renderer_u32 source[ENGINE_TABLE_WORD_COUNT];
    renderer_u32 output[ENGINE_TABLE_WORD_COUNT];
    renderer_u32 slots[10] = {89u, 129u, 130u, 131u, 143u, 148u, 81u,
                              174u, 175u, 89u};
    renderer_u32 values[10] = {0x11111111u, 0x22222222u, 0x33333333u,
                               0xaaaaaaaa, 0x44444444u, 0x55555555u,
                               0x66666666u, 0x77777777u, 0x88888888u,
                               0x99999999u};
    renderer_u32 i;
    int status;

    fill_source(source);
    fill_sentinel(output);
    status = engine_table_clone_selected(source, output, slots, values, 9u);
    if (status != ENGINE_TABLE_CLONE_OK)
        return 1;
    for (i = 0; i < ENGINE_TABLE_WORD_COUNT; ++i) {
        renderer_u32 expected = source[i];
        renderer_u32 j;
        for (j = 0; j < 9u; ++j) {
            if (slots[j] == i)
                expected = values[j];
        }
        if (output[i] != expected)
            return 2;
    }

    /* In-place source/output is safe because the full copy is staged first. */
    status = engine_table_clone_selected(source, source, slots, values, 1u);
    if (status != ENGINE_TABLE_CLONE_OK || source[89] != values[0])
        return 3;

    fill_source(source);
    source[0] = 0u;
    if (!expect_failure_without_output_change(ENGINE_TABLE_CLONE_BAD_SIGNATURE,
                                              source, slots, values, 1u))
        return 4;
    fill_source(source);
    slots[0] = 250u;
    if (!expect_failure_without_output_change(ENGINE_TABLE_CLONE_SLOT_OUT_OF_RANGE,
                                              source, slots, values, 1u))
        return 5;
    slots[0] = 88u;
    if (!expect_failure_without_output_change(ENGINE_TABLE_CLONE_SLOT_NOT_APPROVED,
                                              source, slots, values, 1u))
        return 6;
    slots[0] = 89u;
    slots[1] = 89u;
    if (!expect_failure_without_output_change(ENGINE_TABLE_CLONE_DUPLICATE_SLOT,
                                              source, slots, values, 2u))
        return 7;
    if (!expect_failure_without_output_change(ENGINE_TABLE_CLONE_TOO_MANY_REPLACEMENTS,
                                              source, slots, values, 10u))
        return 8;
    if (!expect_failure_without_output_change(ENGINE_TABLE_CLONE_INVALID_ARGUMENT,
                                              source, 0, values, 1u))
        return 9;

    puts("table clone contract tests passed");
    return 0;
}
