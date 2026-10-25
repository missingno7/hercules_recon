#include <stdio.h>
#include <string.h>
#include "../src/shared/engine_interface.h"
#include "../src/eng1/data_layouts.h"
#include "../src/eng1/resource_runtime_data.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned long u32;

extern u8 g_pending_resource_row_70c50;
extern void __cdecl resource_reclaim(int);
extern int __cdecl dispatch_local_1f9e0(int, int);
extern void __cdecl dispatch_local_1fb90(void);
extern void __cdecl dispatch_local_1fc60(void);
extern unsigned long __cdecl dispatch_local_1fc90(unsigned long);

EngineInterface g_engine_interface;
Eng1FrameDescriptorRow24 eng1_descriptor_rows_5a690[12];
u8 g_frame_setup_callback_58a60[64];
u32 g_frame_global_734f4;
long g_camera_x_70ca0;
u8 g_frame_transition_flag_5ce48 = 0xffU;
signed char g_group_flag_70ecf;
signed char g_active_category_70cfe;
signed char g_active_row_70d01;
u8 *g_header_pointer_70d48;
u8 *g_resource_root;
u8 *g_allocated_row_slots_71e40[32];

static u8 row_table[24];
static u8 root_bytes[128];
static u8 root_replacement_bytes[128];
static ResourceCallbackContext context_storage;
static u8 prefix_text[] = "MAP_DIRECTORY";
static u8 old_buffer[8];
static u8 sync_buffer[8];
static u8 later_category_buffer[8];
static u8 async_old_buffer[8];
static u8 async_buffer[8];
static u32 actor_x_cell;
static int failures;
static int free_count;
static void *freed_values[64];
static int allocation_count;
static void **allocation_slots[8];
static unsigned long allocation_sizes[8];
static unsigned long allocation_flags[8];
static int allocation_failures;
static void *allocation_result;
static int compact_count;
static unsigned long compact_sequence[8];
static int compact_sequence_count;
static int compact_sequence_index;
static int rebuild_blob_count;
static int rebuild_rows_count;
static int select_count;
static int mutate_transition_on_first_select;
static int selected_values[8];
static int host_finish_count;
static int host_setup_count;
static int change_category_on_setup;
static int host_select_count;
static int host_load_count;
static unsigned long host_word_value;
static void *host_row_buffer;
static int host_load_mode;
static const char *host_prefix;
static const char *host_suffix;
static int async_sector;
static void *async_destination;
static unsigned long async_bytes;
static unsigned long upload_rectangles[2][4];
static const void *upload_texels[2];
static int upload_count;
static int change_root_on_second_upload;

#define CHECK(test) do { \
    if (!(test)) { \
        printf("check failed at line %d: %s\n", __LINE__, #test); \
        return __LINE__; \
    } \
} while (0)

static void put_u32(u8 *where, u32 value)
{
    *(u32 *)where = value;
}

static void put_u16(u8 *where, u16 value)
{
    *(u16 *)where = value;
}

static void put_s16(u8 *where, s16 value)
{
    *(s16 *)where = value;
}

static void reset_spies(void)
{
    failures = 0;
    free_count = 0;
    allocation_count = 0;
    allocation_failures = 0;
    allocation_result = 0;
    compact_count = 0;
    compact_sequence_count = 0;
    compact_sequence_index = 0;
    rebuild_blob_count = 0;
    rebuild_rows_count = 0;
    select_count = 0;
    mutate_transition_on_first_select = 0;
    host_finish_count = 0;
    host_setup_count = 0;
    change_category_on_setup = 0;
    host_select_count = 0;
    host_load_count = 0;
    upload_count = 0;
    change_root_on_second_upload = 0;
}

unsigned long __cdecl engine_pool_compact(int pool_id)
{
    unsigned long result;
    (void)pool_id;
    ++compact_count;
    if (compact_sequence_index < compact_sequence_count)
        result = compact_sequence[compact_sequence_index++];
    else
        result = 1;
    return result;
}

