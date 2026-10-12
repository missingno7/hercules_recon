#include <stdio.h>
#include <string.h>
#include "../calibration/engine_interface.h"
#include "../src/eng1/behavioral/data_layouts.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;

extern void __cdecl dispatch_local_1f6f0(void);
extern unsigned long __cdecl dispatch_local_1f750(void);
extern void __cdecl dispatch_local_1f870(int);

EngineInterface g_engine_interface;
ResourceCallbackContext g_test_context;
Eng1FrameDescriptorRow24 eng1_descriptor_rows_5a690[12];
u8 g_frame_setup_callback_58a60[64] = {
    0, 0, 0, 0, 0x40, 1, 0, 2,
    'M', 'A', 'P', '_', 'P', 'S', 'X', '.', 'B', 'I', 'N', 0
};

signed char g_root_count_70ed0;
signed char g_active_category_70cfe;
signed char g_active_row_70d01;
short g_selected_width_70d12;
signed char g_group_flag_70ecf;
u8 *g_header_pointer_70d48;
u8 *g_allocated_row_slots_71e40[32];
long g_camera_x_70ca0;
u8 *g_resource_root;

extern u32 g_frame_file_size_70d20;
extern u8 g_frame_transition_flag_5ce48;
extern unsigned long g_frame_owner_list_73490;
extern unsigned long g_frame_resource_list_734e4;
extern unsigned long g_frame_actor_list_734f8;

static const char g_level_prefix[] = "LEVEL0";
static u8 g_resource_buffer[0x8000];
static u8 g_group_rows[24];
static u32 g_mock_header[2];
static u32 g_mock_root_word;
static unsigned long g_mock_clock;
static unsigned long g_mock_pool_bytes;
static unsigned long g_mock_pool_flags;
static int g_pool_should_fail;
static int g_pool_compact_calls;
static int g_sync_calls;
static int g_rebuild_blob_calls;
static int g_rebuild_rows_calls;
static int g_2c630_calls;
static unsigned long g_last_2c630_arg;
static int g_2c5a0_calls;
static unsigned long g_last_2c5a0_arg;
static int g_clock_calls;
static int g_2c2a0_calls;
static unsigned long g_last_2c2a0_first;
static unsigned long g_last_2c2a0_second;
static int g_2c4a0_calls;
static void *g_last_2c4a0_first;
static unsigned long g_last_2c4a0_second;
static unsigned long g_last_2c4a0_third;
static unsigned long g_last_2c4a0_fourth;
static int g_free_calls;
static void *g_last_freed;
static int g_select_calls;
static int g_selected_groups[4];
static int g_resize_calls;
static int g_last_resize_row;
static int g_last_resize_mode;

static void reset_call_counts(void)
{
    g_pool_compact_calls = 0;
    g_sync_calls = 0;
    g_rebuild_blob_calls = 0;
    g_rebuild_rows_calls = 0;
    g_2c630_calls = 0;
    g_last_2c630_arg = 0;
    g_2c5a0_calls = 0;
    g_last_2c5a0_arg = 0;
    g_clock_calls = 0;
    g_2c2a0_calls = 0;
    g_last_2c2a0_first = 0;
    g_last_2c2a0_second = 0;
    g_2c4a0_calls = 0;
    g_last_2c4a0_first = 0;
    g_last_2c4a0_second = 0;
    g_last_2c4a0_third = 0;
    g_last_2c4a0_fourth = 0;
    g_free_calls = 0;
    g_last_freed = 0;
    g_select_calls = 0;
    g_selected_groups[0] = 0;
    g_selected_groups[1] = 0;
    g_selected_groups[2] = 0;
    g_selected_groups[3] = 0;
    g_resize_calls = 0;
    g_last_resize_row = -1;
    g_last_resize_mode = -1;
}

static void put_s16(u8 *address, int value)
{
    *(short *)address = (short)value;
}

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            printf("check failed at line %d: %s\n", __LINE__, #condition); \
            return 1; \
        } \
    } while (0)

unsigned long __cdecl dispatch_engine_2cc30(unsigned long value)
{
    (void)value;
    ++g_clock_calls;
    return g_mock_clock;
}

void __cdecl dispatch_engine_2c630(unsigned long value)
{
    ++g_2c630_calls;
    g_last_2c630_arg = value;
}

void __cdecl dispatch_engine_2c5a0(unsigned long value)
{
    ++g_2c5a0_calls;
    g_last_2c5a0_arg = value;
}

void __cdecl dispatch_engine_2c2d0(void *pointer)
{
    ++g_free_calls;
    g_last_freed = pointer;
}

