/* ENG1 resource completion.
 *
 * These RVA-named routines are behavioral source hypotheses from the local PC
 * ENG1 oracle. They use the existing typed copied-interface and resource data
 * views. No full capacity, historical source owner, or whole-image match is
 * claimed. Native signed row/category reads and unchecked addressing are kept.
 */
#include "../../../calibration/engine_interface.h"
#include "data_layouts.h"
#include "resource_runtime_data.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned long u32;

/* One separately observed byte at ENG1 RVA 0x70c50. */
extern u8 g_pending_resource_row_70c50;
/* Independently owned frame data cells / literal prefix. */
extern u32 g_frame_global_734f4;
extern long g_camera_x_70ca0;
extern u8 g_frame_transition_flag_5ce48;
extern u8 g_frame_setup_callback_58a60[64];

/* Existing genuine providers: keep each allocation, file, callback and
 * resource-rebind implementation at its measured source owner. */
extern unsigned long __cdecl engine_pool_compact(int pool_id);
extern void *__cdecl engine_pool_allocate(
    void **output_slot, unsigned long bytes, unsigned long flags);
extern int __cdecl engine_file_load_async(
    const char *prefix, const char *suffix, int sector_offset,
    void *destination, unsigned long bytes);
extern void __cdecl dispatch_engine_2c2d0(void *pointer);
extern void __cdecl dispatch_engine_2c280(void);
extern void __cdecl dispatch_engine_2c7f0(void *rectangle,
                                           const void *texels);
extern void __cdecl resource_rebuild_blob(void);
extern void __cdecl resource_rebuild_group_rows(void);
extern void __cdecl resource_select_group(int selector);

/* These three wrapper bodies are separate unresolved graph dependencies. Their
 * exact PC callees and argument words are retained; semantics/names are open. */
extern unsigned long __cdecl eng1_target_02c400(
    const char *prefix, const char *suffix);                  /* RVA 0x2c400 */
extern unsigned long __cdecl eng1_target_02c4f0(
    unsigned long row_word_000);                              /* RVA 0x2c4f0 */
extern unsigned long __cdecl eng1_target_02c480(
    void *row_buffer, int load_mode);                         /* RVA 0x2c480 */

void __cdecl dispatch_local_1fb90(void);
void __cdecl dispatch_local_1fc60(void);

extern void __cdecl resource_reclaim(int wait_for_compaction);

/* Native RVA 0x1f9e0. The first row word is unsigned; the following size word
 * is sign-extended and is also passed to the async file callback as a DWORD.
 * The allocation expression intentionally keeps the native 32-bit shift,
 * mask, and addition. The row/category expressions have no invented clamp. */
int __cdecl dispatch_local_1f9e0(int row, int load_mode)
{
    u8 *record;
    u8 *slot;
    int category;
    int slot_index;
    u16 sector_offset;
    s16 signed_byte_count;
    unsigned long allocation_bytes;
    ResourceCallbackContext *context;
    int level;
    const char *prefix;
    const char *suffix;

    if (row >= (int)(signed char)g_group_flag_70ecf)
        return 0;

    record = g_header_pointer_70d48 + row * 12;
    sector_offset = *(u16 *)(record + 0);
    signed_byte_count = *(s16 *)(record + 2);
    allocation_bytes =
        (((unsigned long)(long)signed_byte_count << 11) & 0xfffff800UL)
        + 0x1000UL;

    category = (int)(signed char)g_active_category_70cfe;
    slot_index = row + category * 8;
    slot = g_allocated_row_slots_71e40[slot_index];
    if (slot != 0)
        dispatch_engine_2c2d0(slot);

    /* The allocator owns this pointer cell and writes its result through it. */
    category = (int)(signed char)g_active_category_70cfe;
    slot_index = row + category * 8;
    engine_pool_allocate((void **)&g_allocated_row_slots_71e40[slot_index],
                         allocation_bytes, 0x20UL);

    category = (int)(signed char)g_active_category_70cfe;
    slot_index = row + category * 8;
    slot = g_allocated_row_slots_71e40[slot_index];
    if (slot == 0) {
        resource_reclaim(0);

        category = (int)(signed char)g_active_category_70cfe;
        slot_index = row + category * 8;
        engine_pool_allocate(
            (void **)&g_allocated_row_slots_71e40[slot_index],
            allocation_bytes, 0x20UL);

        category = (int)(signed char)g_active_category_70cfe;
        slot_index = row + category * 8;
        slot = g_allocated_row_slots_71e40[slot_index];
        if (slot == 0)
            return 0;
    }

    g_pending_resource_row_70c50 = (u8)row;
    if (load_mode == 0) {
        context = g_engine_interface.context_004;
        level = (int)(signed char)context->unknown_000[0];
        prefix = (const char *)eng1_descriptor_rows_5a690[level].pointer_000;
        suffix = (const char *)(g_frame_setup_callback_58a60 + 8);

        eng1_target_02c400(prefix, suffix);
        eng1_target_02c4f0((unsigned long)sector_offset);
        category = (int)(signed char)g_active_category_70cfe;
        slot_index = row + category * 8;
        slot = g_allocated_row_slots_71e40[slot_index];
        eng1_target_02c480(slot, load_mode);
        dispatch_engine_2c280();
        dispatch_local_1fb90();
        return 1;
    }

    context = g_engine_interface.context_004;
    context->load_complete = dispatch_local_1fb90;
    context = g_engine_interface.context_004;
    context->load_failed = dispatch_local_1fc60;

    category = (int)(signed char)g_active_category_70cfe;
    slot_index = row + category * 8;
    slot = g_allocated_row_slots_71e40[slot_index];
    context = g_engine_interface.context_004;
    level = (int)(signed char)context->unknown_000[0];
    prefix = (const char *)eng1_descriptor_rows_5a690[level].pointer_000;
    suffix = (const char *)(g_frame_setup_callback_58a60 + 8);
    engine_file_load_async(prefix, suffix, (int)sector_offset, slot,
                           (unsigned long)(long)signed_byte_count);
    return 1;
}

