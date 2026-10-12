/*
 * Private behavioral source for the ENG1 resource/frame startup helpers.
 *
 * The function bodies follow the native call order and physical widths. Names
 * are RVA-backed labels, not historical source names. The small stack-spin in
 * 0x1f750 is retained because the native routine waits for the host-updated
 * context dwords. Direct local dependencies without a current provider remain
 * explicitly named by RVA.
 */
#include "../../../calibration/engine_interface.h"
#include "data_layouts.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;
typedef short s16;

extern Eng1FrameDescriptorRow24 eng1_descriptor_rows_5a690[12];
extern u8 g_frame_setup_callback_58a60[64];

extern signed char g_root_count_70ed0;
extern signed char g_active_category_70cfe;
extern signed char g_active_row_70d01;
extern short g_selected_width_70d12;
extern signed char g_group_flag_70ecf;
extern u8 *g_header_pointer_70d48;
extern u8 *g_allocated_row_slots_71e40[32];
extern long g_camera_x_70ca0;
extern u8 *g_resource_root;
extern u32 g_frame_file_size_70d20;
extern u8 g_frame_transition_flag_5ce48;

extern unsigned long __cdecl engine_pool_compact(int pool_id);
extern unsigned long __cdecl engine_file_load_sync(
    const char *prefix, const char *suffix, void *destination,
    unsigned long bytes);
extern void *__cdecl engine_pool_allocate(
    void **output_slot, unsigned long bytes, unsigned long flags);

extern unsigned long __cdecl dispatch_engine_2cc30(unsigned long value);
extern void __cdecl dispatch_engine_2c630(unsigned long value);
extern void __cdecl dispatch_engine_2c5a0(unsigned long value);
extern void __cdecl dispatch_engine_2c2d0(void *pointer);
extern unsigned long __cdecl eng1_target_02c2a0(
    unsigned long first, unsigned long second);             /* RVA 0x2c2a0 */
extern void __cdecl eng1_target_02c4a0(
    void *first, unsigned long second,
    unsigned long third, unsigned long fourth);              /* RVA 0x2c4a0 */

extern void __cdecl resource_rebuild_blob(void);             /* RVA 0x20670 */
extern void __cdecl resource_select_group(int selector);      /* RVA 0x20830 */
extern void __cdecl resource_rebuild_group_rows(void);        /* RVA 0x208f0 */
extern unsigned long __cdecl eng1_target_01fc90(
    unsigned long byte_count);                               /* RVA 0x1fc90 */
extern unsigned long __cdecl eng1_target_01f9e0(
    int row, int load_mode);                                  /* RVA 0x1f9e0 */

void __cdecl dispatch_local_1f6f0(void)
{
    ResourceCallbackContext *context;
    unsigned long current;
    long difference;
    long remaining;
    unsigned long allocation_bytes;

    context = g_engine_interface.context_004;
    if (context->mode_09c == 1)
        return;

    current = dispatch_engine_2cc30(1UL);
    context = g_engine_interface.context_004;
    difference = (long)(current - context->unknown_054);
    remaining = 0x1c0L - difference;
    if (remaining <= 0)
        return;

    allocation_bytes = (unsigned long)remaining << 9;
    eng1_target_02c2a0(0UL, allocation_bytes);
    resource_rebuild_blob();
    resource_rebuild_group_rows();
    dispatch_engine_2cc30(1UL);
}

