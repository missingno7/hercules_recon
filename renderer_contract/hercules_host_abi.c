#include "hercules_host_abi.h"

#include <string.h>

static int slot_is_approved(renderer_u32 slot)
{
    /* Interception candidates: submit +0x164, uploads +0x204/+0x208,
     * rectmove +0x20c, color +0x23c, and TITLE movie display +0x250. */
    if (slot == 89u || slot == 129u || slot == 130u || slot == 131u ||
        slot == 143u || slot == 148u)
        return 1;

    /* Shared services normally retained: chain init +0x144 and projection
     * state +0x2b8/+0x2bc. They are allowed only for explicit callers. */
    if (slot == 81u || slot == 174u || slot == 175u)
        return 1;

    /* Passive upstream point/projection observation. The caller must forward
     * the original callback and preserve its physical raw-EAX return. */
    if (slot == HERCULES_SLOT_POINT_INPUT || slot == HERCULES_SLOT_PROJECT ||
        slot == HERCULES_SLOT_SCREEN_OUTPUT)
        return 1;

    return 0;
}

int engine_table_clone_selected(const renderer_u32 *source_words,
                                renderer_u32 *output_words,
                                const renderer_u32 *slots,
                                const renderer_u32 *values,
                                renderer_u32 replacement_count)
{
    renderer_u32 scratch[ENGINE_TABLE_WORD_COUNT];
    renderer_u32 i;
    renderer_u32 j;

    if (source_words == 0 || output_words == 0)
        return ENGINE_TABLE_CLONE_INVALID_ARGUMENT;
    if (replacement_count > ENGINE_TABLE_MAX_REPLACEMENTS)
        return ENGINE_TABLE_CLONE_TOO_MANY_REPLACEMENTS;
    if (replacement_count != 0 && (slots == 0 || values == 0))
        return ENGINE_TABLE_CLONE_INVALID_ARGUMENT;
    if (source_words[0] != ENGINE_TABLE_SIGNATURE)
        return ENGINE_TABLE_CLONE_BAD_SIGNATURE;

    for (i = 0; i < replacement_count; ++i) {
        if (slots[i] >= ENGINE_TABLE_WORD_COUNT)
            return ENGINE_TABLE_CLONE_SLOT_OUT_OF_RANGE;
        if (!slot_is_approved(slots[i]))
            return ENGINE_TABLE_CLONE_SLOT_NOT_APPROVED;
        for (j = 0; j < i; ++j) {
            if (slots[i] == slots[j])
                return ENGINE_TABLE_CLONE_DUPLICATE_SLOT;
        }
    }

    memcpy(scratch, source_words, sizeof(scratch));
    for (i = 0; i < replacement_count; ++i)
        scratch[slots[i]] = values[i];
    memcpy(output_words, scratch, sizeof(scratch));
    return ENGINE_TABLE_CLONE_OK;
}