unsigned long __cdecl eng1_target_02c2a0(
    unsigned long first, unsigned long second)
{
    ++g_2c2a0_calls;
    g_last_2c2a0_first = first;
    g_last_2c2a0_second = second;
    return 0;
}

void __cdecl eng1_target_02c4a0(
    void *first, unsigned long second,
    unsigned long third, unsigned long fourth)
{
    ++g_2c4a0_calls;
    g_last_2c4a0_first = first;
    g_last_2c4a0_second = second;
    g_last_2c4a0_third = third;
    g_last_2c4a0_fourth = fourth;
}

void __cdecl resource_rebuild_blob(void)
{
    ++g_rebuild_blob_calls;
}

void __cdecl resource_select_group(int selector)
{
    if (g_select_calls < 4)
        g_selected_groups[g_select_calls] = selector;
    ++g_select_calls;
}

void __cdecl resource_rebuild_group_rows(void)
{
    ++g_rebuild_rows_calls;
}

unsigned long __cdecl eng1_target_01fc90(unsigned long byte_count)
{
    return byte_count - 3UL;
}

unsigned long __cdecl eng1_target_01f9e0(int row, int load_mode)
{
    ++g_resize_calls;
    g_last_resize_row = row;
    g_last_resize_mode = load_mode;
    return 1UL;
}

unsigned long __cdecl engine_pool_compact(int pool_id)
{
    (void)pool_id;
    ++g_pool_compact_calls;
    return 1UL;
}

unsigned long __cdecl engine_file_load_sync(
    const char *prefix, const char *suffix, void *destination,
    unsigned long bytes)
{
    ++g_sync_calls;
    if (g_sync_calls == 1) {
        if (strcmp(prefix, g_level_prefix) != 0 ||
            strcmp(suffix, "MAP_PSX.BIN") != 0 || bytes != 8UL)
            return 0UL;
        ((u32 *)destination)[0] = g_mock_header[0];
        ((u32 *)destination)[1] = g_mock_header[1];
        return 8UL;
    }

    if (prefix != g_level_prefix || suffix != (const char *)g_frame_setup_callback_58a60 + 8 ||
        destination != g_resource_root || bytes != g_mock_header[1])
        return 0UL;
    *(u32 *)destination = g_mock_root_word;
    return bytes;
}

void *__cdecl engine_pool_allocate(
    void **output_slot, unsigned long bytes, unsigned long flags)
{
    g_mock_pool_bytes = bytes;
    g_mock_pool_flags = flags;
    if (g_pool_should_fail) {
        *output_slot = 0;
        return 0;
    }
    *output_slot = g_resource_buffer;
    return g_resource_buffer;
}