void *__cdecl engine_pool_allocate(void **output_slot,
                                  unsigned long bytes,
                                  unsigned long flags)
{
    if (allocation_count < 8) {
        allocation_slots[allocation_count] = output_slot;
        allocation_sizes[allocation_count] = bytes;
        allocation_flags[allocation_count] = flags;
    }
    ++allocation_count;
    if (allocation_failures != 0) {
        --allocation_failures;
        *output_slot = 0;
        return 0;
    }
    *output_slot = allocation_result;
    return allocation_result;
}

int __cdecl engine_file_load_async(const char *prefix, const char *suffix,
                                   int sector_offset, void *destination,
                                   unsigned long bytes)
{
    ++host_load_count;
    host_prefix = prefix;
    host_suffix = suffix;
    async_sector = sector_offset;
    async_destination = destination;
    async_bytes = bytes;
    return 0;
}

void __cdecl dispatch_engine_2c2d0(void *pointer)
{
    if (free_count < 64)
        freed_values[free_count] = pointer;
    ++free_count;
}

void __cdecl dispatch_engine_2c280(void)
{
    ++host_finish_count;
}

void __cdecl dispatch_engine_2c7f0(void *rectangle, const void *texels)
{
    int i;
    if (upload_count < 2) {
        for (i = 0; i < 4; ++i)
            upload_rectangles[upload_count][i] = ((u16 *)rectangle)[i];
        upload_texels[upload_count] = texels;
    }
    ++upload_count;
    if (change_root_on_second_upload && upload_count == 2)
        g_resource_root = root_replacement_bytes;
}

void __cdecl resource_rebuild_blob(void)
{
    ++rebuild_blob_count;
}

void __cdecl resource_rebuild_group_rows(void)
{
    ++rebuild_rows_count;
}

void __cdecl resource_select_group(int selector)
{
    if (select_count < 8)
        selected_values[select_count] = selector;
    ++select_count;
    if (mutate_transition_on_first_select && select_count == 1)
        g_frame_transition_flag_5ce48 = 2;
}

unsigned long __cdecl eng1_target_02c400(const char *prefix,
                                         const char *suffix)
{
    ++host_setup_count;
    host_prefix = prefix;
    host_suffix = suffix;
    if (change_category_on_setup)
        g_active_category_70cfe = 2;
    return 0;
}

unsigned long __cdecl eng1_target_02c4f0(unsigned long row_word_000)
{
    host_word_value = row_word_000;
    return 0;
}

unsigned long __cdecl eng1_target_02c480(void *row_buffer, int load_mode)
{
    ++host_select_count;
    host_row_buffer = row_buffer;
    host_load_mode = load_mode;
    return 0;
}

static void setup_context(void)
{
    memset(&context_storage, 0, sizeof(context_storage));
    context_storage.unknown_000[0] = 2;
    g_engine_interface.context_004 = &context_storage;
    eng1_descriptor_rows_5a690[2].pointer_000 = prefix_text;
    strcpy((char *)g_frame_setup_callback_58a60 + 8, "MAP_PSX.BIN");
}

