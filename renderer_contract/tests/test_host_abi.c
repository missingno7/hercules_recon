#if defined(_M_IX86) || defined(__i386__)
#define HERCULES_RENDERER_LIVE_X86 1
#endif
#include "hercules_host_abi.h"
#include "hercules_surface.h"
#include "hercules_projection.h"
#ifdef HERCULES_RENDERER_LIVE_X86
typedef char callback_exchange_pointer_width[
    (sizeof(HerculesPrimaryCallbackExchange) == 4) ? 1 : -1];
typedef char host_lock_service_pointer_width[
    (sizeof(HerculesHostSurfaceLockService) == 4) ? 1 : -1];
typedef char host_unlock_service_pointer_width[
    (sizeof(HerculesHostSurfaceUnlockService) == 4) ? 1 : -1];
typedef char host_present_service_pointer_width[
    (sizeof(HerculesHostSurfacePresentService) == 4) ? 1 : -1];
typedef char host_projection_service_pointer_width[
    (sizeof(HerculesProjectionSlot106Callback) == 4) ? 1 : -1];
#endif

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
    HerculesHostSurfaceViewSnapshot view;
    renderer_u32 source[ENGINE_TABLE_WORD_COUNT];
    renderer_u32 output[ENGINE_TABLE_WORD_COUNT];
    renderer_u32 slots[16] = {89u, 129u, 130u, 131u, 143u, 148u, 81u,
                              174u, 175u, 101u, 106u, 114u, 102u, 107u, 117u, 89u};
    renderer_u32 values[16] = {0x11111111u, 0x22222222u, 0x33333333u,
                               0xaaaaaaaa, 0x44444444u, 0x55555555u,
                               0x66666666u, 0x77777777u, 0x88888888u,
                               0x99999999u, 0xbbbbbbbbu, 0xddddddddu,
                               0xeeeeeeeeu, 0x12345678u, 0x87654321u, 0xfedcba98u};
    renderer_u32 i;
    int status;

    view.active_pixel_address = (renderer_raw32)0x12345678u;
    view.pitch_bytes = (renderer_i32)336;
    view.lock_held = (renderer_u8)1;
    view.directdraw_mode = (renderer_u8)1;
    if (view.active_pixel_address != (renderer_raw32)0x12345678u ||
        view.pitch_bytes != 336 || view.lock_held != 1 ||
        view.directdraw_mode != 1)
        return 10;
    if (HERCULES_HOST_LOCK_STATE_VA != (renderer_u32)0x00ba01f0u ||
        HERCULES_HOST_DDRAW_MODE_VA != (renderer_u32)0x00ba020cu ||
        HERCULES_HOST_ACTIVE_PIXELS_VA != (renderer_u32)0x00ba0210u ||
        HERCULES_HOST_ACTIVE_PITCH_VA != (renderer_u32)0x00ba0214u)
        return 11;

    fill_source(source);
    fill_sentinel(output);
    if (HERCULES_SLOT_POINT_INPUT != 101u || HERCULES_SLOT_PROJECT != 106u ||
        HERCULES_SLOT_SCREEN_OUTPUT != 114u ||
        HERCULES_SLOT_BATCH_POINT_INPUT != 102u ||
        HERCULES_SLOT_BATCH_PROJECT != 107u || HERCULES_SLOT_FT4_SCREEN_COPY != 117u ||
        ENGINE_TABLE_MAX_REPLACEMENTS != 15u)
        return 12;
    status = engine_table_clone_selected(source, output, slots, values, 15u);
    if (status != ENGINE_TABLE_CLONE_OK)
        return 1;
    for (i = 0; i < ENGINE_TABLE_WORD_COUNT; ++i) {
        renderer_u32 expected = source[i];
        renderer_u32 j;
        for (j = 0; j < 15u; ++j) {
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
                                              source, slots, values, 16u))
        return 8;
    if (!expect_failure_without_output_change(ENGINE_TABLE_CLONE_INVALID_ARGUMENT,
                                              source, 0, values, 1u))
        return 9;

    puts("table clone contract tests passed");
    return 0;
}