int main(void)
{
    unsigned long result;

    g_engine_interface.context_004 = &g_test_context;
    g_test_context.unknown_000[0] = 0;
    eng1_descriptor_rows_5a690[0].pointer_000 = (void *)g_level_prefix;
    g_mock_header[0] = 0x12345678UL;
    g_mock_header[1] = 0x00001801UL;
    g_mock_root_word = 0x10203040UL;
    g_test_context.unknown_08c = g_mock_root_word;
    g_mock_clock = 200UL;

    CHECK(g_frame_file_size_70d20 == 0UL);
    CHECK(g_frame_owner_list_73490 == 0UL);
    CHECK(g_frame_resource_list_734e4 == 0UL);
    CHECK(g_frame_actor_list_734f8 == 0UL);
    CHECK(g_frame_transition_flag_5ce48 == 0xffU);

    reset_call_counts();
    g_test_context.mode_09c = 0;
    g_test_context.unknown_054 = 100UL;
    dispatch_local_1f6f0();
    CHECK(g_clock_calls == 2);
    CHECK(g_2c2a0_calls == 1);
    CHECK(g_last_2c2a0_first == 0UL);
    CHECK(g_last_2c2a0_second == 178176UL);
    CHECK(g_rebuild_blob_calls == 1);
    CHECK(g_rebuild_rows_calls == 1);

    reset_call_counts();
    g_test_context.mode_09c = 1;
    dispatch_local_1f6f0();
    CHECK(g_clock_calls == 0 && g_2c2a0_calls == 0);

    reset_call_counts();
    g_test_context.mode_09c = 0;
    g_mock_clock = 548UL;
    dispatch_local_1f6f0();
    CHECK(g_clock_calls == 1);
    CHECK(g_2c2a0_calls == 0);
    CHECK(g_rebuild_blob_calls == 0 && g_rebuild_rows_calls == 0);

    reset_call_counts();
    g_mock_clock = 200UL;
    g_test_context.unknown_054 = 100UL;
    g_test_context.unknown_08c = g_mock_root_word;
    g_test_context.unknown_128 = 0;
    g_resource_root = 0;
    g_pool_should_fail = 0;
    result = dispatch_local_1f750();
    CHECK(result == 0UL);
    CHECK(g_pool_compact_calls == 1);
    CHECK(g_sync_calls == 2);
    CHECK(g_frame_file_size_70d20 == 0x1801UL);
    CHECK(g_mock_pool_bytes == 0x2800UL && g_mock_pool_flags == 0UL);
    CHECK(g_resource_root == g_resource_buffer);
    CHECK((g_test_context.unknown_128 & 2) != 0);
    CHECK(*(u32 *)((u8 *)&g_test_context + 0x90) == g_mock_root_word);
    CHECK(g_2c5a0_calls == 1 && g_last_2c5a0_arg == 2UL);
    CHECK(g_2c630_calls == 2 && g_last_2c630_arg == 0UL);
    CHECK(g_2c4a0_calls == 1 && g_last_2c4a0_first == g_resource_buffer);
    CHECK(g_last_2c4a0_second == 0x17feUL);
    CHECK(g_last_2c4a0_third == 0UL && g_last_2c4a0_fourth == 0UL);

    reset_call_counts();
    g_resource_root = 0;
    g_pool_should_fail = 1;
    result = dispatch_local_1f750();
    CHECK(result == 0xffffffffUL);
    CHECK(g_sync_calls == 1 && g_2c5a0_calls == 0 && g_2c4a0_calls == 0);
    CHECK(g_resource_root == 0);
    g_pool_should_fail = 0;

    reset_call_counts();
    memset(&g_test_context, 0, sizeof(g_test_context));
    g_engine_interface.context_004 = &g_test_context;
    g_root_count_70ed0 = 1;
    g_group_flag_70ecf = 2;
    g_active_category_70cfe = 0;
    g_active_row_70d01 = 1;
    g_selected_width_70d12 = 500;
    g_camera_x_70ca0 = 40L * 64L;
    memset(g_group_rows, 0, sizeof(g_group_rows));
    put_s16(g_group_rows + 4, 10);
    put_s16(g_group_rows + 6, 10);
    put_s16(g_group_rows + 8, 10);
    put_s16(g_group_rows + 10, 1);
    put_s16(g_group_rows + 12 + 4, 30);
    put_s16(g_group_rows + 12 + 6, 10);
    put_s16(g_group_rows + 12 + 8, 10);
    put_s16(g_group_rows + 12 + 10, 1);
    g_header_pointer_70d48 = g_group_rows;
    g_allocated_row_slots_71e40[0] = g_resource_buffer;
    g_allocated_row_slots_71e40[1] = g_resource_buffer;
    dispatch_local_1f870(0);
    CHECK(g_free_calls == 1 && g_last_freed == g_resource_buffer);
    CHECK(g_rebuild_rows_calls == 1);
    CHECK(g_active_row_70d01 == 1);

    reset_call_counts();
    g_group_flag_70ecf = 1;
    g_camera_x_70ca0 = 12L * 64L;
    put_s16(g_group_rows + 4, 10);
    put_s16(g_group_rows + 6, 10);
    put_s16(g_group_rows + 8, 30);
    put_s16(g_group_rows + 10, 1);
    g_active_row_70d01 = -1;
    g_allocated_row_slots_71e40[0] = 0;
    dispatch_local_1f870(0);
    CHECK(g_resize_calls == 1);
    CHECK(g_last_resize_row == 0 && g_last_resize_mode == 0);

    reset_call_counts();
    g_active_row_70d01 = 0;
    dispatch_local_1f870(0);
    CHECK(g_resize_calls == 1);
    CHECK(g_last_resize_row == 0 && g_last_resize_mode == 1);

    reset_call_counts();
    put_s16(g_group_rows + 10, 40);
    g_selected_width_70d12 = 50;
    g_active_category_70cfe = 0;
    g_frame_transition_flag_5ce48 = 0;
    dispatch_local_1f870(0);
    CHECK(g_select_calls == 2);
    CHECK(g_selected_groups[0] == 1 && g_selected_groups[1] == 0);
    CHECK(g_resize_calls == 1 && g_last_resize_row == 0 && g_last_resize_mode == 1);
    CHECK(g_frame_transition_flag_5ce48 == 1);
    CHECK(g_active_category_70cfe == 0);

    reset_call_counts();
    g_group_flag_70ecf = 1;
    g_active_row_70d01 = -1;
    g_test_context.mode_09c = 1;
    dispatch_local_1f870(1);
    CHECK(g_free_calls == 0 && g_select_calls == 0 && g_resize_calls == 0);

    printf("startup frame helper and exact-cell fixture passed\n");
    return 0;
}