static int test_root_postprocess(void)
{
    unsigned long result;

    memset(root_bytes, 0, sizeof(root_bytes));
    put_u32(root_bytes + 0x14, 0x20UL);
    put_u32(root_bytes + 0x18, 0x30UL);
    put_u32(root_bytes + 0x20, 0x30UL);
    g_resource_root = root_bytes;
    reset_spies();

    result = dispatch_local_1fc90(0xffffffffUL);
    CHECK(result == 0x4cUL);
    CHECK(upload_count == 2);
    CHECK(upload_texels[0] == root_bytes + 0x34);
    CHECK(upload_texels[1] == root_bytes + 0x48);
    CHECK(upload_rectangles[0][0] == 0 && upload_rectangles[0][1] == 0);
    CHECK(upload_rectangles[0][2] == 0x300 && upload_rectangles[0][3] == 0x200);
    CHECK(upload_rectangles[1][0] == 0x300 && upload_rectangles[1][1] == 0x100);
    CHECK(upload_rectangles[1][2] == 0x100 && upload_rectangles[1][3] == 0x100);

    /* The native routine retains the original root for both upload spans, then
     * reloads the owning root pointer before calculating its returned offset. */
    reset_spies();
    g_resource_root = root_bytes;
    change_root_on_second_upload = 1;
    result = dispatch_local_1fc90(1UL);
    CHECK(upload_texels[0] == root_bytes + 0x34);
    CHECK(upload_texels[1] == root_bytes + 0x48);
    CHECK(g_resource_root == root_replacement_bytes);
    CHECK(result == (unsigned long)(root_bytes + 0x4c) -
                    (unsigned long)root_replacement_bytes);
    return 0;
}