/* Native completion callback registered in ResourceCallbackContext +0xb0.
 * Frame displacement math uses 32-bit wrap, the signed row-word load, and the
 * live category/transition bytes. The four-by-eight free walk is an observed
 * loop, not evidence that the resource has no additional allocation slots. */
void __cdecl dispatch_local_1fb90(void)
{
    int row;
    int category;
    int row_index;
    int slot_index;
    int selector;
    s16 signed_delta;
    unsigned long delta_bits;
    unsigned long *actor_x;
    u8 *record;
    u8 *slot;

    if (g_frame_transition_flag_5ce48 != 0xffU) {
        row = (int)(signed char)g_active_row_70d01;
        record = g_header_pointer_70d48 + row * 12;
        signed_delta = *(s16 *)(record + 0x10);
        delta_bits = (unsigned long)(long)signed_delta << 6;
        g_camera_x_70ca0 -= delta_bits;
        actor_x = (unsigned long *)g_frame_global_734f4;
        *actor_x -= delta_bits << 16;

        selector = (int)(signed char)g_frame_transition_flag_5ce48;
        g_active_category_70cfe = (signed char)selector;
        resource_select_group(selector);
        selector = (int)(signed char)g_frame_transition_flag_5ce48;
        resource_select_group(selector);

        g_frame_transition_flag_5ce48 = 0xffU;
        for (category = 0; category < 4; ++category) {
            for (row_index = 0; row_index < 8; ++row_index) {
                slot_index = row_index + category * 8;
                slot = g_allocated_row_slots_71e40[slot_index];
                if (slot != 0)
                    dispatch_engine_2c2d0(slot);
            }
        }
    }

    g_active_row_70d01 = (signed char)g_pending_resource_row_70c50;
    resource_rebuild_group_rows();
}

/* Native failure callback at RVA 0x1fc60 calls free for the saved row's slot
 * without checking it and does not clear the owner cell. */
void __cdecl dispatch_local_1fc60(void)
{
    int category;
    int row;
    int slot_index;

    category = (int)(signed char)g_active_category_70cfe;
    row = (int)g_pending_resource_row_70c50;
    slot_index = row + category * 8;
    dispatch_engine_2c2d0(g_allocated_row_slots_71e40[slot_index]);
}

/* Native RVA 0x1fc90 ignores byte_count. It sends two root-relative spans
 * through the already-owned host texture callback, then returns the root
 * offset computed from the following DWORD. Stack words are preserved exactly;
 * original rectangle field names are unknown. */
unsigned long __cdecl dispatch_local_1fc90(unsigned long byte_count)
{
    u8 *root;
    u8 *field;
    u8 *texels;
    unsigned long relative;
    u16 rectangle[4];

    (void)byte_count;
    root = g_resource_root;
    field = root + 0x14;

    rectangle[0] = 0;
    rectangle[1] = 0;
    rectangle[2] = 0x300;
    rectangle[3] = 0x200;
    relative = *(u32 *)field;
    texels = field + ((relative >> 2) * 4UL);
    dispatch_engine_2c7f0(rectangle, texels);

    field += 4;
    rectangle[0] = 0x300;
    rectangle[1] = 0x100;
    rectangle[2] = 0x100;
    rectangle[3] = 0x100;
    relative = *(u32 *)field;
    texels = field + ((relative >> 2) * 4UL);
    dispatch_engine_2c7f0(rectangle, texels);

    field += 4;
    relative = *(u32 *)(field + 4);
    root = g_resource_root;
    return (unsigned long)field + relative - (unsigned long)root;
}