unsigned long __cdecl dispatch_local_1f750(void)
{
    ResourceCallbackContext *context;
    volatile ResourceCallbackContext *wait_context;
    const char *prefix;
    const char *suffix;
    u32 file_header[2];
    unsigned long file_bytes;
    unsigned long rounded_bytes;
    unsigned long loaded_bytes;
    int level;

    dispatch_engine_2c630(0UL);
    engine_pool_compact(0);

    context = g_engine_interface.context_004;
    level = (int)(s8)context->unknown_000[0];
    prefix = (const char *)eng1_descriptor_rows_5a690[level].pointer_000;
    suffix = (const char *)(g_frame_setup_callback_58a60 + 8);
    engine_file_load_sync(prefix, suffix, file_header, 8UL);

    file_bytes = file_header[1];
    g_frame_file_size_70d20 = file_bytes;
    rounded_bytes = (file_bytes & 0xfffff800UL) + 0x1000UL;
    if (engine_pool_allocate((void **)&g_resource_root,
                             rounded_bytes, 0UL) == 0)
        return 0xffffffffUL;

    context = g_engine_interface.context_004;
    context->unknown_128 |= 2;

    level = (int)(s8)context->unknown_000[0];
    prefix = (const char *)eng1_descriptor_rows_5a690[level].pointer_000;
    engine_file_load_sync(prefix, suffix, g_resource_root, file_bytes);

    context = g_engine_interface.context_004;
    *(u32 *)((u8 *)context + 0x90) = *(u32 *)g_resource_root;
    for (;;) {
        wait_context = *(ResourceCallbackContext * volatile *)&g_engine_interface.context_004;
        if (*(volatile u32 *)((volatile u8 *)wait_context + 0x90) ==
            wait_context->unknown_08c)
            break;
    }

    dispatch_engine_2c5a0(2UL);
    loaded_bytes = eng1_target_01fc90(file_bytes);
    dispatch_engine_2c630(0UL);
    eng1_target_02c4a0(g_resource_root, loaded_bytes, 0UL, 0UL);
    return 0UL;
}

void __cdecl dispatch_local_1f870(int force_update)
{
    ResourceCallbackContext *context;
    u8 *row;
    u8 *slot;
    int row_count;
    int row_index;
    int candidate_row;
    int active_row;
    int category;
    int x;
    int row_start;
    int row_end;
    int row_width;
    int row_height;
    int slot_index;
    int relative_end;

    if (force_update != 0) {
        context = g_engine_interface.context_004;
        if (context->mode_09c != 0 || context->blocked_0df != 0)
            return;
    }

    x = (int)(g_camera_x_70ca0 >> 6);
    row_count = (int)(s8)g_group_flag_70ecf;
    candidate_row = 0;
    row = g_header_pointer_70d48;

    for (row_index = 0; row_index < row_count; ++row_index) {
        row_start = (int)*(s16 *)(row + row_index * 12 + 4);
        row_height = (int)*(s16 *)(row + row_index * 12 + 6);
        row_width = (int)*(s16 *)(row + row_index * 12 + 8);
        row_end = row_start + row_width;
        if (x >= row_end)
            candidate_row = row_index;

        if (x < row_start || x > row_start + row_height) {
            active_row = (int)(s8)g_active_row_70d01;
            if (row_index != active_row) {
                category = (int)(s8)g_active_category_70cfe;
                slot_index = row_index + category * 8;
                slot = g_allocated_row_slots_71e40[slot_index];
                if (slot != 0)
                    dispatch_engine_2c2d0(slot);
            }
        }
    }

    if ((int)(s8)g_root_count_70ed0 > 0) {
        row = g_header_pointer_70d48 + candidate_row * 12;
        relative_end = (int)*(s16 *)(row + 0x0a);
        row_start = (int)*(s16 *)(row + 4);
        if (row_start + relative_end == (int)(s16)g_selected_width_70d12) {
            g_active_category_70cfe = (s8)(g_active_category_70cfe + 1);
            resource_select_group((int)g_active_category_70cfe);
            eng1_target_01f9e0(0, 1);
            g_active_category_70cfe = (s8)(g_active_category_70cfe - 1);
            resource_select_group((int)g_active_category_70cfe);
            g_frame_transition_flag_5ce48 = 1;
            return;
        }
    }

    category = (int)(s8)g_active_category_70cfe;
    slot_index = candidate_row + category * 8;
    slot = g_allocated_row_slots_71e40[slot_index];
    if (slot != 0) {
        g_active_row_70d01 = (s8)candidate_row;
        resource_rebuild_group_rows();
        return;
    }

    active_row = (int)(s8)g_active_row_70d01;
    if (active_row == -1)
        eng1_target_01f9e0(candidate_row, 0);
    else
        eng1_target_01f9e0(candidate_row, 1);
}