static int test_row_loader_and_callbacks(void)
{
    int result;
    int row_index;
    unsigned long expected_alloc;

    memset(row_table, 0, sizeof(row_table));
    memset(g_allocated_row_slots_71e40, 0, sizeof(g_allocated_row_slots_71e40));
    setup_context();
    reset_spies();
    g_header_pointer_70d48 = row_table;
    g_group_flag_70ecf = 2;
    g_active_category_70cfe = 1;
    g_active_row_70d01 = -1;
    g_frame_transition_flag_5ce48 = 0xffU;
    g_resource_root = 0;
    allocation_result = 0;
    allocation_failures = 2;
    g_pending_resource_row_70c50 = 0x55U;
    put_u16(row_table + 12, 0x1234U);
    put_s16(row_table + 14, 9);

    result = dispatch_local_1f9e0(2, 0);
    CHECK(result == 0);
    CHECK(allocation_count == 0);

    result = dispatch_local_1f9e0(1, 1);
    CHECK(result == 0);
    CHECK(allocation_count == 2 && compact_count == 1);
    CHECK(g_pending_resource_row_70c50 == 0x55U);
    CHECK(host_load_count == 0 && host_setup_count == 0 && host_select_count == 0);
    CHECK(context_storage.load_complete == 0 && context_storage.load_failed == 0);

    reset_spies();
    g_resource_root = root_bytes;
    g_allocated_row_slots_71e40[9] = old_buffer;
    g_allocated_row_slots_71e40[17] = later_category_buffer;
    allocation_result = sync_buffer;
    change_category_on_setup = 1;
    allocation_failures = 1;
    result = dispatch_local_1f9e0(1, 0);
    CHECK(result == 1);
    CHECK(allocation_count == 2);
    CHECK(allocation_slots[0] == (void **)&g_allocated_row_slots_71e40[9]);
    CHECK(allocation_slots[1] == (void **)&g_allocated_row_slots_71e40[9]);
    expected_alloc = ((((unsigned long)9UL << 11) & 0xfffff800UL) + 0x1000UL);
    CHECK(expected_alloc == 0x5800UL);
    CHECK(allocation_sizes[0] == expected_alloc && allocation_sizes[1] == expected_alloc);
    CHECK(allocation_flags[0] == 0x20UL && allocation_flags[1] == 0x20UL);
    CHECK(g_allocated_row_slots_71e40[9] == sync_buffer);
    CHECK(free_count == 1 && freed_values[0] == old_buffer);
    CHECK(compact_count == 1);
    CHECK(rebuild_blob_count == 1 && rebuild_rows_count == 2);
    CHECK(host_setup_count == 1 && host_select_count == 1 && host_finish_count == 1);
    CHECK(host_prefix == prefix_text);
    CHECK(host_suffix == (const char *)(g_frame_setup_callback_58a60 + 8));
    CHECK(host_word_value == 0x1234UL);
    CHECK(host_row_buffer == later_category_buffer && host_load_mode == 0);
    CHECK(g_active_category_70cfe == 2);
    CHECK(g_pending_resource_row_70c50 == 1);
    CHECK((signed char)g_active_row_70d01 == 1);
    CHECK(host_load_count == 0);

    memset(g_allocated_row_slots_71e40, 0, sizeof(g_allocated_row_slots_71e40));
    memset(row_table, 0, sizeof(row_table));
    reset_spies();
    g_group_flag_70ecf = 3;
    g_active_category_70cfe = 2;
    g_active_row_70d01 = 0;
    g_frame_transition_flag_5ce48 = 0xffU;
    g_resource_root = 0;
    allocation_result = async_buffer;
    put_u16(row_table + 12, 0x3456U);
    put_s16(row_table + 14, (s16)-3);

    result = dispatch_local_1f9e0(1, 1);
    CHECK(result == 1);
    CHECK(allocation_count == 1);
    CHECK(allocation_slots[0] == (void **)&g_allocated_row_slots_71e40[17]);
    CHECK(allocation_sizes[0] == 0xfffff800UL);
    CHECK(allocation_flags[0] == 0x20UL);
    CHECK(g_allocated_row_slots_71e40[17] == async_buffer);
    CHECK(host_load_count == 1);
    CHECK(host_prefix == prefix_text);
    CHECK(host_suffix == (const char *)(g_frame_setup_callback_58a60 + 8));
    CHECK(async_sector == 0x3456);
    CHECK(async_destination == async_buffer);
    CHECK(async_bytes == 0xfffffffdUL);
    CHECK(context_storage.load_complete == dispatch_local_1fb90);
    CHECK(context_storage.load_failed == dispatch_local_1fc60);
    CHECK(g_pending_resource_row_70c50 == 1);

    row_index = free_count;
    context_storage.load_failed();
    CHECK(free_count == row_index + 1);
    CHECK(freed_values[row_index] == async_buffer);
    CHECK(g_allocated_row_slots_71e40[17] == async_buffer);

    /* The transition callback shifts camera/actor coordinates by the signed
     * row-word at +0x10, selects the transitioned category twice, releases all
     * observed 4*8 slots, restores the queued row and rebuilds its row pointers. */
    reset_spies();
    put_s16(row_table + 0x10, 3);
    g_camera_x_70ca0 = 0x1000L;
    actor_x_cell = 0x01000000UL;
    g_frame_global_734f4 = (u32)&actor_x_cell;
    g_frame_transition_flag_5ce48 = 1;
    mutate_transition_on_first_select = 1;
    g_active_category_70cfe = 0;
    g_active_row_70d01 = 0;
    g_pending_resource_row_70c50 = 1;
    dispatch_local_1fb90();
    CHECK(g_camera_x_70ca0 == 0x0f40L);
    CHECK(actor_x_cell == 0x00400000UL);
    CHECK(select_count == 2 && selected_values[0] == 1 && selected_values[1] == 2);
    CHECK(g_active_category_70cfe == 1);
    CHECK(g_frame_transition_flag_5ce48 == 0xffU);
    CHECK(g_active_row_70d01 == 1);
    CHECK(free_count == 1 && freed_values[0] == async_buffer);
    CHECK(rebuild_rows_count == 1);
    return 0;
}

static int test_compaction_wait(void)
{
    reset_spies();
    g_resource_root = 0;
    compact_sequence[0] = 0;
    compact_sequence[1] = 0;
    compact_sequence[2] = 1;
    compact_sequence_count = 3;
    resource_reclaim(1);
    CHECK(compact_count == 3);
    CHECK(rebuild_blob_count == 0 && rebuild_rows_count == 0);
    return 0;
}

int main(void)
{
    int result;
    result = test_compaction_wait();
    if (result != 0) return result;
    result = test_root_postprocess();
    if (result != 0) return result;
    result = test_row_loader_and_callbacks();
    if (result != 0) return result;
    puts("ENG1 resource completion fixture passed");
    return 0;
}
